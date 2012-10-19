/* drivers/input/sensors/access/kxtik.c
 *
 * Copyright (C) 2012-2015 ROCKCHIP.
 * Author: luowei <lw@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <mach/gpio.h>
#include <mach/board.h> 
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/sensor-dev.h>

#if 0
#define SENSOR_DEBUG_TYPE SENSOR_TYPE_LIGHT
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

#define CONVERSION_TIME_MS		500

#define LSL29023_ADDR_COM1	0x00
#define LSL29023_ADDR_COM2	0x01
#define LSL29023_ADDR_DATA_MSB	0x03
#define LSL29023_ADDR_DATA_LSB	0x02

#define COMMMAND1_OPMODE_SHIFT		5
#define COMMMAND1_OPMODE_MASK		(7 << COMMMAND1_OPMODE_SHIFT)
#define COMMMAND1_OPMODE_POWER_DOWN	0
#define COMMMAND1_OPMODE_ALS_ONCE	5

#define COMMANDII_RESOLUTION_SHIFT	2
#define COMMANDII_RESOLUTION_MASK	(0x3 << COMMANDII_RESOLUTION_SHIFT)

#define COMMANDII_RANGE_SHIFT		0
#define COMMANDII_RANGE_MASK		(0x3 << COMMANDII_RANGE_SHIFT)

#define COMMANDII_SCHEME_SHIFT		7
#define COMMANDII_SCHEME_MASK		(0x1 << COMMANDII_SCHEME_SHIFT)

#define LSL29023_COM1_VALUE	0xA7	// (GAIN1:GAIN0)=10, (IT_T1:IT_TO)=01,WMD=1,SD=1,
#define LSL29023_COM2_VALUE	0xA0	//100ms

#define LSL29023_CLOSE	0x00

#define LSL29023_CHIP_RANGE      1000
#define LSL29023_ADC_BIT         16

/****************operate according to sensor chip:start************/

static bool lsl29023_write_data(struct i2c_client *client, u8 reg,
	u8 val, u8 mask, u8 shift)
{
	u8 regval;
	int ret = 0;
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	

	regval = sensor_read_reg(client, reg);
	regval &= ~mask;
	regval |= val << shift;

//	sensor->client->addr = reg;		
	sensor->ops->ctrl_data = regval;
	ret = sensor_write_reg(client, reg, regval);
	DBG("%s, reg:%d, regval:%d\n", __FUNCTION__, reg, regval);
	if (ret) {
		dev_err(&client->dev, "Write to device fails status %x\n", ret);
		return false;
	}
	return true;
}

static bool lsl29023_set_range(struct i2c_client *client, unsigned long range,
		unsigned int *new_range)
{
	unsigned long supp_ranges[] = {1000, 4000, 16000, 64000};
	int i;

	for (i = 0; i < (ARRAY_SIZE(supp_ranges) -1); ++i) {
		if (range <= supp_ranges[i])
			break;
	}
	*new_range = (unsigned int)supp_ranges[i];
	DBG("%s, i:%d\n", __FUNCTION__, i);

	return lsl29023_write_data(client, LSL29023_ADDR_COM2,
		i, COMMANDII_RANGE_MASK, COMMANDII_RANGE_SHIFT);
}

static bool lsl29023_set_resolution(struct i2c_client *client,
			unsigned long adcbit, unsigned int *conf_adc_bit)
{
	unsigned long supp_adcbit[] = {16, 12, 8, 4};
	int i;

	for (i = 0; i < (ARRAY_SIZE(supp_adcbit)); ++i) {
		if (adcbit == supp_adcbit[i])
			break;
	}
	*conf_adc_bit = (unsigned int)supp_adcbit[i];

	return lsl29023_write_data(client, LSL29023_ADDR_COM2,
		i, COMMANDII_RESOLUTION_MASK, COMMANDII_RESOLUTION_SHIFT);
}


static int sensor_active(struct i2c_client *client, int enable, int rate)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;
	int status = 0;
	
//	sensor->client->addr = sensor->ops->ctrl_reg;	
	sensor->ops->ctrl_data = sensor_read_reg(client, sensor->ops->ctrl_reg);
	
	//register setting according to chip datasheet		
	if(!enable)
	{	
		status = LSL29023_CLOSE;	//LSL29023	
		sensor->ops->ctrl_data |= status;	
	}
	else
	{
		status = ~LSL29023_CLOSE;	//LSL29023
		sensor->ops->ctrl_data &= status;
	}

	DBG("%s:reg=0x%x,reg_ctrl=0x%x,enable=%d\n",__func__,sensor->ops->ctrl_reg, sensor->ops->ctrl_data, enable);
	result = sensor_write_reg(client, sensor->ops->ctrl_reg, sensor->ops->ctrl_data);
	if(result)
		printk("%s:fail to active sensor\n",__func__);
	
	return result;

}


