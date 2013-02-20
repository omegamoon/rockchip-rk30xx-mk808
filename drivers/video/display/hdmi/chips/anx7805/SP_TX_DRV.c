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

#include <linux/delay.h>
//#include <mach/gpio.h>
#include "SP_TX_DRV.h"
#include "SP_TX_Reg.h"
#include "SP_TX_CTRL.h"
#include "Colorado.h"


//for EDID
BYTE checksum;
BYTE SP_TX_EDID_PREFERRED[18];//edid DTD array
BYTE sp_tx_ds_edid_hdmi; //Downstream is HDMI flag
//BYTE VSDBaddr;
BYTE DTDbeginAddr;
BYTE EDID_Print_Enable;
static BYTE EDIDExtBlock[128];



//for SW HDCP
BYTE AN[8] ;
BYTE AKSV[5];
BYTE BKSV_M[5];
BYTE BKSV[5];

long int pclk; //input video pixel clock 
long int M_val,N_val;
struct LT_Result LTResult;
SP_SW_LINK_State eSW_Link_state;  //software linktraining state
SP_LINK_BW sp_tx_bw;  //linktraining banwidth
static BYTE CRLoop0,CRLoop1; //clock recovery loop count
static BYTE bEQLoopcnt;  //EQ loop count
BYTE sp_tx_link_config_done;//link config done flag
BYTE sp_tx_lane_count; //link training lane count
BYTE sp_tx_test_link_training; //IRQ test link training request 
BYTE sp_tx_test_lane_count;//test link training lane count
BYTE sp_tx_test_bw;//test link trainging bandwidth 


struct AudiInfoframe SP_TX_AudioInfoFrmae;
struct Packet_AVI SP_TX_Packet_AVI;                     
struct Packet_MPEG SP_TX_Packet_MPEG;           
struct Packet_SPD SP_TX_Packet_SPD; 


BYTE bMIPI_Configured;
BYTE bMIPIFormatIndex;
//for MIPI Video format config
struct MIPI_Video_Format mipi_video_timing_table[] = { 
 //////////////////////pixel_clk--Htotal---H active--Vtotal--V active-HFP---HSW--HBP--VFP--VSW--VBP-----////////

 //{0, "1400x1050@60",		 108,			 1688,	 1400,	   1066,	 1050,		  88,	  50,		 150,	  9,  4,   3},
 {0, "720x480@60",		 27,		 858,	 720,	  525,	   480, 	  16,	  60,		 62,  10,  6,   29},
 {1, "1280X720P@60",	   74.25,		   1650,	  1280,    750, 	  720,		 110,	 40,	   220, 	5,	2,	23},
 {2, "1680X1050@60",	   146.25,	  2240, 	1680,	  1089, 	1050,	   104,   176,	   280, 	 3,  6 , 30},
 {3, "1920x1080p@60",	  148.5,	  2200, 	 1920,	   1125,	 1080,		88, 	44, 	 148,	   4, 2, 39},
 {4, "1920x1080p@24",	  59.4, 	 2200,		1920,	  1125, 	1080,	   88,	   44,		148,	  4, 2, 39},
 {5, "1920x1080p@30",	  74.25,	  2200, 	 1920,	   1125,	 1080,		88, 	44, 	 148,	   4, 2, 39},
 {6, "2560x1600@60",	 268,	  2720,  2560,	   1646,	 1600,		48, 	32, 	 80,  2,  6 , 38},
 //{7, "640x480@60",			25.17,			 816,		640,	   525, 	  480,		  32,	  96,	   48,	  10,  2,  33},
 {7, "640x480@60",			25.17,			 800,		640,	   525, 	  480,		  16,	  96,	   48,	  10,  2,  33},
 //{5, "640x480@60",		10.06,			 334,		320,	   502, 	  480,		  3,	 3, 	 8,   7,  8,  7},
 {8, "640x480@75",			 31.5,	  840,	 640,		   500, 	  480,		  16,	  64,		120,  1,  3,  16},
 {9, "1920*1200@60",	 154,			 2080,	 1920,	   1235,	 1200,		  40,	  80,		  40,	  3,  6,  26},	  
 {10, "800x600@60", 	 38,  976,	 800,		645,	   600, 	  32,	  96,		  48,	  10,  2,  33},
 //{11, "320x480@60",		 10.06, 		 334,		320,	   502, 	  480,		  3,	 3, 	 8,   7,  8,  7},
 {11, "320x480@60", 	 10.54, 		 352,		320,	   493, 	  480,		  8,	 8, 	 16,  6,  2,  5},
 {12, "1024x768@60",	 65,		 1344,		 1024,		   806, 	  768,		  24,	  136,		160,  3,  2,  33},
 {13, "1920x1200@60",	 154,		 2080,		 1920,		   1235,	  1200, 	  48,	  32,	   80,	  3,  2,  30},
 {14, "1280X1024@60",	 108,		 1688,		 1280,		   1066,	  1024, 	  48,	  112,	   248,   1,  2,  39}


};
struct Bist_Video_Format video_timing_table[] = { 
//number,video_type[32],pclk,h_total,h_active, v_total,v_active,h_front,h_sync,h_BP, v_FP, v_sync,v_BP, h_polarity, v_polarity, interlaced, repeat_times, frame_rate, bpp, video_mode;

    {0, "1400x1050@60",	  	108,	        1688,	1400,     1066,     1050,	     88,     50,	    150,	 9,  4,  3,   1,  1,  0, 1,	60, 1, 0},
    {1, "1280X720P@60",       75,          1650,      1280,    750,       720,       110,    40,       220,     5,  5,  20,   0,0,  0, 1,    60, 1, 1},
    {2, "1680X1050@60",       146.25,    2240,     1680,     1089,     1050,      104,   176,     280,      3,  6 , 30,  0,  1, 0, 1,   60, 1, 0},
    {3, "1920x1080p@60",     148.5,      2200,      1920,     1125,     1080,      88,     44,      148,      4, 5, 36, 0,0,  0, 1,    60, 1, 1},  
    {4, "2560x1600@60",		268,  	 2720,	2560,     1646,     1600,      48,     32,      80,	 2,  6 , 38,  1,  1, 0, 1, 	60, 1, 0},
    {5, "640x480@60",	       25,	        840,       640,	      500,       480,	     16,     64,      120,	 1,  3,  16,  1,  1,  0, 1,	75, 1, 0},
    {6, "640x480@75",	      	31.5,	 840,	640,	      500,       480,	     16,     64,       120,	 1,  3,  16,  1,  1,	0, 1,	75, 1, 0},
    {7, "1920*1200@60",		154,	        2080,	1920,     1235,     1200,	     40,     80,	     40,	 3,  6,  26,  1,  1, 0, 1,	60, 1, 0},    
    {8, "800x600@75",	  	49.5,	 1056,	800,       628,       600,	     40,     128,	     88,	 3,  6,  29,  1,  1, 0, 1,	75, 1, 0}, 
    {9, "800x480@60",           25,          848,	800,      493,        480,       14,     8,         26,       5,  1,   1,	  1,	1, 0, 1,	60, 1,0}
};



void SP_TX_Initialization(struct VideoFormat* pInputFormat)
{
    BYTE c;

	 //software reset	 
	 SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, &c);
	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, c | SP_TX_RST_SW_RST);
	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, c & ~SP_TX_RST_SW_RST);
	
	 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_EXTRA_ADDR_REG, 0x50);//EDID address for AUX access
	 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CTRL, 0x02);	//disable HDCP polling mode.
	 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_DEBUG_REG, 0x30);//enable M value read out

        /*added for B0 to enable enable c-wire polling-ANX.Fei-20110831*/
	 SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_DEBUG_REG1, &c);

	 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DEBUG_REG1, ((c&0x00)|0x82));//disable polling HPD, force hotplug for HDCP, enable polling


         /*added for B0 to change the c-wire termination from 100ohm to 50 ohm for polling error iisue-ANX.Fei-20110916*/
	 SP_TX_Read_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL1, &c);
	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL1, (c|0x30));//change the c-wire termination from 100ohm to 50 ohm

        SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x37, 0x26);//set 400mv3.5db value according to mehran CTS report-ANX.Fei-20111009
        SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x47, 0x10);//set 400mv3.5db value according to mehran CTS report-ANX.Fei-20111009

    	 SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, &c);
	 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, c | 0x03);//set KSV valid

	 //SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	 //SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, c& (~0x08));//set signle AUX output

	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK1, 0x00);//mask all int
	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK2, 0x00);//mask all int
	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK3, 0x00);//mask all int
	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK4, 0x00);//mask all int

      //PHY parameter for cts
     //change for 200--400mv, 300-600mv, 400-800nv
     //400mv (actual 200mv)
      //Swing
      SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x30, 0x16);//0db
      SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x36, 0x1b);//3.5db
      SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x39, 0x22);//6db
      SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x3b, 0x23);//9db
      
      //Pre-emphasis
      //SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x40, 0x00);//0db
      SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x46, 0x09);//3.5db
      SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x49, 0x16);//6db
      SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x4b, 0x1F);//9db

     
     //600mv (actual 300mv)
     //Swing
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x31, 0x26);//0db
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x37, 0x28);//3.5db
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x3A, 0x2F);//6db

     //Pre-emphasis
     //SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x41, 0x00);//0db
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x47, 0x10);//3.5db
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x4A, 0x1F);//6db


     //800mv (actual 400mv)
     //Swing
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x34, 0x36);//0db
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x38, 0x3c);//3.5db
     //emp     
     //SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x44, 0x00);//0db
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x48, 0x10);//3.5db

     //1200mv (actual 600mv)
     //Swing
     SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x35, 0x3F);//0db
     //Pre-emphasis     
     //SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x45, 0x00);//0db

        /*added for B0 version-ANX.Fei-20110831-Begin*/
	 SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_INT_MASK, 0xb4);//0xb0 unmask IRQ request Int & c-wire polling error int

     	 //M value select, select clock with downspreading
	 SP_TX_API_M_GEN_CLK_Select(1);

	 if(pInputFormat->Interface == LVTTL_RGB)
	 {
		//set clock edge
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, (c & 0xfc) |0x03);

		//set Input BPC mode & color space
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
		c &= 0x8c;
		c = c |(pInputFormat->bColordepth << 4);
		c |= pInputFormat->bColorSpace;
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, c);
	 }


	 //SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DP_POLLING_PERIOD, 0x01);

	  SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0x0F, 0x10);//0db
}


void SP_TX_Power_Down(SP_TX_POWER_BLOCK sp_tx_pd_block)
{
    BYTE c;
    
    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_POWERD_CTRL_REG , &c);
	
	if(sp_tx_pd_block == SP_TX_PWR_REG)//power down register
		c |= SP_POWERD_REGISTER_REG;
	else if(sp_tx_pd_block == SP_TX_PWR_HDCP)//power down IO
		c |= SP_POWERD_HDCP_REG;
	else if(sp_tx_pd_block == SP_TX_PWR_AUDIO)//power down audio
		c |= SP_POWERD_AUDIO_REG;
	else if(sp_tx_pd_block == SP_TX_PWR_VIDEO)//power down video
		c |= SP_POWERD_VIDEO_REG;
	else if(sp_tx_pd_block == SP_TX_PWR_LINK)//power down link
		c |= SP_POWERD_LINK_REG;
	else if(sp_tx_pd_block == SP_TX_PWR_TOTAL)//power down total.
		c |= SP_POWERD_TOTAL_REG;
		
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_POWERD_CTRL_REG, c);
	
    //D("SP_TX_Power_Down");
}

void SP_TX_Power_On(SP_TX_POWER_BLOCK sp_tx_pd_block)
{
    BYTE c;
    
    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_POWERD_CTRL_REG , &c);

	if(sp_tx_pd_block == SP_TX_PWR_REG)//power on register
		{
			c &= ~SP_POWERD_REGISTER_REG;
			//D("Register is power on");
		}
	else if(sp_tx_pd_block == SP_TX_PWR_HDCP)//power on IO
		{
			c &= ~SP_POWERD_HDCP_REG;
			//D("IO is power on");
		}
	else if(sp_tx_pd_block == SP_TX_PWR_AUDIO)//power on audio
		{
			c &= ~SP_POWERD_AUDIO_REG;
			//D("Audio  is power on");
		}
		
	else if(sp_tx_pd_block == SP_TX_PWR_VIDEO)//power on video
		{
			c &= ~SP_POWERD_VIDEO_REG;
			//D("Video is power on");
		}
	else if(sp_tx_pd_block == SP_TX_PWR_LINK)//power on link
		{
			c &= ~SP_POWERD_LINK_REG;
			//D("Link is power on");
		}
	else if(sp_tx_pd_block == SP_TX_PWR_TOTAL)//power on total.
		{
			c &= ~SP_POWERD_TOTAL_REG;
			//D("Total is power on");
		}


    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_POWERD_CTRL_REG, c);
	
}


