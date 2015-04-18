#include <stdlib.h>
#include "vDos.h"
#include "video.h"
#include "cpu.h"
#include "callback.h"
#include "parport.h"
#include "support.h"
#include "bios.h"

#include "mouse.h"
#include "vga.h"
#include "paging.h"

char vDosVersion[] = "2015.04.10";

// The whole load of startups for all the subfunctions
void GUI_StartUp();
void MEM_Init();
void PAGING_Init();
void IO_Init();
void CALLBACK_Init();
void PROGRAMS_Init();
void VGA_Init();
void DOS_Init();
void CPU_Init();
void KEYBOARD_Init();																// TODO This should setup INT 16 too but ok ;)
void MOUSE_Init();
void SERIAL_Init(); 
void PIC_Init();
void TIMER_Init();
void BIOS_Init();
// void CMOS_Init();
void PARALLEL_Init();
// Dos Internal mostly
void XMS_Init();
void EMS_Init();
void SHELL_Init();
void INT10_Init();


#ifdef WITHIRQ1
bool useIrq1 = 0;																	// Should we route keypresses through Irq1/Int9, XY3 needs this
#endif

static Bit32u mSecsLast = 0;
int winHide10th = 0;
bool winHidden = true;
DWORD hideWinTill;
bool usesMouse;
bool blinkCursor;
int wpVersion;																		// 1 - 99 (mostly 51..62, negative value will exclude some WP additions)
bool mouseWP6x;																		// WP6.x with Mouse Driver (Absolute/Pen)
int wsVersion;																		// For now just 0 (no WordStar) or 1
int wsBackGround;																	// BackGround text color WordStar
Bit8u initialvMode = 3;																// Initial videomode, 3 = color, 7 = Hercules for as far it works
int codepage = 0;																	// Current code page, defaults to Windows OEM
bool printTimeout;																	// Should the spoolfile timeout?
int eurAscii = -1;																	// ASCII value to use for the EUro symbol, standard none

Bitu lastOpcode;

Bit32s CPU_Cycles =	0;
Bit32s CPU_CycleMax = CPU_CycleHigh;

static Bit32u prevWinRefresh;
#ifndef WITHIRQ1
bool ISR;
#else
Bit8u ISR;
bool keyb_req = false;
static bool kbPending = false;
#endif

static bool intPending = false;

void RunPC(void)
	{
	while (1)
		{
		while (CPU_Cycles > 0)
			{
			if (BIOS_HostTimeSync() && !ISR)
			{ 
				intPending = true;													// New timer tick
#ifdef WITHIRQ1
				kbPending = false;
#endif
			}
#ifdef WITHIRQ1
			else if (useIrq1 && !intPending && keyb_req && !ISR)
				kbPending=true;
#endif
			if (GETFLAG(IF))														// (hardware) Interrupts handled
				if (intPending)
					{
					intPending = false;
					if ((Mem_Lodsb(4*8+3)|Mem_Lodsb(4*0x1c+3)) != 0xf0)				// And Int 8 or 1C replaced
						{
						ISR = 1;
						CPU_HW_Interrupt(8);										// Setup executing Int 8 (IRQ0)
						}
					}
#ifdef WITHIRQ1
				else if (kbPending && Mem_Lodsb(4 * 9 + 3) != 0xf0)
					{
						
						kbPending = false;
						keyb_req = false;
						ISR = 2;
						CPU_HW_Interrupt(9);
					}
#endif
				else if (mouse_event_type)
					CPU_HW_Interrupt(0x74);											// Setup executing Int 74 (Mouse)
			Bits ret = (*cpudecoder)();
			if (ret < 0)
				return;
			if (ret > 0 && (*CallBack_Handlers[ret])())
				return;
			}
		GFX_Events();
		Bit32u mSecsNew = GetTickCount();
		if (mSecsNew >= prevWinRefresh+40)											// 25 refreshes per second
			{
			prevWinRefresh = mSecsNew;
			VGA_VerticalTimer();
			}
		if (mSecsNew <= mSecsLast+55)												// To be real save???
			{
			LPT_CheckTimeOuts(mSecsNew);
			Sleep(idleCount >= idleTrigger ? 2: 0);									// If idleTrigger or more repeated idle keyboard requests or int 28h called, sleep fixed (CPU usage drops down)
			if (idleCount > idleTrigger && CPU_CycleMax > CPU_CycleLow)
				CPU_CycleMax -= 100;												// Decrease cycles
			else if (idleCount <= idleTrigger  && CPU_CycleMax < CPU_CycleHigh)
				CPU_CycleMax += 1000;												// Fire up again
			}
		mSecsLast = mSecsNew;
		idleCount = 0;
		CPU_Cycles = CPU_CycleMax;
		}
	}


