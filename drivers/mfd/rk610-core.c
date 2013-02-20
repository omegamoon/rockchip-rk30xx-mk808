#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/gpio.h>
#include <linux/mfd/rk610_core.h>
#include <linux/clk.h>
#include <mach/iomux.h>
#include <linux/err.h>
#include <linux/display-sys.h>
/*
 * Debug
 */
#if 0
#define	DBG(x...)	printk(KERN_INFO x)
#else
#define	DBG(x...)
#endif

static struct i2c_client *rk610_control_client = NULL;
#ifdef CONFIG_RK610_LCD
extern int rk610_lcd_init(struct i2c_client *client);
#else
int rk610_lcd_init(struct i2c_client *client){ return 0;}
#endif
int rk610_control_send_byte(const char reg, const char data)
{
	int ret;

	DBG("reg = 0x%02x, val=0x%02x\n", reg ,data);

	if(rk610_control_client == NULL)
		return -1;
	//i2c_master_reg8_send
	ret = i2c_master_reg8_send(rk610_control_client, reg, &data, 1, 100*1000);
	if (ret > 0)
		ret = 0;

	return ret;
}

#ifdef CONFIG_SND_SOC_RK610
static unsigned int current_pll_value = 0;
int rk610_codec_pll_set(unsigned int rate)
{
	char N, M, NO, DIV;
	unsigned int F;
	char data;

	if(current_pll_value == rate)
		return 0;

    // Input clock is 12MHz.
	if(rate == 11289600) {
		// For 11.2896MHz, N = 2 M= 75 F = 0.264(0x43958) NO = 8
		N = 2;
		NO = 3;
		M = 75;
		F = 0x43958;
		DIV = 5;
	}
	else if(rate == 12288000) {
		// For 12.2888MHz, N = 2 M= 75 F = 0.92(0xEB851) NO = 8
		N = 2;
		NO = 3;
		M = 75;
		F = 0xEB851;
		DIV = 5;
	}
	else {
		printk(KERN_ERR "[%s] not support such frequency\n", __FUNCTION__);
		return -1;
	}

	//Enable codec pll fractional number and power down.
    data = 0x00;
    rk610_control_send_byte(C_PLL_CON5, data);
	msleep(10);

    data = (N << 4) | NO;
    rk610_control_send_byte(C_PLL_CON0, data);
    // M
    data = M;
    rk610_control_send_byte(C_PLL_CON1, data);
    // F
    data = F & 0xFF;
    rk610_control_send_byte(C_PLL_CON2, data);
    data = (F >> 8) & 0xFF;
    rk610_control_send_byte(C_PLL_CON3, data);
    data = (F >> 16) & 0xFF;
    rk610_control_send_byte(C_PLL_CON4, data);

    // i2s mclk = codec_pll/5;
    i2c_master_reg8_recv(rk610_control_client, CLOCK_CON1, &data, 1, 100*1000);
    data &= ~CLOCK_CON1_I2S_DVIDER_MASK;
    data |= (DIV - 1);
    rk610_control_send_byte(CLOCK_CON1, data);

    // Power up codec pll.
    data |= C_PLL_POWER_ON;
    rk610_control_send_byte(C_PLL_CON5, data);

    current_pll_value = rate;
    DBG("[%s] rate %u\n", __FUNCTION__, rate);

    return 0;
}

void rk610_control_init_codec(void)
{
    char data = 0;

    if(rk610_control_client == NULL)
    	return;
	DBG("[%s] start\n", __FUNCTION__);

//    rk610_codec_pll_set(11289600);

    //use internal codec, enable DAC ADC LRCK output.
//    i2c_master_reg8_recv(client, RK610_CONTROL_REG_CODEC_CON, &data, 1, 100*1000);
//    data = CODEC_CON_BIT_DAC_LRCL_OUTPUT_DISABLE | CODEC_CON_BIT_ADC_LRCK_OUTPUT_DISABLE;
//	data = CODEC_CON_BIT_ADC_LRCK_OUTPUT_DISABLE;
	data = 0;
   	rk610_control_send_byte(CODEC_CON, data);

    // Select internal i2s clock from codec_pll.
    i2c_master_reg8_recv(rk610_control_client, CLOCK_CON1, &data, 1, 100*1000);
//    data |= CLOCK_CON1_I2S_CLK_CODEC_PLL;
	data = 0;
    rk610_control_send_byte(CLOCK_CON1, data);

//    i2c_master_reg8_recv(client, CODEC_CON, &data, 1, 100*1000);
//    DBG("[%s] RK610_CONTROL_REG_CODEC_CON is %x\n", __FUNCTION__, data);
//
//    i2c_master_reg8_recv(client, CLOCK_CON1, &data, 1, 100*1000);
//    DBG("[%s] RK610_CONTROL_REG_CLOCK_CON1 is %x\n", __FUNCTION__, data);
}
#endif

static int rk610_control_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
    int ret;
	char data;
    struct clk *iis_clk;
	struct rkdisplay_platform_data *tv_data;
	
	DBG("[%s] start\n", __FUNCTION__);
	
	iis_clk = clk_get_sys("rk29_i2s.0", "i2s");
	if (IS_ERR(iis_clk)) {
		printk("failed to get i2s clk\n");
		ret = PTR_ERR(iis_clk);
	}else{
		DBG("got i2s clk ok!\n");
		clk_enable(iis_clk);
		clk_set_rate(iis_clk, 11289600);
		#ifdef CONFIG_ARCH_RK29
		rk29_mux_api_set(GPIO2D0_I2S0CLK_MIIRXCLKIN_NAME, GPIO2H_I2S0_CLK);
		#else
		rk30_mux_api_set(GPIO0B0_I2S8CHCLK_NAME, GPIO0B_I2S_8CH_CLK);
		#endif
		clk_put(iis_clk);
	}

    if(client->dev.platform_data) {
		tv_data = client->dev.platform_data;
		if(tv_data->io_reset_pin != INVALID_GPIO) {
	    	ret = gpio_request(tv_data->io_reset_pin, "rk610reset");
		    if (ret){   
		        printk("rk610_control_probe request gpio fail\n");
		        //goto err1;
		    }
		    
		    gpio_set_value(tv_data->io_reset_pin, GPIO_LOW);
		    gpio_direction_output(tv_data->io_reset_pin, GPIO_LOW);
		    mdelay(2);
		    gpio_set_value(tv_data->io_reset_pin, GPIO_HIGH);
		}
	}
	rk610_control_client = client;
  	// Set i2c glitch timeout.
	data = 0x22;
	ret = i2c_master_reg8_send(client, I2C_CON, &data, 1, 20*1000);

	rk610_lcd_init(client);
	DBG("[%s] sucess\n", __FUNCTION__);
    return 0;
}

static int rk610_control_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id rk610_control_id[] = {
	{ "rk610_ctl", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, rk610_control_id);

static struct i2c_driver rk610_control_driver = {
	.driver = {
		.name = "rk610_ctl",
	},
	.probe = rk610_control_probe,
	.remove = rk610_control_remove,
	.id_table = rk610_control_id,
};

static int __init rk610_control_init(void)
{
	return i2c_add_driver(&rk610_control_driver);
}

static void __exit rk610_control_exit(void)
{
	i2c_del_driver(&rk610_control_driver);
}

subsys_initcall_sync(rk610_control_init);
//module_init(rk610_control_init);
module_exit(rk610_control_exit);


MODULE_DESCRIPTION("RK610 control driver");
MODULE_AUTHOR("Rock-chips, <www.rock-chips.com>");
MODULE_LICENSE("GPL");

