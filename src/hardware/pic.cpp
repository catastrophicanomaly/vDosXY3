#include "vDos.h"
#include "inout.h"
#include "callback.h"

extern void KEYBOARD_NextFromBuf();

static void write_command(Bitu port, Bitu val, Bitu iolen)
	{
		if (port == 0x20 && val == 0x20)
		{
			if (ISR & 2)
				KEYBOARD_NextFromBuf();
			ISR = 0;

		}
	}

static void write_data(Bitu port, Bitu val, Bitu iolen)
	{
	}

static Bitu read_command(Bitu port, Bitu iolen)
	{
	if (port == 0x20 && ISR)
		return ISR;
	return 0;
	}

static Bitu read_data(Bitu port, Bitu iolen)
	{
	return 0xf8;
	}

void PIC_Init()
	{
	IO_RegisterReadHandler(0x20, read_command);
	IO_RegisterReadHandler(0xa0, read_command);
	IO_RegisterWriteHandler(0x20, write_command);
	IO_RegisterWriteHandler(0xa0, write_command);
	IO_RegisterReadHandler(0x21, read_data);
	IO_RegisterReadHandler(0xa1, read_data);
	IO_RegisterWriteHandler(0x21, write_data);
	IO_RegisterWriteHandler(0xa1, write_data);
	}
