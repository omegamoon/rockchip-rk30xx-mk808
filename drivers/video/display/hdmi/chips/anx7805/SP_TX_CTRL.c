// ---------------------------------------------------------------------------
// Analogix Confidential Strictly Private
//
//
// ---------------------------------------------------------------------------
// >>>>>>>>>>>>>>>>>>>>>>>>> COPYRIGHT NOTICE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// ---------------------------------------------------------------------------
// Copyright 2004-20010 (c) Analogix 
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

#include <linux/delay.h>
//#include <mach/gpio.h>
#include "SP_TX_CTRL.h"
#include "SP_TX_DRV.h"
#include "SP_TX_Reg.h"
#include "Colorado.h"



BYTE ByteBuf[MAX_BUF_CNT];

SP_TX_System_State sp_tx_system_state;
BYTE timer_slot;
BYTE timer_slot_start;

//for EDID
BYTE edid_pclk_out_of_range;//the flag when clock out of 256Mhz
BYTE sp_tx_edid_err_code;//EDID read  error type flag
BYTE bEDIDBreak; //EDID access break


//for HDCP
BYTE sp_tx_hdcp_auth_pass; //HDCP auth pass flag
BYTE sp_tx_hdcp_auth_fail_counter;  //HDCP auth fail count flag
//BYTE sp_tx_hdcp_wait_100ms_needed;  //HDCP wait 100ms flag to check auth pass
BYTE sp_tx_hdcp_enable;//enable HDCP process indicator
BYTE sp_tx_ssc_enable; //SSC control anbale indicator
BYTE sp_tx_hdcp_capable_chk;//DPCD HDCP capable check flag 
BYTE sp_tx_hw_hdcp_en;//enable hardware HDCP
BYTE sp_tx_hdcp_auth_done;
BYTE sp_tx_hw_lt_done;//hardware linktraining done indicator
BYTE sp_tx_hw_lt_enable;//hardware linktraining enable

BYTE USER_MENU_ENABLE ;

BYTE sp_tx_ds_hdmi_plugged;//amazon downstream HDMI plug in flag
BYTE sp_tx_ds_hdmi_sensed; //amazon downstream HDMI is power on
BYTE sp_tx_pd_mode; //ANX7805 power state flag


//Global interrupt control
BYTE bInterruptEnable;

//Global cwire polling control
BYTE Polling_Disable;

//external interrupt flag
//BYTE ext_int_index;
struct VideoFormat SP_TX_Video_Input;
struct AudioFormat SP_TX_Audio_Input;


BYTE sp_tx_rx_anx7730;
BYTE sp_tx_rx_anx7730_b0;

BYTE CEC_abort_message_received = 0;
BYTE CEC_get_physical_adress_message_received = 0;
BYTE CEC_logic_addr = 0x00;
int	 CEC_loop_number = 0;
BYTE CEC_resent_flag = 0;



void SP_CTRL_Main_Procss(void)
{
    SP_CTRL_Int_Process();
    SP_CTRL_TimerProcess();
	
}

void SP_CTRL_TimerProcess (void) 
{
    //0811
    //if(timer_slot_start)
	{
	    SP_CTRL_Timer_Slot1();
	    SP_CTRL_Timer_Slot2();
	    SP_CTRL_Timer_Slot3();
	    SP_CTRL_Timer_Slot4(); 
	}
}


void SP_CTRL_Timer_Slot1(void) 
{	
    if(sp_tx_system_state == SP_TX_WAIT_SLIMPORT_PLUGIN)
    {
        SP_CTRL_Slimport_Plug_Process();
    }
    
    if(sp_tx_system_state == SP_TX_PARSE_EDID)
    {
        bEDIDBreak = 0;
        SP_CTRL_EDID_Process();
        if(bEDIDBreak)
        	D("ERR:EDID corruption!\n");
        //0811
        SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
    }

}

void SP_CTRL_Timer_Slot2(void)
{
	if(sp_tx_system_state == SP_TX_LINK_TRAINING)
	{
	    sp_tx_ssc_enable = 1;
        if(SP_TX_LT_Pre_Config())//pre-config not done
       		return;
		// SP_TX_Link_Training();
		SP_TX_HW_Link_Training();
	}
    
	if(sp_tx_system_state == SP_TX_CONFIG_VIDEO)
	{
		SP_TX_Config_Video(&SP_TX_Video_Input);
	}
}

void SP_CTRL_Timer_Slot3(void)
{
   BYTE c;
   
	if(sp_tx_system_state == SP_TX_CONFIG_AUDIO)
	{	
		SP_TX_Config_Audio(&SP_TX_Audio_Input);
	}
    
    if(sp_tx_system_state == SP_TX_HDCP_AUTHENTICATION)
	{
		//D("HDCP enable\n");
		sp_tx_hdcp_enable = 0;
		
		if(sp_tx_hdcp_enable)
		{
			SP_CTRL_HDCP_Process();
		}
		else
		{
			SP_TX_Power_Down(SP_TX_PWR_HDCP);// Poer down HDCP link clock domain logic for B0 version-20110913-ANX.Fei
			SP_TX_AUX_DPCDRead_Bytes(0x0, 0x05, 0x24,1,&c);//check VD21 has been cut off
			if(((c&0x01)==0x01)&&(sp_tx_rx_anx7730_b0))//added for ANX7730 BB version -20111206- ANX.Fei
			SP_TX_VBUS_PowerDown();
			SP_TX_Show_Infomation();
			SP_TX_Video_Mute(0);
			SP_CTRL_Set_System_State(SP_TX_PLAY_BACK);
		}
	}
}
void SP_CTRL_Timer_Slot4(void)
{
    if(sp_tx_system_state == SP_TX_PLAY_BACK)
		SP_CTRL_PlayBack_Process();
}


int SP_CTRL_Chip_Detect(void)
{
    return SP_TX_Chip_Located();
}


void SP_CTRL_Chip_Initial(void)
{
    SP_TX_Hardware_PowerDown();//Power down ANX7805 totally
    SP_TX_VBUS_PowerDown(); // Disable the power supply for ANX7730
    SP_CTRL_Variable_Init();

    //set video Input format
#ifdef LVTTL_EN
    SP_CTRL_InputSet(LVTTL_RGB,COLOR_RGB, COLOR_8_BIT);
    SP_CTRL_Set_LVTTL_Interface(SeparateSync,SeparateDE,UnYCMUX,SDR,Negedge);//separate SYNC,DE, not YCMUX, SDR,negedge
#else
    SP_CTRL_InputSet(MIPI_DSI,COLOR_RGB, COLOR_8_BIT);
#endif
   
    //set audio input format
#ifdef SPDIF_EN
	SP_CTRL_AUDIO_FORMAT_Set(AUDIO_SPDIF,AUDIO_FS_48K,AUDIO_W_LEN_20_24MAX);
#else
	SP_CTRL_AUDIO_FORMAT_Set(AUDIO_I2S,/*AUDIO_FS_48K*/AUDIO_FS_441K,AUDIO_W_LEN_20_24MAX);
	SP_CTRL_I2S_CONFIG_Set(I2S_CH_2, I2S_LAYOUT_0);	
#endif
  

    SP_CTRL_Set_System_State(SP_TX_WAIT_SLIMPORT_PLUGIN);
    
}

void SP_CTRL_Variable_Init(void)
{	
	sp_tx_edid_err_code = 0; 
	edid_pclk_out_of_range = 0; 
	sp_tx_link_config_done = 0; 
	sp_tx_test_link_training = 0;

	sp_tx_test_lane_count=0; 
	sp_tx_test_bw=0;

	sp_tx_ds_hdmi_plugged = 0;
	sp_tx_ds_hdmi_sensed =0;
	sp_tx_ds_edid_hdmi = 0;

	sp_tx_pd_mode = 1;//initial power state is power down.


	sp_tx_hdcp_auth_fail_counter = 0;
	sp_tx_hdcp_auth_pass = 0;
	//sp_tx_hdcp_wait_100ms_needed = 1;
	sp_tx_hw_hdcp_en = 0;
	sp_tx_hdcp_capable_chk = 0;
	sp_tx_hdcp_enable = 0;
	sp_tx_ssc_enable = 0;
	sp_tx_hdcp_auth_done = 0;
	sp_tx_hw_lt_done = 0;
	sp_tx_hw_lt_enable = 0;

	//bInterruptEnable = 1;
	bEDIDBreak = 0;
	//VSDBaddr = 0x84;
	EDID_Print_Enable = 0;


    //for link training change 0811
	LTResult.LT_2_54=0;
	LTResult.LT_2_27=0;
	LTResult.LT_2_162=0;
	LTResult.LT_1_54=0;
	LTResult.LT_1_27=0;
	LTResult.LT_1_162=0;

	sp_tx_rx_anx7730 = 1;//default the Rx is ANX7730
	sp_tx_rx_anx7730_b0 = 0; //default the Rx cable is ANX7730 A0

      
      //CEC support index initial
	CEC_abort_message_received = 0;
	CEC_get_physical_adress_message_received = 0;
	CEC_logic_addr = 0x00;
	CEC_loop_number = 0;
	CEC_resent_flag = 0;

	MIPI_Format_Index_Set(0);
}


