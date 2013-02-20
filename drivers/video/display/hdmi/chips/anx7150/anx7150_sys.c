#include <linux/delay.h>
#include "anx7150.h"
#include "anx7150_hw.h"

int ANX7150_sys_init(struct hdmi *hdmi)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	int rc;
	
	rc = ANX7150_hw_initial(anx->client);
	
	return rc;
}

int ANX7150_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	int rc;
	
	if(hdmi == NULL || hpdstatus == NULL)
		return HDMI_ERROR_FALSE;

	rc = ANX7150_get_hpd(anx->client, hpdstatus);
	if(rc)
	{
		// Get hpd status error, hold previous value.
		*hpdstatus = hdmi->hpd_status;
	}
	ANX7150_hw_process_interrupt(anx->client);
	return rc;
}

int ANX7150_sys_plug(struct hdmi *hdmi)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	return ANX7150_hw_plug(anx->client);
}

int ANX7150_sys_detect_sink(struct hdmi *hdmi, int *sink_status)
{
	struct anx7150_pdata *anx;
	
	if(hdmi == NULL || sink_status == NULL)
		return HDMI_ERROR_FALSE;
	anx = hdmi_get_privdata(hdmi);
	return ANX7150_get_sink_state(anx->client, sink_status);
}

int ANX7150_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	
	return ANX7150_hw_read_edid(anx->client, block, buff);
}

int ANX7150_sys_config_avi(struct hdmi *hdmi, unsigned char vic, unsigned char output_color)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	int rc = 0;
	infoframe_struct *pavi;
	unsigned char colorimetry, input_aspect, output_aspect, pixel_repeat;
	
	pavi = kmalloc(sizeof(infoframe_struct), GFP_KERNEL);
	if(pavi == NULL)
	{
		printk("[ANX7150_sys_config_avi] can not malloc memory.\n");
		return HDMI_ERROR_FALSE;
	}
	memset(pavi, 0, sizeof(infoframe_struct));
	
	pavi->type = 0x82;
	pavi->version = 0x02;
	pavi->length = 0x0D;
	
	pavi->pb_u8[1] = ( output_color << 5 ) | (1 << 4);
	//[3~0]b: active formate aspect ratio, 0x08:same as picture; 0x09: 4:3(center) ; 0x0a: 16:9(center); 0x0b: 14:9(center) 
	//[5~4]b: picture aspect ratio, 0: no data; 1: 4:3; 2:16:9
	//[7~6]b: colorimetry, 0: no data; 1: ITU601; 2: ITU709 3: extended colorimetry valid
	switch(vic)
	{
		case HDMI_720x576i_50Hz_4_3:
		case HDMI_720x480i_60Hz_4_3:
		case HDMI_720x288p_50Hz_4_3:
		case HDMI_720x240p_60Hz_4_3:
			colorimetry = 0x1;
			input_aspect = 0x1;
			output_aspect = 0x09;
			pixel_repeat = 1;
			break;
		case HDMI_720x576i_50Hz_16_9:
		case HDMI_720x480i_60Hz_16_9:
		case HDMI_720x288p_50Hz_16_9:
		case HDMI_720x240p_60Hz_16_9:
			colorimetry = 0x1;
			input_aspect = 0x2;
			output_aspect = 0x0a;
			pixel_repeat = 1;			 
			break;
		//Pixel sent 1 to 10 times, here set 1 times.
		case HDMI_2880x480i_60Hz_4_3:
		case HDMI_2880x240p_60Hz_4_3:
		case HDMI_2880x576i_50Hz_4_3:
		case HDMI_2880x288p_50Hz_4_3:
		//Pixel sent 1, 2 or 4 times, here set 1 times.			
		case HDMI_2880x480p_60Hz_4_3:
		case HDMI_2880x576p_50Hz_4_3:
		//Pixel sent 1 to 2 times, here set 1 times.	
		case HDMI_1440x480p_60Hz_4_3:
		case HDMI_1440x576p_50Hz_4_3:
		//Pixel sent 1 times.
		case HDMI_720x576p_50Hz_4_3:
		case HDMI_720x480p_60Hz_4_3:
			colorimetry = 0x1;
			input_aspect = 0x1;
			output_aspect = 0x09;
			pixel_repeat = 0;
			break;
		default:
			colorimetry = 0x2;
			input_aspect = 0x2;
			output_aspect = 0x0a;
			pixel_repeat = 0;
			break;	
	}
	pavi->pb_u8[2] = (colorimetry << 6) | (input_aspect << 4) | output_aspect ;
	pavi->pb_u8[3] = 0;
	pavi->pb_u8[4] = vic;
	pavi->pb_u8[5] = pixel_repeat;
	
	// Line Number of End of Top Bar
	pavi->pb_u8[6] = 0; //low
	pavi->pb_u8[7] = 0; //high
	// Line Number of Start of Bottom Bar 
	pavi->pb_u8[8] = 0; //low
	pavi->pb_u8[9] = 0; //high
	// Pixel Number of End of Left Bar
	pavi->pb_u8[10] = 0; // low
	pavi->pb_u8[11] = 0; // high
	// Pixel Number of Start of Right Bar
	pavi->pb_u8[12] = 0; // low
	pavi->pb_u8[13] = 0; // high
	
	rc = ANX7150_hw_config_avi(anx->client, pavi);
	
	kfree(pavi);
	return rc;
}

