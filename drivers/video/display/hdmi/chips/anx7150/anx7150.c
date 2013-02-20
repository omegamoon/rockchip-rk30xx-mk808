#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <mach/board.h>

#include "anx7150.h"
#include "anx7150_hw.h"

#ifdef HDMI_USE_IRQ
static irqreturn_t anx7150_detect_irq(int irq, void *dev_id)
{
	struct hdmi *hdmi = (struct hdmi *)dev_id;
#ifndef CONFIG_ANX7150_IRQ_USE_CHIP
	struct anx7150_pdata *anx = hdmi_get_privdata(hdmi);
	if( anx->init == IRQF_TRIGGER_RISING)
		 anx->init = IRQF_TRIGGER_FALLING;
	else
		 anx->init = IRQF_TRIGGER_RISING;
	irq_set_irq_type(anx->irq, anx->init);
#endif
	hdmi_changed(hdmi, 300);
    return IRQ_HANDLED;
}
#endif

static struct hdmi_ops anx7150_ops = {
	.init = ANX7150_sys_init,
	.insert = ANX7150_sys_plug,
	.detect_hpd = ANX7150_sys_detect_hpd,
//	.detect_sink = ANX7150_sys_detect_sink,
	.read_edid = ANX7150_sys_read_edid,
	.config_video = ANX7150_sys_config_video,
	.config_audio = ANX7150_sys_config_audio,
	.config_hdcp = ANX7150_sys_config_hdcp,
	.enable = ANX7150_sys_enalbe_output,
};

static int anx7150_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int rc = 0;
	struct hdmi *hdmi = NULL;
	struct anx7150_pdata *anx = NULL;
	struct rkdisplay_platform_data *hdmi_data = client->dev.platform_data;
	
	anx = kzalloc(sizeof(struct anx7150_pdata), GFP_KERNEL);
	if(!anx)
	{
        dev_err(&client->dev, "no memory for state\n");
        goto err_kzalloc_anx;
    }
	anx->client = client;
	anx->dev.anx7150_detect = 0;
	anx->init = 1;
	// Register HDMI device
	if(hdmi_data) {
		anx->io_pwr_pin = hdmi_data->io_pwr_pin;
		anx->io_rst_pin = hdmi_data->io_reset_pin;
		hdmi = hdmi_register(&client->dev, &anx7150_ops, hdmi_data->video_source, hdmi_data->property);
	}
	else {
		anx->io_pwr_pin = INVALID_GPIO;
		anx->io_rst_pin = INVALID_GPIO;	
		hdmi = hdmi_register(&client->dev, &anx7150_ops, DISPLAY_SOURCE_LCDC0, DISPLAY_MAIN);
	}
	if(hdmi == NULL)
	{
		dev_err(&client->dev, "fail to register hdmi\n");
		goto err_hdmi_register;
	}
	// Set HDMI private data
	hdmi_set_privdata(hdmi, anx);

    anx->dev.hdmi = hdmi;
	i2c_set_clientdata(client, anx);
	
	//Power on anx7150		
	if(anx->io_pwr_pin != INVALID_GPIO) {
		rc = gpio_request(anx->io_pwr_pin, NULL);
		if(rc) {
			gpio_free(anx->io_pwr_pin);
        	printk(KERN_ERR "request anx7150 power control gpio error\n ");
        	goto err_hdmi_register; 
		}
		else
			gpio_direction_output(anx->io_pwr_pin, GPIO_HIGH);
	}
	
    anx->dev.anx7150_detect = ANX7150_hw_detect_device(anx->client);
	if(anx->dev.anx7150_detect) {
    	HDMI_task(hdmi);
	#ifdef HDMI_USE_IRQ
		hdmi_changed(hdmi, 1);
	    if((rc = gpio_request(client->irq, "hdmi gpio")) < 0)
	    {
	        dev_err(&client->dev, "fail to request gpio %d\n", client->irq);
	        goto err_request_gpio;
	    }
	    anx->irq = gpio_to_irq(client->irq);
		anx->io_irq_pin = client->irq;
	    gpio_pull_updown(client->irq,GPIOPullDown);
	    gpio_direction_input(client->irq);
	    #ifndef CONFIG_ANX7150_IRQ_USE_CHIP
	    anx->init = IRQF_TRIGGER_RISING;
	    if((rc = request_irq(anx->irq, anx7150_detect_irq,IRQF_TRIGGER_RISING,NULL,hdmi)) <0)
	    #else
	    if((rc = request_irq(anx->irq, anx7150_detect_irq,IRQF_TRIGGER_FALLING,NULL,hdmi)) <0)
	    #endif
	    {
	        dev_err(&client->dev, "fail to request hdmi irq\n");
	        goto err_request_irq;
	    }
    #else
		queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 1);
	#endif
		hdmi_enable(hdmi);
		dev_info(&client->dev, "anx7150 probe ok\n");
	}
    else
    	goto err_request_irq;

    return 0;
	
err_request_irq:
	gpio_free(client->irq);
err_request_gpio:
	hdmi_unregister(hdmi);
err_hdmi_register:
	kfree(anx);
	anx = NULL;
err_kzalloc_anx:
	hdmi = NULL;
	dev_err(&client->dev, "anx7150 probe error\n");
	return rc;

}

static int __devexit anx7150_i2c_remove(struct i2c_client *client)
{
	struct anx7150_pdata *anx = (struct anx7150_pdata *)i2c_get_clientdata(client);
	struct hdmi *hdmi = anx->dev.hdmi;

	#ifdef HDMI_USE_IRG
	free_irq(anx->irq, NULL);
	gpio_free(client->irq);
	#endif
	hdmi_unregister(hdmi);
	kfree(anx);
	anx = NULL;
	kfree(hdmi);
	hdmi = NULL;
		
	hdmi_dbg(hdmi->dev, "%s\n", __func__);
    return 0;
}


static const struct i2c_device_id anx7150_id[] = {
	{ "anx7150", 0 },
	{ }
};

static struct i2c_driver anx7150_i2c_driver  = {
    .driver = {
        .name  = "anx7150",
        .owner = THIS_MODULE,
    },
    .probe =    &anx7150_i2c_probe,
    .remove     = &anx7150_i2c_remove,
    .id_table	= anx7150_id,
};

static int __init anx7150_init(void)
{
    return i2c_add_driver(&anx7150_i2c_driver);
}

static void __exit anx7150_exit(void)
{
    i2c_del_driver(&anx7150_i2c_driver);
}

//module_init(anx7150_init);
fs_initcall(anx7150_init);
module_exit(anx7150_exit);
