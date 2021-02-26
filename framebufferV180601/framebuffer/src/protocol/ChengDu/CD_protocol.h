#ifndef __CD_PROTOCOL_H
#define __CD_PROTOCOL_H

#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "common.h"



#define PTC_DEBUG

#ifdef PTC_DEBUG
#define PTC_DEBUG_PRINTF	DEBUG_PRINTF
#define ptc_debug_printf 	debug_printf 
#else
#define PTC_DEBUG_PRINTF	
#define ptc_debug_printf 	
#endif




#define 	CD_PLAY_LST		sys_dir"/play.lst"

#define 	CD_BRIGHT_MIN	2

#define shift(A,N)					(A + (N))
#define CD_START_BYTE_POS			0
#define CD_DEVID_BYTE_POS			1
#define CD_CMD_BYTE_POS				3
#define CD_DATAS_BYTE_POS			5
#define CD_PARITY_BYTE_POS(N)		shift(CD_DATAS_BYTE_POS,N)
#define CD_END_BYTE_POS(N)			shift(CD_DATAS_BYTE_POS,N+2)



#define 	CD_SET_DEV_IP			0x3338
#define 	CD_GET_DEV_STATUS		0x3031
#define 	CD_RX_FILE				0x3130
#define 	CD_TX_FILE				0x3039
#define 	CD_SET_PLAY_LIST		0x3938
#define 	CD_GET_PLAY_CONTENT		0x3937
#define 	CD_SET_BRIGHT_MODE		0x3034
#define 	CD_SET_BRIGHT_VAL		0x3035
#define 	CD_GET_BRIGHT_MODE_VAL	0x3036
#define 	CD_SET_VIRT_CONNECT		0x3232
#define		CD_GET_VIRT_CONNECT		0x3231


#define  CD_BRIGHT_AUTO			0x30
#define  CD_BRIGHT_HAND			0x31



typedef struct __CDstruct
{
	user_t		*user;
	
	uint8_t 	StartByte;
	uint16_t 	DevAddr;
	uint16_t 	CmdCode;
	uint16_t 	CRC16;
	uint32_t 	DataLen;
	uint8_t 	*Data;
	uint8_t 	EndByte;
}CDPTCStruct_t;

extern char Playlst[8];


int CD_protocol_processor(user_t *user,uint8_t *input,uint32_t *inputlen);


#endif



