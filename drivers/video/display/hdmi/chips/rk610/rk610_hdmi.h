#ifndef __RK610_HDMI_H__
#define __RK610_HDMI_H__
#include <linux/hdmi-itv.h>
#include "../../hdmi_local.h"

struct rk610_hdmi_dev_s{

};

struct rk610_hdmi_pdata {
	int irq;
	int gpio;
	struct i2c_client *client;
	struct hdmi *hdmi;
	struct work_struct	irq_work;
	int	frequency;		//tmds clk frequncy
	// call back for hdcp operatoion
	void (*hdcp_cb)(void);
	void (*hdcp_irq_cb)(void);
	int (*hdcp_power_on_cb)(void);
	void (*hdcp_power_off_cb)(void);
};

extern struct rk610_hdmi_pdata *rk610_hdmi;

extern int rk610_hdmi_sys_init(struct hdmi *hdmi);
extern void rk610_hdmi_interrupt(void);
extern int rk610_hdmi_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus);
extern int rk610_hdmi_sys_insert(struct hdmi *hdmi);
extern int rk610_hdmi_sys_remove(struct hdmi *hdmi);
extern int rk610_hdmi_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff);
extern int rk610_hdmi_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color);
extern int rk610_hdmi_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio);
extern int rk610_hdmi_sys_config_hdcp(struct hdmi *hdmi, int enable);
extern int rk610_hdmi_sys_enalbe_output(struct hdmi *hdmi, int enable);
extern int rk610_hdmi_register_hdcp_callbacks(void (*hdcp_cb)(void),
					 void (*hdcp_irq_cb)(void),
					 int (*hdcp_power_on_cb)(void),
					 void (*hdcp_power_off_cb)(void));
#endif
