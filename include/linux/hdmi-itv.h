/* 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#ifndef __LINUX_HDMI_ITV_CORE_H
#define __LINUX_HDMI_ITV_CORE_H

#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/completion.h>
#include <linux/fb.h>
#include <linux/display-sys.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_HDMI_DEBUG
#define hdmi_dbg(dev, format, arg...)		\
	dev_printk(KERN_INFO , dev , format , ## arg)
#else
#define hdmi_dbg(dev, format, arg...)	
#endif

#define TRUE		1
#define FALSE 		0

/* HDMI video mode code according CEA-861-E*/
enum hdmi_video_mode
{
	HDMI_640x480p_60Hz = 1,
	HDMI_720x480p_60Hz_4_3,
	HDMI_720x480p_60Hz_16_9,
	HDMI_1280x720p_60Hz,
	HDMI_1920x1080i_60Hz,		//5
	HDMI_720x480i_60Hz_4_3,
	HDMI_720x480i_60Hz_16_9,
	HDMI_720x240p_60Hz_4_3,
	HDMI_720x240p_60Hz_16_9,
	HDMI_2880x480i_60Hz_4_3,	//10
	HDMI_2880x480i_60Hz_16_9,
	HDMI_2880x240p_60Hz_4_3,
	HDMI_2880x240p_60Hz_16_9,
	HDMI_1440x480p_60Hz_4_3,
	HDMI_1440x480p_60Hz_16_9,	//15
	HDMI_1920x1080p_60Hz,
	HDMI_720x576p_50Hz_4_3,
	HDMI_720x576p_50Hz_16_9,
	HDMI_1280x720p_50Hz,
	HDMI_1920x1080i_50Hz,		//20
	HDMI_720x576i_50Hz_4_3,
	HDMI_720x576i_50Hz_16_9,
	HDMI_720x288p_50Hz_4_3,
	HDMI_720x288p_50Hz_16_9,
	HDMI_2880x576i_50Hz_4_3,	//25
	HDMI_2880x576i_50Hz_16_9,
	HDMI_2880x288p_50Hz_4_3,
	HDMI_2880x288p_50Hz_16_9,
	HDMI_1440x576p_50Hz_4_3,
	HDMI_1440x576p_50Hz_16_9,	//30
	HDMI_1920x1080p_50Hz,
	HDMI_1920x1080p_24Hz,
	HDMI_1920x1080p_25Hz,
	HDMI_1920x1080p_30Hz,
	HDMI_2880x480p_60Hz_4_3,	//35
	HDMI_2880x480p_60Hz_16_9,
	HDMI_2880x576p_50Hz_4_3,
	HDMI_2880x576p_50Hz_16_9,
	HDMI_1920x1080i_50Hz_2,		// V Line 1250 total
	HDMI_1920x1080i_100Hz,		//40
	HDMI_1280x720p_100Hz,
	HDMI_720x576p_100Hz_4_3,
	HDMI_720x576p_100Hz_16_9,
	HDMI_720x576i_100Hz_4_3,
	HDMI_720x576i_100Hz_16_9,	//45
	HDMI_1920x1080i_120Hz,
	HDMI_1280x720p_120Hz,
	HDMI_720x480p_120Hz_4_3,
	HDMI_720x480p_120Hz_16_9,	
	HDMI_720x480i_120Hz_4_3,	//50
	HDMI_720x480i_120Hz_16_9,
	HDMI_720x576p_200Hz_4_3,
	HDMI_720x576p_200Hz_16_9,
	HDMI_720x576i_200Hz_4_3,
	HDMI_720x576i_200Hz_16_9,	//55
	HDMI_720x480p_240Hz_4_3,
	HDMI_720x480p_240Hz_16_9,	
	HDMI_720x480i_240Hz_4_3,
	HDMI_720x480i_240Hz_16_9,
	HDMI_1280x720p_24Hz,		//60
	HDMI_1280x720p_25Hz,
	HDMI_1280x720p_30Hz,
	HDMI_1920x1080p_120Hz,
	HDMI_1920x1080p_100Hz,
};

/* HDMI STATUS */
#define HDMI_DISABLE	0
#define HDMI_ENABLE		1
#define HDMI_UNKOWN		0xFF

/* HDMI auto switch */
#define HDMI_AUTO_SWITCH		HDMI_ENABLE

/* HDMI anto config */
#define HDMI_AUTO_CONFIG		HDMI_ENABLE

/* HDMI HDCP CONFIG */
#ifdef CONFIG_HDMI_HDCP_ENABLE
#define HDMI_HDCP_CONFIG  		HDMI_ENABLE
#else
#define HDMI_HDCP_CONFIG  		HDMI_DISABLE
#endif

/* HDMI max video timing mode */
#define HDMI_MAX_VIDEO_MODE	65

/* HDMI default resolution */
#define HDMI_DEFAULT_RESOLUTION HDMI_1280x720p_60Hz
//#define HDMI_DEFAULT_RESOLUTION HDMI_1920x1080p_50Hz
enum hdmi_video_color_mode
{
	HDMI_VIDEO_RGB = 0,
	HDMI_VIDEO_YCbCr422,
	HDMI_VIDEO_YCbCr444
};

/* HDMI default color */
#define HDMI_DEFAULT_COLOR	HDMI_VIDEO_RGB

