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
* åç§°ï¼š                UART0_Set  
* åŠŸèƒ½ï¼š                è®¾ç½®ä¸²å£æ•°æ®ä½ï¼Œåœæ­¢ä½å’Œæ•ˆéªŒä½  
* å…¥å£å‚æ•°ï¼š        fd        ä¸²å£æ–‡ä»¶æè¿°ç¬¦  
*                              speed     ä¸²å£é€Ÿåº¦  
*                              flow_ctrl   æ•°æ®æµæ§åˆ¶  
*                           databits   æ•°æ®ä½   å–å€¼ä¸º 7 æˆ–è€…8  
*                           stopbits   åœæ­¢ä½   å–å€¼ä¸º 1 æˆ–è€…2  
*                           parity     æ•ˆéªŒç±»å‹ å–å€¼ä¸ºN,E,O,,S  
*å‡ºå£å‚æ•°ï¼š          æ­£ç¡®è¿”å›ä¸º1ï¼Œé”™è¯¯è¿”å›ä¸º0  
*******************************************************************/    
static err_t UART_set(Serial_t *comx)    
{    
    
    int   i;    
    int   speed_arr[] = { BR115200, BR57600,BR19200, BR9600, BR4800, BR2400, BR1200, BR300};    
    int   name_arr[] = {115200,  57600,  19200,  9600,  4800,  2400,  1200,  300};    
    
    struct termios options;    
    
    /*tcgetattr(fd,&options)å¾—åˆ°ä¸fdæŒ‡å‘å¯¹è±¡çš„ç›¸å…³å‚æ•°ï¼Œå¹¶å°†å®ƒä»¬ä¿å­˜äºoptions,è¯¥å‡½æ•°è¿˜å¯ä»¥æµ‹è¯•é…ç½®æ˜¯å¦æ­£ç¡®ï¼Œè¯¥ä¸²å£æ˜¯å¦å¯ç”¨ç­‰ã€‚è‹¥è°ƒç”¨æˆåŠŸï¼Œå‡½æ•°è¿”å›å€¼ä¸º0ï¼Œè‹¥è°ƒç”¨
å¤±è´¥ï¼Œå‡½æ•°è¿”å›å€¼ä¸º1.  
    */    
	debug_printf("comx->baudrate = %d,comx->databits = %d,comx->dev = %s,comx->fd = %d,comx->flowctl = %c,comx->parity = %c,comx->stopbits = %d\n",comx->baudrate,comx->databits,comx->dev,comx->fd,comx->flowctl,comx->parity,comx->stopbits);
    if( tcgetattr( comx->fd,&options)  !=  0)    
    {    
        perror("SetupSerial,failed to get serial port attr\n");    
        return ERR_FAIL;    
    }    
    
    //è®¾ç½®ä¸²å£è¾“å…¥æ³¢ç‰¹ç‡å’Œè¾“å‡ºæ³¢ç‰¹ç‡    
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
    
    //ä¿®æ”¹æ§åˆ¶æ¨¡å¼ï¼Œä¿è¯ç¨‹åºä¸ä¼šå ç”¨ä¸²å£    
    options.c_cflag |= CLOCAL;    
    //ä¿®æ”¹æ§åˆ¶æ¨¡å¼ï¼Œä½¿å¾—èƒ½å¤Ÿä»ä¸²å£ä¸­è¯»å–è¾“å…¥æ•°æ®    
    options.c_cflag |= CREAD;    
    
    //è®¾ç½®æ•°æ®æµæ§åˆ¶    
    switch(comx->flowctl)
    {

        case FLOW_CTL_NOTUSE ://ä¸ä½¿ç”¨æµæ§åˆ¶    
              options.c_cflag &= ~CRTSCTS;
              break;

        case FLOW_CTL_HARD ://ä½¿ç”¨ç¡¬ä»¶æµæ§åˆ¶    
              options.c_cflag |= CRTSCTS;
              break;
        case FLOW_CTL_SOFT ://ä½¿ç”¨è½¯ä»¶æµæ§åˆ¶    
              options.c_cflag |= IXON | IXOFF | IXANY;
              break;
	default:
	      options.c_cflag &= ~CRTSCTS;
	      break;
    }
    //è®¾ç½®æ•°æ®ä½    
    //å±è”½å…¶ä»–æ ‡å¿—ä½    
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
    //è®¾ç½®æ ¡éªŒä½    
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
    // è®¾ç½®åœæ­¢ä½     
    switch (comx->stopbits)
    {
        case STOP_BIT_1:
			options.c_cflag &= ~CSTOPB; break;
        case STOP_BIT_2:
			options.c_cflag |= CSTOPB; break;
        default:
			options.c_cflag &= ~CSTOPB; break;
    }

    //ä¿®æ”¹è¾“å‡ºæ¨¡å¼ï¼ŒåŸå§‹æ•°æ®è¾“å‡º    
    options.c_oflag &= ~OPOST;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //options.c_lflag &= ~(ISIG | ICANON);    

    //è®¾ç½®ç­‰å¾…æ—¶é—´å’Œæœ€å°æ¥æ”¶å­—ç¬¦    
    options.c_cc[VTIME] = 1; /* è¯»å–ä¸€ä¸ªå­—ç¬¦ç­‰å¾…1*(1/10)s */
    options.c_cc[VMIN] = 1; /* è¯»å–å­—ç¬¦çš„æœ€å°‘ä¸ªæ•°ä¸º1 */

    //å¦‚æœå‘ç”Ÿæ•°æ®æº¢å‡ºï¼Œæ¥æ”¶æ•°æ®ï¼Œä½†æ˜¯ä¸å†è¯»å– åˆ·æ–°æ”¶åˆ°çš„æ•°æ®ä½†æ˜¯ä¸è¯»    
    tcflush(comx->fd,TCIFLUSH);
    //æ¿€æ´»é…ç½® (å°†ä¿®æ”¹åçš„termiosæ•°æ®è®¾ç½®åˆ°ä¸²å£ä¸­ï¼‰    
    if (tcsetattr(comx->fd,TCSANOW,&options) != 0)
    {
        perror("com set error!\n");
        return ERR_FAIL;
    }
    return ERR_OK;
}



