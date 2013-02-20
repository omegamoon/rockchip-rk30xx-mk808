#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <linux/i2c.h>
#include <linux/hdmi-itv.h>
#include "hdmitx.h"

static struct i2c_client *pcat = NULL;

/* I2C read/write funcs */
BYTE HDMITX_ReadI2C_Byte(SHORT RegAddr)
{
	struct i2c_msg msgs[2];
	SYS_STATUS ret = -1;
	BYTE buf[1];

	buf[0] = RegAddr;

	/* Write device addr fisrt */
	msgs[0].addr	= pcat->addr;
	msgs[0].flags	= !I2C_M_RD;
	msgs[0].len		= 1;
	msgs[0].buf		= &buf[0];
	msgs[0].scl_rate= CAT6611_SCL_RATE;
	/* Then, begin to read data */
	msgs[1].addr	= pcat->addr;
	msgs[1].flags	= I2C_M_RD;
	msgs[1].len		= 1;
	msgs[1].buf		= &buf[0];
	msgs[1].scl_rate= CAT6611_SCL_RATE;
	
	ret = i2c_transfer(pcat->adapter, msgs, 2);
	if(ret != 2)
		printk("I2C transfer Error! ret = %d\n", ret);

	//ErrorF("Reg%02xH: 0x%02x\n", RegAddr, buf[0]);
	return buf[0];
}

SYS_STATUS HDMITX_WriteI2C_Byte(SHORT RegAddr, BYTE data)
{
	struct i2c_msg msg;
	SYS_STATUS ret = -1;
	BYTE buf[2];

	buf[0] = RegAddr;
	buf[1] = data;

	msg.addr	= pcat->addr;
	msg.flags	= !I2C_M_RD;
	msg.len		= 2;
	msg.buf		= buf;		
	msg.scl_rate= CAT6611_SCL_RATE;
	
	ret = i2c_transfer(pcat->adapter, &msg, 1);
	if(ret != 1)
		printk("I2C transfer Error!\n");

	return ret;
}

SYS_STATUS HDMITX_ReadI2C_ByteN(SHORT RegAddr, BYTE *pData, int N)
{
	struct i2c_msg msgs[2];
	SYS_STATUS ret = -1;

	pData[0] = RegAddr;

	msgs[0].addr	= pcat->addr;
	msgs[0].flags	= !I2C_M_RD;
	msgs[0].len		= 1;
	msgs[0].buf		= &pData[0];
	msgs[0].scl_rate= CAT6611_SCL_RATE;

	msgs[1].addr	= pcat->addr;
	msgs[1].flags	= I2C_M_RD;
	msgs[1].len		= N;
	msgs[1].buf		= pData;
	msgs[1].scl_rate= CAT6611_SCL_RATE;
	
	ret = i2c_transfer(pcat->adapter, msgs, 2);
	if(ret != 2)
		printk("I2C transfer Error! ret = %d\n", ret);

	return ret;
}

SYS_STATUS HDMITX_WriteI2C_ByteN(SHORT RegAddr, BYTE *pData, int N)
{
	struct i2c_msg msg;
	SYS_STATUS ret = -1;
	BYTE buf[N + 1];

	buf[0] = RegAddr;
    memcpy(&buf[1], pData, N);

	msg.addr	= pcat->addr;
	msg.flags	= !I2C_M_RD;
	msg.len		= N + 1;
	msg.buf		= buf;		// gModify.Exp."Include RegAddr"
	msg.scl_rate= CAT6611_SCL_RATE;
	
	ret = i2c_transfer(pcat->adapter, &msg, 1);
	if(ret != 1)
		printk("I2C transfer Error! ret = %d\n", ret);

	return ret;
}

#ifdef HDMI_USE_IRQ
static irqreturn_t cat6611_detect_irq(int irq, void *dev_id)
{
	struct hdmi *hdmi = (struct hdmi *)dev_id;

	hdmi_changed(hdmi, 1);
    return IRQ_HANDLED;
}
#endif

