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
#include <linux/i2c.h>
#ifndef _SP_TX_DRV_H
#define _SP_TX_DRV_H


typedef unsigned char BYTE;
typedef unsigned int  WORD;
typedef unsigned char *pByte;


#define MAX_BUF_CNT 6

#define CR_LOOP_TIME 5
#define EQ_LOOP_TIME 5

#define SP_TX_AVI_SEL 0x01
#define SP_TX_SPD_SEL 0x02
#define SP_TX_MPEG_SEL 0x04


#define SP_TX_PORT0_ADDR 0x70
#define SP_TX_PORT1_ADDR 0x74
#define SP_TX_PORT2_ADDR 0x72
#define MIPI_RX_PORT1_ADDR 0x7A


#define D(fmt, arg...) printk("<1>```%s:%d: " fmt, __FUNCTION__, __LINE__, ##arg)
#define D_MSG(fmt, arg...) printk("<2>```%s:%d: " fmt, __FUNCTION__, __LINE__, ##arg)


#define SP_TX_HDCP_FAIL_THRESHOLD         10

#define EDID_Dev_ADDR 0xa0
#define EDID_SegmentPointer_ADDR 0x60

extern BYTE sp_tx_lane_count;
extern BYTE sp_tx_test_bw;
extern BYTE sp_tx_test_lane_count;
extern BYTE sp_tx_link_config_done;
//extern BYTE sp_tx_final_lane_count;
extern BYTE sp_tx_test_link_training;
extern BYTE checksum;
extern BYTE sp_tx_edid_err_code;
extern BYTE sp_tx_hdcp_enable,sp_tx_ssc_enable;
extern BYTE edid_pclk_out_of_range;
extern BYTE SP_TX_EDID_PREFERRED[18];
extern BYTE bEDIDBreak;
extern BYTE sp_tx_ds_edid_hdmi;
//0811
extern long int pclk;
//extern long int pclk;
extern BYTE ByteBuf[MAX_BUF_CNT];
extern BYTE EDID_Print_Enable;



//extern BYTE VSDBaddr;

struct Bist_Video_Format {
    char number;
    char video_type[32];
    unsigned int pixel_frequency;    
    unsigned int h_total_length;
    unsigned int h_active_length;
    unsigned int v_total_length;
    unsigned int v_active_length;
    unsigned int h_front_porch;
    unsigned int h_sync_width;
    unsigned int h_back_porch;
    unsigned int v_front_porch;
    unsigned int v_sync_width;
    unsigned int v_back_porch;
    unsigned char h_sync_polarity;
    unsigned char v_sync_polarity;
    unsigned char is_interlaced;
    unsigned char pix_repeat_times;
    unsigned char frame_rate;
    unsigned char bpp_mode;
    unsigned char video_mode;
};

struct LT_Result {
    BYTE LT_1_54;
    //BYTE LT_1_45;
    BYTE LT_1_27;
    BYTE LT_1_162;
    BYTE LT_2_54;
    //BYTE LT_2_45;
    BYTE LT_2_27;
    BYTE LT_2_162;

};


typedef enum
{
	SP_TX_PWR_REG,
	SP_TX_PWR_HDCP,
	SP_TX_PWR_AUDIO,
	SP_TX_PWR_VIDEO,
	SP_TX_PWR_LINK,
	SP_TX_PWR_TOTAL
}SP_TX_POWER_BLOCK;



typedef enum
{
	AUDIO_BIST,
	AUDIO_SPDIF,
	AUDIO_I2S
}AudioType;

typedef enum
{
	AUDIO_FS_441K = 0x00,
	AUDIO_FS_48K   = 0x02,
	AUDIO_FS_32K   = 0x03,
	AUDIO_FS_882K = 0x08,
	AUDIO_FS_96K   = 0x0a,
	AUDIO_FS_1764K= 0x0c,
	AUDIO_FS_192K =  0x0e
}AudioFs;

