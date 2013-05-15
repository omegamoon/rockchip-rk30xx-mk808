/* drivers/input/touchscreen/gt9xx.c
 * 
 * 2010 - 2012 Goodix Technology.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 * 
 * Version:1.2
 * Author:andrew@goodix.com
 * Release Date:2012/10/15
 * Revision record:
 *      V1.0:2012/08/31,first Release
 *      V1.2:2012/10/15,modify gtp_reset_guitar,slot report,tracking_id & 0x0F
 *
 */

#include <linux/irq.h>
#include <linux/gt9xx.h>

#if GTP_ICS_SLOT_REPORT
    #include <linux/input/mt.h>
#endif

static const char *goodix_ts_name = "Goodix Capacitive TouchScreen";
static struct workqueue_struct *goodix_wq;
struct i2c_client * i2c_connect_client = NULL; 
static u8 config[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH]
                = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
                
static int gt9110_reset_number;
static int gt9110_int_number; 
static int gt915_flag;

#if GTP_HAVE_TOUCH_KEY
	static const u16 touch_key_array[] = GTP_KEY_TAB;
	#define GTP_MAX_KEY_NUM	 (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#endif

static s8 gtp_i2c_test(struct i2c_client *client);
void gtp_reset_guitar(struct i2c_client *client, s32 ms);
void gtp_int_sync(s32 ms);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif
 
#if GTP_CREATE_WR_NODE
extern s32 init_wr_node(struct i2c_client*);
extern void uninit_wr_node(void);
#endif

#if GTP_AUTO_UPDATE
extern u8 gup_init_update_proc(struct goodix_ts_data *);
#endif

#if GTP_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct * gtp_esd_check_workqueue = NULL;
static void gtp_esd_check_func(struct work_struct *);
#endif

/*******************************************************	
Function:
	Read data from the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:read data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer
*********************************************************/
s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msgs[2];
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = client->addr;
    msgs[0].len   = GTP_ADDR_LENGTH;
    msgs[0].buf   = &buf[0];
    msgs[0].scl_rate=200000;

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = client->addr;
    msgs[1].len   = len - GTP_ADDR_LENGTH;
    msgs[1].buf   = &buf[GTP_ADDR_LENGTH];
    msgs[0].scl_rate=200000;

    while(retries < 1)
    {
        ret = i2c_transfer(client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }
    /*if(retries >= 5)
    {
        GTP_DEBUG("I2C retry timeout, reset chip.");
        gtp_reset_guitar(client, 10);
    }*/
    return ret;
}

/*******************************************************	
Function:
	write data to the i2c slave device.

Input:
	client:	i2c device.
	buf[0]:operate address.
	buf[1]~buf[len]:write data buffer.
	len:operate length.
	
Output:
	numbers of i2c_msgs to transfer.
*********************************************************/
s32 gtp_i2c_write(struct i2c_client *client,u8 *buf,s32 len)
{
    struct i2c_msg msg;
    s32 ret=-1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msg.flags = !I2C_M_RD;
    msg.addr  = client->addr;
    msg.len   = len;
    msg.buf   = buf;
    msg.scl_rate=200000;

    while(retries < 1)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret == 1)break;
        retries++;
    }
    /*if(retries >= 5)
    {
        GTP_DEBUG("I2C retry timeout, reset chip.");
        gtp_reset_guitar(client, 10);
    }*/
    return ret;
}

/*******************************************************
Function:
	Send config Function.

Input:
	client:	i2c client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
s32 gtp_send_cfg(struct i2c_client *client)
{
    s32 ret = 0;
    
#if  GTP_DRIVER_SEND_CFG
    s32 retry = 0;
    s32 i;

    for (retry = 0; retry < 5; retry++)
    {
        ret = gtp_i2c_write(client, config , GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
        if (ret > 0)
        {
            break;
        }
    }
#endif

    return ret;
}

/*******************************************************
Function:
	Enable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_disable(struct goodix_ts_data *ts)
{
    unsigned long irqflags;

    GTP_DEBUG_FUNC();

    //spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!ts->irq_is_disable)
    {
        ts->irq_is_disable = 1; 
        disable_irq_nosync(ts->client->irq);
    }
    //spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
	Disable IRQ Function.

Input:
	ts:	i2c client private struct.
	
Output:
	None.
*******************************************************/
void gtp_irq_enable(struct goodix_ts_data *ts)
{
    unsigned long irqflags = 0;

    GTP_DEBUG_FUNC();
    
    //spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (ts->irq_is_disable) 
    {
        enable_irq(ts->client->irq);
        ts->irq_is_disable = 0;	
    }
    //spin_unlock_irqrestore(&ts->irq_lock, irqflags);
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
#if GTP_CHANGE_X2Y
    GTP_SWAP(x, y);
#endif

#if GTP_ICS_SLOT_REPORT
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
#else
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_mt_sync(ts->input_dev);
#endif

    GTP_DEBUG("ID:%d, X:%d, Y:%d, W:%d", id, x, y, w);
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
#if GTP_ICS_SLOT_REPORT
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
    GTP_DEBUG("Touch id[%2d] release!", id);
#else
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
    input_mt_sync(ts->input_dev);
#endif
}