#define LENNAME 10																	// Max length of name
#define MAXNAMES 50																	// Max number of names
#define MAXSTRLEN 8192																// Max storage of all config strings

enum Vtype { V_BOOL, V_INT, V_STRING};

static int confEntries = 0;															// Entries so far
static char confStrings[MAXSTRLEN];
static unsigned int confStrOffset = 0;												// Offset to store new strings
static char errorMess[600];
static bool addMess = true;

static struct
{
char name[LENNAME+1];
Vtype type;
bool set;
union {bool _bool; int _int; char * _string;}value;
} ConfSetting[MAXNAMES];


// No checking on add/get functions, we call them ourselves...
void ConfAddBool(const char *name, bool value)
	{
	strcpy(ConfSetting[confEntries].name, name);
	ConfSetting[confEntries].type = V_BOOL;
	ConfSetting[confEntries].value._bool = value;
	confEntries++;
	}

void ConfAddInt(const char *name, int value)
	{
	strcpy(ConfSetting[confEntries].name, name);
	ConfSetting[confEntries].type = V_INT;
	ConfSetting[confEntries].value._int = value;
	confEntries++;
	}
	
void ConfAddString(const char *name, char* value)
	{
	strcpy(ConfSetting[confEntries].name, name);
	ConfSetting[confEntries].type = V_STRING;
	ConfSetting[confEntries].value._string = value;
	confEntries++;
	}

static int findEntry(const char *name)
	{
	for (int found = 0; found < confEntries; found++)
		if (!stricmp(name, ConfSetting[found].name))
			return found;
	return -1;
	}
	
bool ConfGetBool(const char *name)
	{
	int entry = findEntry(name);
	if (entry >= 0)
		return ConfSetting[entry].value._bool;
	return false;																	// To satisfy compiler and default
	}
int ConfGetInt(const char *name)
	{
	int entry = findEntry(name);
	if (entry >= 0)
		return ConfSetting[entry].value._int;
	return 0;																		// To satisfy compiler and default
	}
char * ConfGetString(const char *name)
	{
	int entry = findEntry(name);
	if (entry >= 0)
		return ConfSetting[entry].value._string;
	return "";																		// To satisfy compiler and default
	}
	
static char* ConfSetValue(const char* name, char* value)
	{
	int entry = findEntry(name);
	if (entry == -1)
		return "No valid config option\n";
	if (ConfSetting[entry].set)
		return "Config option already set\n";
	ConfSetting[entry].set = true;
	switch (ConfSetting[entry].type)
		{
	case V_BOOL:
		if (!stricmp(value, "on"))
			{
			ConfSetting[entry].value._bool = true;
			return NULL;
			}
		else if (!stricmp(value, "off"))
			{
			ConfSetting[entry].value._bool = false;
			return NULL;
			}
		break;
	case V_INT:
		{
		int testVal = atoi(value);
		char testStr[32];
		sprintf(testStr, "%d", testVal);
		if (!strcmp(value, testStr))
			{
			ConfSetting[entry].value._int = testVal;
			return NULL;
			}
		break;
		}
	case V_STRING:
		if (strlen(value) >= MAXSTRLEN - confStrOffset)
			return "vDos ran out of space to store settings\n";
		strcpy(confStrings+confStrOffset, value);
		ConfSetting[entry].value._string = confStrings+confStrOffset;
		confStrOffset += strlen(value)+1;
		return NULL;
		}
	return "Invalid value for this option\n";
	}

