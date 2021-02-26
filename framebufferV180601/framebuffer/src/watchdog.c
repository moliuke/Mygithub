#include <stdio.h>
#include <sys/io.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "config.h"
#include "watchdog.h"

#define outp(a, b) outb(b, a)
#define inp(a) inb(a)
static void wdt_start(void)
{
	//enable watchdog timer
	unsigned char c = inp(0x68);
	c |= 0x40; 
	outp(0x68, c);
}



void wdt_stop(void)
{
	
}


void wdt_init(uint32_t time_ms)
{
	int fd = 0;

	uint32_t lTime = 0x20L * time_ms;
	//debug_printf("phz = %f,counter = %f,lTime = %d\n",phz,counter,lTime);
	
	iopl(3);

	//set time
	outp(0x6C, (lTime >> 16) & 0xff);
	outp(0x6B, (lTime >> 8) & 0xff);
	outp(0x6A, (lTime >> 0) & 0xff);
	
	// Reset system	// Reset system. For example, 0x50 to trigger IRQ7
	outp(0x69, 0xD0);
	
	//enable watchdog timer
	unsigned char c = inp(0x68);
	c |= 0x40; 
	outp(0x68, c);

}


void wdt_feed()
{
	outp(0x67, 0x00);
}