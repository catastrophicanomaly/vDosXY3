#include "vDos.h"
#include "video.h"
#include "render.h"
#include "vga.h"
#include "..\ints\int10.h"

static bool inReset = false;
static bool doWinRefresh = false;

void VGA_VerticalTimer()
	{
	if (!doWinRefresh)
		return;
	vga.draw.vertRetrace = !vga.draw.vertRetrace;									// So it's just half the times
	inReset = false;

	if (vga.mode == M_TEXT)
		{
		if (GFX_StartUpdate())
			{
			vga.draw.cursor.address = vga.config.cursor_start*2;
			newAttrChar = (Bit16u *)(MemBase+((CurMode->mode) == 7 ? 0xB0000 : 0xb8000));	// Pointer to chars+attribs
			GFX_EndUpdate();
			}
		return;
		}
	if (!RENDER_StartUpdate())														// Check if we can actually render, else skip the rest
		return;

	Bitu drawAddress = 0;
	for (Bitu lin = vga.draw.height; lin; lin--) 
		{
		RENDER_DrawLine(&vga.fastmem[drawAddress]);
		drawAddress += vga.config.scan_len*16;
		}
	RENDER_EndUpdate();
	}

void VGA_ResetVertTimer(bool delay)													// Trial to sync keyboard with screen (delay: don't update, for pasting keys)
	{
	if (!doWinRefresh)
		return;
	if (!delay)
		if (inReset)
			VGA_VerticalTimer();
	inReset = true;
	}

void VGA_StartResize(void)
	{
	Bitu width = vga.crtc.horizontal_display_end+1;
	Bitu height = (vga.crtc.vertical_display_end|((vga.crtc.overflow & 2)<<7)|((vga.crtc.overflow & 0x40) << 3))+1; 

	width *= vga.mode == M_TEXT ? 9 : 8;
	if ((width != vga.draw.width) || (height != vga.draw.height))					// Need to resize the output window?
		{
		vga.draw.width = width;
		vga.draw.height = height;
		if (width >= 640 && height >= 350)
			RENDER_SetSize(width, height);
		}
	doWinRefresh = true;
	}
