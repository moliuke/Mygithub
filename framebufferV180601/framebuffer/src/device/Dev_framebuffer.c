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
#include "Dev_framebuffer.h"
#include "debug.h"
#include "display.h"

#if 0
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
unsigned char *frameBuffer = NULL;
#endif

screen_t screen;
static resolution_t resolution;



//œ¬√Ê¡Ω∏ˆ√Ë ˆµƒ « µº œ‘ æ∆¡ƒªµƒ«¯”Ú¥Û–°£¨–Ë“™‘⁄≥Ã–Ú∆Ù∂Ø ±æÕ…Ë÷√∫√
static uint32_t dsp_screen_width = 0;
static uint32_t dsp_screen_height = 0;

void fb_screen_size_set(uint32_t width,uint32_t height)
{
	dsp_screen_width 	= width;
	dsp_screen_height 	= height;
}

void fb_resolution_set(uint32_t res_x,uint32_t res_y,uint32_t bpp)
{
	resolution.res_x 	= res_x;
	resolution.res_y 	= res_y;
	resolution.bpp 		= bpp;
}



//ÊâìÂç∞fbÈ©±Âä®‰∏≠fixÁªìÊûÑ‰ø°ÊÅØÔºåÊ≥®ÔºöÂú®fbÈ©±Âä®Âä†ËΩΩÂêéÔºåfixÁªìÊûÑ‰∏çÂèØË¢´‰øÆÊîπ„ÄÇ
void fb_FixedInfo_printf (screen_t *screen)
{
	debug_printf ("Fixed screen info:\n"
			"\tid: %s\n"
			"\tsmem_start: 0x%lx\n"
			"\tsmem_len: %d\n"
			"\ttype: %d\n"
			"\ttype_aux: %d\n"
			"\tvisual: %d\n"
			"\txpanstep: %d\n"
			"\typanstep: %d\n"
			"\tywrapstep: %d\n"
			"\tline_length: %d\n"
			"\tmmio_start: 0x%lx\n"
			"\tmmio_len: %d\n"
			"\taccel: %d\n"
			"\n",
			screen->finfo.id, screen->finfo.smem_start, screen->finfo.smem_len, screen->finfo.type,
			screen->finfo.type_aux, screen->finfo.visual, screen->finfo.xpanstep, screen->finfo.ypanstep,
			screen->finfo.ywrapstep, screen->finfo.line_length, screen->finfo.mmio_start,
			screen->finfo.mmio_len, screen->finfo.accel);
}


//ÊâìÂç∞fbÈ©±Âä®‰∏≠varÁªìÊûÑ‰ø°ÊÅØÔºåÊ≥®ÔºöfbÈ©±Âä®Âä†ËΩΩÂêéÔºåvarÁªìÊûÑÂèØÊ†πÊçÆÂÆûÈôÖÈúÄË¶ÅË¢´ÈáçÁΩÆ
void fb_VariableInfo_print (screen_t *screen)
{
	debug_printf ("Variable screen info:\n"
			"\txres: %d\n"
			"\tyres: %d\n"
			"\txres_virtual: %d\n"
			"\tyres_virtual: %d\n"
			"\tyoffset: %d\n"
			"\txoffset: %d\n"
			"\tbits_per_pixel: %d\n"
			"\tgrayscale: %d\n"
			"\tred: offset: %2d, length: %2d, msb_right: %2d\n"
			"\tgreen: offset: %2d, length: %2d, msb_right: %2d\n"
			"\tblue: offset: %2d, length: %2d, msb_right: %2d\n"
			"\ttransp: offset: %2d, length: %2d, msb_right: %2d\n"
			"\tnonstd: %d\n"
			"\tactivate: %d\n"
			"\theight: %d\n"
			"\twidth: %d\n"
			"\taccel_flags: 0x%x\n"
			"\tpixclock: %d\n"
			"\tleft_margin: %d\n"
			"\tright_margin: %d\n"
			"\tupper_margin: %d\n"
			"\tlower_margin: %d\n"
			"\thsync_len: %d\n"
			"\tvsync_len: %d\n"
			"\tsync: %d\n"
			"\tvmode: %d\n"
			"\n",
			screen->vinfo.xres, screen->vinfo.yres, screen->vinfo.xres_virtual, screen->vinfo.yres_virtual,
			screen->vinfo.xoffset, screen->vinfo.yoffset, screen->vinfo.bits_per_pixel,
			screen->vinfo.grayscale, screen->vinfo.red.offset, screen->vinfo.red.length,
			screen->vinfo.red.msb_right, screen->vinfo.green.offset, screen->vinfo.green.length,
			screen->vinfo.green.msb_right, screen->vinfo.blue.offset, screen->vinfo.blue.length,
			screen->vinfo.blue.msb_right, screen->vinfo.transp.offset, screen->vinfo.transp.length,
			screen->vinfo.transp.msb_right, screen->vinfo.nonstd, screen->vinfo.activate,
			screen->vinfo.height, screen->vinfo.width, screen->vinfo.accel_flags, screen->vinfo.pixclock,
			screen->vinfo.left_margin, screen->vinfo.right_margin, screen->vinfo.upper_margin,
			screen->vinfo.lower_margin, screen->vinfo.hsync_len, screen->vinfo.vsync_len,
			screen->vinfo.sync, screen->vinfo.vmode);
}


