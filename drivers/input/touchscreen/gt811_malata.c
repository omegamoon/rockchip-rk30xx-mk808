/* drivers/input/touchscreen/gt811.c
 *
 * Copyright (C) 2010 - 2011 Goodix, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *Any problem,please contact andrew@goodix.com,+86 755-33338828
 *
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/irq.h>
#include <linux/syscalls.h>
#include <linux/reboot.h>
#include <linux/proc_fs.h>
#include <linux/gt811.h>
#include <linux/input/mt.h>
#include <mach/iomux.h>

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/completion.h>
#include <asm/uaccess.h>

/*--ArthurLin,2012.06.06,add switch lock for touch capacitive panel--start--*/
#if defined(CONFIG_MACH_B7007)
#define  GOODIX_MODE_VALID_CHECK
extern int getPenPressed();
#endif
/*--ArthurLin,2012.06.06--end--*/

//#define  SHUTDOWN_PORT   0 
//#define  GOODIX_I2C_NAME   "Goodix-TS"

static struct workqueue_struct *goodix_wq;
//static const char *s3c_ts_name = "Goodix811 Capacitive TouchScreen";
static const char *s3c_ts_name = "Goodix Capacitive TouchScreen";
struct i2c_client * i2c_connect_client = NULL;

static struct proc_dir_entry *goodix_proc_entry;
static short  goodix_read_version(struct goodix_ts_data *ts);	

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
int  gt811_downloader( struct goodix_ts_data *ts, unsigned char * data);
#endif
//used by firmware update CRC
unsigned int oldcrc32 = 0xFFFFFFFF;
unsigned int crc32_table[256];
unsigned int ulPolynomial = 0x04c11db7;


#define GTP_READ_BYTES   2+34   
#define   GTP_REG_COOR    0x721
#define  GTP_MAX_TOUCH     5
#define GTP_ADDR_LENGTH   2

unsigned int raw_data_ready = RAW_DATA_NON_ACTIVE;

#ifdef DEBUG
int sum = 0;
int access_count = 0;
int int_count = 0;
#endif

/*******************************************************	
Function:
	Read data from the slave
	Each read operation with two i2c_msg composition, for the first message sent from the machine address,
	Article 2 reads the address used to send and retrieve data; each message sent before the start signal
Parameters:
	client: i2c devices, including device address
	buf [0]: The first byte to read Address
	buf [1] ~ buf [len]: data buffer
	len: the length of read data
return:
	Execution messages
*********************************************************/
/*Function as i2c_master_send */
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret=-1;
	
	msgs[0].flags=!I2C_M_RD;
	msgs[0].addr=client->addr;
	msgs[0].len=2;
	msgs[0].buf=&buf[0];
	msgs[0].udelay = client->udelay;
	msgs[0].scl_rate=200 * 1000;

	msgs[1].flags=I2C_M_RD;
	msgs[1].addr=client->addr;
	msgs[1].len=len-2;
	msgs[1].buf=&buf[2];

	msgs[1].udelay = client->udelay;
	msgs[1].scl_rate=200 * 1000;
	
	ret=i2c_transfer(client->adapter,msgs, 2);
	return ret;
}

/*******************************************************	
Function:
	Write data to a slave
Parameters:
	client: i2c devices, including device address
	buf [0]: The first byte of the write address
	buf [1] ~ buf [len]: data buffer
	len: data length
return:
	Execution messages
*******************************************************/
/*Function as i2c_master_send */
static int i2c_write_bytes(struct i2c_client *client,uint8_t *data,int len)
{
	struct i2c_msg msg;
	int ret=-1;
	msg.flags=!I2C_M_RD;
	msg.addr=client->addr;
	msg.len=len;
	msg.buf=data;	

	msg.udelay = client->udelay;
	msg.scl_rate=200 * 1000;
	
	ret=i2c_transfer(client->adapter,&msg, 1);
	return ret;
}

/*******************************************************
Function:
	Send a prefix command
	
Parameters:
	ts: client private data structure
	
return:
	Results of the implementation code, 0 for normal execution
*******************************************************/
static int i2c_pre_cmd(struct goodix_ts_data *ts)
{
	int ret;
	uint8_t pre_cmd_data[2]={0};	
	pre_cmd_data[0]=0x0f;
	pre_cmd_data[1]=0xff;
	ret=i2c_write_bytes(ts->client,pre_cmd_data,2);
	return ret;
}

/*******************************************************
Function:
	Send a suffix command
	
Parameters:
	ts: client private data structure
	
return:
	Results of the implementation code, 0 for normal execution
*******************************************************/
static int i2c_end_cmd(struct goodix_ts_data *ts)
{
	int ret;
	uint8_t end_cmd_data[2]={0};	
	end_cmd_data[0]=0x80;
	end_cmd_data[1]=0x00;
	ret=i2c_write_bytes(ts->client,end_cmd_data,2);
	return ret;
}

/********************************************************************

*********************************************************************/
#ifdef COOR_TO_KEY
static int list_key(s32 x_value, s32 y_value, u8* key)
{
	s32 i;

#ifdef AREA_Y
	if (y_value <= AREA_Y)
#else
	if (x_value <= AREA_X)
#endif
	{
		return 0;
	}

	for (i = 0; i < MAX_KEY_NUM; i++)
	{
		if (abs(key_center[i][x] - x_value) < KEY_X 
		&& abs(key_center[i][y] - y_value) < KEY_Y)
		{
			(*key) |= (0x01<<i);
        	}
   	 }

    return 1;
}
#endif 

