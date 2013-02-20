#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/ioport.h>
#include <linux/input-polldev.h>
#include <linux/i2c.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <asm/types.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <sound/soc.h>
#include <mach/gpio.h>
//#include <linux/android_power.h>
#include "SP_TX_DRV.h"
#include "SP_TX_Reg.h"
#include "SP_TX_CTRL.h"
#include "Colorado.h"


#undef NULL
#define NULL ((void *)0)


//#define DISABLE 0
//#define ENABLE  1

//#define UNINITIALIZED   0
//#define INITIALIZED     1


struct i2c_client *g_client;
int timer_en_cnt;
static struct workqueue_struct * Colorado_workqueue;
static struct delayed_work Colorado_work;
static long last_jiffies;


static const struct i2c_device_id Colorado_register_id[] = {
     { "Colorado_i2c", 0 },
     { }
};


MODULE_DEVICE_TABLE(i2c, Colorado_register_id);


static void __init Colorado_init_gpio(void)
{
	int ret;
	if(SP_TX_PWR_V12_CTRL != INVALID_GPIO)
	{
	    ret = gpio_request(SP_TX_PWR_V12_CTRL, NULL);
	    if(ret != 0)
	    {
	        gpio_free(SP_TX_PWR_V12_CTRL);
	        printk(">>>>>> SP_TX_PWR_V12_CTRL request err.\n ");
	    }
	}
	if(SP_TX_HW_RESET != INVALID_GPIO)
	{
    	ret = gpio_request(SP_TX_HW_RESET, NULL);
    	if(ret != 0)
	    {
	        gpio_free(SP_TX_HW_RESET);
	        printk(">>>>>> SP_TX_HW_RESET request err.\n ");
	    }
    }
    if(SLIMPORT_CABLE_DETECT != INVALID_GPIO)
    {
    	ret = gpio_request(SLIMPORT_CABLE_DETECT,NULL);
    	if(ret != 0)
	    {
	        gpio_free(SLIMPORT_CABLE_DETECT);
	        printk(">>>>>> SLIMPORT_CABLE_DETECT request err.\n ");
	    }
    }
    if(VBUS_PWR_CTRL != INVALID_GPIO)
    {
    	ret = gpio_request(VBUS_PWR_CTRL, NULL);
    	if(ret != 0)
	    {
	        gpio_free(VBUS_PWR_CTRL);
	        printk(">>>>>> VBUS_PWR_CTRL request err.\n ");
	    }
    }
    if(SP_TX_CHIP_PD_CTRL != INVALID_GPIO)
    {
    	ret = gpio_request(SP_TX_CHIP_PD_CTRL, NULL);
    	if(ret != 0)
	    {
	        gpio_free(SP_TX_CHIP_PD_CTRL);
	        printk(">>>>>> VBUS_PWR_CTRL request err.\n ");
	    }
	}
	if(SLIMPORT_CABLE_DETECT != INVALID_GPIO)
    	gpio_direction_input(SLIMPORT_CABLE_DETECT); 
    if(SP_TX_PWR_V12_CTRL != INVALID_GPIO)
   		gpio_direction_output(SP_TX_PWR_V12_CTRL, 0); /* Default V12_CTRL low */
    if(VBUS_PWR_CTRL != INVALID_GPIO)
    	gpio_direction_output(VBUS_PWR_CTRL,0);/* Default VBUS_PWR_CTRL low */
    if(SP_TX_CHIP_PD_CTRL != INVALID_GPIO)
    	gpio_direction_output(SP_TX_CHIP_PD_CTRL,1);/* Default SP_TX_CHIP_PD_CTRL high */
    if(SP_TX_HW_RESET != INVALID_GPIO)
    	gpio_direction_output(SP_TX_HW_RESET,0);/* Default SP_TX_HW_RESET high */

}

static struct hdmi_ops anx7805_ops = {
	.init = ANX7805_sys_init,
//	.insert = ANX7805_sys_plug,
	.detect_hpd = ANX7805_sys_detect_hpd,
//	.detect_sink = ANX7805_sys_detect_sink,
	.read_edid = ANX7805_sys_read_edid,
	.config_video = ANX7805_sys_config_video,
	.config_audio = ANX7805_sys_config_audio,
	.config_hdcp = ANX7805_sys_config_hdcp,
	.enable = ANX7805_sys_enalbe_output,
};

static int Colorado_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err = 0;
	struct hdmi *hdmi = NULL;
	struct anx7805_pdata *anx = NULL;
	D("##########Colorado_i2c_probe##############\n");

	memcpy(&g_client, &client, sizeof(client));	
    
	Colorado_init_gpio();
       
	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_I2C/*I2C_FUNC_SMBUS_I2C_BLOCK*/)) {
		dev_err(&client->dev, "i2c bus does not support the Colorado\n");
		err = -ENODEV;
		goto exit_kfree;
	}
	
	anx = kzalloc(sizeof(struct anx7805_pdata), GFP_KERNEL);
	if(!anx)
	{
        dev_err(&client->dev, "no memory for state\n");
        goto err_kzalloc_anx;
    }
	anx->client = client;
	anx->dev.detect = 0;
	
	// Register HDMI device
	hdmi = hdmi_register(&client->dev, &anx7805_ops);
	if(hdmi == NULL)
	{
		dev_err(&client->dev, "fail to register hdmi\n");
		goto err_hdmi_register;
	}
	hdmi_set_privdata(hdmi, anx);
	anx->dev.hdmi = hdmi;
	i2c_set_clientdata(client, anx);
	
	err = Colorado_System_Init();
	if (err)
		goto exit_kfree;
	HDMI_task(hdmi);
	queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 1);
	hdmi_enable(hdmi);
	dev_info(&client->dev, "anx7150 probe ok\n");

	return 0;
	
