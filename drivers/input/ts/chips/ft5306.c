/* drivers/input/ts/chips/ts_ft5306.c
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
	 
static struct i2c_client *this_client;
static int  reset_pin;
	 
#if 0
#define DBG(x...)  printk(x)
#else
#define DBG(x...)
#endif


#define FT5306_ID_REG		0x00
#define FT5306_DEVID		0x00
#define FT5306_DATA_REG		0x00


#define CONFIG_SUPPORT_FTS_CTP_UPG
#ifdef CONFIG_SUPPORT_FTS_CTP_UPG

typedef enum
{
    ERR_OK,
    ERR_MODE,
    ERR_READID,
    ERR_ERASE,
    ERR_STATUS,
    ERR_ECC,
    ERR_DL_ERASE_FAIL,
    ERR_DL_PROGRAM_FAIL,
    ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

typedef unsigned char         FTS_BYTE;     
typedef unsigned short        FTS_WORD;
typedef unsigned int          FTS_DWRD;  
typedef unsigned char         FTS_BOOL; 

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE              0x0

#define I2C_CTPM_ADDRESS       0x70

void delay_qt_ms(unsigned long  w_ms)
{
    unsigned long i;
    unsigned long j;

    for (i = 0; i < w_ms; i++)
    {
        for (j = 0; j < 855; j++)
        {
            udelay(1);
        }
    }
}

static int ft5x0x_i2c_txdata(char *txdata, int length)
{

	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
			.scl_rate = 200000,
		},
	};

	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
 
}

/*
[function]: 
    callback: read data from ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[out]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
FTS_BOOL i2c_read_interface(FTS_BYTE bt_ctpm_addr, FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    int ret;
    
    ret=i2c_master_recv(this_client, pbt_buf, dw_lenth);

    if(ret<=0)
    {
        printk("[TSP]i2c_read_interface error\n");
        return FTS_FALSE;
    }
  
    return FTS_TRUE;
}

/*
[function]: 
    callback: write data to ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[in]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/

FTS_BOOL i2c_write_interface(FTS_BYTE bt_ctpm_addr, FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    int ret;
    ret=i2c_master_send(this_client, pbt_buf, dw_lenth);
    if(ret<=0)
    {
        printk("[TSP]i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);
        return FTS_FALSE;
    }

    return FTS_TRUE;
}

/*
[function]: 
    send a command to ctpm.
[parameters]:
    btcmd[in]        :command code;
    btPara1[in]    :parameter 1;    
    btPara2[in]    :parameter 2;    
    btPara3[in]    :parameter 3;    
    num[in]        :the valid input parameter numbers, if only command code needed and no parameters followed,then the num is 1;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/

FTS_BOOL cmd_write(FTS_BYTE btcmd,FTS_BYTE btPara1,FTS_BYTE btPara2,FTS_BYTE btPara3,FTS_BYTE num)
{
    FTS_BYTE write_cmd[4] = {0};

    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    return i2c_write_interface(I2C_CTPM_ADDRESS, write_cmd, num);
}

/*
[function]: 
    write data to ctpm , the destination address is 0.
[parameters]:
    pbt_buf[in]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_write(FTS_BYTE* pbt_buf, FTS_DWRD dw_len)
{
    
    return i2c_write_interface(I2C_CTPM_ADDRESS, pbt_buf, dw_len);
}

/*
[function]: 
    read out data from ctpm,the destination address is 0.
[parameters]:
    pbt_buf[out]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_read(FTS_BYTE* pbt_buf, FTS_BYTE bt_len)
{
    return i2c_read_interface(I2C_CTPM_ADDRESS, pbt_buf, bt_len);
}






static int ft5x0x_write_reg(u8 addr, u8 para)
{
    u8 buf[3];
    int ret = -1;

    buf[0] = addr;
    buf[1] = para;
    ret = ft5x0x_i2c_txdata(buf, 2);
    if (ret < 0) {
        pr_err("write reg failed! %#x ret: %d", buf[0], ret);
        return -1;
    }
    
    return 0;
}
 
static int ft5x0x_read_reg(u8 addr, u8 *pdata)
{
	int ret;
	u8 buf[2] = {0};
    struct i2c_msg msgs[2];

     buf[0] = addr;

     msgs[0].addr	= this_client->addr;
     msgs[0].flags	= 0;
     msgs[0].len	= 1;   
     msgs[0].buf	= buf;
     msgs[0].scl_rate  = 200000;

     msgs[1].addr	= this_client->addr;
     msgs[1].flags	= I2C_M_RD;
     msgs[1].len	= 1;  
     msgs[1].buf	= buf;
  


	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	*pdata = buf[0];
	return ret;
} 
 
 
#define  FT5X0X_REG_FIRMID       0xa6
static unsigned char ft5x0x_read_fw_ver(void)
{
	unsigned char ver;
	if(ft5x0x_read_reg(FT5X0X_REG_FIRMID, &ver)<0)
	{
	   ver = 0xff; /*invailed*/
	}
	return(ver);
}


