/*
 * adv7180.c Analog Devices ADV7180 video decoder driver
 * Copyright (c) 2009 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <media/v4l2-ioctl.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <media/soc_camera.h>
#include <linux/platform_device.h>
#include <plat/rk_camera.h>

//#ifndef DEBUG
//#define DEBUG
//#endif

#ifdef DEBUG
#define DBG(format, ...) \
		printk(KERN_INFO "ADV7180: " format "\n", ## __VA_ARGS__)
#else
#define DBG(format, ...)
#endif

#define DRIVER_NAME "adv7180"

#define ADV7180_INPUT_CONTROL_REG			0x00
#define ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM	0x00
#define ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM_PED 0x10
#define ADV7180_INPUT_CONTROL_AD_PAL_N_NTSC_J_SECAM	0x20
#define ADV7180_INPUT_CONTROL_AD_PAL_N_NTSC_M_SECAM	0x30
#define ADV7180_INPUT_CONTROL_NTSC_J			0x40
#define ADV7180_INPUT_CONTROL_NTSC_M			0x50
#define ADV7180_INPUT_CONTROL_PAL60			0x60
#define ADV7180_INPUT_CONTROL_NTSC_443			0x70
#define ADV7180_INPUT_CONTROL_PAL_BG			0x80
#define ADV7180_INPUT_CONTROL_PAL_N			0x90
#define ADV7180_INPUT_CONTROL_PAL_M			0xa0
#define ADV7180_INPUT_CONTROL_PAL_M_PED			0xb0
#define ADV7180_INPUT_CONTROL_PAL_COMB_N		0xc0
#define ADV7180_INPUT_CONTROL_PAL_COMB_N_PED		0xd0
#define ADV7180_INPUT_CONTROL_PAL_SECAM			0xe0
#define ADV7180_INPUT_CONTROL_PAL_SECAM_PED		0xf0

#define ADV7180_EXTENDED_OUTPUT_CONTROL_REG		0x04
#define ADV7180_EXTENDED_OUTPUT_CONTROL_NTSCDIS		0xC4// 0xCC

#define ADV7180_AUTODETECT_ENABLE_REG			0x07
#define ADV7180_AUTODETECT_DEFAULT			0x7f

#define ADV7180_ADI_CTRL_REG				0x0e
#define ADV7180_ADI_CTRL_IRQ_SPACE			0x20

#define ADV7180_STATUS1_REG				0x10
#define ADV7180_STATUS1_IN_LOCK		0x01
#define ADV7180_STATUS1_AUTOD_MASK	0x70
#define ADV7180_STATUS1_AUTOD_NTSM_M_J	0x00
#define ADV7180_STATUS1_AUTOD_NTSC_4_43 0x10
#define ADV7180_STATUS1_AUTOD_PAL_M	0x20
#define ADV7180_STATUS1_AUTOD_PAL_60	0x30
#define ADV7180_STATUS1_AUTOD_PAL_B_G	0x40
#define ADV7180_STATUS1_AUTOD_SECAM	0x50
#define ADV7180_STATUS1_AUTOD_PAL_COMB	0x60
#define ADV7180_STATUS1_AUTOD_SECAM_525	0x70

#define ADV7180_IDENT_REG 0x11
#define ADV7180_ID_7180 0x1c

#define ADV7180_ICONF1_ADI		0x40
#define ADV7180_ICONF1_ACTIVE_LOW	0x01
#define ADV7180_ICONF1_PSYNC_ONLY	0x10
#define ADV7180_ICONF1_ACTIVE_TO_CLR	0xC0

#define ADV7180_IRQ1_LOCK	0x01
#define ADV7180_IRQ1_UNLOCK	0x02
#define ADV7180_ISR1_ADI	0x42
#define ADV7180_ICR1_ADI	0x43
#define ADV7180_IMR1_ADI	0x44
#define ADV7180_IMR2_ADI	0x48
#define ADV7180_IRQ3_AD_CHANGE	0x08
#define ADV7180_ISR3_ADI	0x4A
#define ADV7180_ICR3_ADI	0x4B
#define ADV7180_IMR3_ADI	0x4C
#define ADV7180_IMR4_ADI	0x50

#define ADV7180_POLARITY_REG		0x37
#define ADV7180_PCLK_NORMAIL	(1 << 0)
#define ADV7180_PFIEID_LOW		(1 << 3)
#define ADV7180_PVSYNC_LOW		(1 << 5)
#define ADV7180_PHSYNC_LOW		(1 << 7)

#define ADV7180_VS_FIELD_PIN_REG	0x58
#define ADV7180_VS_OUTPUT	0x01

struct adv7180_state {
	struct v4l2_subdev	sd;
	struct work_struct	work;
	struct mutex		mutex; /* mutual excl. when accessing chip */
	int			irq;
	v4l2_std_id		curr_norm;
	bool			autodetect;
	struct soc_camera_device *icd;
};

