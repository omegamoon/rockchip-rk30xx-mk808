///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >cat6611_sys.c<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/

///////////////////////////////////////////////////////////////////////////////
// This is the sample program for CAT6611 driver usage.
///////////////////////////////////////////////////////////////////////////////

#include "hdmitx.h"

/*
 *	Input Video Data Bus Type
 */
#define HDMITX_INPUT_SIGNAL_TYPE 0  // for default(Sync Sep Mode)
//#define HDMITX_INPUT_SIGNAL_TYPE T_MODE_SYNCEMB                    // for 16bit sync embedded, color is YUV422 only
//#define HDMITX_INPUT_SIGNAL_TYPE (T_MODE_SYNCEMB | T_MODE_CCIR656) // for 8bit sync embedded, color is YUV422 only
//#define HDMITX_INPUT_SIGNAL_TYPE T_MODE_INDDR                      // for DDR input
//#define HDMITX_INPUT_SIGNAL_TYPE T_MODE_DEGEN                      // for DE generating by input sync.
//#define HDMITX_INPUT_SIGNAL_TTYP (T_MODE_DEGEN| T_MODE_SYNCGEN)    // for DE and sync regenerating by input sync.

/*
 *	Input Audio Bus Type: 0 for I2S; 1 for SPDIF.
 */
#ifdef CONFIG_MACH_RK29_ITV_HOTDOG
#define INPUT_SPDIF_ENABLE      1
#else
#define INPUT_SPDIF_ENABLE	0
#endif

/*******************************
 * Global Data
 ******************************/
static _XDATA AVI_InfoFrame AviInfo;
static _XDATA Audio_InfoFrame AudioInfo;
static unsigned long VideoPixelClock;
static unsigned int pixelrep;

extern int CAT6611_Interrupt_Process(void);
/*******************************
 * Functions
 ******************************/
int CAT6611_detect_device(void)
{
	uint8_t VendorID0, VendorID1, DeviceID0, DeviceID1;
	
	Switch_HDMITX_Bank(0);
	VendorID0 = HDMITX_ReadI2C_Byte(REG_VENDOR_ID0);
	VendorID1 = HDMITX_ReadI2C_Byte(REG_VENDOR_ID1);
	DeviceID0 = HDMITX_ReadI2C_Byte(REG_DEVICE_ID0);
	DeviceID1 = HDMITX_ReadI2C_Byte(REG_DEVICE_ID1);
	ErrorF("CAT6611: Reg[0-3] = 0x[%02x].[%02x].[%02x].[%02x]\n",
			   VendorID0, VendorID1, DeviceID0, DeviceID1);
	if( (VendorID0 == 0x00) && (VendorID1 == 0xCA) &&
		(DeviceID0 == 0x11) && (DeviceID1 == 0x16) )
		return 1;

	printk("[CAT6611] Device not found!\n");
	return 0;
}

int CAT6611_sys_init(struct hdmi *hdmi)
{
	mdelay(5);
	VideoPixelClock = 0;
	pixelrep = 0;
	InitCAT6611();
	msleep(100);
	return HDMI_ERROR_SUCESS;
}

int CAT6611_sys_unplug(struct hdmi *hdmi)
{
	HDMITX_WriteI2C_Byte(REG_SW_RST, B_AREF_RST | B_VID_RST | B_AUD_RST | B_HDCP_RST) ;
    mdelay(1) ;
    HDMITX_WriteI2C_Byte(REG_AFE_DRV_CTRL, B_AFE_DRV_RST | B_AFE_DRV_PWD) ;
//    ErrorF("Unplug, %x %x\n", HDMITX_ReadI2C_Byte(REG_SW_RST), HDMITX_ReadI2C_Byte(REG_AFE_DRV_CTRL)) ;
    return HDMI_ERROR_SUCESS;
}

int CAT6611_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus)
{
//	BYTE sysstat;
	
	//sysstat = HDMITX_ReadI2C_Byte(REG_SYS_STATUS) ;   
    //*hpdstatus = ((sysstat & B_HPDETECT) == B_HPDETECT)?TRUE:FALSE ;
    *hpdstatus = CAT6611_Interrupt_Process();
    
    return HDMI_ERROR_SUCESS;
}

int CAT6611_sys_detect_sink(struct hdmi *hdmi, int *sink_status)
{
	u8 sysstatus ;
    Switch_HDMITX_Bank(0);
    sysstatus = HDMITX_ReadI2C_Byte(REG_SYS_STATUS);
    *sink_status = ((sysstatus & (B_HPDETECT|B_RXSENDETECT)) == (B_HPDETECT|B_RXSENDETECT))?TRUE:FALSE ;
    return HDMI_ERROR_SUCESS;
}