/*******************************************************
Function:
	Goodix touchscreen work function.

Input:
	work:	work_struct of goodix_wq.
	
Output:
	None.
*******************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    u8  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
    u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1]={GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    u8  touch_num = 0;
    u8  finger = 0;
    static u16 pre_touch = 0;
    static u8 pre_key = 0;
    u8  key_value = 0;
    u8* coor_data = NULL;
    s32 input_x = 0;
    s32 input_y = 0;
    s32 input_w = 0;
    s32 id = 0;
    s32 i  = 0;
    s32 ret = -1;
    struct goodix_ts_data *ts = NULL;

    GTP_DEBUG_FUNC();

    ts = container_of(work, struct goodix_ts_data, work);
    if (ts->enter_update)
    {
        return;
    }

    ret = gtp_i2c_read(ts->client, point_data, 12);
    if (ret < 0)
    {
    	/* If touchscreen is reset for any reason, the i2c address maybe changed */
    	if(ts->client->addr == 0x14)
		ts->client->addr = 0x5d;
	else
		ts->client->addr = 0x14;        
		   
        GTP_ERROR("I2C transfer error. errno:%d , change i2c address as %d\n ", ret,ts->client->addr);
        goto exit_work_func;
    }

    finger = point_data[GTP_ADDR_LENGTH];    
    if((finger & 0x80) == 0)
    {
        goto exit_work_func;
    }

    touch_num = finger & 0x0f;
    if (touch_num > GTP_MAX_TOUCH)
    {
        goto exit_work_func;
    }

    if (touch_num > 1)
    {
        u8 buf[8 * GTP_MAX_TOUCH] = {(GTP_READ_COOR_ADDR + 10) >> 8, (GTP_READ_COOR_ADDR + 10) & 0xff};

        ret = gtp_i2c_read(ts->client, buf, 2 + 8 * (touch_num - 1)); 
        memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
    }

#if GTP_HAVE_TOUCH_KEY
    key_value = point_data[3 + 8 * touch_num];
    
    if(key_value || pre_key)
    {
        for (i = 0; i < GTP_MAX_KEY_NUM; i++)
        {
            input_report_key(ts->input_dev, touch_key_array[i], key_value & (0x01<<i));   
        }
        touch_num = 0;
        pre_touch = 0;
    }
#endif
    pre_key = key_value;

    GTP_DEBUG("pre_touch:%02x, finger:%02x.", pre_touch, finger);

#if GTP_ICS_SLOT_REPORT
    if (pre_touch || touch_num)
    {
        s32 pos = 0;
        u16 touch_index = 0;

        coor_data = &point_data[3];
        if(touch_num)
        {
            id = coor_data[pos] & 0x0F;
            touch_index |= (0x01<<id);
        }

        GTP_DEBUG("id=%d,touch_index=0x%x,pre_touch=0x%x\n",id, touch_index,pre_touch);
        for (i = 0; i < GTP_MAX_TOUCH; i++)
        {
            if (touch_index & (0x01<<i))
            {
                input_x  = coor_data[pos + 1] | coor_data[pos + 2] << 8;
                input_y  = coor_data[pos + 3] | coor_data[pos + 4] << 8;
                input_w  = coor_data[pos + 5] | coor_data[pos + 6] << 8;
                
                gtp_touch_down(ts, id, input_x, input_y, input_w);
                pre_touch |= 0x01 << i;

                pos += 8;
                id = coor_data[pos] & 0x0F;
                touch_index |= (0x01<<id);
            }
            else// if (pre_touch & (0x01 << i))
            {
                gtp_touch_up(ts, i);
                pre_touch &= ~(0x01 << i);
            }
        }
    }