/*******************************************************************  
* åç§°ï¼š                  UART0_Open  
* åŠŸèƒ½ï¼š                æ‰“å¼€ä¸²å£å¹¶è¿”å›ä¸²å£è®¾å¤‡æ–‡ä»¶æè¿°  
* å…¥å£å‚æ•°ï¼š        fd    :æ–‡ä»¶æè¿°ç¬¦     port :ä¸²å£å·(ttyS0,ttyS1,ttyS2)  
* å‡ºå£å‚æ•°ï¼š        æ­£ç¡®è¿”å›ä¸º1ï¼Œé”™è¯¯è¿”å›ä¸º0  
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
    //æ¢å¤ä¸²å£ä¸ºé˜»å¡çŠ¶æ€
    fcntl_val = fcntl(comx->fd, F_SETFL, 0);
    if(fcntl_val < 0)
    {
            debug_printf("fcntl failed !\n");
            return ERR_FAIL;
    }
    debug_printf("fcntl=%d\n",fcntl_val);
	DEBUG_PRINTF;
    //æµ‹è¯•æ˜¯å¦ä¸ºç»ˆç«¯è®¾å¤‡ 

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
* åç§°ï¼š                  UART0_Recv  
* åŠŸèƒ½ï¼š                æ¥æ”¶ä¸²å£æ•°æ®  
* å…¥å£å‚æ•°ï¼š        fd                  :æ–‡ä»¶æè¿°ç¬¦      
*                              rcv_buf     :æ¥æ”¶ä¸²å£ä¸­æ•°æ®å­˜å…¥rcv_bufç¼“å†²åŒºä¸­  
*                              data_len    :ä¸€å¸§æ•°æ®çš„é•¿åº¦  
* å‡ºå£å‚æ•°ï¼š        æ­£ç¡®è¿”å›ä¸º1ï¼Œé”™è¯¯è¿”å›ä¸º0  
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
    //ä½¿ç”¨selectå®ç°ä¸²å£çš„å¤šè·¯é€šä¿¡    
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
* åç§°ï¼š       UART0_Send  
* åŠŸèƒ½ï¼š       å‘é€æ•°æ®  
*              send_buf    :å­˜æ”¾ä¸²å£å‘é€æ•°æ®  
*                              data_len    :ä¸€å¸§æ•°æ®çš„ä¸ªæ•°  
* å‡ºå£å‚æ•°ï¼š        æ­£ç¡®è¿”å›ä¸º1ï¼Œé”™è¯¯è¿”å›ä¸º0  
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
* Ãû³Æ£º                UART0_Set  
* ¹¦ÄÜ£º                ÉèÖÃ´®¿ÚÊı¾İÎ»£¬Í£Ö¹Î»ºÍĞ§ÑéÎ»  
* Èë¿Ú²ÎÊı£º        fd        ´®¿ÚÎÄ¼şÃèÊö·û  
*                              speed     ´®¿ÚËÙ¶È  
*                              flow_ctrl   Êı¾İÁ÷¿ØÖÆ  
*                           databits   Êı¾İÎ»   È¡ÖµÎª 7 »òÕß8  
*                           stopbits   Í£Ö¹Î»   È¡ÖµÎª 1 »òÕß2  
*                           parity     Ğ§ÑéÀàĞÍ È¡ÖµÎªN,E,O,,S  
*³ö¿Ú²ÎÊı£º          ÕıÈ··µ»ØÎª1£¬´íÎó·µ»ØÎª0  
*******************************************************************/    
static err_t UART_set(Serial_t *comx)    
{    
    
    int   i;    
    int   speed_arr[] = { B115200,B57600,B19200, B9600, B4800, B2400, B1200, B300};    
    int   name_arr[] = {115200,57600,19200,9600,4800,2400,1200,300};    
    
    struct termios options;    
    
    /*tcgetattr(fd,&options)µÃµ½ÓëfdÖ¸Ïò¶ÔÏóµÄÏà¹Ø²ÎÊı£¬²¢½«ËüÃÇ±£´æÓÚoptions,¸Ãº¯Êı»¹¿ÉÒÔ²âÊÔÅäÖÃÊÇ·ñÕıÈ·£¬¸Ã´®¿ÚÊÇ·ñ¿ÉÓÃµÈ¡£Èôµ÷ÓÃ³É¹¦£¬º¯Êı·µ»ØÖµÎª0£¬Èôµ÷ÓÃ
Ê§°Ü£¬º¯Êı·µ»ØÖµÎª1.  
    */    
    if( tcgetattr( comx->fd,&options)  !=  0)    
    {    
        perror("SetupSerial,failed to get serial port attr\n");    
        return ERR_FAIL;    
    }    
    
    //ÉèÖÃ´®¿ÚÊäÈë²¨ÌØÂÊºÍÊä³ö²¨ÌØÂÊ    
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
    
    //ĞŞ¸Ä¿ØÖÆÄ£Ê½£¬±£Ö¤³ÌĞò²»»áÕ¼ÓÃ´®¿Ú    
    options.c_cflag |= CLOCAL;    
    //ĞŞ¸Ä¿ØÖÆÄ£Ê½£¬Ê¹µÃÄÜ¹»´Ó´®¿ÚÖĞ¶ÁÈ¡ÊäÈëÊı¾İ    
    options.c_cflag |= CREAD;    
    
    //ÉèÖÃÊı¾İÁ÷¿ØÖÆ    
    switch(comx->flowctl)
    {

        case FLOW_CTL_NOTUSE ://²»Ê¹ÓÃÁ÷¿ØÖÆ    
              options.c_cflag &= ~CRTSCTS;
              break;

        case FLOW_CTL_HARD ://Ê¹ÓÃÓ²¼şÁ÷¿ØÖÆ    
              options.c_cflag |= CRTSCTS;
              break;
        case FLOW_CTL_SOFT ://Ê¹ÓÃÈí¼şÁ÷¿ØÖÆ    
              options.c_cflag |= IXON | IXOFF | IXANY;
              break;
	default:
			options.c_cflag &= ~CRTSCTS;
			break;
    }
    //ÉèÖÃÊı¾İÎ»    
    //ÆÁ±ÎÆäËû±êÖ¾Î»    
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
    //ÉèÖÃĞ£ÑéÎ»    
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
    // ÉèÖÃÍ£Ö¹Î»     
    switch (comx->stopbits)
    {
        case STOP_BIT_1:
                 options.c_cflag &= ~CSTOPB; break;
        case STOP_BIT_2:
                 options.c_cflag |= CSTOPB; break;
        default:
				options.c_cflag &= ~CSTOPB; break;
    }

    //ĞŞ¸ÄÊä³öÄ£Ê½£¬Ô­Ê¼Êı¾İÊä³ö    
    options.c_oflag &= ~OPOST;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //options.c_lflag &= ~(ISIG | ICANON);    

    //ÉèÖÃµÈ´ıÊ±¼äºÍ×îĞ¡½ÓÊÕ×Ö·û    
    options.c_cc[VTIME] = 1; /* ¶ÁÈ¡Ò»¸ö×Ö·ûµÈ´ı1*(1/10)s */
    options.c_cc[VMIN] = 1; /* ¶ÁÈ¡×Ö·ûµÄ×îÉÙ¸öÊıÎª1 */

    //Èç¹û·¢ÉúÊı¾İÒç³ö£¬½ÓÊÕÊı¾İ£¬µ«ÊÇ²»ÔÙ¶ÁÈ¡ Ë¢ĞÂÊÕµ½µÄÊı¾İµ«ÊÇ²»¶Á    
    tcflush(comx->fd,TCIFLUSH);
    //¼¤»îÅäÖÃ (½«ĞŞ¸ÄºóµÄtermiosÊı¾İÉèÖÃµ½´®¿ÚÖĞ£©    
    if (tcsetattr(comx->fd,TCSANOW,&options) != 0)
    {
        perror("com set error!\n");
        return ERR_FAIL;
    }
    return ERR_OK;
}



