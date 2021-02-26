#include<stdio.h>
#include <string.h>
#include "mbedtls/aes.h"
#include "mbedtls/config.h"
//#include "debug.h"
//#include "config.h"

#include "aes_interf.h"

#include "mbedtls/compat-1.3.h"
//#include "aes.c"
 
#define AES_ECB 0
#define AES_CBC 1
#define AES_CFB 2
#define AES_CTR 3
#define MODE AES_ECB
 
unsigned char key[16] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36 };
unsigned char input[138] = {
	0x5b, 0x70, 0x6c, 0x61, 0x79, 0x6c, 0x69, 0x73, 0x74, 0x5d,
	0xd, 0xa, 0x69, 0x74, 0x65, 0x6d, 0x5f, 0x6e, 0x6f, 0x20,
	0x3d, 0x20, 0x33, 0xd, 0xa, 0x69, 0x74, 0x65, 0x6d, 0x30,
	0x20, 0x3d, 0x20, 0x31, 0x30, 0x30, 0x2c, 0x20, 0x31, 0x2c,
	0x20, 0x31, 0x30, 0x2c, 0x20, 0x5c, 0x43, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x5c, 0x42, 0x35, 0x35, 0x35, 0x62, 0x6d,
	0x70, 0xd, 0xa, 0x69, 0x74, 0x65, 0x6d, 0x31, 0x20, 0x3d,
	0x20, 0x31, 0x30, 0x30, 0x2c, 0x20, 0x31, 0x2c, 0x20, 0x31,
	0x30, 0x2c, 0x5c, 0x43, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
	0x5c, 0x4a, 0x36, 0x36, 0x36, 0x6a, 0x70, 0x67, 0xd, 0xa,
	0x69, 0x74, 0x65, 0x6d, 0x32, 0x20, 0x3d, 0x20, 0x31, 0x30,
	0x30, 0x2c, 0x20, 0x31, 0x2c, 0x20, 0x31, 0x30, 0x2c, 0x20,
	0x5c, 0x43, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5c, 0x50,
	0x37, 0x37, 0x37, 0x70, 0x6e, 0x67, 0xd, 0xa
	};
unsigned char plain_decrypt[32] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned char IV[16];
unsigned char mid_input[16];
unsigned char mid_input2[16] = {0}; 
unsigned char inv_output[1024] = {0}; 
unsigned char inv_output2[1024] = {0}; 
unsigned char cypher[16];
unsigned char * phd_input=NULL;
int i = 0;
mbedtls_aes_context aes;
 
 
 
void SetIV()
{
	int i;
	for (i = 0; i < 16; i++)
	{ 
		IV[i] = 0x55;
	}
	
}




int AES_ECB_encrypt(unsigned char *insrc,unsigned short srcLen,unsigned char *outdest,unsigned short *outLen)
{
	int i = 0;
	int wgr;//总的加密次数
	unsigned char mid_input[16];
	unsigned char cypher[16];
	
	if(srcLen < 16)
		wgr=1;
	else
		wgr = srcLen/16+1;
	int j;
	
	for(i=0;i<wgr;i++)
	{
		mbedtls_aes_setkey_enc(&aes, key, 128);//  set encrypt key	
		int k = 0;
		for(j=0; j<16; ++j)
		{
			if((16*i+j)<srcLen)
			{
				mid_input[j] = insrc[16*i+j];
			}
			else
			{
				mid_input[j] = 16 - srcLen % 16;
			}
		}
		mbedtls_aes_crypt_ecb(&aes, 1, mid_input, cypher);
		for(j=0; j<16; ++j)
		outdest[16*i+j] = cypher[j];
	} 
	//printf("wgr * 16 = %d\n",wgr * 16);
	*outLen = wgr * 16;
	return 0;
		
}



