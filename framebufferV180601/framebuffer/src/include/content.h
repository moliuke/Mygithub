#ifndef __XKTYPE_H
#define __XKTYPE_H


#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

// ����
#define MAXUNIT 16;


//���Ͷ���
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

#define PRIORITYN	-1
#define PRIORITY0	0
#define PRIORITY1	1
#define PRIORITY2	2
#define PRIORITY3	3
#define PRIORITY4	4


//---------------------------------------------------------------------- ��ʾ����---------------------------------------------------//


//����Ϣ
typedef struct 
{
	uint16_t LineWidth;
	uint16_t LineHeight;
}LineMst_t;


#define COLOR_H		0
#define COLOR_R		1
#define COLOR_G		2
#define COLOR_B		3
#define COLOR_Y		4

// �ַ�����Ԫ
typedef struct String
{

	int ScreenSeq;
	int LineSeq;
	
	int cx;
	int cy;
	int nCellOrder;
	int Color;

	int flash;

	// ����
	int nSpace;
	int nLineSpace;
	//  ������ɫ
	uint8_t nForeColor[3];
	uint8_t nBkColor[3];
	uint8_t nShadowColor[3];
	//  ��������
	uint8_t nFontSize;
	uint8_t nFontType;			//��������
	uint8_t strContent[1024];
	uint16_t strContentLen;
	//
	struct String *pNext;
}XKCellString;

// ͼ��ͼ�ε�Ԫ
typedef struct Image
{
	int ScreenSeq;
	int LineSeq;

	int cx;
	int cy;
	int type;
	int nCellOrder;
	char strImage[16];
	//
	struct Image *pNext;
}XKCellImage;


typedef struct _PNGCell
{
	int ScreenSeq;
	int LineSeq;
	int cx;
	int cy;
	int type;
	int nCellOrder;
	char pngImage[16];
	struct _PNGCell *pNext;
	
}XKCellPngimg;



// ������Ԫ
typedef struct Animate
{
	 int cx;
	 int cy;
	 int nCellOrder;
	 int nFormatType;
	 int nPlayTime;
	 char strAnimate[16];
	//
	struct Animate *pNext;
}XKCellAnimate;

// Ч���б�
typedef struct Effect
{
	int cx;
	int cy;
    int nCellOrder;
	char strEffect[4];
	//
	struct Effect *pNext;
}XKCellEffect;

// ��˸��Ԫ
typedef struct Twinkle
{
	int cx1;
	int cy1;
	int cx2;
	int cy2;

	// ��˸����
	BYTE nTwinkleTime;
	BYTE nTwinkleNum;
    //
	struct Twinkle * pNext;
}XKCellTwinkle;


typedef struct _priority
{
	int8_t lbpri;
	int8_t strpri;
	int8_t gifpri;
	int8_t bmppri;
	int8_t pngpri;
}DSPriorit_t;



#define	 DSPTYPE_DEF	0
#define  DSPTYPE_IMG	1
#define  DSPTYPE_STR	2
#define  DSPTYPE_GIF	3
#define  DSPTYPE_LBD	4
#define  DSPTYPE_PNG	5
#define  DSPTYPE_JPG	6
#define  DSPTYPE_CGL	7
#define  DSPTYPE_CGS	8


typedef struct 
{
	uint16_t Cx;
	uint16_t Cy;
	
	uint8_t  LBnum;
	int (*LBandFunc)(int,int,int,int,uint8_t *);
}LIGHTBand_t;



//��ʾԪ�أ���ʾԪ��ָֻ����Ԫ�صģ����͡���ʾ���ꡢ��������ʽ�Լ�����ʾԪ�ص�����
typedef struct Dspnode
{
	uint8_t 		type;	//�����ʾ���ͣ�ͼƬ������
	uint8_t 		Sseq;	//���Ļ��
	uint8_t 		Lseq;	//����к�
	uint8_t 		time;	//ͣ��ʱ��
	uint8_t 	 	effIn;	//������ʽ
	uint8_t 		effOut;	//������ʽ
	
	uint16_t 		Cx;		//��ʾ������
	uint16_t 		Cy;		//��ʾ������
	uint16_t 		width;	//���ݿ��
	uint16_t 		height;	//���ݸ߶�
	
	XKCellString 	*XKCellStr;	//��������
	XKCellImage		*XKCellIma;	//ͼƬ����
	XKCellAnimate   *XKCellAni;	//��������
	LIGHTBand_t		*LIGHTBand;	//�������
	XKCellPngimg	*XKCellPng;	//pngͼƬ����

	int (*DSPfunc)(struct Dspnode *,uint8_t *);
	struct Dspnode	*pNext; 
}DSPNode_t;


	