#else
    if (touch_num )
    {
        for (i = 0; i < touch_num; i++)
        {
            coor_data = &point_data[i * 8 + 3];

            id = coor_data[0] & 0x0F;
            input_x  = coor_data[1] | coor_data[2] << 8;
            input_y  = coor_data[3] | coor_data[4] << 8;
            input_w  = coor_data[5] | coor_data[6] << 8;

            gtp_touch_down(ts, id, input_x, input_y, input_w);
        }
    }
    else if (pre_touch)
    {
        GTP_DEBUG("Touch Release!");
        gtp_touch_up(ts, 0);
    }

    pre_touch = touch_num;
    input_report_key(ts->input_dev, BTN_TOUCH, (touch_num || key_value));
#endif

    input_sync(ts->input_dev);

exit_work_func:
    
    if(!ts->gtp_rawdiff_mode)
    {
        ret = gtp_i2c_write(ts->client, end_cmd, 3);
        if (ret < 0)
        {
            GTP_INFO("I2C write end_cmd  error!"); 
        }
    }

    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
}

/*******************************************************
Function:
	Timer interrupt service routine.

Input:
	timer:	timer struct pointer.
	
Output:
	Timer work mode. HRTIMER_NORESTART---not restart mode
*******************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
    struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);

    GTP_DEBUG_FUNC();

    queue_work(goodix_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME+6)*1000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Function:
	External interrupt service routine.

Input:
	irq:	interrupt number.
	dev_id: private data pointer.
	
Output:
	irq execute status.
*******************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    struct goodix_ts_data *ts = dev_id;

    GTP_DEBUG_FUNC();

    gtp_irq_disable(ts);
    queue_work(goodix_wq, &ts->work);

    return IRQ_HANDLED;
}
/*******************************************************
Function:
	Int sync Function.

Input:
	ms:sync time.
	
Output:
	None.
*******************************************************/
void gtp_int_sync(s32 ms)
{
    //GTP_GPIO_OUTPUT(gt9110_int_number, 0);
    msleep(ms);
    //GTP_GPIO_AS_INT(gt9110_int_number);
}

/*******************************************************
Function:
	Reset chip Function.

Input:
	ms:reset time.
	
Output:
	None.
*******************************************************/
void gtp_reset_guitar(struct i2c_client *client, s32 ms)
{
    GTP_DEBUG_FUNC();

    GTP_GPIO_OUTPUT(gt9110_reset_number, 0);   //begin select I2C slave addr
    msleep(ms);
    GTP_GPIO_OUTPUT(gt9110_int_number, client->addr == 0x14);

    msleep(2);
    GTP_GPIO_OUTPUT(gt9110_reset_number, 1);
    
    msleep(6);                          //must > 3ms
    GTP_GPIO_AS_INPUT(gt9110_int_number);    //end select I2C slave addr
    
    gtp_int_sync(50);
}