void SP_TX_Config_Video (struct VideoFormat* pInputFormat)
{
    BYTE c,c1;
	WORD wPacketLenth;

	//SP_TX_Clean_HDCP();
	SP_CTRL_Clean_HDCP();
	SP_TX_Power_On(SP_TX_PWR_VIDEO);

	if(pInputFormat->Interface == LVTTL_RGB)
	{
		if(SP_TX_Config_Video_LVTTL(pInputFormat))
			return;
	}
	else if(pInputFormat->Interface == MIPI_DSI)
	{
		if(SP_TX_Config_Video_MIPI())
			return;
	}

	//if(pInputFormat->Interface == LVTTL_RGB)
	//SP_TX_Config_Video_LVTTL(pInputFormat);
	//else if(pInputFormat->Interface == MIPI_DSI)
	//SP_TX_Config_Video_MIPI();

	//enable video input
	SP_TX_Enable_Video_Input();

	mdelay(50);
	if(pInputFormat->Interface == LVTTL_RGB)
	{
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, c);
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, &c);
		if(!(c & SP_TX_SYS_CTRL3_STRM_VALID))
		{
			D("video stream not valid!");
			return;
		}
		D("video stream valid!");
	}
	else //MIPI DSI
	{
		//check video packet to determin if video stream valid
		SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_LONG_PACKET_LENTH_LOW, &c);
		SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_LONG_PACKET_LENTH_HIGH, &c1);

		wPacketLenth = (mipi_video_timing_table[bMIPIFormatIndex].MIPI_HActive)*3;

		if(((wPacketLenth&0x00ff)!=c)||(((wPacketLenth&0xff00)>>8)!=c1))
		{
			D("mipi video stream not valid!");
			return;
		}
		else
			D("mipi video stream valid!");
	}
    
       SP_TX_AVI_Setup();//initial AVI infoframe packet
       SP_TX_Config_Packets(AVI_PACKETS);
       //3d packed config
     #ifdef EN_3D
	{
		 
		 D("##########Config 3D Start!#############");
		 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0xeb, 0x02);//set 3D format type to stacked frame
		 SP_TX_Read_Reg(SP_TX_PORT0_ADDR, 0xea, &c);
		 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0xea, c|0x01);//enable VSC

		 SP_TX_Read_Reg(SP_TX_PORT0_ADDR, 0x90, &c);
		 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0x90, (c&0xfe));//set spd_en=0 

		 SP_TX_Read_Reg(SP_TX_PORT0_ADDR, 0x90, &c);
		 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0x90, (c|0x10));//read spd_update=1

	        SP_TX_Read_Reg(SP_TX_PORT0_ADDR, 0x90, &c);
		 SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0x90, c|0x01);//read spd_en=1


		 D("##########Config 3D End!#############");
		  
		 ///////////////////////////////////////////////////////////////
	}
    #endif


	
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK1, 0xf5);//unmask video clock change&format change int
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_STATUS1, 0x0a);//Clear video format and clock change-20111206-ANX.Fei
	
	
	//if(Force_AUD)//force to configure audio regadless of EDID info.
	 sp_tx_ds_edid_hdmi =1;
	
        if(sp_tx_ds_edid_hdmi)
        {
            SP_CTRL_Set_System_State(SP_TX_CONFIG_AUDIO);
        }
        else
        {
            SP_TX_Power_Down(SP_TX_PWR_AUDIO);//power down audio when DVI
            SP_CTRL_Set_System_State(SP_TX_HDCP_AUTHENTICATION);
        }

	SP_TX_Video_Mute(1);

}

/*
void SP_TX_DE_reGenerate (WORD sp_tx_bist_select_number)
{
    BYTE c;

	//interlace scan mode configuration
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG,
	    c & 0xfb | (video_timing_table[sp_tx_bist_select_number].is_interlaced<< 2));

	//V sync polarity
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, 
	    c & 0xfd | (video_timing_table[sp_tx_bist_select_number].v_sync_polarity<< 1) );

	//H sync polarity
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, 
	    c & 0xfe | (video_timing_table[sp_tx_bist_select_number].h_sync_polarity) );

	//active line
	c = video_timing_table[sp_tx_bist_select_number].v_active_length& 0xff;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINEL_REG, c);
	c = video_timing_table[sp_tx_bist_select_number].v_active_length >> 8;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINEH_REG, c);

	//V sync width
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VSYNC_CFG_REG, 
	    video_timing_table[sp_tx_bist_select_number].v_sync_width);

	//V sync back porch.
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VB_PORCH_REG, 
	    video_timing_table[sp_tx_bist_select_number].v_back_porch);

	//total pixel in each frame
	c = video_timing_table[sp_tx_bist_select_number].h_total_length& 0xff;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXELL_REG, c);
	c = video_timing_table[sp_tx_bist_select_number].h_total_length>> 8;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXELH_REG, c);

	//active pixel in each frame.
	c = video_timing_table[sp_tx_bist_select_number].h_active_length& 0xff;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXELL_REG, c);
	c = video_timing_table[sp_tx_bist_select_number].h_active_length >> 8;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXELH_REG, c);

	//pixel number in H period
	c = video_timing_table[sp_tx_bist_select_number].h_sync_width& 0xff;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HSYNC_CFGL_REG, c);
	c = video_timing_table[sp_tx_bist_select_number].h_sync_width >> 8;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HSYNC_CFGH_REG, c);

	//pixel number in frame horizontal back porch
	c = video_timing_table[sp_tx_bist_select_number].h_back_porch& 0xff;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HB_PORCHL_REG, c);
	c = video_timing_table[sp_tx_bist_select_number].h_back_porch >> 8;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HB_PORCHH_REG, c);

	//enable DE mode.
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, c | SP_TX_VID_CTRL1_DE_GEN);
}

void SP_TX_Embedded_Sync(struct VideoFormat* pInputFormat, WORD sp_tx_bist_select_number)
{
    BYTE c;

	//set embeded sync flag check 
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, 
	    (c & ~SP_TX_VID_CTRL4_EX_E_SYNC) | 
	    pInputFormat->bLVTTL_HW_Interface.sEmbedded_Sync.Extend_Embedded_Sync_flag << 6);

	// SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_VID_CTRL3_REG, &c);
	// SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_VID_CTRL3_REG,(c & 0xfb | 0x02));

	//set Embedded sync repeat mode.
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, 
	    c & 0xcf | (video_timing_table[sp_tx_bist_select_number].pix_repeat_times<< 4) );

	//V sync polarity
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, 
	    c & 0xfd | (video_timing_table[sp_tx_bist_select_number].v_sync_polarity<< 1) );

	//H sync polarity
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL10_REG, 
	    c & 0xfe | (video_timing_table[sp_tx_bist_select_number].h_sync_polarity) );

	//V  front porch.
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VF_PORCH_REG, 
	    video_timing_table[sp_tx_bist_select_number].v_front_porch);

	//V sync width
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VSYNC_CFG_REG, 
	    video_timing_table[sp_tx_bist_select_number].v_sync_width);

	//H front porch
	c = video_timing_table[sp_tx_bist_select_number].h_front_porch& 0xff;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HF_PORCHL_REG, c);
	c = video_timing_table[sp_tx_bist_select_number].h_front_porch >> 8;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HF_PORCHH_REG, c);

	//H sync width
	c = video_timing_table[sp_tx_bist_select_number].h_sync_width& 0xff;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HSYNC_CFGL_REG, c);
	c = video_timing_table[sp_tx_bist_select_number].h_sync_width >> 8;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HSYNC_CFGH_REG, c);

	//Enable Embedded sync 
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, c | SP_TX_VID_CTRL4_E_SYNC_EN);

}
*/
BYTE SP_TX_Config_Video_LVTTL (struct VideoFormat* pInputFormat)
{
    BYTE c;//,i;
    //power down MIPI,enable lvttl input
	SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_ANALOG_PWD_CTRL1, &c);
	c |= 0x10;
	SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_ANALOG_PWD_CTRL1, c);

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL1_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL1_REG, c);
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL1_REG, &c);
	if(!(c & SP_TX_SYS_CTRL1_DET_STA))
	{
		D_MSG("Stream clock not found!");
		return 1;
	}

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL2_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL2_REG, c);
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL2_REG, &c);
	if(c & SP_TX_SYS_CTRL2_CHA_STA)
	{
		D_MSG("Stream clock not stable!");
		return 1;
	}

    	if(pInputFormat->bColorSpace==COLOR_YCBCR_444)
	{
		//D("ColorSpace YCbCr444");
              SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
              SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, ((c&0xfc)|0x02));
		//SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL5_REG, 0x90);//enable Y2R conversion based on BT709
		//SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL6_REG, 0x40);//enable video porcess
	}
	else if(pInputFormat->bColorSpace==COLOR_YCBCR_422)
	{
            //D("ColorSpace YCbCr422");
            SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, ((c&0xfc)|0x01));


           // SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL5_REG, 0x90);//enable Y2R conversion based on BT709
            //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL6_REG, 0x42);//enable video porcess and upsample
	}
	else
	{
            //D("ColorSpace RGB");
            SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, (c&0xfc));
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL5_REG, 0x00);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL6_REG, 0x00);
	}
    
    
	if(pInputFormat->bLVTTL_HW_Interface.sEmbedded_Sync.Embedded_Sync)
	{
		D_MSG("Embedded_Sync");
		//SP_TX_Embedded_Sync(pInputFormat,1);//set 720p as the default
	}
	else
	{
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL4_REG, (c & ~SP_TX_VID_CTRL4_E_SYNC_EN));
	}

	if(pInputFormat->bLVTTL_HW_Interface.DE_reDenerate)
	{
		D_MSG("DE_reDenerate\n");
		//SP_TX_DE_reGenerate(1);//set 720p as the default
	}
	else
	{
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, (c & ~SP_TX_VID_CTRL1_DE_GEN));
	}

	if(pInputFormat->bLVTTL_HW_Interface.sYC_MUX.YC_MUX)
	{
		D_MSG("YC_MUX\n");
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, ((c & 0xef) | SP_TX_VID_CTRL1_DEMUX));

		if(pInputFormat->bLVTTL_HW_Interface.sYC_MUX.YC_BIT_SEL)
		{
			SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, ((c & 0xfb) |SP_TX_VID_CTRL1_YCBIT_SEL) );
		}
		
	}
	else
	{
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, (c & ~SP_TX_VID_CTRL1_DEMUX));
	}

	if(pInputFormat->bLVTTL_HW_Interface.DDR_Mode)
	{
		D_MSG("DDR_mode\n");
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, ((c & 0xfb) | SP_TX_VID_CTRL1_IN_BIT));
	}
	else
	{
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, (c & ~SP_TX_VID_CTRL1_IN_BIT));
	}

	SP_TX_LVTTL_Bit_Mapping(pInputFormat);
	return 0;

}

void SP_TX_LVTTL_Bit_Mapping(struct VideoFormat* pInputFormat)//the default mode is 12bit ddr
{
    
	BYTE c,offset,value;
	if(pInputFormat->bLVTTL_HW_Interface.DDR_Mode)
        {
            switch(pInputFormat->bColordepth)
            {
                case COLOR_8_BIT://correspond with ANX8770 24bit DDR mode 
                    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, (((c&0x8f)|0x10)));//set input video 8-bit
                    for(c=0; c<12; c++)
                    {	 
                        offset = (0x40+c);
                        value = (0x06+c);
                        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, offset, value);
                    }
                    for(c=0; c<12; c++)
                    {	 
                        offset = (0x4d+c);
                        value = (c + 0x1e);
                        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, offset, value);
                    }
                    break;
                case COLOR_10_BIT://correspond with ANX8770 30bit DDR mode 
                    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG,( ((c&0x8f)|0x20)));//set input video 10-bit
                    for(c=0; c<15; c++)
                    {	 
                        offset = 0x40+c;
                        value = 0x03+c;
                        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, offset,value);
                    }
                    for(c=0; c<15; c++)
                    {	 
                        offset = (0x4f+c);
                        value = (0x1b+c);
                        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, offset, value);
                    }
                    break;
                case COLOR_12_BIT://correspond with ANX8770 36bit DDR mode 
                    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, (((c&0x8f)|0x30)));//set input video 12-bit
                    for(c=0; c<18; c++)
                    {	 
                        offset = (0x40+c);
                        value = (0x00+c);
                        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, offset, value);
                    }
                    for(c=0; c<18; c++)
                    {	
                        offset = (0x52+c);
                        value = (0x18+c);
                        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, offset, value);
                    }
                    break;	
                default:
                    break;
            }
        }
       else
        {
            switch(pInputFormat->bColordepth)
            {
                case COLOR_8_BIT://8bit SDR mode
                    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG, &c);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL2_REG,( ((c&0x8f)|0x10)));//set input video 8-bit
                    for(c=0; c<24; c++)
                    {	 
                        offset = (0x40+c);
                        value = (0x00+c);
                        SP_TX_Write_Reg(SP_TX_PORT2_ADDR, (0x40+c), (0x00+c));
                    }
                    break;
                default:
                        break;
            }
        }
	
		//mdelay(10);
}
BYTE  SP_TX_Config_Video_MIPI (void)
{
        BYTE c;//,i;
    
        if(!bMIPI_Configured)
        {
		//config video format
		SP_TX_Config_MIPI_Video_Format();

		//force blanking vsync
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, 0xB4, &c);
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, 0xb4, (c|0x02));

		//power down MIPI,
		SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_ANALOG_PWD_CTRL1, &c);
		c |= 0x10;
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_ANALOG_PWD_CTRL1, c);

		//set power up counter, never power down high speed data path during blanking
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_TIMING_REG2, 0x40);
		//SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_TIMING_REG3, 0xC4);

		//set LP high reference voltage to 800mv
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x2A, 0x0b);

		//SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x1c, 0x10);
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x1c, 0x11);
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x1b, 0xbb);

		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x19, 0x3e);

		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x08, 0x12);



		//set mipi data lane count

		SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_MISC_CTRL, &c);

		c&= 0xF9;
		c|= (MIPI_LANE_COUNT-1)<<1;// four lanes	

		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_MISC_CTRL, c);//Set 4 lanes, link clock 270M


		//power on MIPI, //enable MIPI input
		SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_ANALOG_PWD_CTRL1, &c);
		c &= 0xEF;
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_ANALOG_PWD_CTRL1, c);

		//control reset_n_ls_clk_comb to reset mipi
		SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_MISC_CTRL, &c);
		c&=0xF7;
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_MISC_CTRL, c);
		mdelay(1);
		c|=0x08;
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_MISC_CTRL, c);

		//reset low power mode
		SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_TIMING_REG2, &c);
		c|=0x01;
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_TIMING_REG2, c);
		mdelay(1);
		c&=0xfe;
		SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, MIPI_TIMING_REG2, c);

		D("MIPI configured!");

		SP_TX_MIPI_CONFIG_Flag_Set(1);
        
    
        }
        else
            D("MIPI interface enabled");
        
    
        SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL1_REG, &c);
        SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL1_REG, c);
        SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL1_REG, &c);
        if(!(c & SP_TX_SYS_CTRL1_DET_STA))
        {
            D("Stream clock not found!");
            D("0x70:0x80=%.2x\n",(WORD)c);
            mdelay(1000);
            return 1;
        }