#define    FTS_PACKET_LENGTH        128

static unsigned char CTPM_FW[]=
{
	 #include "ft5406_ver_15.h"
};

/*
[function]: 
    burn the FW to ctpm.
[parameters]:(ref. SPEC)
    pbt_buf[in]    :point to Head+FW ;
    dw_lenth[in]:the length of the FW + 6(the Head length);    
    bt_ecc[in]    :the ECC of the FW
[return]:
    ERR_OK        :no error;
    ERR_MODE    :fail to switch to UPDATE mode;
    ERR_READID    :read id fail;
    ERR_ERASE    :erase chip fail;
    ERR_STATUS    :status error;
    ERR_ECC        :ecc error.
*/

E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    FTS_BYTE reg_val[2] = {0};
    FTS_DWRD i = 0;

    FTS_DWRD  packet_number;
    FTS_DWRD  j;
    FTS_DWRD  temp;
    FTS_DWRD  lenght;
    FTS_BYTE  packet_buf[FTS_PACKET_LENGTH + 6];
    FTS_BYTE  auc_i2c_write_buf[10];
    FTS_BYTE bt_ecc;
    int      i_ret;
    j= 0;
    
    unsigned char au_delay_timings[11] = {30, 33, 36, 39, 42, 45, 27, 24,21,18,15}; 

   /* Below code is used fot test delay_qt_ms timing */
   /*
   for(i = 0; i<1000; i++)
   {    
       gpio_direction_output(reset_pin, GPIO_LOW);
       delay_qt_ms(10);
       gpio_direction_output(reset_pin, GPIO_HIGH);
       delay_qt_ms(10);
   }
   i = 0;*/

    /*********Step 1:Reset  CTPM *****/
    /*write 0xaa to register 0xfc*/  
   // ft5x0x_write_reg(0xfc,0xaa);
   // delay_qt_ms(50);
     /*write 0x55 to register 0xfc*/
   // ft5x0x_write_reg(0xfc,0x55);
   // delay_qt_ms(30);   
    
