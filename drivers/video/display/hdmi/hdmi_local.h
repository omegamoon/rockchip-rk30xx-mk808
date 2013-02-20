#ifndef _HDMI_LOCAL_H
#define _HDMI_LOCAL_H

#include <linux/fb.h>
#include <linux/hdmi-itv.h>

#ifdef CONFIG_HDMI_USE_IRQ
#define HDMI_USE_IRQ
#endif

/* HDMI HPD Status */
enum hdmi_status {
	HDMI_RECEIVER_REMOVE = 0,
	HDMI_RECEIVER_INACTIVE,
	HDMI_RECEIVER_ACTIVE
};

/* HDMI Display Status */
enum hdmi_display_status {
	HDMI_DISPLAY_OFF = 0,
	HDMI_DISPLAY_ON,
	HDMI_DISPLAY_HOLD
};

// HDMI state machine
enum hdmi_state{
	HDMI_UNKNOWN = 0,
	HDMI_INITIAL,
	WAIT_HOTPLUG,
	READ_PARSE_EDID,
	WAIT_RX_SENSE,
	WAIT_HDMI_ENABLE,
	SYSTEM_CONFIG,
	CONFIG_VIDEO,
	CONFIG_AUDIO,
	CONFIG_PACKETS,
	HDCP_AUTHENTICATION,
	PLAY_BACK,
	RESET_LINK,
	HDMI_UNPLUG,
};

// HDMI configuration command
enum hdmi_change {
	HDMI_CONFIG_NONE = 0,
	HDMI_CONFIG_VIDEO,
	HDMI_CONFIG_AUDIO,
	HDMI_CONFIG_COLOR,
	HDMI_CONFIG_HDCP,
	HDMI_CONFIG_ENABLE,
	HDMI_CONFIG_DISPLAY
};

struct hdmi_video_timing {
	unsigned int VIC;
	unsigned int OUT_CLK;
	unsigned int H_FP;
	unsigned int H_PW;
	unsigned int H_BP;
	unsigned int H_VD;
	unsigned int V_FP;
	unsigned int V_PW;
	unsigned int V_BP;
	unsigned int V_VD;
	unsigned int PorI;
	unsigned int Polarity;
};

extern void *hdmi_get_privdata(struct hdmi *hdmi);
extern void hdmi_set_privdata(struct hdmi *hdmi, void *data);
extern int HDMI_EDID_parse_base(unsigned char *buf, int *extend_num, struct hdmi_edid *pedid);
extern int HDMI_EDID_parse_extensions(unsigned char *buf, struct hdmi_edid *pedid);
extern const char *ext_hdmi_get_video_mode_name(unsigned char vic);
extern int ext_hdmi_videomode_to_vic(struct fb_videomode *vmode);
extern const struct fb_videomode* ext_hdmi_vic_to_videomode(int vic);
extern int ext_hdmi_add_videomode(const struct fb_videomode *mode, struct list_head *head);
extern int HDMI_task(struct hdmi *hdmi);
extern void hdmi_changed(struct hdmi *hdmi, unsigned int msec);
extern int hdmi_enable(struct hdmi *hdmi);
extern struct hdmi_video_timing * ext_hdmi_find_mode(int vic);
extern int ext_hdmi_switch_fb(struct hdmi *hdmi, int type);
extern int ext_hdmi_find_best_mode(struct hdmi* hdmi, int vic);
extern int ext_hdmi_ouputmode_select(struct hdmi *hdmi, int edid_ok);
extern int hdmi_task_interval(void);
extern struct rk_display_device* ext_hdmi_register_display_sysfs(struct hdmi *hdmi, struct device *parent);
extern void ext_hdmi_init_modelist(struct hdmi *hdmi);
#endif
