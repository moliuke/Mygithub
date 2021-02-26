#ifndef __CONFIG_H
#define __CONFIG_H
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "../module/log.h"


extern struct timeval StartTime,endTime;

/*/////////////////////////////////////////////////////////////////////////////////////////
����˵��:
1������ģʽ���أ�Ĭ�Ͽ���
2��Э������ѡ��,Ĭ���Կ�Э��
3��Ӳ���汾ѡ��,Ĭ��TXRX
4��ͨѶ����TCP����UDP��Ĭ��TCP
5���Ƿ񵥶������Կ�Э���շ��ļ��Ĺ��ܣ����Կ�Э��ʱ��ѡ��Ĭ�ϲ�ѡ��
6���Ƿ�����־ϵͳ��Ĭ�Ϲر�
7����ʾ�Ǻ��������ƫ��
8���汾�����е���Ŀ���ƣ��Լ��汾��
/////////////////////////////////////////////////////////////////////////////////////////*/

/*------------------------------------����ģʽ�ܿ���--------------------------------------*/
//#define DEBUG_MODE  
//#define DEBUG_TO_FILE

/*-----------------------------------���ػ�ѡ��------------------------------------------*/
#define BOAD6153		0
//#define BOAD6353		1

/*----------------------------------TCPIPЭ��ѡ��------------------------------------------*/
//#define TCPIP_TCP         //����udp
//#define TCPIP_UDP             //Ŀǰֻ���к��γ�����ʹ��udpͨ��

/*-----------------------------------�����Կ�Э���շ��ļ��߳�-------------------------------*/
//ֻ�е�Э��Ϊ���Կ�Э��ʱ������Ҫ����
//#define SEEWOR_RECV_FILE

/*-----------------------------------��־ϵͳ����ѡ��---------------------------------------*/
//Ĭ�Ϲر�
//#define CONFIG_LOG					1  


/*------------------------------------�����̵߳Ĵ���ر�----------------------------------*/
#if 0
#if (defined TCPIP_TCP) 
#define CONFIG_TASK_MONITOR			1		//TCPͨ��
#elif (defined TCPIP_UDP) 
#define CONFIG_UDP_TASK				1		//UDPͨ��
#endif
#endif

#if (defined SEEWOR_RECV_FILE)
//#define CONFIG_FILE_RECV			1		//�Կ�Э���շ��ļ�������IP
#endif

#define CONFIG_TASK_PROTOCOL   		1		//Э�鴦��

#define CONFIG_TASK_DISPLAY			1		//��ʾ�߳�
#define CONFIG_TASK_FRAMEBREAK		1		//֡�ֽ⣬��̬��ʾ
#define CONFIG_TASK_UPDATE   		1		//���ڸ���ʵʱ״̬����
#define CONFIG_TASK_UART_MONITOR	1 		//���ڽ���Э������
//#define CONFIG_TASK_PING			1		//�����Զ�ping����


/*------------------------------------����汾����------------------------------------------*/
//�汾��ʽ:SWR(SEEWOR)_CMS(�豸���)_XK(�Կ�Э��)_LINUX(linux����ϵͳ)_GENERAL(��Ŀ����:ͨ��)_V1.2.2(����汾)

/*�汾��ʽ : �汾��(1-16)*/
#define MAJOR		1	//���汾��
#define MINOR		3	//�ΰ汾��
#define REVISION1	2	//�޶��汾��
#define REVISION2   1   //�޶��汾��

#define BRIGHT_AUTO		0x31
#define BRIGHT_HAND		0x30
/*
#if (defined PROTOCOL_CD)
#define BRIGHT_AUTO		0x30
#define BRIGHT_HAND		0x31
#else
#define BRIGHT_AUTO		0x31
#define BRIGHT_HAND		0x30
#endif
*/

#ifdef BOAD6153
#define SLEEPTIME		1000
#else
#define SLEEPTIME		6000
#endif

#define SCRTYPE_DOOR			0x01
#define SCRTYPE_ARM				0x02

#define TRANSCARD_TXRX			0x00
#define TRANSCARD_400			0x01
#define TRANSCARD_200			0x02

#define SLED_ON			1
#define SLED_OFF		0

#define CACHE_WRITE		1

//����Э��
#define SEEWOR 			0
#define CHENGDU 		1
#define JINXIAO 			2
#define MODBUS 		    3
#define PERPLELIGHT 	4
//#define SANSI   		5
#define XIAMEN  		6
#define ZHICHAO 	    7
#define ZHONGHAIZC 		8
#define UPGRADE         9