typedef enum
{
	AUDIO_W_LEN_16_20MAX = 0x02,
	AUDIO_W_LEN_18_20MAX = 0x04,
	AUDIO_W_LEN_17_20MAX = 0x0c,
	AUDIO_W_LEN_19_20MAX = 0x08,
	AUDIO_W_LEN_20_20MAX = 0x0a,
	AUDIO_W_LEN_20_24MAX = 0x03,
	AUDIO_W_LEN_22_24MAX = 0x05,
	AUDIO_W_LEN_21_24MAX = 0x0d,
	AUDIO_W_LEN_23_24MAX = 0x09,
	AUDIO_W_LEN_24_24MAX = 0x0b
}AudioWdLen;

typedef enum
{
	I2S_CH_2 =0x01,
	I2S_CH_4 =0x03,
	I2S_CH_6 =0x05,
	I2S_CH_8 =0x07
}I2SChNum;

typedef enum
{
	I2S_LAYOUT_0,
	I2S_LAYOUT_1 
}I2SLayOut;


typedef struct 
{
     I2SChNum Channel_Num;
     I2SLayOut  AUDIO_LAYOUT;	
     unsigned char SHIFT_CTRL;
     unsigned char DIR_CTRL;
     unsigned char WS_POL;
     unsigned char JUST_CTRL;
     unsigned char EXT_VUCP;
     unsigned char I2S_SW;
     unsigned char Channel_status1;
     unsigned char Channel_status2;
     unsigned char Channel_status3;
     unsigned char Channel_status4;
     unsigned char Channel_status5;
	
}I2S_FORMAT;




struct AudioFormat
{
	AudioType bAudioType;
	unsigned char  bAudio_Fs;
	unsigned char bAudio_word_len;
	I2S_FORMAT bI2S_FORMAT;
};

typedef enum
{
	AVI_PACKETS,
	SPD_PACKETS,
	MPEG_PACKETS
}PACKETS_TYPE;

struct Packet_AVI{
     unsigned char AVI_data[13];
} ;


struct Packet_SPD{
     unsigned char SPD_data[25];    
};      


struct Packet_MPEG{
     unsigned char MPEG_data[10];   
} ;


struct AudiInfoframe
{
     unsigned char type;
     unsigned char version; 
     unsigned char length;
     unsigned char pb_byte[10]; //modify to 10 bytes from 28.2008/10/23   
};


typedef struct subEmbedded_Sync_t
{
     unsigned char Embedded_Sync;
     unsigned char Extend_Embedded_Sync_flag;
}subEmbedded_Sync;

typedef struct subYC_MUX_t
{
     unsigned char YC_MUX;
     unsigned char YC_BIT_SEL;
}subYC_MUX;

struct LVTTL_HW_Interface
{
	unsigned char DE_reDenerate;
	unsigned char DDR_Mode;
	//unsigned char BPC;
	unsigned char Clock_EDGE;// 1:negedge 0:posedge
	subEmbedded_Sync sEmbedded_Sync;// 1:valueable
	subYC_MUX sYC_MUX;
};

typedef enum
{
	LVTTL_RGB,
	MIPI_DSI
}VideoInterface;

typedef enum
{
	COLOR_6_BIT,
	COLOR_8_BIT,
	COLOR_10_BIT,
	COLOR_12_BIT    	
}ColorDepth;

typedef enum
{
	COLOR_RGB,
	COLOR_YCBCR_422,
	COLOR_YCBCR_444
}ColorSpace;

struct VideoFormat
{    
	VideoInterface Interface;// 0:LVTTL ; 1:mipi
	ColorDepth bColordepth;
	ColorSpace bColorSpace;
	struct LVTTL_HW_Interface bLVTTL_HW_Interface;
};

typedef enum
{
	COMMON_INT_1 = 0,
	COMMON_INT_2 = 1,
	COMMON_INT_3 = 2,
	COMMON_INT_4 = 3,
	SP_INT_STATUS = 6
}INTStatus;


typedef enum
{
	PLL_BLOCK,
	AUX_BLOCK,
	CH0_BLOCK,
	CH1_BLOCK,
	ANALOG_TOTAL,
	POWER_ALL
}ANALOG_PWD_BLOCK;

typedef enum
{
	PRBS_7,
	D10_2,
	TRAINING_PTN1,
	TRAINING_PTN2,
	NONE
}PATTERN_SET;

typedef enum
{
	BW_54G = 0x14,
	//BW_45G = 0x10,	
	BW_27G = 0x0A,
	BW_162G = 0x06,
	BW_NULL = 0x00
}SP_LINK_BW;

