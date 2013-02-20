#ifndef _ANX7150_H
#define _ANX7150_H

#include <linux/hdmi-itv.h>
#include "../../hdmi_local.h"

#ifndef CONFIG_ANX7150_IRQ_USE_CHIP
#define ANX7150_I2C_ADDR0		0X39
#define ANX7150_I2C_ADDR1		0X3d
#else
#define ANX7150_I2C_ADDR0		0X3b
#define ANX7150_I2C_ADDR1		0X3f
#endif

#define ANX7150_SCL_RATE		100 * 1000

struct anx7150_dev_s{
	struct i2c_driver *i2c_driver;
	int anx7150_detect;
	struct hdmi *hdmi;
};

struct anx7150_pdata {
	int irq;
	int io_irq_pin;
	int io_pwr_pin;
	int io_rst_pin;
	int init;
	struct i2c_client *client;
	struct anx7150_dev_s dev;
};


/* I2C interface */
int anx7150_i2c_read_p0_reg(struct i2c_client *client, char reg, char *val);
int anx7150_i2c_write_p0_reg(struct i2c_client *client, char reg, char *val);
int anx7150_i2c_read_p1_reg(struct i2c_client *client, char reg, char *val);
int anx7150_i2c_write_p1_reg(struct i2c_client *client, char reg, char *val);

/* HDMI system control interface */
int ANX7150_sys_init(struct hdmi *hdmi);
int ANX7150_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus);
int ANX7150_sys_plug(struct hdmi *hdmi);
int ANX7150_sys_detect_sink(struct hdmi *hdmi, int *sink_status);
int ANX7150_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff);
int ANX7150_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color);
int ANX7150_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio);
int ANX7150_sys_config_hdcp(struct hdmi *hdmi, int enable);
int ANX7150_sys_enalbe_output(struct hdmi *hdmi, int enable);
#endif