static struct hdmi_ops cat6611_ops = {
	.init = CAT6611_sys_init,
	.remove = CAT6611_sys_unplug,
	.detect_hpd = CAT6611_sys_detect_hpd,
//	.detect_sink = CAT6611_sys_detect_sink,
	.read_edid = CAT6611_sys_read_edid,
	.config_video = CAT6611_sys_config_video,
	.config_audio = CAT6611_sys_config_audio,
	.config_hdcp = CAT6611_sys_config_hdcp,
	.enable = CAT6611_sys_enalbe_output,
};

static int cat6611_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int rc = 0;
	struct hdmi *hdmi = NULL;
	struct cat6611_pdata *anx = NULL;
	struct rkdisplay_platform_data *hdmi_data = client->dev.platform_data;
	
	anx = kzalloc(sizeof(struct cat6611_pdata), GFP_KERNEL);
	if(!anx)
	{
        dev_err(&client->dev, "no memory for state\n");
        goto err_kzalloc_anx;
    }
	anx->client = client;
	anx->dev.cat6611_detect = 0;
	/* global var */
	pcat = client;		// gModify.Add
	
	// Register HDMI device
	if(hdmi_data) {
		anx->io_pwr_pin = hdmi_data->io_pwr_pin;
		anx->io_rst_pin = hdmi_data->io_reset_pin;
		hdmi = hdmi_register(&client->dev, &cat6611_ops, hdmi_data->video_source, hdmi_data->property);
	}
	else {
		anx->io_pwr_pin = INVALID_GPIO;
		anx->io_rst_pin = INVALID_GPIO;	
		hdmi = hdmi_register(&client->dev, &cat6611_ops, DISPLAY_SOURCE_LCDC0, DISPLAY_MAIN);
	}
	if(hdmi == NULL)
	{
		dev_err(&client->dev, "fail to register hdmi\n");
		goto err_hdmi_register;
	}
	
	hdmi->support_r2y = 1;
	anx->dev.hdmi = hdmi;
	hdmi_set_privdata(hdmi, anx);
	i2c_set_clientdata(client, anx);
	
	//Power on IT6610		
	if(anx->io_pwr_pin != INVALID_GPIO) {
		rc = gpio_request(anx->io_pwr_pin, NULL);
		if(rc) {
			gpio_free(anx->io_pwr_pin);
        	printk(KERN_ERR "request cat6610 power control gpio error\n ");
        	goto err_hdmi_register; 
		}
		else
			gpio_direction_output(anx->io_pwr_pin, GPIO_HIGH);
	}
	
	anx->dev.cat6611_detect = CAT6611_detect_device();
	if(anx->dev.cat6611_detect) {
		HDMI_task(hdmi);
		#ifdef HDMI_USE_IRQ
		//CAT6611_sys_init(hdmi);
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
	    if((rc = request_irq(anx->irq, cat6611_detect_irq,IRQF_TRIGGER_FALLING,NULL,hdmi)) <0)
	    {
	        dev_err(&client->dev, "fail to request hdmi irq\n");
	        goto err_request_irq;
	    }
		#else
		queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 200);
		#endif
		hdmi_enable(hdmi);
		dev_info(&client->dev, "cat6611 i2c probe ok\n");
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
	dev_err(&client->dev, "cat6611 probe error\n");
	return rc;

}

static int __devexit cat6611_i2c_remove(struct i2c_client *client)
{
	struct cat6611_pdata *anx = (struct cat6611_pdata *)i2c_get_clientdata(client);
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

    return 0;
}

static const struct i2c_device_id cat6611_id[] = {
	{ "cat6611", 0 },
	{ }
};

static struct i2c_driver cat6611_i2c_driver = {
    .driver = {
        .name  = "cat6611",
        .owner = THIS_MODULE,
    },
    .probe      = &cat6611_i2c_probe,
    .remove     = &cat6611_i2c_remove,
    .id_table	= cat6611_id,
};

static int __init cat6611_init(void)
{
    return i2c_add_driver(&cat6611_i2c_driver);
}

static void __exit cat6611_exit(void)
{
    i2c_del_driver(&cat6611_i2c_driver);
}

//module_init(cat6611_init);
fs_initcall(cat6611_init);
module_exit(cat6611_exit);