/*******************************************************
Function:
	Eter sleep function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_enter_sleep(struct goodix_ts_data * ts)
{
    s8 ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 5};

    GTP_DEBUG_FUNC();
    
    if( gt915_flag == 1)
    	return 0;

    GTP_GPIO_OUTPUT(gt9110_int_number, 0);
    msleep(10);
	  
    while(retry++ < 5)
    {
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        if (ret > 0)
        {
            printk("GTP enter sleep!\n");
            return ret;
        }
        msleep(10);
    }
    GTP_ERROR("GTP send sleep cmd failed.");
    return ret;
}

/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	ts:	private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_wakeup_sleep(struct goodix_ts_data * ts)
{
    u8 retry = 0;
    s8 ret = -1;

    GTP_DEBUG_FUNC();

#if GTP_POWER_CTRL_SLEEP
    while(retry++ < 5)
    {
        gtp_reset_guitar(ts->client, 20);
        ret = gtp_send_cfg(ts->client);
        if (ret > 0)
        {
            GTP_DEBUG("Wakeup sleep send config success.");
            return ret;
        }
    }
#else

     if( gt915_flag == 1)
        return 0;

     gpio_direction_output(gt9110_int_number,1);
     msleep(10);
   
    //gpio_direction_output(gt9110_reset_number,0);
    //msleep(5);
    //gpio_direction_output(gt9110_reset_number,1);
    //msleep(50);
    
    
    gpio_direction_input(gt9110_int_number);
    gpio_pull_updown(gt9110_int_number, PullDisable);
    ret = 0; 
    printk("GTP wakeup \n ");

#endif

    return ret;
}

/*******************************************************
Function:
	GTP initialize function.

Input:
	ts:	i2c client private struct.
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
static s32 gtp_init_panel(struct goodix_ts_data *ts)
{
    s32 ret = -1;
  
#if GTP_DRIVER_SEND_CFG
    s32 i;
    u8 check_sum = 0;
    u8 rd_cfg_buf[16];
    u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};
    u8 cfg_info_group1[] = CTP_CFG_GROUP1;
    u8 cfg_info_group2[] = CTP_CFG_GROUP2;
    u8 cfg_info_group3[] = CTP_CFG_GROUP3;
    u8 *send_cfg_buf[3] = {cfg_info_group1, cfg_info_group2, cfg_info_group3};
    u8 cfg_info_len[3] = {sizeof(cfg_info_group1)/sizeof(cfg_info_group1[0]), 
                          sizeof(cfg_info_group2)/sizeof(cfg_info_group2[0]),
                          sizeof(cfg_info_group3)/sizeof(cfg_info_group3[0])};
    for(i=0; i<3; i++)
    {
        if(cfg_info_len[i] > ts->gtp_cfg_len)
        {
            ts->gtp_cfg_len = cfg_info_len[i];
        }
    }
    ts->gtp_cfg_len =  ts->gtp_cfg_len ;
     
    GTP_DEBUG("len1=%d,len2=%d,len3=%d,send_len:%d",cfg_info_len[0],cfg_info_len[1],cfg_info_len[2],ts->gtp_cfg_len);
    if((!cfg_info_len[1]) && (!cfg_info_len[2]))
    {
        rd_cfg_buf[GTP_ADDR_LENGTH] = 0; 
    }
    else
    {
       /* rd_cfg_buf[0] = GTP_REG_SENSOR_ID >> 8;
        rd_cfg_buf[1] = GTP_REG_SENSOR_ID & 0xff;
        ret = gtp_i2c_read(ts->client, rd_cfg_buf, 3);
        GTP_DEBUG ("Read SENSOR ID %d\n ", rd_cfg_buf[GTP_ADDR_LENGTH] );
        if (ret < 0)
        {
            GTP_ERROR("Read SENSOR ID failed,default use group1 config!");
            rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
        }
        rd_cfg_buf[GTP_ADDR_LENGTH] &= 0x07;*/
        ret = gtp_i2c_read(ts->client, buf, sizeof(buf));
        if (ret < 0)
        {
        	GTP_ERROR("GTP read version failed , use default value\n");	
        	ts->abs_x_max = 800;
        	ts->abs_y_max = 1280;
        	ts->int_trigger_type = 1;  //Falliing 
        	msleep(500);
    		gpio_direction_output(gt9110_int_number,0);
    		msleep(10);
        	return 0;
	}	
        /* IF Buf[2] is  '5' , then this means gt915 ic */
        if( buf[4] == '5')
        {
            rd_cfg_buf[GTP_ADDR_LENGTH]	= 1;
            gt915_flag = 1;
	}
	else
	{
	     rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
	     gt915_flag = 0;
	}		
    }
    GTP_DEBUG("SENSOR ID:%d", rd_cfg_buf[GTP_ADDR_LENGTH]);
    memset(&config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
    memcpy(&config[GTP_ADDR_LENGTH], send_cfg_buf[rd_cfg_buf[GTP_ADDR_LENGTH]], ts->gtp_cfg_len);

#if GTP_CUSTOM_CFG
    config[RESOLUTION_LOC]     = (u8)GTP_MAX_WIDTH;
    config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH>>8);
    config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
    config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT>>8);
    
    if (GTP_INT_TRIGGER == 0)  //RISING
    {
        config[TRIGGER_LOC] &= 0xfe; 
    }
    else if (GTP_INT_TRIGGER == 1)  //FALLING
    {
        config[TRIGGER_LOC] |= 0x01;
    }
