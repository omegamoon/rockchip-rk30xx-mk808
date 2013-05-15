/* drivers/input/ts/chips/gt8110.c
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
#include <linux/input/mt.h>
#include <mach/gpio.h>
#include <mach/board.h> 
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif	 
#include <linux/ts-auto.h>
#include <linux/rk_board_id.h>	 
	 
#if 0
#define DBG(x...)  printk(x)
#else
#define DBG(x...)
#endif

#define PEN_DOWN 1
#define PEN_RELEASE 0
#define PEN_DOWN_UP 2	//from down to up


#define GT8110_ID_REG		0x00
#define GT8110_DATA_REG		0x00

struct ts_operate ts_gt8110_ops;


#if defined(CONFIG_RK_BOARD_ID)
extern enum rk_board_id rk_get_board_id(void);
#else
static enum rk_board_id rk_get_board_id(void)
{
	return -1;
}
#endif

/****************operate according to ts chip:start************/

static int ts_active(struct ts_private_data *ts, int enable)
{
	unsigned char buf_suspend[2] = {0x38, 0x56};		//suspend cmd
	int result = 0;

	if(enable)
	{
		gpio_direction_output(ts->pdata->reset_pin, GPIO_LOW);
		mdelay(10);
		gpio_direction_output(ts->pdata->reset_pin, GPIO_HIGH);
		msleep(200);
	}
	else
	{
		result = ts_bulk_write(ts, (unsigned short )buf_suspend[0], 2, &buf_suspend[1]);
		if(result < 0)
		{
			printk("%s:fail to init ts\n",__func__);
			return result;
		}
		
		gpio_direction_output(ts->pdata->reset_pin, GPIO_LOW);	
	}
		
	
	return result;
}

static int ts_init(struct ts_private_data *ts)
{
	int irq_pin = irq_to_gpio(ts->pdata->irq);
	char version_data[18] = {240};
	char tp_config[10] = {0x65};
	char init_data_c1014[95] = {
	0x65,0x02,0x00,0x10,0x00,0x10,0x0A,0x62,0x4A,0x00,
	0x0F,0x28,0x02,0x10,0x10,0x00,0x00,0x20,0x00,0x00,
	0x10,0x10,0x10,0x00,0x37,0x00,0x00,0x00,0x01,0x02,
	0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,
	0x0D,0xFF,0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
	0x07,0x08,0x09,0x0A,0x0B,0x0C,0xFF,0xFF,0xFF,0x00,
	0x00,0x3C,0x64,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00
        };

    char init_data_c8002[95] = {
	0x65,0x01,0x00,0x10,0x00,0x10,0x05,0x6E,0x0A,0x00,
	0x0F,0x1E,0x02,0x08,0x10,0x00,0x00,0x27,0x00,0x00,
	0x50,0x10,0x10,0x11,0x37,0x00,0x00,0x00,0x01,0x02,
	0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0xFF,
	0xFF,0xFF,0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06,
	0x07,0x08,0x09,0x0A,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,
	0x00,0x50,0x64,0x50,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x20
        };
	int result = 0, i = 0;
	int board_id; 
	//read version	
	result = ts_bulk_read(ts, (unsigned short)version_data[0], 16, &version_data[1]);
	if(result < 0)
	{
		printk("%s:fail to init ts\n",__func__);
		return result;
	}
	version_data[17]='\0';

//read config
	result = ts_bulk_read(ts, (unsigned short)tp_config[0], sizeof(tp_config) - 1, &tp_config[1]);
	if(result < 0)
	{
		printk("%s:read ts config fail\n",__func__);
		return result;
	}

	printk("%s:%s version is %s, config:%d\n",__func__,ts->ops->name, version_data, tp_config[6]);

#if defined(CONFIG_RK_BOARD_ID)
	board_id = rk_get_board_id();
	if ( board_id ==  BOARD_ID_C8002 )
	{
		ts_gt8110_ops.ts_id = TS_ID_GT8005;
		result = ts_bulk_write(ts, (unsigned short )init_data_c8002[0], 94, &init_data_c8002[1]);
		if(result < 0)
		{
			printk("%s:fail to init ts\n",__func__);
			return result;
		}
			
	}
	else
	{
		ts_gt8110_ops.ts_id = TS_ID_GT8110;
		result = ts_bulk_write(ts, (unsigned short )init_data_c1014[0], 94, &init_data_c1014[1]);
		if(result < 0)
		{
			printk("%s:fail to init ts\n",__func__);
			return result;
		}
		
	}			
#else
	//init some register
	if (tp_config[6] == 0x0a) {
		result = ts_bulk_write(ts, (unsigned short )init_data_c1014[0], 94, &init_data_c1014[1]);
		if(result < 0)
		{
			printk("%s:fail to init ts\n",__func__);
			return result;
		}
		ts_gt8110_ops.ts_id = TS_ID_GT8110;
	} else {
		result = ts_bulk_write(ts, (unsigned short )init_data_c8002[0], 94, &init_data_c8002[1]);
		if(result < 0)
		{
			printk("%s:fail to init ts\n",__func__);
			return result;
		}
		ts_gt8110_ops.ts_id = TS_ID_GT8005;
	}	
#endif
	result = ts_bulk_read(ts, (unsigned short)init_data_c1014[0], 94, &init_data_c1014[1]);
	if(result < 0)
	{
		printk("%s:fail to init ts\n",__func__);
		return result;
	}

	
	DBG("%s:rx:",__func__);
	for(i=0; i<95; i++)
	DBG("0x%x,",init_data_c1014[i]);

	DBG("\n");
	
	return result;
}

