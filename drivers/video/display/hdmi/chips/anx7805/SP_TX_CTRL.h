// ---------------------------------------------------------------------------
// Analogix Confidential Strictly Private
//
//
// ---------------------------------------------------------------------------
// >>>>>>>>>>>>>>>>>>>>>>>>> COPYRIGHT NOTICE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// ---------------------------------------------------------------------------
// Copyright 2004-2010 (c) Analogix 
//
//Analogix owns the sole copyright to this software. Under international
// copyright laws you (1) may not make a copy of this software except for
// the purposes of maintaining a single archive copy, (2) may not derive
// works herefrom, (3) may not distribute this work to others. These rights
// are provided for information clarification, other restrictions of rights
// may apply as well.
//
// This is an unpublished work.
// ---------------------------------------------------------------------------
// >>>>>>>>>>>>>>>>>>>>>>>>>>>> WARRANTEE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// ---------------------------------------------------------------------------
// Analogix  MAKES NO WARRANTY OF ANY KIND WITH REGARD TO THE USE OF
// THIS SOFTWARE, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR
// PURPOSE.
// ---------------------------------------------------------------------------

#ifndef _SP_TX_DRV_DEF_H
#define _SP_TX_DRV_DEF_H
#include "SP_TX_DRV.h"


extern SP_LINK_BW sp_tx_bw;
extern struct LT_Result LTResult;
extern BYTE Polling_Disable;
extern BYTE sp_tx_pd_mode; //ANX7805 power state flag
//extern struct Packet_AVI SP_TX_Packet_AVI; 
extern BYTE sp_tx_charge_mode_state;// 0: no state 1:slimport state  2:USB state
extern BYTE sp_tx_hw_lt_done;//hardware linktraining done indicator
extern BYTE sp_tx_hw_lt_enable;//hardware linktraining enable

extern BYTE ext_int_index;

#define	EmbededSync     1
#define	SeparateSync     0
#define	NoDE     1
#define	SeparateDE     0
#define	YCMUX    1
#define	UnYCMUX     0
#define	DDR    1
#define	SDR     0
#define	Negedge    1
#define	Posedge       0
 

typedef enum
{
    SP_TX_INITIAL = 1,
    SP_TX_WAIT_SLIMPORT_PLUGIN,
    SP_TX_PARSE_EDID,
    SP_TX_LINK_TRAINING,
    SP_TX_CONFIG_VIDEO,
    SP_TX_CONFIG_AUDIO,
    SP_TX_HDCP_AUTHENTICATION,
    SP_TX_PLAY_BACK
} SP_TX_System_State;


void SP_CTRL_Main_Procss(void);
void SP_CTRL_TimerProcess (void);
void SP_CTRL_Timer_Slot1(void) ;
void SP_CTRL_Timer_Slot2(void);
void SP_CTRL_Timer_Slot3(void);
void SP_CTRL_Timer_Slot4(void);
int  SP_CTRL_Chip_Detect(void);
void SP_CTRL_Chip_Initial(void);
void SP_CTRL_Variable_Init(void);
void SP_CTRL_Set_LVTTL_Interface(BYTE eBedSync, BYTE rDE, BYTE sYCMUX, BYTE sDDR,BYTE lEdge );
void SP_CTRL_Set_BPC_Select(BYTE bpc);
void SP_CTRL_InputSet(VideoInterface Interface,ColorSpace bColorSpace, ColorDepth cBpc);
void SP_CTRL_AUDIO_FORMAT_Set(AudioType cAudio_Type,AudioFs cAudio_Fs,AudioWdLen cAudio_Word_Len);
void SP_CTRL_I2S_CONFIG_Set(I2SChNum cCh_Num, I2SLayOut cI2S_Layout);
void SP_CTRL_Set_System_State(SP_TX_System_State ss);
void SP_CTRL_Int_Process(void);
void SP_CTRL_Slimport_Plug_Process(void);
void SP_CTRL_Video_Changed_Int_Handler (BYTE changed_source);
void SP_CTRL_PLL_Changed_Int_Handler(void);
void SP_CTRL_AudioClk_Change_Int_Handler(void);
void  SP_CTRL_Auth_Done_Int_Handler(void);
//void SP_CTRL_Auth_Change_Int_Handler(void);
void SP_CTRL_SINK_IRQ_Int_Handler(void);
void SP_CTRL_POLLING_ERR_Int_Handler(void);
void SP_CTRL_LT_DONE_Int_Handler(void);
void SP_CTRL_IRQ_ISP(void);
void SP_CTRL_Clean_HDCP(void);
void SP_CTRL_EDID_Process(void);
void SP_CTRL_EDID_Read(void);
void SP_CTRL_HDCP_Process(void);
void SP_CTRL_PlayBack_Process(void);

BYTE SP_CTRL_Check_Cable_Status(void);
 void SP_CTRL_LINK_CHANGE_Int_Handler(void);

void CEC_Abort_Message_Process(void);
void CEC_Get_Physical_Address_Message_Process(void);
void CEC_Rx_Process(void);


BYTE System_State_Get(void);
#endif