//����Э��
#define BOZHOU          20
#define FUZHOU			21
#define GENERAL			22
#define HEAO			23
#define HEBEIERQIN		24
#define MALAYSIA		25
#define ZHUHAIPROJ		26
#define LIANDONG		28
//#define CMD				27

#if 1
typedef signed char		int8_t;
typedef unsigned char 	uint8_t;
typedef signed short	int16_t;
typedef unsigned short	uint16_t;
typedef	signed int 		int32_t; 
typedef unsigned int	uint32_t;
#endif

//���Ͷ���
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;


#define GET_PIXELS_FUN			    1 


  

//����ģʽ������ 
#ifdef DEBUG_MODE
//�򿪴�ӡ��Ϣ����
#define PRINTF_DEB
#define __DEBUG
//#define WDT_ENABLE
#define UART_RX_ENABLE
#else 
//�򿪿��Ź� 
//#define WDT_ENABLE
//�򿪴�����Ϣ���ܣ����ڽ��ܵ����ص�״̬��Ϣ̫��������ģʽ�¹ص�

#define UART_RX_ENABLE
#endif 



#define TEST_MODE		1
#define NORMAL_MODE		0
#define TEST_FLAG_SET	1
#define TEST_FLAG_CLR	0


#ifdef DMP
#define PIXELS_BITS				32
#define FRAME_BUFFER_WIDTH		640
#define FRAME_BUFFER_HEIGHT		480
#define sys_dir "/home/LEDscr"
#endif

#ifdef CQA
#define PIXELS_BITS				24
#define FRAME_BUFFER_WIDTH		1024
#define FRAME_BUFFER_HEIGHT		768
#define sys_dir "/root/LEDscr"
#endif

#define SCREEN_BPP			(PIXELS_BITS  >> 3)
#define FRAME_BUFFER_LINE_BYTES (FRAME_BUFFER_WIDTH * SCREEN_BPP)


#define conf_dir 		sys_dir"/config"
#define image_dir		sys_dir"/image"
#define setting_dir		sys_dir"/setting"
#define font_dir		sys_dir"/FONT"
#define list_dir		sys_dir
#define list_dir_1		sys_dir"/list"
#define boot_dir		sys_dir"/boot"
#define bmp_dir			image_dir"/bmp"
#define jpg_dir			image_dir"/jpg"
#define gif_dir			image_dir"/gif"
#define png_dir			image_dir"/png"
#define Upgrade_dir     sys_dir"/upgrade"


#define debugfile						sys_dir"/log/debug.log"
#define ledstatefile					conf_dir"/ledstate"
#define ConFigFile						conf_dir"/cls.conf"
#define ConFigFile_CPY					conf_dir"/cpy_cls.conf"
#define ConFigFile_Setting				setting_dir"/cls.conf"
#define ConFigFile_Setting_config		setting_dir"/config/cls.conf"
#define ConFigFile_Setting_config_cpy	setting_dir"/config/cpy_cls.conf"
#define config_sh						conf_dir"/config.sh"
#define _cls							conf_dir"/_cls.conf"
#define f_cls							sys_dir"/factory/config/_cls.conf"
#define RecordPtcFile                   conf_dir"/protocol"
#define CurrentPtcFile 					conf_dir"/protocol.conf"
#define WatchdogFile					conf_dir"/watchdog.conf"
#define CHECKPATH 						conf_dir"/check.conf"
#define AUTOBRIGHT						conf_dir"/autobright.conf"


#define GkFile							Upgrade_dir"/ledscreen"
#define TxFile							Upgrade_dir"/TX_CARD.bin"
#define RxFile							Upgrade_dir"/RX_CARD.bin"


#define syscrashlock	sys_dir"/crash.lock"
#define logdir			sys_dir"/log"
#define IPCONFIG		sys_dir"/ipconfig.sh"

#ifdef CONFIG_LOG
#define _log_file_write_  log_file_write
#else
#define _log_file_write_  
#endif

#define BYTE_RGB_B		0
#define BYTE_RGB_G		1
#define BYTE_RGB_R		2
#define BYTE_RGB_A		3


//��SDLģ����ʹ�ã�Ĭ�����ó�640*480,�ȸĳ�����ֵ
#define VIDEO_SCREEN_WIDTH    800
#define VIDEO_SCREEN_HEIGHT   600


extern pthread_mutex_t content_lock;

#endif