void SP_CTRL_Set_LVTTL_Interface(BYTE eBedSync, BYTE rDE, BYTE sYCMUX, BYTE sDDR,BYTE lEdge )
{
    /*
	eBedSync:    1_Embeded SYNC, 0_Separate SYNC
	rDE:         1_Regenerate DE, 0_Separate DE
	sYCMUX:      1_YCMUX, 0_Not YCMUX
	sDDR:        1_DDR,  0_SDR   
	lEdge:       1_negedge, 0_posedge  
    */
    
    SP_TX_Video_Input.bLVTTL_HW_Interface.sEmbedded_Sync.Embedded_Sync = eBedSync;
    SP_TX_Video_Input.bLVTTL_HW_Interface.sEmbedded_Sync.Extend_Embedded_Sync_flag = 0;
    SP_TX_Video_Input.bLVTTL_HW_Interface.DE_reDenerate = rDE;
    SP_TX_Video_Input.bLVTTL_HW_Interface.sYC_MUX.YC_MUX = sYCMUX;
    SP_TX_Video_Input.bLVTTL_HW_Interface.sYC_MUX.YC_BIT_SEL = 1;
    SP_TX_Video_Input.bLVTTL_HW_Interface.DDR_Mode = sDDR;
    SP_TX_Video_Input.bLVTTL_HW_Interface.Clock_EDGE = lEdge;
}


void SP_CTRL_InputSet(VideoInterface Interface,ColorSpace bColorSpace, ColorDepth cBpc)
{
    //0811
	if(Interface == LVTTL_RGB)
        D("***LVTTL interface is configured.***");
	else if(Interface == MIPI_DSI)
        D("***MIPI interface is configured.***");
	   
	SP_TX_Video_Input.Interface = Interface;
	SP_TX_Video_Input.bColordepth = cBpc;
	SP_TX_Video_Input.bColorSpace = bColorSpace;

}

void SP_CTRL_AUDIO_FORMAT_Set(AudioType cAudio_Type,AudioFs cAudio_Fs,AudioWdLen cAudio_Word_Len)
{
        SP_TX_Audio_Input.bAudioType = cAudio_Type;
        SP_TX_Audio_Input.bAudio_Fs = cAudio_Fs;
        SP_TX_Audio_Input.bAudio_word_len = cAudio_Word_Len;}


void SP_CTRL_I2S_CONFIG_Set(I2SChNum cCh_Num, I2SLayOut cI2S_Layout)
{
	SP_TX_Audio_Input.bI2S_FORMAT.AUDIO_LAYOUT  = cI2S_Layout;
	SP_TX_Audio_Input.bI2S_FORMAT.Channel_Num     = cCh_Num;
	SP_TX_Audio_Input.bI2S_FORMAT.Channel_status1 = 0x00;
	SP_TX_Audio_Input.bI2S_FORMAT.Channel_status2 = 0x00;
	SP_TX_Audio_Input.bI2S_FORMAT.Channel_status3 = 0x00;
	SP_TX_Audio_Input.bI2S_FORMAT.Channel_status4 = SP_TX_Audio_Input.bAudio_Fs;
	SP_TX_Audio_Input.bI2S_FORMAT.Channel_status5 = SP_TX_Audio_Input.bAudio_word_len;
	SP_TX_Audio_Input.bI2S_FORMAT.SHIFT_CTRL = 0;
	SP_TX_Audio_Input.bI2S_FORMAT.DIR_CTRL = 0;
	SP_TX_Audio_Input.bI2S_FORMAT.WS_POL = 0;
	SP_TX_Audio_Input.bI2S_FORMAT.JUST_CTRL = 0;
	SP_TX_Audio_Input.bI2S_FORMAT.EXT_VUCP = 0;
}



void SP_CTRL_Set_System_State(SP_TX_System_State ss) 
{
    D("SP_TX To System State: ");
    switch (ss) 
    {
        case SP_TX_INITIAL:
            sp_tx_system_state = SP_TX_INITIAL;
            D("SP_TX_INITIAL");
            break;
        case SP_TX_WAIT_SLIMPORT_PLUGIN: 
            sp_tx_system_state = SP_TX_WAIT_SLIMPORT_PLUGIN;
            D("SP_TX_WAIT_SLIMPORT_PLUGIN");
            break;
        case SP_TX_PARSE_EDID:
            sp_tx_system_state = SP_TX_PARSE_EDID;
            D("SP_TX_READ_PARSE_EDID");
            break;
        case SP_TX_CONFIG_VIDEO:
            sp_tx_system_state = SP_TX_CONFIG_VIDEO;
            D("SP_TX_CONFIG_VIDEO");
            break;
	 case SP_TX_CONFIG_AUDIO:
            sp_tx_system_state = SP_TX_CONFIG_AUDIO;
            D("SP_TX_CONFIG_AUDIO");
            break;
        case SP_TX_LINK_TRAINING:
            sp_tx_system_state = SP_TX_LINK_TRAINING;
	     sp_tx_hw_lt_enable = 0;
            D("SP_TX_LINK_TRAINING");
            break;
        case SP_TX_HDCP_AUTHENTICATION:
            sp_tx_system_state = SP_TX_HDCP_AUTHENTICATION;
            D("SP_TX_HDCP_AUTHENTICATION");
            break;
        case SP_TX_PLAY_BACK:
            sp_tx_system_state = SP_TX_PLAY_BACK;
            D("SP_TX_PLAY_BACK");
            break;
        default:
            break;
    }	
}

//check downstream cable stauts ok-20110906-ANX.Fei
BYTE SP_CTRL_Check_Cable_Status(void)
{
    BYTE c;
	if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x18,1,&c)==AUX_OK)
	{
		if((c&0x28)==0x28)
			return 1; //RX OK
	}

	return 0;

}
void SP_CTRL_Int_Process(void)
{
    BYTE c1,c2,c3,c4,c5;

	if(sp_tx_rx_anx7730_b0)
	{
	    if(!SP_CTRL_Check_Cable_Status())//wait for downstream cable stauts ok-20110906-ANX.Fei
	    {
	    	//RX not ready, check DPCD polling is still available 
	    	SP_CTRL_POLLING_ERR_Int_Handler();
	        return;	    	
	    }

	}

	if(sp_tx_pd_mode )//When chip is power down, do not care the int.-ANX.Fei-20111020
	{
	     return;
	}

    SP_TX_Get_Int_status(COMMON_INT_1,&c1);
	SP_TX_Get_Int_status(COMMON_INT_2,&c2);
	SP_TX_Get_Int_status(COMMON_INT_3,&c3);
	SP_TX_Get_Int_status(COMMON_INT_4,&c4);
	SP_TX_Get_Int_status(SP_INT_STATUS,&c5);

	if(c1 & SP_COMMON_INT1_VIDEO_CLOCK_CHG)//video clock change
		SP_CTRL_Video_Changed_Int_Handler(0);

	if(c1 & SP_COMMON_INT1_VIDEO_FORMAT_CHG)//video format change
		SP_CTRL_Video_Changed_Int_Handler(1);

	if(c1 & SP_COMMON_INT1_PLL_LOCK_CHG)//pll lock change
		SP_CTRL_PLL_Changed_Int_Handler();

	if(c1 & SP_COMMON_INT1_PLL_LOCK_CHG)//audio clock change
		SP_CTRL_AudioClk_Change_Int_Handler();

	//if(c2 & SP_COMMON_INT2_AUTHCHG)//auth change
	//  SP_CTRL_Auth_Change_Int_Handler();

	if(c2 & SP_COMMON_INT2_AUTHDONE)//auth done
		SP_CTRL_Auth_Done_Int_Handler();

	/*added for B0 version-ANX.Fei-20110831-Begin*/
	//if(c5 & SP_TX_INT_DPCD_IRQ_REQUEST)//IRQ int
	    SP_CTRL_SINK_IRQ_Int_Handler();

	if(c5 & SP_TX_INT_STATUS1_POLLING_ERR)//c-wire polling error
	    SP_CTRL_POLLING_ERR_Int_Handler();

	if(c5 & SP_TX_INT_STATUS1_TRAINING_Finish)//link training finish int
	    SP_CTRL_LT_DONE_Int_Handler();

	if(c5 & SP_TX_INT_STATUS1_LINK_CHANGE)//link is lost  int
	    SP_CTRL_LINK_CHANGE_Int_Handler();
	/*added for B0 version-ANX.Fei-20110831-End*/
}

