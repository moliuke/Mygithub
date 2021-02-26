#ifndef ptc_init_h
#define ptc_init_h
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "../update.h"
#include "common.h"
#include "content.h"


void protocolRelatInitDestroy(void);
void protocolRelatInit(void *arg1,void *arg2);
void DefaultLstDisplay(void);
void DefmsgDisplay(void);
int UpdateExtendInterf(uint8_t *data,uint16_t Len);

int NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int recv_task_process(void * arg);
int protocol_processor(user_t *user,uint8_t *input,uint32_t *inputlen);
int SetEffectOption(uint8_t option,uint8_t *type,uint8_t *dire);

void PixTimerRegister(void);
void SetVersion(void);
char *GetProtocolStr(void);
char *GetProjectStr(void);


int (*defmsgdispaly)(void);
void (*dspdefaultlst)(void);
void (*PixTimerReg)(void);
int (*EffectOption)(uint8_t,uint8_t *,uint8_t *);

int (*protocol)(user_t *,uint8_t *,uint32_t *);

int (*netSendback)(user_t *,uint8_t *,int);
int (*recv_process)(void *);
int (*updateInterf)(uint8_t *,uint16_t);

int (*curplaying)(void *);

void (*version)(void);
char *(*protocolstr)(void);
char *(*projectstr)(void);


extern uint8_t *FREEdata;



#endif
