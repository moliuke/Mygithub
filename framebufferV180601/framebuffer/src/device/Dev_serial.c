#include "Dev_serial.h"
#include "debug.h"
#include <stdbool.h>


#define SERIAL_TEST_USE
#define LOCK	1
#define UNLOCK	0

Serial_t serial_grup[4];
static bool COM2islock = false;

bool isCOM2_lock(void)
{
	return (COM2islock == true);
}

void COM2_lock(int lock)
{
	COM2islock = (lock == LOCK) ? true : false;
}

/*******************************************************************  
* ���ƣ�                UART0_Set  
* ���ܣ�                ���ô�������λ��ֹͣλ��Ч��λ  
* ��ڲ�����        fd        �����ļ�������  
*                              speed     �����ٶ�  
*                              flow_ctrl   ����������  
*                           databits   ����λ   ȡֵΪ 7 ����8  
*                           stopbits   ֹͣλ   ȡֵΪ 1 ����2  
*                           parity     Ч������ ȡֵΪN,E,O,,S  
*���ڲ�����          ��ȷ����Ϊ1�����󷵻�Ϊ0  
*******************************************************************/    
static err_t UART_set(Serial_t *comx)    
{    
    
    int   i;    
    int   speed_arr[] = { B115200,B57600,B19200, B9600, B4800, B2400, B1200, B300};    
    int   name_arr[] = {115200,57600,19200,9600,4800,2400,1200,300};    
    
    struct termios options;    
    
    /*tcgetattr(fd,&options)�õ���fdָ��������ز������������Ǳ�����options,�ú��������Բ��������Ƿ���ȷ���ô����Ƿ���õȡ������óɹ�����������ֵΪ0��������
ʧ�ܣ���������ֵΪ1.  
    */    
    if( tcgetattr( comx->fd,&options)  !=  0)    
    {    
        perror("SetupSerial,failed to get serial port attr\n");    
        return ERR_FAIL;    
    }    
    
    //���ô������벨���ʺ����������    
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)    
    {    
        if  (comx->baudrate == name_arr[i])    
        {    
	    	uart_debug_printf("comx->baudrate == name_arr[i]\n");
	    	uart_debug_printf("comx->baudrate = %d\n",comx->baudrate);
            cfsetispeed(&options, speed_arr[i]);    
            cfsetospeed(&options, speed_arr[i]);    
        }    
    }    
    
    //�޸Ŀ���ģʽ����֤���򲻻�ռ�ô���    
    options.c_cflag |= CLOCAL;    
    //�޸Ŀ���ģʽ��ʹ���ܹ��Ӵ����ж�ȡ��������    
    options.c_cflag |= CREAD;    
    
    //��������������    
    switch(comx->flowctl)
    {

        case FLOW_CTL_NOTUSE ://��ʹ��������    
              options.c_cflag &= ~CRTSCTS;
              break;

        case FLOW_CTL_HARD ://ʹ��Ӳ��������    
              options.c_cflag |= CRTSCTS;
              break;
        case FLOW_CTL_SOFT ://ʹ�����������    
              options.c_cflag |= IXON | IXOFF | IXANY;
              break;
	default:
			options.c_cflag &= ~CRTSCTS;
			break;
    }
    //��������λ    
    //����������־λ    
    options.c_cflag &= ~CSIZE;
    switch (comx->databits)
    {
        case DATA_BIT_5:
             options.c_cflag |= CS5;
             break;
        case DATA_BIT_6:
             options.c_cflag |= CS6;
             break;
        case DATA_BIT_7:
             options.c_cflag |= CS7;
             break;
        case DATA_BIT_8:
             options.c_cflag |= CS8;
             break;
        default:
			options.c_cflag |= CS8;
			break;
    }
    //����У��λ    
    switch (comx->parity)
    {
        case PARITY_N:
		 		options.c_cflag &= ~PARENB;
                 options.c_iflag &= ~INPCK;
                 break;
        
      	case PARITY_O:
                options.c_cflag |= (PARODD | PARENB);
                 options.c_iflag |= INPCK;
                 break;
     
   		case PARITY_E:
                 options.c_cflag |= PARENB;
                 options.c_cflag &= ~PARODD;
                 options.c_iflag |= INPCK;
                 break;
        
      	case PARITY_S:
                 options.c_cflag &= ~PARENB;
                 options.c_cflag &= ~CSTOPB;
                 break;
        default:
			options.c_cflag &= ~PARENB;
			 options.c_iflag &= ~INPCK;
			 break;
    }

	options.c_iflag &= ~(ICRNL | IXON);
    // ����ֹͣλ     
    switch (comx->stopbits)
    {
        case STOP_BIT_1:
                 options.c_cflag &= ~CSTOPB; break;
        case STOP_BIT_2:
                 options.c_cflag |= CSTOPB; break;
        default:
				options.c_cflag &= ~CSTOPB; break;
    }

    //�޸����ģʽ��ԭʼ�������    
    options.c_oflag &= ~OPOST;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //options.c_lflag &= ~(ISIG | ICANON);    

    //���õȴ�ʱ�����С�����ַ�    
    options.c_cc[VTIME] = 1; /* ��ȡһ���ַ��ȴ�1*(1/10)s */
    options.c_cc[VMIN] = 1; /* ��ȡ�ַ������ٸ���Ϊ1 */

    //�����������������������ݣ����ǲ��ٶ�ȡ ˢ���յ������ݵ��ǲ���    
    tcflush(comx->fd,TCIFLUSH);
    //�������� (���޸ĺ��termios�������õ������У�    
    if (tcsetattr(comx->fd,TCSANOW,&options) != 0)
    {
        perror("com set error!\n");
        return ERR_FAIL;
    }
    return ERR_OK;
}