void SP_CTRL_Slimport_Plug_Process(void)
{
	BYTE c;
	int i;
	BYTE bAux_OK;


	
	if (gpio_get_value(SLIMPORT_CABLE_DETECT))//(SLIMPORT_CABLE_DETECT)
	{
		mdelay(50);//dglitch the cable detection
		if(gpio_get_value(SLIMPORT_CABLE_DETECT))
	   	{

            if(sp_tx_pd_mode)
            {
                SP_TX_Hardware_PowerOn();
                SP_TX_Power_On(SP_TX_PWR_REG);
                SP_TX_Power_On(SP_TX_PWR_TOTAL);
                SP_TX_Initialization(&SP_TX_Video_Input);
				sp_tx_pd_mode = 0; 
				//check whether slimport cable has been power on.--20111206-ANX.Fei
				SP_TX_AUX_DPCDRead_Bytes(0x00,0x00,0x00,1,&c);
				if(c == 0x11)
				{
					D("Slimport cable has been powered on.");
				
				}else
				{
					SP_TX_VBUS_PowerOn();
				}

				bAux_OK = 0;
                //mdelay(500);//added for some TV DC abnormal issue -FeiWang.2011.6.8
				for(i=0;i<5;i++)//loop 5 times to check whether the Rx is ANX7730
				{
					//check anx7805 rx anx7730 or not, if 0x00523 is not 0, it means the rx is anx7730. 
					if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x23,1,&c)==AUX_OK)
					{
						bAux_OK = 1;
						
					       //SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x23,1,&c);
					       //D("*****0x00523 = %.2x*****\n",(WORD)c);
						if((c !=0)&&(c!=0xff))
						{
							  sp_tx_rx_anx7730 = 1;
							  if((c&0x80)==0x80)
							  {
								sp_tx_rx_anx7730_b0 = 1;
								D("Downstream is ANX7730 B0 cable.");
							  }else{
							       sp_tx_rx_anx7730_b0 = 0;
								D("Downstream is ANX7730 A0 cable.");
							  }
					
							  return;
						}
					}				
					mdelay(100);
				}

				//if aux communication is not good, return;
				if(!bAux_OK)
				{
					//aux not ok, power down.
					SP_TX_Power_Down(SP_TX_PWR_TOTAL);
					SP_TX_Power_Down(SP_TX_PWR_REG);
					SP_TX_VBUS_PowerDown();
					SP_TX_Hardware_PowerDown();
					sp_tx_pd_mode = 1;
					sp_tx_ds_hdmi_sensed = 0;
					sp_tx_ds_hdmi_plugged =0;
					sp_tx_link_config_done = 0;
					sp_tx_hw_lt_enable = 0;
					SP_CTRL_Set_System_State(SP_TX_WAIT_SLIMPORT_PLUGIN);		  		
					return;
				}
				
				sp_tx_rx_anx7730 = 0;
				D("Downstream is not ANX7730 cable.");		   
			}
			if(sp_tx_rx_anx7730)
			{
				// added for Y-cable & HDMI does not plug out-20111206-ANX.Fei
				SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x18,1,&c);
				if((c&0x1)==0x1)
					sp_tx_ds_hdmi_plugged =1;
				if((c&0x40)==0x40)
					sp_tx_ds_hdmi_sensed = 1;
			
				if((sp_tx_ds_hdmi_plugged&&sp_tx_ds_hdmi_sensed&&sp_tx_rx_anx7730_b0)
				||((sp_tx_ds_hdmi_plugged)&&(!sp_tx_rx_anx7730_b0)))//check both hotplug&Rx sense for ANX7730 B0, and only hotplug for ANX7730 A0  -20111011-ANX.Fei
				{
					SP_CTRL_Set_System_State(SP_TX_PARSE_EDID);
				}
			}else
				SP_CTRL_Set_System_State(SP_TX_PARSE_EDID);
		}
	
	}
	else if(sp_tx_pd_mode==0)
	{
		SP_TX_Power_Down(SP_TX_PWR_TOTAL);
		SP_TX_Power_Down(SP_TX_PWR_REG);
		SP_TX_VBUS_PowerDown();
		SP_TX_Hardware_PowerDown();
		sp_tx_pd_mode = 1;
		sp_tx_ds_hdmi_sensed = 0;
		sp_tx_ds_hdmi_plugged =0;
		sp_tx_link_config_done = 0;
		sp_tx_hw_lt_enable = 0;
		SP_CTRL_Set_System_State(SP_TX_WAIT_SLIMPORT_PLUGIN);

	}

}





void SP_CTRL_Video_Changed_Int_Handler (BYTE changed_source)
{
    //D("[INT]: SP_CTRL_Video_Changed_Int_Handler");
    //0811
    //if(sp_tx_system_state > SP_TX_LINK_TRAINING)        
    if(sp_tx_system_state > SP_TX_LINK_TRAINING)
    {
        switch(changed_source) 
        {
            
            case 0:
                //0811                
                D("Video:_______________Video clock changed!");
                SP_TX_Disable_Video_Input();
                SP_TX_Disable_Audio_Input();
                SP_TX_Enable_Audio_Output(0);
                SP_CTRL_Clean_HDCP();
		        SP_CTRL_Set_System_State(SP_TX_CONFIG_VIDEO);
                break;
            case 1:
                D("Video:_______________Video format changed!");
                //SP_TX_Disable_Video_Input();
                // SP_TX_Disable_Audio_Input();
                // SP_TX_Enable_Audio_Output(0);
                //SP_CTRL_Set_System_State(SP_TX_CONFIG_VIDEO);
                break;
            default:
                break;
        } 
    }
}

void SP_CTRL_PLL_Changed_Int_Handler(void)
{
    //0811
    if (sp_tx_system_state > SP_TX_PARSE_EDID)
    {
	  if(!SP_TX_Get_PLL_Lock_Status())
        {
            D("PLL:_______________PLL not lock!");
            SP_CTRL_Clean_HDCP();
            //0811
            SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
            sp_tx_link_config_done = 0;
        }
    }
}
void SP_CTRL_AudioClk_Change_Int_Handler(void)
{
    if (sp_tx_system_state > SP_TX_CONFIG_VIDEO)
    {

            D("Audio:_______________Audio clock changed!");
            SP_TX_Disable_Audio_Input();
	     SP_TX_Enable_Audio_Output(0);
            SP_CTRL_Clean_HDCP();
            SP_CTRL_Set_System_State(SP_TX_CONFIG_AUDIO);
    }
}

void  SP_CTRL_Auth_Done_Int_Handler(void) 
{
	BYTE c;
    
	SP_TX_Get_HDCP_status(&c);
	if(c & SP_TX_HDCP_AUTH_PASS) 
	{
		D("Authentication pass in Auth_Done");
		sp_tx_hdcp_auth_pass = 1;
		sp_tx_hdcp_auth_fail_counter = 0;
	} 
	else 
	{
		D("Authentication failed in AUTH_done");
		//sp_tx_hdcp_wait_100ms_needed = 1;
		sp_tx_hdcp_auth_pass = 0;
		sp_tx_hdcp_auth_fail_counter ++;
		if(sp_tx_hdcp_auth_fail_counter >= SP_TX_HDCP_FAIL_THRESHOLD) 
		{
			sp_tx_hdcp_auth_fail_counter = 0;
			SP_TX_Video_Mute(1);
			SP_TX_HDCP_Encryption_Disable();
			//SP_TX_HDCP_ReAuth();
                     //SP_TX_VBUS_PowerDown();//Let ANX7730 get power from HDMI link
			D("Re-authentication!");
		}
	}

	sp_tx_hdcp_auth_done = 1;
}

