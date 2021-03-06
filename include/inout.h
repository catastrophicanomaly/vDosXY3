#ifndef vDOS_INOUT_H
#define vDOS_INOUT_H

#define IO_MAX (64*1024+3)

#define IO_MB	0x1
#define IO_MW	0x2
#define IO_MD	0x4
#define IO_MA	(IO_MB | IO_MW | IO_MD )

typedef Bitu IO_ReadHandler(Bitu port, Bitu iolen);
typedef void IO_WriteHandler(Bitu port, Bitu val, Bitu iolen);

void IO_RegisterReadHandler(Bitu port, IO_ReadHandler * handler);
void IO_RegisterWriteHandler(Bitu port, IO_WriteHandler * handler);

void IO_FreeReadHandler(Bitu port, Bitu mask, Bitu range = 1);
void IO_FreeWriteHandler(Bitu port, Bitu mask, Bitu range = 1);

void IO_WriteB(Bitu port, Bitu val);
void IO_WriteW(Bitu port, Bitu val);
void IO_WriteD(Bitu port, Bitu val);

Bitu IO_ReadB(Bitu port);
Bitu IO_ReadW(Bitu port);
Bitu IO_ReadD(Bitu port);

// Classes to manage the IO objects created by the various devices.
class IO_Base
	{
protected:
	Bitu m_port;
	};

class IO_ReadHandleObject: private IO_Base
	{
public:
	void Install(Bitu port, IO_ReadHandler * handler);
	};

class IO_WriteHandleObject: private IO_Base
	{
public:
	void Install(Bitu port, IO_WriteHandler * handler);
	};

static void IO_Write(Bitu port, Bit8u val)
	{
	IO_WriteB(port, val);
	}

static Bit8u IO_Read(Bitu port)
	{
	return (Bit8u)IO_ReadB(port);
	}

#endif
