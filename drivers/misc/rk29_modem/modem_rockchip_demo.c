#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <mach/iomux.h>
#include <mach/gpio.h>
#include <linux/delay.h>

#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <mach/board.h>

#include <linux/platform_device.h>

#include "rk29_modem.h"

static int rk30_modem_probe(struct platform_device *pdev);
static int rk30_modem_suspend(struct platform_device *pdev, pm_message_t state);
static int rk30_modem_resume(struct platform_device *pdev);

static struct platform_driver demo_platform_driver = {
    .probe  = rk30_modem_probe,
	.suspend    = rk30_modem_suspend,
	.resume     = rk30_modem_resume,
    .driver = {
        .name   = "rk30_modem",
    },
};
#define RK30_AP_WAKEUP_BP        RK30_PIN2_PB7
#define RK30_AP_WAKEUP_BP_IOMUX  rk29_mux_api_set(GPIO2B7_LCDC1DATA15_SMCADDR19_HSADCDATA7_NAME, GPIO2B_GPIO2B7)

#define RK30_3G_DISABLE          RK30_PIN2_PC0
#define RK30_3G_DISABLE_IOMUX    rk29_mux_api_set(GPIO2C0_LCDCDATA16_GPSCLK_HSADCCLKOUT_NAME, GPIO2C_GPIO2C0)
#define RK30_3G_SUSPEND          RK30_PIN0_PB5
#define RK30_3G_SUSPEND_IOMUX    rk29_mux_api_set(GPIO0B5_I2S8CHSDO1_NAME, GPIO0B_GPIO0B5)

static int rk30_ap_wake_io_init(void)
{
    printk("%s\n", __FUNCTION__);
    RK30_AP_WAKEUP_BP_IOMUX;
    RK30_3G_SUSPEND_IOMUX;         //init suspend io

	return 0;
}

static struct rk29_io_t rk30_ap_wakeup_bp_io = {
    .io_addr    = RK30_AP_WAKEUP_BP,
    .enable     = GPIO_HIGH,
    .disable    = GPIO_LOW,
    .io_init    = rk30_ap_wake_io_init,
};

static int rk30_bp_disable_io_init(void)
{
    printk("%s\n", __FUNCTION__);
    RK30_3G_DISABLE_IOMUX;

	return 0;
}

static struct rk29_io_t rk30_bp_disable_io = {
    .io_addr    = RK30_3G_DISABLE,
    .enable     = GPIO_HIGH,
    .disable    = GPIO_LOW,
    .io_init    = rk30_bp_disable_io_init,
};


static struct rk29_modem_t demo_driver = {
    .driver         = &demo_platform_driver,
    .modem_power    = NULL,//&demo_io_power,
    .ap_ready       = &rk30_ap_wakeup_bp_io,
    .bp_disable     = &rk30_bp_disable_io,
    .bp_wakeup_ap   = NULL,
    .status         = MODEM_ENABLE,
    .dev_init       = NULL,
    .dev_uninit     = NULL,
    .irq_handler    = NULL,
    
    .enable         = NULL,
    .disable        = NULL,
    .sleep          = NULL,
    .wakeup         = NULL,
};

static int rk30_modem_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct rk29_io_t *rk29_io_info = pdev->dev.platform_data;

    printk("%s\n", __FUNCTION__);
    demo_driver.modem_power = rk29_io_info;
    demo_driver.modem_power->io_init();
    if (demo_driver.ap_ready)
    	demo_driver.ap_ready->io_init();
    if (demo_driver.bp_disable)
    	demo_driver.bp_disable->io_init();

	ret = gpio_request(RK30_3G_SUSPEND, "3G_SUSPEND");
    if(ret != 0)
    {
        gpio_free(RK30_3G_SUSPEND);
        printk(">>>>>>  3G Suspend io request failed!\n");
        return ret;
    }

    gpio_direction_output(RK30_3G_SUSPEND, GPIO_HIGH);

    return 0;
}

static int rk30_modem_suspend(struct platform_device *pdev, pm_message_t state)
{
//    printk("rk30_modem_suspend\n");
//	rk29_modem_change_status(&demo_driver, MODEM_DISABLE);
	gpio_direction_output(RK30_3G_SUSPEND, GPIO_LOW);
	return 0;
}

static int rk30_modem_resume(struct platform_device *pdev)
{
//	printk("rk30_modem_resume\n");
//	rk29_modem_change_status(&demo_driver, MODEM_ENABLE);
	gpio_direction_output(RK30_3G_SUSPEND, GPIO_HIGH);
	return 0;
}

static int __init demo_init(void)
{
    printk("%s[%d]: %s\n", __FILE__, __LINE__, __FUNCTION__);

    return rk29_modem_init(&demo_driver);
}

static void __exit demo_exit(void)
{
    printk("%s[%d]: %s\n", __FILE__, __LINE__, __FUNCTION__);
    rk29_modem_exit();
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_AUTHOR("lintao lintao@rock-chips.com, cmy@rock-chips.com");
MODULE_DESCRIPTION("ROCKCHIP modem driver");
MODULE_LICENSE("GPL");