#if 0
void SP_CTRL_Auth_Change_Int_Handler(void) 
{
	BYTE c;

	SP_TX_Get_HDCP_status(&c);
	if(c & SP_TX_HDCP_AUTH_PASS) 
	{
		sp_tx_hdcp_auth_pass = 1;
		D("Authentication pass in Auth_Change");
	} 
	else 
	{
		D("Authentication failed in Auth_change");
		sp_tx_hdcp_auth_pass = 0;
		//sp_tx_hdcp_wait_100ms_needed = 1;
		SP_TX_Video_Mute(1);
		SP_TX_HDCP_Encryption_Disable();
		if(sp_tx_system_state > SP_TX_CONFIG_VIDEO)
		{
			SP_CTRL_Set_System_State(SP_TX_HDCP_AUTHENTICATION);
			SP_CTRL_Clean_HDCP();
		}
	}
}
#endif

/*added for B0 version-ANX.Fei-20110901-Start*/

// hardware linktraining finish interrupt handle process
void SP_CTRL_LT_DONE_Int_Handler(void)
{
	BYTE c;
	//D("[INT]: SP_CTRL_LT_DONE_Int_Handler");

	if((sp_tx_hw_lt_done)||(sp_tx_system_state != SP_TX_LINK_TRAINING))
	return;

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_TRAINING_CTRL_REG, &c);
	if(c&0x80)
	{
		c = (c & 0x70) >> 4;
		D("HW LT failed, ERR code = %.2x\n",(WORD)c);

		 sp_tx_link_config_done = 0;
		 sp_tx_hw_lt_enable = 1;
		 SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
		 
	}else{

	       sp_tx_hw_lt_done = 1;
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_LANE0_SET_REG, &c);
		D("HW LT succeed ,LANE0_SET = %.2x\n",(WORD)c);
	}

       // sp_tx_hw_lt_done = 0; 
}


 void SP_CTRL_LINK_CHANGE_Int_Handler(void)
 {

     BYTE lane0_1_status,sl_cr,al;
  
	
		if(sp_tx_system_state < SP_TX_LINK_TRAINING )
			return;
      
           
            if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE_ALIGN_STATUS_UPDATED,1,ByteBuf)==AUX_OK)
			al = ByteBuf[0];
	      else
			return;
		  
            if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE0_1_STATUS,1,ByteBuf)==AUX_OK)
			lane0_1_status = ByteBuf[0];
	     else
		 	return;
            
           // D("al = %x, lane0_1 = %x\n",(WORD)al,(WORD)lane0_1_status);

            if (sp_tx_lane_count == 0x01)
            {
                if(((lane0_1_status & 0x01) == 0) || ((lane0_1_status & 0x04) == 0))
                sl_cr = 0;
                else 
                sl_cr = 1;
            }/*//removed for B0 version
            else if(sp_tx_lane_count == 0x02)
            {
                if(((lane0_1_status&0x10) == 0) ||((lane0_1_status&0x40) == 0) ||((lane0_1_status & 0x01) == 0) || ((lane0_1_status & 0x04) == 0))
                sl_cr = 0;
                else 
                sl_cr = 1;
            }*/
            else
                sl_cr = 1;

            if(((al & 0x01) == 0) || (sl_cr == 0) )//align not done, CR not done
            {
                if((al & 0x01)==0)
                    D("Lane align not done\n");
                if(sl_cr == 0)
                    D("Lane clock recovery not done\n");

                //re-link training only happen when link traing have done		  
                if((sp_tx_system_state > SP_TX_LINK_TRAINING )&&sp_tx_link_config_done)
                {
                    sp_tx_link_config_done = 0;
                    SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
                    D("IRQ:____________re-LT request!");
                }
            }
        

 }

// downstream DPCD IRQ request interrupt handle process
void SP_CTRL_SINK_IRQ_Int_Handler(void)
{
    //D("[INT]: SP_CTRL_SINK_IRQ_Int_Handler\n");
    SP_CTRL_IRQ_ISP();
}

// c-wire polling error interrupt handle process
void SP_CTRL_POLLING_ERR_Int_Handler(void)
{
    BYTE c;
    int i;

    // D("[INT]: SP_CTRL_POLLING_ERR_Int_Handler\n");
    if((sp_tx_system_state < SP_TX_WAIT_SLIMPORT_PLUGIN)||sp_tx_pd_mode)
        return;

    for(i=0;i<5;i++)
    {
        if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x00,0x00,1,&c)==AUX_OK)
        {
	        if(c==0x11)
	            return;
        }        
        mdelay(10);
    }
    
	if(sp_tx_pd_mode ==0)
    {
        //D("read dpcd 0x00000=%.2x\n",(WORD)c);	
        D("Cwire polling is corrupted,power down ANX7805.\n"); 
        SP_TX_Power_Down(SP_TX_PWR_TOTAL);
        SP_TX_Power_Down(SP_TX_PWR_REG);
        SP_TX_VBUS_PowerDown();
        SP_TX_Hardware_PowerDown();
        SP_CTRL_Set_System_State(SP_TX_WAIT_SLIMPORT_PLUGIN);
		SP_CTRL_Clean_HDCP();
		sp_tx_pd_mode = 1;
		sp_tx_ds_hdmi_plugged = 0;
		sp_tx_ds_hdmi_sensed =0;
		sp_tx_link_config_done =0;
		sp_tx_hw_lt_enable = 0;
    }
}

void SP_CTRL_IRQ_ISP(void)
{
    BYTE c,c1,test_irq,test_lt =0,lane0_1_status,sl_cr,al;
    BYTE IRQ_Vector, test_vector,Int_vector1,Int_vector2;
    BYTE cRetryCount;
    //SP_TX_RST_AUX();

	if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_DEVICE_SERVICE_IRQ_VECTOR,1,ByteBuf) == AUX_OK)
	{
		IRQ_Vector = ByteBuf[0];
		SP_TX_AUX_DPCDWrite_Bytes(0x00, 0x02, DPCD_DEVICE_SERVICE_IRQ_VECTOR,1, ByteBuf);//write clear IRQ
	}
	else //DPCD read error
	{
		//DPCD read error 
		return;
	}


	//SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_DEVICE_SERVICE_IRQ_VECTOR,1,ByteBuf);
	//IRQ_Vector = ByteBuf[0];
	//SP_TX_AUX_DPCDWrite_Bytes(0x00, 0x02, DPCD_DEVICE_SERVICE_IRQ_VECTOR,1, ByteBuf);//write clear IRQ
	


   //add by span for IRQ interrupt issue.
   if((sp_tx_rx_anx7730)&&((IRQ_Vector&0x04)!=0x04)&&((IRQ_Vector&0x40)!=0x40))
   {
   	//no specific and HDCP irq
   	return;
   }
#ifdef AUX_DBG
   D("IRQ_Vector = %.2x\n", (WORD)IRQ_Vector);
