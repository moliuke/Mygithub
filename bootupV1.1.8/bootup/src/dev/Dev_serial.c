#if 0
#include "../include/serial.h"
#include "../include/debug.h"
#else
#include "Dev_serial.h"
#include "../debug.h"
#endif




#define SERIAL_TEST_USE



Serial_t serial_grup[4];




#if 0
/*******************************************************************  
* 名称：                UART0_Set  
* 功能：                设置串口数据位，停止位和效验位  
* 入口参数：        fd        串口文件描述符  
*                              speed     串口速度  
*                              flow_ctrl   数据流控制  
*                           databits   数据位   取值为 7 或者8  
*                           stopbits   停止位   取值为 1 或者2  
*                           parity     效验类型 取值为N,E,O,,S  
*出口参数：          正确返回为1，错误返回为0  
*******************************************************************/    
static err_t UART_set(Serial_t *comx)    
{    
    
    int   i;    
    int   speed_arr[] = { BR115200, BR57600,BR19200, BR9600, BR4800, BR2400, BR1200, BR300};    
    int   name_arr[] = {115200,  57600,  19200,  9600,  4800,  2400,  1200,  300};    
    
    struct termios options;    
    
    /*tcgetattr(fd,&options)得到与fd指向对象的相关参数，并将它们保存于options,该函数还可以测试配置是否正确，该串口是否可用等。若调用成功，函数返回值为0，若调用
失败，函数返回值为1.  
    */    
	debug_printf("comx->baudrate = %d,comx->databits = %d,comx->dev = %s,comx->fd = %d,comx->flowctl = %c,comx->parity = %c,comx->stopbits = %d\n",comx->baudrate,comx->databits,comx->dev,comx->fd,comx->flowctl,comx->parity,comx->stopbits);
    if( tcgetattr( comx->fd,&options)  !=  0)    
    {    
        perror("SetupSerial,failed to get serial port attr\n");    
        return ERR_FAIL;    
    }    
    
    //设置串口输入波特率和输出波特率    
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)    
    {    
        if  (comx->baudrate == name_arr[i])    
        {    
        	DEBUG_PRINTF;
			debug_printf("comx->baudrate = %d\n",comx->baudrate);
            cfsetispeed(&options, speed_arr[i]);    
            cfsetospeed(&options, speed_arr[i]);    
        }    
    }    
    
    //修改控制模式，保证程序不会占用串口    
    options.c_cflag |= CLOCAL;    
    //修改控制模式，使得能够从串口中读取输入数据    
    options.c_cflag |= CREAD;    
    
    //设置数据流控制    
    switch(comx->flowctl)
    {

        case FLOW_CTL_NOTUSE ://不使用流控制    
              options.c_cflag &= ~CRTSCTS;
              break;

        case FLOW_CTL_HARD ://使用硬件流控制    
              options.c_cflag |= CRTSCTS;
              break;
        case FLOW_CTL_SOFT ://使用软件流控制    
              options.c_cflag |= IXON | IXOFF | IXANY;
              break;
	default:
	      options.c_cflag &= ~CRTSCTS;
	      break;
    }
    //设置数据位    
    //屏蔽其他标志位    
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
    //设置校验位    
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
    // 设置停止位     
    switch (comx->stopbits)
    {
        case STOP_BIT_1:
			options.c_cflag &= ~CSTOPB; break;
        case STOP_BIT_2:
			options.c_cflag |= CSTOPB; break;
        default:
			options.c_cflag &= ~CSTOPB; break;
    }

    //修改输出模式，原始数据输出    
    options.c_oflag &= ~OPOST;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //options.c_lflag &= ~(ISIG | ICANON);    

    //设置等待时间和最小接收字符    
    options.c_cc[VTIME] = 1; /* 读取一个字符等待1*(1/10)s */
    options.c_cc[VMIN] = 1; /* 读取字符的最少个数为1 */

    //如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读    
    tcflush(comx->fd,TCIFLUSH);
    //激活配置 (将修改后的termios数据设置到串口中）    
    if (tcsetattr(comx->fd,TCSANOW,&options) != 0)
    {
        perror("com set error!\n");
        return ERR_FAIL;
    }
    return ERR_OK;
}



/*******************************************************************  
* 名称：                  UART0_Open  
* 功能：                打开串口并返回串口设备文件描述  
* 入口参数：        fd    :文件描述符     port :串口号(ttyS0,ttyS1,ttyS2)  
* 出口参数：        正确返回为1，错误返回为0  
*******************************************************************/
static err_t UART_open(Serial_t *comx)
{
    int fcntl_val;
	DEBUG_PRINTF;

    comx->fd = open(comx->dev,O_RDWR | O_NOCTTY | O_NDELAY);
	
	DEBUG_PRINTF;
    if(comx->fd < 0)
    {
            perror("Can't open Serial Port!\n");
            return ERR_FAIL;
    }
	DEBUG_PRINTF;
    //恢复串口为阻塞状态
    fcntl_val = fcntl(comx->fd, F_SETFL, 0);
    if(fcntl_val < 0)
    {
            debug_printf("fcntl failed !\n");
            return ERR_FAIL;
    }
    debug_printf("fcntl=%d\n",fcntl_val);
	DEBUG_PRINTF;
    //测试是否为终端设备 

#if 0
    if(0 == isatty(STDIN_FILENO))
    {
            debug_printf("standard input is not a terminal device\n");
            return ERR_FAIL;
    }
    debug_printf("isatty success!\n");
#endif
	DEBUG_PRINTF;
    debug_printf("comx->fd=%d\n",comx->fd);
	
	return ERR_OK;

}