#ifdef MIPI_DEBUG
        else
        {
            D("#######Stream clock found!");
            D("0x70:0x80=%.2x\n",(WORD)c);
        }
#endif
            
    
        SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL2_REG, &c);
        SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL2_REG, c);
        SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL2_REG, &c);
        if(c & SP_TX_SYS_CTRL2_CHA_STA)
        {
            D("Stream clock not stable!");
            D("0x70:0x04=%.2x\n",(WORD)c);
            mdelay(1000);
    
            return 1;
        }
#ifdef MIPI_DEBUG
        else
        {
            D("#######Stream clock stable!");
            D("0x70:0x04=%.2x\n",(WORD)c);
        }
#endif
    



      return 0;

}


void SP_TX_Disable_Video_Input(void)
{
    BYTE c;
    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, c &~SP_TX_VID_CTRL1_VID_EN);
}



void SP_TX_EnhaceMode_Set(void)
{
    BYTE c;    
	SP_TX_AUX_DPCDRead_Bytes(0x00,0x00,DPCD_MAX_LANE_COUNT,1,&c);
	if(c & 0x80)
	{

		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL4_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL4_REG, c | SP_TX_SYS_CTRL4_ENHANCED);

		SP_TX_AUX_DPCDRead_Bytes(0x00,0x01,DPCD_LANE_COUNT_SET,1,&c);
		SP_TX_AUX_DPCDWrite_Byte(0x00,0x01,DPCD_LANE_COUNT_SET, c | 0x80);

		D_MSG("Enhance mode enabled");
	}
	else
	{

		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL4_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL4_REG, c & (~SP_TX_SYS_CTRL4_ENHANCED));

		SP_TX_AUX_DPCDRead_Bytes(0x00,0x01,DPCD_LANE_COUNT_SET,1,&c);
		SP_TX_AUX_DPCDWrite_Byte(0x00,0x01,DPCD_LANE_COUNT_SET, c & (~0x80));

		D_MSG("Enhance mode disabled");
	}
}


void SP_TX_Clean_HDCP(void)
{
    BYTE c;
    
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, 0x00);//disable HW HDCP

    //reset HDCP logic
    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, &c);
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, c | SP_TX_RST_HDCP_REG);
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, c& ~SP_TX_RST_HDCP_REG);

    //set re-auth
    SP_TX_HDCP_ReAuth();

}

void SP_TX_HDCP_Encryption_Disable(void) 
{
    BYTE c;     
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, &c);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, c & ~SP_TX_HDCP_CONTROL_0_HDCP_ENC_EN);
}

void SP_TX_HDCP_Encryption_Enable(void)
{
    BYTE c;
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, &c);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, c |SP_TX_HDCP_CONTROL_0_HDCP_ENC_EN);
}

void SP_TX_HW_HDCP_Enable(void)
{
    BYTE c;
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, &c);
    c&=0xf3;
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, c );
    c|=0x0c;//enable HDCP and encryption
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, c );
    D_MSG("Hardware HDCP is enabled.");

}

void SP_TX_HW_HDCP_Disable(void)
{
    BYTE c;
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, &c);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, c & ~SP_TX_HDCP_CONTROL_0_HARD_AUTH_EN);
}

void SP_TX_PCLK_Calc(BYTE hbr_rbr)
{
    long int str_clk = 0;
    BYTE c;

    switch(hbr_rbr)
    {
      case BW_54G:
	  	str_clk = 540;
		break;
      /*case BW_45G:
	  	str_clk = 450;
		break;*/
      case BW_27G:
	  	str_clk = 270;
		break;
      case BW_162G:
	  	str_clk = 162;
		break;
	default:
		break;
	  
    }


    SP_TX_Read_Reg(SP_TX_PORT0_ADDR,M_VID_2, &c);
    M_val = c * 0x10000;
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR,M_VID_1, &c);
    M_val = M_val + c * 0x100;
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR,M_VID_0, &c);
    M_val = M_val + c;

    SP_TX_Read_Reg(SP_TX_PORT0_ADDR,N_VID_2, &c);
    N_val = c * 0x10000;
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR,N_VID_1, &c);
    N_val = N_val + c * 0x100;
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR,N_VID_0, &c);
    N_val = N_val + c;

    str_clk = str_clk * M_val;
    pclk = str_clk ;
    pclk = pclk / N_val;
}


void SP_TX_Show_Infomation(void)
{
	BYTE c,c1;
	WORD h_res,h_act,v_res,v_act;
	WORD h_fp,h_sw,h_bp,v_fp,v_sw,v_bp;
	long int fresh_rate;
	BYTE vbus_status;
	D("\n*************************SP Video Information*************************\n");

	D("   SP TX mode = Normal mode");



	SP_TX_Read_Reg(SP_TX_PORT0_ADDR,SP_TX_LANE_COUNT_SET_REG, &c);
	if(c==0x00)
		D("   LC = 1");
	//else if(c==0x02)
	//	D("   LC = 2");
	//else
	//	D("   LC = 4");


	SP_TX_Read_Reg(SP_TX_PORT0_ADDR,SP_TX_LINK_BW_SET_REG, &c);
	if(c==0x06)
	{
		D("   BW = 1.62G");
		SP_TX_PCLK_Calc(BW_162G);//str_clk = 162;
	}
	else if(c==0x0a)
	{
		D("   BW = 2.7G");
		SP_TX_PCLK_Calc(BW_27G);//str_clk = 270;
	}
	else if(c==0x14)
	{
		D("   BW = 5.4G");
		SP_TX_PCLK_Calc(BW_54G);//str_clk = 540;
	}
	
	

	if(sp_tx_ssc_enable)
		D("   SSC On");
	else
		D("   SSC Off");

	D("   M = %lu, N = %lu, PCLK = %lu MHz\n",M_val,N_val,pclk);


	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_LINE_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_LINE_STA_H,&c1);

	v_res = c1;
	v_res = v_res << 8;
	v_res = v_res + c;


	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINE_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINE_STA_H,&c1);

	v_act = c1;
	v_act = v_act << 8;
	v_act = v_act + c;


	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXEL_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXEL_STA_H,&c1);

	h_res = c1;
	h_res = h_res << 8;
	h_res = h_res + c;


	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXEL_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXEL_STA_H,&c1);

	h_act = c1;
	h_act = h_act << 8;
	h_act = h_act + c;

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_H_F_PORCH_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_H_F_PORCH_STA_H,&c1);
	
	h_fp = c1;
	h_fp = h_fp << 8;
	h_fp = h_fp + c;

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_H_SYNC_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_H_SYNC_STA_H,&c1);
	
	h_sw = c1;
	h_sw = h_sw << 8;
	h_sw = h_sw + c;

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_H_B_PORCH_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_H_B_PORCH_STA_H,&c1);
	
	h_bp = c1;
	h_bp = h_bp << 8;
	h_bp = h_bp + c;

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_V_F_PORCH_STA,&c);
	v_fp = c;

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_V_SYNC_STA,&c);
	v_sw = c;

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_V_B_PORCH_STA,&c);
	v_bp = c;
	
	D("   Total resolution is %d * %d \n", h_res, v_res);
	
	D("   HF=%d, HSW=%d, HBP=%d\n", h_fp, h_sw, h_bp);
	D("   VF=%d, VSW=%d, VBP=%d\n", v_fp, v_sw, v_bp);
	D("   Active resolution is %d * %d ", h_act, v_act);
	

/*
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_LINE_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_LINE_STA_H,&c1);

	v_res = c1;
	v_res = v_res << 8;
	v_res = v_res + c;


	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINE_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINE_STA_H,&c1);

	v_act = c1;
	v_act = v_act << 8;
	v_act = v_act + c;


	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXEL_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXEL_STA_H,&c1);

	h_res = c1;
	h_res = h_res << 8;
	h_res = h_res + c;


	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXEL_STA_L,&c);
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXEL_STA_H,&c1);

	h_act = c1;
	h_act = h_act << 8;
	h_act = h_act + c;

	D("   Total resolution is %d * %d \n", h_res, v_res);
	D("   Active resolution is %d * %d ", h_act, v_act);
*/
	{
	fresh_rate = pclk * 1000;
	fresh_rate = fresh_rate / h_res;
	fresh_rate = fresh_rate * 1000;
	fresh_rate = fresh_rate / v_res;
	D(" @ %luHz\n", fresh_rate);
	}

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_VID_CTRL,&c);
	if((c & 0x06) == 0x00)
	D("   ColorSpace: RGB,");
	else if((c & 0x06) == 0x02)
	D("   ColorSpace: YCbCr422,");
	else if((c & 0x06) == 0x04)
	D("   ColorSpace: YCbCr444,");


	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_VID_CTRL,&c);
	if((c & 0xe0) == 0x00)
	D("  6 BPC");
	else if((c & 0xe0) == 0x20)
	D("  8 BPC");
	else if((c & 0xe0) == 0x40)
	D("  10 BPC");
	else if((c & 0xe0) == 0x60)
	D("  12 BPC");


	SP_TX_AUX_DPCDRead_Bytes(0x00, 0x05, 0x23, 1, ByteBuf);
	if((ByteBuf[0]&0x0f)!=0x02)
		{
			D("   ANX7730 BB current FW Ver %.2x \n", (WORD)(ByteBuf[0]&0x0f));
			D("   The latest version is 02 ");
		}
	else
		D("   ANX7730 BB current FW is the latest version 02.");
	vbus_status = VBUS_PWR_CTRL;
	if(vbus_status)
	   D("   Slimport cable work in CoolHD mode");
		


	D("\n********************************************************************\n");

}


    
 void SP_TX_Enable_Video_Input(void)
{
    BYTE c;

       SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
       c &= 0xf7;
	//D("not one lane\n");
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, ( c | SP_TX_VID_CTRL1_VID_EN ));

    D("Video Enabled!");

    
}


void SP_TX_AUX_WR (BYTE offset)
{
	BYTE c,cnt;
	cnt = 0;
    //load offset to fifo
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG, offset);
    //set I2C write com 0x04 mot = 1
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, 0x04);
	//enable aux
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x01);
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
    while(c&0x01)
    {
        mdelay(10);
        cnt ++;
        //D("cntwr = %.2x\n",(WORD)cnt);
        if(cnt == 10)
        {
            D("write break");
            //SP_TX_RST_AUX();
            cnt = 0;
            bEDIDBreak=1;
            break;
        }
        SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
    }

} 

void SP_TX_AUX_RD (BYTE len_cmd)
{
	BYTE c,cnt;
	cnt = 0;
	
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, len_cmd);
	//enable aux
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x01);
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	while(c & 0x01)
	{
		mdelay(10);
		cnt ++;
        //D("cntrd = %.2x\n",(WORD)cnt);
		if(cnt == 10)
		{
			D("read break");
			SP_TX_RST_AUX();
                     bEDIDBreak=1;
			break;
		}
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	}

}



void SP_TX_Insert_Err(void)
{
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_DEBUG_REG, 0x02);
	D("Insert err\n");
}

int SP_TX_Chip_Located(void)
{
    BYTE c1,c2;
    SP_TX_Hardware_PowerOn();
    mdelay(10);
   // SP_TX_Hardware_Reset();
    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_DEV_IDL_REG , &c1);
    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_DEV_IDH_REG , &c2);


    if ((c1==0x04) && (c2==0x78))
    {
        D("Chip revision B0");      
    }
    else if ((c1==0x05) && (c2==0x78))
    {
        D("Chip revision A0");
    }
    else
    	D("Chip revision error 0x%02x 0x%02x\n", c1, c2);
    return 1;
        
}
void SP_TX_Hardware_PowerOn(void) 
{

	gpio_set_value(SP_TX_HW_RESET,0);//SP_TX_HW_RESET= 0;
	mdelay(20);
	gpio_set_value(SP_TX_CHIP_PD_CTRL,0);//SP_TX_CHIP_PD_CTRL= 0;
	mdelay(20);
	gpio_set_value(SP_TX_PWR_V12_CTRL,0);//SP_TX_PWR_V12_CTRL= 0;
	mdelay(10);
	gpio_set_value(SP_TX_PWR_V12_CTRL,1);//SP_TX_PWR_V12_CTRL= 1;
	mdelay(500);
	gpio_set_value(SP_TX_HW_RESET,1);//SP_TX_HW_RESET= 0;
	mdelay(20);
	//SP_TX_Hardware_Reset();
	D_MSG("Chip is power on\n");

}