int CAT6611_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff)
{
	return (GetEDIDData(block, buff) == TRUE)?HDMI_ERROR_SUCESS:HDMI_ERROR_FALSE;
}

static void cat6611_sys_config_avi(int VIC, int bOutputColorMode, int aspec, int Colorimetry, int pixelrep)
{
//     AVI_InfoFrame AviInfo;

    AviInfo.pktbyte.AVI_HB[0] = AVI_INFOFRAME_TYPE|0x80 ; 
    AviInfo.pktbyte.AVI_HB[1] = AVI_INFOFRAME_VER ; 
    AviInfo.pktbyte.AVI_HB[2] = AVI_INFOFRAME_LEN ; 
    
    switch(bOutputColorMode)
    {
	    case F_MODE_YUV444:
	        // AviInfo.info.ColorMode = 2 ;
	        AviInfo.pktbyte.AVI_DB[0] = (2<<5)|(1<<4) ;
	        break ;
	    case F_MODE_YUV422:
	        // AviInfo.info.ColorMode = 1 ;
	        AviInfo.pktbyte.AVI_DB[0] = (1<<5)|(1<<4) ;
	        break ;
	    case F_MODE_RGB444:
	    default:
	        // AviInfo.info.ColorMode = 0 ;
	        AviInfo.pktbyte.AVI_DB[0] = (0<<5)|(1<<4) ;
	        break ;
    }
// 
//     AviInfo.info.ActiveFormatAspectRatio = 8 ; // same as picture aspect ratio
    AviInfo.pktbyte.AVI_DB[1] = 8 ;
//     AviInfo.info.PictureAspectRatio = (aspec != HDMI_16x9)?1:2 ; // 4x3
    AviInfo.pktbyte.AVI_DB[1] |= (aspec != HDMI_16x9)?(1<<4):(2<<4) ; // 4:3 or 16:9
//     AviInfo.info.Colorimetry = (Colorimetry != HDMI_ITU709) ? 1:2 ; // ITU601
    AviInfo.pktbyte.AVI_DB[1] |= (Colorimetry != HDMI_ITU709)?(1<<6):(2<<6) ; // 4:3 or 16:9
//     AviInfo.info.Scaling = 0 ;
    AviInfo.pktbyte.AVI_DB[2] = 0 ;
//     AviInfo.info.VIC = VIC ;
    AviInfo.pktbyte.AVI_DB[3] = VIC ;
//     AviInfo.info.PixelRepetition = pixelrep;
    AviInfo.pktbyte.AVI_DB[4] =  pixelrep & 3 ;
    AviInfo.pktbyte.AVI_DB[5] = 0 ;
    AviInfo.pktbyte.AVI_DB[6] = 0 ;
    AviInfo.pktbyte.AVI_DB[7] = 0 ;
    AviInfo.pktbyte.AVI_DB[8] = 0 ;
    AviInfo.pktbyte.AVI_DB[9] = 0 ;
    AviInfo.pktbyte.AVI_DB[10] = 0 ;
    AviInfo.pktbyte.AVI_DB[11] = 0 ;
    AviInfo.pktbyte.AVI_DB[12] = 0 ;

    EnableAVIInfoFrame(TRUE, (unsigned char *)&AviInfo) ;
}

int CAT6611_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color)
{
	int ret;
	BYTE bInputColorMode, bOutputColorMode, aspec, Colorimetry;
	struct fb_videomode *vmode;
	
	vmode = (struct fb_videomode*)ext_hdmi_vic_to_videomode(vic);
	if(vmode == NULL)
		return HDMI_ERROR_FALSE;

    
    if(vmode->xres == 1280 || vmode->xres == 1920)
    {
    	aspec = HDMI_16x9;
    	Colorimetry = HDMI_ITU709;
    }
    else
    {
    	aspec = HDMI_4x3;
    	Colorimetry = HDMI_ITU601;
    }
    
    if(vmode->xres == 1440)
    	pixelrep = 1;
    else if(vmode->xres == 2880)
    	pixelrep = 3;
    else
    	pixelrep = 0;
	
	bInputColorMode = input_color & 0xFF;
	// Input color is IT RGB mode, so no need to set color quantization area, default is F_VIDMODE_0_255.  
	if( Colorimetry == HDMI_ITU709 )
        bInputColorMode |= F_VIDMODE_ITU709 ;
    else
        bInputColorMode &= ~F_VIDMODE_ITU709 ;

    switch(output_color)
    {
	    case HDMI_VIDEO_YCbCr444:
	        bOutputColorMode = F_MODE_YUV444 ;
	        break ;
	    case HDMI_VIDEO_YCbCr422:	
	        bOutputColorMode = F_MODE_YUV422 ;
	        break ;
	    case HDMI_VIDEO_RGB:
	        bOutputColorMode = F_MODE_RGB444 ;
	        break ;	
	    default:
	        bOutputColorMode = F_MODE_RGB444 ;
	        break ;
    }
    
    VideoPixelClock = vmode->pixclock;
	ret = EnableVideoOutput(VideoPixelClock > 80000000, bInputColorMode,
		   	HDMITX_INPUT_SIGNAL_TYPE, bOutputColorMode, hdmi->edid.is_hdmi) ;
	if(ret && hdmi->edid.is_hdmi)
		cat6611_sys_config_avi(vic, bOutputColorMode, aspec, Colorimetry, pixelrep);
	return ret == TRUE ? HDMI_ERROR_SUCESS : HDMI_ERROR_FALSE;
}

