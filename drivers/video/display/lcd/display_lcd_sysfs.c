#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/display-sys.h>
#include <linux/rk_screen.h>
#include "../../rk29_fb.h"


extern int FB_Switch_Screen( struct rk29fb_screen *screen, u32 enable );

static int lcd_enable = 0;
static struct list_head	lcd_modelist;
static struct fb_videomode *lcd_current_mode = NULL;

/*
 * rk29_lcd_mode: LCD screen timing define. Default timeing is VESA 1600x900@60Hz.
 * If the screen timing is different, please reconfigure this structure.
 */
static const struct fb_videomode rk29_lcd_mode = {	
	"1600x900p@60Hz",		60,		1600,	900,	108000000,	96,		24,		96,		1,		80,		3,		FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT,	0,		0
};

static void set_lcd_pin(struct platform_device *pdev, int enable)
{
	struct rk29fb_info *mach_info = pdev->dev.platform_data;
	if(enable)
		mach_info->io_enable();
	else
		mach_info->io_disable();
}

static int lcd_mode2screen(struct fb_videomode *modedb, struct rk29fb_screen *screen)
{
	if(modedb == NULL || screen == NULL)
		return -1;
		
	memset(screen, 0, sizeof(struct rk29fb_screen));
	
	/* screen type & face */
    screen->type = SCREEN_HDMI;
    screen->face = OUT_P888;
	
	/* Screen size */
	screen->x_res = modedb->xres;
    screen->y_res = modedb->yres;

    /* Timing */
    screen->pixclock = modedb->pixclock;
	screen->lcdc_aclk = 500000000;
	screen->left_margin = modedb->left_margin;
	screen->right_margin = modedb->right_margin;
	screen->hsync_len = modedb->hsync_len;
	screen->upper_margin = modedb->upper_margin;
	screen->lower_margin = modedb->lower_margin;
	screen->vsync_len = modedb->vsync_len;

	/* Pin polarity */
	if(FB_SYNC_HOR_HIGH_ACT & modedb->sync)
		screen->pin_hsync = 1;
	else
		screen->pin_hsync = 0;
	if(FB_SYNC_VERT_HIGH_ACT & modedb->sync)
		screen->pin_vsync = 1;
	else
		screen->pin_vsync = 0;	
	screen->pin_den = 0;
	screen->pin_dclk = 1;

	/* Swap rule */
    screen->swap_rb = 0;
    screen->swap_rg = 0;
    screen->swap_gb = 0;
    screen->swap_delta = 0;
    screen->swap_dumy = 0;

    /* Operation function*/
    screen->init = NULL;
    screen->standby = NULL;
    return 0;
}

static int lcd_set_enable(struct rk_display_device *device, int enable)
{
	struct platform_device *pdev = device->priv_data;
	struct rk29fb_screen screen;
	struct fb_videomode *mode =(struct fb_videomode *)&rk29_lcd_mode;

	if(lcd_enable != enable) {
		if(enable && lcd_current_mode == NULL) {
			lcd_mode2screen(mode, &screen);
			FB_Switch_Screen(&screen, 1);
			lcd_current_mode = mode;
		}
		else if(enable == 0)
			lcd_current_mode = NULL;
		
		set_lcd_pin(pdev, enable);
		lcd_enable = enable;
	}
	return 0;
}

static int lcd_get_enable(struct rk_display_device *device)
{
	return lcd_enable;
}

static int lcd_get_status(struct rk_display_device *device)
{
	return 1;
}

static int lcd_get_modelist(struct rk_display_device *device, struct list_head **modelist)
{
	*modelist = &lcd_modelist;
	return 0;
}

static int lcd_set_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
	struct rk29fb_screen screen;
	
	if(fb_mode_is_equal(&rk29_lcd_mode, mode)) {
		lcd_mode2screen(mode, &screen);
		FB_Switch_Screen(&screen, 1);
		lcd_current_mode = (struct fb_videomode *)&rk29_lcd_mode;
	}
	return 0;
}

static int lcd_get_mode(struct rk_display_device *device, struct fb_videomode *mode)
{
	*mode = rk29_lcd_mode;
	return 0;
}

static struct rk_display_ops lcd_display_ops = {
	.setenable = lcd_set_enable,
	.getenable = lcd_get_enable,
	.getstatus = lcd_get_status,
	.getmodelist = lcd_get_modelist,
	.setmode = lcd_set_mode,
	.getmode = lcd_get_mode,
};

static int display_lcd_probe(struct rk_display_device *device, void *devdata)
{
	device->owner = THIS_MODULE;
	strcpy(device->type, "LCD");
	device->priority = DISPLAY_PRIORITY_LCD;
	device->priv_data = devdata;
	device->ops = &lcd_display_ops;
	return 1;
}

static struct rk_display_driver display_lcd = {
	.probe = display_lcd_probe,
};

static struct rk_display_device *display_device_lcd = NULL;

void lcd_register_display_sysfs(void *devdata)
{
	INIT_LIST_HEAD(&lcd_modelist);
	fb_add_videomode(&rk29_lcd_mode, &lcd_modelist);
	display_device_lcd = rk_display_device_register(&display_lcd, NULL, devdata);
	rk_display_device_enable(display_device_lcd);
}