/*******************************************************************  
* ���ƣ�                  UART0_Open  
* ���ܣ�                �򿪴��ڲ����ش����豸�ļ�����  
* ��ڲ�����        fd    :�ļ�������     port :���ں�(ttyS0,ttyS1,ttyS2)  
* ���ڲ�����        ��ȷ����Ϊ1�����󷵻�Ϊ0  
*******************************************************************/
static err_t UART_open(Serial_t *comx)
{
        int fcntl_val;
		//O_NONBLOCKָ�����豸��дʱ������������
        comx->fd = open(comx->dev,O_RDWR | O_NOCTTY | O_NDELAY);
        if(comx->fd < 0)
        {
                perror("Can't open Serial Port!\n");
                return ERR_FAIL;
        }

        //�ָ�����Ϊ����״̬
        fcntl_val = fcntl(comx->fd, F_SETFL, 0);
        if(fcntl_val < 0)
        {
            uart_debug_printf("fcntl failed !\n");
            return ERR_FAIL;
        }
        uart_debug_printf("fcntl=%d\n",fcntl_val);

		fcntl_val |= O_NONBLOCK;
		fcntl(comx->fd,F_SETFL,fcntl_val);


	#if 0
        //�����Ƿ�Ϊ�ն��豸        
        if(0 == isatty(STDIN_FILENO))
        {
                debug_printf("standard input is not a terminal device\n");
                return ERR_FAIL;
        }
        debug_printf("isatty success!\n");
	#endif

        uart_debug_printf("comx->fd=%d\n",comx->fd);
	return ERR_OK;

}


err_t UART_init(Serial_t *comx)
{

	if(UART_open(comx) != ERR_OK)
	{
		uart_debug_printf("err:fail to open the serial device \n");
		return ERR_FAIL;
	}
	
	if(UART_set(comx) != ERR_OK)
	{
		uart_debug_printf("error:failed to set the argument of serial\n");
		return ERR_FAIL;
	}		
	
	//printf("init the serial ok\n");
	return ERR_OK;

}
void UART_close(Serial_t *comx)
{
	close(comx->fd);
}