// ��ʾ��Ԫ 
typedef struct Item
{
	uint8_t  nID;
	uint8_t  align;			//���뷽ʽ
	uint8_t  nItemType;

	// ��������
	uint8_t  IsHaveAnimate;
	char strAnimateName[20];

	// ���Ų���
	uint32_t nMoveSpeed;
	uint32_t nDelayTime;
	
	int nEffectIn;
	uint8_t nEffectOut;
	uint8_t nEffectShow;

	// �ռ䲼��
	uint8_t nMargin[4];

	//�������ֵ
	uint8_t FontMaxSize;

	//һĻ��Ϣ����ʾ������Ϣʱ��ռ����Ļ�����Ⱥ͸߶�
	uint8_t StrLines;
	uint16_t ScrMaxWidth;
	uint16_t ScrMaxHeight;

	//����Ϣ
	LineMst_t LineMst[16];
	
	// ��������
	XKCellTwinkle 	*pCellTwinkle_head,	*pCellTwinkle_tail;
	XKCellString 	*pCellString_head,	*pCellString_tail;
	XKCellImage 	*pCellImage_head,	*pCellImage_tail;
    XKCellAnimate 	*pCellAnimate_head,	*pCellAnimate_tail;
    XKCellEffect 	*pCellEffect_head,	*pCellEffect_tail;


	DSPNode_t		*DSPNode_head,	*DSPNode_tail;

	//��ǰ��ʾ����
	void			*Curplay;
	void			(*setcurplaying)(void *);
	
	//add by mo 2020.10.14
	//���ڴ�����һĻԭʼ������Ϣ
	uint8_t itemconent[512];
	
	struct Item 	*pNext;

}XKDisplayItem;






struct COLORREF
{
  BYTE byRed;
  BYTE byGreen;
  BYTE byBlue;
};


/**grup @refresh flag*/
enum{
	FLAG_NOT_REFRESH = 0,		//�����б��޸���
	FLAG_RESFRESH,				//�����б��и���
	LST_FREE,					//�����б�Ϊ��
	LST_REFLASH					//�����б��и���
	};
typedef struct
{
	uint32_t 		refresh;//refer to @refresh,indicate wether the playing list has been refresh or not
	uint16_t 		itemcount;
	XKDisplayItem 	*head;	//playing list head pointer
	XKDisplayItem 	*tail;	//playing list tail pointer
}ContentList;



// --------------------------------------------------------------------------------�豸����---------------------------------------------------//
struct XKBoxStatus                   // �����������
{
	BYTE nType[8];
	BYTE nData[8];		//���������¶�ֵ����ѹ1����ѹ2��.....��ѹ5���ſ���״̬����7���ֽڣ���8�ֽڱ���
	BYTE nTemperature;
	BYTE nVoltage[5];
	BYTE bDoorSwitch;
	BYTE bDriver[8];
	BYTE bDriverEx;		//����ͨ����ͨ��״̬
	BYTE bSystem;		//����ͨ����ϵͳ״̬
};

struct XKValueRange                 //   ������Χ
{
   unsigned int  Type;
   unsigned int  LowValue;
   unsigned int  HighValue;
   unsigned int  LowAlarm;
   unsigned int  HighAlarm;
};

struct XKDeviceAction
{
	BYTE IsPowerSwitchOn;  // ��Դ����״̬
	BYTE IsFanSwitchOn ;   // ���ȿ���״̬
	BYTE IsAlarmSwitchOn;  // ����������״̬
};


extern ContentList content;
BYTE DecodeItemString(ContentList *head,char *strItemContent);
void ClearContent(ContentList *head);
int XKDisplayInsert(XKDisplayItem *XKDisplay,int scenecnt);


int InitContentlist(ContentList *head);
int DecodePlayList2(ContentList *head,char *strPlayListName);
XKDisplayItem *GetItemHead(ContentList *head);
void Initcontentnode(XKDisplayItem *head);
int AddItemString(XKDisplayItem *head,XKCellString *pCellString);
int AddDisplayItem(ContentList *head,XKDisplayItem * pItem);
int AddItemDSPNode(XKDisplayItem *head,DSPNode_t * DSPNode);

int MXKDisplayInsert(XKDisplayItem *XKDisplay,int scenecnt);

void DSPrioritInit(void);
void SetDSPriorit(uint8_t type,uint8_t priority);
void GetDSPriorit(uint8_t type,uint8_t *priority);
#ifdef __cplusplus
}
#endif

#endif /* _DMP_DOS_Xktypes_H_ */