/*******************************************************
Function:
	Guitar initialization function, used to send configuration information, access to version information
Parameters:
	ts: client private data structure
return:
	Results of the implementation code, 0 for normal execution
*******************************************************/
static int goodix_init_panel(struct goodix_ts_data *ts)
{
	short ret=-1;
	uint8_t config_info[] = {
	0x06,0xA2,
	// beitai  g+f
	/*0x12,0x10,0x0E,0x0C,0x0A,0x08,0x06,0x04,0x02,0x00,
	0x04,0x44,0x14,0x44,0x24,0x44,0x34,0x44,0x44,0x44,
	0x54,0x44,0x64,0x44,0x74,0x44,0x84,0x44,0x94,0x44,
	0xA4,0x44,0xB4,0x44,0xC4,0x44,0xD4,0x44,0xE4,0x44,
	0xF4,0x44,0x1B,0x03,0x00,0x00,0x00,0x12,0x12,0x12,
	0x0F,0x0F,0x0A,0x30,0x20,0x09,0x03,0x00,0x05,0x58,
	0x02,0x00,0x04,0x00,0x00,0x49,0x4A,0x4C,0x4D,0x00,
	0x00,0x03,0x14,0x25,0x04,0x00,0x00,0x00,0x00,0x00,
	0x14,0x10,0x31,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x0C,0x30,0x25,0x28,0x14,0x00,
	0x00,0x00,0x00,0x00,0x00,0x01*/

	//beitai g+g low freq ; tian zhi yu
	0x12,0x10,0x0E,0x0C,0x0A,0x08,0x06,0x04,0x02,0x00,
	0x05,0x66,0x15,0x66,0x25,0x66,0x35,0x66,0x45,0x66,
	0x55,0x66,0x65,0x66,0x75,0x66,0x85,0x66,0x95,0x66,
	0xA5,0x66,0xB5,0x66,0xC5,0x66,0xD5,0x66,0xE5,0x66,
	0xF4,0x44,0x0B,0x13,0x80,0x80,0x80,0x1A,0x1A,0x1A,
	0x0F,0x0F,0x0A,0x48,0x2A,0x4F,0x03,0x00,0x05,0x58,
	0x02,0x00,0x04,0x00,0x00,0xF5,0xF3,0x50,0x50,0x00,
	0x00,0x26,0x20,0x25,0x08,0x00,0x00,0x00,0x00,0x00,
	0x14,0x10,0x52,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x01

	};
	
	ret = i2c_write_bytes(ts->client, config_info, sizeof(config_info)/sizeof(config_info[0]));
	if(ret < 0)
	{
		dev_info(&ts->client->dev, "GT811 Send config failed!\n");
		return ret;
	}
	ts->abs_y_max = (config_info[62]<<8) + config_info[61];
	ts->abs_x_max = (config_info[64]<<8) + config_info[63];
	ts->max_touch_num = config_info[60];
	ts->int_trigger_type = ((config_info[57]>>3)&0x01);
	dev_info(&ts->client->dev, "GT811 init info:X_MAX=%d,Y_MAX=%d,TRIG_MODE=%s\n",
	ts->abs_x_max, ts->abs_y_max, ts->int_trigger_type?"RISING EDGE":"FALLING EDGE");

	return 0;
}