static bool goodix_get_status(int *p1,int*p2)
{
	bool status = PEN_DOWN;
	if((*p2==PEN_DOWN) && (*p1==PEN_RELEASE))
	{
		*p2 = PEN_DOWN_UP; //¸Õ¸Õµ¯Æð
		status = PEN_RELEASE; 
	}
	else if((*p2==PEN_RELEASE) && (*p1==PEN_RELEASE))
	{
		*p2 = PEN_RELEASE;
		status = PEN_RELEASE; 
	}
	else
	{
		*p2 = PEN_DOWN;
	}
	return status;
}



static int ts_check_irq(struct ts_private_data *ts)
{
	struct ts_platform_data *pdata = ts->pdata;	
	struct ts_event *event = &ts->event;
	int gpio_level_no_int = GPIO_HIGH;
	int id = 0, i = 0;
	
	if((ts->ops->trig & IRQF_TRIGGER_LOW) || (ts->ops->trig & IRQF_TRIGGER_FALLING))
		gpio_level_no_int = GPIO_HIGH;
	else	
		gpio_level_no_int = GPIO_LOW;

	if(gpio_get_value(ts->pdata->irq) == gpio_level_no_int)	
	{
		for(i = 0; i<ts->ops->max_point; i++)
		{			
			id = i;				
			if(event->point[id].last_status != 0)
			{
				event->point[i].last_status = PEN_RELEASE;				
				input_mt_slot(ts->input_dev, event->point[i].id);				
				input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
				input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
				DBG("%s:%s press up,id=%d\n\n",__func__,ts->ops->name, event->point[i].id);
			}
		}
		
		input_sync(ts->input_dev);	
		memset(event, 0x00, sizeof(struct ts_event));	
		enable_irq(ts->irq);
	}
	else
	schedule_delayed_work(&ts->delaywork, msecs_to_jiffies(ts->ops->poll_delay_ms));

	return 0;
	
}