err_t UART_init(Serial_t *comx)
{
	if(UART_open(comx) != ERR_OK)
	{
		debug_printf("err:fail to open the serial device \n");
		return ERR_FAIL;
	}
	if(UART_set(comx) != ERR_OK)
	{
		debug_printf("error:failed to set the argument of serial\n");
		return ERR_FAIL;
	}		
	debug_printf("init the serial ok\n");
	return ERR_OK;

}
void UART_close(Serial_t *comx)
{
	close(comx->fd);
}



/*******************************************************************  
* 名称：                  UART0_Recv  
* 功能：                接收串口数据  
* 入口参数：        fd                  :文件描述符      
*                              rcv_buf     :接收串口中数据存入rcv_buf缓冲区中  
*                              data_len    :一帧数据的长度  
* 出口参数：        正确返回为1，错误返回为0  
*******************************************************************/
int UART_recv(Serial_t *comx, char *rcv_buf,int data_len)
{
    int len,fs_sel;
    fd_set fs_read;

    struct timeval time;

    FD_ZERO(&fs_read);
    FD_SET(comx->fd,&fs_read);

    time.tv_sec = 10;
    time.tv_usec = 0;
	DEBUG_PRINTF;
    //使用select实现串口的多路通信    
    fs_sel = select(comx->fd+1,&fs_read,NULL,NULL,&time);
    debug_printf("fs_sel = %d\n",fs_sel);
	
	DEBUG_PRINTF;
    if(fs_sel)
    {
		DEBUG_PRINTF;
        len = read(comx->fd,rcv_buf,data_len);
        debug_printf("I am right!(version1.2) ,len = %d\n",len);
        return len;
    }
    else
    {
		DEBUG_PRINTF;
        debug_printf("Sorry,I am wrong!\n");
        return -1;
    }
}


/********************************************************************  
* 名称：       UART0_Send  
* 功能：       发送数据  
*              send_buf    :存放串口发送数据  
*                              data_len    :一帧数据的个数  
* 出口参数：        正确返回为1，错误返回为0  
*******************************************************************/
int UART_send(Serial_t *comx, unsigned char *send_buf,int data_len)
{
    int len = 0;
	int i = 0;
	DEBUG_PRINTF;
    len = write(comx->fd,send_buf,data_len);
    if (len == data_len )
    {
        debug_printf("send data is : len = %d,comx->fd = %d\n",len,comx->fd);
		for(i = 0 ; i < data_len ; i ++)
		{
			debug_printf("%x\t",send_buf[i]);
		}
		debug_printf("\n");
		DEBUG_PRINTF; 
        return len;
    }
    else
    {
		DEBUG_PRINTF;
        tcflush(comx->fd,TCOFLUSH);
        return -1;
    }

}

#endif


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
	
	uart_debug_printf("init the serial ok\n");
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
	UART_DEBUG_PRINTF;
	len = read(comx->fd,rcv_buf,data_len);
	uart_debug_printf("I am right!(version1.2) ,len = %d\n",len);
#if 0
	for(i = 0 ; i < len ; i ++)
		uart_debug_printf("0x%x ",rcv_buf[i]);
	uart_debug_printf("\n");
	UART_DEBUG_PRINTF;
#endif 
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
	debug_printf("tx_len = %d\n",tx_len);
	for(j = 0 ; j < tx_len ; j++)//j ++);
	{
		debug_printf("0x%x ",tx_buf[j]);
	}
	debug_printf("\n");
#endif
	return UART_send(&serial_grup[comx],tx_buf,tx_len);
}







#if 0
#ifdef SERIAL_TEST_USE
int main(int argc,char **argv)
{
	err_t err;
	char send_buf[50] = "huang ke ning da ben dan!";
	char recv_buf[50];
	Serial_t comx;
	int len = 0;
	int i = 1;
	comx.baudrate = BR115200;
	comx.databits  = DATA_BIT_8;
	comx.stopbits  = STOP_BIT_1;
	comx.flowctl  = FLOW_CTL_NOTUSE;
	comx.parity   = PARITY_N;
	comx.dev      = argv[1];

	err = UART_init(&comx);
	if(err != ERR_OK)
		return -1;
	
    if(0 == strcmp(argv[2],"0"))
    {
	while(1)
        {
            len = uart_send(xCOM1,send_buf,strlen(send_buf));
            if(len > 0)
                debug_printf(" %d time send %d data successful\n",i++,len);
            else
                debug_printf("send data failed!\n");

            sleep(2);
        }
        UART_close(&comx);
    }
    else
    {
        while (1) //循环读取数据    
        {
	    memset(recv_buf,0,sizeof(recv_buf));
            len = UART_recv(&comx, recv_buf,50);
            if(len > 0)
            {
                recv_buf[len] = '\0';
                debug_printf("data len = %d,receive data is %s\n",len,recv_buf);
                debug_printf("len = %d\n",len);
            }
            else
            {
                debug_printf("cannot receive data\n");
            }
            sleep(2);
        }
        UART_close(&comx);
    }

	return 0;
}

#endif
#endif


#if 0

int main(void)
{
	uart_init(xCOM1);//����λ��ͨ��
	uart_init(xCOM2);//����λ��ͨ��

	uint8_t A[8] = {1,2,3,4,5,6,7,8};
	uart_send(xCOM2,A,8);
	
	return 0;
}


#endif