static v4l2_std_id adv7180_std_to_v4l2(u8 status1)
{
	switch (status1 & ADV7180_STATUS1_AUTOD_MASK) {
	case ADV7180_STATUS1_AUTOD_NTSM_M_J:
		return V4L2_STD_NTSC;
	case ADV7180_STATUS1_AUTOD_NTSC_4_43:
		return V4L2_STD_NTSC_443;
	case ADV7180_STATUS1_AUTOD_PAL_M:
		return V4L2_STD_PAL_M;
	case ADV7180_STATUS1_AUTOD_PAL_60:
		return V4L2_STD_PAL_60;
	case ADV7180_STATUS1_AUTOD_PAL_B_G:
		return V4L2_STD_PAL;
	case ADV7180_STATUS1_AUTOD_SECAM:
		return V4L2_STD_SECAM;
	case ADV7180_STATUS1_AUTOD_PAL_COMB:
		return V4L2_STD_PAL_Nc | V4L2_STD_PAL_N;
	case ADV7180_STATUS1_AUTOD_SECAM_525:
		return V4L2_STD_SECAM;
	default:
		return V4L2_STD_UNKNOWN;
	}
}

static int v4l2_std_to_adv7180(v4l2_std_id std)
{
	if (std == V4L2_STD_PAL_60)
		return ADV7180_INPUT_CONTROL_PAL60;
	if (std == V4L2_STD_NTSC_443)
		return ADV7180_INPUT_CONTROL_NTSC_443;
	if (std == V4L2_STD_PAL_N)
		return ADV7180_INPUT_CONTROL_PAL_N;
	if (std == V4L2_STD_PAL_M)
		return ADV7180_INPUT_CONTROL_PAL_M;
	if (std == V4L2_STD_PAL_Nc)
		return ADV7180_INPUT_CONTROL_PAL_COMB_N;

	if (std & V4L2_STD_PAL)
		return ADV7180_INPUT_CONTROL_PAL_BG;
	if (std & V4L2_STD_NTSC)
		return ADV7180_INPUT_CONTROL_NTSC_M;
	if (std & V4L2_STD_SECAM)
		return ADV7180_INPUT_CONTROL_PAL_SECAM;

	return -EINVAL;
}

static u32 adv7180_status_to_v4l2(u8 status1)
{
	if (!(status1 & ADV7180_STATUS1_IN_LOCK))
		return V4L2_IN_ST_NO_SIGNAL;

	return 0;
}

static int __adv7180_status(struct i2c_client *client, u32 *status,
	v4l2_std_id *std)
{
	int status1 = i2c_smbus_read_byte_data(client, ADV7180_STATUS1_REG);

	if (status1 < 0)
		return status1;

	if (status)
		*status = adv7180_status_to_v4l2(status1);
	if (std)
		*std = adv7180_std_to_v4l2(status1);

	return 0;
}

static inline struct adv7180_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct adv7180_state, sd);
}