UPGR_START:
    
    /* use hardware reset method instead of software method */
    gpio_direction_output(reset_pin, GPIO_LOW);
    delay_qt_ms(50);
    gpio_direction_output(reset_pin, GPIO_HIGH);
    delay_qt_ms(au_delay_timings[j]); 
    
    printk("[TSP] Step 1: Reset CTPM test\n");
   
    /*********Step 2:Enter upgrade mode *****/
    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do
    {
        i ++;
        i_ret = ft5x0x_i2c_txdata(auc_i2c_write_buf, 2);
        delay_qt_ms(5);
    }while(i_ret <= 0 && i < 5 );

    /*********Step 3:check READ-ID***********************/    
    delay_qt_ms(100);     
    cmd_write(0x90,0x00,0x00,0x00,4);
    byte_read(reg_val,2);

    if (reg_val[0] == 0x79 && reg_val[1] == 0x3)
    {
        printk("[TSP] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    }
    else
    {
	if (j < 10)
	{
	     j ++;
	     //msleep(200);
	     printk("[TSP] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
	     printk("[FTS]goto UPGR_START!\n");
	     goto UPGR_START; 
	}
	else
	{
    	
	    printk("[TSP] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
	    gpio_direction_output(reset_pin, GPIO_LOW);
	    delay_qt_ms(50);
	    gpio_direction_output(reset_pin, GPIO_HIGH);
	    delay_qt_ms(40); 
	    return ERR_READID;
	}
    }

     /*********Step 4:erase app*******************************/
    cmd_write(0x61,0x00,0x00,0x00,1);
   
    delay_qt_ms(1500);
    printk("[TSP] Step 4: erase. \n");

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    printk("[TSP] Step 5: start upgrade. \n");
    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(lenght>>8);
        packet_buf[5] = (FTS_BYTE)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++)
        {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }
        
        byte_write(&packet_buf[0],FTS_PACKET_LENGTH + 6);
        delay_qt_ms(FTS_PACKET_LENGTH/6 + 1);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0)
        {
              printk("[TSP] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;

        for (i=0;i<temp;i++)
        {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }

        byte_write(&packet_buf[0],temp+6);    
        delay_qt_ms(20);
    }

    /*send the last six byte*/
    for (i = 0; i<6; i++)
    {
        temp = 0x6ffa + i;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        temp =1;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i]; 
        bt_ecc ^= packet_buf[6];

        byte_write(&packet_buf[0],7);  
        delay_qt_ms(20);
    }

    /*********Step 6: read out checksum***********************/
    /*send the opration head*/
    cmd_write(0xcc,0x00,0x00,0x00,1);
    byte_read(reg_val,1);
    printk("[TSP] Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc)
    {
        return ERR_ECC;
    }

    /*********Step 7: reset the new FW***********************/
    cmd_write(0x07,0x00,0x00,0x00,1);

    return ERR_OK;
}


#define FTS_FACTORYMODE_VALUE		0x40
#define FTS_WORKMODE_VALUE		0x00

int fts_ctpm_auto_clb(void)
{
	unsigned char uc_temp = 0x00;
	unsigned char i = 0;

	printk("start auto clb.\n");
	/*start auto CLB */
	msleep(200);

	ft5x0x_write_reg(0, FTS_FACTORYMODE_VALUE);
	/*make sure already enter factory mode */
	msleep(100);
	/*write command to start calibration */
	ft5x0x_write_reg(2, 0x4);
	msleep(300);
	for (i = 0; i < 100; i++) {
		ft5x0x_read_reg(0, &uc_temp);
		/*return to normal mode, calibration finish */
		if (0x0 == ((uc_temp & 0x70) >> 4))
			break;
	}

	//msleep(200);
	/*calibration OK */
	msleep(300);
	ft5x0x_write_reg(0, FTS_FACTORYMODE_VALUE);	/*goto factory mode for store */
	msleep(100);	/*make sure already enter factory mode */
	ft5x0x_write_reg(2, 0x5);	/*store CLB result */
	msleep(300);
	ft5x0x_write_reg(0, FTS_WORKMODE_VALUE);	/*return to normal mode */
	msleep(300);

	printk("end auto clb.\n");
	/*store CLB result OK */
	return 0;
}

int fts_ctpm_fw_upgrade_with_i_file(void)
{
   FTS_BYTE*     pbt_buf = FTS_NULL;
   int i_ret;
    
    /* FW upgrade*/
   pbt_buf = CTPM_FW;
   /*call the upgrade function*/
   i_ret =  fts_ctpm_fw_upgrade(pbt_buf,sizeof(CTPM_FW));
   if (i_ret != 0)
   {
       //error handling ...
       printk("fts_ctpm_fw_upgrade failed!\n");
   }
   else
   {
	fts_ctpm_auto_clb();
   }

   return i_ret;
}

unsigned char fts_ctpm_get_upg_ver(void)
{
    unsigned int ui_sz;
    ui_sz = sizeof(CTPM_FW);
    if (ui_sz > 2)
    {
        return CTPM_FW[ui_sz - 2];
    }
    else
    {
         //TBD, error handling
        return 0xff; //default value
    }
}

#endif


/****************operate according to ts chip:start************/

static int ts_active(struct ts_private_data *ts, int enable)
{	
	int result = 0;

	if(enable)
	{
		gpio_direction_output(ts->pdata->reset_pin, GPIO_LOW);
		msleep(10);
		gpio_direction_output(ts->pdata->reset_pin, GPIO_HIGH);
		msleep(100);
	}
	else
	{
		gpio_direction_output(ts->pdata->reset_pin, GPIO_LOW);	
	}
		
	
	return result;
}

static int ts_init(struct ts_private_data *ts)
{
	int irq_pin = irq_to_gpio(ts->pdata->irq);
	int result = 0;
	int uc_reg_value ;
	
	//gpio_direction_output(ts->pdata->reset_pin, GPIO_LOW);
	//mdelay(10);
	//gpio_direction_output(ts->pdata->reset_pin, GPIO_HIGH);
	//msleep(300);

	//init some register
	//to do
	printk("ft5306 touchscreen  init\n");
	this_client =  ts->control_data;
	reset_pin = ts->pdata->reset_pin;
	
	uc_reg_value = ft5x0x_read_fw_ver();
    	printk("[FST] Firmware version = 0x%x\n", uc_reg_value);

#ifdef  CONFIG_SUPPORT_FTS_CTP_UPG  	
    	if( uc_reg_value != 0xff )
	{
          /* compare to last version, if not equal,do update! 
           0xa6 means update failed last time*/  
 		if(( uc_reg_value < 0x15) || ( uc_reg_value == 0xa6))
		    fts_ctpm_fw_upgrade_with_i_file();
	}
#endif	
	return result;
}


static int ts_report_value(struct ts_private_data *ts)
{
	struct ts_platform_data *pdata = ts->pdata;
	struct ts_event *event = &ts->event;
	unsigned char buf[32] = {0};
	int result = 0 , i = 0, off = 0, id = 0;

	result = ts_bulk_read(ts, (unsigned short)ts->ops->read_reg, ts->ops->read_len, buf);
	if(result < 0)
	{
		printk("%s:fail to init ts\n",__func__);
		return result;
	}

	//for(i=0; i<ts->ops->read_len; i++)
	//DBG("buf[%d]=0x%x\n",i,buf[i]);
	
	event->touch_point = buf[2] & 0x07;// 0000 1111

	for(i=0; i<ts->ops->max_point; i++)
	{
		event->point[i].status = 0;
		event->point[i].x = 0;
		event->point[i].y = 0;
	}

	if(event->touch_point == 0)
	{	
		for(i=0; i<ts->ops->max_point; i++)
		{
			if(event->point[i].last_status != 0)
			{
				event->point[i].last_status = 0;				
				input_mt_slot(ts->input_dev, event->point[i].id);				
				input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
				input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
				DBG("%s:%s press up,id=%d\n",__func__,ts->ops->name, event->point[i].id);
			}
		}
		
		input_sync(ts->input_dev);
		memset(event, 0x00, sizeof(struct ts_event));
				
		return 0;
	}

	for(i = 0; i<event->touch_point; i++)
	{
		off = i*6+3;
		id = (buf[off+2] & 0xf0) >> 4;				
		event->point[id].id = id;
		event->point[id].status = (buf[off+0] & 0xc0) >> 6;
		event->point[id].x = ((buf[off+0] & 0x0f)<<8) | buf[off+1];
		event->point[id].y = ((buf[off+2] & 0x0f)<<8) | buf[off+3];
		
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
		
	}

	for(i=0; i<ts->ops->max_point; i++)
	{
		if(event->point[i].status != 0)
		{		
			input_mt_slot(ts->input_dev, event->point[i].id);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, event->point[i].id);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, event->point[i].x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, event->point[i].y);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 1);
	   		DBG("%s:%s press down,id=%d,x=%d,y=%d\n",__func__,ts->ops->name, event->point[id].id, event->point[id].x,event->point[id].y);
		}
		else if ((event->point[i].status == 0) && (event->point[i].last_status != 0))
		{				
			input_mt_slot(ts->input_dev, event->point[i].id);				
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
			DBG("%s:%s press up1,id=%d\n",__func__,ts->ops->name, event->point[i].id);
		}

		event->point[i].last_status = event->point[i].status;
	}
	input_sync(ts->input_dev);

	return 0;
}

