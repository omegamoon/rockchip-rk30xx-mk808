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

#include "sii902x.h"

struct sii902x_pdata *sii902x = NULL;

#ifdef HDMI_USE_IRQ
static irqreturn_t sii902x_detect_irq(int irq, void *dev_id)
{
	struct hdmi *hdmi = (struct hdmi *)dev_id;
	hdmi_changed(hdmi, 300);
    return IRQ_HANDLED;
}
#endif

static struct hdmi_ops sii902x_ops = {
	.init 			=	sii902x_sys_init,
	.insert 		=	sii902x_sys_insert,
	.remove 		= 	sii902x_sys_remove,
	.detect_hpd 	= 	sii902x_sys_detect_hpd,
	.read_edid 		= 	sii902x_sys_read_edid,
	.config_video 	=	sii902x_sys_config_video,
	.config_audio 	=	sii902x_sys_config_audio,
	.config_hdcp 	= 	sii902x_sys_config_hdcp,
	.enable 		=	sii902x_sys_enalbe_output,
};

static int sii902x_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int rc = 0;
	struct hdmi *hdmi = NULL;
	struct rkdisplay_platform_data *hdmi_data = client->dev.platform_data;
	sii902x = kzalloc(sizeof(struct sii902x_pdata), GFP_KERNEL);
	if(!sii902x)
	{
        dev_err(&client->dev, "no memory for state\n");
        goto err_kzalloc_sii902x;
    }
	sii902x->client = client;
	sii902x->dev.detected = 0;
	sii902x->dev.hdcp_supported = 0;
	sii902x->init = 1;
	
	// Register HDMI device
	if(hdmi_data) {
		sii902x->io_pwr_pin = hdmi_data->io_pwr_pin;
		sii902x->io_rst_pin = hdmi_data->io_reset_pin;
		hdmi = hdmi_register(&client->dev, &sii902x_ops, hdmi_data->video_source, hdmi_data->property);
	}
	else {
		sii902x->io_pwr_pin = INVALID_GPIO;
		sii902x->io_rst_pin = INVALID_GPIO;	
		hdmi = hdmi_register(&client->dev, &sii902x_ops, DISPLAY_SOURCE_LCDC0, DISPLAY_MAIN);
	}
	if(hdmi == NULL)
	{
		dev_err(&client->dev, "fail to register hdmi\n");
		goto err_hdmi_register;
	}
	hdmi->support_r2y = 1;
	// Set HDMI private data
	hdmi_set_privdata(hdmi, sii902x);

    sii902x->dev.hdmi = hdmi;
	i2c_set_clientdata(client, sii902x);
	
	//Power on sii902x
	if(sii902x->io_pwr_pin != INVALID_GPIO) {
		rc = gpio_request(sii902x->io_pwr_pin, NULL);
		if(rc) {
			gpio_free(sii902x->io_pwr_pin);
        	printk(KERN_ERR "request sii902x power control gpio error\n ");
        	goto err_hdmi_register; 
		}
		else
			gpio_direction_output(sii902x->io_pwr_pin, GPIO_HIGH);
	}
	
	if(sii902x->io_rst_pin != INVALID_GPIO) {
		rc = gpio_request(sii902x->io_rst_pin, NULL);
		if(rc) {
			gpio_free(sii902x->io_rst_pin);
        	printk(KERN_ERR "request sii902x reset control gpio error\n ");
        	goto err_hdmi_register; 
		}
		else
			gpio_direction_output(sii902x->io_rst_pin, GPIO_HIGH);
	}
	
    sii902x->dev.detected = sii902x_detect_device(sii902x);
	if(sii902x->dev.detected) {
    	HDMI_task(hdmi);
	#ifdef HDMI_USE_IRQ
		hdmi_changed(hdmi, 1);
	    if((rc = gpio_request(client->irq, "hdmi gpio")) < 0)
	    {
	        dev_err(&client->dev, "fail to request gpio %d\n", client->irq);
	        goto err_request_gpio;
	    }
	    sii902x->irq = gpio_to_irq(client->irq);
		sii902x->io_irq_pin = client->irq;
	    gpio_pull_updown(client->irq,GPIOPullDown);
	    gpio_direction_input(client->irq);
	    if((rc = request_irq(sii902x->irq, sii902x_detect_irq,IRQF_TRIGGER_FALLING,NULL,hdmi)) <0)
	    {
	        dev_err(&client->dev, "fail to request hdmi irq\n");
	        goto err_request_irq;
	    }
    #else
		queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 1);
	#endif
		hdmi_enable(hdmi);
		dev_info(&client->dev, "sii902x probe ok\n");
	}
    else
    	goto err_request_gpio;

    return 0;
	
err_request_irq:
	gpio_free(client->irq);
err_request_gpio:
	hdmi_unregister(hdmi);
err_hdmi_register:
	kfree(sii902x);
	sii902x = NULL;
err_kzalloc_sii902x:
	hdmi = NULL;
	dev_err(&client->dev, "sii902x probe error\n");
	return rc;

}

static int __devexit sii902x_i2c_remove(struct i2c_client *client)
{
	struct sii902x_pdata *sii902x = (struct sii902x_pdata *)i2c_get_clientdata(client);
	struct hdmi *hdmi = sii902x->dev.hdmi;

	#ifdef HDMI_USE_IRG
	free_irq(sii902x->irq, NULL);
	gpio_free(client->irq);
	#endif
	hdmi_unregister(hdmi);
	kfree(sii902x);
	sii902x = NULL;
	kfree(hdmi);
	hdmi = NULL;
		
	hdmi_dbg(hdmi->dev, "%s\n", __func__);
    return 0;
}


static const struct i2c_device_id sii902x_id[] = {
	{ "sii902x", 0 },
	{ }
};

static struct i2c_driver sii902x_i2c_driver  = {
    .driver = {
        .name  = "sii902x",
        .owner = THIS_MODULE,
    },
    .probe 		= &sii902x_i2c_probe,
    .remove     = &sii902x_i2c_remove,
    .id_table	= sii902x_id,
};

static int __init sii902x_init(void)
{
    return i2c_add_driver(&sii902x_i2c_driver);
}

static void __exit sii902x_exit(void)
{
    i2c_del_driver(&sii902x_i2c_driver);
}

//module_init(sii902x_init);
fs_initcall(sii902x_init);
module_exit(sii902x_exit);
