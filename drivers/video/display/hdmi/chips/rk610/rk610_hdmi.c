#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <linux/i2c.h>
#include "rk610_hdmi.h"

struct rk610_hdmi_pdata *rk610_hdmi = NULL;

int rk610_hdmi_register_hdcp_callbacks(void (*hdcp_cb)(void),
					 void (*hdcp_irq_cb)(void),
					 int (*hdcp_power_on_cb)(void),
					 void (*hdcp_power_off_cb)(void))
{
	rk610_hdmi->hdcp_cb = hdcp_cb;
	rk610_hdmi->hdcp_irq_cb = hdcp_irq_cb;
	rk610_hdmi->hdcp_power_on_cb = hdcp_power_on_cb;
	rk610_hdmi->hdcp_power_off_cb = hdcp_power_off_cb;
	
	return HDMI_ERROR_SUCESS;
}

#ifdef HDMI_USE_IRQ
static void rk610_irq_work_func(struct work_struct *work)
{
	rk610_hdmi_interrupt();
	if(rk610_hdmi->hdcp_irq_cb)
		rk610_hdmi->hdcp_irq_cb();
}

static irqreturn_t rk610_irq(int irq, void *dev_id)
{
	printk(KERN_INFO "rk610 irq triggered.\n");
	schedule_work(&rk610_hdmi->irq_work);
    return IRQ_HANDLED;
}
#endif

static struct hdmi_ops rk610_hdmi_ops = {
	.init = rk610_hdmi_sys_init,
	.insert = rk610_hdmi_sys_insert,
	.remove = rk610_hdmi_sys_remove,
	.detect_hpd = rk610_hdmi_sys_detect_hpd,
//	.detect_sink = rk610_hdmi_sys_detect_sink,
	.read_edid = rk610_hdmi_sys_read_edid,
	.config_video = rk610_hdmi_sys_config_video,
	.config_audio = rk610_hdmi_sys_config_audio,
//	.config_hdcp = rk610_hdmi_sys_config_hdcp,
	.enable = rk610_hdmi_sys_enalbe_output,
};

static int rk610_hdmi_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int rc = 0;
	struct hdmi *hdmi = NULL;
	struct rkdisplay_platform_data *hdmi_data = client->dev.platform_data;
	
	rk610_hdmi = kzalloc(sizeof(struct rk610_hdmi_pdata), GFP_KERNEL);
	if(!rk610_hdmi)
	{
        dev_err(&client->dev, "no memory for state\n");
        goto err_kzalloc_rk610_hdmi;
    }
	rk610_hdmi->client = client;
	
	if(hdmi_data)
		hdmi = hdmi_register(&client->dev, &rk610_hdmi_ops, hdmi_data->video_source, hdmi_data->property);
	else
		hdmi = hdmi_register(&client->dev, &rk610_hdmi_ops, DISPLAY_SOURCE_LCDC0, DISPLAY_MAIN);
	if(hdmi == NULL)
	{
		dev_err(&client->dev, "fail to register hdmi\n");
		goto err_hdmi_register;
	}
	hdmi->support_r2y = 1;
	rk610_hdmi->hdmi = hdmi;
	hdmi_set_privdata(hdmi, rk610_hdmi);
	i2c_set_clientdata(client, rk610_hdmi);
	
	{
		#ifdef HDMI_USE_IRQ
//		hdmi_changed(hdmi, 0);
		INIT_WORK(&rk610_hdmi->irq_work, rk610_irq_work_func);
		if((rc = gpio_request(client->irq, "hdmi gpio")) < 0)
	    {
	        dev_err(&client->dev, "fail to request gpio %d\n", client->irq);
	        goto err_request_gpio;
	    }
	    rk610_hdmi->irq = gpio_to_irq(client->irq);
		rk610_hdmi->gpio = client->irq;
	    gpio_pull_updown(client->irq,GPIOPullUp);
	    gpio_direction_input(client->irq);
	    if((rc = request_irq(rk610_hdmi->irq, rk610_irq,IRQF_TRIGGER_RISING,NULL,hdmi)) < 0)
	    {
	        dev_err(&client->dev, "fail to request hdmi irq\n");
	        goto err_request_irq;
	    }
		#else
		HDMI_task(hdmi);
		queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 200);
		#endif
		hdmi_enable(hdmi);
		dev_info(&client->dev, "rk610 hdmi i2c probe ok\n");
	}
	
    return 0;
	
err_request_irq:
	gpio_free(client->irq);
err_request_gpio:
	hdmi_unregister(hdmi);
err_hdmi_register:
	kfree(rk610_hdmi);
	rk610_hdmi = NULL;
err_kzalloc_rk610_hdmi:
	hdmi = NULL;
	dev_err(&client->dev, "rk610 hdmi probe error\n");
	return rc;

}

static int __devexit rk610_hdmi_i2c_remove(struct i2c_client *client)
{
	struct rk610_hdmi_pdata *rk610_hdmi = (struct rk610_hdmi_pdata *)i2c_get_clientdata(client);
	struct hdmi *hdmi = rk610_hdmi->hdmi;
	
	#ifdef HDMI_USE_IRG
	free_irq(rk610_hdmi->irq, NULL);
	gpio_free(client->irq);
	#endif
	hdmi_unregister(hdmi);
	kfree(rk610_hdmi);
	rk610_hdmi = NULL;
	kfree(hdmi);
	hdmi = NULL;
		
	hdmi_dbg(hdmi->dev, "%s\n", __func__);
    return 0;
}

static const struct i2c_device_id rk610_hdmi_id[] = {
	{ "rk610_hdmi", 0 },
	{ }
};

static struct i2c_driver rk610_hdmi_i2c_driver = {
    .driver = {
        .name  = "rk610_hdmi",
        .owner = THIS_MODULE,
    },
    .probe      = &rk610_hdmi_i2c_probe,
    .remove     = &rk610_hdmi_i2c_remove,
    .id_table	= rk610_hdmi_id,
};

static int __init rk610_hdmi_init(void)
{
    return i2c_add_driver(&rk610_hdmi_i2c_driver);
}

static void __exit rk610_hdmi_exit(void)
{
    i2c_del_driver(&rk610_hdmi_i2c_driver);
}

//late_initcall(rk610_hdmi_init);
module_init(rk610_hdmi_init);
//fs_initcall(rk610_init);
module_exit(rk610_hdmi_exit);
