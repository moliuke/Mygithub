#ifndef _update_h
#define _update_h

#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "Dev_serial.h"
#include "module/mtimer.h"

#define WARM_ON		0x01
#define WARM_OFF	0x00


void GetUpdateDate(void);
void SET_LED_STATE(uint8_t state);
int LEDstateRecord(uint8_t state);
void ResetTxRxCardMsg(void);
void SetBrightParam(void);
void SetParameter(void);
void PixelsTimerReg(void);

void PixelsTimerUnreg(void);

void *RoutineMonitor(void *arg);

void update_timer_register(void);


#endif

