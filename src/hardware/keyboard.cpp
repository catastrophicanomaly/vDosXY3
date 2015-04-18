#include "vDos.h"
#include "inout.h"
#ifdef WITHIRQ1
#include "SDL_keysym.h"
#endif
#include <stdio.h>

static Bitu dummy_read(Bitu /*port*/, Bitu /*iolen*/)
	{
	return 0;
	}	

static void dummy_write(Bitu /*port*/, Bitu /*val*/, Bitu /*iolen*/)
	{
	}

static Bitu read_p64(Bitu port,Bitu iolen)
	{
	return 0x1d;																	// Just to get rid of of all these reads
	}

#ifdef WITHIRQ1
#define KEYBUFSIZE 32
#define KEYDELAY 0.300f			//Considering 20-30 khz serial clock and 11 bits/char
static struct {
	Bit8u buffer[KEYBUFSIZE];
	Bitu used;
	Bitu pos;
	
	
	Bit8u p60data;
	bool p60changed;
	bool active;
	bool scanning;
	bool scheduled;
} keyb;



static void KEYBOARD_SetPort60(Bit8u val) {
	keyb.p60changed = true;
	keyb.p60data = val;
	//PIC_ActivateIRQ(1);
	keyb_req = true;

}


void KEYBOARD_NextFromBuf()
{
	if (keyb.used && !keyb.p60changed)
	{
		Bit8u val = keyb.buffer[keyb.pos];
		if (++keyb.pos >= KEYBUFSIZE) keyb.pos -= KEYBUFSIZE;
		keyb.used--;
		KEYBOARD_SetPort60(val);
	}

}

static void KEYBOARD_AddBuffer(Bit8u data) {
	if (keyb.used >= KEYBUFSIZE) {
		//LOG(LOG_KEYBOARD, LOG_NORMAL)("Buffer full, dropping code");
		return;
	}
	Bitu start = keyb.pos + keyb.used;
	if (start >= KEYBUFSIZE) start -= KEYBUFSIZE;
	keyb.buffer[start] = data;
	keyb.used++;
	
	KEYBOARD_NextFromBuf();
		
}


static Bitu read_p60(Bitu port, Bitu iolen) {
	/*if (!keyb.p60changed)
		printf("Stale Read\n");
	else
		printf("Read\n");*/
	keyb.p60changed = false;
	keyb_req = false;
	Bitu res = keyb.p60data;
	
	//Moved to EOI processing
	//Otherwise XY3 got into trouble due to too fast changing input
	//It seems that the XY3 IRQ9 handler depends on reading the same value twice
	//KEYBOARD_NextFromBuf();
	return res;
}



