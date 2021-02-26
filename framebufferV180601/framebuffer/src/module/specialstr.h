#ifndef SPECIALSTR_H
#define SPECIALSTR_H
#include <stdio.h>
#include "config.h"
#include "debug.h"

#define FONT16_BYTES_32		32
#define FONT24_BYTES_72		72
#define FONT32_BYTES_128	128
#define FONT48_BYTES_288	288


int SpecialStrProcess(char *RBuf,uint8_t FontSize,uint8_t HByte,uint8_t LByte);
#endif


