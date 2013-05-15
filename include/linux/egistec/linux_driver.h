#ifndef _LINUX_DIRVER_H_
#define _LINUX_DIRVER_H_

#include <asm/ioctl.h>

#define SIGIO_RESUME     SIGRTMIN+1
#define SIGIO_SUSPEND    SIGRTMIN+2
#define SIGIO_DISCONNECT SIGRTMIN+3

typedef enum _SIG_TYPE
{
	SIG_UNKNOW		= 0,
	SIG_RESUME 		= 1,
	SIG_DISCONNECT 	= 2,

	SIG_SUSPEND 	= 3	// only for test tool, SDK do not use it
}Sig_Type;

//================================ IOCTL =======================================//
//==============================================================================//
/* Define size of CBW, CSW*/
#define CBW_SIZE 31
#define CSW_SIZE 13

/* Define ioctl code*/
#define EGIS_IOCTL_MAXNR	17

#define EGIS_IOCTL_MAGIC				'S'
#define EGIS_IOCTL_SCSI_READ			_IOW(EGIS_IOCTL_MAGIC,  0, int)
#define EGIS_IOCTL_SCSI_WRITE			_IOW(EGIS_IOCTL_MAGIC,  1, int)

#define EGIS_IOCTL_ENTER_SUSPEND		_IO( EGIS_IOCTL_MAGIC,  2	  )
#define EGIS_IOCTL_SET_AUTOSUSPEND_TIME	_IOW(EGIS_IOCTL_MAGIC,  3, int) // in second

#define EGIS_IOCTL_RESET_DEVICE 		_IO( EGIS_IOCTL_MAGIC,  4	  )

#define EGIS_IOCTL_SET_NORMALMODE_REG  	_IOW(EGIS_IOCTL_MAGIC,  5, int)
#define EGIS_IOCTL_SET_CONTACTMODE_REG 	_IOW(EGIS_IOCTL_MAGIC,  6, int)

#define EGIS_IOCTL_CREATE_SIGNAL 		_IOW(EGIS_IOCTL_MAGIC,  7, int)	// pass one of Sig_Type type

//----- FOR Debug -----//
#define EGIS_IOCTL_ENABLE_DBG_MSG 		_IOW(EGIS_IOCTL_MAGIC,  8, int)

//----- FOR TEST -----//
#define EGIS_IOCTL_TS_SWITCH_AUTOSUSPEND _IOW(EGIS_IOCTL_MAGIC, 9, int)
#define EGIS_IOCTL_TS_SWITCH_RMWAKEUP 	_IOW(EGIS_IOCTL_MAGIC, 10, int)
#define EGIS_IOCTL_TS_SIGNAL 			_IO(EGIS_IOCTL_MAGIC,  11	  )
//--------------------//

#define EGIS_IOCTL_GET_VERSION  		_IOR(EGIS_IOCTL_MAGIC, 12, int)

#define EGIS_IOCTL_BULK_READ			_IOW(EGIS_IOCTL_MAGIC, 13, int)
#define EGIS_IOCTL_BULK_WRITE			_IOW(EGIS_IOCTL_MAGIC, 14, int)
#define EGIS_IOCTL_CTRLXFER_READ		_IOW(EGIS_IOCTL_MAGIC, 15, int)
#define EGIS_IOCTL_CTRLXFER_WRITE		_IOW(EGIS_IOCTL_MAGIC, 16, int)

#define EGIS_IOCTL_WRITELOG             _IOW(EGIS_IOCTL_MAGIC, 17, int)

//--------------------- SCSI command -------------------------//
//#pragma pack( push, before_include1 )
#pragma pack(1)
typedef struct _CBW_FORM_
{
	unsigned char	USBC[4];
	unsigned char	Tag[4];
	unsigned int	Length;
	unsigned char	Direction;
	unsigned char	LUN;
	unsigned char	CommandLength;
	unsigned char	VendCmd[16];
}CBW_Form, *PCBW_Form;


/* scsi transfer struct */
typedef struct _SCSI_BUFFER
{
    CBW_Form 	   CBW;
    unsigned char* data;
    unsigned short datasize;
    unsigned char  CSW[CSW_SIZE];
	unsigned long  timeout;		// in jiffies, 0 means infinite
}SCSICmd_Buf, *PSCSICmd_Buf;

/* bulk transfer stuct */
typedef struct _BULK_BUFFER
{
	unsigned short DataSize;
	unsigned long  Timeout;		// in juffies, 0 means infinite
	unsigned char* Data;
}BULK_BUF, *PBULK_BUF;

/* Ctrl transfer package */
typedef	struct _CTRL_SETUP_PACKAGE
{
	unsigned char		bRequest;
	unsigned char		bRequestType;
	unsigned short		wValue;
	unsigned short		wIndex;
	unsigned short		wLength;
}CTRL_SETUP_PACKAGE, *PCTRL_SETUP_PACKAGE;

typedef struct _CTRLXFER_PACKAGE
{
	CTRL_SETUP_PACKAGE 	SetupPacket;
	unsigned char		DataPacket[64];
	unsigned long  		Timeout;		// in juffies, 0 means infinite
}CTRLXFER_PACKAGE, *PCTRLXFER_PACKAGE;

typedef struct _LOG_PACKAGE
{
	unsigned long ulDataSize;
	char * szLog;
}LOG_PACKAGE, *PLOG_PACKAGE;

#pragma pack()

//#pragma pack( pop, before_include1 )
//-------------------------------------------------------------//

//-------------------------- Version --------------------------//
typedef struct _DRIVER_VERSION{
    unsigned int vnumber;
    char 	 vstring[16]; //xx.xx.xx.xx
}FPDriver_Version;
//-------------------------------------------------------------//


//==============================================================================//
//==============================================================================//


#endif
