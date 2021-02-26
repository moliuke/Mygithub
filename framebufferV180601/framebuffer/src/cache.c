#include <stdio.h>
#include <stdbool.h>
#include "cache.h"
#include "debug.h"
#include "config.h"
#include "Hardware/Data_pool.h"

CACHEstruct_t *DSPcache = NULL;
CACHEstruct_t *CTTcache = NULL;//(CACHEstruct_t *)malloc(sizeof(CACHEstruct_t));

DSPCStruct_t DSPHEAD;




//写位置追追上读位置，缓存处于满状态
static bool cache_full(CACHEstruct_t *CACHEstruct)
{
	//debug_printf("cache->wr_pos = %d,cache->rd_pos = %d\n",cache->wr_pos,cache->rd_pos);
	return ((CACHEstruct->wr_pos + 1) % CACHEstruct->blockCNT == CACHEstruct->rd_pos);
}

//当读位置追上写位置，缓存处于空状态
static bool cache_empty(CACHEstruct_t *CACHEstruct)
{
	//debug_printf("cache_writeBusy : cache->rd_pos = %d,cache->wr_pos = %d\n",cache->rd_pos,cache->wr_pos);
	return ((CACHEstruct->rd_pos + 1) % CACHEstruct->blockCNT == CACHEstruct->wr_pos);
}


uint8_t *cache_WriteAddr(CACHEstruct_t *CACHEstruct)
{
	uint8_t *retAddr = CACHEstruct->cache;
	uint32_t unit_block_size = CACHEstruct->CHwidth * CACHEstruct->CHheight * SCREEN_BPP;		//每块缓存大小

	#if 1
	if(cache_full(CACHEstruct) == true)
		return  NULL;
	
	CACHEstruct->wr_pos = (CACHEstruct->wr_pos + 1) % CACHEstruct->blockCNT;
	CACHEstruct->W_Addr = retAddr + unit_block_size * CACHEstruct->wr_pos;
	#else
	CACHEstruct->W_Addr = retAddr + unit_block_size * CACHEstruct->wr_pos;
	if(cache_full(CACHEstruct) != true)
		CACHEstruct->wr_pos = (CACHEstruct->wr_pos + 1) % CACHEstruct->blockCNT;
	#endif
	return CACHEstruct->W_Addr;
	
}


uint8_t *cache_ReadAddr(CACHEstruct_t *CACHEstruct)
{
	uint8_t *retAddr = CACHEstruct->cache;
	uint32_t unit_block_size = CACHEstruct->CHwidth * CACHEstruct->CHheight * SCREEN_BPP;		//每块缓存大小

	#if 1
	if(cache_empty(CACHEstruct) == true)
	{
		//DEBUG_PRINTF;
		return NULL;
	}
	DEBUG_PRINTF;
	CACHEstruct->rd_pos= (CACHEstruct->rd_pos + 1) % CACHEstruct->blockCNT;
	CACHEstruct->R_Addr = retAddr + unit_block_size * CACHEstruct->rd_pos;
	#else
	CACHEstruct->R_Addr = retAddr + unit_block_size * CACHEstruct->rd_pos;
	if(cache_empty(CACHEstruct) != true)
		CACHEstruct->rd_pos= (CACHEstruct->rd_pos + 1) % CACHEstruct->blockCNT;
	#endif
	
	return CACHEstruct->R_Addr;
}


void Write_SetNextFrameAddr(CACHEstruct_t *CACHEstruct)
{
	uint8_t *WriteAddr = NULL;
	CACHEstruct->W_Pos = (CACHEstruct->W_Pos + 1) % 3;
	while(CACHEstruct->RWflag[CACHEstruct->W_Pos] != CACHE_WRITE)
	{
		//debug_printf("CACHEstruct->RWflag[%d] = %d,CACHE_WRITE = %d\n",CACHEstruct->W_Pos,CACHEstruct->RWflag[CACHEstruct->W_Pos],CACHE_WRITE);
		CACHEstruct->W_Pos = (CACHEstruct->W_Pos + 1) % 3;
		usleep(1000);
		continue;
	}

	WriteAddr = CACHEstruct->cache + CACHEstruct->W_Pos * CACHEstruct->CHwidth * CACHEstruct->CHheight * SCREEN_BPP;

	CACHEstruct->W_Addr = WriteAddr;
}


