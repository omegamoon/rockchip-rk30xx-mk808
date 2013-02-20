#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <linux/i2c.h>
#include "it66121.h"

struct it66121 it66121;

#ifdef HDMI_USE_IRQ
static irqreturn_t it66121_detect_irq(int irq, void *dev_id)
{
	struct hdmi *hdmi = (struct hdmi *)dev_id;

	hdmi_changed(hdmi, 1);
    return IRQ_HANDLED;
}
#endif

static struct hdmi_ops it66121_ops = {
	.init = it66121_sys_init,
	.insert = it66121_sys_insert,
	.remove = it66121_sys_remove,
	.detect_hpd = it66121_sys_poll_status,
	.read_edid = it66121_sys_read_edid,
	.config_video = it66121_sys_config_video,
	.config_audio = it66121_sys_config_audio,
	.config_hdcp = it66121_sys_config_hdcp,
	.enable = it66121_sys_enalbe_output,
};

static int it66121_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int rc = 0;
	struct hdmi *hdmi = NULL;
	struct rkdisplay_platform_data *hdmi_data = client->dev.platform_data;
	
	memset(&it66121, 0, sizeof(struct it66121));
	it66121.client = client;
	
	// Register HDMI device
	if(hdmi_data) {
		it66121.io_pwr_pin = hdmi_data->io_pwr_pin;
		it66121.io_rst_pin = hdmi_data->io_reset_pin;
		hdmi = hdmi_register(&client->dev, &it66121_ops, hdmi_data->video_source, hdmi_data->property);
	}
	else {
		it66121.io_pwr_pin = INVALID_GPIO;
		it66121.io_rst_pin = INVALID_GPIO;	
		hdmi = hdmi_register(&client->dev, &it66121_ops, DISPLAY_SOURCE_LCDC0, DISPLAY_MAIN);
	}
	if(hdmi == NULL)
	{
		dev_err(&client->dev, "fail to register hdmi\n");
		goto err_hdmi_register;
	}	
	it66121.hdmi = hdmi;
	hdmi->priv = &it66121;

	//Power on IT66121	
	if(it66121.io_pwr_pin != INVALID_GPIO) {
		rc = gpio_request(it66121.io_pwr_pin, NULL);
		if(rc) {
			gpio_free(it66121.io_pwr_pin);
        	printk(KERN_ERR "request it66121 power control gpio error\n ");
        	goto err_request_gpio2; 
		}
		else
			gpio_direction_output(it66121.io_pwr_pin, GPIO_HIGH);
	}
	
	if(it66121.io_rst_pin != INVALID_GPIO) {
		rc = gpio_request(it66121.io_rst_pin, NULL);
		if(rc) {
			gpio_free(it66121.io_rst_pin);
        	printk(KERN_ERR "request it66121 reset control gpio error\n ");
        	goto err_request_gpio1; 
		}
		else {
			gpio_direction_output(it66121.io_rst_pin, GPIO_LOW);
			msleep(10);
			gpio_direction_output(it66121.io_rst_pin, GPIO_HIGH);
		}
	}
	
	if(it66121_initial())
		goto err_request_gpio;
	hdmi_enable(hdmi);
	HDMI_task(hdmi);
	#ifdef HDMI_USE_IRQ
	hdmi_changed(hdmi, 1);
	if((rc = gpio_request(client->irq, "hdmi gpio")) < 0)
    {
        dev_err(&client->dev, "fail to request gpio %d\n", client->irq);
        goto err_request_gpio;
    }
    it66121.irq = gpio_to_irq(client->irq);
	it66121.io_irq_pin = client->irq;
    gpio_direction_input(client->irq);
    if((rc = request_irq(it66121.irq, it66121_detect_irq,IRQF_TRIGGER_FALLING,NULL,hdmi)) <0)
    {
        dev_err(&client->dev, "fail to request hdmi irq\n");
        goto err_request_irq;
    }
	#else
	queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 200);
	#endif
	printk(KERN_INFO "IT66121 probe success.");	
	return 0;
	
err_request_irq:
	gpio_free(client->irq);
err_request_gpio:
	if(it66121.io_rst_pin != INVALID_GPIO)
		gpio_free(it66121.io_rst_pin);
err_request_gpio1:
	if(it66121.io_pwr_pin != INVALID_GPIO)
		gpio_free(it66121.io_pwr_pin);
err_request_gpio2:
	hdmi_unregister(hdmi);
err_hdmi_register:
	hdmi = NULL;
	dev_err(&client->dev, "it66121 probe error\n");
	return rc;
}

static int __devexit it66121_i2c_remove(struct i2c_client *client)
{
	#ifdef HDMI_USE_IRG
	free_irq(it66121.irq, NULL);
	gpio_free(client.irq);
	#endif
	hdmi_unregister(it66121.hdmi);
	kfree(it66121.hdmi);
	it66121.hdmi = NULL;
    return 0;
}

static const struct i2c_device_id it66121_id[] = {
	{ "it66121", 0 },
	{ }
};

static struct i2c_driver it66121_i2c_driver = {
    .driver = {
        .name  = "it66121",
        .owner = THIS_MODULE,
    },
    .probe      = &it66121_i2c_probe,
    .remove     = &it66121_i2c_remove,
    .id_table	= it66121_id,
};

static int __init it66121_init(void)
{
    return i2c_add_driver(&it66121_i2c_driver);
}

static void __exit it66121_exit(void)
{
    i2c_del_driver(&it66121_i2c_driver);
}

//module_init(it66121_init);
fs_initcall(it66121_init);
module_exit(it66121_exit);