void SP_TX_Hardware_PowerDown(void) 
{
	gpio_set_value(SP_TX_HW_RESET,0);// SP_TX_HW_RESET= 0;
	mdelay(10);
	gpio_set_value(SP_TX_PWR_V12_CTRL,0);//SP_TX_PWR_V12_CTRL= 0;
	mdelay(10);
	gpio_set_value(SP_TX_CHIP_PD_CTRL,1);//SP_TX_CHIP_PD_CTRL = 1;
       mdelay(20);

	D_MSG("Chip is power down\n");
	
}

void SP_TX_VBUS_PowerOn(void) 
{
    gpio_set_value(VBUS_PWR_CTRL,1);//VBUS_PWR_CTRL= 0;
    mdelay(10);
}

void SP_TX_VBUS_PowerDown(void) 
{
    gpio_set_value(VBUS_PWR_CTRL,0);//VBUS_PWR_CTRL= 1;
}

void SP_TX_Hardware_Reset(void) 
{
    gpio_set_value(SP_TX_HW_RESET,0);//SP_TX_HW_RESET= 0;
    mdelay(20);
    gpio_set_value(SP_TX_HW_RESET,1);//SP_TX_HW_RESET = 1;
    mdelay(10);
}






void SP_TX_CONFIG_SSC(SP_LINK_BW linkbw) 
{
	BYTE c;

	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SSC_CTRL_REG1, 0x00); 			// disable SSC first
	//SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_BW_SET_REG, 0x00);		//disable speed first

	
	SP_TX_AUX_DPCDRead_Bytes(0x00, 0x00,DPCD_MAX_DOWNSPREAD,1,&c);
          

#ifndef SSC_1
	//D("############### Config SSC 0.4% ####################");
	if(linkbw == BW_54G) 
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL1, 0xc0);	              // set value according to mehran CTS report -ANX.Fei-20111009
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, 0x00);	              
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL3, 0x75);			// ctrl_th 
	
	}
	else if(linkbw == BW_27G) 
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL1, 0x5f);			// ssc d  0.4%, f0/4 mode
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, 0x00);	
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL3, 0x75);			// ctrl_th 
	}
	else 
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL1, 0x9e);         	//  set value according to mehran CTS report -ANX.Fei-20111009
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, 0x00);	
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL3, 0x6d);			// ctrl_th 
	}
#else
      //D("############### Config SSC 1% ####################");
	if(linkbw == BW_54G) 
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL1, 0xdd);	              // ssc d  1%, f0/8 mode
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, 0x01);	              
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL3, 0x76);			// ctrl_th 
	}
	else if(linkbw == BW_27G) 
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL1, 0xef);			// ssc d  1%, f0/4 mode
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, 0x00);	
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL3, 0x76);			// ctrl_th 
	}
	else 
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL1, 0x8e);			// ssc d 0.4%, f0/4 mode
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, 0x01);	
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL3, 0x6d);			// ctrl_th 
	}
#endif

	//Enable SSC
	SP_TX_SPREAD_Enable(1);


}



void SP_TX_Config_Audio_SPDIF(void) 
{
       BYTE c;

       D("############## Config SPDIF audio #####################\n");

	// disable  I2S input
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CTRL, 0x00 ); 
	   
	SP_TX_Read_Reg (SP_TX_PORT2_ADDR, SPDIF_AUDIO_CTRL0, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SPDIF_AUDIO_CTRL0, (c | SPDIF_AUDIO_CTRL0_SPDIF_IN) ); // enable SPDIF input

	mdelay(2);

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SPDIF_AUDIO_STATUS0, &c);

	if ( ( c & SPDIF_AUDIO_STATUS0_CLK_DET ) != 0 ) 
		D("SPDIF Clock is detected!\n");
	else 
		D("ERR:SPDIF Clock is Not detected!\n");

	if ( ( c & SPDIF_AUDIO_STATUS0_AUD_DET ) != 0 ) 
		D("SPDIF Audio is detected!\n");
	else 
		D("ERR:SPDIF Audio is Not detected!\n");
}

void SP_TX_Config_Audio_I2S(struct AudioFormat *bAudioFormat) 
{
     BYTE c;

	 // Disable SPDIF input
	SP_TX_Read_Reg (SP_TX_PORT2_ADDR, SPDIF_AUDIO_CTRL0, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SPDIF_AUDIO_CTRL0, ((c & 0x7e )|0x01)); 
	
       D("############## Config I2S audio #####################\n");
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CTRL,&c); // enable I2S input
       c = (c&~0xff)|bAudioFormat->bI2S_FORMAT.SHIFT_CTRL<<3|bAudioFormat->bI2S_FORMAT.DIR_CTRL <<2 
	   	|bAudioFormat->bI2S_FORMAT.WS_POL <<1|bAudioFormat->bI2S_FORMAT.JUST_CTRL;
	 switch(bAudioFormat->bI2S_FORMAT.Channel_Num)
	 {
		case I2S_CH_2: 
			c = c|0x10;
			break;
		case I2S_CH_4:
			c = c|0x30;
			break;
		case I2S_CH_6: 
			c = c|0x70;
			break;
		case I2S_CH_8:
			c = c|0xf0;
			break;
		default: 
			c = c|0x10;
			break;
	 }
       SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CTRL,c); // enable I2S input

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUD_CTRL,&c); // select I2S clock as audio reference clock-2011.9.9-ANX.Fei
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUD_CTRL,c|0x06); 

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_FMT,&c); // configure I2S format
       c =  (c&~0xe5)|bAudioFormat->bI2S_FORMAT.EXT_VUCP <<2 |bAudioFormat->bI2S_FORMAT.AUDIO_LAYOUT 
	   	|bAudioFormat->bI2S_FORMAT.Channel_Num << 5;
       SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_FMT,c); // configure I2S format

	c = bAudioFormat->bI2S_FORMAT.Channel_status1;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CH_Status1, c ); // configure I2S channel status1
	c = bAudioFormat->bI2S_FORMAT.Channel_status2;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CH_Status2, c ); // configure I2S channel status2
	c = bAudioFormat->bI2S_FORMAT.Channel_status3;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CH_Status3, c ); // configure I2S channel status3
	c = bAudioFormat->bI2S_FORMAT.Channel_status4;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CH_Status4, c ); // configure I2S channel status4
	c = bAudioFormat->bI2S_FORMAT.Channel_status5;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CH_Status5, c ); // configure I2S channel status5

}


void SP_TX_Enable_Audio_Output(BYTE bEnable)
{
	BYTE c;

	SP_TX_Read_Reg (SP_TX_PORT0_ADDR, SP_TX_AUD_CTRL, &c);
	if(bEnable)
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUD_CTRL, ( c| SP_TX_AUD_CTRL_AUD_EN ) ); // enable SP audio
		
		// write audio info-frame
		SP_TX_AudioInfoFrameSetup();
		SP_TX_InfoFrameUpdate(&SP_TX_AudioInfoFrmae);
	}
	else
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUD_CTRL, (c &(~SP_TX_AUD_CTRL_AUD_EN))); // Disable SP audio

		
		SP_TX_Read_Reg (SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, ( c&(~SP_TX_PKT_AUD_EN )) ); // Disable the audio info-frame
	}
    

}

void SP_TX_Disable_Audio_Input(void)
{
	BYTE c;

    	SP_TX_Read_Reg (SP_TX_PORT2_ADDR, SPDIF_AUDIO_CTRL0, &c);
       SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SPDIF_AUDIO_CTRL0, (c &(~SPDIF_AUDIO_CTRL0_SPDIF_IN))); // Disable SPDIF

       // disable  I2S input
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_I2S_CTRL, 0x00 ); 	

}


void SP_TX_AudioInfoFrameSetup(void)
{
	SP_TX_AudioInfoFrmae.type = 0x84;
	SP_TX_AudioInfoFrmae.version = 0x01;
	SP_TX_AudioInfoFrmae.length = 0x0A;

	SP_TX_AudioInfoFrmae.pb_byte[0]=0x00;//coding type ,refer to stream header, audio channel count,two channel
	SP_TX_AudioInfoFrmae.pb_byte[1]=0x00;//refer to stream header
	SP_TX_AudioInfoFrmae.pb_byte[2]=0x00;
	SP_TX_AudioInfoFrmae.pb_byte[3]=0x00;//for multi channel LPCM
	SP_TX_AudioInfoFrmae.pb_byte[4]=0x00;//for multi channel LPCM
	SP_TX_AudioInfoFrmae.pb_byte[5]=0x00;//reserved to 0
	SP_TX_AudioInfoFrmae.pb_byte[6]=0x00;//reserved to 0
	SP_TX_AudioInfoFrmae.pb_byte[7]=0x00;//reserved to 0
	SP_TX_AudioInfoFrmae.pb_byte[8]=0x00;//reserved to 0
	SP_TX_AudioInfoFrmae.pb_byte[9]=0x00;//reserved to 0
}

void SP_TX_InfoFrameUpdate(struct AudiInfoframe* pAudioInfoFrame)
{
	BYTE c;

	c = pAudioInfoFrame->type;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_TYPE, c); // Audio infoframe

	
	c = pAudioInfoFrame->version;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_VER,	c);

	c = pAudioInfoFrame->length;
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_LEN,	c);

	c = pAudioInfoFrame->pb_byte[0];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB1,c);
	
	c = pAudioInfoFrame->pb_byte[1];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB2,c);

	c = pAudioInfoFrame->pb_byte[2];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB3,c);

	c = pAudioInfoFrame->pb_byte[3];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB4,c);

	c = pAudioInfoFrame->pb_byte[4];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB5,c);

	c = pAudioInfoFrame->pb_byte[5];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB6,c);

	c = pAudioInfoFrame->pb_byte[6];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB7,c);

	c = pAudioInfoFrame->pb_byte[7];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB8,c);

	c = pAudioInfoFrame->pb_byte[8];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB9,c);

	c = pAudioInfoFrame->pb_byte[9];
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AUD_DB10,c);
	

	SP_TX_Read_Reg (SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, ( c | SP_TX_PKT_AUD_UP ) ); // update the audio info-frame

		
	SP_TX_Read_Reg (SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, ( c | SP_TX_PKT_AUD_EN ) ); // enable the audio info-frame
}
	

void SP_TX_Config_Audio(struct AudioFormat *bAudio) 
{
	BYTE c;
	D("############## Config audio #####################");

       SP_TX_Power_On(SP_TX_PWR_AUDIO);
       
       switch(bAudio->bAudioType)
       {
       case AUDIO_I2S:
	   	SP_TX_Config_Audio_I2S(bAudio);
		break;
	case AUDIO_SPDIF:
	   	SP_TX_Config_Audio_SPDIF();
		break;
	default:
		D("ERR:Illegal audio format.\n");
		break;
       }

	SP_TX_Enable_Audio_Output(1);//enable audio
	
       SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK1, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_MASK1, c|0x04);//Unmask audio clock change int
       SP_CTRL_Set_System_State(SP_TX_HDCP_AUTHENTICATION);

}



void SP_TX_RST_AUX(void)
{
	BYTE c,c1;

	//D("reset aux");

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_DEBUG_REG1, &c1);
	c = c1;
	c1&=0xdd;//clear HPD polling and Transmitter polling
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DEBUG_REG1, c1); //disable  polling  before reset AUX-ANX.Fei-2011.9.19 
	
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL2_REG, &c1);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL2_REG, c1|SP_TX_AUX_RST);
	mdelay(1);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL2_REG, c1& (~SP_TX_AUX_RST));

	//set original polling enable
	//SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_DEBUG_REG1, &c1);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DEBUG_REG1, c); //enable  polling  after reset AUX-ANX.Fei-2011.9.19 
}


BYTE SP_TX_AUX_DPCDRead_Bytes(BYTE addrh, BYTE addrm, BYTE addrl,BYTE cCount,pByte pBuf)
{
	BYTE c,i;
	BYTE bOK;
	//BYTE c1;

/*
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_STATUS, &c);
    	if(c!=0)
	{
		D("AUX status is wrong, the error is 0x%.2x\n",(WORD)c);
		return 1;
	}
	
	if (cCount > MAX_BUF_CNT) {
		D("Wrong Aux CH access length!\n");
		return 1;
	}*/

	//clr buffer
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, 0x80);

	//set read cmd and count
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, (((cCount-1) <<4)|0x09));


	//set aux address15:0
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_7_0_REG, addrl);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_15_8_REG, addrm);

	//set address19:16 and enable aux
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, ((c & 0xf0) | addrh));


	//Enable Aux
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, (c | 0x01));


	mdelay(2);

	bOK = SP_TX_Wait_AUX_Finished();

	if(!bOK)
    	{
#ifdef AUX_DBG
        	D("aux read failed");
#endif
		if(SP_TX_HDCP_AUTHENTICATION != System_State_Get())
		SP_TX_RST_AUX();
        	return AUX_ERR;
    	}

		
	//SP_TX_Wait_AUX_Finished();