static void adv7180_pwr_ctl(struct soc_camera_device *icd, bool onoff)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	int enable;
	
	if(onoff == 0)
		enable = 1;
	else
		enable = 0;
		
	if (icl->powerdown) {
		icl->powerdown(icd->pdev, enable);
		msleep(10);
	}
}

static int adv7180_querystd(struct v4l2_subdev *sd, v4l2_std_id *std)
{
	struct adv7180_state *state = to_state(sd);
	int err = mutex_lock_interruptible(&state->mutex);
	if (err)
		return err;

	/* when we are interrupt driven we know the state */
	if (!state->autodetect || state->irq > 0)
		*std = state->curr_norm;
	else
		err = __adv7180_status(v4l2_get_subdevdata(sd), NULL, std);

	mutex_unlock(&state->mutex);
	return err;
}

static int adv7180_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	struct adv7180_state *state = to_state(sd);
	int ret = mutex_lock_interruptible(&state->mutex);
	if (ret)
		return ret;

	ret = __adv7180_status(v4l2_get_subdevdata(sd), status, NULL);
	mutex_unlock(&state->mutex);
	return ret;
}

static int adv7180_v412_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct soc_camera_device *icd = client->dev.platform_data;
	struct adv7180_state *state = to_state(sd);
    int ret;
    
	DBG("%s val %d", __FUNCTION__, val);
	adv7180_pwr_ctl(icd, 1);
	
	/* Initialize adv7180 */
	/* Enable autodetection */
	ret = i2c_smbus_write_byte_data(client, ADV7180_INPUT_CONTROL_REG,
		ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM_PED);
	if (ret < 0)
		goto err_unreg_subdev;

	ret = i2c_smbus_write_byte_data(client, ADV7180_AUTODETECT_ENABLE_REG,
		ADV7180_AUTODETECT_DEFAULT);
	if (ret < 0)
		goto err_unreg_subdev;

	/* ITU-R BT.656-4 compatible */
	ret = i2c_smbus_write_byte_data(client,
		ADV7180_EXTENDED_OUTPUT_CONTROL_REG,
		ADV7180_EXTENDED_OUTPUT_CONTROL_NTSCDIS);
	if (ret < 0)
		goto err_unreg_subdev;

	/* set vs/field pin output vsync */
	ret = i2c_smbus_write_byte_data(client,
		ADV7180_VS_FIELD_PIN_REG,
		ADV7180_VS_OUTPUT);
	if (ret < 0)
		goto err_unreg_subdev;
	
	/* set vs/hs polarity */
//	ret = i2c_smbus_write_byte_data(client,
//		ADV7180_POLARITY_REG,
//		ADV7180_PCLK_NORMAIL | ADV7180_PVSYNC_LOW | ADV7180_PHSYNC_LOW);
//	if (ret < 0)
//		goto err_unreg_subdev;
	
	msleep(100);
	/* read current norm */
	__adv7180_status(client, NULL, &state->curr_norm);

	return 0;
	
err_unreg_subdev:
	return ret;
}

static long adv7180_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
    struct soc_camera_device *icd = client->dev.platform_data;
	DBG("%s cmd %x", __FUNCTION__, cmd);
	if(cmd == RK29_CAM_SUBDEV_DEACTIVATE)
		adv7180_pwr_ctl(icd, 0);
	return 0;
}

static int adv7180_g_chip_ident(struct v4l2_subdev *sd,
	struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_ADV7180, 0);
}

static int adv7180_s_std(struct v4l2_subdev *sd, v4l2_std_id std)
{
	struct adv7180_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = mutex_lock_interruptible(&state->mutex);
	if (ret)
		return ret;

	/* all standards -> autodetect */
	if (std == V4L2_STD_ALL) {
		ret = i2c_smbus_write_byte_data(client,
			ADV7180_INPUT_CONTROL_REG,
			ADV7180_INPUT_CONTROL_AD_PAL_BG_NTSC_J_SECAM);
		if (ret < 0)
			goto out;

		__adv7180_status(client, NULL, &state->curr_norm);
		state->autodetect = true;
	} else {
		ret = v4l2_std_to_adv7180(std);
		if (ret < 0)
			goto out;

		ret = i2c_smbus_write_byte_data(client,
			ADV7180_INPUT_CONTROL_REG, ret);
		if (ret < 0)
			goto out;

		state->curr_norm = std;
		state->autodetect = false;
	}
	ret = 0;
out:
	mutex_unlock(&state->mutex);
	return ret;
}

