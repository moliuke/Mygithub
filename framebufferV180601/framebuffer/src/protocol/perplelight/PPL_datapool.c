#include "PPL_datapool.h"
#include "debug.h"
#include "config.h"

//�Ϲ�Э�����ݽ�����
typedef struct __DATApool
{
	char		curlist[8];		//��ǰ�Ĳ����б�
	uint16_t 	listLen;
	
	char 		curdsplay[512];	//��ǰ��ʾ������
	uint16_t 	contentLen;
	uint16_t 	inform;			//���ַ�ʽ
	uint32_t 	inSpeed;		//�����ٶ�
	uint32_t 	stayTime;		//ͣ��ʱ��

	uint8_t 	brightMode;		//����ģʽ
	uint8_t 	bright_R;		//����ֵ
	uint8_t 	bright_G;
	uint8_t 	bright_B;
	
}DATApool_t;

static DATApool_t PLL_DATA_pool;


#if 0
//���õ�ǰ��ʾ���ֵ����ݡ����ȡ����ַ�ʽ�������ٶȡ�ͣ��ʱ��
void PPL_SETCurDspContent(char *Content,uint16_t len,uint16_t inform,uint32_t inSpeed,uint32_t stayTime)
{
	memcpy(PLL_DATA_pool.curdsplay,Content,len);
	PLL_DATA_pool.contentLen = len;
	
	PLL_DATA_pool.inform	= inform;
	PLL_DATA_pool.inSpeed	= inSpeed;
	PLL_DATA_pool.stayTime	= stayTime * 100;
	
}

//��ȡ��ǰ��ʾ���ֵ����ݡ����ȡ����ַ�ʽ�������ٶȡ�ͣ��ʱ��
void PPL_GETCurDspContent(char *Content,uint16_t *len,uint16_t *inform,uint32_t *inSpeed,uint32_t *stayTime)
{
	DEBUG_PRINTF;
	memcpy(Content,PLL_DATA_pool.curdsplay,PLL_DATA_pool.contentLen);
	DEBUG_PRINTF;
	*len 		= PLL_DATA_pool.contentLen;
	DEBUG_PRINTF;
	*inform 	= PLL_DATA_pool.inform;
	DEBUG_PRINTF;
	*inSpeed 	= PLL_DATA_pool.inSpeed;
	DEBUG_PRINTF;
	*stayTime 	= PLL_DATA_pool.stayTime;
	DEBUG_PRINTF;
}



//���õ�ǰ��ʾ�б�
void PPL_SETCurDspLst(char *list,uint8_t len)
{
	memcpy(PLL_DATA_pool.curlist,list,len);
	PLL_DATA_pool.listLen = len;
}
//��ȡ��ǰ��ʾ�б�
void PPL_GETCurDspLst(char *list,uint8_t *len)
{
	memcpy(list,PLL_DATA_pool.curlist,PLL_DATA_pool.listLen);
	*len = PLL_DATA_pool.listLen;
}


//���õ�ǰ����
void PPL_SETCurBright(uint8_t RGB_R,uint8_t RGB_G,uint8_t RGB_B)
{
	PLL_DATA_pool.bright_R	= RGB_R;
	PLL_DATA_pool.bright_G	= RGB_G;
	PLL_DATA_pool.bright_B	= RGB_B;
	debug_printf("PLL_DATA_pool.bright_R = %d,PLL_DATA_pool.bright_G = %d,PLL_DATA_pool.bright_B = %d\n",PLL_DATA_pool.bright_R,PLL_DATA_pool.bright_G,PLL_DATA_pool.bright_B);
}
//��ȡ��ǰ����
void PPL_GETCurBright(uint8_t *RGB_R,uint8_t *RGB_G,uint8_t *RGB_B)
{
	*RGB_R = PLL_DATA_pool.bright_R;
	*RGB_G = PLL_DATA_pool.bright_G;
	*RGB_B = PLL_DATA_pool.bright_B;
}



void PPL_SETBrightMode(uint8_t mode)
{
	PLL_DATA_pool.brightMode = mode;
}

void PPL_GETBrightMode(uint8_t *mode)
{
	*mode = PLL_DATA_pool.brightMode;
}


#endif