#endif  //endif GTP_CUSTOM_CFG
    
    check_sum = 0;
    for (i = GTP_ADDR_LENGTH; i < ts->gtp_cfg_len ; i++)
    {
        check_sum += config[i];
    }
    config[ts->gtp_cfg_len ] = (~check_sum) + 1;
    
    GTP_DEBUG("ts->gtp_cfg_len is %d, Check_sum is  0x%x",ts->gtp_cfg_len, config[ts->gtp_cfg_len]);  
    
#else //else DRIVER NEED NOT SEND CONFIG

    if(ts->gtp_cfg_len == 0)
    {
        ts->gtp_cfg_len = GTP_CONFIG_MAX_LENGTH;
    }
    ret = gtp_i2c_read(ts->client, config, ts->gtp_cfg_len + GTP_ADDR_LENGTH);
    if (ret < 0)
    {
        GTP_ERROR("GTP read resolution & max_touch_num failed, use default value!");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
        ts->int_trigger_type = GTP_INT_TRIGGER;
    }
#endif //endif GTP_DRIVER_SEND_CFG

    GTP_DEBUG_FUNC();

    ts->abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
    ts->abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
    ts->int_trigger_type = (config[TRIGGER_LOC]) & 0x03;
    if ((!ts->abs_x_max)||(!ts->abs_y_max))
    {
        GTP_ERROR("GTP resolution & max_touch_num invalid, use default value!");
        ts->abs_x_max = GTP_MAX_WIDTH;
        ts->abs_y_max = GTP_MAX_HEIGHT;
    }


    msleep(500);
    gpio_direction_output(gt9110_int_number,0);
    msleep(10);
    
    ret = gtp_send_cfg(ts->client);
    if (ret < 0)
    {
        GTP_ERROR("Send config error.");
    }

    GTP_DEBUG("X_MAX = %d,Y_MAX = %d,TRIGGER = 0x%02x",
             ts->abs_x_max,ts->abs_y_max,ts->int_trigger_type);

    return 0;
}

/*******************************************************
Function:
	Read goodix touchscreen version function.

Input:
	client:	i2c client struct.
	version:address to store version info
	
Output:
	Executive outcomes.0---succeed.
*******************************************************/
s32 gtp_read_version(struct i2c_client *client, u16* version)
{
    s32 ret = -1;
    s32 i = 0;
    u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};

    GTP_DEBUG_FUNC();

    ret = gtp_i2c_read(client, buf, sizeof(buf));
    if (ret < 0)
    {
        GTP_ERROR("GTP read version failed");
        return ret;
    }

    if (version)
    {
        *version = (buf[7] << 8) | buf[6];
    }

    for(i=2; i<6; i++)
    {
        if(!buf[i])
        {
            buf[i] = 0x30;
        }
    }
    GTP_INFO("IC VERSION:%c%c%c%c_%02x%02x", 
              buf[2], buf[3], buf[4], buf[5], buf[7], buf[6]);

    return ret;
}

