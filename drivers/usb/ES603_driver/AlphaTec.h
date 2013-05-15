#ifndef _ALPHATEC_H_
#define _ALPHATEC_H_

#include <linux/egistec/linux_driver.h>
#include "vendorcmd.h"


//----------------- Fill CBW with contact/normal mode command  -----------------//
//------------------------------------------------------------------------------//
void AlphaTec_Fill_Normalmode_Cmd(PCBW_Form pnorCBW);
void AlphaTec_Fill_Contactmode_Cmd(PCBW_Form pconCBW);


//----------------- Fill CBW with command that you want to set -----------------//
//------------------------------------------------------------------------------//
void AlphaTec_Fill_ReadReg_Cmd(PCBW_Form pCBW, Register_Func Reg_fn, unsigned int BufLen);
void AlphaTec_Fill_WriteReg_Cmd(PCBW_Form pCBW, Register_Func Reg_fn, bool bON, unsigned char* Data, unsigned int DataLen);


int AlphaTec_SwitchIOPwrOff(usb_ss801u *dev);
#endif