/*
    	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, &c1);
	if ( c1 > MAX_BUF_CNT ) {
		D("Wrong Aux CH length read back!\n");
		return 1;
	}*/

	for(i =0;i<cCount;i++)
	{
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG+i, &c);

		*(pBuf+i) = c;

		if(i >= MAX_BUF_CNT)
			break;
	}

	return AUX_OK;
	
}


BYTE SP_TX_AUX_DPCDWrite_Bytes(BYTE addrh, BYTE addrm, BYTE addrl,BYTE cCount,pByte pBuf)
{
	BYTE c,i;
	BYTE bOK;
	
/*
    	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_STATUS, &c);
    	if(c!=0)
	{
		//D("AUX status is wrong, the error is 0x%.2x\n",(WORD)c);
		return 1;
	}
	
	if (cCount > MAX_BUF_CNT) {
		D("Wrong Aux CH access length!\n");
		return 1;
	}*/

	//clr buffer
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, 0x80);

	//set write cmd and count;
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, (((cCount-1) <<4)|0x08));

	//set aux address15:0
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_7_0_REG, addrl);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_15_8_REG, addrm);

	//set address19:16
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, ((c & 0xf0) | addrh));


	//write data to buffer
	for(i =0;i<cCount;i++)
	{
		c = *pBuf;
		pBuf++;
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG+i, c);

		if(i >= MAX_BUF_CNT)
			break;
	}

	//Enable Aux
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, (c | 0x01));

	bOK = SP_TX_Wait_AUX_Finished();

	if(bOK)
		return AUX_OK;
	else
	{
	#ifdef AUX_DBG
		D("aux write failed");
	#endif
		//SP_TX_RST_AUX();
		return AUX_ERR;
	}

}
/*

BYTE SP_TX_AUX_DPCDRead_Byte(BYTE addrh, BYTE addrm, BYTE addrl)
{
	BYTE c;
	//BYTE c1;

	//clr buffer
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, 0x80);

	//set read cmd and count
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, (0 <<4)|0x09);


	//set aux address15:0
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_7_0_REG, addrl);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_15_8_REG, addrm);

	//set address19:16 and enable aux
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, c & 0xf0 | addrh);


	//Enable Aux
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, c | 0x01);


	////mdelay(2);
	SP_TX_Wait_AUX_Finished();

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG, &c);



	return c;
	
}
*/

void SP_TX_AUX_DPCDWrite_Byte(BYTE addrh, BYTE addrm, BYTE addrl, BYTE data1)
{

    BYTE c;
/*
    	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_STATUS, &c);
    	if(c!=0)
	{
		//D("AUX status is wrong, the error is 0x%.2x\n",(WORD)c);
		return;
	}*/

    //clr buffer
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, 0x80);

    //set write cmd and count;
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, ((0 <<4)|0x08));

    //set aux address15:0
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_7_0_REG, addrl);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_15_8_REG, addrm);

    //set address19:16
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, &c);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, ((c & 0xf0) | addrh));


    //write data to buffer
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG, data1);

    //Enable Aux
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, (c | 0x01));

    SP_TX_Wait_AUX_Finished();
	
    return ;
 }



BYTE SP_TX_Wait_AUX_Finished(void)
{
	BYTE c;
	BYTE cCnt;
	cCnt = 0;
	
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_STATUS, &c);
	while(c & 0x10)
	{
		cCnt++;
               
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_STATUS, &c);

		if(cCnt>100)
                 {
 #ifdef AUX_DBG
                  D("AUX Operaton does not finished, and tome out.");
#endif
                   break; 
                 }
			
	}

    if(c&0x0F)
    {
  #ifdef AUX_DBG
        D("aux operation failed %.2x\n",(WORD)c);
  #endif
        return 0;
    }
    else
        return 1; //succeed

	//SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_STATUS, &c);
	//if(c&0x0f !=0)
	//D("**AUX Access error code = %.2x***\n",(WORD)c);
	
}


/*
void SP_TX_SW_Reset(void)
{
	BYTE c;

	//software reset    
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, c | SP_TX_RST_SW_RST);
	//mdelay(10);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL_REG, c & ~SP_TX_RST_SW_RST);
}

*/


void SP_TX_SPREAD_Enable(BYTE bEnable)
{
	BYTE c;

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SSC_CTRL_REG1, &c);
	
	if(bEnable)
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SSC_CTRL_REG1, c | SPREAD_AMP);// enable SSC
		//SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, (c &(~0x04)));// powerdown SSC

		//reset SSC
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL2_REG, &c);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL2_REG, c | SP_TX_RST_SSC);
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_RST_CTRL2_REG, c & (~SP_TX_RST_SSC));
                    
		//enable the DPCD SSC
		SP_TX_AUX_DPCDRead_Bytes(0x00, 0x01, DPCD_DOWNSPREAD_CTRL,1,&c);
              SP_TX_AUX_DPCDWrite_Byte(0x00, 0x01, DPCD_DOWNSPREAD_CTRL, (c | 0x10));

	}
	else
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SSC_CTRL_REG1, (c & (~SPREAD_AMP)));// disable SSC
              //SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_DOWN_SPREADING_CTRL2, c|0x04);// powerdown SSC
		//disable the DPCD SSC
		SP_TX_AUX_DPCDRead_Bytes(0x00, 0x01, DPCD_DOWNSPREAD_CTRL,1,&c);
		SP_TX_AUX_DPCDWrite_Byte(0x00, 0x01, DPCD_DOWNSPREAD_CTRL, (c & 0xef));
	}
		
}



void SP_TX_Get_Int_status(INTStatus IntIndex, BYTE *cStatus)
{
	BYTE c;

	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_STATUS1 + IntIndex, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_COMMON_INT_STATUS1 + IntIndex, c);

	*cStatus = c;
}

/*
void SP_TX_Get_HPD_status( BYTE *cStatus)
{
	BYTE c;
	
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, &c);

	*cStatus = c;
}
*/

BYTE SP_TX_Get_PLL_Lock_Status(void)
{
	BYTE c;
	
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_DEBUG_REG1, &c);
	if(c & SP_TX_DEBUG_PLL_LOCK)
	{
		return 1;
	}
	else
		return 0;

}


void SP_TX_Get_HDCP_status( BYTE *cStatus)
{
	BYTE c;
	
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_STATUS, &c);

	*cStatus = c;
}


void SP_TX_HDCP_ReAuth(void)
{	
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, 0x23);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_HDCP_CONTROL_0_REG, 0x03);
}


void SP_TX_Lane_Set(BYTE lane, BYTE bVal)
{	

    BYTE c,bSwing,bw_current;

    bSwing = ((bVal&0x18)>>1)|(bVal&0x03);
    SP_TX_Get_Link_BW(&bw_current);
    if(bw_current==BW_54G)
    {

        if(lane == 0)
        {
            switch(bSwing)
            {
                case 0x00:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, ((c&~0x08)|0x30));//increase the terminal, and normal swing
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x30);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x1f);
                    D("lane0,Swing400mv,6db.");
                    break;
                case 0x01: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c|0x38));//increase the terminal, and set 30% larger swing
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x27);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x1a);
                    D("lane0,Swing600mv,6db. does not support.");
                    break;
                case 0x02:
                    D("lane0,Swing800mv,6db. does not support.");
                    break;
                case 0x03:
                    D("Does not support");
                    break;
                case 0x04:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, ((c&~0x08)|0x30));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x28);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x10);
                    D("lane0,Swing400mv,3.5db.");
                    break;
                case 0x05: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, ((c&~0x08)|0x30));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x3c);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x18);
                    D("lane0,Swing600mv,3.5db.");
                    break;
                case 0x06: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c|0x38));
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x2c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x11);
                    D("lane0,Swing800mv,3.5db.does not support");
                    break;
                case 0x07:
                    D("Does not support");
                    break;
                case 0x08: 
                   //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, ((c&~0x08)|0x30));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x20);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x00);
                    D("lane0,Swing400mv,0db.");
                    break;
                case 0x09: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, ((c&~0x08)|0x30));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x30);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x00);
                    D("lane0,Swing600mv,0db.");
                    break;
                case 0x0a:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, ((c&~0x08)|0x30));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x3f);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x00);
                    D("Swing800mv,0db.");
                    break;
                case 0x0b:
                    D("lane0,Does not support");
                    break;
                default:
                    break;

            }
       }

    }
    else
    {

       if(lane == 0)
        {
                switch(bSwing)
            {
                case 0x00:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c&~0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x10);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x00);
                    D("lane0,Swing200mv,0db.");
                    break;
                case 0x01: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c&~0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x20);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x00);
                    D("lane0,Swing400mv,0db.");
                    break;
                case 0x02:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c|0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x30);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x00);
                    D("lane0,Swing600mv,0db.");
                    break;
                case 0x03:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                   //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c|0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x3f);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x00);
                    D("lane0,Swing800mv,0db.");
                    break;
                case 0x04: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c&~0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x14);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x08);
                    D("lane0,Swing200mv,3.5db.");
                    break;
                case 0x05: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c&~0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x28);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x10);
                    D("lane0,Swing400mv,3.5db.");
                    break;
                case 0x06: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c&~0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x3c);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x18);
                    D("lane0,Swing600mv,3.5db.");
                    break;
                case 0x07: 
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c|0x08));
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x2c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x11);
                    D("lane0,Swing800mv,3.5db.does not support");
                    break;
                case 0x08:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c&~0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x18);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x10);
                    D("lane0,Swing200mv,6db.");
                    break;
                case 0x09:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c|0x08));
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x30);
                    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x1f);
                    D("lane0,Swing400mv,6db.");
                    break;
                case 0x0a:
                    //SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0xdc, &c);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0xdc, (c|0x08));
                   // SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, 0x27);
                    //SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, 0x1a);
                    D("lane0,Swing600mv,6db.does not support");
                    break;
                case 0x0b:
                    D("lane0,Swing800mv,6db. Exceed the capability.");
                    break;
                default:
                   break;
                }
        }

      
    }
    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, &c);
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL2, c|0x80);// force CH0 AMP use register setting

    SP_TX_Read_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, &c);
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, PLL_FILTER_CTRL3, c|0x80);// force CH0 EMP use register setting

    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_LANE0_SET_REG + lane, bVal);



}

void SP_TX_Lane_Get(BYTE lane, BYTE *bVal)
{
	BYTE c;

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_LANE0_SET_REG + lane, &c);

	*bVal = c;
}


void SP_TX_Lanes_PWR_Ctrl(ANALOG_PWD_BLOCK eBlock, BYTE powerdown)
{
	BYTE c;

	switch(eBlock)
	{
			
		case CH0_BLOCK:
			if(powerdown)
			{
				SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, &c);
				c|=SP_TX_ANALOG_POWER_DOWN_CH0_PD;
				SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, c);				
			}
			else
			{
				SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, &c);
				c&=~SP_TX_ANALOG_POWER_DOWN_CH0_PD;
				SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, c);
			}

			break;
			
		case CH1_BLOCK:
			if(powerdown)
			{
				SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, &c);
				c|=SP_TX_ANALOG_POWER_DOWN_CH1_PD;
				SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, c);				
			}
			else
			{
				SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, &c);
				c&=~SP_TX_ANALOG_POWER_DOWN_CH1_PD;
				SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_ANALOG_POWER_DOWN_REG, c);
			}

			break;
			
		default:
			break;
	}
}


void SP_TX_Get_Rx_LaneCount(BYTE bMax,BYTE *cLaneCnt)
{
	if(bMax)
	    SP_TX_AUX_DPCDRead_Bytes(0x00, 0x00,DPCD_MAX_LANE_COUNT,1,cLaneCnt);
	else
	    SP_TX_AUX_DPCDRead_Bytes(0x00, 0x01,DPCD_LANE_COUNT_SET,1,cLaneCnt);

		

}


/*
void SP_TX_Set_Rx_laneCount(BYTE cLaneCnt)
{
	SP_TX_AUX_DPCDWrite_Byte(0x00, 0x00, DPCD_LANE_COUNT_SET, cLaneCnt);
}
*/

void SP_TX_Get_Rx_BW(BYTE bMax,BYTE *cBw)
{
	if(bMax)
	   SP_TX_AUX_DPCDRead_Bytes(0x00, 0x00,DPCD_MAX_LINK_RATE,1,cBw);
	else
	    SP_TX_AUX_DPCDRead_Bytes(0x00, 0x01,DPCD_LINK_BW_SET,1,cBw);
 
}

/*
void SP_TX_Set_Rx_BW(BYTE cBw)
{
	SP_TX_AUX_DPCDWrite_Byte(0x00, 0x01, DPCD_LINK_BW_SET, cBw);
}
*/
void SP_TX_Set_Link_BW(SP_LINK_BW bwtype)
{
	//set bandwidth
        SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_BW_SET_REG, bwtype);

}

void SP_TX_Set_Lane_Count(BYTE count)
{
	//set lane conut
    	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LANE_COUNT_SET_REG, count);
	
}

void SP_TX_Get_Link_BW(BYTE *bwtype)
{
	BYTE c;

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_BW_SET_REG, &c);

	*bwtype = c;
}

/*
void SP_TX_Get_Lane_Count(BYTE *count)
{
	BYTE c;

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_LANE_COUNT_SET_REG, &c);

	*count = c;

}
*/

void SP_TX_EDID_Read_Initial(void)
{
	BYTE c;

	//Set I2C address	
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_7_0_REG, 0x50);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_15_8_REG, 0);
    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, &c);
    SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_19_16_REG, c & 0xf0);
}


