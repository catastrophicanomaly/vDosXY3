#ifndef VDOS_RENDER_H
#define VDOS_RENDER_H

#include "ttf.h"

// reduced to save some memory
#define RENDER_MAXWIDTH		800 
#define RENDER_MAXHEIGHT	600

typedef void (*ScalerLineHandler_t)(const void *src);

extern Bit8u rendererCache[];
extern void SimpleRenderer(const void *s);

typedef struct {
	struct { 
		Bit8u blue;
		Bit8u green;
		Bit8u red;
		Bit8u alpha;		// unused
	} rgb[256];
} RenderPal_t;

typedef struct {
	struct {
		bool invalid;
		bool nextInvalid;
		Bit8u *pointer;
		Bitu width, height;
		int start_y, past_y, curr_y;
	} cache;
	RenderPal_t pal;
	bool active;
} Render_t;

#define STYLE_STRIKEOUT	1;
#define STYLE_ITALIC	2;
#define STYLE_UNDERLINE	4;

typedef struct {
	TTF_Font *SDL_font;
	bool	vDos;								// is vDos.ttf loaded, pointsizes are preferred to be even to look really nice
	int		pointsize;
	int		height;								// height of character cell
	int		width;								// width
	int		cursor;
	int		lins;								// number of lines 24-60
	int		cols;								// number of columns 80-160
	bool	fullScrn;							// in fake fullscreen
	int		offX;								// horizontal offset to center content
	int		offY;								// vertical ,,
} Render_ttf;

extern Render_t render;
extern Render_ttf ttf;
extern ScalerLineHandler_t RENDER_DrawLine;
extern Bit16u curAttrChar[];					// currently displayed textpage
extern Bit16u * newAttrChar;					// to be replaced by

void RENDER_SetSize(Bitu width,Bitu height);
bool RENDER_StartUpdate(void);
void RENDER_EndUpdate(void);
void RENDER_ForceUpdate(void);

inline void RENDER_SetPal(Bit8u entry, Bit8u red, Bit8u green, Bit8u blue)
	{
	render.pal.rgb[entry].red = red;
	render.pal.rgb[entry].green = green;
	render.pal.rgb[entry].blue = blue;
//	render.pal.rgb[entry].alpha = 128;
	render.cache.nextInvalid = true;				// simply do a compleet cache update
	}

#endif
