/****************************************************************************/
/* Project		: A Sample Code for Mass Storage Class  	    */
/* Date			: 2008-11-18					    */
/* Written by	        : Sky K.Y. Shyu      			            */
/* Compiler		: Keil C51 V7.04c				    */
/* Code Version	: V1.00							    */
/****************************************************************************/
#ifndef __UNSIGNED_DATA_TYPE__
#define __UNSIGNED_DATA_TYPE__
#define UCHAR   unsigned char
#define ULONG   unsigned long
#define UINT    unsigned int
#endif

//#define	DEBUG			0
#define TESTBUF			0
#define DEBUG_BULK		0
#define	DEBUG_INT		0
#define	DEBUG_JADE		0
#define	DEBUG_WD		0
#define	DEBUG_SUSPEND	0
#define	DEBUG_RECON		0

#define	WD_ENABLE	1

#define	SYS_CLK				24000000			//24000000Hz
#define TM0_MS				30					//30ms
#define	TM0_TICK_CNT		(65536-(TM0_MS*SYS_CLK/12000))
//#define	CARD_ACCESS_TIMER	2000/TM0_MS
//#define min(a,b) (((a)<(b))?(a):(b))

//==============================================================
//				Recon Parameter Define
//==============================================================
#define PADDING_SIZE			256
#define PADDING_SIZE_HALF		PADDING_SIZE/2
#define NO_PADDING_SIZE			192
#define NO_PADDING_SIZE_HALF	NO_PADDING_SIZE/2
#define UP_RECON				1
#define DOWN_RECON				2
#define DUAL_RECON				3
#define RECON_PADDING_ON		1
#define RECON_PADDING_OFF		0
#define MAX_PACKAGE_SIZE		64

#define NORMAL_MODE				1

//==============================================================
//				Bit Position Define
//==============================================================
#define	BIT0			0x01
#define	BIT1			0x02
#define BIT2			0x04
#define BIT3			0x08
#define BIT4			0x10
#define BIT5			0x20
#define BIT6			0x40
#define BIT7			0x80

//==============================================================
//				Chip Select Define
//==============================================================
#define MARCH_CS		BIT0
#define FLASH_CS		BIT1

//==============================================================
//				March OP Code Define
//==============================================================
#define MARCH_ADDRESS_0			0x00	//0000 0000
#define MARCH_WRITE_ADDRESS		0xAC	//1010 1100
#define MARCH_READ_DATA			0xAF	//1010 1111
#define	MARCH_WRITE_DATA		0xAE	//1010 1110

//==============================================================
//				Error Code Define
//==============================================================
#define SUCCESS					0x01
#define TIME_OUT				0x03
#define ERASE_TYPE_FAIL			0x04
#define FLASH_TYPE_FAIL			0x05
//#define NO_SUPPORT				0x99

//==============================================================
//				General Define
//==============================================================
//#define READ					1
//#define WRITE					2

#define GOOD_STATUS             1
#define FAIL_STATUS             0

#define FAIL                    0
#define	FALSE					0
#define TRUE                    1
#define ERROR                   2

//==============================================================
//				Other Define
//==============================================================

/*
#define	cSenIdxNoSense		0		//sense data index
#define	cSenIdxNoMedia		1
#define	cSenIdxReadEccErr	2
#define	cSenIdxUnsupport	3
#define	cSenIdxFieldError	4
#define	cSenIdxMediaChange	5
#define	cSenIdxLunError		6
#define	cSenIdxWritProtect	7
#define	cSenIdxConfigNow	8
#define	cSenIdxPasswdFail	9
#define	cSenIdxOutOfRetry	10
#define	cSenIdxNoSecurity	11
#define	cSenIdxDeviceLock	12
#define	cSenIdxSpecRWLock	13
#define	cSenIdxConfigBTnow	14
#define	cSenIdxConfigBTokay	15
#define	cSenIdxConfigBTfail	16
#define	cSenIdxBTinvalid	17
#define	cSenIdxVendorSigErr	18

#define KEY_NO_SENSE            0
#define ASC_NO_SENSE            0
#define ASCQ_NO_SENSE           0
#define KEY_WR_PROTECT          0x07
#define ASC_WR_PROTECT          0x027
#define ASCQ_WR_PROTECT         0
#define KEY_UNSUPPORT           0x05
#define ASC_UNSUPPORT           0x020
#define ASCQ_UNSUPPORT          0

#define CARD_CHANGE				0x01
#define CARD_READY				0x02
#define WRITE_ERR				0x04
#define WRITE_PROT				0x08
*/

typedef union {
    ULONG ul;
    UCHAR uc[4];
} UN_LONG;

typedef union {
    UINT ui;
    UCHAR uc[2];
} UN_INT;

/*
typedef union {
    ULONG ulSecAddr;
    UCHAR ucSecAddr[4];
} SCSI_SEC_ADDR;
*/

/*
typedef union {
    UINT  uiSecCnt;
    UCHAR ucSecCnt[2];
} SCSI_SEC_CNT;
*/

//typedef struct {
//    UN_LONG         unCommandLen;
//    UN_LONG         unResponseLen;
//    UCHAR       ucType;
//} USB_PARA;
//
//typedef struct {
//    UCHAR bmRequestType;
//    UCHAR bRequest;
//    char  (*function)();
//} USB_REQUEST_DEF;
//
//typedef struct {
//    UCHAR EgisCmdCode;
//    char  (*function)();
//} EGIS_CMD_REC;
/*
typedef struct {
    UCHAR Sig[4];
    UN_LONG ComLen;	//+4
    UN_LONG RespLen;	//+8
    UCHAR Type;         //+12
			//+13 command, +14 payload data
} USB_COMM;

typedef struct {
    UCHAR Sig[4];
    UCHAR PackType;     //+4
    UCHAR RetValue;     //+5
			//+6 payload data 
} USB_RESP;
*/
