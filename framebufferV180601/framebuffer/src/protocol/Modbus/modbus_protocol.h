#ifndef __MODBUS_PROTOCOL_H
#define __MODBUS_PROTOCOL_H
#include "config.h"
#include "common.h"
#include "debug.h"
#include "modbus_config.h"
//#define MDBS_PROTOCOL_DEBUG


#ifdef MDBS_PROTOCOL_DEBUG
#define MDBS_PTCPARSEPRINTF			DEBUG_PRINTF
#define MDBS_protoparse_printf		debug_printf
#else
#define MDBS_PTCPARSEPRINTF	
#define MDBS_protoparse_printf	
#endif


#define VIRTUAL_CLOSE		0
#define VIRTUAL_OPEN		1



//#define MODBUS_ASCII	
//#define MODBUS_RTU
#define MODBUS_TCPIP

#define MDBUS_BRIGHT_AUTO	0
#define MDBUS_BRIGHT_HAND	1



#define DEV_ADDR_POS	0x00
#define FUNC_CODE_POS	0x01
#define	DATA_POS		0x02


#define	DISPLAY_0X1500  				0x1500
#define SETAUTOCHECKTIME_0X1005			0x1005
#define SETBRIGHT_0X1002				0x1002
#define SETCURTIME_0X1009				0x1009
#define STARTCHECKSELF_0X1100			0x1100
#define SETLIGHTBAND_0X1700				0x1700
#define SETPIXELSTRCSTATUS_0X1B00		0x1B00
#define SETPIXELSDSPSTATUS_0X2000		0x2000

typedef struct __Modbus
{
	user_t 	*user;
	uint8_t *TCPhead;
	
	uint8_t DevAddr;
	uint8_t FunCode;
	
	uint32_t dataLen;
	uint8_t *data;

	uint16_t CheckOut;
}MODBSStruct_t;
typedef struct tagBITMAPFILEHEADER {
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
} BITMAPFILEHEADER;
typedef struct tagBITMAPINFOHEADER{
	DWORD      biSize;
	long       biWidth;
	long       biHeight;
	WORD       biPlanes;
	WORD       biBitCount;
	DWORD      biCompression;
	DWORD      biSizeImage;
	long       biXPelsPerMeter;
	long       biYPelsPerMeter;
	DWORD      biClrUsed;
	DWORD      biClrImportant;
} BITMAPINFOHEADER;


int modbs_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen);
void Mdbs_timerInit(void);


#endif
