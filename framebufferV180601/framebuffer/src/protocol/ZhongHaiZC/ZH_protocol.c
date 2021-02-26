#include <sys/types.h>
#include <sys/socket.h>
#include<netinet/in.h>

#include <stdio.h>
#include "ZH_charparse.h"
#include "common.h"
#include "ZH_protocol.h"
#include "../PTC_init.h"
#include "../Defmsg.h"
#include "../../Hardware/Data_pool.h"



static uint8_t ClearScr[] = 
{0x00,0x00,0x00,0x00,0xC0,0x00,0x50,0x00,0xCB,0xCE,0xCC,0xE5,0x18,0x18,0xFF,0x00,0x00,0x5C,0x6E,0x5C,0x6E,0x00};
int WritePlayLst(uint8_t *Content,uint16_t Len)
{
	char strlen[3];
	FILE *FP = NULL;
	FP = fopen(ZH_PLAYLIST,"w+");
	if(FP == NULL)
		return -1;
	//千三个字节记录数据长度
	fseek(FP,0,SEEK_SET);
	//uint16_t contentlen = Len;
	if(Len < 10)
	{
		strlen[0] = 0x30;
		strlen[1] = 0x30;
		strlen[2] = 0x30 + Len ;
	}
	if(Len >= 10 && Len < 100)
	{
		sprintf(strlen,"%d",Len);
		strlen[2] = strlen[1];
		strlen[1] = strlen[0];
		strlen[0] = 0x30;
	}
	if(Len >= 100 && Len < 1000)
	{
		sprintf(strlen,"%d",Len);
	}
	fwrite(strlen,3,1,FP);
	fseek(FP,3,SEEK_SET);
	fwrite(Content,Len,1,FP);
	fflush(FP);
	fclose(FP);
	return 0;
}


int ZH_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen)
{
	uint8_t *StartStr = input;
	uint8_t *pkgnumberStr = input + 4;
	uint8_t *pkgLenStr = input + 8;
	uint8_t *pkgreserdStr = input + 10;
	uint8_t *pkgcmdStr = input + 12;
	uint8_t *pkgdataStr = input + 16;
	uint8_t *EndStr = input + *inputlen - 1;
	/*************************************************************************************/
	//每个协议都可以加这条,注意这里用的是udp,还需要修改通信协议配置文件,目前不支持进入维护模式
	    struct sockaddr_in clientaddr;  
   		socklen_t addrlen = sizeof(clientaddr);
		clientaddr.sin_family = AF_INET;  
    	clientaddr.sin_port = user->port;  
    	clientaddr.sin_addr.s_addr = htonl(user->ip); 
		uint8_t vindicate[9] = {0x02,0x39,0x30,0x30,0x30,0x30,0x7E,0x18,0x03};
		uint8_t reply[9] = {0x02,0x39,0x30,0x30,0x30,0x01,0x58,0x6A,0x03};
		if(memcmp(input,vindicate,9)==0)
		{
	
			//回复上位机 
			if(user->type == 0) //网口
				sendto(user->fd,reply,9,0,(struct sockaddr*)&clientaddr,addrlen);//这里需要改sendto
			else if(user->type == 1)
				uart_send(0,reply,9);  //xCOM1
	
			
			char buf_frist[16];
			char buf_second[16];
			char content[256];
	
			memset(buf_frist,0,sizeof(buf_frist));
			memset(buf_second,0,sizeof(buf_second));
			memset(content,0,sizeof(content));
	
			conf_file_read(CurrentPtcFile,"protocol","protocol",buf_frist);
			conf_file_read(CurrentPtcFile,"protocol","swr_protocol",buf_second);
	
			debug_printf("buf_frist is %s  buf_second is %s\n",buf_frist,buf_second);
			sprintf(content,"[protocol]\nprotocol = %s\nswr_protocol = %s\n",buf_frist,buf_second);
			debug_printf("%s\n",content);
			//保存升级之前的协议，用于升级完成后，切换回原来的协议
			FILE *IPF = fopen(RecordPtcFile,"wb+");
			if(IPF == NULL)
				return -1;
			fwrite(content,1,sizeof(content),IPF);
			fflush(IPF);
			fclose(IPF);
			
			conf_file_write(CurrentPtcFile,"protocol","protocol","upgrade");
			conf_file_write(CurrentPtcFile,"protocol","swr_protcol","general");
			system("killall ledscreen");
		}	
	
	/*******************************************************************************************/

	if(StartStr[0] != 0x54 || StartStr[1] != 0x43 || StartStr[2] != 0x4c || StartStr[3] != 0x59 || EndStr[0] != 0x00)
		return -1;

	uint16_t pkgLen = pkgLenStr[1] << 8 | pkgLenStr[0];
	//printf("pkgLen = %d\n",pkgLen);

	uint32_t pkgcmd = pkgcmdStr[3] << 24 | pkgcmdStr[2] << 16 | pkgcmdStr[1] << 8 | pkgcmdStr[0];
	//printf("pkgcmd = 0x%x\n",pkgcmd);

	int i;
	for(i = 0 ; i < *inputlen ; i++)
		debug_printf("0x%x, ",input[i]);
	debug_printf("\n");


	uint8_t TemStatus = 0,SenStatus = 0,ThandStatus = 0,PowerStatus = 0x30,
		    SysStatus = 0,SingPxStatus = 0x30,ModuleStatus = 0x30,CtrStatus = 0x30;
	uint8_t Tstatus,vals = 0;
	uint16_t devstatus = 0;
	switch(pkgcmd)
	{
		case GET_DEVSTATUS:
			DP_GetSysDataAndStatus(PID_TOTAL_TEMP,&TemStatus,&vals);
			devstatus |= (TemStatus == 0x30) ? (1 << 9) : 0;

			DP_GetSysDataAndStatus(PID_LIGHT_SENSITIVE,&SenStatus,&vals);
			devstatus |= (SenStatus == 0x30) ? (1 << 8) : 0;

			DP_GetSysDataAndStatus(PID_THANDER,&ThandStatus,&vals);
			devstatus |= (ThandStatus == 0x30) ? (1 << 7) : 0;

			devstatus |= (PowerStatus == 0x30) ? (1 << 6) : 0;

			if(TemStatus != 0x30 || SenStatus != 0x30 || ThandStatus != 0x30 || PowerStatus != 0x30)
				SysStatus = 0x31;
			else
				SysStatus = 0x30;
			devstatus |= (SysStatus == 0x30) ? (1 << 5) : 0;

			devstatus |= (SingPxStatus == 0x30) ? (1 << 4) : 0;
			devstatus |= (ModuleStatus == 0x30) ? (1 << 2) : 0;
			devstatus |= (CtrStatus == 0x30) ? (1 << 1) : 0;

			//printf("devstatus = 0x%x\n",devstatus);

			pkgdataStr[0] = (uint8_t)(devstatus >> 8);
			pkgdataStr[1] = (uint8_t)devstatus;
			
			pkgLenStr[0] = 0x12;
			pkgLenStr[1] = 0x00;

			*inputlen = 17 + 2;
			input[*inputlen - 1] = 0x00;
			
			break;

		case DSP_CURCONTENT:
			DEBUG_PRINTF;
			WritePlayLst(pkgdataStr,*inputlen - 17);
			ZH_Lstparsing(pkgdataStr,*inputlen - 17,&content);
			break;

		case CLR_SCREEN:
			ZH_Lstparsing(ClearScr,*inputlen - 13,&content);
			break;

		default:
			break;
	}

	
	return 0;
}