/* HDMI Audio type */
enum hdmi_audio_type
{
	HDMI_AUDIO_LPCM = 1,
	HDMI_AUDIO_AC3,
	HDMI_AUDIO_MPEG1,
	HDMI_AUDIO_MP3,
	HDMI_AUDIO_MPEG2,
	HDMI_AUDIO_AAC_LC,		//AAC
	HDMI_AUDIO_DTS,
	HDMI_AUDIO_ATARC,
	HDMI_AUDIO_DSD,			//One bit Audio
	HDMI_AUDIO_E_AC3,
	HDMI_AUDIO_DTS_HD,
	HDMI_AUDIO_MLP,
	HDMI_AUDIO_DST,
	HDMI_AUDIO_WMA_PRO
};

/* I2S Fs */
enum hdmi_audio_fs {
	HDMI_AUDIO_FS_32000  = 0x1,
	HDMI_AUDIO_FS_44100  = 0x2,
	HDMI_AUDIO_FS_48000  = 0x4,
	HDMI_AUDIO_FS_88200  = 0x8,
	HDMI_AUDIO_FS_96000  = 0x10,
	HDMI_AUDIO_FS_176400 = 0x20,
	HDMI_AUDIO_FS_192000 = 0x40
};

/* Audio Word Length */
enum hdmi_audio_word_length {
	HDMI_AUDIO_WORD_LENGTH_16bit = 0x1,
	HDMI_AUDIO_WORD_LENGTH_20bit = 0x2,
	HDMI_AUDIO_WORD_LENGTH_24bit = 0x4
};

/* Audio default type */
#define HDMI_AUDIO_DEFAULT_TYPE	HDMI_AUDIO_LPCM

/* Audio default channel number */
#define HDMI_AUDIO_DEFAULT_CHANNEL_NUM	2

/* Audio default sample rate */
#define HDMI_AUDIO_DEFAULT_Fs HDMI_AUDIO_FS_44100

/* Audio default audio word length */
#define HDMI_AUDIO_DEFAULT_WORD_LENGTH HDMI_AUDIO_WORD_LENGTH_16bit

/* HDMI Color BIT */
#define HDMI_DEEP_COLOR_48BIT	1<<3;
#define HDMI_DEEP_COLOR_36BIT	1<<2;
#define HDMI_DEEP_COLOR_30BIT	1<<1;
#define HDMI_DEEP_COLOR_Y444	1<<0;

struct hdmi_edid {
	unsigned char is_hdmi;				//HDMI display device flag
	unsigned char ycbcr444;				//Display device support YCbCr444
	unsigned char ycbcr422;				//Display device support YCbCr422
	unsigned char deepcolor;			//bit3:DC_48bit; bit2:DC_36bit; bit1:DC_30bit; bit0:DC_Y444;
	struct fb_monspecs	*specs;			//Device spec
	struct list_head modelist;			//Device supported display mode list
	struct hdmi_audio *audio;			//Device supported audio info
	int	audio_num;						//Device supported audio type number
};

/* HDMI audio parameters */
struct hdmi_audio {
	u32 type;							//Audio type
	u32	channel;						//Audio channel number
	u32	rate;							//Audio sampling rate
	u32	word_length;					//Audio data word length
};

/* HDMI Configuration */
struct hdmi_configs {
	u8		display_on;					//HDMI video displaying flag
	u8		hdcp_on;					//HDMI HDCP function enable flag	
	u8		resolution;					//HDMI video mode code
	u8		color;						//HDMI video color mode: RGB, YCbCr422 or YCbCr444
	struct hdmi_audio	audio;
};

struct hdmi;

/* HDMI transmitter control functions*/
struct hdmi_ops {
	int (*init)(struct hdmi *);								//initialize transmitter, needed
	int (*insert)(struct hdmi*);							//process when HDMI inserted, optional
	int (*remove)(struct hdmi*);							//process when HDMI removeed, optional
	int (*detect_hpd)(struct hdmi *, int *);				//detect HDMI hotplug, needed
	int (*detect_sink)(struct hdmi *, int *);				//detect sink active status, needed
	int (*read_edid)(struct hdmi *, int , unsigned char *); //read one section edid data, needed
	int (*config_video)(struct hdmi *, int, int, int);		//configure video, needed
	int (*config_audio)(struct hdmi *, struct hdmi_audio *);		//configure audio, needed
	int (*config_hdcp)(struct hdmi *, int);					//configure HDCP, optional
	int (*enable)(struct hdmi*, int);						//enable HDMI signal output, needed
};

/* Max HDMI device id */
#define HDMI_MAX_ID	4

struct hdmi {
	int id;
	struct rk_display_device *ddev;
	struct device *dev;
	void *priv;
		
	struct workqueue_struct *workqueue;
	struct delayed_work delay_work;	
	int	rate;
	
	int wait;
	struct completion	complete;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend	early_suspend;
#endif	
	int		video_source;
	int		property;
	int		enable;
	int		auto_switch;
	int		hpd_status;
	int		auto_config;
	int		param_config;
	int		support_r2y;	// support RGB to YCbCr convert.
	
	struct hdmi_configs config_set;
	struct hdmi_configs config_real;

	struct hdmi_edid	edid;	
	struct hdmi_ops		*ops;
};

/* HDMI Error Code */
enum hdmi_errorcode
{
	HDMI_ERROR_SUCESS = 0,
	HDMI_ERROR_FALSE,
	HDMI_ERROR_I2C,
	HDMI_ERROR_EDID,
};

extern struct hdmi* hdmi_register(struct device *parent, struct hdmi_ops *ops, int video_source, int property);
extern void hdmi_unregister(struct hdmi *hdmi);
extern int hdmi_config_audio(struct hdmi_audio	*audio);
extern struct hdmi *get_hdmi_struct(int id);
#endif /*__LINUX_HDMI_ITV_CORE_H*/