static char * ParseConfigLine(char *line)
	{
	char *val = strchr(line, '=');
	if (!val)
		return "= missing\n";
	*val = 0;
	val++;
	char *name = lrTrim(line);
	if (!strlen(name))
		return "Option name missing\n";
	val = lrTrim(val);
	if (!strlen(val))
		return "Value of option missing\n";
	if (strlen(name) == 4 && (!strnicmp(name, "LPT", 3) || !strnicmp(name, "COM", 3)) && (name[3] > '0' && name[3] <= '9'))
		{
		ConfAddString(name, val);
		ConfSetValue(name, val);													// Have to use this, ConfAddString() uses static ref to val!
		return NULL;
		}
	return ConfSetValue(name, val);
	}

void ConfAddError(char* desc, char* errLine)
	{
	if (addMess)
		{
		if (strlen(errLine) > 40)
			strcpy(errLine+37, "...");
		strcat(strcat(strcat(errorMess, "\n"), desc), errLine);
		if (strlen(errorMess) > 500)												// Don't flood the MesageBox with error lines
			{
			strcat(errorMess, "\n...");
			addMess = false;
			}
		}
	}

void ParseConfigFile()
	{
	char * parseRes;
	char lineIn[1024];
	FILE * cFile;
	errorMess[0] = 0;
	if (!(cFile = fopen("config.txt", "r")))
		return;
	while (fgets(lineIn, 1023, cFile))
		{
		char *line = lrTrim(lineIn);
		if (strlen(line) && !(!strnicmp(line, "rem", 3) && (line[3] == 0 || line[3] == 32 || line[3] == 9)))	// Filter out rem ...
			if (parseRes = ParseConfigLine(line))
				ConfAddError(parseRes, line);
		}
	fclose(cFile);

	
	}


void vDos_LoadConfig(void)
	{
#ifdef WITHIRQ1
	//Option to disable the full IRQ1 keyboard handling
	ConfAddBool("kbxy3", true);
#endif
	ConfAddInt("kbrepdel", 500); 
	ConfAddInt("kbrepinter", 10);
#ifdef BEEP
	//Option to disable the rudimentary sound support
	ConfAddBool("beepxy3", true);
#endif
#ifdef SFN83
	//Option to disable support for short (8.3) file names.
	ConfAddBool("sfn83", true);
#endif
	ConfAddInt("scale", 0);
	ConfAddString("window", "");

	// title and icon emendelson from rhenssel
	ConfAddString("title", "vDos");
	ConfAddString("icon", "vDos_ico");
	
	ConfAddBool("low", false);
	ConfAddString("xmem", "");
	ConfAddString("colors", "");
	ConfAddBool("mouse", false);
	ConfAddInt("lins", 25);
	ConfAddInt("cols", 80);
	ConfAddBool("frame", false);
	ConfAddBool("timeout", true);
	ConfAddString("font", "");
	ConfAddString("wp", "");
	ConfAddBool("blinkc", false);
	ConfAddInt("euro", -1);
	ParseConfigFile();
	}

void vDos_Init(void)
	{
	hideWinTill = GetTickCount()+2500;												// Auto hidden till first keyboard check, parachute at 2.5 secs

	LOG_MSG("vDos version: %s", vDosVersion);

#ifndef WITHIRQ1
	// Wil have been called earlier in starup if WITHIRQ1 is defined
	vDos_LoadConfig();
#endif

	GUI_StartUp();
	IO_Init();
	PAGING_Init();
	MEM_Init();
	CALLBACK_Init();
	PIC_Init();
	PROGRAMS_Init();
	TIMER_Init();
//	CMOS_Init();
	VGA_Init();
	CPU_Init();
	KEYBOARD_Init();
	BIOS_Init();
	INT10_Init();
	MOUSE_Init();
	SERIAL_Init();
	PARALLEL_Init();
	printTimeout = ConfGetBool("timeout");
	DOS_Init();
	XMS_Init();
	EMS_Init();
	if (errorMess[0])
		MessageBox(NULL, errorMess+1, "vDos: CONFIG.TXT has unresolved items", MB_OK|MB_ICONWARNING);
	SHELL_Init();																	// Start up main machine
	}