BYTE SP_TX_AUX_EDIDRead_Byte(BYTE offset)
{
    BYTE c,i,edid[16],data_cnt,cnt,vsdbdata[4],VSDBaddr;
    BYTE bReturn=0;
    //D("***************************offset = %.2x\n", (unsigned int)offset);

    cnt = 0;
	   
    SP_TX_AUX_WR(offset);//offset 
    
    if((offset == 0x00) || (offset == 0x80))
    	checksum = 0;
	
	SP_TX_AUX_RD(0xf5);//set I2C read com 0x05 mot = 1 and read 16 bytes
	
	data_cnt = 0;
	while(data_cnt < 16)
	{
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, &c);
		c = c & 0x1f;
		//D("cnt_d = %.2x\n",(WORD)c);
		if(c != 0)
		{
		    for( i = 0; i < c; i ++)
		    {
		        SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG + i, &edid[i + data_cnt]);
			 //D("edid[%.2x] = %.2x\n",(WORD)(i + offset),(WORD)edid[i + data_cnt]);
		      	checksum = checksum + edid[i + data_cnt];
		    }
		}
		else
		{
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, 0x01);
			//enable aux
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x03);//set address only
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
			while(c & 0x01)
			{
        		mdelay(2);
        		cnt ++;
        		if(cnt == 10)
        		{
        			//D("read break");
					SP_TX_RST_AUX();
					bEDIDBreak=1;
					bReturn = 0x01;
        		}
        		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
            }
			//D("cnt_d = 0, break");
			sp_tx_edid_err_code = 0xff;
			bReturn = 0x02;// for fixing bug leading to dead lock in loop "while(data_cnt < 16)"
			return bReturn;
		}
		data_cnt = data_cnt + c;
		if(data_cnt < 16)// 080610. solution for handle case ACK + M byte
		{		
			//SP_TX_AUX_WR(offset);
			SP_TX_RST_AUX();
			mdelay(10);
			
			c = 0x05 | ((0x0f - data_cnt) << 4);//Read MOT = 1
			SP_TX_AUX_RD(c);
			//D("M < 16");
		}
	}

	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, 0x01);
	//enable aux
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x03);//set address only to stop EDID reading
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	while(c & 0x01)
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);

	//D("***************************offset %.2x reading completed\n", (unsigned int)offset);

	if(EDID_Print_Enable)
	{
		for(i=0;i<16;i++) 
		{
			if((i&0x0f)==0)
				D("\n edid: [%.2x]  %.2x  ", (unsigned int)offset, (unsigned int)edid[i]);
			else 
				D("%.2x  ", (unsigned int)edid[i]);

			if((i&0x0f)==0x0f)
				D("\n");
		}

	}




    if(offset == 0x00)
    {
        if((edid[0] == 0) && (edid[7] == 0) && (edid[1] == 0xff) && (edid[2] == 0xff) && (edid[3] == 0xff)
            && (edid[4] == 0xff) && (edid[5] == 0xff) && (edid[6] == 0xff))
            D("Good EDID header!");
        else
        {
            D("Bad EDID header!");
            sp_tx_edid_err_code = 0x01;
        }
            
    }

    else if(offset == 0x30)
    {
        for(i = 0; i < 10; i ++ )
            SP_TX_EDID_PREFERRED[i] = edid[i + 6];//edid[0x36]~edid[0x3f]
    }

    else if(offset == 0x40)
    {
        for(i = 0; i < 8; i ++ )
            SP_TX_EDID_PREFERRED[10 + i] = edid[i];//edid[0x40]~edid[0x47]
    }

    else if(offset == 0x70)
    {
		checksum = checksum&0xff;
		checksum = checksum - edid[15];
		checksum = ~checksum + 1;
        if(checksum != edid[15])
        {
            D("Bad EDID check sum1!");
            sp_tx_edid_err_code = 0x02;
            checksum = edid[15];
        }
		else
			D("Good EDID check sum1!");
    }
/*
    else if(offset == 0xf0)
    {
        checksum = checksum - edid[15];
        checksum = ~checksum + 1;
        if(checksum != edid[15])
        {
            D("Bad EDID check sum2!");
            sp_tx_edid_err_code = 0x02;
        }
	 else
	 	D("Good EDID check sum2!");
    }*/
    else if( (offset >= 0x80)&&(sp_tx_ds_edid_hdmi==0))
    {
		if(offset ==0x80)
		{
			if(edid[0] !=0x02)
			return 0x03;
		}
		for(i=0;i<16;i++)//record all 128 data in extsion block.
		    EDIDExtBlock[offset-0x80+i]=edid[i];
/*
		for(i=0;i<16;i++) 
		{
			if((i&0x0f)==0)
				D("\n edid: [%.2x]  %.2x  ", (unsigned int)offset, (unsigned int)EDIDExtBlock[offset-0x80+i]);
			else 
				D("%.2x  ", (unsigned int)edid[i]);

			if((i&0x0f)==0x0f)
				D("\n");
		}*/
        
        if(offset ==0x80)
			DTDbeginAddr = edid[2];

		if(offset == 0xf0)
		{
			checksum = checksum - edid[15];
			checksum = ~checksum + 1;
			if(checksum != edid[15])
			{
				D("Bad EDID check sum2!");
				sp_tx_edid_err_code = 0x02;
			}
			else
				D("Good EDID check sum2!");
		 
	        for(VSDBaddr = 0x04;VSDBaddr <DTDbeginAddr;)
	        {
	            //D("####VSDBaddr = %.2x\n",(WORD)(VSDBaddr+0x80));
	
	            vsdbdata[0] = EDIDExtBlock[VSDBaddr];
	            vsdbdata[1] = EDIDExtBlock[VSDBaddr+1];
	            vsdbdata[2] = EDIDExtBlock[VSDBaddr+2];
	            vsdbdata[3] = EDIDExtBlock[VSDBaddr+3];
	
	            //D("vsdbdata= %.2x,%.2x,%.2x,%.2x\n",(WORD)vsdbdata[0],(WORD)vsdbdata[1],(WORD)vsdbdata[2],(WORD)vsdbdata[3]);
	            if((vsdbdata[0]&0xe0)==0x60)
	            {
	                if((vsdbdata[1]==0x03)&&(vsdbdata[2]==0x0c)&&(vsdbdata[3]==0x00))
	                {
	                    sp_tx_ds_edid_hdmi = 1;
	                    return 0;
	                }
	                else
	                {
	                    sp_tx_ds_edid_hdmi = 0;
	                    return 0x03;
	                }
	
	            }	
	            else
	            {
	                sp_tx_ds_edid_hdmi = 0;
	                VSDBaddr = VSDBaddr+(vsdbdata[0]&0x1f);
	                VSDBaddr = VSDBaddr + 0x01;
	            }
	    
	            if(VSDBaddr > DTDbeginAddr)
	                return 0x03;
	
	        }
        }
          
    }      

	return bReturn;
}

void SP_TX_Parse_Segments_EDID(BYTE segment, BYTE offset)
{
	BYTE c,cnt;
	int i;


	//set I2C write com 0x04 mot = 1
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, 0x04);

	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_7_0_REG, 0x30);	

	// adress_only
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x03);//set address only

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	
	//while(c & 0x01)
	//	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	SP_TX_Wait_AUX_Finished();
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, &c);

	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG, segment);

	//set I2C write com 0x04 mot = 1
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, 0x04);
	//enable aux
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x01);
	cnt = 0;
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	while(c&0x01)
	{
		mdelay(10);
		cnt ++;
		//D("cntwr = %.2x\n",(WORD)cnt);
		if(cnt == 10)
		{
			D("write break");
			SP_TX_RST_AUX();
			cnt = 0;
			bEDIDBreak=1;
			break;
		}
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	}

	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_ADDR_7_0_REG, 0x50);//set EDID addr 0xa0	
	// adress_only
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x03);//set address only

	SP_TX_AUX_WR(offset);//offset   
	//adress_only
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x03);//set address only

	SP_TX_AUX_RD(0xf5);//set I2C read com 0x05 mot = 1 and read 16 bytes    
       cnt = 0;
	for(i=0; i<16; i++)
	{
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, &c);
		while((c & 0x1f) == 0)
		{
			mdelay(2);
			cnt ++;
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_COUNT_REG, &c);
			if(cnt == 10)
			{
				D("read break");
				SP_TX_RST_AUX();
				bEDIDBreak=1;
				return;
			}
		}

		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG+i, &c);
		//D("edid[0x%.2x] = 0x%.2x\n",(WORD)(offset+i),(WORD)c);
	} 
	///*
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG, 0x01);
	//enable aux
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, 0x03);//set address only to stop EDID reading
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	while(c & 0x01)
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);

}




BYTE SP_TX_Get_EDID_Block(void)
{
	BYTE c;
	SP_TX_AUX_WR(0x00);
	SP_TX_AUX_RD(0x01);
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG, &c);
	//D("[a0:00] = %.2x\n", (WORD)c);
	
	SP_TX_AUX_WR(0x7e);
	SP_TX_AUX_RD(0x01);
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_BUF_DATA_0_REG, &c);

	D("EDID Block = %d\n",(int)(c+1));
	return c;
}




void SP_TX_AddrOnly_Set(BYTE bSet)
{
	BYTE c;

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, &c);
	if(bSet)
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, (c|SP_TX_ADDR_ONLY_BIT));
	}
	else
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_AUX_CTRL_REG2, (c&~SP_TX_ADDR_ONLY_BIT));
	}	
}

/*
void SP_TX_Scramble_Enable(BYTE bEnabled)
{
	BYTE c;

	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, &c);
	if(bEnabled)//enable scramble
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, (c&~SP_TX_SCRAMBLE_DISABLE));
	}
	else
	{
		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, (c|SP_TX_SCRAMBLE_DISABLE));
	}

}
*/

void SP_TX_Training_Pattern_Set(PATTERN_SET PATTERN)
{

	switch(PATTERN)
	{
		case PRBS_7:
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, 0x0c);
			break;
		case D10_2:
			break;
		case TRAINING_PTN1:
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, 0x21);
			break;
		case TRAINING_PTN2:
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, 0x22);
			break;
		case NONE:
#ifdef DISBALBE_SCRAMBLE
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, 0x20);
#else
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_PTN_SET_REG, 0x00);
#endif
			break;
		default:
			break;
	}
	
}


void SP_TX_API_M_GEN_CLK_Select(BYTE bSpreading)
{
    BYTE c;

    SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_M_CALCU_CTRL, &c);    
    if(bSpreading)
    {
            //M value select, select clock with downspreading
            SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_M_CALCU_CTRL, (c | M_GEN_CLK_SEL));    		      
    }
    else
    {
            //M value select, initialed as clock without downspreading
            SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_M_CALCU_CTRL, c&(~M_GEN_CLK_SEL));
    }
}


void SP_TX_Config_Packets(PACKETS_TYPE bType)
{
	BYTE c;

	switch(bType)
	{
		case AVI_PACKETS:
			//clear packet enable
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c&(~SP_TX_PKT_AVI_EN));

			//get input color space
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_VID_CTRL, &c);

			SP_TX_Packet_AVI.AVI_data[0] = SP_TX_Packet_AVI.AVI_data[0] & 0x9f;
			SP_TX_Packet_AVI.AVI_data[0] = SP_TX_Packet_AVI.AVI_data[0] | c <<4;

                    //D("AVI 0 =%x\n", (WORD)SP_TX_Packet_AVI.AVI_data[0]);

			
			SP_TX_Load_Packet(AVI_PACKETS);

			//send packet update
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c | SP_TX_PKT_AVI_UD);

			//enable packet
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c | SP_TX_PKT_AVI_EN);			
			
			break;
			
		case SPD_PACKETS:
			//clear packet enable
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c&(~SP_TX_PKT_SPD_EN));

			SP_TX_Load_Packet(SPD_PACKETS);

			//send packet update
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c | SP_TX_PKT_SPD_UD);

			//enable packet
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c | SP_TX_PKT_SPD_EN);	

			break;

		case MPEG_PACKETS:
			//clear packet enable
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c&(~SP_TX_PKT_MPEG_EN));

			SP_TX_Load_Packet(MPEG_PACKETS);

			//send packet update
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c | SP_TX_PKT_MPEG_UD);

			//enable packet
			SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, &c);
			SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_PKT_EN_REG, c | SP_TX_PKT_MPEG_EN);	

			break;

		default:
			break;
	}
	
}

void SP_TX_Load_Packet (PACKETS_TYPE type)
{
    BYTE i;
    
    switch(type)
    {
        case AVI_PACKETS:
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AVI_TYPE , 0x82);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AVI_VER , 0x02);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AVI_LEN , 0x0d);
			
            for(i=0;i<13;i++)
            {
                SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_AVI_DB1 + i, SP_TX_Packet_AVI.AVI_data[i]);                
            }
            break;

        case SPD_PACKETS:
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_SPD_TYPE , 0x83);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_SPD_VER , 0x01);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_SPD_LEN , 0x19);
            for(i=0;i<25;i++)
            {
                SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_SPD_DATA1 + i, SP_TX_Packet_SPD.SPD_data[i]);
            }
            break;

        case MPEG_PACKETS:
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_MPEG_TYPE , 0x85);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_MPEG_VER , 0x01);
            SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_MPEG_LEN , 0x0a);
            for(i=0;i<10;i++)
            {
                SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_MPEG_DATA1 + i, SP_TX_Packet_MPEG.MPEG_data[i]);
            }
            break;
			
	default:
	    break;
    }
}
void SP_TX_AVI_Setup(void)
{
	SP_TX_Packet_AVI.AVI_data[0]=0x10;// Active video, color space RGB
	SP_TX_Packet_AVI.AVI_data[1]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[2]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[3]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[4]=0x00;//repeat 0
	SP_TX_Packet_AVI.AVI_data[5]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[6]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[7]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[8]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[9]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[10]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[11]=0x00;//reserved to 0
	SP_TX_Packet_AVI.AVI_data[12]=0x00;//reserved to 0

}