static int ts_suspend(struct ts_private_data *ts)
{
	struct ts_platform_data *pdata = ts->pdata;
	int result = 0;

	if(ts->ops->active)
		ts->ops->active(ts, 0);
	msleep(5);
	if(ts->ops->active)
		ts->ops->active(ts, 1);
	msleep(20);

	/* For ft5406 , 0xa5 is power mode register , set 0x03 to the register ,ic will enter standy mode*/
	result =  ft5x0x_write_reg(0xa5,0x03);
	if( result >= 0 )
	{
		printk("Touchsreen ft5406 Send  suspend command sucessfully\n");
	}
	else
	{
		printk("Touchsreen ft5406 Send  suspend command failed\n");	
	}	
	
	return 0;
}


static int ts_resume(struct ts_private_data *ts)
{
	struct ts_platform_data *pdata = ts->pdata;
	
	if(ts->ops->active)
		ts->ops->active(ts, 0);
	
	msleep(5);
	
	if(ts->ops->active)
		ts->ops->active(ts, 1);
	return 0;
}


struct ts_operate ts_ft5306_ops = {
	.name				= "ft5306",
	.slave_addr			= 0x3e,
	.ts_id				= TS_ID_FT5306,			//i2c id number
	.bus_type			= TS_BUS_TYPE_I2C,
	.reg_size			= 1,
	.id_reg				= FT5306_ID_REG,
	.id_data			= TS_UNKNOW_DATA,
	.version_reg			= TS_UNKNOW_DATA,
	.version_len			= 0,
	.version_data			= NULL,
	.read_reg			= FT5306_DATA_REG,		//read data
	.read_len			= 32,				//data length
	.trig				= IRQF_TRIGGER_FALLING,		
	.max_point			= 5,
	.xy_swap 			= 1,
	.x_revert 			= 1,
	.y_revert			= 0,
	.range				= {1024,768},
	.irq_enable			= 1,
	.poll_delay_ms			= 0,
	.active				= ts_active,	
	.init				= ts_init,
	.check_irq			= NULL,
	.report 			= ts_report_value,
	.firmware			= NULL,
	.suspend			= ts_suspend,
	.resume				= ts_resume,
};

/****************operate according to ts chip:end************/

//function name should not be changed
static struct ts_operate *ts_get_ops(void)
{
	return &ts_ft5306_ops;
}


static int __init ts_ft5306_init(void)
{
	struct ts_operate *ops = ts_get_ops();
	int result = 0;
	result = ts_register_slave(NULL, NULL, ts_get_ops);	
	DBG("%s\n",__func__);
	return result;
}

static void __exit ts_ft5306_exit(void)
{
	struct ts_operate *ops = ts_get_ops();
	ts_unregister_slave(NULL, NULL, ts_get_ops);
}


subsys_initcall(ts_ft5306_init);
module_exit(ts_ft5306_exit);