/*******************************************************
Function:
	I2c test Function.

Input:
	client:i2c client.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_i2c_test(struct i2c_client *client)
{
    u8 test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
    u8 retry = 0;
    s8 ret = -1;
  
    GTP_DEBUG_FUNC();
  
    while(retry++ < 2)
    {
        ret = gtp_i2c_read(client, test, 3);
        if (ret > 0)
        {
            return ret;
        }
        GTP_ERROR("GTP i2c test failed time %d.",retry);
        msleep(10);
    }
    return ret;
}

/*******************************************************
Function:
	Request gpio Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_io_port(struct goodix_ts_data *ts)
{

}

/*******************************************************
Function:
	Request irq Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_irq(struct goodix_ts_data *ts)
{
    s32 ret = -1;
    const u8 irq_table[] = GTP_IRQ_TAB;

    GTP_DEBUG("INT trigger type:%x", ts->int_trigger_type);
    
    gpio_request(ts->client->irq, "GT9110_IRQ_PIN");
    
    ret  = request_irq(ts->client->irq, 
                       goodix_ts_irq_handler,
                       irq_table[ts->int_trigger_type],
                       ts->client->name,
                       ts);
    if (ret)
    {
        GTP_ERROR("Request IRQ failed!ERRNO:%d.", ret);
        GTP_GPIO_AS_INPUT(gt9110_int_number);
        GTP_GPIO_FREE(gt9110_int_number);

        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = goodix_ts_timer_handler;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
        return -1;
    }
    else 
    {
    	GTP_GPIO_AS_INPUT(gt9110_int_number);
    	gpio_pull_updown(ts->client->irq, PullDisable);
        gtp_irq_disable(ts);
        ts->use_irq = 1;
        return 0;
    }
}

/*******************************************************
Function:
	Request input device Function.

Input:
	ts:private data.
	
Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_request_input_dev(struct goodix_ts_data *ts)
{
    s8 ret = -1;
    s8 phys[32];
#if GTP_HAVE_TOUCH_KEY
    u8 index = 0;
#endif
  
    GTP_DEBUG_FUNC();
  
    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        GTP_ERROR("Failed to allocate input device.");
        return -ENOMEM;
    }

    ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
#if GTP_ICS_SLOT_REPORT
    __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
    input_mt_init_slots(ts->input_dev, 255);
#else
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

#if GTP_HAVE_TOUCH_KEY
    for (index = 0; index < GTP_MAX_KEY_NUM; index++)
    {
        input_set_capability(ts->input_dev,EV_KEY,touch_key_array[index]);	
    }
#endif

#if GTP_CHANGE_X2Y
    GTP_SWAP(ts->abs_x_max, ts->abs_y_max);
#endif

    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);	
    input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 255, 0, 0);

    sprintf(phys, "input/ts");
    ts->input_dev->name = goodix_ts_name;
    ts->input_dev->phys = phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;
	
    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        GTP_ERROR("Register %s input device failed", ts->input_dev->name);
        return -ENODEV;
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    return 0;
}

/*******************************************************
Function:
	Goodix touchscreen probe function.

Input:
	client:	i2c device struct.
	id:device id.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s32 ret = -1;
    struct goodix_ts_data *ts;
    struct  goodix_9110_platform_data  *gt9110_platform_data;  
    u16 version_info;

    GTP_DEBUG_FUNC();
    
    //do NOT remove these output log
    GTP_INFO("GTP Driver Version:%s",GTP_DRIVER_VERSION);
    GTP_INFO("GTP Driver build@%s,%s", __TIME__,__DATE__);
    GTP_INFO("GTP I2C Address:0x%02x", client->addr);

    i2c_connect_client = client;
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
    {
        GTP_ERROR("I2C check functionality failed.");
        return -ENODEV;
    }
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        GTP_ERROR("Alloc GFP_KERNEL memory failed.");
        return -ENOMEM;
    }
    
    gt9110_platform_data = client->dev.platform_data;
    printk("GT9110 reset pin is  %d, INT pin is %d \n ", gt9110_platform_data->reset, gt9110_platform_data->irq_pin);
   
    gt9110_reset_number = gt9110_platform_data->reset;
    gt9110_int_number  = gt9110_platform_data->irq_pin;
    
    memset(ts, 0, sizeof(*ts));
    INIT_WORK(&ts->work, goodix_ts_work_func);
    ts->client = client;
    i2c_set_clientdata(client, ts);
    //ts->irq_lock = SPIN_LOCK_UNLOCKED;
    ts->gtp_rawdiff_mode = 0;

   /* ret = gtp_request_io_port(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP request IO port failed.");
        kfree(ts);
        return ret;
    }*/

    //gpio_request(ts->client->irq, "GT9110_IRQ_PIN");
    
    //gtp_reset_guitar(client,10);
    
    ret = gtp_i2c_test(client);
    if (ret < 0)
    {
    	client->addr = 0x5d;
    	ret = gtp_i2c_test(client);
    	if (ret < 0)
    	{
        	GTP_ERROR("I2C communication ERROR!");
        	return -ENODEV;
	}
	GTP_INFO("GTP I2C new Address:0x%02x", client->addr);
    }

#if GTP_AUTO_UPDATE
    ret = gup_init_update_proc(ts);
    if (ret < 0)
    {
        GTP_ERROR("Create update thread error.");
    }