void KEYBOARD_AddKey(SDLKey keytype, bool pressed) {

	Bit8u ret = 0; bool extend = false;
	switch (keytype) {
		
	case SDLK_LSUPER:
	case SDLK_RSUPER:
	case SDLK_MENU:
	case SDLK_HELP:
		return;
	case SDLK_ESCAPE:ret = 1; break;
	case SDLK_1:ret = 2; break;
	case SDLK_2:ret = 3; break;
	case SDLK_3:ret = 4; break;
	case SDLK_4:ret = 5; break;
	case SDLK_5:ret = 6; break;
	case SDLK_6:ret = 7; break;
	case SDLK_7:ret = 8; break;
	case SDLK_8:ret = 9; break;
	case SDLK_9:ret = 10; break;
	case SDLK_0:ret = 11; break;

	case SDLK_MINUS:ret = 12; break;
	case SDLK_EQUALS:ret = 13; break;
	
	case SDLK_BACKSPACE:ret = 14; break;
	case SDLK_TAB:ret = 15; break;
	

	case SDLK_q:ret = 16; break;
	case SDLK_w:ret = 17; break;
	case SDLK_e:ret = 18; break;
	case SDLK_r:ret = 19; break;
	case SDLK_t:ret = 20; break;
	case SDLK_y:ret = 21; break;
	case SDLK_u:ret = 22; break;
	case SDLK_i:ret = 23; break;
	case SDLK_o:ret = 24; break;
	case SDLK_p:ret = 25; break;

	
	case SDLK_LEFTBRACKET:ret = 26; break;
	case SDLK_RIGHTBRACKET:ret = 27; break;
	
	case SDLK_RETURN:ret = 28; break;
	
	case SDLK_LCTRL:ret = 29; break;
	
	case SDLK_a:ret = 30; break;
	case SDLK_s:ret = 31; break;
	case SDLK_d:ret = 32; break;
	case SDLK_f:ret = 33; break;
	case SDLK_g:ret = 34; break;
	case SDLK_h:ret = 35; break;
	case SDLK_j:ret = 36; break;
	case SDLK_k:ret = 37; break;
	case SDLK_l:ret = 38; break;
		
	case SDLK_SEMICOLON:ret = 39; break;
		
	case SDLK_QUOTE:ret = 40; break;
	
	case SDLK_BACKQUOTE:ret = 41; break;
	
	case SDLK_LSHIFT:ret = 42; break;
	case SDLK_BACKSLASH:ret = 43; break;

	case SDLK_z:ret = 44; break;
	case SDLK_x:ret = 45; break;
	case SDLK_c:ret = 46; break;
	case SDLK_v:ret = 47; break;
	case SDLK_b:ret = 48; break;
	case SDLK_n:ret = 49; break;
	case SDLK_m:ret = 50; break;
		
	case SDLK_COMMA:ret = 51; break;
	case SDLK_PERIOD:ret = 52; break;
	case SDLK_SLASH:ret = 53; break;
	case SDLK_RSHIFT:ret = 54; break;
	case SDLK_KP_MULTIPLY:ret = 55; break;
	case SDLK_LALT:ret = 56; break;
	
	case SDLK_SPACE:ret = 57; break;
	
	case SDLK_CAPSLOCK:ret = 58; break;

	case SDLK_F1:ret = 59; break;
	case SDLK_F2:ret = 60; break;
	case SDLK_F3:ret = 61; break;
	case SDLK_F4:ret = 62; break;
	case SDLK_F5:ret = 63; break;
	case SDLK_F6:ret = 64; break;
	case SDLK_F7:ret = 65; break;
	case SDLK_F8:ret = 66; break;
	case SDLK_F9:ret = 67; break;
	case SDLK_F10:ret = 68; break;

	case SDLK_NUMLOCK:ret = 69; break;
	case SDLK_SCROLLOCK:ret = 70; break;

	case SDLK_KP7:ret = 71; break;
	case SDLK_KP8:ret = 72; break;
	case SDLK_KP9:ret = 73; break;
	case SDLK_KP_MINUS:ret = 74; break;
	case SDLK_KP4:ret = 75; break;
	case SDLK_KP5:ret = 76; break;
	case SDLK_KP6:ret = 77; break;
	case SDLK_KP_PLUS:ret = 78; break;
	case SDLK_KP1:ret = 79; break;
	case SDLK_KP2:ret = 80; break;
	case SDLK_KP3:ret = 81; break;
	case SDLK_KP0:ret = 82; break;
	case SDLK_KP_PERIOD:ret = 83; break;

	case SDLK_LESS:ret = 86; break;

	case SDLK_F11:ret = 87; break;
	case SDLK_F12:ret = 88; break;

		//The Extended keys

	case SDLK_KP_ENTER:
		extend = true; 
		ret = 28; 
		break;
	case SDLK_RCTRL:extend = true; ret = 29; break;
	case SDLK_KP_DIVIDE:extend = true; ret = 53; break;
	case SDLK_RALT:extend = true; ret = 56; break;
	case SDLK_HOME:extend = true; ret = 71; break;
	case SDLK_UP:extend = true; ret = 72; break;
	case SDLK_PAGEUP:extend = true; ret = 73; break;
	case SDLK_LEFT:extend = true; ret = 75; break;
	case SDLK_RIGHT:extend = true; ret = 77; break;
	case SDLK_END:extend = true; ret = 79; break;
	case SDLK_DOWN:extend = true; ret = 80; break;
	case SDLK_PAGEDOWN:extend = true; ret = 81; break;
	case SDLK_INSERT:extend = true; ret = 82; break;
	case SDLK_DELETE:extend = true; ret = 83; break;
	case SDLK_PAUSE:
		KEYBOARD_AddBuffer(0xe1);
		KEYBOARD_AddBuffer(29 | (pressed ? 0 : 0x80));
		KEYBOARD_AddBuffer(69 | (pressed ? 0 : 0x80));
		return;
	case SDLK_PRINT:
		// Not handled yet. But usuable in mapper for special events 
		return;

	default:
		//E_Exit("Unsupported key press");
		keytype=(SDLKey)0; //just to be able to breakpoint
		return;
		break;
	}
	/* Add the actual key in the keyboard queue */
	if (pressed) {
		//if (keyb.repeat.key == keytype) keyb.repeat.wait = keyb.repeat.rate;
		//else keyb.repeat.wait = keyb.repeat.pause;
		//keyb.repeat.key = keytype;
		
	}
	else {
		//if (keyb.repeat.key == keytype) {
		//	/* repeated key being released */
		//	keyb.repeat.key = KBD_NONE;
		//	keyb.repeat.wait = 0;
		//}
		ret += 128;
	}
	if (extend) KEYBOARD_AddBuffer(0xe0);
	KEYBOARD_AddBuffer(ret);
}
#endif

#if BEEP

unsigned latchval = 100;
static void write_p61(Bitu port, Bitu val, Bitu iolen)
{
	if (ConfGetBool("beepxy3"))
		if (val & 3)
			Beep(1193182 / latchval, 50);


}

static void write_p42(Bitu port, Bitu val, Bitu iolen)
{
	static int lsb = 1;
	if (lsb)
		latchval = val;
	else
		latchval |= (val << 8);

	lsb ^= 1;
}

#endif


void KEYBOARD_Init()
	{
	IO_RegisterWriteHandler(0x60, dummy_write);


#ifdef BEEP
	IO_RegisterWriteHandler(0x61, write_p61);
	IO_RegisterWriteHandler(0x42, write_p42);
#else
	IO_RegisterWriteHandler(0x61, dummy_write);
#endif
	IO_RegisterWriteHandler(0x64, dummy_write);

#ifdef WITHIRQ1
	if (useIrq1)
		IO_RegisterReadHandler(0x60, read_p60);
	else
#endif
	IO_RegisterReadHandler(0x60, dummy_read);

	
	IO_RegisterReadHandler(0x61, dummy_read);
	IO_RegisterReadHandler(0x64, read_p64);
	}
