
#include "AlphaTec.h"
#include "vendorcmd.h"
#include "USBComm.h"
#include <linux/egistec/utility.h>


//======================================================================//
//================= Internal Function declaration ======================//
static void Fill_WriteReg_Cmd(PCBW_Form pCBW, unsigned char HAddr, unsigned char LAddr, unsigned int BufLen);
static void Fill_ReadReg_Cmd(PCBW_Form pCBW, unsigned char HAddr, unsigned char LAddr, unsigned int BufLen);
static void Fill_SCSIVendor_Cmd(
			PCBW_Form pCBW, 
			__u8 RWDir , __u8 MemType, 
			__u8 HAddr, __u8 LAddr, 
			__u8 xHLen, __u8 xLLen);

static int Read_Write_Register(
			usb_ss801u *dev, 
			__u8 RWDir , __u8 MemType, 
			__u8 HAddr, __u8 LAddr, 
			__u8 xHLen, __u8 xLLen, 
			unsigned char* Buffer , unsigned int BufLen);
//======================================================================//



//======================== Internal function =================================//
//============================================================================//

static void Fill_WriteReg_Cmd(PCBW_Form pCBW, unsigned char HAddr, unsigned char LAddr, unsigned int BufLen)
{
    pCBW->Direction = SCSI_OUT;
    pCBW->CommandLength = 10;
    pCBW->Length = BufLen;

	Fill_SCSIVendor_Cmd(pCBW, 0xDA, 0x23, HAddr, LAddr, 0x0, 0x1);
}

static void Fill_ReadReg_Cmd(PCBW_Form pCBW,  unsigned char HAddr, unsigned char LAddr, unsigned int BufLen)
{
    pCBW->Direction = SCSI_IN;
    pCBW->CommandLength = 10;
    pCBW->Length = BufLen;

	Fill_SCSIVendor_Cmd(pCBW, 0xD2, 0x13, HAddr, LAddr, 0x0, 0x1);
}

static void Fill_SCSIVendor_Cmd(
			PCBW_Form pCBW, 
			__u8 RWDir , __u8 MemType, 
			__u8 HAddr, __u8 LAddr, 
			__u8 xHLen, __u8 xLLen)
{
    pCBW->VendCmd[ 0] = RWDir;  // Read memory command
    pCBW->VendCmd[ 1] = 0x00;	// it must be 0
    pCBW->VendCmd[ 2] = MemType;// memory type(0x12->8051 idata, 0x13->register, 0x19->write SPI_NORFlash, 0x1A->8051 SFR
    pCBW->VendCmd[ 3] = 0x00;	// it must be 0
    pCBW->VendCmd[ 4] = HAddr;	// memory address(MSB)
    pCBW->VendCmd[ 5] = LAddr;	// memory address(LSB)
    pCBW->VendCmd[ 6] = 0x0;	// it must be 0
    pCBW->VendCmd[ 7] = xHLen;	// transfer length(MSB)
    pCBW->VendCmd[ 8] = xLLen;	// transfer length(LSB)
    pCBW->VendCmd[ 9] = 0x00; 	// it must be 0
}

static int Read_Write_Register(
			usb_ss801u *dev, 
			__u8 RWDir , __u8 MemType, 
			__u8 HAddr, __u8 LAddr, 
			__u8 xHLen, __u8 xLLen, 
			unsigned char* Buffer , unsigned int BufLen)
{
	int iRet;
	trans_dir xDir;
	CBW_Form CBW;
	memcpy(&CBW, &dev->normalCBW, CBW_SIZE);

	CBW.Length = BufLen;
	CBW.CommandLength = 10;
	if(0xD2 == RWDir){ // 0xD2 : Read
		xDir = dir_in;
		CBW.Direction = SCSI_IN;
	}
	else if(0xDA == RWDir){ // 0xDA : Write
		xDir = dir_out;
		CBW.Direction = SCSI_OUT;
	}
	else {
		return -1;
	}

	Fill_SCSIVendor_Cmd(&CBW, RWDir, MemType, HAddr, LAddr, xHLen, xLLen);
	iRet = Egis_SndSCSICmd(	dev, 					// device infomation
		    	   			&CBW, 					// CBW
		    	   			Buffer, BufLen, xDir,	// data infomation
		    	   			NULL,					// CSW
							5000					// timeout
		    	  			);
	return iRet;
}

//======================================================================================//
//======================================================================================//

