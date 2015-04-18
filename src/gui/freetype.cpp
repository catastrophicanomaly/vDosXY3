#include <stdlib.h>
#include "logging.h"
#include "vDos.h"
#include "support.h"
#include "ttf.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define NUM_GRAYS	256

// Handy routines for converting from fixed point
#define FT_FLOOR(X)	(X>>6)
#define FT_CEIL(X)	((X+63)>>6)

// Cached glyph information
typedef struct cached_glyph
	{
	FT_Bitmap pixmap;
	int minx;
	int maxx;
	int miny;
	int maxy;
	int yoffset;
	} c_glyph;

// The structure used to hold internal font information
struct _TTF_Font
	{
	FT_Face face;

	int height;
	int width;
	int ascent;
	int descent;
	int underline_offset;
	int underline_height;

	c_glyph cache[256];																// Cached glyphs
	};

static FT_Library library;															// The FreeType font engine/library
static bool TTF_initialized = false;

void TTF_Init(void)
	{
	if (!TTF_initialized && FT_Init_FreeType(&library))
		E_Exit("TTF: Couldn't init FreeType engine");
	TTF_initialized = true;
	}

void TTF_Flush_Cache(TTF_Font* font)
	{
	for (int i = 0; i < 256; ++i)
		if (font->cache[i].pixmap.buffer)
			{
			free(font->cache[i].pixmap.buffer);
			font->cache[i].pixmap.buffer = 0;
			}	
	}

void TTF_SetCharSize(TTF_Font* font, int ptsize)
	{
	TTF_Flush_Cache(font);
	FT_Face face = font->face;
	if (FT_Set_Char_Size(face, 0, ptsize*64, 0, 0))									// Set the character size and use default DPI (72)
		E_Exit("TTF: Couldn't set font size");
	// Get the scalable font metrics for this font
	FT_Fixed scale = face->size->metrics.y_scale;
	font->ascent  = FT_CEIL(FT_MulFix(face->ascender, scale));
	font->descent = FT_CEIL(FT_MulFix(face->descender, scale));
	font->height  = font->ascent-font->descent;
	font->underline_offset = FT_FLOOR(FT_MulFix(face->underline_position, scale));
	font->underline_height = FT_FLOOR(FT_MulFix(face->underline_thickness, scale));
	if (font->underline_height < 1)
		font->underline_height = 1;
	font->width = FT_FLOOR(FT_MulFix(face->max_advance_width, face->size->metrics.x_scale));
	}

