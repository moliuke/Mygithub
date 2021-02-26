#ifndef __ZH_PROTOCOL_H
#define __ZH_PROTOCOL_H

#include "common.h"


#define GET_DEVSTATUS		0x00000010
#define DSP_CURCONTENT		0x00000020
#define CLR_SCREEN			0x00000030

#define ZH_PLAYLIST		list_dir_1"/play.lst"



int ZH_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen);



#endif