static int sensor_init(struct i2c_client *client)
{	
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;
	int new_adc_bit;
	unsigned int new_range;
	
	result = sensor->ops->active(client,0,0);
	if(result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
	
	sensor->status_cur = SENSOR_OFF;
	
#if 0
	sensor->client->addr = sensor->ops->ctrl_reg;		
	sensor->ops->ctrl_data = LSL29023_COM1_VALUE;	
	result = sensor_write_reg_normal(client, sensor->ops->ctrl_data);

	sensor->client->addr = LSL29023_ADDR_COM2;	
	result = sensor_write_reg_normal(client, LSL29023_COM2_VALUE);
	if(result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
#endif
	result = lsl29023_set_range(client, LSL29023_CHIP_RANGE, &new_range);
	if (result)
		result = lsl29023_set_resolution(client, LSL29023_ADC_BIT,
				&new_adc_bit);
				
	if(!result)
	{
		printk("%s:line=%d,error\n",__func__,__LINE__);
		return result;
	}
	
	
	return result;
}


static void light_report_value(struct input_dev *input, int data)
{
	unsigned char index = 0;
	if(data <= 10){
		index = 0;goto report;
	}
	else if(data <= 160){
		index = 1;goto report;
	}
	else if(data <= 225){
		index = 2;goto report;
	}
	else if(data <= 320){
		index = 3;goto report;
	}
	else if(data <= 640){
		index = 4;goto report;
	}
	else if(data <= 1280){
		index = 5;goto report;
	}
	else if(data <= 2600){
		index = 6;goto report;
	}
	else{
		index = 7;goto report;
	}

report:
	DBG("LSL29023 report data=%d,index = %d\n",data,index);
	input_report_abs(input, ABS_MISC, index);
	input_sync(input);
}


static int sensor_report_value(struct i2c_client *client)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);	
	int result = 0;
	char msb = 0, lsb = 0;
	bool status;

	/* Set mode */
	status = lsl29023_write_data(client, LSL29023_ADDR_COM1,
			COMMMAND1_OPMODE_ALS_ONCE, COMMMAND1_OPMODE_MASK, COMMMAND1_OPMODE_SHIFT);
	if (!status) {
		dev_err(&client->dev, "Error in setting operating mode\n");
		return -EBUSY;
	}

	msleep(CONVERSION_TIME_MS);
	
//	sensor->client->addr = LSL29023_ADDR_DATA_LSB;
	lsb = sensor_read_reg(sensor->client, LSL29023_ADDR_DATA_LSB);
//	sensor->client->addr = LSL29023_ADDR_DATA_MSB;
	msb = sensor_read_reg(sensor->client, LSL29023_ADDR_DATA_MSB);
	result = ((msb << 8) | lsb) & 0xffff;
	
	DBG("%s:result=%d, lsb:%d, msb:%d\n",__func__,result, lsb, msb);
	light_report_value(sensor->input_dev, result);

	if((sensor->pdata->irq_enable)&& (sensor->ops->int_status_reg >= 0))	//read sensor intterupt status register
	{
		
		result= sensor_read_reg(client, sensor->ops->int_status_reg);
		if(result)
		{
			printk("%s:fail to clear sensor int status,ret=0x%x\n",__func__,result);
		}
	}
	
	return result;
}


struct sensor_operate light_ops = {
	.name				= "lsl2903",
	.type				= SENSOR_TYPE_LIGHT,	//sensor type and it should be correct
	.id_i2c				= LIGHT_ID_LSL29023,	//i2c id number
	.read_reg			= LSL29023_ADDR_DATA_LSB,	//read data
	.read_len			= 2,			//data length
	.id_reg				= SENSOR_UNKNOW_DATA,	//read device id from this register
	.id_data 			= SENSOR_UNKNOW_DATA,	//device id
	.precision			= LSL29023_ADC_BIT,			//8 bits
	.ctrl_reg 			= LSL29023_ADDR_COM1,	//enable or disable 
	.int_status_reg 		= SENSOR_UNKNOW_DATA,	//intterupt status register
	.range				= {0,10},		//range
	.trig				= SENSOR_UNKNOW_DATA,		
	.active				= sensor_active,	
	.init				= sensor_init,
	.report				= sensor_report_value,
};

/****************operate according to sensor chip:end************/

//function name should not be changed
struct sensor_operate *light_get_ops(void)
{
	return &light_ops;
}

EXPORT_SYMBOL(light_get_ops);

static int __init light_init(void)
{
	struct sensor_operate *ops = light_get_ops();
	int result = 0;
	int type = ops->type;
	result = sensor_register_slave(type, NULL, NULL, light_get_ops);
	printk("%s\n",__func__);
	return result;
}

static void __exit light_exit(void)
{
	struct sensor_operate *ops = light_get_ops();
	int type = ops->type;
	sensor_unregister_slave(type, NULL, NULL, light_get_ops);
}


module_init(light_init);
module_exit(light_exit);