struct adv7180_datafmt {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
};

struct adv7180_picsize {
	unsigned int width;
	unsigned int height;
};

static const struct adv7180_picsize adv7180_pic_sizes[] = {
	//Default
	{720, 480},
	//NTSC
	#ifdef CONFIG_ARCH_RK30
	{720, 506},
	#else
	{720, 480},
	#endif
	//PAL
	{720, 576},
};

static const struct adv7180_datafmt adv7180_colour_fmts[] = {
    {V4L2_MBUS_FMT_UYVY8_2X8, V4L2_COLORSPACE_SMPTE170M},
};

static int adv7180_set_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
	struct adv7180_state *state = to_state(sd);
	int index = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	DBG("%s width %d height %d code 0x%x colorspace 0x%x",
		__FUNCTION__, mf->width, mf->height, mf->code, mf->colorspace);
		
	__adv7180_status(client, NULL, &state->curr_norm);	
	
	if(state->curr_norm == V4L2_STD_PAL)
		index = 2;
	else if(state->curr_norm == V4L2_STD_NTSC)
		index = 1;
	else
		index = 0;
	
	mf->width	= adv7180_pic_sizes[index].width;
	mf->height	= adv7180_pic_sizes[index].height;
	return 0;
}

static int adv7180_get_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{
	mf->width	= adv7180_pic_sizes[0].width;
	mf->height	= adv7180_pic_sizes[0].height;
	mf->code	=  adv7180_colour_fmts[0].code;
	mf->colorspace	= adv7180_colour_fmts[0].colorspace;
//	mf->pixelformat = V4L2_PIX_FMT_UYVY;
	mf->field	= V4L2_FIELD_NONE;
	DBG("%s width %d height %d code 0x%x colorspace 0x%x",
		__FUNCTION__, mf->width, mf->height, mf->code, mf->colorspace);
	return 0;
}

static int adv7180_try_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf)
{	
	int i;
	
	DBG("%s width %d height %d code 0x%x colorspace 0x%x",
		__FUNCTION__, mf->width, mf->height, mf->code, mf->colorspace);
	//If input picture size is larger than supported picture size, just query max supported picture size.
	i = 2;
	if(mf->width > adv7180_pic_sizes[i].width || mf->height > adv7180_pic_sizes[i].height) {
		mf->width = adv7180_pic_sizes[i].width;
		mf->height = adv7180_pic_sizes[i].height;
		return 0;
	}
	else
		return 0;
//	for(i = 0; i < ARRAY_SIZE(adv7180_colour_fmts); i++)
//	{
//		if( mf->width == adv7180_colour_fmts[i].width && 
//			mf->height == adv7180_colour_fmts[i].height &&
//			mf->code == adv7180_colour_fmts[i].code )
//			return 0;
//	}
	
	return -EINVAL;
}

static int adv7180_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned int index,
			    enum v4l2_mbus_pixelcode *code)
{
	DBG("%s index %d", __FUNCTION__, index);
	
	if (index >= ARRAY_SIZE(adv7180_colour_fmts))
		return -EINVAL;

	*code = adv7180_colour_fmts[index].code;
	return 0;
}

static int adv7180_enum_mbus_pixelfmt(struct v4l2_subdev *sd, unsigned int index,
			    int *code)
{
	DBG("%s index %d", __FUNCTION__, index);
	
	if(index > 0)
		return -EINVAL;
	
	*code = V4L2_PIX_FMT_NV16;
	return 0;
}

static int adv7180_set_bus_param(struct soc_camera_device *icd, unsigned long flags)
{
    return 0;
}

