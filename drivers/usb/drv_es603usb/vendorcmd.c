
#include <linux/kernel.h>
#include <linux/errno.h>

#include "vendorcmd.h"

#include "AlphaTec.h"
#include "EgisTec.h"
#include "USBComm.h"

//======================================================================//
//==================== Function declaration ============================//

//------------------------- Init command -------------------------------//
static void Init_stdCBWHdr(PCBW_Form stdCBW);


//------------------------- Power function -----------------------------//
static int PowerProc(Device_Vendor Dev_vendor, usb_ss801u *dev, Register_Func Reg_fn, bool bON);
static int PowerSwitch(Device_Vendor Dev_vendor, usb_ss801u *dev,bool bON);

//------------------------- Helper function ----------------------------//
static void Vendor_Fill_ReadReg_Cmd(Device_Vendor Dev_vendor, PCBW_Form pCBW, 
									Register_Func Reg_fn, unsigned int BufLen);

static void Vendor_Fill_WriteReg_Cmd(Device_Vendor Dev_vendor, PCBW_Form pCBW, 
									 Register_Func Reg_fn, bool bON, 
									 unsigned char* Data, unsigned int DataLen);
//======================================================================//



//======================== Internal function =================================//
//============================================================================//

static int PowerSwitch(Device_Vendor Dev_vendor, usb_ss801u *dev,bool bON)
{
    if(true == bON){
		if(PowerProc(Dev_vendor, dev, PWR_GPIOPower, false)) return -1;
		if(PowerProc(Dev_vendor, dev, PWR_FPPower,   true)) return -1;
    }
    else{
		if(PowerProc(Dev_vendor, dev, PWR_FPPower,   false)) return -1;
    }
    return 0;
}

static int PowerProc(Device_Vendor Dev_vendor, usb_ss801u *dev, Register_Func Reg_fn, bool bON)
{
    int iRet;
    CBW_Form PwrCBW;
    unsigned char Data[512];
    unsigned int DataLen=512;

    Init_stdCBWHdr(&PwrCBW);

    //____Step1. Read register
	Vendor_Fill_ReadReg_Cmd(Dev_vendor, &PwrCBW, Reg_fn, DataLen);
    iRet = Egis_SndSCSICmd(	dev, 					// device infomation
		    	   			&PwrCBW, 				// CBW
		    	   			Data, DataLen, dir_in, 	// data infomation
		    	   			NULL,					// CSW
							5000					// timeout
		    	  			);


    //____Step2. Write register
    Vendor_Fill_WriteReg_Cmd(Dev_vendor, &PwrCBW, Reg_fn, bON, Data, 512);
    if(0 == iRet)
		iRet = Egis_SndSCSICmd(	dev, 					// device infomation
					   			&PwrCBW, 				// CBW
					   			Data, DataLen, dir_out,	// data infomation
					   			NULL,					// CSW
								5000					// timeout
					  			);

    return iRet;
}

static void Vendor_Fill_ReadReg_Cmd(Device_Vendor Dev_vendor, PCBW_Form pCBW, 
									Register_Func Reg_fn, unsigned int BufLen)
{
    switch(Dev_vendor){
		case USBEST_801u:
	    	AlphaTec_Fill_ReadReg_Cmd(pCBW, Reg_fn, BufLen);
			break;
		case MOAI_802u:
	     
			break;

 		case DEVICE_NOTSUPPORT:
		default:
			err("Vendor device is not supported!! \r\n");
			break;
   } 
}

static void Vendor_Fill_WriteReg_Cmd(Device_Vendor Dev_vendor, PCBW_Form pCBW, 
									 Register_Func Reg_fn, bool bON, 
									 unsigned char* Data, unsigned int DataLen)
{
    switch(Dev_vendor){
		case USBEST_801u:
	    	AlphaTec_Fill_WriteReg_Cmd(pCBW, Reg_fn, bON, Data, DataLen);
			break;
		case MOAI_802u:
	     
			break;

 		case DEVICE_NOTSUPPORT:
		default:
			err("Vendor device is not supported!! \r\n");
			break;
   } 
}