BYTE SP_TX_BW_LC_Sel(struct VideoFormat* pInputFormat)
{
    BYTE over_bw;
    int pixel_clk;

    over_bw = 0;
    pixel_clk = pclk;

    if(pInputFormat->bColordepth != COLOR_8_BIT)
	return 1;
/*
    if(BIST_EN)
    {
        if((sp_tx_lane_count != 0x01)||(pInputFormat->bColordepth == COLOR_12_BIT))
        {
            //if(bForceSelIndex != 4)
                pixel_clk = 2 * pixel_clk;
        }
    }*/

    D("pclk = %d\n",(WORD)pixel_clk);
	SP_TX_AUX_DPCDRead_Bytes(0x00,0x00,0x01,2,ByteBuf);
	sp_tx_bw = ByteBuf[0];
	//sp_tx_lane_count = ByteBuf[1] & 0x0f;

    
    if(pixel_clk <= 54)
    {
        if(LTResult.LT_1_162)
            {
                sp_tx_bw = BW_162G;
                sp_tx_lane_count = 0x01;
            }
         else 
           over_bw = 1; 

    }
    else if((54 < pixel_clk) && (pixel_clk <= 90))
    {
        if((sp_tx_bw >= BW_27G)&&(LTResult.LT_1_27))
        {
            sp_tx_bw = BW_27G;
            sp_tx_lane_count = 0x01;
        }
        else
        {
            if((sp_tx_lane_count == 0x02)&&(LTResult.LT_2_162))
            {
                sp_tx_bw = BW_162G;
                sp_tx_lane_count = 0x02;
            }
            else 
                over_bw = 1;
        }
    }
    else if((90 < pixel_clk) && (pixel_clk <= 108))
    {
         if((sp_tx_bw >= BW_54G)&&(LTResult.LT_1_54))
        {
            sp_tx_bw = BW_54G;
            sp_tx_lane_count = 0x01;
        }
        else 
        {
            if((sp_tx_lane_count == 0x02)&&(LTResult.LT_2_162))
            {
                sp_tx_bw = BW_162G;
                sp_tx_lane_count = 0x02;
            }
            else 
                over_bw = 1;
        }
    }
    else if((108 < pixel_clk) && (pixel_clk <= 180))
    {
         if((sp_tx_bw == BW_54G)&&(LTResult.LT_1_54))
        {
            sp_tx_bw = BW_54G;
            sp_tx_lane_count = 0x01;
        }
        else
        {
            if((sp_tx_lane_count == 0x02)&&(LTResult.LT_2_27))
            {
                sp_tx_bw = BW_27G;
                sp_tx_lane_count = 0x02;
            }
            else 
                over_bw = 1;
        }
    }
    else if((180 < pixel_clk) && (pixel_clk <= 360))
    {
        if((sp_tx_lane_count < 2)||(sp_tx_bw < BW_54G))
            over_bw = 1;
        else
        {
             if(LTResult.LT_2_54)
                {
                    sp_tx_bw = BW_54G;
                    sp_tx_lane_count = 0x02;
                }
             else
                over_bw = 1;
                
        }
    }
    else
    {
   
            over_bw = 1;
    }
	
    if(over_bw)
        D("over bw!\n");
     else
	D("The optimized BW =%.2x, Lane cnt=%.2x\n",(WORD)sp_tx_bw,(WORD)sp_tx_lane_count);

     return over_bw;

}

#ifndef SW_LINK_TRAINING
void SP_TX_HW_Link_Training (void)
{
    
    BYTE c;

   

    if(!sp_tx_hw_lt_enable)
    {

        D("Hardware link training");
        if(sp_tx_ssc_enable)
            SP_TX_CONFIG_SSC(sp_tx_bw);
        else
            SP_TX_SPREAD_Enable(0);

        //set bandwidth
        SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_BW_SET_REG, sp_tx_bw);
        //set lane conut
        SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LANE_COUNT_SET_REG, sp_tx_lane_count);

        SP_TX_EnhaceMode_Set();

        ByteBuf[0] = 0x01;
		SP_TX_AUX_DPCDWrite_Bytes(0x00,0x06,0x00, 0x01, ByteBuf); //Set sink to D0 mode

		SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_LANE0_SET_REG, 0x09);//link training from 400mv3.5db for ANX7730 B0-ANX.Fei-20111011

        SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_TRAINING_CTRL_REG, SP_TX_LINK_TRAINING_CTRL_EN);

        sp_tx_hw_lt_enable = 1;
        return;

    }

	mdelay(10);
  
	if(!sp_tx_hw_lt_done)
	{
		SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_TRAINING_CTRL_REG, &c);
		if(c&0x01)
		{
			return; //link training is not finished.

		}else{

			if(c&0x80)
			{
				c = (c & 0x70) >> 4;
				D("HW LT failed in process, ERR code = %.2x\n",(WORD)c);

			sp_tx_link_config_done = 0;
			sp_tx_hw_lt_enable = 1;
			SP_CTRL_Set_System_State(SP_TX_LINK_TRAINING);
			return;

		}else{

				sp_tx_hw_lt_done = 1;
				SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_TRAINING_LANE0_SET_REG, &c);
				D("HW LT succeed in process,LANE0_SET = %.2x\n",(WORD)c);
			}
		}
	}


    if(sp_tx_hw_lt_done)
    {
		SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x02, 1, ByteBuf);
		if(ByteBuf[0] != 0x07)
		{
			sp_tx_hw_lt_enable = 0;
			sp_tx_hw_lt_done = 0;
			return;
		}else{
		
			sp_tx_hw_lt_done = 0; 
			SP_TX_MIPI_CONFIG_Flag_Set(0);
			SP_CTRL_Set_System_State(SP_TX_CONFIG_VIDEO);
		}
		
    }


}

#else

void SP_TX_Link_Training (void)
{
	BYTE lane_count,i,j=0;
	BYTE linkbw[3]={0x14,0x0a,0x06};
	

	lane_count = sp_tx_lane_count;

	while(lane_count>0)
	{
		for(i=0;i<3;i++)
		{
			if(linkbw[i]==sp_tx_bw)
			{
                        j=i;break;
			}
		}
		for(;j<3;j++)
		{
			SP_TX_SW_LINK_Process(lane_count,linkbw[j]);
		}
		
		lane_count--;
	}
	SP_CTRL_Set_System_State(SP_TX_CONFIG_VIDEO);


}

#endif
BYTE SP_TX_LT_Pre_Config(void)
{
    BYTE legel_bw,legel_lc,c;  

	legel_bw = legel_lc = 1;


	if(!SP_TX_Get_PLL_Lock_Status())
	{
		D("PLL not lock!");
		return 1;
	}


        if(!sp_tx_link_config_done)
        {    	 
            if(sp_tx_test_link_training)
            {
                sp_tx_test_link_training = 0;
                sp_tx_bw = sp_tx_test_bw;
                sp_tx_lane_count = sp_tx_test_lane_count;
            }
            else
            {
                SP_TX_Get_Rx_LaneCount(1,&c);
                sp_tx_lane_count = c;
                SP_TX_Get_Rx_BW(1,&c);
                sp_tx_bw = c;
                sp_tx_lane_count = sp_tx_lane_count & 0x0f;
            }

           // D("sp_rx_dpcd_bw = %.2x\n",(WORD)sp_tx_bw);
           // D("sp_rx_dpcd_lane_count = %.2x\n",(WORD)sp_tx_lane_count);

            if((sp_tx_bw != BW_27G) && (sp_tx_bw != BW_162G)&& (sp_tx_bw != BW_54G))
                legel_bw = 0;
            if((sp_tx_lane_count != 0x01) && (sp_tx_lane_count != 0x02) && (sp_tx_lane_count != 0x04))
                legel_lc = 0;
            if((legel_bw == 0) || (legel_lc == 0))
                return 1;

           // if(sp_tx_lane_count>2)//ANX7805 supports 2 lanes max.
           sp_tx_lane_count = 1;


            //if(Force_BW27)//force ANX7805 work in 2.7G mode.
	     //sp_tx_bw = 0x0a;
           

            //SP_TX_EnhaceMode_Set();

            if(sp_tx_lane_count ==1)
                SP_TX_Lanes_PWR_Ctrl(CH1_BLOCK, 1);
            else 
                SP_TX_Lanes_PWR_Ctrl(CH1_BLOCK, 0);


            sp_tx_link_config_done = 1;
        }

        SP_TX_Enable_Audio_Output(0);	//Disable audio  output
        SP_TX_Disable_Audio_Input();  //Disable audio input
        SP_TX_Disable_Video_Input();//Disable video input 

        SP_TX_HDCP_Encryption_Disable();

    return 0;// pre-config done
}




void SP_TX_SW_LINK_Process(BYTE lanecnt, SP_LINK_BW linkbw)
{
    BYTE bLinkFinished;

    eSW_Link_state = LINKTRAINING_START;
    bLinkFinished = 0;


	while(!bLinkFinished)
	{
		switch(eSW_Link_state)
		{
			case LINKTRAINING_START:
				SP_TX_Link_Start_Process(lanecnt,linkbw);
				break;
			case CLOCK_RECOVERY_PROCESS:
				SP_TX_CLOCK_Recovery_Process();
				break;
			case EQ_TRAINING_PROCESS:
				SP_TX_EQ_Process();
				break;
			case LINKTRAINING_FINISHED:
                            bLinkFinished = 1;
				SP_TX_LT_Finished_Process();
				break;
			default:
				break;
		}
	}
}


void SP_TX_Link_Start_Process(BYTE lanecnt, SP_LINK_BW linkbw)
{
	//BYTE c1;

       ByteBuf[0] = 0x01;
	SP_TX_AUX_DPCDWrite_Bytes(0x00,0x06,0x00, 0x01, ByteBuf); //Set sink to D0 mode

	if(sp_tx_ssc_enable)
	    SP_TX_CONFIG_SSC(linkbw);
      else
           SP_TX_SPREAD_Enable(0);
        
        
       SP_TX_Set_Link_BW(linkbw);//set bandwidth
       SP_TX_Set_Lane_Count(lanecnt);  //set lane conut

	ByteBuf[0] = linkbw;
	ByteBuf[1] = lanecnt;

	SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x00,2,ByteBuf);
    
       SP_TX_EnhaceMode_Set();

       SP_TX_Lane_Set(0,0);

	//Start CR training, set LT PT1,disable scramble
	SP_TX_Training_Pattern_Set(TRAINING_PTN1);

	ByteBuf[0] = 0x21;
	ByteBuf[1] = 0x00;
	ByteBuf[2] = 0x00;
	//ByteBuf[3] = 0x00;
	//ByteBuf[4] = 0x00;
	SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x02,3,ByteBuf);
	
	//initialize the CR loop counter
	CRLoop0 = 0;
	CRLoop1 = 0;
	bEQLoopcnt = 0;

	SP_TX_Set_Link_state(CLOCK_RECOVERY_PROCESS);
}

