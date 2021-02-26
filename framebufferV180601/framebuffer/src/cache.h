#ifndef __CACHE_H
#define __CACHE_H

#include "config.h"
#include <stdbool.h>

#define TYPE_STR		1
#define TYPE_IMG		2
#define TYPE_GIF		3
#define TYPE_LBD		4
#define TYPE_PNG		5




typedef struct 
{
	uint32_t pos_x;
	uint32_t pos_y;
}Coord_t;

typedef struct _CACHEstruct
{	
	uint8_t 		rd_pos;				//缓存读到第几块
	uint8_t 		wr_pos;				//缓存写到第几块
	uint8_t 		blockCNT;			//缓存共多少块
	uint8_t 		unreadable;

	uint16_t 		CHwidth;
	uint16_t 		CHheight;

	uint8_t 		RDpos;
	uint8_t 		WRpos;
	uint8_t 		W_Pos;
	uint8_t 		R_Pos;
	uint8_t 		RWflag[3];
	uint8_t 		clearLock;

	uint8_t 		*W_Addr;
	uint8_t 		*R_Addr;
	
	uint8_t 		*cache;		//解帧的目标缓存
	void 			*arg;
	int 			(*function)(struct _CACHEstruct *);

	bool 			(*empty)(struct _CACHEstruct *);
	bool			(*full)(struct _CACHEstruct *);
	uint8_t 		*(*readAddr)(struct _CACHEstruct *);
	uint8_t 		*(*writeAddr)(struct _CACHEstruct *);

	pthread_mutex_t RWlock;
}CACHEstruct_t;


typedef struct __Dspcache
{
	uint8_t 	Dcount;
	uint8_t 	Dnumber;

	uint8_t 	EFFin;
	uint8_t 	EFFout;
	uint32_t 	StopTime;
	
	uint16_t 	Dwidth;
	uint16_t 	Dheight;
	uint8_t 	*Dspbuf;
	struct __Dspcache *Dnext;
	struct __Dspcache *head;
	struct __Dspcache *tail;
}DSPCStruct_t;




 

extern CACHEstruct_t *DSPcache;
extern CACHEstruct_t *CTTcache;//(CACHEstruct_t *)malloc(sizeof(CACHEstruct_t));
extern DSPCStruct_t DSPHEAD;

void CACHE_INIStruct(CACHEstruct_t **CACHEstruct,int blkCNT,int Cwidth,int Cheight);
void Ca_INITDSPCStruct(uint8_t CNT);
#endif

