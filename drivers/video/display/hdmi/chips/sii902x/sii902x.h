#ifndef _SII902X_H
#define _SII902X_H

#include <linux/hdmi-itv.h>
#include "../../hdmi_local.h"
#include "siHdmiTx_902x_TPI.h"

#ifndef INVALID_GPIO
#define INVALID_GPIO	-1
#endif

struct sii902x_dev_s{
	struct i2c_driver *i2c_driver;
	int detected;
	int hdcp_supported;
	struct hdmi *hdmi;
};

struct sii902x_pdata {
	int irq;
	int io_irq_pin;
	int io_pwr_pin;
	int io_rst_pin;
	int init;
	struct i2c_client *client;
	struct sii902x_dev_s dev;
};

extern struct sii902x_pdata *sii902x;

extern int sii902x_detect_device(struct sii902x_pdata *anx);
extern int sii902x_sys_init(struct hdmi *hdmi);
extern int sii902x_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus);
extern int sii902x_sys_insert(struct hdmi *hdmi);
extern int sii902x_sys_remove(struct hdmi *hdmi);
extern int sii902x_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff);
extern int sii902x_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color);
extern int sii902x_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio);
extern int sii902x_sys_config_hdcp(struct hdmi *hdmi, int enable);
extern int sii902x_sys_enalbe_output(struct hdmi *hdmi, int enable);

#endif