/*******************************************************
FUNCTION:
	Read gt811 IC Version
Argument:
	ts:	client
return:
	0:success
       -1:error
*******************************************************/
static short  goodix_read_version(struct goodix_ts_data *ts)
{
	short ret;
	uint8_t version_data[5]={0x07,0x17,0,0};	//store touchscreen version infomation
	uint8_t version_data2[5]={0x07,0x17,0,0};	//store touchscreen version infomation

	char i = 0;
	char cpf = 0;
	memset(version_data, 0, 5);
	version_data[0]=0x07;
	version_data[1]=0x17;	

      	ret=i2c_read_bytes(ts->client, version_data, 4);
	if (ret < 0) 
		return ret;
	
	for(i = 0;i < 10;i++)
	{
		i2c_read_bytes(ts->client, version_data2, 4);
		if((version_data[2] !=version_data2[2])||(version_data[3] != version_data2[3]))
		{
			version_data[2] = version_data2[2];
			version_data[3] = version_data2[3];
			msleep(5);
			break;
		}
		msleep(5);
		cpf++;
	}

	if(cpf == 10)
	{
		ts->version = (version_data[2]<<8)+version_data[3];
		dev_info(&ts->client->dev, "GT811 Verion:0x%04x\n", ts->version);
		ret = 0;
	}
	else
	{
		dev_info(&ts->client->dev," Guitar Version Read Error: %d.%d\n",version_data[3],version_data[2]);
		ts->version = 0xffff;
		ret = -1;
	}
	
	return ret;
	
}
/******************start add by kuuga*******************/
static void gt811_irq_enable(struct goodix_ts_data *ts)
{	
	unsigned long irqflags;	
	spin_lock_irqsave(&ts->irq_lock, irqflags);
	if (ts->irq_is_disable) 
	{		
		enable_irq(ts->irq);		
		ts->irq_is_disable = 0;	
	}	
	spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

static void gt811_irq_disable(struct goodix_ts_data *ts)
{	
	unsigned long irqflags;
	spin_lock_irqsave(&ts->irq_lock, irqflags);
	if (!ts->irq_is_disable) 
	{		
		disable_irq_nosync(ts->irq);		
		ts->irq_is_disable = 1;	
	}	
	spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}


/*******************************************************
Function:
	Touch down report function.

Input:
	ts:private data.
	id:tracking id.
	x:input x.
	y:input y.
	w:input weight.
	
Output:
	None.
*******************************************************/
static void gtp_touch_down(struct goodix_ts_data* ts,s32 id,s32 x,s32 y,s32 w)
{

    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(ts->input_dev, ABS_MT_PRESSURE, w);


    //GTP_DEBUG("ID:%d, X:%d, Y:%d, W:%d", id, x, y, w);
}

/*******************************************************
Function:
	Touch up report function.

Input:
	ts:private data.
	
Output:
	None.
*******************************************************/
static void gtp_touch_up(struct goodix_ts_data* ts, s32 id)
{

    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
   // GTP_DEBUG("Touch id[%2d] release!", id);

}



static void goodix_ts_work_func(struct work_struct *work)
{
    u8  point_data[GTP_READ_BYTES] = {(u8)(GTP_REG_COOR>>8),(u8)GTP_REG_COOR,0}; 
    u8  check_sum = 0;
    u8  read_position = 0;
    u8  track_id[GTP_MAX_TOUCH];
    u8  point_index = 0;
    u8  point_tmp = 0;
    u8  touch_num = 0;
    u8  key_value = 0;
    u8  input_w = 0;
    u8 point_num = 0;
    u16 input_x = 0;
    u16 input_y = 0;
    u32 count = 0;
    u32 position = 0;	
    s32 ret = -1;

    static u8 last_point_num = 0;
    static u8 last_point_index = 0;
    u8 report_num = 0;
    u8 touch_count = 0;


    struct goodix_ts_data *ts;
    
    //GTP_DEBUG_FUNC();

    ts = container_of(work, struct goodix_ts_data, work);

    //if(ts->enter_update)
   // {
    //    goto exit_work_func;
   // }
    ret = i2c_read_bytes(ts->client, point_data, sizeof(point_data)/sizeof(point_data[0]));
    //////////////////////////////////////////
    
    if (ret < 0) 
    {
        goto exit_work_func;  
    }
    //GTP_DEBUG_ARRAY(point_data, sizeof(point_data)/sizeof(point_data[0]));
    if(point_data[GTP_ADDR_LENGTH]&0x20)
    {
        if(point_data[3]==0xF0)
        {
            //GTP_DEBUG("Reload config!");
            ret = goodix_init_panel(ts);
            ///////////////////////////////////////////////
            if (ret < 0)
            {
               // GTP_ERROR("Send config error.");
            }
            goto exit_work_func;
        }
    }
    
    point_index = point_data[2]&0x1f;
    point_tmp = point_index;
    for(position=0; (position<GTP_MAX_TOUCH)&&point_tmp; position++)
    {
        if(point_tmp&0x01)
        {
            track_id[touch_num++] = position;
        }
        point_tmp >>= 1;
    }
    point_num = position;
    //GTP_DEBUG("Touch num:%d", touch_num);

    if(touch_num)
    {
        switch(point_data[2]& 0x1f)
        {
            case 0:
                read_position = 3;
                break;
            case 1:
                for (count=2; count<9; count++)
                {
                    check_sum += (s32)point_data[count];
                }
                read_position = 9;
                break;
            case 2:
            case 3:
                for (count=2; count<14;count++)
                {
                    check_sum += (s32)point_data[count];
                }
                read_position = 14;
                break;
            //touch number more than 3 fingers
            default:
                for (count=2; count<35;count++)
                {
                    check_sum += (s32)point_data[count];
                }
                read_position = 35;
        }
        if (check_sum != point_data[read_position])
        {
            //GTP_DEBUG("Cal_chksum:%d,  Read_chksum:%d", check_sum, point_data[read_position]);
            //GTP_ERROR("Coordinate checksum error!");
            goto exit_work_func;
        }
    }
    
    key_value = point_data[3]&0x0F;
#if  0//GTP_HAVE_TOUCH_KEY
    for (count = 0; count < GTP_MAX_KEY_NUM; count++)
    {
        input_report_key(ts->input_dev, touch_key_array[count], !!(key_value&(0x01<<count)));	
    }
#endif


    report_num = (point_num>last_point_num) ? point_num : last_point_num;
    last_point_num = point_num;
    
    for(count=0,touch_count=0; count<report_num; count++)
    {
        if(point_index&(0x01<<count))
        {
            if(track_id[touch_count]!=3)
            {
                if(track_id[touch_count]<3)
                {
                    position = 4+track_id[touch_count]*5;
                }
                else
                {
                    position = 30;
                }
                input_y = (u16)(point_data[position]<<8)+(u16)point_data[position+1];
                input_x = (u16)(point_data[position+2]<<8)+(u16)point_data[position+3];
                input_w = point_data[position+4];
            }
            else
            {
                input_y = (u16)(point_data[19]<<8)+(u16)point_data[26];
                input_x = (u16)(point_data[27]<<8)+(u16)point_data[28];
                input_w = point_data[29];	
            }
            touch_count++;
            
			input_x = 1024 - input_x;
			input_y = 600 - input_y; 

			if((input_x>0)&&(input_x<100))
			{
				point_tmp = (100-input_x)>>3;
				input_x += point_tmp;
			}

			if((input_x<1024)&&(input_x>924))
			{
				point_tmp = (input_x-924)>>3;
				input_x -= point_tmp;
			}

			if((input_y>0)&&(input_y<110))
			{
				point_tmp = (110-input_y)>>3;
				input_y += point_tmp;
			}

			if((input_y>520)&&(input_y<600))
			{
				point_tmp = (input_y-520)>>3;
				input_y -= point_tmp;
			}

            gtp_touch_down(ts, count, input_x, input_y, input_w);
        }
        else if(last_point_index&(0x01<<count))
        {
            gtp_touch_up(ts, count);
        }
    }
    last_point_index = point_index;

    input_sync(ts->input_dev);

exit_work_func:
    if (ts->use_irq)
    {
        //gtp_irq_enable(ts);
		gt811_irq_enable(ts);     
    }
}


/*******************************************************	
Function:
	Response function timer
	Triggered by a timer, scheduling the work function of the touch screen operation; after re-timing
Parameters:
	timer: the timer function is associated
return:
	Timer mode, HRTIMER_NORESTART that do not automatically restart
********************************************************/
static enum hrtimer_restart goodix_ts_timer_func(struct hrtimer *timer)
{
	struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);
	queue_work(goodix_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, (POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

/*******************************************************	
Function:
	Interrupt response function
	Triggered by an interrupt, the scheduler runs the touch screen handler
********************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
	struct goodix_ts_data *ts = dev_id;

	//printk(KERN_INFO"-------------------ts_irq_handler------------------\n");
#ifndef STOP_IRQ_TYPE
	gt811_irq_disable(ts);     
#endif
	//disable_irq_nosync(ts->client->irq);
	queue_work(goodix_wq, &ts->work);
	
	return IRQ_HANDLED;
}

/*******************************************************	
Function:
	Power management gt811, gt811 allowed to sleep or to wake up
Parameters:
	on: 0 that enable sleep, wake up 1
return:
	Is set successfully, 0 for success
	Error code: -1 for the i2c error, -2 for the GPIO error;-EINVAL on error as a parameter
********************************************************/
static int goodix_ts_power(struct goodix_ts_data * ts, int on)
{

	int ret = -1;

	unsigned char i2c_control_buf[3] = {0x06,0x92,0x01};		//suspend cmd
	
	printk("Goodix GT811 power action\n");		
	switch(on)
	{
		case 0:
			printk("Goodix GT811 suspend\n");
			gpio_direction_output(ts->rst_pin,1);
			msleep(20);
			gpio_direction_output(ts->rst_pin,0);
			msleep(20);
			gpio_direction_output(ts->rst_pin,1);
			msleep(20);
			ret = i2c_write_bytes(ts->client, i2c_control_buf, 3);
			dev_info(&ts->client->dev, "Send suspend cmd\n");
			if(ret <= 0)
			{
				printk("Goodix GT811 Send suspend cmd failed \n");
			}
			else
			{
				printk(" Send suspend cmd successully\n");	
			}
			ret = 0;
			return ret;
			
		case 1:
			printk("Goodix GT811 resume\n");
			gpio_direction_output(ts->rst_pin,1);
			msleep(20);
			gpio_direction_output(ts->rst_pin,0);
			msleep(20);
			gpio_direction_output(ts->rst_pin,1);
			msleep(20);
			goodix_init_panel(ts);
			ret = 0;
			return ret;
				
		default:
			//dev_info(&ts->client->dev, "%s: Cant't support this command.", s3c_ts_name);
			return -EINVAL;
	}


}
/*******************************************************	
Function:
	Touch-screen detection function
	Called when the registration drive (required for a corresponding client);
	For IO, interrupts and other resources to apply; equipment registration; touch screen initialization, etc.
Parameters:
	client: the device structure to be driven
	id: device ID
return:
	Results of the implementation code, 0 for normal execution
********************************************************/
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	int retry=0;
        char test_data = 1;
	//const char irq_table[2] = {IRQ_TYPE_EDGE_FALLING,IRQ_TYPE_EDGE_RISING};
	struct goodix_ts_data *ts;

	struct goodix_811_platform_data *pdata;
	dev_info(&client->dev,"Install gt811 driver.\n");
	dev_info(&client->dev,"Driver Release Date:2012-02-08\n");	

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		dev_err(&client->dev, "Must have I2C_FUNC_I2C.\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	i2c_connect_client = client;

	pdata = client->dev.platform_data;
	ts->rst_pin = pdata->reset;
	
	gpio_set_value(pdata->reset, 1);
	msleep(20);
	gpio_set_value(pdata->reset, 0);
	msleep(20);
	gpio_set_value(pdata->reset, 1);
	msleep(20);

	for(retry=0;retry <= 10; retry++)
	{	
		ret =i2c_write_bytes(client, &test_data, 1);	//Test I2C connection.
		if (ret > 0)
			break;
		msleep(5);
		dev_info(&client->dev, "I2C failed times:%d\n",retry);
	}

	if(ret <= 0)
	{
		dev_err(&client->dev, "Warnning: I2C communication might be ERROR!\n");
		goto err_i2c_failed;
	}	
    
	INIT_WORK(&ts->work, goodix_ts_work_func);		//init work_struct
	ts->client = client;
	i2c_set_clientdata(client, ts);
	//pdata = client->dev.platform_data;

	msleep(20);
        goodix_read_version(ts);

#ifdef AUTO_UPDATE_GT811            
        ret = gt811_downloader( ts, goodix_gt811_firmware);
        if(ret < 0)
        {
                dev_err(&client->dev, "Warnning: gt811 update might be ERROR!\n");
                //goto err_input_dev_alloc_failed;
        }
#endif
    

	//client->irq=TS_INT;		
	//ts->irq = TS_INT;  
  	ts->irq = client->irq;
	ts->irq_is_disable = 0;   
        
	for(retry=0; retry<3; retry++)
	{
		ret=goodix_init_panel(ts);
		msleep(2);
		if(ret != 0)	//Initiall failed
			continue;
		else
			break;
	}
	if(ret != 0) 
	{
		ts->bad_data=1;
		goto err_init_godix_ts;
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) 
	{
		ret = -ENOMEM;
		dev_dbg(&client->dev,"goodix_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_ABS) ;
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);

	set_bit(ABS_MT_POSITION_X, ts->input_dev->evbit);   
        set_bit(ABS_MT_POSITION_Y, ts->input_dev->evbit);   
        set_bit(ABS_MT_TRACKING_ID, ts->input_dev->evbit);  

#ifdef HAVE_TOUCH_KEY
	for(retry = 0; retry < MAX_KEY_NUM; retry++)
	{
		input_set_capability(ts->input_dev,EV_KEY,touch_key_array[retry]);	
	}
#endif

	input_set_abs_params(ts->input_dev, ABS_X, 0,  ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0, ts->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	
#ifdef GOODIX_MULTI_TOUCH
	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
	input_mt_init_slots(ts->input_dev, 5);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0,  ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);	
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, MAX_FINGER_NUM, 0, 0);
        
#endif	

	//set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

	sprintf(ts->phys, "input/ts");
	ts->input_dev->name = s3c_ts_name;
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;
	ts->input_dev->id.vendor = 0xDEAD;
	ts->input_dev->id.product = 0xBEEF;
	ts->input_dev->id.version = 10427;	//screen firmware version
	
	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev,"Probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	ts->bad_data = 0;

#ifdef INT_PORT		
	ret  = request_irq(client->irq, goodix_ts_irq_handler, IRQ_TYPE_EDGE_RISING ,
			client->name, ts);
	if (ret != 0)
	{
		dev_err(&client->dev,"Cannot allocate ts INT!ERRNO:%d\n", ret);
		gpio_direction_input(INT_PORT);
		gpio_free(INT_PORT);
		goto err_init_godix_ts;
	}
	else 
	{	
		ts->use_irq = 1;
		dev_dbg(&client->dev,"Reques EIRQ %d succesd on GPIO:%d\n",TS_INT,INT_PORT);
	}	
#endif	
	
	if (!ts->use_irq) 
	{
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = goodix_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
	
//	if(ts->use_irq)
//		enable_irq(client->irq);
			
	ts->power = goodix_ts_power;

/////////////////////////////// UPDATE STEP 2 START /////////////////////////////////////////////////////////////////
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
	goodix_proc_entry = create_proc_entry("goodix-update", 0666, NULL);
	if(goodix_proc_entry == NULL)
	{
		dev_info(&client->dev, "Couldn't create proc entry!\n");
		ret = -ENOMEM;
		goto err_create_proc_entry;
	}
	else
	{
		dev_info(&client->dev, "Create proc entry success!\n");
		goodix_proc_entry->write_proc = goodix_update_write;
		goodix_proc_entry->read_proc = goodix_update_read;
	}
#endif
///////////////////////////////UPDATE STEP 2 END /////////////////////////////////////////////////////////////////
	dev_info(&client->dev,"Start %s in %s mode,Driver Modify Date:2012-01-05\n", 
		ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");
	return 0;

err_init_godix_ts:
	i2c_end_cmd(ts);
	if(ts->use_irq)
	{
		ts->use_irq = 0;
		free_irq(client->irq,ts);
	#ifdef INT_PORT	
		gpio_direction_input(INT_PORT);
		gpio_free(INT_PORT);
	#endif	
	}
	else 
		hrtimer_cancel(&ts->timer);

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
	i2c_set_clientdata(client, NULL);
err_gpio_request:
err_i2c_failed:	
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
err_create_proc_entry:
	return ret;
}


/*******************************************************	
Function:
	Drive the release of resources
Parameters:
	client: the device structure
return:
	Results of the implementation code, 0 for normal execution
********************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
/////////////////////////////// UPDATE STEP 3 START/////////////////////////////////////////////////////////////////
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
	remove_proc_entry("goodix-update", NULL);
#endif
/////////////////////////////////UPDATE STEP 3 END///////////////////////////////////////////////////////////////

	if (ts && ts->use_irq) 
	{
	#ifdef INT_PORT
		gpio_direction_input(INT_PORT);
		gpio_free(INT_PORT);
	#endif	
		free_irq(client->irq, ts);
	}	
	else if(ts)
		hrtimer_cancel(&ts->timer);
	
	dev_notice(&client->dev,"The driver is removing...\n");
	i2c_set_clientdata(client, NULL);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	return 0;
}


static int goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
	printk("Goodix touchscreen GT811 suspend\n");
	if (ts->use_irq)
		disable_irq(client->irq);
	else
		hrtimer_cancel(&ts->timer);

	ret = cancel_work_sync(&ts->work);

	if (ts->power) {	
		ret = ts->power(ts, 0);
		if (ret == 0)
			printk("goodix_ts suspend \n");
	}

	return 0;
}

static int goodix_ts_resume(struct i2c_client *client)
{
	int ret;
	struct goodix_ts_data *ts = i2c_get_clientdata(client);
	printk("Goodix touchscreen GT811 resume\n");

	if (ts->power) {
		ret = ts->power(ts, 1);
		if (ret < 0)
			printk(KERN_ERR "goodix_ts_resume power on failed\n");
	}
	if (ts->use_irq)
		enable_irq(client->irq);
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
	struct goodix_ts_data *ts;
	ts = container_of(h, struct goodix_ts_data, early_suspend);
	goodix_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void goodix_ts_late_resume(struct early_suspend *h)
{
	struct goodix_ts_data *ts;
	ts = container_of(h, struct goodix_ts_data, early_suspend);
	goodix_ts_resume(ts->client);
}
#endif
/////////////////////////////// UPDATE STEP 4 START/////////////////////////////////////////////////////////////////
//******************************Begin of firmware update surpport*******************************
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
static struct file * update_file_open(char * path, mm_segment_t * old_fs_p)
{
	struct file * filp = NULL;
	int errno = -1;
		
	filp = filp_open(path, O_RDONLY, 0644);
	
	if(!filp || IS_ERR(filp))
	{
		if(!filp)
			errno = -ENOENT;
		else 
			errno = PTR_ERR(filp);					
		printk(KERN_ERR "The update file for Guitar open error.\n");
		return NULL;
	}
	*old_fs_p = get_fs();
	set_fs(get_ds());

	filp->f_op->llseek(filp,0,0);
	return filp ;
}

static void update_file_close(struct file * filp, mm_segment_t old_fs)
{
	set_fs(old_fs);
	if(filp)
		filp_close(filp, NULL);
}
static int update_get_flen(char * path)
{
	struct file * file_ck = NULL;
	mm_segment_t old_fs;
	int length ;
	
	file_ck = update_file_open(path, &old_fs);
	if(file_ck == NULL)
		return 0;

	length = file_ck->f_op->llseek(file_ck, 0, SEEK_END);

	if(length < 0)
		length = 0;
	update_file_close(file_ck, old_fs);
	return length;	
}

static int goodix_update_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
	unsigned char cmd[120];
	int ret = -1;
        int retry = 0;
	static unsigned char update_path[60];
	struct goodix_ts_data *ts;
	struct file * file_data = NULL;
    	mm_segment_t old_fs;
	unsigned char *file_ptr = NULL;
	unsigned int file_len;
	
	ts = i2c_get_clientdata(i2c_connect_client);
	if(ts==NULL)
	{
            printk(KERN_INFO"goodix write to kernel via proc file!\n");
		return 0;
	}
	
	//printk(KERN_INFO"goodix write to kernel via proc file!\n");
	if(copy_from_user(&cmd, buff, len))
	{
            printk(KERN_INFO"goodix write to kernel via proc file!\n");
		return -EFAULT;
	}
	//printk(KERN_INFO"Write cmd is:%d,write len is:%ld\n",cmd[0], len);
	switch(cmd[0])
	{
            case APK_UPDATE_TP:
            printk(KERN_INFO"Write cmd is:%d,cmd arg is:%s,write len is:%ld\n",cmd[0], &cmd[1], len);
            memset(update_path, 0, 60);
            strncpy(update_path, cmd+1, 60);
			

	    disable_irq(ts->client->irq);

	file_data = update_file_open(update_path, &old_fs);
        if(file_data == NULL)   //file_data has been opened at the last time
        {
		dev_info(&ts->client->dev, "cannot open update file\n");
		return 0;
        }

        file_len = update_get_flen(update_path);
	dev_info(&ts->client->dev, "Update file length:%d\n", file_len);
	file_ptr = (unsigned char*)vmalloc(file_len);
	if(file_ptr==NULL)
	{
		dev_info(&ts->client->dev, "cannot malloc memory!\n");
		return 0;
	}	

        ret = file_data->f_op->read(file_data, file_ptr, file_len, &file_data->f_pos);
        if(ret <= 0)
        {
		dev_info(&ts->client->dev, "read file data failed\n");
		return 0;
        }
        update_file_close(file_data, old_fs);	

        ret = gt811_downloader(ts, file_ptr);
        vfree(file_ptr);
	if(ret < 0)
        {
                printk(KERN_INFO"Warnning: GT811 update might be ERROR!\n");
                return 0;
        }
             	
	gpio_direction_output(SHUTDOWN_PORT, 0);
        msleep(5);
        gpio_direction_input(SHUTDOWN_PORT);
	msleep(20);
	for(retry=0; retry<3; retry++)
	{
		ret=goodix_init_panel(ts);
		msleep(2);
		if(ret != 0)	//Initiall failed
		{
			dev_info(&ts->client->dev, "Init panel failed!\n");
			continue;
		}
		else
			break;
		
	}
	gpio_free(INT_PORT); 
	ret = gpio_request(INT_PORT, "TS_INT"); //Request IO
        if (ret < 0)
        {
              dev_err(&ts->client->dev, "Failed to request GPIO:%d, ERRNO:%d\n",(int)INT_PORT,ret);
	      return 0;
        }

	enable_irq(ts->client->irq);

        return 1;
    
    case APK_READ_FUN:			     		//functional command
		if(cmd[1] == CMD_READ_VER)
		{
			printk(KERN_INFO"Read version!\n");
			ts->read_mode = MODE_RD_VER;
		}
        else if(cmd[1] == CMD_READ_CFG)
		{
			printk(KERN_INFO"Read config info!\n");

			ts->read_mode = MODE_RD_CFG;
		}
		else if (cmd[1] == CMD_READ_RAW)
		{
		    printk(KERN_INFO"Read raw data!\n");

			ts->read_mode = MODE_RD_RAW;
		}
        else if (cmd[1] == CMD_READ_CHIP_TYPE)
		{
		    printk(KERN_INFO"Read chip type!\n");

			ts->read_mode = MODE_RD_CHIP_TYPE;
		}
        return 1;
        
    case APK_WRITE_CFG:			
		printk(KERN_INFO"Begin write config info!Config length:%d\n",cmd[1]);
		i2c_pre_cmd(ts);
        ret = i2c_write_bytes(ts->client, cmd+2, cmd[1]+2); 
        i2c_end_cmd(ts);
        if(ret != 1)
        {
            printk("Write Config failed!return:%d\n",ret);
            return -1;
        }
        return 1;
            
    default:
	    return 0;
	}
	return 0;
}

static int goodix_update_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
	int ret = -1;
    	int len = 0;
    	int read_times = 0;
	struct goodix_ts_data *ts;

	unsigned char read_data[360] = {80, };

	ts = i2c_get_clientdata(i2c_connect_client);
	if(ts==NULL)
		return 0;
    
       	printk("___READ__\n");
	if(ts->read_mode == MODE_RD_VER)		//read version data
	{
		i2c_pre_cmd(ts);
		ret = goodix_read_version(ts);
             	i2c_end_cmd(ts);
		if(ret < 0)
		{
			printk(KERN_INFO"Read version data failed!\n");
			return 0;
		}
        
             	read_data[1] = (char)(ts->version&0xff);
             	read_data[0] = (char)((ts->version>>8)&0xff);

		memcpy(page, read_data, 2);
		//*eof = 1;
		return 2;
	}
    else if (ts->read_mode == MODE_RD_CHIP_TYPE)
    {
        page[0] = GT811;
        return 1;
    }
    else if(ts->read_mode == MODE_RD_CFG)
	{

            read_data[0] = 0x06;
            read_data[1] = 0xa2;       // cfg start address
            printk("read config addr is:%x,%x\n", read_data[0],read_data[1]);

	     len = 106;
           i2c_pre_cmd(ts);
	     ret = i2c_read_bytes(ts->client, read_data, len+2);
            i2c_end_cmd(ts);
            if(ret <= 0)
		{
			printk(KERN_INFO"Read config info failed!\n");
			return 0;
		}
              
		memcpy(page, read_data+2, len);
		return len;
	}
	else if (ts->read_mode == MODE_RD_RAW)
	{
#define TIMEOUT (-100)
	    int retry = 0;
        if (raw_data_ready != RAW_DATA_READY)
        {
            raw_data_ready = RAW_DATA_ACTIVE;
        }

RETRY:
        read_data[0] = 0x07;
        read_data[1] = 0x11;
        read_data[2] = 0x01;
        
        ret = i2c_write_bytes(ts->client, read_data, 3);
        
#ifdef DEBUG
        sum += read_times;
        printk("count :%d\n", ++access_count);
        printk("A total of try times:%d\n", sum);
#endif
               
        read_times = 0;
	    while (RAW_DATA_READY != raw_data_ready)
	    {
	        msleep(4);

	        if (read_times++ > 10)
	        {
    	        if (retry++ > 5)
    	        {
    	            return TIMEOUT;
    	        }
                goto RETRY;
	        }
	    }
#ifdef DEBUG	    
        printk("read times:%d\n", read_times);
#endif	    
        read_data[0] = 0x08;
        read_data[1] = 0x80;       // raw data address
        
	len = 160;

        i2c_pre_cmd(ts);
	ret = i2c_read_bytes(ts->client, read_data, len+2);    	    
     
        if(ret <= 0)
		{
			printk(KERN_INFO"Read raw data failed!\n");
			return 0;
		}
		memcpy(page, read_data+2, len);

		read_data[0] = 0x09;
        read_data[1] = 0xC0;

	    ret = i2c_read_bytes(ts->client, read_data, len+2);    	    
        i2c_end_cmd(ts);
        
        if(ret <= 0)
		{
			printk(KERN_INFO"Read raw data failed!\n");
			return 0;
		}
		memcpy(&page[160], read_data+2, len);

#ifdef DEBUG
//**************
        for (i = 0; i < 300; i++)
        {
            printk("%6x", page[i]);

            if ((i+1) % 10 == 0)
            {
                printk("\n");
            }
        }
//********************/  
#endif
        raw_data_ready = RAW_DATA_NON_ACTIVE;
    
		return (2*len);   
		
    }
	return 0;
#endif
}             
//********************************************************************************************
static u8  is_equal( u8 *src , u8 *dst , int len )
{
    int i;

    for( i = 0 ; i < len ; i++ )
    {
        if ( src[i] != dst[i] )
        {
            return 0;
        }
    }
    
    return 1;
}

static  u8 gt811_nvram_store( struct goodix_ts_data *ts )
{
    int ret;
    int i;
    u8 inbuf[3] = {REG_NVRCS_H,REG_NVRCS_L,0};

    ret = i2c_read_bytes( ts->client, inbuf, 3 );
    
    if ( ret < 0 )
    {
        return 0;
    }
    
    if ( ( inbuf[2] & BIT_NVRAM_LOCK ) == BIT_NVRAM_LOCK )
    {
        return 0;
    }
    
    inbuf[2] = (1<<BIT_NVRAM_STROE);		//store command
	    
    for ( i = 0 ; i < 300 ; i++ )
    {
        ret = i2c_write_bytes( ts->client, inbuf, 3 );
        
        if ( ret < 0 )
            break;
    }
    
    return ret;
}

static u8  gt811_nvram_recall( struct goodix_ts_data *ts )
{
    int ret;
    u8 inbuf[3] = {REG_NVRCS_H,REG_NVRCS_L,0};
    
    ret = i2c_read_bytes( ts->client, inbuf, 3 );
    
    if ( ret < 0 )
    {
        return 0;
    }
    
    if ( ( inbuf[2]&BIT_NVRAM_LOCK) == BIT_NVRAM_LOCK )
    {
        return 0;
    }
    
    inbuf[2] = ( 1 << BIT_NVRAM_RECALL );		//recall command
    ret = i2c_write_bytes( ts->client , inbuf, 3);
    return ret;
}

static  int gt811_reset( struct goodix_ts_data *ts )
{
    int ret = 1;
    u8 retry;
    
    unsigned char outbuf[3] = {0,0xff,0};
    unsigned char inbuf[3] = {0,0xff,0};

    gpio_direction_output(SHUTDOWN_PORT,0);
    msleep(20);
    gpio_direction_input(SHUTDOWN_PORT);
    msleep(100);
    for(retry=0;retry < 80; retry++)
    {
        ret =i2c_write_bytes(ts->client, inbuf, 0);	//Test I2C connection.
        if (ret > 0)
        {
            msleep(10);
            ret =i2c_read_bytes(ts->client, inbuf, 3);	//Test I2C connection.
            if (ret > 0)
            {
                if(inbuf[2] == 0x55)
                {
			ret =i2c_write_bytes(ts->client, outbuf, 3);
			msleep(10);
			break;						
		}
	    }			
	}
	else
	{
		gpio_direction_output(SHUTDOWN_PORT,0);
		msleep(20);
		gpio_direction_input(SHUTDOWN_PORT);
		msleep(20);
		dev_info(&ts->client->dev, "i2c address failed\n");
	}	
		
    }
    dev_info(&ts->client->dev, "Detect address %0X\n", ts->client->addr);
    return ret;	
}

static  int gt811_reset2( struct goodix_ts_data *ts )
{
    int ret = 1;
    u8 retry;
    
    unsigned char inbuf[3] = {0,0xff,0};

    gpio_direction_output(SHUTDOWN_PORT,0);
    msleep(20);
    gpio_direction_input(SHUTDOWN_PORT);
    msleep(100);
    for(retry=0;retry < 80; retry++)
    {
        ret =i2c_write_bytes(ts->client, inbuf, 0);	//Test I2C connection.
        if (ret > 0)
        {
            msleep(10);
            ret =i2c_read_bytes(ts->client, inbuf, 3);	//Test I2C connection.
            if (ret > 0)
            {
		break;						
	    }			
	}	
		
    }
    dev_info(&ts->client->dev, "Detect address %0X\n", ts->client->addr);
    //msleep(500);
    return ret;	
}
static  int gt811_set_address_2( struct goodix_ts_data *ts )
{
    unsigned char inbuf[3] = {0,0,0};
    int i;

    for ( i = 0 ; i < 12 ; i++ )
    {
        if ( i2c_read_bytes( ts->client, inbuf, 3) )
        {
            dev_info(&ts->client->dev, "Got response\n");
            return 1;
        }
        dev_info(&ts->client->dev, "wait for retry\n");
        msleep(50);
    } 
    return 0;
}
static u8  gt811_update_firmware( u8 *nvram, u16 start_addr, u16 length, struct goodix_ts_data *ts)
{
    u8 ret,err,retry_time,i;
    u16 cur_code_addr;
    u16 cur_frame_num, total_frame_num, cur_frame_len;
    u32 gt80x_update_rate;

    unsigned char i2c_data_buf[PACK_SIZE+2] = {0,};
    unsigned char i2c_chk_data_buf[PACK_SIZE+2] = {0,};
    
    if( length > NVRAM_LEN - NVRAM_BOOT_SECTOR_LEN )
    {
        dev_info(&ts->client->dev, "Fw length %d is bigger than limited length %d\n", length, NVRAM_LEN - NVRAM_BOOT_SECTOR_LEN );
        return 0;
    }
    	
    total_frame_num = ( length + PACK_SIZE - 1) / PACK_SIZE;  


    gt80x_update_rate = 0;

    for( cur_frame_num = 0 ; cur_frame_num < total_frame_num ; cur_frame_num++ )	  
    {
        retry_time = 5;
       
	dev_info(&ts->client->dev, "PACK[%d]\n",cur_frame_num); 
        cur_code_addr = start_addr + cur_frame_num * PACK_SIZE; 	
        i2c_data_buf[0] = (cur_code_addr>>8)&0xff;
        i2c_data_buf[1] = cur_code_addr&0xff;
        
        i2c_chk_data_buf[0] = i2c_data_buf[0];
        i2c_chk_data_buf[1] = i2c_data_buf[1];
        
        if( cur_frame_num == total_frame_num - 1 )
        {
            cur_frame_len = length - cur_frame_num * PACK_SIZE;
        }
        else
        {
            cur_frame_len = PACK_SIZE;
        }
        
        for(i=0;i<cur_frame_len;i++)
        {
            i2c_data_buf[2+i] = nvram[cur_frame_num*PACK_SIZE+i];
        }
        do
        {
            err = 0;	
	    ret = i2c_write_bytes(ts->client, i2c_data_buf, (cur_frame_len+2));
            if ( ret <= 0 )
            {
                dev_info(&ts->client->dev, "write fail\n");
                err = 1;
            }
            
            ret = i2c_read_bytes(ts->client, i2c_chk_data_buf, (cur_frame_len+2));
            // ret = gt811_i2c_read( guitar_i2c_address, cur_code_addr, inbuf, cur_frame_len);
            if ( ret <= 0 )
            {
                dev_info(&ts->client->dev, "read fail\n");
                err = 1;
            }
	    
            if( is_equal( &i2c_data_buf[2], &i2c_chk_data_buf[2], cur_frame_len ) == 0 )
            {
                dev_info(&ts->client->dev, "not equal\n");
                err = 1;
            }
			
        } while ( err == 1 && (--retry_time) > 0 );
        
        if( err == 1 )
        {
            break;
        }
		
        gt80x_update_rate = ( cur_frame_num + 1 )*128/total_frame_num;
    
    }

    if( err == 1 )
    {
        dev_info(&ts->client->dev, "write nvram fail\n");
        return 0;
    }
    
    ret = gt811_nvram_store(ts);
    
    msleep( 20 );

    if( ret == 0 )
    {
        dev_info(&ts->client->dev, "nvram store fail\n");
        return 0;
    }
    
    ret = gt811_nvram_recall(ts);

    msleep( 20 );
    
    if( ret == 0 )
    {
        dev_info(&ts->client->dev, "nvram recall fail\n");
        return 0;
    }

    for ( cur_frame_num = 0 ; cur_frame_num < total_frame_num ; cur_frame_num++ )		
    {

        cur_code_addr = NVRAM_UPDATE_START_ADDR + cur_frame_num*PACK_SIZE;
        retry_time=5;
        i2c_chk_data_buf[0] = (cur_code_addr>>8)&0xff;
        i2c_chk_data_buf[1] = cur_code_addr&0xff;
        
        
        if ( cur_frame_num == total_frame_num-1 )
        {
            cur_frame_len = length - cur_frame_num*PACK_SIZE;
        }
        else
        {
            cur_frame_len = PACK_SIZE;
        }
        
        do
        {
            err = 0;
            ret = i2c_read_bytes(ts->client, i2c_chk_data_buf, (cur_frame_len+2));

            if ( ret == 0 )
            {
                err = 1;
            }
            
            if( is_equal( &nvram[cur_frame_num*PACK_SIZE], &i2c_chk_data_buf[2], cur_frame_len ) == 0 )
            {
                err = 1;
            }
        } while ( err == 1 && (--retry_time) > 0 );
        
        if( err == 1 )
        {
            break;
        }
        
        gt80x_update_rate = 127 + ( cur_frame_num + 1 )*128/total_frame_num;
    }
    
    gt80x_update_rate = 255;

    if( err == 1 )
    {
        dev_info(&ts->client->dev, "nvram validate fail\n");
        return 0;
    }
    
    return 1;
}

static u8  gt811_update_proc( u8 *nvram, u16 start_addr , u16 length, struct goodix_ts_data *ts )
{
    u8 ret;
    u8 error = 0;

    GT811_SET_INT_PIN( 0 );
    msleep( 20 );
    ret = gt811_reset(ts);
    if ( ret < 0 )
    {
        error = 1;
        dev_info(&ts->client->dev, "reset fail\n");
        goto end;
    }

    ret = gt811_set_address_2( ts );
    if ( ret == 0 )
    {
        error = 1;
        dev_info(&ts->client->dev, "set address fail\n");
        goto end;
    }

    ret = gt811_update_firmware( nvram, start_addr, length, ts);
    if ( ret == 0 )
    {
        error=1;
       	dev_info(&ts->client->dev, "firmware update fail\n");
        goto end;
    }

end:
    GT811_SET_INT_PIN( 1 );
 
    msleep( 500 );
    ret = gt811_reset2(ts);
    if ( ret < 0 )
    {
        error=1;
        dev_info(&ts->client->dev, "final reset fail\n");
        goto end;
    }
    if ( error == 1 )
    {
        return 0; 
    }
	
//    i2c_pre_cmd(ts);
    while(goodix_read_version(ts)<0);
    
//    i2c_end_cmd(ts);
    return 1;
}

u16 Little2BigEndian(u16 little_endian)
{
	u16 temp = 0;
	temp = little_endian&0xff;
	return (temp<<8)+((little_endian>>8)&0xff);
}

int  gt811_downloader( struct goodix_ts_data *ts,  unsigned char * data)
{
    struct tpd_firmware_info_t *fw_info = (struct tpd_firmware_info_t *)data;
    unsigned int  fw_checksum = 0;
    unsigned short fw_version;
    unsigned short fw_start_addr;
    unsigned short fw_length;
    unsigned char *data_ptr;
    int retry = 0,ret;
    int err = 0;
    unsigned char rd_buf[4] = {0};
    unsigned char *mandatory_base = "GOODIX";
    unsigned char rd_rom_version;
    unsigned char rd_chip_type;
    unsigned char rd_nvram_flag;
    
    rd_buf[0]=0x14;
    rd_buf[1]=0x00;
    rd_buf[2]=0x80;
    ret = i2c_write_bytes(ts->client, rd_buf, 3);
    if(ret<0)
    {
            dev_info(&ts->client->dev, "i2c write failed\n");
            goto exit_downloader;
    }
    rd_buf[0]=0x40;
    rd_buf[1]=0x11;
    ret = i2c_read_bytes(ts->client, rd_buf, 3);
    if(ret<=0)
    {
            dev_info(&ts->client->dev, "i2c request failed!\n");
            goto exit_downloader;
    }
    rd_chip_type = rd_buf[2];
    rd_buf[0]=0xFB;
    rd_buf[1]=0xED;
    ret = i2c_read_bytes(ts->client, rd_buf, 3);
    if(ret<=0)
    {
            dev_info(&ts->client->dev, "i2c read failed!\n");
            goto exit_downloader;
    }
    rd_rom_version = rd_buf[2];
    rd_buf[0]=0x06;
    rd_buf[1]=0x94;
    ret = i2c_read_bytes(ts->client, rd_buf, 3);
    if(ret<=0)
    {
            dev_info(&ts->client->dev, "i2c read failed!\n");
            goto exit_downloader;
    }
    rd_nvram_flag = rd_buf[2];

    fw_version = Little2BigEndian(fw_info->version);
    fw_start_addr = Little2BigEndian(fw_info->start_addr);
    fw_length = Little2BigEndian(fw_info->length);	
    data_ptr = &(fw_info->data);	

    dev_info(&ts->client->dev,"chip_type=0x%02x\n", fw_info->chip_type);
    dev_info(&ts->client->dev,"version=0x%04x\n", fw_version);
    dev_info(&ts->client->dev,"rom_version=0x%02x\n",fw_info->rom_version);
    dev_info(&ts->client->dev,"start_addr=0x%04x\n",fw_start_addr);
    dev_info(&ts->client->dev,"file_size=0x%04x\n",fw_length);
    fw_checksum = ((u32)fw_info->checksum[0]<<16) + ((u32)fw_info->checksum[1]<<8) + ((u32)fw_info->checksum[2]);
    dev_info(&ts->client->dev,"fw_checksum=0x%06x\n",fw_checksum);
    dev_info(&ts->client->dev,"%s\n", __func__ );
    dev_info(&ts->client->dev,"current version 0x%04X, target verion 0x%04X\n", ts->version, fw_version );

//chk_chip_type:
    if(rd_chip_type!=fw_info->chip_type)
    {
	dev_info(&ts->client->dev, "Chip type not match,exit downloader\n");
	goto exit_downloader;
    }
	
//chk_mask_version:	
    if(!rd_rom_version)
    {
 	if(fw_info->rom_version!=0x45)
	{
		dev_info(&ts->client->dev, "Rom version not match,exit downloader\n");
		goto exit_downloader;
	}
	dev_info(&ts->client->dev, "Rom version E.\n");
	goto chk_fw_version;
    }
    else if(rd_rom_version!=fw_info->rom_version);
    {
	dev_info(&ts->client->dev, "Rom version not match,exidownloader\n");
	goto exit_downloader;
    }
    dev_info(&ts->client->dev, "Rom version %c\n",rd_rom_version);

//chk_nvram:	
    if(rd_nvram_flag==0x55)
    {
	dev_info(&ts->client->dev, "NVRAM correct!\n");
	goto chk_fw_version;
    }
    else if(rd_nvram_flag==0xAA)
    {
	dev_info(&ts->client->dev, "NVRAM incorrect!Need update.\n");
	goto begin_upgrade;
    }
    else
    {
	dev_info(&ts->client->dev, "NVRAM other error![0x694]=0x%02x\n", rd_nvram_flag);
	goto begin_upgrade;
    }
chk_fw_version:
//	ts->version -= 1;               //test by andrew        
    if( ts->version >= fw_version )   // current low byte higher than back-up low byte
    {
            dev_info(&ts->client->dev, "Fw verison not match.\n");
            goto chk_mandatory_upgrade;
    }
    dev_info(&ts->client->dev,"Need to upgrade\n");
    goto begin_upgrade;
chk_mandatory_upgrade:

    ret = memcmp(mandatory_base, fw_info->mandatory_flag, 6);
    if(ret)
    {
 	dev_info(&ts->client->dev,"Not meet mandatory upgrade,exit downloader!ret:%d\n", ret);
	goto exit_downloader;
    }
    dev_info(&ts->client->dev, "Mandatory upgrade!\n");
begin_upgrade:
    dev_info(&ts->client->dev, "Begin upgrade!\n");

    dev_info(&ts->client->dev,"STEP_0:\n");
    gpio_free(INT_PORT);
    ret = gpio_request(INT_PORT, "TS_INT");	//Request IO
    if (ret < 0) 
    {
        dev_info(&ts->client->dev,"Failed to request GPIO:%d, ERRNO:%d\n",(int)INT_PORT,ret);
        err = -1;
        goto exit_downloader;
    }
   
    dev_info(&ts->client->dev, "STEP_1:\n");
    err = -1;
    while( retry < 3 ) 
    {
        ret = gt811_update_proc( data_ptr,fw_start_addr, fw_length, ts);
        if(ret == 1)
        {
            err = 1;
            break;
        }
        retry++;
    }
    
exit_downloader:
    gpio_free(INT_PORT);

    return err;

}
//******************************End of firmware update surpport*******************************
/////////////////////////////// UPDATE STEP 4 END /////////////////////////////////////////////////////////////////

//only one client
static const struct i2c_device_id goodix_ts_id[] = {
	{ GOODIX_I2C_NAME, 0 },
	{ }
};


static struct i2c_driver goodix_ts_driver = {
	.probe		= goodix_ts_probe,
	.remove		= goodix_ts_remove,
	.suspend	= goodix_ts_suspend,
	.resume		= goodix_ts_resume,
	.id_table	= goodix_ts_id,
	.driver = {
		.name	= GOODIX_I2C_NAME,
		.owner = THIS_MODULE,
	},
};

/*******************************************************	
functionï¼?	 load goodix ts driver
returnï¼?	 0  successful
********************************************************/
static int __devinit goodix_ts_init(void)
{
	int ret;
	printk("Goodix ts GT811 init\n");
	goodix_wq = create_workqueue("goodix_wq");		//create a work queue and worker thread
	if (!goodix_wq) {
		printk(KERN_ALERT "creat workqueue faiked\n");
		return -ENOMEM;
		
	}
	ret=i2c_add_driver(&goodix_ts_driver);
	return ret; 
}

/*******************************************************	
functionï¼?	 exit goodix ts driver
returnï¼?	 none
********************************************************/
static void __exit goodix_ts_exit(void)
{
	printk(KERN_ALERT "Touchscreen driver of guitar exited.\n");
	i2c_del_driver(&goodix_ts_driver);
	if (goodix_wq)
		destroy_workqueue(goodix_wq);		
}

late_initcall(goodix_ts_init);				
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("Goodix Touchscreen Driver");
MODULE_LICENSE("GPL");
               
