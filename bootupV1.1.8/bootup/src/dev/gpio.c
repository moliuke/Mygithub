#include <stdio.h>
#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>

#define LEVEL_HEIGHT	1
#define LEVEL_LOW		0

#define outp(a, b) outb(b, a)
#define inp(a) inb(a)


inline void GPIO_pin_init(void)
{
	iopl(3);
	/* set GPIO port0[7-0] as input mode */
	outp(0x98, 0xfe);

	/* set GPIO port1[7-0] as input mode */
	outp(0x99, 0x7f);
}


inline int restore_pin_level(void)
{
	return inp(0x78) & 0x01;
}

inline int IPrestore_pin_level(void)
{
	return inp(0x79) & 0x80;
}

int check_manMadeRestore(void)
{
	if(restore_pin_level())
		return -1;
	
	usleep(5 * 1000);
	//Ïû¶¶
	if(restore_pin_level())
		return -1;

	return 0;
}

int check_IPRestore(void)
{
	if(IPrestore_pin_level())
		return -1;
	
	usleep(5 * 1000);
	//Ïû¶¶
	if(IPrestore_pin_level())
		return -1;

	return 0;
}


#if 0
#include <stdio.h>
#include <stdio.h>
#include <sys/io.h>
#define outportb(a,b) outb(b,a)
#define inportb(a) inb(a)
void main(void)
{
	//iopl(3);
	/* set GPIO port0[7-0] as input mode */
	//outportb(0x98, 0xfe);
	//restore_pin_init();
	/* read data from GPIO port0 */
	//inportb(0x78);
	while(1)
	{
		sleep(1);
			//debug_printf("%d !\n",restore_pin_level());
		debug_printf("huang ke ning da ben dan1\n");	
	}
}
#endif