#define SENSOR_BUS_PARAM  (SOCAM_MASTER | SOCAM_PCLK_SAMPLE_RISING|\
                          SOCAM_HSYNC_ACTIVE_LOW | SOCAM_VSYNC_ACTIVE_LOW |\
                          SOCAM_DATA_ACTIVE_HIGH | SOCAM_DATAWIDTH_8  |SOCAM_MCLK_24MHZ)
                          
static unsigned long adv7180_query_bus_param(struct soc_camera_device *icd)
{
	struct soc_camera_link *icl = to_soc_camera_link(icd);
	unsigned long flags = SENSOR_BUS_PARAM;
	
	return soc_camera_apply_sensor_flags(icl, flags);
}

static int adv7180_suspend(struct soc_camera_device *icd, pm_message_t pm_msg)
{
	DBG("%s", __FUNCTION__);
	if (pm_msg.event == PM_EVENT_SUSPEND) {
		adv7180_pwr_ctl(icd, 0);
	}
	return 0;
}

static int adv7180_resume(struct soc_camera_device *icd)
{
	DBG("%s", __FUNCTION__);
	adv7180_pwr_ctl(icd, 1);
	return 0;
}

static struct soc_camera_ops adv7180_sensor_ops =
{
	.suspend            = adv7180_suspend,
	.resume 			= adv7180_resume,
	.set_bus_param		= adv7180_set_bus_param,
	.query_bus_param	= adv7180_query_bus_param,
};

static const struct v4l2_subdev_video_ops adv7180_video_ops = {
	.querystd = adv7180_querystd,
	.g_input_status = adv7180_g_input_status,
	.s_mbus_fmt = adv7180_set_mbus_fmt,
	.g_mbus_fmt = adv7180_get_mbus_fmt,
	.try_mbus_fmt = adv7180_try_mbus_fmt,
	.enum_mbus_fmt = adv7180_enum_mbus_fmt,
//	.enum_mbus_pixelfmt = adv7180_enum_mbus_pixelfmt,
};

static const struct v4l2_subdev_core_ops adv7180_core_ops = {
	.init = adv7180_v412_init,
	.ioctl = adv7180_ioctl,
	.g_chip_ident = adv7180_g_chip_ident,
	.s_std = adv7180_s_std,
};

static const struct v4l2_subdev_ops adv7180_ops = {
	.core = &adv7180_core_ops,
	.video = &adv7180_video_ops,
};

static void adv7180_work(struct work_struct *work)
{
	struct adv7180_state *state = container_of(work, struct adv7180_state,
		work);
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	u8 isr3;

	mutex_lock(&state->mutex);
	i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG,
		ADV7180_ADI_CTRL_IRQ_SPACE);
	isr3 = i2c_smbus_read_byte_data(client, ADV7180_ISR3_ADI);
	/* clear */
	i2c_smbus_write_byte_data(client, ADV7180_ICR3_ADI, isr3);
	i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG, 0);

	if (isr3 & ADV7180_IRQ3_AD_CHANGE && state->autodetect)
		__adv7180_status(client, NULL, &state->curr_norm);
	mutex_unlock(&state->mutex);

	enable_irq(state->irq);
}

static irqreturn_t adv7180_irq(int irq, void *devid)
{
	struct adv7180_state *state = devid;

	schedule_work(&state->work);

	disable_irq_nosync(state->irq);

	return IRQ_HANDLED;
}

/*
 * Generic i2c probe
 * concerning the addresses: i2c wants 7 bit (without the r/w bit), so '>>1'
 */

