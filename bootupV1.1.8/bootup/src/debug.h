#ifndef __DEBUG_H
#define __DEBUG_H

//����ģʽ�ܿ��� 
//#define DEBUG_MODE

//����ģʽ������ 
#ifdef DEBUG_MODE
#define PRINTF_DEB
#define __DEBUG
//#define WDT_ENABLE
//#define UART_RX_ENABLE
#else 
//�򿪿��Ź� 
#define WDT_ENABLE
//�򿪴�����Ϣ���ܣ����ڽ��ܵ����ص�״̬��Ϣ̫��������ģʽ�¹ص�
#define UART_RX_ENABLE
#endif 

#ifdef __debug
#define __debug_printf printf("[ %s ] printf in line [ %d ]\n",__func__,__LINE__)
#else 
#define __debug_printf 
#endif
//this will decide wether our printf message is print on the screen or not!!!!!
#define PRINTF_COPY 1

#ifdef PRINTF_DEB 
#define DEBUG_PRINTF printf("[ %s ] printf in line [ %d ]\n",__func__,__LINE__)
#else
#define DEBUG_PRINTF 
#endif
//int printf(const char *format,...);

//#define __DEBUG
//#define __func_DEBUG
#ifdef  __DEBUG  
#ifdef  __func_DEBUG
#define debug_printf(fmt, args...) printf("%s: "fmt,__func__, ## args)
#else
#define debug_printf(fmt, args...) printf(fmt, ## args)
#endif
#else   
#define debug_printf(fmt, args...) { } 
#endif  //end DBG_AP_STA

void kill_app(void);

#endif