#endif


    if(IRQ_Vector & 0x04)//HDCP IRQ
    {
        //SP_TX_Read_Reg(SP_TX_PORT0_ADDR, 0x92, &c);//clear  the HDCP int
        //SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0x92, c|0x01);//clear  the HDCP int

	if(sp_tx_hdcp_auth_pass)
	{
		if(SP_TX_AUX_DPCDRead_Bytes(0x06,0x80,0x29,1,&c1) == AUX_OK)
		{
			//D("Bstatus = %.2x\n", (WORD)c1);
			/*
			if(c1 & 0x01)
			{
				D("IRQ:____________HDCP repeater BKSVlist and V' are ready!");
				SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0x92, 0x01);//clear  the HDCP int

			SP_TX_AUX_DPCDRead_Bytes(0x06,0x80,0x2a,1,&c);//0x6802a:Binfo DPCD address


			SP_TX_AUX_DPCDRead_Bytes(0x06,0x80,0x2b,1, &c2);//0x6802b:Binfo DPCD address


			if(c & 0x80)
				D("sp_tx_repeater_max_devs (>=127) are exceeded!");

			if(c2 & 0x08)
				D("sp_tx_repeater_max_level (>=7) are exceeded!");

		}


		if(c1 & 0x02)
		{
			D("IRQ:____________HDCP R0' ready!");
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0x92, 0x01);//clear the hDCP int
		}

		*/  

			if(c1 & 0x04)
			{
				if(sp_tx_system_state > SP_TX_HDCP_AUTHENTICATION) 
				{
					SP_CTRL_Set_System_State(SP_TX_HDCP_AUTHENTICATION);
					sp_tx_hw_hdcp_en = 0;
					SP_TX_HW_HDCP_Disable();
					D("IRQ:____________HDCP Sync lost!");
				}
			}
		}
	}
        
       
    }
    if((IRQ_Vector & 0x40)&&(sp_tx_rx_anx7730))//specific int
    {
        //D("Rx specific interrupt IRQ!\n");

	if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_1,1,&Int_vector1)==AUX_OK)
	{
		SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_1,Int_vector1);
#ifdef AUX_DBG
		D("DPCD00510 = 0x%.2x!\n",(WORD)Int_vector1);
#endif
	}
	else
		return;//DPCD read error

	if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_2,1,&Int_vector2)==AUX_OK)
	{
		SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_2,Int_vector2);
#ifdef AUX_DBG
		D("DPCD00511 = 0x%.2x!\n",(WORD)Int_vector2);