static __devinit int adv7180_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct adv7180_state *state;
	struct v4l2_subdev *sd;
	struct soc_camera_device *icd = client->dev.platform_data;
	int ret;
	
	#ifndef CONFIG_ARCH_RK29
	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;
	#endif
	
	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

	state = kzalloc(sizeof(struct adv7180_state), GFP_KERNEL);
	if (state == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	state->irq = client->irq;
	INIT_WORK(&state->work, adv7180_work);
	mutex_init(&state->mutex);
	state->autodetect = true;
	sd = &state->sd;
	v4l2_i2c_subdev_init(sd, client, &adv7180_ops);

	state->icd = icd;
	icd->ops = &adv7180_sensor_ops;
	icd->user_width = adv7180_pic_sizes[0].width;
	icd->user_height = adv7180_pic_sizes[0].height;
	icd->colorspace = adv7180_colour_fmts[0].colorspace;
	
	adv7180_pwr_ctl(icd, 1);
	ret = 0;
	ret = i2c_smbus_read_byte_data(client, ADV7180_IDENT_REG);
	if (ret != ADV7180_ID_7180)
		goto err_unreg_subdev;
	
	/* register for interrupts */
	if (state->irq > 0) {
		ret = request_irq(state->irq, adv7180_irq, 0, DRIVER_NAME,
			state);
		if (ret)
			goto err_unreg_subdev;

		ret = i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG,
			ADV7180_ADI_CTRL_IRQ_SPACE);
		if (ret < 0)
			goto err_unreg_subdev;

		/* config the Interrupt pin to be active low */
		ret = i2c_smbus_write_byte_data(client, ADV7180_ICONF1_ADI,
			ADV7180_ICONF1_ACTIVE_LOW | ADV7180_ICONF1_PSYNC_ONLY);
		if (ret < 0)
			goto err_unreg_subdev;

		ret = i2c_smbus_write_byte_data(client, ADV7180_IMR1_ADI, 0);
		if (ret < 0)
			goto err_unreg_subdev;

		ret = i2c_smbus_write_byte_data(client, ADV7180_IMR2_ADI, 0);
		if (ret < 0)
			goto err_unreg_subdev;

		/* enable AD change interrupts interrupts */
		ret = i2c_smbus_write_byte_data(client, ADV7180_IMR3_ADI,
			ADV7180_IRQ3_AD_CHANGE);
		if (ret < 0)
			goto err_unreg_subdev;

		ret = i2c_smbus_write_byte_data(client, ADV7180_IMR4_ADI, 0);
		if (ret < 0)
			goto err_unreg_subdev;

		ret = i2c_smbus_write_byte_data(client, ADV7180_ADI_CTRL_REG,
			0);
		if (ret < 0)
			goto err_unreg_subdev;
	}
	
	adv7180_pwr_ctl(icd, 0);
	
	v4l_info(client, "chip probe success @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);
	
	return 0;

err_unreg_subdev:
	adv7180_pwr_ctl(icd, 0);
	mutex_destroy(&state->mutex);
	v4l2_device_unregister_subdev(sd);
	kfree(state);
err:
	printk(KERN_ERR DRIVER_NAME ": Failed to probe: %d\n", ret);
	return ret;
}

static __devexit int adv7180_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct adv7180_state *state = to_state(sd);

	if (state->irq > 0) {
		free_irq(client->irq, state);
		if (cancel_work_sync(&state->work)) {
			/*
			 * Work was pending, therefore we need to enable
			 * IRQ here to balance the disable_irq() done in the
			 * interrupt handler.
			 */
			enable_irq(state->irq);
		}
	}

	mutex_destroy(&state->mutex);
	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id adv7180_id[] = {
	{DRIVER_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, adv7180_id);

static struct i2c_driver adv7180_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= DRIVER_NAME,
	},
	.probe		= adv7180_probe,
	.remove		= __devexit_p(adv7180_remove),
	.id_table	= adv7180_id,
};

static __init int adv7180_init(void)
{
	return i2c_add_driver(&adv7180_driver);
}

static __exit void adv7180_exit(void)
{
	i2c_del_driver(&adv7180_driver);
}

module_init(adv7180_init);
module_exit(adv7180_exit);

MODULE_DESCRIPTION("Analog Devices ADV7180 video decoder driver");
MODULE_AUTHOR("Mocean Laboratories");
MODULE_LICENSE("GPL v2");