/**
函数名:AES_ECB_decrypt
参数1--insrc:待解密的数据
参数2--srcLen:待解密数据长度
参数3--outdest:解密后的数据
参数4--outLen:解密后数据长度
参数5--endFlag:标记是否解密的最后一帧数据,0表示非最后一节，1表示最后一节
*/
int AES_ECB_decrypt(unsigned char *insrc,unsigned short srcLen,unsigned char *outdest,unsigned short *outLen,unsigned char endFlag)
{
	int size = 0;
	int i = 0,j = 0;
	int wgr;//总的加密次数
	unsigned char mid_input[16];
	unsigned char mid_input2[16] = {0}; 
	unsigned char cypher[16];
	aes_debug_printf("srcLen = %d\n",srcLen);
	
	if(srcLen < 16)
		wgr=1;
	else
		wgr = srcLen/16;

	aes_debug_printf("wgr = %d\n",wgr);

	mbedtls_aes_setkey_dec(&aes, key, 128);//  set decrypt key
	for(i=0;i<wgr;i++)
	{
		for(j=0; j<16; ++j)
			mid_input[j] = insrc[16*i+j];
		//		   memcpy(mid_input,output+16*i,16);
	#if 0
		mbedtls_aes_crypt_ecb(&aes, 0, mid_input, mid_input2);
		for(j=0; j<16; ++j)
		{
			outdest[16*i+j] = mid_input2[j];
		}
	#else
		mbedtls_aes_crypt_ecb(&aes, 0, mid_input, (outdest + 16 * i));
	#endif

	} 
	

	size = (endFlag) ? (wgr*16-outdest[wgr*16-1]) : (wgr*16);
	aes_debug_printf("size = %d\n",size);
	*outLen = size;
	return 0;
}

#if 0
int main()
{
	
	unsigned char *reslutData;
	unsigned char *output;
	int size = 0;
	int t = 0,j;
	int i,linp;
	int wgr;//总的加密次数
	linp=sizeof(input);//长度用下载时计算出的长度
	//   linp = 33;
	FILE *FP = NULL,*WP = NULL;


	unsigned short cryptLen = 0;
	unsigned char cryptcache[1024];
	unsigned short outLen = 0;
	unsigned char outPut[1024];

#if 0
	FP = fopen("./002.lst","r");
	if(FP == NULL)
	{
		perror("open 001.lst");
		exit(1);
	}

	char filedata[1024];
	int Len = 0;
	memset(filedata,0,sizeof(filedata));
	Len = fread(filedata,1,1024,FP);

	AES_ECB_encrypt(filedata,Len,cryptcache,&cryptLen);

	WP = fopen("./003.lst","wb+");
	if(WP == NULL)
	{
		perror("open Def.lst");
		exit(1);
	}

	fwrite(cryptcache,1,cryptLen,WP);
	fflush(WP);
#endif


#if 1
	unsigned short CRC16 = 0;
	int ii = 0;
	unsigned short enLen = 0; 
	char Send[36] = {0x02,0x30,0x30};
	char enc[16] = {0x02,0x30,0x30,0x31,0x30,0x30};
	AES_ECB_encrypt(enc + 3,3,Send + 3,&enLen);

	CRC16 = XKCalculateCRC(Send+1,2+enLen);
	debug_printf("priorty = 0x%x\n",CRC16);

	for(ii = 0 ; ii < enLen + 3 ; ii++)
		debug_printf("0x%x ",(uint8_t)Send[ii]);
	debug_printf("----------\n\n");

	char DD[16];
	unsigned short LL = 0;
	AES_ECB_decrypt(Send + 3,16,DD,&LL,1);
	for(ii = 0 ; ii < LL ; ii++)
		debug_printf("0x%x, ",DD[ii]);
	debug_printf("\n\n");
	//exit(1);
#endif
	
#if 0	
	uint8_t input[] = {};
	AES_ECB_encrypt(input,sizeof(input),cryptcache,&cryptLen);
	t = cryptLen;
	for(j=0; j<t; ++j)
		printf("%02x ", cryptcache[j]);
	printf("\n");

	
	AES_ECB_decrypt(cryptcache,cryptLen,outPut,&outLen);
	
	for(j=0; j<outLen; j++)
		printf("%02x ", outPut[j]);
	printf("\n");
#endif

#if 0	

	////////////////////////////

	linp=t;//长度用下载时计算出的长度

	if(linp < 16)
		wgr=1;
	else
		wgr = linp/16;

	mbedtls_aes_setkey_dec(&aes, key, 128);//  set decrypt key
	for(i=0;i<wgr;i++)
	{
		for(j=0; j<16; ++j)
			mid_input[j] = output[16*i+j];
		//		   memcpy(mid_input,output+16*i,16);
		mbedtls_aes_crypt_ecb(&aes, 0, mid_input, mid_input2);
		for(j=0; j<16; ++j)
			inv_output[16*i+j] = mid_input2[j];
	} 
	size = wgr*16-inv_output[wgr*16-1];
	reslutData = (unsigned char *)malloc(size);
	for(j=0; j<(size); ++j)
	{
		reslutData[j] = inv_output[j];
	}
	for(j=0; j<(size); j++)
		printf("%02x ", *reslutData++);
	printf("\n");
	i++;
#endif
	
}

#endif