#endif
	}
	else
		return;//DPCD read error

        //SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_1,1,&Int_vector1);
	 //SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_1,Int_vector1);
        //D("Rx specific interrupt IRQ 1 = 0x%.2x!\n",(WORD)Int_vector1);


        //SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_2,1,&Int_vector2);
	 //SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,DPCD_SPECIFIC_INTERRUPT_2,Int_vector2);
        //D("Rx specific interrupt IRQ 2 = 0x%.2x!\n",(WORD)Int_vector2);


        if(sp_tx_rx_anx7730_b0)
	{
		if((Int_vector1&0x01)==0x01)//check downstream HDMI hpd
		{
			//check downstream HDMI hotplug status plugin
			if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x18,1,&c) ==AUX_OK)
			{
#ifdef AUX_DBG
				D("Rx 00518= 0x%.2x!\n",(WORD)c);
#endif
				if((c&0x01)==0x01)
				{
					sp_tx_ds_hdmi_plugged = 1;
					D("Downstream HDMI is pluged!\n");
				}
			}
			else
				return;
		}
		
		if((Int_vector1&0x02)==0x02)
		{
			//check downstream HDMI hotplug status unplug
			//SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x18,1,&c);
			if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x18,1,&c) == AUX_OK)
			{
#ifdef AUX_DBG
				D("Rx 00518= 0x%.2x!\n",(WORD)c);
#endif
				/*if((c&0x01)==0x01)
				{
					return;
				}*/
	            		if((c&0x01)!=0x01)
				//else
	            		{
					D("Downstream HDMI is unpluged!\n");
					sp_tx_ds_hdmi_plugged = 0;
					if((sp_tx_system_state>SP_TX_WAIT_SLIMPORT_PLUGIN)&&(!sp_tx_pd_mode))
					{
						SP_TX_Power_Down(SP_TX_PWR_TOTAL);
						SP_TX_Power_Down(SP_TX_PWR_REG);
						//SP_TX_VBUS_PowerDown();
						SP_TX_Hardware_PowerDown();
						SP_CTRL_Clean_HDCP();
						SP_CTRL_Set_System_State(SP_TX_WAIT_SLIMPORT_PLUGIN);
						sp_tx_ds_hdmi_sensed = 0;
						sp_tx_pd_mode = 1;
						sp_tx_link_config_done =0;
					}
				}
			}
		}
	}
	else
	{//anx7730 A0 cable
       
		if((Int_vector1&0x01)==0x01)//check downstream HDMI hpd
		{
			sp_tx_ds_hdmi_plugged = 1;
			D("Downstream HDMI is pluged!\n");
		}
		if((Int_vector1&0x02)==0x02)
        {
             D("Downstream HDMI is unpluged!\n");
             sp_tx_ds_hdmi_plugged = 0;
            if((sp_tx_system_state>SP_TX_WAIT_SLIMPORT_PLUGIN)&&(!sp_tx_pd_mode))
               {
                    SP_TX_Power_Down(SP_TX_PWR_TOTAL);
                    SP_TX_Power_Down(SP_TX_PWR_REG);
                    SP_TX_VBUS_PowerDown();
                    SP_TX_Hardware_PowerDown();
                    SP_CTRL_Clean_HDCP();
                    SP_CTRL_Set_System_State(SP_TX_WAIT_SLIMPORT_PLUGIN);
                    //sp_tx_ds_hdmi_sensed = 0;
                    sp_tx_pd_mode = 1;
                    sp_tx_link_config_done =0;
               }

        }
	}

        //0811
        //if(((Int_vector&0x04)==0x04)&&(sp_tx_system_state > SP_TX_LINK_TRAINING ))    
        if(((Int_vector1&0x04)==0x04)&&(sp_tx_system_state > SP_TX_LINK_TRAINING ))
        {

		D("Rx specific  IRQ: Link is down!\n");

		if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE_ALIGN_STATUS_UPDATED,1,ByteBuf) == AUX_OK)
		{
			al = ByteBuf[0];
		}
		else
			return;

		if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE0_1_STATUS,1,ByteBuf)==AUX_OK)
		{
			lane0_1_status = ByteBuf[0];
		}
		else
			return;
  
            //SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE_ALIGN_STATUS_UPDATED,1,ByteBuf);
             //al = ByteBuf[0];
            //SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE0_1_STATUS,1,ByteBuf);
            //lane0_1_status = ByteBuf[0];
            
           // D("al = %x, lane0_1 = %x\n",(WORD)al,(WORD)lane0_1_status);

            if (sp_tx_lane_count == 0x01)
            {
                if(((lane0_1_status & 0x01) == 0) || ((lane0_1_status & 0x04) == 0))
                sl_cr = 0;
                else 
                sl_cr = 1;
            }
            else
                sl_cr = 1;

            if(((al & 0x01) == 0) || (sl_cr == 0) )//align not done, CR not done
            {
                if((al & 0x01)==0)
                    D("Lane align not done\n");
                if(sl_cr == 0)
                    D("Lane clock recovery not done\n");

                //re-link training only happen when link traing have done  0811		  
                //if((sp_tx_system_state > SP_TX_LINK_TRAINING )&&sp_tx_link_config_done)
                if((sp_tx_system_state > SP_TX_LINK_TRAINING )&&sp_tx_link_config_done)
                {
                    sp_tx_link_config_done = 0;

                    SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
                    D("IRQ:____________re-LT request!");
                }
            }
        

        }

        if((Int_vector1&0x08)==0x08)
        {
             D("Downstream HDCP is done!\n");

	     {
			//I2C_Master_Read_Reg(7,0x00, &c);
			//if(c&0x02)
			if((Int_vector1&0x10) !=0x10)
				D("Downstream HDCP is passed!\n");
			else
			{
				if(sp_tx_system_state > SP_TX_CONFIG_VIDEO )
				{
					SP_TX_Video_Mute(1);
					SP_TX_HDCP_Encryption_Disable();
					SP_CTRL_Clean_HDCP();
					SP_CTRL_Set_System_State(SP_TX_HDCP_AUTHENTICATION);
					D("Re-authentication due to downstream HDCP failure!");
				}
			}
	     }

        }
            /*
        if((Int_vector&0x10)==0x10)
        {
            D("Downstream HDCP is failed!");

            if(sp_tx_system_state > SP_TX_CONFIG_VIDEO )
            {
                SP_TX_Video_Mute(1);
                SP_TX_HDCP_Encryption_Disable();
                SP_CTRL_Clean_HDCP();
                SP_CTRL_Set_System_State(SP_TX_HDCP_AUTHENTICATION);
                D("Re-authentication due to downstream HDCP failure!");
            }
        }*/
        if((Int_vector1&0x20)==0x20)
        {
              D(" Downstream HDCP link integrity check fail!");
		//add for hdcp fail
		if(sp_tx_system_state > SP_TX_HDCP_AUTHENTICATION) 
		{
			SP_CTRL_Set_System_State(SP_TX_HDCP_AUTHENTICATION);
			sp_tx_hw_hdcp_en = 0;
			SP_TX_HW_HDCP_Disable();
			D("IRQ:____________HDCP Sync lost!");
		}
        }

        if((Int_vector1&0x40)==0x40)
        {
            D("Receive CEC command from upstream done!");
	     CEC_Rx_Process();
        }

        if((Int_vector1&0x80)==0x80)
        {
            D("CEC command transfer to downstream done!");
        }

	if(sp_tx_rx_anx7730_b0)//added only for ANX7730 B0 chip-ANX.Fei-20111011 
	{
		/*check downstream HDMI Rx sense status -20110906-ANX.Fei*/
		if((Int_vector2&0x04)==0x04)
		{

			if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x18,1,&c)==AUX_OK)
			{
				if((c&0x40)==0x40)
				{
					//sp_tx_ds_hdmi_sensed= 1;//remove because of late  Rx sense
					D("Downstream HDMI termination is detected!\n");
				}
			}
		}

		/*check downstream V21 power ready status -20110908-ANX.Fei*/
		if((sp_tx_ds_hdmi_plugged)&&(sp_tx_ds_hdmi_sensed == 0))//check V21 ready
		//if((Int_vector2&0x08)==0x08)
		{

			if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x19,1,&c) == AUX_OK)
			//if(I2C_Master_Read_Reg(SINK_TX_PORT0_ADDR,0xcb, &c) == SOURCE_REG_OK)
			{
				if((c&0x01)==0x01)//DPCD V21 ready
				{
					sp_tx_ds_hdmi_sensed = 1; //Rx sense is ready when V21 power is ok.
					D("Downstream V21 power is ready!\n");
					//SP_TX_VBUS_PowerDown();
				}
			}
		}

	}

 

    } 	
	 if((!sp_tx_rx_anx7730)&&(sp_tx_system_state > SP_TX_LINK_TRAINING ))//if rx is not anx7730, check the align and symbol lock only
        {
           
            if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE_ALIGN_STATUS_UPDATED,1,ByteBuf)== AUX_OK)
            {
             		al = ByteBuf[0];
            }
	     else
		 	return;

	     if(SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE0_1_STATUS,1,ByteBuf))
	     	{
	     		lane0_1_status = ByteBuf[0];
	     	}
		 else
		 	return;
	
	  
            //SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,DPCD_LANE0_1_STATUS,1,ByteBuf);
            //lane0_1_status = ByteBuf[0];
            
           // D("al = %x, lane0_1 = %x\n",(WORD)al,(WORD)lane0_1_status);

            if (sp_tx_lane_count == 0x01)
            {
                if(((lane0_1_status & 0x01) == 0) || ((lane0_1_status & 0x04) == 0))
                sl_cr = 0;
                else 
                sl_cr = 1;
            }/*
            else if(sp_tx_lane_count == 0x02)
            {
                if(((lane0_1_status&0x10) == 0) ||((lane0_1_status&0x40) == 0) ||((lane0_1_status & 0x01) == 0) || ((lane0_1_status & 0x04) == 0))
                sl_cr = 0;
                else 
                sl_cr = 1;
            }*/
            else
                sl_cr = 1;

            if(((al & 0x01) == 0) || (sl_cr == 0) )//align not done, CR not done
            {
                if((al & 0x01)==0)
                    D("Lane align not done\n");
                if(sl_cr == 0)
                    D("Lane clock recovery not done\n");

                //re-link training only happen when link traing have done 0811		  
                if((sp_tx_system_state > SP_TX_LINK_TRAINING )&&sp_tx_link_config_done)
                {
                    sp_tx_link_config_done = 0;

                    //0811
                    //SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
                    SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
                    D("IRQ:____________re-LT request!");
                }
            }
        

        }

    if(IRQ_Vector & 0x02)//automated test request
        test_irq = 1;
    else
        test_irq = 0;


    if(test_irq)
    {
          if(SP_TX_AUX_DPCDRead_Bytes(0x00, 0x02, DPCD_TEST_REQUEST,1,ByteBuf) == AUX_OK)
          {
           	test_vector = ByteBuf[0];
        
        if(test_vector & 0x01)
        {
            sp_tx_test_link_training = 1;
            test_lt = 1;
            
           SP_TX_AUX_DPCDRead_Bytes(0x00, 0x02, DPCD_TEST_LANE_COUNT,1,&c);
            sp_tx_test_lane_count = c;
            D("sp_tx_test_lane_count = %.2x\n", (WORD)sp_tx_test_lane_count);

            sp_tx_test_lane_count = sp_tx_test_lane_count & 0x0f;
            SP_TX_AUX_DPCDRead_Bytes(0x00, 0x02, DPCD_TEST_LINK_RATE, 1, &c);
             sp_tx_test_bw = c;
            D("sp_tx_test_bw = %.2x\n", (WORD)sp_tx_test_bw);

            SP_TX_AUX_DPCDRead_Bytes(0x00, 0x02, DPCD_TEST_Response,1,&c);
            SP_TX_AUX_DPCDWrite_Byte(0x00, 0x02, DPCD_TEST_Response, c|TEST_ACK);
            D("Set TEST_ACK!");
        }
        else 
        {
            sp_tx_test_link_training = 0;
            test_lt = 0;
        }

        if(test_vector & 0x04)//test edid
        {
            D("Test EDID Read Requested!\n");
            if (sp_tx_system_state >SP_TX_WAIT_SLIMPORT_PLUGIN) 
            {
                SP_CTRL_Set_System_State(SP_TX_PARSE_EDID);
            }

        } 	


        if(test_vector & 0x02)//test pattern
        {
            SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x60, 1,&c);
            //D("respone = %.2x\n", (WORD)c);
            SP_TX_AUX_DPCDWrite_Byte(0x00,0x02,0x60, c|0x01);
            SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x60, 1,&c);
 

            cRetryCount = 0;
            while((c & 0x03) == 0)
            {
                cRetryCount++;
                SP_TX_AUX_DPCDWrite_Byte(0x00,0x02,0x60, c|0x01);
                SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x60, 1,&c);
                if(cRetryCount>10)
                {
                    D("Fail to write ack!");
                    break;
                }
            }

	            D("respone = %.2x\n", (WORD)c);
       	 }
    	}

    }
    else
    {
        sp_tx_test_link_training = 0;
        test_lt = 0;
    }

    if(test_lt == 1)//test link traing
    {
        D("Test link training requested\n");
        //0811
        //if (sp_tx_system_state >SP_TX_PARSE_EDID)        
        if (sp_tx_system_state >SP_TX_PARSE_EDID) 
        {
            sp_tx_link_config_done = 0;
            SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
        }
    }	


}

void SP_CTRL_Clean_HDCP(void)
{
      // D("HDCP Clean!");
	sp_tx_hdcp_auth_fail_counter = 0;
	sp_tx_hdcp_auth_pass = 0;
	//sp_tx_hdcp_wait_100ms_needed = 1;
	sp_tx_hw_hdcp_en = 0;
	sp_tx_hdcp_capable_chk = 0;
	sp_tx_hdcp_auth_done = 0;

	SP_TX_Clean_HDCP();

}

void SP_CTRL_EDID_Process(void)
{
	BYTE i;
	//read DPCD 00000-0000b
	for(i = 0; i <= 0x0b; i ++)
        SP_TX_AUX_DPCDRead_Bytes(0x00,0x00,i,1,ByteBuf);
           
	SP_CTRL_EDID_Read();
	SP_TX_RST_AUX();
}