static int ts_report_value(struct ts_private_data *ts)
{
	struct ts_platform_data *pdata = ts->pdata;
	struct ts_event *event = &ts->event;
	unsigned char buf[54] = {0};
	int result = 0 , i = 0, j = 0, off = 0, id = 0;
	int temp = 0, num = 0;

	result = ts_bulk_read(ts, ts->ops->read_reg, ts->ops->read_len, buf);
	if(result < 0)
	{
		printk("%s:fail to init ts\n",__func__);
		return result;
	}

//	for(i=0; i<ts->ops->read_len; i++)
//	DBG("buf[%d]=0x%x\n",i,buf[i]);

	temp = (buf[1] << 8) | buf[0];

	for(i=0; i<ts->ops->max_point; i++)
	{
		if(temp & (1 << i)) 
		{	
			event->point[i].status = PEN_DOWN;
			num++;
		}
	}
	
	if(num > event->touch_point)
	event->touch_point = num;

	for(i = 0; i<event->touch_point; i++)
	{
		off = 2 + i*4;
		
		id = i;				
		event->point[id].id = id;
		
		if(goodix_get_status(&event->point[id].status,&event->point[id].last_status))
		{
			event->point[id].x = (unsigned int)(buf[off+1]<<8) + (unsigned int)buf[off+0];
			event->point[id].y = (unsigned int)(buf[off+3]<<8) + (unsigned int)buf[off+2];
		}
		
		if((event->point[id].x <= 0) || (event->point[id].x >= ts->ops->range[0]) || (event->point[id].y <= 0) || (event->point[id].y >= ts->ops->range[1]))
		{
			event->point[id].status = 0;
			continue;
		}
		//for(j=0; j<(2 + (i+1)*4); j++)
		//DBG("buf[%d]=0x%x\n",j,buf[j]);
		
		if(ts->ops->xy_swap)
		{
			swap(event->point[id].x, event->point[id].y);
		}

		if(ts->ops->x_revert)
		{
			event->point[id].x = ts->ops->range[0] - event->point[id].x;	
		}

		if(ts->ops->y_revert)
		{
			event->point[id].y = ts->ops->range[1] - event->point[id].y;
		}

		
		if(event->point[id].status==PEN_DOWN_UP)
		{
		        event->point[id].status=PEN_RELEASE;
			input_mt_slot(ts->input_dev, event->point[i].id);				
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
			DBG("%s:%s press is uping,id=%d\n\n",__func__,ts->ops->name, event->point[i].id);
			continue;
		}
		
		if(event->point[id].status==PEN_DOWN)
		{		
			input_mt_slot(ts->input_dev, event->point[id].id);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, event->point[id].id);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			if (ts_gt8110_ops.ts_id == TS_ID_GT8005) {
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, 4096 - event->point[id].x);
			} else {
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, event->point[id].x);
			}
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, event->point[id].y);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);	
	   		DBG("%s:%s press down,id=%d,x=%d,y=%d\n\n",__func__,ts->ops->name, event->point[id].id, event->point[id].x,event->point[id].y);
		}
		
		event->point[id].last_status = event->point[id].status;
	}
	
	input_sync(ts->input_dev);

	return 0;
}

static int ts_suspend(struct ts_private_data *ts)
{	
	struct ts_platform_data *pdata = ts->pdata;

	if(ts->ops->active)
		ts->ops->active(ts, 0);
	
	return 0;
}


static int ts_resume(struct ts_private_data *ts)
{
	struct ts_platform_data *pdata = ts->pdata;
	
	if(ts->ops->active)
		ts->ops->active(ts, 1);
	return 0;
}



struct ts_operate ts_gt8110_ops = {
	.name				= "gt8110",
	.slave_addr			= 0x5c,
	.ts_id				= TS_ID_GT8110,			//i2c id number
	.bus_type			= TS_BUS_TYPE_I2C,
	.reg_size			= 1,
	.id_reg				= GT8110_ID_REG,
	.id_data			= TS_UNKNOW_DATA,
	.version_reg			= TS_UNKNOW_DATA,
	.version_len			= 0,
	.version_data			= NULL,
	.read_reg			= GT8110_DATA_REG,		//read data
	.read_len			= 5*10+3+1,			//data length
	.trig				= IRQ_TYPE_LEVEL_LOW,		
	.max_point			= 10,
	.xy_swap 			= 0,
	.x_revert 			= 0,
	.y_revert			= 0,
	.range				= {4096,4096},
	.irq_enable			= 1,
	.poll_delay_ms			= 30,
	.active				= ts_active,	
	.init				= ts_init,	
	.check_irq			= ts_check_irq,
	.report 			= ts_report_value,
	.firmware			= NULL,
	.suspend			= ts_suspend,
	.resume				= ts_resume,
};

/****************operate according to ts chip:end************/

//function name should not be changed
static struct ts_operate *ts_get_ops(void)
{
	return &ts_gt8110_ops;
}


static int __init ts_gt8110_init(void)
{
	struct ts_operate *ops = ts_get_ops();
	int result = 0;
	result = ts_register_slave(NULL, NULL, ts_get_ops);	
	DBG("%s\n",__func__);
	return result;
}

static void __exit ts_gt8110_exit(void)
{
	struct ts_operate *ops = ts_get_ops();
	ts_unregister_slave(NULL, NULL, ts_get_ops);
}


subsys_initcall(ts_gt8110_init);
module_exit(ts_gt8110_exit);