int ANX7150_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	int rc = 0, pixel_repeat;
	
	if ((vic == HDMI_720x480i_60Hz_4_3)
		|| (vic == HDMI_720x480i_60Hz_16_9)
		|| (vic == HDMI_720x576i_50Hz_4_3)
		|| (vic == HDMI_720x576i_50Hz_16_9))
		pixel_repeat = input_pixel_clk_2x_repeatition;
	else
		pixel_repeat = input_pixel_clk_1x_repeatition;
	
	rc = ANX7150_hw_config_video(anx->client, vic, input_color, output_color, 
						pixel_repeat, pixel_repeat,	hdmi->edid.is_hdmi, ANX7150_RGB_YCrCb444_SepSync);
	if(!rc && hdmi->edid.is_hdmi)
		rc = ANX7150_sys_config_avi(hdmi, vic, HDMI_VIDEO_RGB);
	return rc;
}

int ANX7150_sys_config_aai(struct hdmi *hdmi)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	int rc = 0;
	infoframe_struct *paai;
	
	paai = kmalloc(sizeof(infoframe_struct), GFP_KERNEL);
	if(paai == NULL)
	{
		printk("[ANX7150_sys_config_aai] can not malloc memory.\n");
		return HDMI_ERROR_FALSE;
	}
	memset(paai, 0, sizeof(infoframe_struct));
	
	paai->type = 0x84;
	paai->version = 0x01;
	paai->length = 0x0A;

	rc = ANX7150_hw_config_aai(anx->client, paai);
	
	kfree(paai);
	
	return rc;
}

int ANX7150_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio)
{
	int rc = 0;
	struct anx7150_pdata *anx;
	unsigned char anx7150_fs, anx7150_word_length, mclk_fs;
	
	if(hdmi == NULL || audio == NULL)
		return HDMI_ERROR_FALSE;
	
	if(audio->type != HDMI_AUDIO_LPCM)
		return HDMI_ERROR_FALSE;
		
	switch(audio->rate)
	{
		case HDMI_AUDIO_FS_32000:
			anx7150_fs = 3;
			mclk_fs = 0x02;
			break;
		case HDMI_AUDIO_FS_44100:
			anx7150_fs = 0;
			mclk_fs = 0x01;
			break;
		case HDMI_AUDIO_FS_48000:
			anx7150_fs = 2;
			mclk_fs = 0x01;
			break;
		case HDMI_AUDIO_FS_88200:
			anx7150_fs = 8;
			mclk_fs = 0x00;
			break;
		case HDMI_AUDIO_FS_96000:
			anx7150_fs = 10;
			mclk_fs = 0x00;
			break;
		case HDMI_AUDIO_FS_176400:
			anx7150_fs = 12;
			mclk_fs = 0x00;
			break;
		case HDMI_AUDIO_FS_192000:
			anx7150_fs = 14;
			mclk_fs = 0x00;
			break;
		default:
			return HDMI_ERROR_FALSE;
	}
	
	switch(audio->word_length)
	{
		case HDMI_AUDIO_WORD_LENGTH_16bit:
			anx7150_word_length = 0x0b;
			break;
		default:
			dev_printk(KERN_INFO , hdmi->dev, "Not support such word length\n");
			return HDMI_ERROR_FALSE;
	}
	anx = hdmi_get_privdata(hdmi);
	rc = ANX7150_hw_config_audio(anx->client, ANX7150_AUD_HW_INTERFACE, audio->channel, 
								 ANX7150_AUD_CLK_EDGE, anx7150_fs, mclk_fs, 0, anx7150_word_length);
	if(!rc)
		rc = ANX7150_sys_config_aai(hdmi);
	return rc;
}

int ANX7150_sys_config_hdcp(struct hdmi *hdmi, int enable)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	
	return ANX7150_hw_config_hdcp(anx->client, hdmi->edid.is_hdmi, enable);
}

int ANX7150_sys_enalbe_output(struct hdmi *hdmi, int enable)
{
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	
	return ANX7150_hw_enalbe_output(anx->client, enable);
}
