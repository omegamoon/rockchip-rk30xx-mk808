#include "EgisTec.h"
#include "vendorcmd.h"
#include "USBComm.h"
#include <linux/egistec/utility.h>

void EgisTec_Fill_Contactmode_Cmd(PCBW_Form pconCBW)
{
	//Wayne for test
//  char *pchCmd = pconCBW;
//
//  pchCmd[ 0] = 0x45; //'E'
//  pchCmd[ 1] = 0x47; //'G'
//  pchCmd[ 2] = 0x49; //'I'
//  pchCmd[ 3] = 0x53; //'S'
//  pchCmd[ 4] = 0x09; //Package type, always 0x09
//  pchCmd[ 5] = 0x02; //operation code. 0x01: Read register. 0x02: Write register. 0x03: Get image. 0x05: Get reconstruction image
//  pchCmd[ 6] = 0x12; //number
//  pchCmd[ 7] = 0xE6;
//  pchCmd[ 8] = 0x29; //Jade DC Offset
//  pchCmd[ 9] = 0xE0;
//  pchCmd[10] = 0x04; //Jade Gain
//  pchCmd[11] = 0xE1;
//  pchCmd[12] = 0x0E; //Jade VRT
//  pchCmd[13] = 0xE2;
//  pchCmd[14] = 0x02; //Jade VRB
//  pchCmd[15] = 0xE5;
//  pchCmd[16] = 0x13;
//  pchCmd[17] = 0xF0;
//  pchCmd[18] = 0x01;
//  pchCmd[19] = 0xF2;
//  pchCmd[20] = 0x4E;
//  pchCmd[21] = 0x50;
//  pchCmd[22] = 0x8F;
//  pchCmd[23] = 0x59;
//  pchCmd[24] = 0x18;
//  pchCmd[25] = 0x5A;
//  pchCmd[26] = 0x08;
//  pchCmd[27] = 0x5B;
//  pchCmd[28] = 0x10;
//  pchCmd[29] = 0x02;
//  pchCmd[30] = 0x11;
}

void EgisTec_Fill_Normalmode_Cmd(PCBW_Form pnorCBW)
{
	EgisTec_Fill_Contactmode_Cmd(pnorCBW);
}