static void cat6611_sys_config_aai(void)
{
    int i ;
    ErrorF("ConfigAudioInfoFrm(%d)\n",2) ;
//    memset(&AudioInfo,0,sizeof(Audio_InfoFrame)) ;
//    
    AudioInfo.pktbyte.AUD_HB[0] = AUDIO_INFOFRAME_TYPE ;
    AudioInfo.pktbyte.AUD_HB[1] = 1 ;
    AudioInfo.pktbyte.AUD_HB[2] = AUDIO_INFOFRAME_LEN ;
//    
//    // 6611 did not accept this, cannot set anything.
//    // AudioInfo.info.AudioCodingType = 1 ; // IEC60958 PCM 
//    AudioInfo.info.AudioChannelCount = 2 - 1 ;
    AudioInfo.pktbyte.AUD_DB[0] = 1 ;
    for( i = 1 ;i < AUDIO_INFOFRAME_LEN ; i++ )
    {
        AudioInfo.pktbyte.AUD_DB[i] = 0 ;
    }
//
//    /* 
//    CAT6611 does not need to fill the sample size information in audio info frame.
//    ignore the part.
//    */
// 
//    AudioInfo.info.SpeakerPlacement = 0x00 ;   //                     FR FL
//    AudioInfo.info.LevelShiftValue = 0 ;    
//    AudioInfo.info.DM_INH = 0 ;    
//
    EnableAudioInfoFrame(TRUE, (unsigned char *)&AudioInfo) ;
}

int CAT6611_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio)
{
	unsigned long audio_freq;
	unsigned char word_length;
	
	switch(audio->rate)
	{
		case HDMI_AUDIO_FS_32000:
			audio_freq = 32000;
			break;
		case HDMI_AUDIO_FS_44100:
			audio_freq = 44100;
			break;
		case HDMI_AUDIO_FS_48000:
			audio_freq = 48000;
			break;
		case HDMI_AUDIO_FS_88200:
			audio_freq = 88200;
			break;
		case HDMI_AUDIO_FS_96000:
			audio_freq = 96000;
			break;
		case HDMI_AUDIO_FS_176400:
			audio_freq = 176400;
			break;
		case HDMI_AUDIO_FS_192000:
			audio_freq = 192000;
		default:
			printk("[%s] not support such sample rate\n", __FUNCTION__);
			return HDMI_ERROR_FALSE;
	}
	switch(audio->word_length)
	{
		case HDMI_AUDIO_WORD_LENGTH_16bit:
			word_length = 16;
			break;
		case HDMI_AUDIO_WORD_LENGTH_20bit:
			word_length = 20;
			break;
		case HDMI_AUDIO_WORD_LENGTH_24bit:
			word_length = 24;
			break;
		default:
			word_length = 24;
			break;
	}
    SetNonPCMAudio(0);
    EnableAudioOutput(VideoPixelClock*(pixelrep + 1), audio_freq, audio->channel, word_length, INPUT_SPDIF_ENABLE);
    cat6611_sys_config_aai() ;
	
	return HDMI_ERROR_SUCESS;
}

int CAT6611_sys_config_hdcp(struct hdmi *hdmi, int enable)
{
	SetAVMute(TRUE) ;
	EnableHDCP(enable);
//	if(!enable) SetAVMute(FALSE) ;
	return HDMI_ERROR_SUCESS;
}

int CAT6611_sys_enalbe_output(struct hdmi *hdmi, int enable)
{
	if(enable) {
		SetAVMute(FALSE);
		HDMITX_WriteI2C_Byte(REG_AUDIO_CTRL0, (INPUT_SPDIF_ENABLE << 4) | 0x01) ;
	} else
		SetAVMute(TRUE);
	return HDMI_ERROR_SUCESS;
}