static void Init_stdCBWHdr(PCBW_Form stdCBW)
{
    memset(stdCBW, 0, CBW_SIZE);

    stdCBW->USBC[0] = 0x55;    stdCBW->USBC[1] = 0x53;
    stdCBW->USBC[2] = 0x42;    stdCBW->USBC[3] = 0x43; // USBC
    stdCBW->Tag[0]  = 0x08;    stdCBW->Tag[1]  = 0x09; // TAG
    stdCBW->Tag[2]  = 0xAA;    stdCBW->Tag[3]  = 0xBB;
}

//==============================================================================//
//==============================================================================//


void Egis_Fill_Normalmode_Cmd(Device_Vendor Dev_vendor,PCBW_Form norCBW)
{
    Init_stdCBWHdr(norCBW);

    switch(Dev_vendor){
		case USBEST_801u:
	    	AlphaTec_Fill_Normalmode_Cmd(norCBW);
			break;
		case MOAI_802u:
	     
			break;

		case EGIS_603:
			//Wayne
//  		EgisTec_Fill_Normalmode_Cmd(norCBW);
			break;

 		case DEVICE_NOTSUPPORT:
		default:
			err("Vendor device is not supported!! \r\n");
			break;
   }    
}

void Egis_Fill_Contactmode_Cmd(Device_Vendor Dev_vendor,PCBW_Form conCBW)
{
    Init_stdCBWHdr(conCBW);

    switch(Dev_vendor){
		case USBEST_801u:
		    AlphaTec_Fill_Contactmode_Cmd(conCBW);
			break;
		case MOAI_802u:
		     
			break;

		case EGIS_603:
			//Wayne: useless in ES603
//  		EgisTec_Fill_Contactmode_Cmd(conCBW);
			break;

		case DEVICE_NOTSUPPORT:
		default:
			err("Vendor device is not supported!! \r\n");
			break;
    }     
}

int Egis_SndContactModeCmd(Device_Vendor Dev_vendor, usb_ss801u *dev, unsigned char* CSW)
{
    int retval=0;
	int bytes_write = 0;
	int bytes_read = 0;
	unsigned char inBuffer[6];

#ifndef BEFORE_ES603
		
	if(Dev_vendor == EGIS_603) 
	{
		return 0;
	}

#endif
	//____step1. pre-set contact mode
	switch(Dev_vendor)
	{
		case USBEST_801u:
		retval = PowerSwitch(Dev_vendor, dev, true);
			break;

		case EGIS_603: //No need to do PowerSwitch in 603
			
			break;
		case DEVICE_NOTSUPPORT:
		default:
			err("Vendor device is not supported!! \r\n");
			break;
	}

    
	//____step2. set contact mode command
	switch(Dev_vendor)
	{
		case USBEST_801u:
    	if(0==retval)
    		retval = Egis_SndSCSICmd(dev, 			  	// device infomation
		    	 	       	 		 &dev->contactCBW,  // CBW
			  				     	 0, 0, dir_out,		// data infomation
							     	 CSW,				// CSW
									 5000				// timeout		
					    			);
			break;
		case EGIS_603:
			//send command to host
			retval = Egis_BulkXfer(
				dev,
	  			dir_out,
       			(void*)&dev->contactCBW,
       			31,
       			&bytes_write, 
				5000
			 );
			//read data
			if(0 == retval)
				{
					retval = Egis_BulkXfer(
					dev,
	  				dir_in,
       				(void*)inBuffer,
       				6,
       				&bytes_read, 
					5000
			 		);
				}
			else
				{
					err("USB bulk transfer fail! \r\n");
				}
			break;
		case DEVICE_NOTSUPPORT:
		default:
			err("Vendor device is not supported!! \r\n");
			break;
	}
	//____step3. post-set contact mode command
    switch(Dev_vendor){
		case USBEST_801u:
		    AlphaTec_SwitchIOPwrOff(dev);
			break;
		case MOAI_802u:
		     
			break;
		case EGIS_603:
			break;

		case DEVICE_NOTSUPPORT:
		default:
			err("Vendor device is not supported!! \r\n");
			break;
    }

    return retval;
}

int Egis_SndNormalModeCmd(Device_Vendor Dev_vendor, usb_ss801u *dev, unsigned char* CSW)
{
    return  Egis_SndSCSICmd(dev, 				// device infomation
		     	     		&dev->normalCBW,	// CBW
			  			    0, 0, dir_out,		// data infomation
			     			CSW,			  	// CSW
							5000				// timeout
				    	   );
}