void SP_CTRL_EDID_Read(void)//add adress only cmd before every I2C over AUX access.-fei
{
    BYTE i,j,test_vector,edid_block = 0,segment = 0,offset = 0;

    SP_TX_EDID_Read_Initial();

    checksum = 0;
    sp_tx_ds_edid_hdmi =0;
    //VSDBaddr = 0x84;
    bEDIDBreak = 0;
    //Set the address only bit
    SP_TX_AddrOnly_Set(1);
    //set I2C write com 0x04 mot = 1
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, 0x04);
    //enable aux
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x01);
    SP_TX_Wait_AUX_Finished();
       
    edid_block = SP_TX_Get_EDID_Block();
   	if(edid_block < 2)
	{

		edid_block = 8 * (edid_block + 1);

		for(i = 0; i < edid_block; i ++)
		{
			if(!bEDIDBreak)
				SP_TX_AUX_EDIDRead_Byte(i * 16);
		}
		
		//clear the address only bit
		SP_TX_AddrOnly_Set(0);

	}else
	{

		for(i = 0; i < 16; i ++)
		{
			if(!bEDIDBreak)
				SP_TX_AUX_EDIDRead_Byte(i * 16);
		}

		//clear the address only bit
		SP_TX_AddrOnly_Set(0);
		
		edid_block = (edid_block + 1);
		for(i=0; i<((edid_block-1)/2); i++)//for the extern 256bytes EDID block
		{
			D("EXT 256 EDID block");		
			segment = i + 1;		
			for(j = 0; j<16; j++)
			{
				SP_TX_Parse_Segments_EDID(segment, offset);
				//mdelay(1);
				offset = offset + 0x10;
			}
		}

		if(edid_block%2)//if block is odd, for the left 128BYTEs EDID block
		{
			D("Last block");
			segment = segment + 1;
			for(j = 0; j<8; j++)
			{
				SP_TX_Parse_Segments_EDID(segment, offset);
				//mdelay(1);
				offset = offset + 0x10;
			}
		}	
	}


	//clear the address only bit
	SP_TX_AddrOnly_Set(0);
    

    SP_TX_RST_AUX();

    if(sp_tx_ds_edid_hdmi)
    	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x26, 0x01);//inform ANX7730 the downstream is HDMI
    else
    	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x26, 0x00);//inform ANX7730 the downstream is not HDMI
		

    SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x18,1,ByteBuf);
  

    test_vector = ByteBuf[0];

    if(test_vector & 0x04)//test edid
    {
        D("check sum = %.2x\n", (WORD)checksum);

        {
            SP_TX_AUX_DPCDWrite_Byte(0x00,0x02,0x61,checksum);   
            SP_TX_AUX_DPCDWrite_Byte(0x00,0x02,0x60,0x04);
        }
        D("Test read EDID finished");
    }
}


void SP_CTRL_HDCP_Process(void)
{
    BYTE c;
    
  
    if(!sp_tx_hdcp_capable_chk)
    {            
        sp_tx_hdcp_capable_chk = 1; 
        //SP_TX_AUX_DPCDRead_Bytes(0x06, 0x80, 0x28,1,&c);//read Bcaps

	if((AUX_ERR == SP_TX_AUX_DPCDRead_Bytes(0x06, 0x80, 0x28,1,&c))||(!(c & 0x01)))
         
        D("bcaps= %.2x\n",(WORD)c);
        //if(!(c & 0x01))
        {  
            D("Sink is not capable HDCP");
            SP_TX_Video_Mute(1);//when Rx does not support HDCP, force to send blue screen
            SP_CTRL_Set_System_State(SP_TX_PLAY_BACK);
            return;
        }
    }

    if(!sp_tx_hw_hdcp_en)
    {      
        SP_TX_Power_On(SP_TX_PWR_HDCP);// Poer on HDCP link clock domain logic for B0 version-20110913-ANX.Fei
        SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0x40, 0xc8);
        SP_TX_HW_HDCP_Enable();
		
	 SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK2, &c);
        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK2, c|0x03);//unmask auth change&done int
        sp_tx_hw_hdcp_en = 1;
    }

    if(sp_tx_hdcp_auth_done)//this flag will be only set in auth done interrupt
    {
         sp_tx_hdcp_auth_done = 0;   
            
        if(sp_tx_hdcp_auth_pass)
        {            
        
            //SP_TX_HDCP_Encryption_Enable();
            SP_TX_Video_Mute(0);
            D("@@@@@@@@@@@@@@@@@@@@@@@hdcp_auth_pass@@@@@@@@@@@@@@@@@@@@\n");
            
        }
        else
        {            
            SP_TX_HDCP_Encryption_Disable();
            SP_TX_Video_Mute(1);
            D("***************************hdcp_auth_failed*********************************\n");
            return;
        }
        
 
        SP_CTRL_Set_System_State(SP_TX_PLAY_BACK);


	SP_TX_AUX_DPCDRead_Bytes(0x0, 0x05, 0x24,1,&c);//check VD21 has been cut off
	if(((c&0x01)==0x01)&&(sp_tx_rx_anx7730_b0))//added for ANX7730 BB version -20111206- ANX.Fei
        SP_TX_VBUS_PowerDown();
        //SP_TX_RST_AUX();
        SP_TX_Show_Infomation();
    }

}


void SP_CTRL_PlayBack_Process(void)
{	
	//for MIPI video change
	if(SP_TX_Video_Input.Interface  == MIPI_DSI)
	{
		//polling checksum error
		if(!MIPI_CheckSum_Status_OK())
		{
			D("mipi checksum error!");
			SP_TX_MIPI_CONFIG_Flag_Set(0);
			SP_CTRL_Set_System_State(SP_TX_CONFIG_VIDEO);
		}
		//else
			//D("mipi checksum ok!");
	}

      //mdelay(1000);
	//SP_TX_VBUS_PowerDown();

}

void CEC_Abort_Message_Process()
{
	unsigned char c, i;
	CEC_abort_message_received = 0;
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x01);//reset CEC

	mdelay(10);
	//D("indicator_addr = %.2x\n",(WORD)indicator_addr);
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,CEC_logic_addr);
    	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,0x00);//Feature Abort Message
	//SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,0x00);//Feature Abort Message

	SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x70,1,&c);
    	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,(c|0x04));//send CEC massage
	
    	SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);

	CEC_loop_number = 0;
	while(c&0x80)//tx busy
	{
		CEC_loop_number ++; 				
    	       SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);
		
		if(CEC_loop_number > 50)
		{
			mdelay(10);
			CEC_resent_flag = 1;
	              SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x01);//reset CEC
	              SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x48);//Enable CEC receiver and Set CEC logic address

			D("loop number > 20000, break from sent");
			break;						
		}
	}
    
    	SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);
	//D("Recieved Abort Message and Send Feature Abort Message Finished, TX STATUS = %.2x\n",(WORD)c);
        D("R&s Finished, STATUS = %.2x\n",(WORD)c);
	mdelay(220);
	
	for(i = 0;i<5;i++)
	{
		mdelay(10);
            	SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x11,1,&c);
              SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x11,c);
              D("IRQ 2 = 0x%.2x!\n",(WORD)c);

		if((c&0x02)||CEC_resent_flag)// if sent failed, resend 1 time, add for 9.3-1
		{		
			CEC_resent_flag = 0;
			//D("Resend Feature Abort Message.....................................................\n");
	              SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x01);//reset CEC
			mdelay(10);
                	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,CEC_logic_addr);
                    	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,0x00);//Feature Abort Message

                     SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x70,1,&c);
   	              SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,(c|0x04));//send CEC massage
			
    	              SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);
			CEC_loop_number = 0;
			while(c&0x80)
			{
				CEC_loop_number ++; 						
    	                     SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);
				
				if(CEC_loop_number > 20) 
				{								
					CEC_resent_flag = 1;
					mdelay(10);
	                            SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x01);//reset CEC
	                            SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x48);//Enable CEC receiver and Set CEC logic address
			
					D("loop number > 20000, break from sent");
					break;
				}
			}	
    	              SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);
                    D("R&s Finished, STATUS = %.2x\n",(WORD)c);
                    // D("Recieved Abort Message and Send Feature Abort Message Finished, TX STATUS = %.2x\n",(WORD)c); 
		}
	
	}				
	
       SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x01);//reset CEC
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x48);//Enable CEC receiver and Set CEC logic address
}


