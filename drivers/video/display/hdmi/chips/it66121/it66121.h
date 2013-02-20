#ifndef __IT66121_H__
#define __IT66121_H__
#include <linux/hdmi-itv.h>
#include <linux/delay.h>
#include "../../hdmi_local.h"
#include "hdmitx.h"
#include "hdmitx_sys.h"

#define IT66121_I2C_RATE	100 * 1000
#define delay1ms	msleep

#ifdef CONFIG_HDMI_DEBUG
#define DBG(format, ...) \
		printk(KERN_INFO "IT66121: " format "\n", ## __VA_ARGS__)
#define HDMITX_DEBUG_PRINTF(x)  DBG x
#define HDMITX_DEBUG_PRINTF1(x)	//DBG x
#define HDMITX_DEBUG_PRINTF2(x) //DBG x
#define HDMITX_DEBUG_PRINTF3(x) //DBG x

#define HDCP_DEBUG_PRINTF(x)   //DBG x
#define HDCP_DEBUG_PRINTF1(x)  //DBG x
#define HDCP_DEBUG_PRINTF2(x)  //DBG x
#define HDCP_DEBUG_PRINTF3(x)  //DBG x
#else
#define DBG(format, ...)
#define HDMITX_DEBUG_PRINTF(x)  
#define HDMITX_DEBUG_PRINTF1(x)
#define HDMITX_DEBUG_PRINTF2(x)
#define HDMITX_DEBUG_PRINTF3(x) // printf x

#define HDCP_DEBUG_PRINTF(x)
#define HDCP_DEBUG_PRINTF1(x)
#define HDCP_DEBUG_PRINTF2(x)
#define HDCP_DEBUG_PRINTF3(x)
#endif

struct it66121 {
	struct i2c_client *client;
	struct hdmi *hdmi;

	int irq;
	int io_irq_pin;
	int io_pwr_pin;
	int io_rst_pin;
	
	unsigned long tmdsclk;
};

extern struct it66121 it66121;

extern int it66121_initial(void);
extern int it66121_sys_init(struct hdmi *hdmi);
extern int it66121_sys_insert(struct hdmi *hdmi);
extern int it66121_sys_remove(struct hdmi *hdmi);
extern int it66121_sys_poll_status(struct hdmi *hdmi, int *status);
extern int it66121_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff);
extern int it66121_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color);
extern int it66121_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio);
extern int it66121_sys_config_hdcp(struct hdmi *hdmi, int enable);
extern int it66121_sys_enalbe_output(struct hdmi *hdmi, int enable);

#endif