void SP_TX_CLOCK_Recovery_Process(void)
{
	BYTE bLane0_1_Status;
	BYTE c0,c1,c2;
	BYTE bAdjust_Req_0_1;
    BYTE s1;

       BYTE cCRloopCount;


	mdelay(16);//TRAINING_AUX_RD_INTERVAL  max is 16ms.

    SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x02, 6, ByteBuf);



		bLane0_1_Status = ByteBuf[0];

		bAdjust_Req_0_1 = ByteBuf[4];
		

		//D("Reading lane status: bLane0_1_Status = %.2x\n",(WORD)bLane0_1_Status);


		if((bLane0_1_Status&0x01)== 0x01)//all channel CR done
		{

			D("CR training succeed\n");
			
			SP_TX_Training_Pattern_Set(TRAINING_PTN2);
#if 0
			//Lane 0 setting
			c0 = bAdjust_Req_0_1&0x0f;//level
			c1 = c0 & 0x03;//swing
			c2 = (c0 & 0x0c)<<1;//pre-emphasis	
			if(((c0 & 0x0c) == 0x0c)||((c0 & 0x03) == 0x03))//maxswing reached or max pre-emphasis
			{
				c1 |= 0x04;
				c2 |= 0x20;
			}
			s1 = c1 | c2;

			SP_TX_Lane_Set(0,s1);
#endif
			//Write 2 bytes
			ByteBuf[0] = 0x22;
			//ByteBuf[1] = s1;
			SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x02,1,ByteBuf);

			SP_TX_Set_Link_state(EQ_TRAINING_PROCESS);
		}
		else
		{
			SP_TX_Lane_Get(0,&c0);
			SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x06,1,ByteBuf);
                       
                     bAdjust_Req_0_1 = ByteBuf[0];

			if((c0&0x03)==(bAdjust_Req_0_1&0x03))//|| ((c0&0x18)==((bAdjust_Req_0_1&0x0c)<<1)))//lane 0 same voltage count
				CRLoop0++;


                   // if(sp_tx_bw == 0x06) //for 1.62G
                    //    cCRloopCount = CR_LOOP_TIME -1;
                    //else
                        cCRloopCount = CR_LOOP_TIME ;

			//if max swing reached or same voltage 5 times, try reduced bit-rate
			if(((c0&0x03)==0x03)||(CRLoop0 ==CR_LOOP_TIME))
			{
		
                        D("CR training failed due to loop > 5");

                        //Set to Normal, and enable scramble
                        SP_TX_Training_Pattern_Set(NONE);
                        ByteBuf[0] = 0;
                        SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x02, 0x01, ByteBuf);

                        D("Set PT0 to terminate LT");

                        SP_TX_Set_Link_state(LINKTRAINING_FINISHED);
				
			}
			else //increase voltage swing as requested,write an updated value
			{
				//Lane 0 setting
				//c0 = bAdjust_Req_0_1&0x0f;//level
				c1 = bAdjust_Req_0_1 & 0x03;//swing
				c2 = (bAdjust_Req_0_1 & 0x0c)<<1;//pre-emphasis	
				if((bAdjust_Req_0_1 & 0x03) == 0x03)//(((bAdjust_Req_0_1 & 0x0c) == 0x08)||((bAdjust_Req_0_1 & 0x03) == 0x03))//maxswing reached or max pre-emphasis
				{
					c1 |= 0x04;
					//c2 |= 0x20;
				}
                            if((bAdjust_Req_0_1 & 0x0c) == 0x08)
                            {
                                   c2 |= 0x20;
                            }
				s1 = c1 | c2;
                            SP_TX_Lane_Set(0, s1);
                           
				//Write 1 bytes
				ByteBuf[0] = s1;
				SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x03, 1, ByteBuf);
				
			}
		}		


}

void SP_TX_EQ_Process(void)
{
	BYTE bLane0_1_Status;
	BYTE bAdjust_Req_0_1;
	BYTE bLane_align_Status;
	BYTE c1,c2,c;
       BYTE s1;
	
	mdelay(16);


	SP_TX_AUX_DPCDRead_Bytes(0x00,0x02,0x02, 6, ByteBuf);
         

		bLane0_1_Status = ByteBuf[0];
		//D("Reading lane status: lane0 = %.2x\n",(WORD)bLane0_1_Status);


		bAdjust_Req_0_1 = ByteBuf[4];
		bLane_align_Status = ByteBuf[2];
		//D("Reading lane align status: align = %.2x\n",(WORD)bLane_align_Status);


		if((bLane0_1_Status&0x01)==0x01)//all channel CR done
		{
			if(((bLane0_1_Status&0x06)!= 0x06)||(bLane_align_Status &0x01) != 0x01 ) //not all locked
			{
				bEQLoopcnt++;

				if(bEQLoopcnt >EQ_LOOP_TIME)
				{

                                    D("EQ training failed due to loop > 5");

                                    //Set to Normal, and enable scramble
                                    SP_TX_Training_Pattern_Set(NONE);
                                    ByteBuf[0] = 0;
                                    SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x02, 0x01, ByteBuf);

                                    D("EQ failed, loop > 5");
                                    SP_TX_Get_Link_BW(&c);
                                    switch(c&0x1f)
                                    {
                                        case 0x14:
                                            LTResult.LT_1_54 = 0;
                                            break;
                                        //case 0x10:
                                        //    LTResult.LT_1_45 = 0;
                                        //    break;
                                        case 0x0a:
                                            LTResult.LT_1_27 = 0;
                                            break;
                                        case 0x06:
                                            LTResult.LT_1_162= 0;
                                            break;
                                        default:
                                            break;
                                    }

                                    SP_TX_Set_Link_state(LINKTRAINING_FINISHED);	 				
	
				}
				else //adjust pre-emphasis level 
				{
					
					D("Reading lane adjust bAdjust_Req0 = %.2x\n",(WORD)bAdjust_Req_0_1);

					//Lane 0 setting
					///c0 = bAdjust_Req_0_1&0x0f;//level
					c1 = bAdjust_Req_0_1 & 0x03;//swing
					c2 = (bAdjust_Req_0_1 & 0x0c)<<1;//pre-emphasis	
					if((bAdjust_Req_0_1 & 0x03) == 0x03)
                                    {
                                         c1 |= 0x04;
                                    }
					if((bAdjust_Req_0_1 & 0x0c) == 0x08)//||((bAdjust_Req_0_1 & 0x03) == 0x03))//maxswing reached or max pre-emphasis
					{
					      c2 |= 0x20;
					}
					s1 = c1 | c2;

					SP_TX_Lane_Set(0, s1);

					ByteBuf[0] = s1;
					SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x03, 1, ByteBuf);


				}
			}
			else //EQ succeed
			{

				D("EQ training succeed!\n");

				//Set to Normal, and enable scramble
				SP_TX_Training_Pattern_Set(NONE);
#ifdef DISBALBE_SCRAMBLE
					ByteBuf[0] = 0x20;
#else
					ByteBuf[0] = 0;
#endif
				SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x02, 0x01, ByteBuf);
                            SP_TX_Get_Link_BW(&c);
                            switch(c&0x1f)
                            {
                                case 0x14:
                                    LTResult.LT_1_54 = 1;
                                    break;
                                case 0x0a:
                                    LTResult.LT_1_27 = 1;
                                    break;
                                case 0x06:
                                    LTResult.LT_1_162= 1;
                                    break;
                                default:
                                    break;
                            }

				SP_TX_Set_Link_state(LINKTRAINING_FINISHED);
			}
				
		}
		else//already in reduced bit rate
		{

			D("EQ training failed due to CR loss");

			//Set to Normal, and enable scramble
			SP_TX_Training_Pattern_Set(NONE);
                     ByteBuf[0] = 0;
			SP_TX_AUX_DPCDWrite_Bytes(0x00,0x01,0x02, 0x01, ByteBuf);
                     D("LT failed2!\n");
			SP_TX_Set_Link_state(LINKTRAINING_FINISHED);
		}

	}

void SP_TX_LT_Finished_Process(void)
{
        BYTE c;
	
	SP_TX_Lane_Get(0,&c);
	//D("LANE0_SET = %.2x\n",(WORD)c);
	SP_TX_Get_Link_BW(&c);
	D("sp_tx_final_bw = %.2x\n",(WORD)c);    
	//SP_TX_Get_Lane_Count(&sp_tx_final_lane_count);
	//D("sp_tx_final_lane_count = %.2x\n",(WORD)sp_tx_final_lane_count);
}

void SP_TX_Set_Link_state(SP_SW_LINK_State eState)
{
	switch(eState)
	{
		case LINKTRAINING_START:
			eSW_Link_state = LINKTRAINING_START;
			D("LINKTRAINING_START\n");
			break;
		case CLOCK_RECOVERY_PROCESS:
			eSW_Link_state = CLOCK_RECOVERY_PROCESS;
			D("CLOCK_RECOVERY_PROCESS\n");
			break;
		case EQ_TRAINING_PROCESS:
			eSW_Link_state = EQ_TRAINING_PROCESS;	
			D("EQ_TRAINING_PROCESS");
			break;
		case LINKTRAINING_FINISHED:
			eSW_Link_state = LINKTRAINING_FINISHED;
			D("LINKTRAINING_FINISHED");
			break;
		default:
			break;
	}

}

void SP_TX_Video_Mute(BYTE enable)
{
        BYTE c;
	if(enable)
	{
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		c |=SP_TX_VID_CTRL1_VID_MUTE;
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG , c);
	}
	else
	{
		SP_TX_Read_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG, &c);
		c &=~SP_TX_VID_CTRL1_VID_MUTE;
		SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VID_CTRL1_REG , c);
	}
		

}


void SP_TX_Config_MIPI_Video_Format()
{
	long int M_vid;
	//long float lTemp;
	long int lBW =0;
	long int l_M_Vid =0;

	BYTE bIndex;
	WORD MIPI_Format_data;
	
	BYTE c,c1,c2;

	//clear force stream valid flag
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, c&0xfc);
	

	//Get BW
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_LINK_BW_SET_REG, &c);
	if(c==0x06)
	{
		D("1.62G");
		lBW = 162;
	}
	else if(c==0x0a)
	{
		D("2.7G");
		lBW = 270;
	}
	else if(c==0x14)
	{
		D("5.4G");
		lBW = 540;
	}
	else
		D("#############invalid BW##############");

	bIndex = MIPI_Format_Index_Get();
	
	M_vid = (long int)((mipi_video_timing_table[bIndex].MIPI_pixel_frequency)*100);

	c = (unsigned char)M_vid;
	c1 = (unsigned char)(M_vid>>8);
	c2 = (unsigned char)(M_vid>>16);
	
	//D("m_vid h = %x,m_vid m = %x, mvid l= %x \n",(WORD)c2,(WORD)c1,(WORD)c);

	
	M_vid = M_vid*32768;

	c = (unsigned char)M_vid;
	c1 = (unsigned char)(M_vid>>8);
	c2 = (unsigned char)(M_vid>>16);
	
	//D("m_vid h = %x,m_vid m = %x, mvid l= %x \n",(WORD)c2,(WORD)c1,(WORD)c);
	
	M_vid=M_vid/(lBW*100);
	
	//D("m_vid = %x \n", M_vid);

	c = (unsigned char)M_vid;
	c1 = (unsigned char)(M_vid>>8);
	c2 = (unsigned char)(M_vid>>16);
	
	//D("m_vid h = %x,m_vid m = %x, mvid l= %x \n",(WORD)c2,(WORD)c1,(WORD)c);
	
	//M_vid = ((mipi_video_timing_table[bIndex].MIPI_pixel_frequency)*32768)/lBW;

	//D("m_vid = %x \n", M_vid);

	l_M_Vid =(long int)M_vid;

	//D("m_vid1 = %x \n", l_M_Vid);

	//Set M_vid
	SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x20, c);
	SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x21, c1);
	SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x22, c2);

	/*
	SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x20, 0x75);
	SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x21, 0x27);
	SP_TX_Write_Reg(MIPI_RX_PORT1_ADDR, 0x22, 0x0);
*/

	//Vtotal
	//MIPI_Format_data = (mipi_video_timing_table[bIndex].VTOTAL);
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_VTOTAL);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_LINEL_REG, MIPI_Format_data);
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_LINEH_REG, (MIPI_Format_data >> 8));

	//V active
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_VActive);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINEL_REG, MIPI_Format_data);
    SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_LINEH_REG, (MIPI_Format_data >> 8));

	//V Front porch
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_V_Front_Porch);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VF_PORCH_REG, MIPI_Format_data);

	//V Sync width
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_V_Sync_Width);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VSYNC_CFG_REG, MIPI_Format_data);

	//V Back porch
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_V_Back_Porch);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_VB_PORCH_REG, MIPI_Format_data);

	//H total
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_HTOTAL);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXELL_REG, MIPI_Format_data);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_TOTAL_PIXELH_REG, (MIPI_Format_data >> 8));

	//H active
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_HActive);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXELL_REG, MIPI_Format_data);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_ACT_PIXELH_REG, (MIPI_Format_data >> 8));

	//H Front porch
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_H_Front_Porch);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HF_PORCHL_REG, MIPI_Format_data);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HF_PORCHH_REG, (MIPI_Format_data >> 8));

	//H Sync width
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_H_Sync_Width);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HSYNC_CFGL_REG, MIPI_Format_data);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HSYNC_CFGH_REG, (MIPI_Format_data >> 8));

	//H Back porch
	MIPI_Format_data = (mipi_video_timing_table[bIndex].MIPI_H_Back_Porch);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HB_PORCHL_REG, MIPI_Format_data);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, SP_TX_HB_PORCHH_REG, (MIPI_Format_data >> 8));


	//force stream valid for MIPI
	SP_TX_Read_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, &c);
	SP_TX_Write_Reg(SP_TX_PORT0_ADDR, SP_TX_SYS_CTRL3_REG, c|0x03);

	//force video format select from register
	SP_TX_Read_Reg(SP_TX_PORT2_ADDR, 0x011, &c);
	SP_TX_Write_Reg(SP_TX_PORT2_ADDR, 0x11, c|0x10);


}

void MIPI_Format_Index_Set(BYTE bFormatIndex)
{
	bMIPIFormatIndex = bFormatIndex;
	//D("Set MIPI video format index to %d\n",(WORD)bMIPIFormatIndex);
}

BYTE MIPI_Format_Index_Get(void)
{
	D("MIPI video format index is %d\n",(WORD)bMIPIFormatIndex);
	return bMIPIFormatIndex;
}

void SP_TX_MIPI_CONFIG_Flag_Set(BYTE bConfigured)
{
	if(bConfigured)
		bMIPI_Configured = 1;
	else
		bMIPI_Configured = 0;
	
}

BYTE MIPI_CheckSum_Status_OK(void)
{
	BYTE c;
	
	SP_TX_Read_Reg(MIPI_RX_PORT1_ADDR, MIPI_PROTOCOL_STATE, &c);
	//D("protocol state = %.2x\n",(WORD)c);
	if(c&MIPI_CHECKSUM_ERR)
		return 0;
	else
		return 1;
}