typedef enum{
    LINKTRAINING_START,
    CLOCK_RECOVERY_PROCESS,
    EQ_TRAINING_PROCESS,
    LINKTRAINING_FINISHED
} SP_SW_LINK_State;

struct MIPI_Video_Format {
    char MIPI_number;
	char MIPI_video_type[32];
	int MIPI_pixel_frequency;
	unsigned int MIPI_HTOTAL;
	unsigned int MIPI_HActive;
	unsigned int MIPI_VTOTAL;
	unsigned int MIPI_VActive;

	unsigned int MIPI_H_Front_Porch;
	unsigned int MIPI_H_Sync_Width;
	unsigned int MIPI_H_Back_Porch;
	

	unsigned int MIPI_V_Front_Porch;
	unsigned int MIPI_V_Sync_Width;
	unsigned int MIPI_V_Back_Porch;
};


void SP_TX_Initialization(struct VideoFormat* pInputFormat);
void SP_TX_Config_Video (struct VideoFormat* pInputFormat);
void SP_TX_Show_Infomation(void);
void SP_TX_Disable_Video_Input(void);
void SP_TX_Power_Down(SP_TX_POWER_BLOCK sp_tx_pd_block);
void SP_TX_Power_On(SP_TX_POWER_BLOCK sp_tx_pd_block);
//void SP_TX_Enable_Video_Input(ColorDepth cColorDepth);
void SP_TX_AVI_Setup(void);
void SP_TX_Enable_Video_Input(void);
//BYTE SP_TX_AUX_DPCDRead_Byte(BYTE addrh, BYTE addrm, BYTE addrl);
void SP_TX_AUX_DPCDWrite_Byte(BYTE addrh, BYTE addrm, BYTE addrl, BYTE data1);
void SP_TX_HDCP_Encryption_Disable(void) ;
void SP_TX_HDCP_Encryption_Enable(void) ;
void SP_TX_HW_HDCP_Enable(void);
void SP_TX_HW_HDCP_Disable(void);
void SP_TX_Clean_HDCP(void);
void SP_TX_PBBS7_Test(void);
void SP_TX_Insert_Err(void);
void SP_TX_EnhaceMode_Set(void);
void SP_TX_CONFIG_SSC(SP_LINK_BW linkbw);
void SP_TX_Config_Audio(struct AudioFormat *bAudio);
void SP_TX_Config_Audio_I2S(struct AudioFormat *bAudioFormat) ;
void SP_TX_Config_Audio_SPDIF(void) ;
int SP_TX_Chip_Located(void);
void SP_TX_Hardware_Reset(void);
void SP_TX_Hardware_PowerOn(void);
void SP_TX_Hardware_PowerDown(void);
void SP_TX_VBUS_PowerOn(void) ;
void SP_TX_VBUS_PowerDown(void) ;
void SP_TX_RST_AUX(void);
BYTE SP_TX_AUX_DPCDRead_Bytes(BYTE addrh, BYTE addrm, BYTE addrl,BYTE cCount,pByte pBuf);
BYTE SP_TX_Wait_AUX_Finished(void);
//void SP_TX_Wait_AUX_Finished(void);
BYTE SP_TX_AUX_DPCDWrite_Bytes(BYTE addrh, BYTE addrm, BYTE addrl,BYTE cCount,pByte pBuf);
void SP_TX_SW_Reset(void);
void SP_TX_SPREAD_Enable(BYTE bEnable);
void SP_TX_Enable_Audio_Output(BYTE bEnable);
void SP_TX_Disable_Audio_Input(void);
void SP_TX_AudioInfoFrameSetup(void);
void SP_TX_InfoFrameUpdate(struct AudiInfoframe* pAudioInfoFrame);
void SP_TX_Get_Int_status(INTStatus IntIndex, BYTE *cStatus);
//void SP_TX_Get_HPD_status( BYTE *cStatus);
BYTE SP_TX_Get_PLL_Lock_Status(void);
void SP_TX_Get_HDCP_status( BYTE *cStatus);
void SP_TX_HDCP_ReAuth(void);
//void SP_TX_Amplitude(BYTE lane, BYTE level);
//void SP_TX_Emphasis_Level(BYTE lane, BYTE level);
void SP_TX_Lane_Set(BYTE lane, BYTE bVal);
void SP_TX_Lane_Get(BYTE lane, BYTE *bVal);
//void SP_TX_Analog_PD(ANALOG_PWD_BLOCK eBlock, BYTE enable);
void SP_TX_Get_Rx_LaneCount(BYTE bMax,BYTE *cLaneCnt);
void SP_TX_Set_Rx_laneCount(BYTE cLaneCnt);
void SP_TX_Get_Rx_BW(BYTE bMax,BYTE *cBw);
void SP_TX_Set_Rx_BW(BYTE cBw);
void SP_TX_Set_Link_BW(SP_LINK_BW bwtype);
void SP_TX_Set_Lane_Count(BYTE count);
void SP_TX_Get_Link_BW(BYTE *bwtype);
//void SP_TX_Get_Lane_Count(BYTE *count);
void SP_TX_Get_Link_Status(BYTE *cStatus);
void SP_TX_Get_lane_Setting(BYTE cLane, BYTE *cSetting);
//void SP_TX_Set_KSV_Valid(void);
//BYTE SP_TX_Read_BKSV_Valid(void);
//void SP_TX_Generate_AN(void);
//void SP_TX_Read_AKSV(void);
//void SP_TX_Write_BKSV(void);
//void SP_TX_Write_AN(void);
//void SP_TX_Write_AKSV(void);
//BYTE SP_TX_R0_Compare(void);
//void SP_TX_Set_SW_Auth(BYTE bOK);
void SP_TX_EDID_Read_Initial(void);
BYTE SP_TX_AUX_EDIDRead_Byte(BYTE offset);
void SP_TX_Parse_Segments_EDID(BYTE segment, BYTE offset);
BYTE SP_TX_Get_EDID_Block(void);
void SP_TX_AddrOnly_Set(BYTE bSet);
void SP_TX_Scramble_Enable(BYTE bEnabled);
void SP_TX_Training_Pattern_Set(PATTERN_SET PATTERN);
void SP_TX_API_M_GEN_CLK_Select(BYTE bSpreading);
void SP_TX_Config_Packets(PACKETS_TYPE bType);
void SP_TX_Load_Packet (PACKETS_TYPE type);
BYTE SP_TX_Config_Video_LVTTL (struct VideoFormat* pInputFormat);
BYTE SP_TX_Config_Video_MIPI (void);
BYTE SP_TX_BW_LC_Sel(struct VideoFormat* pInputFormat);
void SP_TX_SW_LINK_Process(BYTE lanecnt, SP_LINK_BW linkbw);
void SP_TX_Link_Start_Process(BYTE lanecnt, SP_LINK_BW linkbw);
void SP_TX_CLOCK_Recovery_Process(void);
void SP_TX_EQ_Process(void);
void SP_TX_LT_Finished_Process(void);
void SP_TX_Set_Link_state(SP_SW_LINK_State eState);
void SP_TX_Embedded_Sync(struct VideoFormat* pInputFormat, WORD sp_tx_bist_select_number);
//void SP_TX_DE_reGenerate (WORD sp_tx_bist_select_number);
void SP_TX_PCLK_Calc(BYTE hbr_rbr);
void SP_TX_Link_Training (void);
void SP_TX_HW_Link_Training (void);
BYTE SP_TX_LT_Pre_Config(void);
void SP_TX_LVTTL_Bit_Mapping(struct VideoFormat* pInputFormat);
void SP_TX_Video_Mute(BYTE enable);
void SP_TX_Lanes_PWR_Ctrl(ANALOG_PWD_BLOCK eBlock, BYTE powerdown);
void SP_TX_AUX_WR (BYTE offset);
void SP_TX_AUX_RD (BYTE len_cmd);
void SP_TX_MIPI_CONFIG_Flag_Set(BYTE bConfigured);
void SP_TX_Config_MIPI_Video_Format(void);
void MIPI_Format_Index_Set(BYTE bFormatIndex);
BYTE MIPI_Format_Index_Get(void);
BYTE MIPI_CheckSum_Status_OK(void);




#endif
