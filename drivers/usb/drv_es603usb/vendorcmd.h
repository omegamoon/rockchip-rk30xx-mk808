#ifndef _VENDOR_CMD_
#define _VENDOR_CMD_

#include <linux/egistec/linux_driver.h>
#include "driver_usb.h"


//----- used in urb for transfer direction
#define SCSI_IN  0x80
#define SCSI_OUT 0x00


//----- For set register
typedef enum _REGISTER_FUNC
{
    PWR_FPPower		= 0,
    PWR_GPIOPower	= 1,
    GEN_AssignGPIOPin 	= 2
}Register_Func;


//----------------- Fill CBW with contact/normal mode command  -----------------//
//------------------------------------------------------------------------------//
void Egis_Fill_Contactmode_Cmd(Device_Vendor Dev_vendor, PCBW_Form conCBW);
void Egis_Fill_Normalmode_Cmd(Device_Vendor Dev_vendor, PCBW_Form norCBW);


//----------------- send contact/normal mode command to device -----------------//
//------------------------------------------------------------------------------//
int Egis_SndContactModeCmd(Device_Vendor Dev_vendor, usb_ss801u *dev, unsigned char* CSW);
int Egis_SndNormalModeCmd(Device_Vendor Dev_vendor, usb_ss801u *dev, unsigned char* CSW);

#endif
