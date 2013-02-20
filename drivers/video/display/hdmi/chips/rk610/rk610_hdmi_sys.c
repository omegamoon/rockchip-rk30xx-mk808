#include "rk610_hdmi.h"
#include "rk610_hdmi_hw.h"

int rk610_hdmi_sys_init(struct hdmi *hdmi)
{
	struct rk610_hdmi_pdata *rk610_hdmi = hdmi_get_privdata(hdmi);
	return Rk610_hdmi_init(rk610_hdmi->client);
}

int rk610_hdmi_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus)
{
	struct rk610_hdmi_pdata *rk610_hdmi = hdmi_get_privdata(hdmi);
	*hpdstatus = Rk610_hdmi_hpd(rk610_hdmi->client);
	return 0;
}

int rk610_hdmi_sys_insert(struct hdmi *hdmi);
int rk610_hdmi_sys_remove(struct hdmi *hdmi);

int rk610_hdmi_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff)
{
	return HDMI_ERROR_FALSE;
}

int rk610_hdmi_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color)
{
	return Rk610_hdmi_Set_Video(vic);
}

int rk610_hdmi_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio)
{
	return 0;
}

int rk610_hdmi_sys_config_hdcp(struct hdmi *hdmi, int enable);

int rk610_hdmi_sys_enalbe_output(struct hdmi *hdmi, int enable)
{
	struct rk610_hdmi_pdata *rk610_hdmi = hdmi_get_privdata(hdmi);
	return Rk610_hdmi_Config_Done(rk610_hdmi->client);
}