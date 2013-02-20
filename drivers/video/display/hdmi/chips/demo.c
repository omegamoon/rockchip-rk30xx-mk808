#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <mach/gpio.h>
#include <mach/iomux.h>

static struct hdmi *hdmi = NULL;
static int irq_handle;


/* HDMI transmitter control interface */

/* 
 *	功能：初始化HDMI传输器
 *	参数：hdmi		hdmi设备指针，输入
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_init(struct hdmi *hdmi)
{
	
}

/* 
 *	功能：检测Hotplug
 *	参数：hdmi			hdmi设备指针，输入
 *		  hpdstatus		Hotplug状态，输出, 此地址由调用者分配
 × 						TRUE: hotplug insert, FALSE: hotplug remove
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus)
{

}

/* 
 *	功能：检测显示设备是否处于激活状态
 *	参数：hdmi			hdmi设备指针，输入
 *		  sink_status	显示设备状态，输出，此地址由调用者分配
 ×						TRUE: sink active, FALSE: sink inactive
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_detect_sink(struct hdmi *hdmi, int *sink_status)
{

}

/* 
 *	功能：HDMI设备插入后的动作
 *	参数：hdmi			hdmi设备指针，输入
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_insert(struct hdmi *hdmi)
{

}

/* 
 *	功能：HDMI设备拔出后的动作
 *	参数：hdmi			hdmi设备指针，输入
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_remove(struct hdmi *hdmi)
{

}

/* 
 *	功能：读取EDID数据
 *	参数：hdmi			hdmi设备指针，输入
 *		  block			EDID数据block编号，输入
 *		  buff			返回的EDID数据指针，输出，此地址由调用者分配
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff)
{

}

/* 
 *	功能：配置视频输出
 *	参数：hdmi			hdmi设备指针，输入
 *		  vic			HDMI视频格式编码，输入，取值详见hdmi-itv.h的enum hdmi_video_mode结构。
 *		  input_color	输入的LCDC视频数据颜色模式，默认为HDMI_DEFAULT_COLOR
 *		  output_color	输出的HDMI视频颜色模式，默认为HDMI_DEFAULT_COLOR
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color)
{
	// 配置视频输出
}

/* 
 *	功能：配置音频输出
 *	参数：hdmi			hdmi设备指针，输入
 *		  channel_num	音频数据声道数，输入，默认HDMI_AUDIO_DEFAULT_CHANNEL_NUM
 *		  fs			音频采样率，输入，默认为HDMI_AUDIO_DEFAULT_Fs
 *		  word_length	音频数据位宽，输入，默认为HDMI_AUDIO_DEFAULT_WORD_LENGTH
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_config_audio(struct hdmi *hdmi, int channel_num, int fs, int word_length)
{

}

/* 
 *	功能：配置音频输出
 *	参数：hdmi			hdmi设备指针，输入
 *		  enable		是否开启HDCP，输入，默认HDMI_HDCP_CONFIG
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_config_hdcp(struct hdmi *hdmi, int enable)
{

}

/* 
 *	功能：配置音频输出
 *	参数：hdmi			hdmi设备指针，输入
 *		  enable		是否开启HDMI输出，输入
 *	返回：HDMI_ERROR_SUCESS		成功
 *		  HDMI_ERROR_FALSE			失败
 */
static int demo_sys_enalbe_output(struct hdmi *hdmi, int enable)
{

}

#ifdef HDMI_USE_IRQ
static irqreturn_t demo_detect_irq(int irq, void *dev_id)
{
	hdmi_changed(hdmi, 300);
    return IRQ_HANDLED;
}
#endif
static struct hdmi_ops demo_ops = {
	.init = demo_sys_init,
	.insert = demo_sys_insert,
	.remove = demo_sys_remove,
	.detect_hpd = demo_sys_detect_hpd,
	.detect_sink = demo_sys_detect_sink,
	.read_edid = demo_sys_read_edid,
	.config_video = demo_sys_config_video,
	.config_audio = demo_sys_config_audio,
	.config_hdcp = demo_sys_config_hdcp,
	.enable = demo_sys_enalbe_output,
};
/* I2C driver functions */
static int demo_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int rc = 0;
	struct demo_pdata *anx = NULL;
	
	// Register HDMI device
	hdmi = hdmi_register(&client->dev, &demo_ops);
	if(hdmi == NULL)
	{
		dev_err(&client->dev, "fail to register hdmi\n");
		goto err_hdmi_register;
	}
#ifdef HDMI_USE_IRQ
	//中断方式
	//启动HDMI操作线程
	hdmi_changed(hdmi, 1);
	
	//注册中断
    if((rc = gpio_request(client->irq, "hdmi gpio")) < 0)
    {
        dev_err(&client->dev, "fail to request gpio %d\n", client->irq);
        goto err_request_gpio;
    }
    irq_handle = gpio_to_irq(client->irq);
    gpio_pull_updown(client->irq,GPIOPullDown);
    gpio_direction_input(client->irq);
    if((rc = request_irq(irq_handle, demo_detect_irq,IRQF_TRIGGER_FALLING,NULL,hdmi)) <0)
    {
        dev_err(&client->dev, "fail to request hdmi irq\n");
        goto err_request_irq;
    }
#else
	//轮询方式
	//启动HDMI操作线程
	queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, 200);
#endif
	hdmi_enable(hdmi);
	dev_info(&client->dev, "demo probe ok\n");

    return 0;
	
err_request_irq:
	gpio_free(client->irq);
err_request_gpio:
	hdmi_unregister(hdmi);
err_kzalloc_anx:
	kfree(hdmi);
	hdmi = NULL;
	dev_err(&client->dev, "demo probe error\n");
	return rc;

}

static int __devexit demo_i2c_remove(struct i2c_client *client)
{
	#ifdef HDMI_USE_IRG
	free_irq(irq_handle, NULL);
	gpio_free(client->irq);
	#endif
	hdmi_unregister(hdmi);
	hdmi_dbg(hdmi->dev, "%s\n", __func__);
    return 0;
}

static const struct i2c_device_id demo_id[] = {
	{ "demo", 0 },
	{ }
};

static struct i2c_driver demo_i2c_driver  = {
    .driver = {
        .name  = "demo",
        .owner = THIS_MODULE,
    },
    .probe =    &demo_i2c_probe,
    .remove     = &demo_i2c_remove,
    .id_table	= demo_id,
};

static int __init demo_init(void)
{
    return i2c_add_driver(&demo_i2c_driver);
}

static void __exit demo_exit(void)
{
    i2c_del_driver(&demo_i2c_driver);
}

fs_initcall(demo_init);
module_exit(demo_exit);