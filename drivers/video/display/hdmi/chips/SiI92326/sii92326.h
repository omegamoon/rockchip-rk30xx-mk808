#ifndef _SII92326_H
#define _SII92326_H

#include <linux/hdmi-itv.h>
#include "../../hdmi_local.h"
#include "sii_92326_driver.h"
#include <mach/gpio.h>

struct sii92326_dev_s{
	struct i2c_driver *i2c_driver;
	int detected;
	struct hdmi *hdmi;
};

struct sii92326_pdata {
	int irq;
	int io_irq_pin;
	int io_pwr_pin;
	int io_rst_pin;
	struct i2c_client *client;
	struct i2c_client *client_page1;
	struct i2c_client *client_page2;
	struct i2c_client *client_cbus;
	struct i2c_client *client_edid;
	struct i2c_client *client_segedid;
	struct i2c_client *client_hdcp;
	struct sii92326_dev_s dev;
};

extern struct sii92326_pdata *sii92326;

extern void SiI92326_Reset(void);

extern int sii92326_detect_device(struct sii92326_pdata *anx);
extern int sii92326_sys_init(struct hdmi *hdmi);
extern int sii92326_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus);
extern int sii92326_sys_insert(struct hdmi *hdmi);
extern int sii92326_sys_remove(struct hdmi *hdmi);
extern int sii92326_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff);
extern int sii92326_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color);
extern int sii92326_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio);
extern int sii92326_sys_config_hdcp(struct hdmi *hdmi, int enable);
extern int sii92326_sys_enalbe_output(struct hdmi *hdmi, int enable);
extern int sii92326_sys_detect_sink(struct hdmi *hdmi, int *sinkstatus);
#endif