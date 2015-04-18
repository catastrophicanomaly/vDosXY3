#ifndef VDOS_H
#define VDOS_H

#include "config.h"
#include <io.h>
#include <windows.h>
#include "logging.h"

void E_Exit(const char * message,...);

void MSG_Init();																	// Set default (English) messages
void MSG_Add(const char*,const char*);												// Add messages to the internal langaugefile
const char* MSG_Get(char const *);													// Get messages from the internal langaugafile

extern char vDosVersion[];

void RunPC();
bool ConfGetBool(const char *name);
int ConfGetInt(const char *name);
char * ConfGetString(const char *name);
void ConfAddError(char* desc, char* errLine);
void vDos_Init(void);
void vDos_LoadConfig(void);

extern HWND	sdlHwnd;

#define MAX_PATH_LEN 512															// Maximum filename size

#define txtMaxCols	160
#define txtMaxLins	60

#define DOS_FILES 255

#ifdef WITHIRQ1
extern bool useIrq1;
#endif

extern int winHide10th;
extern bool winHidden;
extern DWORD hideWinTill;
extern bool usesMouse;
extern bool blinkCursor;
extern int wpVersion;
extern bool	mouseWP6x;
extern int wsVersion;
extern int wsBackGround;
extern int codepage;
extern bool printTimeout;
extern int eurAscii;

extern Bit8u initialvMode;

extern bool wpExclude;
extern int idleCount;

#define CPU_CycleLow	5000
#define CPU_CycleHigh	30000
extern Bit32s CPU_Cycles;
extern Bit32s CPU_CycleMax;
#define PIT_TICK_RATE 1193182

#define idleTrigger 5																// When to sleep

extern Bitu lastOpcode;

extern Bit8u ISR;																	// Interrupt Service Routine
extern bool keyb_req;

#endif
