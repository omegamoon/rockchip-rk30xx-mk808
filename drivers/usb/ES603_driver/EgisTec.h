#ifndef _EGISTEC_H_
#define _EGISTEC_H_

#include <linux/egistec/linux_driver.h>
#include "vendorcmd.h"


//----------------- Fill CBW with contact/normal mode command  -----------------//
//------------------------------------------------------------------------------//
void EgisTec_Fill_Normalmode_Cmd(PCBW_Form pnorCBW);
void EgisTec_Fill_Contactmode_Cmd(PCBW_Form pconCBW);


//----------------- Fill CBW with command that you want to set -----------------//
//------------------------------------------------------------------------------//
void EgisTec_Fill_ReadReg_Cmd(PCBW_Form pCBW, Register_Func Reg_fn, unsigned int BufLen);
void EgisTec_Fill_WriteReg_Cmd(PCBW_Form pCBW, Register_Func Reg_fn, bool bON, unsigned char* Data, unsigned int DataLen);


int EgisTec_SwitchIOPwrOff(usb_ss801u *dev);
#endif