#endif
    
    ret = gtp_init_panel(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP init panel failed.");
    }

    ret = gtp_request_input_dev(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP request input dev failed");
    }
    
    ret = gtp_request_irq(ts); 
    if (ret < 0)
    {
        GTP_INFO("GTP works in polling mode.");
    }
    else
    {
        GTP_INFO("GTP works in interrupt mode.");
    }

    ret = gtp_read_version(client, &version_info);
    if (ret < 0)
    {
        GTP_ERROR("Read version failed.");
    }
    //spin_lock_init(&ts->irq_lock);
    //ts->irq_lock = SPIN_LOCK_UNLOCKED;

    gtp_irq_enable(ts);

#if GTP_CREATE_WR_NODE
    init_wr_node(client);
#endif

#if GTP_ESD_PROTECT
    INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE); 
#endif

    return 0;
}


/*******************************************************
Function:
	Goodix touchscreen driver release function.

Input:
	client:	i2c device struct.
	
Output:
	Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
	
    GTP_DEBUG_FUNC();
	
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif

#if GTP_CREATE_WR_NODE
    uninit_wr_node();
#endif

#if GTP_ESD_PROTECT
    destroy_workqueue(gtp_esd_check_workqueue);
#endif

    if (ts) 
    {
        if (ts->use_irq)
        {
            GTP_GPIO_AS_INPUT(gt9110_int_number);
            GTP_GPIO_FREE(gt9110_int_number);
            free_irq(client->irq, ts);
        }
        else
        {
            hrtimer_cancel(&ts->timer);
        }
    }	
	
    GTP_INFO("GTP driver is removing...");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    kfree(ts);

    return 0;
}

/*******************************************************
Function:
	Early suspend function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;	
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	
    GTP_DEBUG_FUNC();

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 1;
    cancel_delayed_work_sync(&gtp_esd_check_work);
#endif

    if (ts->use_irq)
    {
        gtp_irq_disable(ts);
    }
    else
    {
        hrtimer_cancel(&ts->timer);
    }
    ret = gtp_enter_sleep(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP early suspend failed.");
    }
}

/*******************************************************
Function:
	Late resume function.

Input:
	h:early_suspend struct.
	
Output:
	None.
*******************************************************/
static void goodix_ts_late_resume(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;
    ts = container_of(h, struct goodix_ts_data, early_suspend);
	
    GTP_DEBUG_FUNC();
	
    ret = gtp_wakeup_sleep(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP later resume failed.");
    }

    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
    else
    {
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 0;
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
#endif
}
#endif

#if GTP_ESD_PROTECT
static void gtp_esd_check_func(struct work_struct *work)
{
    s32 i;
    s32 ret = -1;
    struct goodix_ts_data *ts = NULL;
    u8 test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

    GTP_DEBUG_FUNC();

    ts = i2c_get_clientdata(i2c_connect_client);

    if (ts->gtp_is_suspend)
    {
        return;
    }
    
    for (i = 0; i < 3; i++)
    {
        ret = gtp_i2c_read(i2c_connect_client, test, 3);
        if (ret >= 0)
        {
            break;
        }
    }
    
    if (i >= 3)
    {
        gtp_reset_guitar(ts->client, 50);
    }

    if(!ts->gtp_is_suspend)
    {
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
    }

    return;
}
#endif

static const struct i2c_device_id goodix_ts_id[] = {
    { GTP_I2C_NAME, 0 },
    { }
};

static struct i2c_driver goodix_ts_driver = {
    .probe      = goodix_ts_probe,
    .remove     = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = goodix_ts_early_suspend,
    .resume     = goodix_ts_late_resume,
#endif
    .id_table   = goodix_ts_id,
    .driver = {
        .name     = GTP_I2C_NAME,
        .owner    = THIS_MODULE,
    },
};

/*******************************************************	
Function:
	Driver Install function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit goodix_ts_init(void)
{
    s32 ret;

    GTP_DEBUG_FUNC();	
    GTP_INFO("GTP driver install.");
    goodix_wq = create_singlethread_workqueue("goodix_wq");
    if (!goodix_wq)
    {
        GTP_ERROR("Creat workqueue failed.");
        return -ENOMEM;
    }
    ret = i2c_add_driver(&goodix_ts_driver);
    return ret; 
}

/*******************************************************	
Function:
	Driver uninstall function.
Input:
  None.
Output:
	Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
    GTP_DEBUG_FUNC();
    GTP_INFO("GTP driver exited.");
    i2c_del_driver(&goodix_ts_driver);
    if (goodix_wq)
    {
        destroy_workqueue(goodix_wq);
    }
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");