void CEC_Get_Physical_Address_Message_Process()
{
	unsigned char c, i;
	CEC_get_physical_adress_message_received = 0;
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,  0x01);//reset CEC
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x4f);//destination address is 0x0F;
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x84);//Report Physical Address
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x00);
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x00);
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x00);
	
	//I2C_Master_Read_Reg(DP_TX_PORT3_ADDR,0x80,&c);  
	SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x04);//send CEC massage.
	
	SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);  			
	while(c&0x80)				
		SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);  	
	
	SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);  	
	D("Received Get Physical Address Message and Send Broadcast Finished, TX STATUS = %.2x\n",(WORD)c);

	for(i = 0;i<5;i++)
	{
		mdelay(100);
		SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x11,1,&c); 
              SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x11,c);
              D("IRQ 2 = 0x%.2x!\n",(WORD)c);

		if(c&0x02)// if sent failed, resend 1 time, add for 9.3-1
		{					
			D("Resend Feature Abort Message.....................................................\n");
			SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x01);//reset CEC
			SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x4f);//destination address is 0x0F;
			SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x84);//Report Physical Address
			SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x00);
			SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x00);
			SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x80,  0x00);				
			//I2C_Master_Read_Reg(DP_TX_PORT3_ADDR,0x80,&c);  
			SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70, 0x04);//send CEC massage.
			
			SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);  			
			while(c&0x80)  //tx busy
				SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);  	
			SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x72,1,&c);  
			D("Recieved Abort Message and Send Feature Abort Message Finished, TX STATUS = %.2x\n",(WORD)c); 
		}				
	}

	
        SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x01);//reset CEC
	 SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x70,0x48);//Enable CEC receiver and Set CEC logic address
}


void CEC_Rx_Process(void)
{	
	//unsigned char p;
    //unsigned char cec[16];
	unsigned char i,c,c0;
	//SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x10,1,&p);//CEC int bit6
	//SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x10,0x40);//CEC int bit6
      //if((p&0x08)==0x08)   //CEC_MSG_READY
 
              D("Receive CEC command from upstream done!");
		SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x71,1,&c);//Check RX FIFO Length

		D("The CEC Rx status = %.2x\n",(WORD)c);

		if (c & 0x20)
			c =16;
		else
			c = c&0x0f;
		//D("c = %.2x\n",(WORD)c);

		for(i=0;i<16;i++)
		{
		       SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x80,1,&c0);
			//D("RX indicator is: %.2x, ", (WORD)c);
		       SP_TX_AUX_DPCDRead_Bytes(0x00,0x05,0x80,1,&c0);
			D("FIFO[%.2x] = %.2x\n",(WORD)i,(WORD)c0);
	
			if(i==0) 
			{
				CEC_logic_addr = (c0 & 0x0f)<<4;
				CEC_logic_addr=CEC_logic_addr|((c0 & 0xf0)>>4);
			}
			if((i == 1)&&(c0 == 0xFF)) CEC_abort_message_received = 1;
			if((i == 1)&&(c0 == 0x83)) CEC_get_physical_adress_message_received = 1;

		}

		if(CEC_abort_message_received)
			CEC_Abort_Message_Process();	
		else if(CEC_get_physical_adress_message_received)
			CEC_Get_Physical_Address_Message_Process(); 


}


BYTE System_State_Get(void)
{
	return sp_tx_system_state;
}

/*********************************************************

*********************************************************/
int ANX7805_sys_init(struct hdmi *hdmi)
{
//	SP_CTRL_Chip_Initial();
	return HDMI_ERROR_SUCESS;
}

int ANX7805_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus)
{
	if(hdmi == NULL || hpdstatus == NULL)
		return HDMI_ERROR_FALSE;
		
	SP_CTRL_Int_Process();
	if(sp_tx_system_state == SP_TX_WAIT_SLIMPORT_PLUGIN)
    {
        SP_CTRL_Slimport_Plug_Process();
    }
//	SP_CTRL_Timer_Slot1();
	if(gpio_get_value(SLIMPORT_CABLE_DETECT))
	{
		if(sp_tx_rx_anx7730)
			if(sp_tx_ds_hdmi_plugged == 1)
				*hpdstatus = 1;
			else
				*hpdstatus = 0;
		else
			*hpdstatus = 1;
	}
	else
    	*hpdstatus = 0;
    return HDMI_ERROR_SUCESS;
}

int ANX7805_sys_plug(struct hdmi *hdmi)
{
	return HDMI_ERROR_SUCESS;
}
int ANX7805_sys_detect_sink(struct hdmi *hdmi, int *sink_status)
{
	return HDMI_ERROR_SUCESS;
}

int ANX7805_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff)
{
	if(hdmi == NULL || buff == NULL)
		return HDMI_ERROR_FALSE;
//    if(sp_tx_system_state == SP_TX_PARSE_EDID)
//    {
//        bEDIDBreak = 0;
//        SP_CTRL_EDID_Process();
//        if(bEDIDBreak)
//        	D("ERR:EDID corruption!\n");
//        //0811
//        SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
//    }
	return HDMI_ERROR_FALSE;
}

int ANX7805_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color)
{
//	msleep(500);
	SP_TX_RST_AUX();
	if(hdmi->edid.is_hdmi)
		SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x26, 0x01);//inform ANX7730 the downstream is HDMI
	else
		SP_TX_AUX_DPCDWrite_Byte(0x00,0x05,0x26, 0x00);//inform ANX7730 the downstream is not HDMI
	SP_TX_RST_AUX();
	SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
	while(sp_tx_system_state < SP_TX_CONFIG_AUDIO)
	{ 
		SP_CTRL_Timer_Slot2();
		if(sp_tx_system_state > SP_TX_CONFIG_VIDEO)
			break;
		msleep(10);
	}
	return HDMI_ERROR_SUCESS;
}

int ANX7805_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio)
{
	int audio_fs;
	
	if(hdmi == NULL || audio == NULL)
		return HDMI_ERROR_FALSE;
		
	if(audio->type != HDMI_AUDIO_LPCM)
		return HDMI_ERROR_FALSE;
	
	switch(audio->rate)
	{
		case HDMI_AUDIO_FS_32000:
			audio_fs = AUDIO_FS_32K;
			break;
		case HDMI_AUDIO_FS_44100:
			audio_fs = AUDIO_FS_441K;
			break;
		case HDMI_AUDIO_FS_48000:
			audio_fs = AUDIO_FS_48K;
			break;
		case HDMI_AUDIO_FS_88200:
			audio_fs = AUDIO_FS_882K;
			break;
		case HDMI_AUDIO_FS_96000:
			audio_fs = AUDIO_FS_96K;
			break;
		case HDMI_AUDIO_FS_176400:
			audio_fs = AUDIO_FS_1764K;
			break;
		case HDMI_AUDIO_FS_192000:
			audio_fs = AUDIO_FS_192K;
			break;
		default:
			return HDMI_ERROR_FALSE;
	}
	SP_CTRL_AUDIO_FORMAT_Set(AUDIO_I2S,audio_fs,AUDIO_W_LEN_20_24MAX);
	SP_CTRL_I2S_CONFIG_Set(I2S_CH_2, I2S_LAYOUT_0);	
	
	SP_TX_Config_Audio(&SP_TX_Audio_Input);

	return HDMI_ERROR_SUCESS;
}

int ANX7805_sys_config_hdcp(struct hdmi *hdmi, int enable)
{
	BYTE c;
	
	sp_tx_hdcp_enable = enable;
	
	if(sp_tx_hdcp_enable)
	{
		D("HDCP enable\n");
		SP_CTRL_HDCP_Process();
	}
	else
	{
		SP_TX_Power_Down(SP_TX_PWR_HDCP);// Poer down HDCP link clock domain logic for B0 version-20110913-ANX.Fei
		SP_TX_AUX_DPCDRead_Bytes(0x0, 0x05, 0x24,1,&c);//check VD21 has been cut off
		if(((c&0x01)==0x01)&&(sp_tx_rx_anx7730_b0))//added for ANX7730 BB version -20111206- ANX.Fei
		SP_TX_VBUS_PowerDown();
		SP_TX_Show_Infomation();
		SP_TX_Video_Mute(0);
		SP_CTRL_Set_System_State(SP_TX_PLAY_BACK);
	}
		
	return HDMI_ERROR_SUCESS;
}

int ANX7805_sys_enalbe_output(struct hdmi *hdmi, int enable)
{
	SP_CTRL_Timer_Slot4();
	return HDMI_ERROR_SUCESS;
}