/*******************************************************************  
* Ãû³Æ£º                  UART0_Open  
* ¹¦ÄÜ£º                ´ò¿ª´®¿Ú²¢·µ»Ø´®¿ÚÉè±¸ÎÄ¼şÃèÊö  
* Èë¿Ú²ÎÊı£º        fd    :ÎÄ¼şÃèÊö·û     port :´®¿ÚºÅ(ttyS0,ttyS1,ttyS2)  
* ³ö¿Ú²ÎÊı£º        ÕıÈ··µ»ØÎª1£¬´íÎó·µ»ØÎª0  
*******************************************************************/
static err_t UART_open(Serial_t *comx)
{
        int fcntl_val;
		//O_NONBLOCKÖ¸¶¨¸ÃÉè±¸¶ÁĞ´Ê±²»»áÒıÆğ×èÈû
        comx->fd = open(comx->dev,O_RDWR | O_NOCTTY | O_NDELAY);
        if(comx->fd < 0)
        {
                perror("Can't open Serial Port!\n");
                return ERR_FAIL;
        }

        //»Ö¸´´®¿ÚÎª×èÈû×´Ì¬
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
        //²âÊÔÊÇ·ñÎªÖÕ¶ËÉè±¸        
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
* Ãû³Æ£º                  UART0_Recv  
* ¹¦ÄÜ£º                ½ÓÊÕ´®¿ÚÊı¾İ  
* Èë¿Ú²ÎÊı£º        fd                  :ÎÄ¼şÃèÊö·û      
*                              rcv_buf     :½ÓÊÕ´®¿ÚÖĞÊı¾İ´æÈërcv_buf»º³åÇøÖĞ  
*                              data_len    :Ò»Ö¡Êı¾İµÄ³¤¶È  
* ³ö¿Ú²ÎÊı£º        ÕıÈ··µ»ØÎª1£¬´íÎó·µ»ØÎª0  
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
* Ãû³Æ£º       UART0_Send  
* ¹¦ÄÜ£º       ·¢ËÍÊı¾İ  
*              send_buf    :´æ·Å´®¿Ú·¢ËÍÊı¾İ  
*                              data_len    :Ò»Ö¡Êı¾İµÄ¸öÊı  
* ³ö¿Ú²ÎÊı£º        ÕıÈ··µ»ØÎª1£¬´íÎó·µ»ØÎª0  
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
        while (1) //å¾ªç¯è¯»å–æ•°æ®    
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
	uart_init(xCOM1);//ÓëÉÏÎ»»úÍ¨ĞÅ
	uart_init(xCOM2);//ÓëÏÂÎ»»úÍ¨ĞÅ

	uint8_t A[8] = {1,2,3,4,5,6,7,8};
	uart_send(xCOM2,A,8);
	
	return 0;
}


#endif