exit_kfree:
	hdmi_unregister(hdmi);
err_hdmi_register:
	kfree(anx);
	anx = NULL;
err_kzalloc_anx:
	hdmi = NULL;
	dev_err(&client->dev, "anx7805 probe error\n");
	return err;

}

static int Colorado_i2c_remove(struct i2c_client *client)
{
	return 0;
}


static struct i2c_driver Colorado_driver = {
     .driver = {
	  .name   = "Colorado_i2c",
	  .owner  = THIS_MODULE,
     },
     .probe            = Colorado_i2c_probe,
     .remove         = Colorado_i2c_remove,
     .id_table         = Colorado_register_id,
};


void Colorado_work_func(struct work_struct * work)
{

    last_jiffies = (long) jiffies;

    if(timer_en_cnt > 0)
    {
        SP_CTRL_Main_Procss();
        timer_en_cnt--;
    }

    timer_en_cnt = 1;
    queue_delayed_work(Colorado_workqueue, &Colorado_work, msecs_to_jiffies(500));

}



int  Colorado_System_Init (void)
{
    int ret;
    
    ret = SP_CTRL_Chip_Detect();
    if(ret<0)
    {
        D("Chip detect error\n");
        return -1;
    }

    //Chip initialization
    SP_CTRL_Chip_Initial();
    return 0;

}

static int __init Colorado_init(void)
{
	int ret;

        timer_en_cnt = 0;

	D("##########Colorado_init################\n");
	
	/* step 1 i2c_add_driver */
	ret = i2c_add_driver(&Colorado_driver);
	if (ret < 0){
		D("i2c_add_driver err!\n");
		goto err1;
	}
	#if 0
	/* step 2 add misc device */
	ret = misc_register(&Colorado_dev.misc_dev);
	if (ret < 0){
		E("misc_register err!\n");
		goto err2;
	}
	#endif

	#if 0
	/* step 3 init timer */
	Colorado_workqueue = create_singlethread_workqueue("ANX7805_WORKQUEUE");

	if (Colorado_workqueue == NULL)
	{
		printk("Colorado: Failed to creat work queue.\n");
		goto err3;
	}

	last_jiffies = (long) jiffies;
	INIT_DELAYED_WORK(&Colorado_work, Colorado_work_func);
	queue_delayed_work(Colorado_workqueue, &Colorado_work, 0);
	#endif
//err2:
//	misc_deregister(&Colorado_dev.misc_dev);
err1:
	i2c_del_driver(&Colorado_driver);
err3:
	return ret;
}



static void __exit Colorado_exit(void)
{
	D("###########Colorado_exit!!###########\n");

	//misc_deregister(&Colorado_dev.misc_dev);
	i2c_del_driver(&Colorado_driver);
}

int Colorado_send_msg(unsigned char addr, unsigned char *buf, unsigned short len, unsigned short rw_flag)
{
	int rc;
	char data = 0;
	g_client-> addr = addr;
	if(rw_flag)
	{
//		rc=i2c_smbus_read_byte_data(g_client, *buf);
		rc = i2c_master_reg8_recv(g_client, *buf, &data, 1, 100000);
//		D("Colorado_send_msg read addr 0x%02x reg 0x%x data 0x%x\n", addr << 1, *buf, data);
		*buf = data;
//		*buf = rc;
		
	} else
	{
//		rc=i2c_smbus_write_byte_data(g_client, buf[0], buf[1]);
//		D("Colorado_send_msg send addr 0x%02x reg 0x%x data 0x%x\n", addr << 1, buf[0], buf[1]);
		rc = i2c_master_reg8_send(g_client, buf[0], &(buf[1]), 1, 100000);	
	}
//	D("------\n");	
	return 0;
}

unsigned char SP_TX_Read_Reg(unsigned char dev,unsigned char offset, unsigned char *d)
{
	unsigned char c;
	int ret;
	c = offset;

	ret = Colorado_send_msg(dev >> 1, &c, 1, 1);
	if(ret < 0){
		D("Colorado_send_msg err!\n");
		return 1;
	}

	*d = c;
	return 0;
}


unsigned char SP_TX_Write_Reg(unsigned char dev,unsigned char offset, unsigned char d)
{
	unsigned char buf[2] = {offset, d};

	return Colorado_send_msg(dev >> 1, buf, 2, 0);
}




module_init(Colorado_init);
module_exit(Colorado_exit);

MODULE_DESCRIPTION ("Colorado driver");
MODULE_AUTHOR("FeiWang<fwang@analogixsemi.com>");
MODULE_LICENSE("GPL");







