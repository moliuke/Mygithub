#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>   
//#include <sys/io.h>
#include <stddef.h>
#include <ctype.h>


#include "../task.h"
#include "../protocol/seewor/SWR_protocol.h"
#include "../protocol/seewor/SWR_display.h"
#include "../protocol/perplelight/PPL_display.h"
#include "../protocol/perplelight/PPL_datapool.h"
#include "../protocol/perplelight/PPL_net.h"
#include "common.h"
#include "queue.h"
#include "Dev_tcpserver.h"
#include "debug.h"
#include "config.h"
#include "conf.h"
#include "wdt.h"
#include "../clientlist.h"
#include "../threadpool.h"
#include "../mtimer.h"
#include "../include/display.h"
#include "../module/image_gif.h"

#include "../protocol/Modbus/modbus_task.h"
#include "../protocol/Modbus/modbus_protocol.h"

#include "../protocol/ZhiChao/ZC_task.h"
#include "../protocol/ZhiChao/ZC_protocol.h"
#include "../protocol/ZhiChao/ZC_display.h"

#include "../Hardware/HW3G_RXTX.h"
#include "../Hardware/HW2G_400.h"
#include "../Hardware/HW2G_200.h"


#include "../cache.h"
#include "../Hardware/Data_pool.h"

#include "SDL_task.h"

/*
	  此线程用于在PC端测试使用，在板子上由于缺乏相关的库，请勿打开

	  在PC端上开启一个SDL窗口，此窗口实际上就是一个虚拟屏幕，大小可以认为的设定
*/
#ifdef CONFIG_VIDEO_TEST
void DrawScreen(SDL_Surface* screen, unsigned  char * fb)
{
    memcpy(screen->pixels, fb, VIDEO_SCREEN_WIDTH * VIDEO_SCREEN_HEIGHT * SCREEN_BPP);
    SDL_Flip(screen);
}

void *pthread_SDL_task(void *arg)
{
    SDL_Surface* virt_screen;  
	SDL_Event event;
	int keypress = 0;
	
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        debug_printf("SDL_Init failed.\n");
        return NULL;
    }

    if (!(virt_screen = SDL_SetVideoMode(VIDEO_SCREEN_WIDTH, VIDEO_SCREEN_HEIGHT, 32, SDL_HWSURFACE)))
    {
        SDL_Quit();
        debug_printf("SDL_Quit failed.\n");
        return NULL;
    }
	
    while(!keypress)
	{
		DrawScreen(virt_screen, screen.frameBuffer);
		while(SDL_PollEvent(&event))
		{ 
			switch(event.type)
			{ 
				case SDL_QUIT: 
					keypress = 1; 
					break;
				case SDL_KEYDOWN:
					keypress = 1;
					break;
			}
		}
	}
}

#endif