//ÁîªÂ§ßÂ∞è‰∏∫width*heightÁöÑÂêåËâ≤Áü©ÈòµÔºå8alpha+8reds+8greens+8blues
static void fb_drawRect_rgb32 (int x0, int y0, int width, int height, int color)
{
	const int bytesPerPixel = 4;
	const int stride = screen.finfo.line_length / bytesPerPixel;


	int *dest = (int *) (screen.frameBuffer)
		+ (y0 + screen.vinfo.yoffset) * stride + (x0 + screen.vinfo.xoffset);


	int x, y;
	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			dest[x] = color;
		}
		dest += stride;
	}
}


//ÁîªÂ§ßÂ∞è‰∏∫width*heightÁöÑÂêåËâ≤Áü©ÈòµÔºå5reds+6greens+5blues
static void fb_drawRect_rgb16 (int x0, int y0, int width, int height, int color)
{
	const int bytesPerPixel = 2;
	const int stride = screen.finfo.line_length / bytesPerPixel;
	const int red = (color & 0xff0000) >> (16 + 3);
	const int green = (color & 0xff00) >> (8 + 2);
	const int blue = (color & 0xff) >> 3;
	const short color16 = blue | (green << 5) | (red << (5 + 6));


	short *dest = (short *) (screen.frameBuffer)
		+ (y0 + screen.vinfo.yoffset) * stride + (x0 + screen.vinfo.xoffset);


	int x, y;
	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			dest[x] = color16;
		}
		dest += stride;
	}
}



//ÁîªÂ§ßÂ∞è‰∏∫width*heightÁöÑÂêåËâ≤Áü©ÈòµÔºå5reds+5greens+5blues
static void fb_drawRect_rgb15 (int x0, int y0, int width, int height, int color)
{
	const int bytesPerPixel = 2;
	const int stride = screen.finfo.line_length / bytesPerPixel;
	const int red = (color & 0xff0000) >> (16 + 3);
	const int green = (color & 0xff00) >> (8 + 3);
	const int blue = (color & 0xff) >> 3;
   const short color15 = blue | (green << 5) | (red << (5 + 5)) | 0x8000;


	short *dest = (short *) (screen.frameBuffer)
		+ (y0 + screen.vinfo.yoffset) * stride + (x0 + screen.vinfo.xoffset);


	int x, y;
	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			dest[x] = color15;
		}
		dest += stride;
	}
}


static void fb_drawRect_rgb8(int x0,int y0,int width,int height,unsigned int color)
{
     const int bytesPerPixel = 1;
     const int stride = screen.finfo.line_length / bytesPerPixel;


     unsigned char *dest = (unsigned char *) (screen.frameBuffer)
         + (y0 + screen.vinfo.yoffset) * stride + (x0 + screen.vinfo.xoffset);

     int x, y;
     for (y = 0; y < height; ++y)
     {
         for (x = 0; x < width; ++x)
         {
             dest[x] = 0x0;
         }
         dest += stride;
     }

}