int AlphaTec_SwitchIOPwrOff(usb_ss801u *dev)
{
	int retval=0;
	int buf_size=512;
	unsigned char* buffer = kzalloc(buf_size, GFP_KERNEL);
	

	//_____	0x300
	retval = Read_Write_Register(dev, 0xD2, 0x13, 0x03, 0x00, 0x00, 0x01, buffer, buf_size);
	buffer[0] &= 0xBF;
	if(EGIS_SUCCESS(retval)){
		retval = Read_Write_Register(dev, 0xDA, 0x23, 0x03, 0x00, 0x00, 0x01, buffer, buf_size);
	}

	//____	0x301
	if(EGIS_SUCCESS(retval)){
		retval = Read_Write_Register(dev, 0xD2, 0x13, 0x03, 0x01, 0x00, 0x01, buffer, buf_size);
		buffer[0] &= 0xFB;
		if(EGIS_SUCCESS(retval)){
			retval = Read_Write_Register(dev, 0xDA, 0x23, 0x03, 0x01, 0x00, 0x01, buffer, buf_size);
		}
	}

	//____	0x0302 Off Bit5 to 0
	if(EGIS_SUCCESS(retval)){
		retval = Read_Write_Register(dev, 0xD2 ,0x13, 0x03, 0x02, 0x00, 0x01, buffer, buf_size);
		buffer[0] &= 0xDF;
		if(EGIS_SUCCESS(retval)){
			retval = Read_Write_Register(dev, 0xDA, 0x23, 0x03, 0x02, 0x00, 0x01, buffer, buf_size);
		}
	}

	//____	0x30c ~0x30f
	if(EGIS_SUCCESS(retval)){
		buffer[0] = 0x0;
		buffer[1] = 0x0;
		buffer[2] = 0x0;
		buffer[3] = 0x0;
		retval = Read_Write_Register(dev, 0xDA ,0x23, 0x03, 0x0C, 0x00, 0x04, buffer, buf_size);
	}

	//____	0x310 ~ 0x313
	if(EGIS_SUCCESS(retval)){
		buffer[0] = 0xFF;
		buffer[1] = 0xFF;
		buffer[2] = 0xFF;
		buffer[3] = 0xFF;
		retval = Read_Write_Register(dev, 0xDA ,0x23, 0x03, 0x10, 0x00, 0x04, buffer, buf_size);
	}

	//____	0x80
	if(EGIS_SUCCESS(retval)){
		buffer[0] = 0xFF;
		retval = Read_Write_Register(dev, 0xDA ,0x2A, 0x00, 0x80, 0x00, 0x01, buffer, buf_size);
	}

	//____	0x90
	if(EGIS_SUCCESS(retval)){
		buffer[0] = 0xFF;
		retval = Read_Write_Register(dev, 0xDA ,0x2A, 0x00, 0x90, 0x00, 0x01, buffer, buf_size);
	}

	//____	0xA0
	if(EGIS_SUCCESS(retval)){
		buffer[0] = 0xFF;
		retval = Read_Write_Register(dev, 0xDA ,0x2A, 0x00, 0xA0, 0x00, 0x01, buffer, buf_size);
	}

	//____	0xB0
	if(EGIS_SUCCESS(retval)){
		buffer[0] = 0xFF;
		retval = Read_Write_Register(dev, 0xDA ,0x2A, 0x00, 0xB0, 0x00, 0x01, buffer, buf_size);
	}

	if(buffer) kfree(buffer);

	return retval;
}



void AlphaTec_Fill_ReadReg_Cmd(PCBW_Form pCBW, Register_Func Reg_fn, unsigned int BufLen)
{
    unsigned char AddrLSB=0;

    //____Step1. Select function
    switch(Reg_fn)
    {
		case PWR_FPPower:
			AddrLSB = 0x04;
		break;

		case PWR_GPIOPower:
			AddrLSB = 0x05;
		break;	

		case GEN_AssignGPIOPin:
			AddrLSB = 0x00;
		break;
    }

    Fill_ReadReg_Cmd(pCBW,  0x03, AddrLSB, BufLen);
}

void AlphaTec_Fill_WriteReg_Cmd(PCBW_Form pCBW, Register_Func Reg_fn, bool bON, unsigned char* Data, unsigned int DataLen)
{
    unsigned char AddrLSB=0;

    switch(Reg_fn)
    {
		case PWR_FPPower:
			AddrLSB = 0x04;
			if(bON) Data[0] |= 0x01; //bit0=1 => FPPower pull high 
			else    Data[0] &= 0xFE; //bit0=0 => FPPower pull low
		break;

		case PWR_GPIOPower:
			AddrLSB = 0x05;
			if(bON) Data[0] |= 0x10; //bit4=0 => Enable GPIO放電 
			else    Data[0] &= 0xEF; //bit4=0 => Disable GPIO放電
		break;	

		case GEN_AssignGPIOPin:
			AddrLSB = 0x0;
			Data[0] &= 0xBF;	//bit6=0 => Assign Pin to be GPIO
		break;
    }

    Fill_WriteReg_Cmd(pCBW, 0x03, AddrLSB, DataLen);
}



void AlphaTec_Fill_Normalmode_Cmd(PCBW_Form pnorCBW)
{
    pnorCBW->Length = 0;
    pnorCBW->Direction = SCSI_OUT;
    pnorCBW->CommandLength = 10;

    pnorCBW->VendCmd[ 0] = 0xCF;   pnorCBW->VendCmd[ 1] = 0x20;
    pnorCBW->VendCmd[ 2] = 0x0;    pnorCBW->VendCmd[ 3] = 0x0;
    pnorCBW->VendCmd[ 4] = 0xC4;   pnorCBW->VendCmd[ 5] = 0xB0;
    pnorCBW->VendCmd[ 6] = 0x05;   pnorCBW->VendCmd[ 7] = 0xC0;
    pnorCBW->VendCmd[ 8] = 0xC0;   pnorCBW->VendCmd[ 9] = 0x0; 

}

void AlphaTec_Fill_Contactmode_Cmd(PCBW_Form pconCBW)
{
    pconCBW->Length = 0;
    pconCBW->Direction = SCSI_OUT;
    pconCBW->CommandLength = 10;

    pconCBW->VendCmd[ 0] = 0xCF;   pconCBW->VendCmd[ 1] = 0x28;
    pconCBW->VendCmd[ 2] = 0x81;   pconCBW->VendCmd[ 3] = 0x0;
    pconCBW->VendCmd[ 4] = 0xC4;   pconCBW->VendCmd[ 5] = 0xB0;
    pconCBW->VendCmd[ 6] = 0x05;   pconCBW->VendCmd[ 7] = 0xC0;
    pconCBW->VendCmd[ 8] = 0xC0;   pconCBW->VendCmd[ 9] = 0x0; 
}

