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

#include "sii92326.h"
#include "sii_92326_api.h"
#include "sii_92326_driver.h"

struct sii92326_pdata *sii92326 = NULL;

static const struct i2c_device_id mhl_sii_id[] = {
	{ "sii92326", 0 },
	{ "sii92326_page1", 0 },
	{ "sii92326_page2", 0 },
	{ "sii92326_cbus", 0 },
	{ "siiEDID", 0},
	{ "siiSegEDID", 0},
	{ "siiHDCP", 0},
	{ }
};

static int match_id(const struct i2c_device_id *id, const struct i2c_client *client)
{
	if (strcmp(client->name, id->name) == 0)
		return true;

	return false;
}

#ifdef HDMI_USE_IRQ
static irqreturn_t sii92326_detect_irq(int irq, void *dev_id)
{
//	struct hdmi *hdmi = (struct hdmi *)dev_id;
	MhlTxISRCounter += 30;
//	hdmi_changed(hdmi, 1);
    return IRQ_HANDLED;
}
#endif

static struct hdmi_ops sii92326_ops = {
	.init 			=	sii92326_sys_init,
//	.insert 		=	sii92326_sys_insert,
//	.remove 		= 	sii92326_sys_remove,
	.detect_hpd 	= 	sii92326_sys_detect_hpd,
	.read_edid 		= 	sii92326_sys_read_edid,
	.config_video 	=	sii92326_sys_config_video,
	.config_audio 	=	sii92326_sys_config_audio,
	.config_hdcp 	= 	sii92326_sys_config_hdcp,
	.enable 		=	sii92326_sys_enalbe_output,
};

static int sii92326_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int rc = 0;
	struct hdmi *hdmi = NULL;
	struct rkdisplay_platform_data *hdmi_data = client->dev.platform_data;

	//static struct mxc_lcd_platform_data *plat_data;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
        printk("%s i2c check function err!\n",__func__); 
		return -ENODEV;
	}

	if(sii92326 == NULL)
	{
		sii92326 = kzalloc(sizeof(struct sii92326_pdata), GFP_KERNEL);
		if(!sii92326)
		{
	        dev_err(&client->dev, "no memory for state\n");
	        goto err_kzalloc_sii92326;
	    }
    	sii92326->dev.detected = 0;
	}

	if(match_id(&mhl_sii_id[0], client))
	{
		sii92326->client = client;

		// Register HDMI device
		if(hdmi_data) {
			sii92326->io_pwr_pin = hdmi_data->io_pwr_pin;
			sii92326->io_rst_pin = hdmi_data->io_reset_pin;
			hdmi = hdmi_register(&client->dev, &sii92326_ops, hdmi_data->video_source, hdmi_data->property);
		}
		else {
			sii92326->io_pwr_pin = INVALID_GPIO;
			sii92326->io_rst_pin = INVALID_GPIO;
			hdmi = hdmi_register(&client->dev, &sii92326_ops, DISPLAY_SOURCE_LCDC0, DISPLAY_MAIN);
		}
		if(hdmi == NULL)
		{
			dev_err(&client->dev, "fail to register hdmi\n");
			goto err_hdmi_register;
		}
		hdmi->support_r2y = 1;
		// Set HDMI private data
		hdmi_set_privdata(hdmi, sii92326);
	
	    sii92326->dev.hdmi = hdmi;
		i2c_set_clientdata(client, sii92326);
		//Power on sii92326		
		if(sii92326->io_pwr_pin != INVALID_GPIO) {
			rc = gpio_request(sii92326->io_pwr_pin, NULL);
			if(rc) {
				gpio_free(sii92326->io_pwr_pin);
	        	printk(KERN_ERR "request sii92326 power control gpio error\n ");
	        	goto err_hdmi_register; 
			}
			else
				gpio_direction_output(sii92326->io_pwr_pin, GPIO_HIGH);
		}
		
		if(sii92326->io_rst_pin != INVALID_GPIO) {
			rc = gpio_request(sii92326->io_rst_pin, NULL);
			if(rc) {
				gpio_free(sii92326->io_rst_pin);
	        	printk(KERN_ERR "request sii92326 reset control gpio error\n ");
	        	goto err_hdmi_register; 
			}
			else
				gpio_direction_output(sii92326->io_rst_pin, GPIO_HIGH);
		}
		
	    sii92326->dev.detected = sii92326_detect_device(sii92326);
		if(sii92326->dev.detected) {
		    if((rc = gpio_request(client->irq, "hdmi gpio")) < 0)
		    {
		        dev_err(&client->dev, "fail to request gpio %d\n", client->irq);
		        goto err_request_gpio;
		    }
		    sii92326->irq = gpio_to_irq(client->irq);
			sii92326->io_irq_pin = client->irq;
		    gpio_pull_updown(client->irq,GPIOPullDown);
		    gpio_direction_input(client->irq);
		    if((rc = request_irq(sii92326->irq, sii92326_detect_irq,IRQF_TRIGGER_FALLING,NULL,hdmi)) <0)
		    {
		        dev_err(&client->dev, "fail to request hdmi irq\n");
		        goto err_request_irq;
		    }
			queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 1);
			hdmi_enable(hdmi);
			dev_info(&client->dev, "sii92326 probe ok\n");
		}
	    else
	    	goto err_request_irq;
	}
    return 0;
	
err_request_irq:
	gpio_free(client->irq);
err_request_gpio:
	hdmi_unregister(hdmi);
err_hdmi_register:
	kfree(sii92326);
	sii92326 = NULL;
err_kzalloc_sii92326:
	hdmi = NULL;
	dev_err(&client->dev, "sii92326 probe error\n");
	return rc;

}

static int __devexit sii92326_i2c_remove(struct i2c_client *client)
{
	struct sii92326_pdata *sii92326 = (struct sii92326_pdata *)i2c_get_clientdata(client);
	struct hdmi *hdmi = sii92326->dev.hdmi;

	#ifdef HDMI_USE_IRG
	free_irq(sii92326->irq, NULL);
	gpio_free(client->irq);
	#endif
	hdmi_unregister(hdmi);
	kfree(sii92326);
	sii92326 = NULL;
	kfree(hdmi);
	hdmi = NULL;
		
	hdmi_dbg(hdmi->dev, "%s\n", __func__);
    return 0;
}

static struct i2c_driver sii92326_i2c_driver  = {
    .driver = {
        .name  = "sii92326",
        .owner = THIS_MODULE,
    },
    .probe 		= &sii92326_i2c_probe,
    .remove     = &sii92326_i2c_remove,
    .id_table	= mhl_sii_id,
};

static int __init sii92326_init(void)
{
    return i2c_add_driver(&sii92326_i2c_driver);
}

static void __exit sii92326_exit(void)
{
    i2c_del_driver(&sii92326_i2c_driver);
}

module_init(sii92326_init);
//fs_initcall(sii92326_init);
module_exit(sii92326_exit);