void fb_drawRect (int x0, int y0, int width, int height, int color)
{
	switch (screen.vinfo.bits_per_pixel)
	{
	case 32:
		fb_drawRect_rgb32 (x0, y0, width, height, color);
		break;
	case 16:
		fb_drawRect_rgb16 (x0, y0, width, height, color);
		break;
	case 15:
		fb_drawRect_rgb15 (x0, y0, width, height, color);
		break;
	case 8:
		fb_drawRect_rgb8(x0,y0,width,height,color);
   		break;
	default:
		debug_printf("Warning: drawRect() not implemented for color depth %i\n",
				screen.vinfo.bits_per_pixel);
		break;
	}
}



#define PERFORMANCE_RUN_COUNT 5
int fb_frame_speed_test(int fbSize)
{
	int i, j, run;
	struct timeval startTime, endTime;
	unsigned long long results[PERFORMANCE_RUN_COUNT];
	unsigned long long average;
	unsigned int *testImage;
	unsigned int randData[17] = {
		0x3A428472, 0x724B84D3, 0x26B898AB, 0x7D980E3C, 0x5345A084,
		0x6779B66B, 0x791EE4B4, 0x6E8EE3CC, 0x63AF504A, 0x18A21B33,
		0x0E26EB73, 0x022F708E, 0x1740F3B0, 0x7E2C699D, 0x0E8A570B,
		0x5F2C22FB, 0x6A742130
		};
	debug_printf ("Frame Buffer Performance testing...\n");
	for (run = 0; run < PERFORMANCE_RUN_COUNT; ++run)
	{
		
		gettimeofday (&startTime, NULL);
		/* Generate test image with random(ish) data: */
		testImage = (unsigned int *) malloc (fbSize);
		j = run;
		for (i = 0; i < (int) (fbSize / sizeof (int)); ++i)
		{
			testImage[i] = randData[j];
			j++;
			if (j >= 17)
				j = 0;
		}
		
		//gettimeofday (&startTime, NULL);
		memcpy (screen.frameBuffer, testImage, fbSize);	
		gettimeofday (&endTime, NULL);
		long secsDiff = endTime.tv_sec - startTime.tv_sec;
		results[run] = 
			secsDiff * 1000000 + (endTime.tv_usec - startTime.tv_usec);
		free (testImage);
	}
	average = 0;
	for (i = 0; i < PERFORMANCE_RUN_COUNT; ++i)
		average += results[i];
	average = average / PERFORMANCE_RUN_COUNT;
	
	debug_printf (" Average: %llu usecs\n", average);
	debug_printf (" Bandwidth: %.03f MByte/Sec\n",
		(fbSize / 1048576.0) / ((double) average / 1000000.0));
	debug_printf (" Max. FPS: %.03f fps\n\n",
		1000000.0 / (double) average);
	/* Clear the framebuffer back to black again: */
	memset (screen.frameBuffer, 0, fbSize);
}





FB_err_t fb_screen_init(void)
{
	long int screensize = 0;

	DEBUG_PRINTF;
	screen.dev = "/dev/fb0";
	
	/*1°¢Open the file for reading and writing */
	screen.fd = open (screen.dev, O_RDWR);
	if (screen.fd == -1)
	{
		perror ("Error: cannot open framebuffer device");
		return FB_ERR_FAIL;
	}

#if 1
	/*2  init the size of our screen ,uniting in pixel*/
	screen.vinfo.xres = FB_WITH;
	screen.vinfo.yres = FB_HEIGHT;
	screen.vinfo.xres_virtual = FB_WITH;
	screen.vinfo.yres_virtual = FB_HEIGHT; 
	screen.vinfo.bits_per_pixel = SCREEN_BITS_PIXELS;
	debug_printf("resolution.res_y = %d\n",resolution.res_y);
	if(ioctl(screen.fd,FBIOPUT_VSCREENINFO,&(screen.vinfo)) == -1)
	{
		perror("failed to modify the infomation");
		goto FREE_RESRC;
	}
#endif
	
	/*3  reads the screen fix infomation*/
	if (ioctl (screen.fd, FBIOGET_FSCREENINFO, &(screen.finfo)) == -1)
	{
		perror ("Error reading fixed information");
		goto FREE_RESRC;
	}
	//fb_FixedInfo_printf(&screen);
	
	/*4  reads the screen variable infomation*/
	if (ioctl (screen.fd, FBIOGET_VSCREENINFO, &(screen.vinfo)) == -1)
	{
		perror ("Error reading variable information");
		goto FREE_RESRC;
	}
	//fb_VariableInfo_print(&screen);

	/* 5  get the total size of the framebuffer */
	screensize = screen.finfo.smem_len;


	/* 6  Map the hardware device memory to the process memory,by doing this we\
	can operate (reading and writing) the device on our application process*/
	screen.frameBuffer =
		(char *) mmap (0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
					 screen.fd, 0);
	if (screen.frameBuffer == MAP_FAILED)
	{
		perror ("Error: Failed to map framebuffer device to memory");
		goto FREE_RESRC;
	}

	/*7  ok,now we finish the init operation*/
	return FB_ERR_OK;

	FREE_RESRC:
		close(screen.fd);
		return FB_ERR_FAIL;

}

