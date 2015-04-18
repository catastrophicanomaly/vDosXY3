#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "shell.h"
#include "support.h"
#include <sys/types.h>
#include <sys/stat.h>

BatchFile::BatchFile(DOS_Shell * host, char const * const resolved_name, char const * const entered_name, char const * const cmd_line)
	{
	struct _stat statBuf;

	prev = host->bf;
	echo = host->echo;
	shell = host;
	location = 0;
	bfSize = 0;
	char totalname[DOS_PATHLENGTH+4];
	DOS_Canonicalize(resolved_name, totalname);										// Get fullname including drive specificiation
	char winPathName[512];
	if (!stricmp(totalname, "C:\\AUTOEXEC.BAT"))
		strcpy(winPathName, "autoexec.txt");
	else
		strcat(strcpy(winPathName,  Drives[*totalname-'A']->GetWinDir()), totalname+3);
	if (!_stat(winPathName, &statBuf))
		{
		if (statBuf.st_size > 65534)
			E_Exit("%s exceeds maximum batch file size of 64KB", resolved_name);
		if (!(bfText = (char *)malloc(statBuf.st_size+1)))
			E_Exit("Couldn't allocate memory to run batch file: %s", resolved_name);
		int fd;
		if(!_sopen_s(&fd, winPathName, _O_RDONLY | _O_TEXT, _SH_DENYNO, 0))
			{
			bfSize = _read(fd, bfText, statBuf.st_size);
			close(fd);
//			bfText[bfSize] = 0;
			if (bfSize == -1)
				bfSize = 0;
			}
		}
	else
		bfText = (char *)malloc(16);
	bfText[bfSize] = 0;
	cmd = new CommandLine(entered_name, cmd_line);
	}

BatchFile::~BatchFile()
	{
	free(bfText);
	delete cmd;
	shell->bf = prev;
	shell->echo = echo;
	}

bool BatchFile::ReadLine(char *line)
	{
	for (char *bfPtr = bfText+location; *bfPtr; bfPtr++)
		{
		char *lineStart = bfPtr;
		while (*bfPtr == ' ' || *bfPtr == 9)										// Skip leading spaces
			bfPtr++;
		char *charStart = bfPtr;
		while (*bfPtr && *bfPtr != '\n')
			bfPtr++;
		if (*charStart == ':')														// Assume label
			continue;
		if (bfPtr != charStart)
			{
			strncpy(line, lineStart, bfPtr-lineStart);
			line[bfPtr-lineStart] = 0;												// Add end-of-string
			while (*bfPtr == '\n')
				bfPtr++;
			location = bfPtr-bfText;												// Store current location and close bat file
			return true;	
			}
		}
	delete this;
	return false;	
	}

bool BatchFile::Goto(char *label)
	{
	for (char *bfPtr = bfText; *bfPtr;)
		{
		char *lineStart = bfPtr;
		while (*bfPtr == ' ' || *bfPtr == 9)										// Skip leading white-space in line
			bfPtr++;
		if (*bfPtr == ':')															// Labels start with ':'
			{
			bfPtr++;																// Skip ':'
			while (*bfPtr == ' ' || *bfPtr == 9 || *bfPtr == '=')					// Skip spaces and '='
				bfPtr++;
			int lblLen = strlen(label);
			if (!strnicmp(bfPtr, label, lblLen))									// First part matches
				if (strchr(" \t\n", bfPtr[lblLen]))									// Ends with white-space, EOL, EOF?
					{																// Found
					this->location = lineStart-bfText;
					return true;
					}
			}
		while (*bfPtr && *bfPtr != '\n')											// Skip to next line
			bfPtr++;
		while (*bfPtr == '\n')
			bfPtr++;
		}
	delete this;
	return false;	
	}

void BatchFile::Shift(void)
	{
	cmd->Shift(1);
	}
