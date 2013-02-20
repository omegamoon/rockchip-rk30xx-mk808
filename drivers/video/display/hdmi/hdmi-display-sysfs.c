#include <linux/ctype.h>
#include <linux/string.h>
#include "hdmi_local.h"

static int hdmi_get_enable(struct rk_display_device *device)
{
	struct hdmi *hdmi = device->priv_data;

	return hdmi->config_set.display_on;
}

static int hdmi_set_enable(struct rk_display_device *device, int enable)
{
	struct hdmi *hdmi = device->priv_data;
	
	if(hdmi->config_set.display_on == enable)
		return 0;
	hdmi->config_set.display_on = enable;
	hdmi->param_config = HDMI_CONFIG_DISPLAY;
	#ifdef HDMI_USE_IRQ
	hdmi_changed(hdmi, 1);
	#endif
	return 0;
//	init_completion(&hdmi->complete);
//	hdmi->wait = 1;
//	return wait_for_completion_interruptible_timeout(&hdmi->complete,
//							msecs_to_jiffies(10000));
}

static int hdmi_get_status(struct rk_display_device *device)
{
	struct hdmi *hdmi = device->priv_data;
	if(hdmi->hpd_status > HDMI_RECEIVER_REMOVE)
		return 1;
	else
		return 0;
}

static int hdmi_get_modelist(struct rk_display_device *device, struct list_head **modelist)
{
	struct hdmi *hdmi = device->priv_data;
	*modelist = &hdmi->edid.modelist;
	return 0;
}

static int hdmi_set_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
	struct hdmi *hdmi = device->priv_data;
	int vic = ext_hdmi_videomode_to_vic(mode);
	
	if(!hdmi->hpd_status)
		return -1;
	
	if(vic && hdmi->config_set.resolution != vic)
	{
		hdmi->config_set.resolution = vic;
		hdmi->auto_config = HDMI_DISABLE;
		hdmi->param_config = HDMI_CONFIG_VIDEO;
		init_completion(&hdmi->complete);
		hdmi->wait = 1;
		#ifdef HDMI_USE_IRQ
		hdmi_changed(hdmi, 1);
		#endif
		wait_for_completion_interruptible_timeout(&hdmi->complete,
								msecs_to_jiffies(10000));
	}
	return 0;
}

static int hdmi_get_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
	struct hdmi *hdmi = device->priv_data;
	struct fb_videomode *vmode;
	int vic;
//	if(!hdmi->hpd_status)
//		return -1;
	vic = hdmi->config_real.resolution;
	if(vic == HDMI_UNKOWN)
		vic = HDMI_DEFAULT_RESOLUTION;
	vmode = (struct fb_videomode*) ext_hdmi_vic_to_videomode(vic);
	if(unlikely(vmode == NULL))
		return -1;
	*mode = *vmode;
	return 0;
}

static struct rk_display_ops hdmi_display_ops = {
	.setenable = hdmi_set_enable,
	.getenable = hdmi_get_enable,
	.getstatus = hdmi_get_status,
	.getmodelist = hdmi_get_modelist,
	.setmode = hdmi_set_mode,
	.getmode = hdmi_get_mode,
};

static int hdmi_display_probe(struct rk_display_device *device, void *devdata)
{
	struct hdmi* hdmi = devdata;
	device->owner = THIS_MODULE;
	strcpy(device->type, "HDMI");
	device->name = "hdmi-transmitter";
	device->priority = DISPLAY_PRIORITY_HDMI;
	device->property = hdmi->property;
	device->priv_data = devdata;
	device->ops = &hdmi_display_ops;
	printk("hdmi->property is %d\n", hdmi->property);
	return 1;
}

static struct rk_display_driver display_hdmi = {
	.probe = hdmi_display_probe,
};

struct rk_display_device* ext_hdmi_register_display_sysfs(struct hdmi *hdmi, struct device *parent)
{
	return rk_display_device_register(&display_hdmi, parent, hdmi);
}

void ext_hdmi_unregister_display_sysfs(struct hdmi *hdmi)
{
	if(hdmi->ddev)
		rk_display_device_unregister(hdmi->ddev);
}