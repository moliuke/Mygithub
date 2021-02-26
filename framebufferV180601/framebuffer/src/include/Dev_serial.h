#ifndef __SERIAL_H
#define __SERIAL_H
//串口相关的头文件    
#include<stdio.h>      /*标准输入输出定义*/
#include<stdlib.h>     /*标准函数库定义*/
#include<unistd.h>     /*Unix 标准函数定义*/
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>      /*文件控制定义*/
#include<termios.h>    /*PPSIX 终端控制定义*/
#include<errno.h>      /*错误号定义*/
#include<string.h>
#include <stdbool.h>

#include "config.h"
#include "myerror.h"

#include "debug.h"


#define UART_DEBUG
#ifdef UART_DEBUG
#define UART_DEBUG_PRINTF	DEBUG_PRINTF
#define uart_debug_printf 	debug_printf
#else
#define UART_DEBUG_PRINTF	
#define uart_debug_printf 	
#endif

#define LOCK	1
#define UNLOCK	0
bool isCOM2_lock(void);
void COM2_lock(int lock);

/******************grup for @BAUDRATE ************************/
#define BR115200	115200
#define BR57600		57600
#define BR19200		19200
#define BR9600		9600
#define BR4800		4800
#define BR2400		2400
#define BR1200		1200
#define BR300		300

/******************grup for @DATABIT*************************/
#define DATA_BIT_8	8
#define DATA_BIT_7	7
#define DATA_BIT_6	6
#define DATA_BIT_5	5

/*******************grup for @STOPBIT *************************/
#define STOP_BIT_1	1
#define STOP_BIT_2	2

/*******************grup for @FLOW_CTL **********************/
#define FLOW_CTL_NOTUSE 'N'
#define FLOW_CTL_HARD	'H'
#define FLOW_CTL_SOFT	'S'

/*******************grup for @PARITY ***************************/
#define PARITY_N	'N'
#define PARITY_O	'O'
#define PARITY_E	'E'
#define PARITY_S	'S'

/*******************grup for @DEV ******************************/
#define COM1	"/dev/ttyS0"
#define COM2	"/dev/ttyS1"
#define COM3	"/dev/ttyS2"
#define COM4	"/dev/ttyS3"
#define COM5	"/dev/ttyS4"
#define COM6	"/dev/ttyS5"
#define COM7	"/dev/ttyUSB0"	//COM6������PC�˿������ʼǱ�ֻ��USBת����


typedef uint8_t COMx_t;

#define xCOM1		0
#define xCOM2		1
#define xCOM3		2


typedef struct
{

        int     fd;
        char    dev[24];		//refer to grup @DEV	

        int     baudrate;       //refer to grup @BAUDRATE
        int     databits;        //refer to grup @DATABIT
        int     stopbits;	//refer to grup @STOPBIT
        char    flowctl;	//refer to grup @FLOWCTL
        char    parity;		//refer to grup @PARITY

}Serial_t;

extern Serial_t com_updt;
extern Serial_t serial_grup[4];
void serial_param_set(COMx_t COMx,uint32_t baudrate,uint8_t databits,uint8_t stopbits,uint8_t flowctl,uint8_t parity);
err_t uart_init(COMx_t comx);
err_t uart_exit(COMx_t comx);
int uart_recv(COMx_t comx,unsigned char *rx_buf,int rx_len);
int uart_send(COMx_t comx,unsigned char *tx_buf,int tx_len);
int uart_Recv(COMx_t comx,unsigned char *rx_buf,int rx_len); 
void xCOM2_send(uint8_t *data,uint8_t Len);

#endif