void CACHE_INIStruct(CACHEstruct_t **CACHEstruct,int blkCNT,int Cwidth,int Cheight)
{
	uint8_t i = 0;
	CACHEstruct_t *CACHEstruct_P = *CACHEstruct;

	*CACHEstruct 				= (CACHEstruct_t *)malloc(sizeof(CACHEstruct_t));
	
	(*CACHEstruct)->blockCNT	= blkCNT;
	(*CACHEstruct)->CHwidth		= Cwidth;
	(*CACHEstruct)->CHheight 	= Cheight;

	(*CACHEstruct)->cache		= (uint8_t *)malloc(Cwidth * Cheight * SCREEN_BPP * blkCNT);
	//debug_printf("#Cwidth * Cheight * SCREEN_BPP * blkCNT = %d,(*CACHEstruct)->CHwidth = %d,(*CACHEstruct)->CHheight = %d\n",Cwidth * Cheight * SCREEN_BPP * blkCNT,(*CACHEstruct)->CHwidth,(*CACHEstruct)->CHheight);

	(*CACHEstruct)->empty		= cache_empty;
	(*CACHEstruct)->full 		= cache_full;
	(*CACHEstruct)->readAddr	= cache_ReadAddr;
	(*CACHEstruct)->writeAddr  	= cache_WriteAddr;
	(*CACHEstruct)->unreadable	= 0;

	(*CACHEstruct)->wr_pos		= 1;			//写完再偏移，即所指地方没写
	(*CACHEstruct)->rd_pos		= 0;			//读完在偏移，即所指地方没读
	(*CACHEstruct)->R_Pos		= 0;
	(*CACHEstruct)->W_Pos		= 1;
	for(i = 0 ; i < 3 ; i ++)
		(*CACHEstruct)->RWflag[i] = CACHE_WRITE;

	
	pthread_mutex_init(&((*CACHEstruct)->RWlock),NULL);
	
}

void CACHE_STRFree(CACHEstruct_t **CACHEstruct)
{	
	free((*CACHEstruct)->cache);
	free((*CACHEstruct));
}





void Ca_INITDSPCStruct(uint8_t CNT)
{
	int i = 0;
	int BufSize = 0;
	uint32_t Swidth,Sheight;
	uint8_t *DSPbuf = NULL;
	DSPHEAD.head = NULL;
	DSPHEAD.tail = NULL;

	DSPCStruct_t *NEWDSPStruct = NULL;

	DP_GetScreenSize(&Swidth,&Sheight);
	BufSize = Swidth * Sheight * SCREEN_BPP;

	for(i = 0 ; i < CNT ; i++)
	{
		NEWDSPStruct = (DSPCStruct_t *)malloc(sizeof(DSPCStruct_t));
		if(NEWDSPStruct == NULL)
		{
			perror("malloc NEWDSPStruct");
			return;
		}
		DSPbuf = (uint8_t *)malloc(BufSize);
		if(DSPbuf == NULL)
		{
			perror("malloc DSPbuf");
			return;
		}
		
		memset(NEWDSPStruct,0,sizeof(DSPCStruct_t));
		memset(DSPbuf,0,BufSize);
		
		NEWDSPStruct->Dcount = 0;
		NEWDSPStruct->Dnumber = 0;
		NEWDSPStruct->Dheight = Sheight;
		NEWDSPStruct->Dwidth = Swidth;
		NEWDSPStruct->Dspbuf = DSPbuf;
		NEWDSPStruct->Dnext = NULL;
		
		if(DSPHEAD.head == NULL && DSPHEAD.tail == NULL)
		{
			DSPHEAD.head = NEWDSPStruct;
			DSPHEAD.tail = NEWDSPStruct;
		}
		else
		{
			DSPHEAD.tail->Dnext = NEWDSPStruct;
			DSPHEAD.tail = NEWDSPStruct;
		}
	}
	
}