void fb_screen_destroy(void)
{
	long int screensize = 0;
	close (screen.fd);
	munmap(screen.frameBuffer,screensize);
}


void fb_frame_Clear(uint16_t Xoffset,uint16_t Yoffset)
{
	int h = 0,w = 0;
	int writePos = Yoffset * SCREEN_LINE_BYTES + Xoffset * SCREEN_BPP;
	int memsetWidth = dsp_screen_width * SCREEN_BPP;
	for(h = 0 ; h < dsp_screen_height ; h++)
	{
		memset(screen.frameBuffer + writePos,0,memsetWidth);
		writePos += SCREEN_LINE_BYTES;
	}
}



int fb_frame_display(uint16_t Xoffset,uint16_t Yoffset,unsigned char *framedata,int bpp,uint32_t speed_us)
{
	int h,w;
	int x_pos,y_pos;		//this indicates where the piture will be showed on our screen
	int scr_Bpp,ptr_Bpp;	//bytes per pixel of screen or piture
	int scr_line_bytes,ptr_line_bytes; 
	int pixels_fr = 0 , pixels_ct = 0;
	int tmp_fr = 0,tmp_ct = 0;
	uint32_t width 		= dsp_screen_width; 
	uint32_t height 	= dsp_screen_height;

	int SLineBytes = SCREEN_LINE_BYTES;
	int ClineBytes = dsp_screen_width * 4;
	int RlineBytes = 0;

	
	int cnt = 0;
	int read_pos = 0,write_pos = 0;
	int read_pos_tmp = 0,write_pos_tmp = 0;
	ptr_line_bytes	= width * SCREEN_BPP;
	scr_line_bytes 	= SCREEN_LINE_BYTES;

	if(DSPcnt.clrenable)
	{
		DSPcnt.clrenable = 0;
		fb_frame_Clear(Xoffset,Yoffset);
		debug_printf("================fb_frame_display==============\n\n\n");
	}
	
	for(cnt = 0 ; cnt < DSPcnt.dspcnt ; cnt++)
	{
		if(DSPcnt.DSPStruct[cnt].cx >= width || DSPcnt.DSPStruct[cnt].cy >= height)
			continue;

		if(DSPcnt.DSPStruct[cnt].cx + DSPcnt.DSPStruct[cnt].width > width)
			DSPcnt.DSPStruct[cnt].width = width - DSPcnt.DSPStruct[cnt].cx;

		if(DSPcnt.DSPStruct[cnt].cy + DSPcnt.DSPStruct[cnt].height > height)
			DSPcnt.DSPStruct[cnt].height = height - DSPcnt.DSPStruct[cnt].cy;

		RlineBytes = DSPcnt.DSPStruct[cnt].width * SCREEN_BPP;
		
		write_pos = (DSPcnt.DSPStruct[cnt].cy + Yoffset) * SLineBytes + (DSPcnt.DSPStruct[cnt].cx + Xoffset) * 4;
		read_pos = DSPcnt.DSPStruct[cnt].cy * ClineBytes + DSPcnt.DSPStruct[cnt].cx * 4;
		for(h = 0 ; h < DSPcnt.DSPStruct[cnt].height; h++)
		{
			memcpy(screen.frameBuffer + write_pos,framedata + read_pos,RlineBytes);
			write_pos += SLineBytes;
			read_pos += ClineBytes;
		}
	}

	return 0;
}

