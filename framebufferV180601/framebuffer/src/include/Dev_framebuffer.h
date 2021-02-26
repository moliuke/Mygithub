#ifndef __FRAMEBUFFER_H
#define __FRAMEBUFFER_H

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
//#include "framebuffer.h"
#include "debug.h"
#include "config.h"


#define BYTE_RGB_B		0
#define BYTE_RGB_G		1
#define BYTE_RGB_R		2
#define BYTE_RGB_A		3

#define FB_WITH				FRAME_BUFFER_WIDTH
#define FB_HEIGHT			FRAME_BUFFER_HEIGHT
#define SCREEN_LINE_PIXELS	FB_WITH
#define SCREEN_LINE_BYTES	(FB_WITH * SCREEN_BPP)
#define SCREEN_BITS_PIXELS  PIXELS_BITS



typedef enum
{
	FB_ERR_OK	= 0,
	FB_ERR_FAIL
}FB_err_t;


typedef struct
{
	int 				fd;
	const char 			*dev;
	unsigned char 		*frameBuffer;
	
	struct fb_var_screeninfo 	vinfo;
	struct fb_fix_screeninfo 	finfo;
}screen_t;

typedef struct 
{
	uint32_t res_x;
	uint32_t res_y;
	uint32_t bpp;
}resolution_t;

extern screen_t screen;




void fb_FixedInfo_printf (screen_t *screen);
void fb_VariableInfo_print (screen_t *screen);

void fb_drawRect (int x0, int y0, int width, int height, int color);
int fb_frame_speed_test(int fbSize);
int fb_frame_display(uint16_t Xoffset,uint16_t Yoffset,unsigned char *framedata,int bpp,uint32_t speed_us);
FB_err_t fb_screen_init(void);
void fb_screen_destroy(void);
int diaplay(int xPos,int yPos,unsigned char *framedata,int bpp,int width,int height);

void fb_frame_Clear(uint16_t Xoffset,uint16_t Yoffset);
void fb_screen_size_set(uint32_t width,uint32_t height);
void fb_resolution_set(uint32_t res_x,uint32_t res_y,uint32_t bpp);
extern screen_t screen;
#endif