TTF_Font* TTF_New_Memory_Face(const FT_Byte* file_base, FT_Long file_size, int ptsize)
	{
	TTF_Font *font = (TTF_Font*)malloc(sizeof *font);
	if (font == NULL)
		E_Exit("TTF: Out of memory");
	memset(font, 0, sizeof(*font));

	if (FT_New_Memory_Face(library, file_base, file_size, 0, &font->face))
		E_Exit("TTF: Couldn't init font");
	FT_Face face = font->face;
	if (!FT_IS_SCALABLE(face))														// Make sure that our font face is scalable (global metrics)
		E_Exit("TTF: Font is not scalable");

	for (int i = 0; i < face->num_charmaps; i++)									// Set charmap for loaded font
		{
		FT_CharMap charmap = face->charmaps[i];
		if (charmap->platform_id == 3 && charmap->encoding_id == 1)					// Windows Unicode
			{
			FT_Set_Charmap(face, charmap);
			break;
			}
		}
 
	TTF_SetCharSize(font, ptsize);
	bool fontOK = false;
	if (!FT_Load_Glyph(face, 0, FT_LOAD_DEFAULT))									// Test pixel mode
		if (!FT_Render_Glyph(font->face->glyph, FT_RENDER_MODE_NORMAL))				// Render the glyph
			if (font->face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
				fontOK = true;
	if (!fontOK)
		E_Exit("TTF: Font is not 8 bits gray scale");
	return font;
	}

static c_glyph* LoadGlyph(TTF_Font* font, Uint8 ch)
	{
	c_glyph* cached = &font->cache[ch];
	if (cached->pixmap.buffer)														// If already cached
		return cached;

	FT_Face face = font->face;
	if (FT_Load_Glyph(face, FT_Get_Char_Index(face, cpMap[ch]), FT_LOAD_NO_AUTOHINT))
		return 0;

	FT_GlyphSlot glyph = face->glyph;
	FT_Glyph_Metrics* metrics = &glyph->metrics;

	cached->minx = FT_FLOOR(metrics->horiBearingX);
	cached->maxx = FT_CEIL(metrics->horiBearingX+metrics->width);
	cached->maxy = FT_FLOOR(metrics->horiBearingY);
	cached->miny = cached->maxy-FT_CEIL(metrics->height);
	cached->yoffset = font->ascent-cached->maxy;

	if (FT_Render_Glyph(glyph, FT_RENDER_MODE_NORMAL))								// Render the glyph
		return 0;

	FT_Bitmap* src = &glyph->bitmap;												// Copy information to cache
	FT_Bitmap* dst = &cached->pixmap;
	memcpy(dst, src, sizeof(*dst));
	if (dst->rows != 0)
		{
		int len = dst->pitch*dst->rows;
		dst->buffer = (unsigned char *)malloc(len);
		if (!dst->buffer)
			return 0;
		memcpy(dst->buffer, src->buffer, len);
		}
	return cached;
	}

void TTF_CloseFont(TTF_Font* font)
	{
	if (font)
		{
		TTF_Flush_Cache(font);
		if (font->face)
			FT_Done_Face(font->face);
		free(font);
		}
	}

int TTF_FontHeight(const TTF_Font *font)
	{
	return(font->height);
	}

int TTF_GlyphIsProvided(const TTF_Font *font, Uint16 ch)
	{
	return(FT_Get_Char_Index(font->face, ch));
	}

int TTF_FontWidth(TTF_Font *font)
	{
	return font->width;
	}

SDL_Surface* TTF_RenderASCII(TTF_Font* font, const char* text, int textlen, SDL_Color fg, SDL_Color bg, int style)
	{
	if (textlen <= 0)
		E_Exit("TTF: Draw text called with invalid length");
	SDL_Surface* surface = SDL_CreateRGBSurface(SDL_SWSURFACE, textlen*font->width, font->height, 8, 0, 0, 0, 0);	// Create the target surface
	if (surface == NULL)
		E_Exit("TTF: Draw string couldn't create surface");
	
	SDL_Palette* palette = surface->format->palette;								// Fill the palette with NUM_GRAYS levels of shading from bg to fg
	int rdiff = fg.r-bg.r;
	int gdiff = fg.g-bg.g;
	int bdiff = fg.b-bg.b;

	for (int i = 0; i < NUM_GRAYS; ++i)
		{
		palette->colors[i].r = bg.r+(i*rdiff)/(NUM_GRAYS-1);
		palette->colors[i].g = bg.g+(i*gdiff)/(NUM_GRAYS-1);
		palette->colors[i].b = bg.b+(i*bdiff)/(NUM_GRAYS-1);
		}

	int prevChar = 256;
	for (int xPos = 0; xPos < textlen; xPos++)										// Load and render each character
		{
		if (text[xPos] == prevChar)													// If it's the same as the previous, just copy
			{																		// Could be futher optimzed counting repeated characters
			Uint8* dst = (Uint8*)surface->pixels+xPos*font->width;					// and using a "smart" copy procedure
			for (int row = 0; row < font->height; row++)							// but updating the window is no hotspot
				{
				memcpy(dst, dst-font->width, font->width);
				dst += surface->pitch;
				}
			}
		else
			{
			prevChar = text[xPos];
			c_glyph *glyph = LoadGlyph(font, prevChar);
			if (!glyph)
				E_Exit("TTF: Couldn't load glyph");
			FT_Bitmap* pixmap = &glyph->pixmap;
			int width = pixmap->width;
			if (width > font->width)
				width = font->width;
			int xstart = xPos*font->width;

			for (int row = 0; row < pixmap->rows; ++row)
				{
				if (row+glyph->yoffset < 0 || row+glyph->yoffset >= surface->h)		// Make sure we don't go either over, or under the limit
					continue;
				Uint8* dst = (Uint8*)surface->pixels+(row+glyph->yoffset)*surface->pitch+xstart+glyph->minx;
				Uint8* src = pixmap->buffer+row*pixmap->pitch;
				for (int col = width; col > 0; --col)
					*dst++ |= *src++;
				}
			}
		}
	if (style&TTF_STYLE_UNDERLINE)													// Add underline
		memset((Uint8 *)surface->pixels+(font->ascent-font->underline_offset-1)*surface->pitch, NUM_GRAYS-1, font->underline_height*surface->pitch);
	if (style&TTF_STYLE_STRIKETHROUGH)												// Add strikethrough
		memset((Uint8 *)surface->pixels+(font->height/2)*surface->pitch, NUM_GRAYS-1, font->underline_height*surface->pitch);
	return surface;
	}

void TTF_Quit(void)
	{
	if (TTF_initialized)
		FT_Done_FreeType(library);
	}