/*******************************************************************  
* ���ƣ�                  UART0_Recv  
* ���ܣ�                ���մ�������  
* ��ڲ�����        fd                  :�ļ�������      
*                              rcv_buf     :���մ��������ݴ���rcv_buf��������  
*                              data_len    :һ֡���ݵĳ���  
* ���ڲ�����        ��ȷ����Ϊ1�����󷵻�Ϊ0  
*******************************************************************/
int UART_recv(Serial_t *comx, unsigned char *rcv_buf,int data_len)
{
	int len,i;	
	len = read(comx->fd,rcv_buf,data_len);

	return len;  

}

int UART_Recv(Serial_t *comx, unsigned char *rcv_buf,int data_len)
{
    return read(comx->fd,rcv_buf,data_len);
}



/********************************************************************  
* ���ƣ�       UART0_Send  
* ���ܣ�       ��������  
*              send_buf    :��Ŵ��ڷ�������  
*                              data_len    :һ֡���ݵĸ���  
* ���ڲ�����        ��ȷ����Ϊ1�����󷵻�Ϊ0  
*******************************************************************/
int UART_send(Serial_t *comx, unsigned char *send_buf,int data_len)
{
    int len = 0;
	int i = 0;

    len = write(comx->fd,send_buf,data_len);
    if (len == data_len )
    {
    #if 1
        uart_debug_printf("send data is : ");
		for(i = 0 ; i < data_len ; i ++)
		{
			uart_debug_printf("0x%x ",send_buf[i]);
		}
		uart_debug_printf("\n");
		UART_DEBUG_PRINTF;
	#endif
	
        return len;
    }
	
#if 0
    else
    {

        tcflush(comx->fd,TCOFLUSH);
        return -1;
    }
#endif
}

void serial_param_set(COMx_t COMx,uint32_t baudrate,uint8_t databits,uint8_t stopbits,uint8_t flowctl,uint8_t parity)
{
	char serial_dev[24];

	memset(serial_dev,0x00,sizeof(serial_dev));
	sprintf(serial_dev,"/dev/ttyS%d",COMx);
	uart_debug_printf("serial_dev = %s\n",serial_dev);

	memcpy(serial_grup[COMx].dev,serial_dev,strlen(serial_dev));
	serial_grup[COMx].baudrate = baudrate;
	serial_grup[COMx].databits = databits;
	serial_grup[COMx].stopbits = stopbits;
	serial_grup[COMx].flowctl  = FLOW_CTL_NOTUSE;
	serial_grup[COMx].parity   = PARITY_N;
}



err_t uart_init(COMx_t comx)
{
	return UART_init(&serial_grup[comx]);
}
err_t uart_exit(COMx_t comx)
{
	UART_close(&serial_grup[comx]);
}


int uart_recv(COMx_t comx,unsigned char *rx_buf,int rx_len)
{
	return UART_recv(&serial_grup[comx],rx_buf,rx_len);
}

int uart_Recv(COMx_t comx,unsigned char *rx_buf,int rx_len)
{
	return UART_Recv(&serial_grup[comx],rx_buf,rx_len);
}

int uart_send(COMx_t comx,unsigned char *tx_buf,int tx_len)
{
	int i = 0,j = 0; 
#if 1
	DEBUG_PRINTF;
	debug_printf("tx_len = %d\n",tx_len);
	for(j = 0 ; j < tx_len ; j++)
	{
		debug_printf("0x%02x ",tx_buf[j]);
	}
	debug_printf("\n");
	debug_printf("comx = %d\n",comx);
#endif
	return UART_send(&serial_grup[comx],tx_buf,tx_len);
}

void xCOM2_send(uint8_t *data,uint8_t Len)
{
	if(isCOM2_lock() == true)
	{

	}
	else
	{
		COM2_lock(LOCK);
		uart_send(xCOM2,data,Len);
		COM2_lock(UNLOCK);
	}
}


