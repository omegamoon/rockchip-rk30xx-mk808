#include <linux/delay.h>
#include <linux/i2c.h>

#include "anx7150_hw.h"
#include "anx7150.h"

static unsigned char anx7150_in_pix_rpt_bkp, anx7150_tx_pix_rpt_bkp;
static unsigned char ANX7150_hdcp_init_done,ANX7150_hdcp_auth_fail_counter, ANX7150_hdcp_auth_pass;
static unsigned char ANX7150_hdcp_bcaps;

static int anx7150_blue_screen(struct i2c_client *client, int enable);
static int anx7150_hdcp_encryption(struct i2c_client *client, int enable);
static int anx7150_set_avmute(struct i2c_client *client);


static void anx7150_show_video_info(struct i2c_client *client)
{
	int rc;
	char c, c1;	

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c1);
    c1 = (c1 & 0x02);
    if (c1)
    {
        hdmi_dbg(&client->dev, "   Output video mode = HDMI\n");
		rc = anx7150_i2c_read_p1_reg(client, 0x04, &c);
        c = (c & 0x60) >> 5;
        switch (c)
        {
            case ANX7150_RGB:
                hdmi_dbg(&client->dev, "   Output video color format = RGB\n");
                break;
            case ANX7150_YCbCr422:
                hdmi_dbg(&client->dev, "   Output video color format = YCbCr422\n");
                break;
            case ANX7150_YCbCr444:
                hdmi_dbg(&client->dev, "   Output video color format = YCbCr444\n");
                break;
            default:
                break;
        }
    }
    else
    {
        hdmi_dbg(&client->dev, "   Output video mode = DVI\n");
        hdmi_dbg(&client->dev, "   Output video color format = RGB\n");
    }
}

static void anx7150_show_audio_info(struct i2c_client *client)
{
	int rc;
	char c;
	
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL0_REG, &c);
    c = c & 0x03;
    switch (c)
    {
        case 0x00:
            hdmi_dbg(&client->dev, "   MCLK Frequence = 128 * Fs.\n");
            break;
        case 0x01:
            hdmi_dbg(&client->dev, "   MCLK Frequence = 256 * Fs.\n");
            break;
        case 0x02:
            hdmi_dbg(&client->dev, "   MCLK Frequence = 384 * Fs.\n");
            break;
        case 0x03:
            hdmi_dbg(&client->dev, "   MCLK Frequence = 512 * Fs.\n");
            break;
        default :
            hdmi_dbg(&client->dev, "Wrong MCLK output.\n");
            break;
    }

    if ( ANX7150_AUD_HW_INTERFACE == 0x01)
    {
        hdmi_dbg(&client->dev, "   Input Audio Interface = I2S.\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_I2SCH_STATUS4_REG, &c);
    }
    else if (ANX7150_AUD_HW_INTERFACE == 0x02)
    {
        hdmi_dbg(&client->dev, "   Input Audio Interface = SPDIF.\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_SPDIFCH_STATUS_REG, &c);
        c=c>>4;
    }
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_I2SCH_STATUS4_REG, &c);
    c &= 0x0f;
    switch (c)
    {
        case 0x00:
            hdmi_dbg(&client->dev, "   Audio Fs = 44.1 KHz.\n");
            break;
        case 0x02:
			hdmi_dbg(&client->dev, "   Audio Fs = 48 KHz.\n");
            break;
        case 0x03:
			hdmi_dbg(&client->dev, "   Audio Fs = 32 KHz.\n");
            break;
        case 0x08:
			hdmi_dbg(&client->dev, "   Audio Fs = 88.2 KHz.\n");
            break;
        case 0x0a:
			hdmi_dbg(&client->dev, "   Audio Fs = 96 KHz.\n\n");
            break;
        case 0x0c:
			hdmi_dbg(&client->dev, "   Audio Fs = 176.4 KHz.\n");
            break;
        case 0x0e:
			hdmi_dbg(&client->dev, "   Audio Fs = 192 KHz.\n");
            break;
        default :
			hdmi_dbg(&client->dev, "   Audio Fs = Wrong Fs output.\n");
            break;
    }
}
/************** DDC Operation *******************/

/* Reset ddc channel. */
static int anx7150_rst_ddcchannel(struct i2c_client *client)
{
	int rc = 0;
	char c;
    //Reset the DDC channel
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);

	c |= (ANX7150_SYS_CTRL2_DDC_RST);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);

	c &= (~ANX7150_SYS_CTRL2_DDC_RST);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);


	c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_ACC_CMD_REG, &c);//abort current operation

	c = 0x06;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_ACC_CMD_REG, &c);//reset I2C command

	//Clear FIFO
	c = 0x05;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_ACC_CMD_REG, &c);//reset I2C command

	return rc;
}

/* Initialize ddc read. */
static int anx7150_initddc_read(struct i2c_client *client, 
								u8 devaddr, u8 segmentpointer,
                          		u8 offset, u8 access_num_Low,
                          		u8 access_num_high)
{
	int rc = 0;
	char c;

    //Write slave device address
    c = devaddr;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_SLV_ADDR_REG, &c);
    // Write segment address
    c = segmentpointer;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_SLV_SEGADDR_REG, &c);
    //Write offset
    c = offset;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_SLV_OFFADDR_REG, &c);
    //Write number for access
    c = access_num_Low;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_ACCNUM0_REG, &c);
	c = access_num_high;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_ACCNUM1_REG, &c);
    //Clear FIFO
    c = 0x05;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_ACC_CMD_REG, &c);
    //EDDC sequential Read
    c = 0x04;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_DDC_ACC_CMD_REG, &c);

	return rc;
}

/* Read 1 segment edid data. */
static int anx7150_ddc_mass_read(struct i2c_client *client, u32 length, u8 * buff)
{
//	struct anx7150_pdata *anx = (struct anx7150_pdata *)i2c_get_clientdata(client);
	int rc = 0;
    u32 i, j;
    char c, c1,ddc_empty_cnt;
    char ANX7150_ddc_fifo_full, ANX7150_ddc_progress;
	
	if(client == NULL || buff == NULL)
		return -1;
//	hdmi_dbg(&client->dev, "anx7150_ddc_mass_read");
	i = length;
    while (i > 0)
    {
        //check DDC FIFO statue
        rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_CHSTATUS_REG, &c);
        if (c & ANX7150_DDC_CHSTATUS_DDC_OCCUPY)
        {
            hdmi_dbg(&client->dev, "ANX7150 DDC channel is accessed by an external device, break!.\n");
            break;
        }
        if (c & ANX7150_DDC_CHSTATUS_FIFO_FULL)
            ANX7150_ddc_fifo_full = 1;
        else
            ANX7150_ddc_fifo_full = 0;
        if (c & ANX7150_DDC_CHSTATUS_INPRO)
            ANX7150_ddc_progress = 1;
        else
            ANX7150_ddc_progress = 0;
        if (ANX7150_ddc_fifo_full)
        {
//            hdmi_dbg(&client->dev, "DDC FIFO is full during edid reading");
			rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFOCNT_REG, &c);
//            hdmi_dbg(&client->dev, "FIFO counter is %.2x\n", (u32) c);
            for (j=0; j<c; j++)
            {
            	rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFO_ACC_REG, &c1);
                buff[length - i + j] = c1;
                    //D("EDID[0x%.2x]=0x%.2x    ", (u32)(length - i + j), (u32) c1);

                ANX7150_ddc_fifo_full = 0;
            }
            i = i - c;
            //D("\n");
        }else if(!ANX7150_ddc_progress)
        {
            hdmi_dbg(&client->dev, "ANX7150 DDC FIFO access finished.\n");
            rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFOCNT_REG, &c);
            hdmi_dbg(&client->dev, "FIFO counter is %.2x\n", (u32) c);
            if (!c)
            {
                i =0;
                break;
            }
            for (j=0; j<c; j++)
            {
            	rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFO_ACC_REG, &c1);
                buff[length - i + j] = c1;
                    //D("EDID[0x%.2x]=0x%.2x    ", (u32)(length - i + j), (u32) c1);
            }
            i = i - c;
            //D("\ni=%d\n", i);
        }
        else
        {
            ddc_empty_cnt = 0x00;
            for (c1=0; c1<0x0a; c1++)
            {
            	rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_CHSTATUS_REG, &c);
//                hdmi_dbg(&client->dev, "DDC FIFO access is progressing.\n");
                //D("DDC Channel status is 0x%.2x\n",(u32)c);
                if (c & ANX7150_DDC_CHSTATUS_FIFO_EMPT)
                    ddc_empty_cnt++;
                else
                	break;
                msleep(5);
                //D("ddc_empty_cnt =  0x%.2x\n",(u32)ddc_empty_cnt);
            }
            if (ddc_empty_cnt >= 0x0a)
                i = 0;
        }
    }
	return rc;
}

//******************** HDCP process ********************************
/* Reset HDCP */
static int anx7150_clean_hdcp(struct i2c_client *client)
{
	int rc = 0;
	char c;
    //mute TMDS link
    //ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);//jack wen
    //ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, c & (~ANX7150_TMDS_CLKCH_MUTE));

    //Disable hardware HDCP
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
	c &= (~ANX7150_HDCP_CTRL0_HW_AUTHEN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
   
    //Reset HDCP logic
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_SRST_REG, &c);
	c |= (ANX7150_SRST_HDCP_RST);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SRST_REG, &c);
	c &= (~ANX7150_SRST_HDCP_RST);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SRST_REG, &c);

    //Set ReAuth
     rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
	c |= (ANX7150_HDCP_CTRL0_RE_AUTH);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
	c &= (~ANX7150_HDCP_CTRL0_RE_AUTH);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
    
    rc = anx7150_rst_ddcchannel(client);

	return rc;
}

/* Initialize hdcp auth, detect sink type */
static int anx7150_hardware_hdcp_auth_init(struct i2c_client *client)
{
	int rc = 0;
    u8 c;

//    ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c); //72:07.2 hdcp on
//    ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL1_REG, (c | ANX7150_SYS_CTRL1_HDCPMODE));
	// disable hw hdcp
//    ANX7150_i2c_read_p0_reg(ANX7150_HDCP_CTRL0_REG, &c);
//    ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, (c & (~ANX7150_HDCP_CTRL0_HW_AUTHEN)));

    //ANX7150_i2c_write_p0_reg(ANX7150_HDCP_CTRL0_REG, 0x03); //h/w auth off, jh simplay/hdcp
	c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c); //bit 0/1 off, as from start, we don't know if Bksv/srm/KSVList valid or not. SY.

    // DDC reset
    rc = anx7150_rst_ddcchannel(client);

    anx7150_initddc_read(client, 0x74, 0x00, 0x40, 0x01, 0x00);
    mdelay(5);
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFO_ACC_REG, &ANX7150_hdcp_bcaps);
    hdmi_dbg(&client->dev, "ANX7150_Hardware_HDCP_Auth_Init(): ANX7150_hdcp_bcaps = 0x%.2x\n",    (u32)ANX7150_hdcp_bcaps);

    if (ANX7150_hdcp_bcaps & 0x02)
    {   //enable 1.1 feature
    	hdmi_dbg(&client->dev, "ANX7150_Hardware_HDCP_Auth_Init(): bcaps supports 1.1\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c);
		c |= ANX7150_HDCP_CTRL1_HDCP11_EN;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c);
     }
    else
    {   //disable 1.1 feature and enable HDCP two special point check
    	hdmi_dbg(&client->dev, "bcaps don't support 1.1\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c);
		c = ((c & (~ANX7150_HDCP_CTRL1_HDCP11_EN)) | ANX7150_LINK_CHK_12_EN);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c);
    }
    //handle repeater bit. SY.
    if (ANX7150_hdcp_bcaps & 0x40)
    {
	    //repeater
		hdmi_dbg(&client->dev, "ANX7150_Hardware_HDCP_Auth_Init(): bcaps shows Sink is a repeater\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
		c |= ANX7150_HDCP_CTRL0_RX_REP;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
	}
    else
    {
		//receiver
		hdmi_dbg(&client->dev, "ANX7150_Hardware_HDCP_Auth_Init(): bcaps shows Sink is a receiver\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
		c &= ~ANX7150_HDCP_CTRL0_RX_REP;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
	}
    anx7150_rst_ddcchannel(client);
//    ANX7150_hdcp_auth_en = 0;
	return rc;
}

// BKSV SRM check.
static int anx7150_bksv_srm(struct i2c_client *client)
{
	int rc = 0;
#if 1
    u8 bksv[5],i,bksv_one,c1;
    anx7150_initddc_read(client, 0x74, 0x00, 0x00, 0x05, 0x00);
    mdelay(15);
    for (i = 0; i < 5; i ++)
    {
    	rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFO_ACC_REG, &bksv[i]);
    }

    bksv_one = 0;
    for (i = 0; i < 8; i++)
    {
        c1 = 0x01 << i;
        if (bksv[0] & c1)
            bksv_one ++;
        if (bksv[1] & c1)
            bksv_one ++;
        if (bksv[2] & c1)
            bksv_one ++;
        if (bksv[3] & c1)
            bksv_one ++;
        if (bksv[4] & c1)
            bksv_one ++;
    }
    //wen HDCP CTS
    if (bksv_one != 20)
    {
        hdmi_dbg(&client->dev, "BKSV check fail\n");
        return 0;
    }
    else
    {
        hdmi_dbg(&client->dev, "BKSV check OK\n");
        return 1;
    }
#endif

#if 0					//wen HDCP CTS
    /*address by gerard.zhu*/
    u8 i,j,bksv_ones_count,bksv_data[Bksv_Data_Nums] = {0};
    ANX7150_DDC_Addr bksv_ddc_addr;
    u32 bksv_length;
    ANX7150_DDC_Type ddc_type;

    i = 0;
    j = 0;
    bksv_ones_count = 0;
    bksv_ddc_addr.dev_addr = HDCP_Dev_Addr;
    bksv_ddc_addr.sgmt_addr = 0;
    bksv_ddc_addr.offset_addr = HDCP_Bksv_Offset;
    bksv_length = Bksv_Data_Nums;
    ddc_type = DDC_Hdcp;

    if (!ANX7150_DDC_Read(bksv_ddc_addr, bksv_data, bksv_length, ddc_type))
    {
        /*Judge validity for Bksv*/
        while (i < Bksv_Data_Nums)
        {
            while (j < 8)
            {
                if (((bksv_data[i] >> j) & 0x01) == 1)
                {
                    bksv_ones_count++;
                }
                j++;
            }
            i++;
            j = 0;
        }
        if (bksv_ones_count != 20)
        {
            rk29printk ("!!!!BKSV 1s 20\n");					//update  rk29printk ("!!!!BKSV 1s 20\n");
            return 0;
        }
    }
    /*end*/

    D("bksv is ready.\n");
    // TODO: Compare the bskv[] value to the revocation list to decide if this value is a illegal BKSV. This is system depended.
    //If illegal, return 0; legal, return 1. Now just return 1
    return 1;
#endif
}

/* Set blue screen format */
static int anx7150_blue_screen_format_config(struct i2c_client *client, int colorspace)
{
 	int rc = 0 ;
	char c;
	
    // TODO:Add ITU 601 format.(Now only ITU 709 format added)
    switch (colorspace)
    {
        case ANX7150_RGB: //select RGB mode
        	c = 0x10;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN0_REG, &c);
			c = 0xeb;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN1_REG, &c);
			c = 0x10;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN2_REG, &c);
            break;
        case ANX7150_YCbCr422: //select YCbCr4:2:2 mode
        	c = 0x00;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN0_REG, &c);
			c = 0xad;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN1_REG, &c);
			c = 0x2a;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN2_REG, &c);
            break;
        case ANX7150_YCbCr444: //select YCbCr4:4:4 mode
        	c = 0x1a;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN0_REG, &c);
			c = 0xad;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN1_REG, &c);
			c = 0x2a;
        	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN2_REG, &c);
            break;
        default:
            break;
    }
	return rc;
}

/* Enable or disable video output blue screen */
static int anx7150_blue_screen(struct i2c_client *client, int enable)
{
	int rc = 0;
	char c;

	if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c)) < 0)
		return rc;
		
	if(enable)
		c |= (ANX7150_HDCP_CTRL1_BLUE_SCREEN_EN);
	else
		c &= (0xfb);
	if((rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c)) < 0)
		return rc;
	
	return rc;
}

// Enable or disable HDCP encryption.
static int anx7150_hdcp_encryption(struct i2c_client *client, int enable)
{
	int rc = 0;
	u8 c;
	
	if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c)) < 0)
		return rc;

	if(enable)
		c |= (ANX7150_HDCP_CTRL0_ENC_EN);
	else
		c &= (0xfb);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
	return rc;
}

// Check HDMI repeater HDCP ksvlist
static int anx7150_is_ksvlist_vld(struct i2c_client *client)
{
	int rc = 0;
	u8 ANX7150_hdcp_bstatus[2];
//wen HDCP CTS
#if 1
    hdmi_dbg(&client->dev, "ANX7150_IS_KSVList_VLD() is called.\n");
    anx7150_initddc_read(client, 0x74, 0x00, 0x41, 0x02, 0x00); //Bstatus, two u8s
    mdelay(5);
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFO_ACC_REG, &ANX7150_hdcp_bstatus[0]);
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_DDC_FIFO_ACC_REG, &ANX7150_hdcp_bstatus[1]);

    if ((ANX7150_hdcp_bstatus[0] & 0x80) | (ANX7150_hdcp_bstatus[1] & 0x08))
    {
        hdmi_dbg(&client->dev, "Max dev/cascade exceeded: ANX7150_hdcp_bstatus[0]: 0x%x,ANX7150_hdcp_bstatus[1]:0x%x\n", (u32)ANX7150_hdcp_bstatus[0],(u32)ANX7150_hdcp_bstatus[1]);
        return 0;//HDCP topology error. More than 127 RX are attached or more than seven levels of repeater are cascaded.
    }
    return 1;
#endif
//wen HDCP CTS

}

// Authentication done process.
static int anx7150_auth_done(struct i2c_client *client)
{
	int rc = 0;
    char c;

	hdmi_dbg(&client->dev, "anx7150 auth done\n");
	
	if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_STATUS_REG, &c)) < 0)
		return rc;
	
    if (c & ANX7150_HDCP_STATUS_AUTH_PASS)
    {
        hdmi_dbg(&client->dev, "ANX7150_Authentication pass in Auth_Done\n");
        //Clear the SRM_Check_Pass u8, then when reauthentication occurs, firmware can catch it.
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
        c &= 0xfc;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
		
		if(ANX7150_hdcp_bcaps & 0x40) // It's a repeater.
		{
			//actually it is KSVList check. we can't do SRM check due to the lack of SRM file. SY.
			rc = anx7150_is_ksvlist_vld(client);
			if(rc == 0)	
			{
				hdmi_dbg(&client->dev, "ksvlist not good. disable encryption");
				anx7150_blue_screen(client, 1);
            	anx7150_hdcp_encryption(client, 0);
            	anx7150_clean_hdcp(client);
            	return rc;
			}
		}
		anx7150_hdcp_encryption(client, 1);
        anx7150_blue_screen(client, 0);
        ANX7150_hdcp_auth_pass = 1;
        ANX7150_hdcp_auth_fail_counter = 0;
    }
    else
    {
        hdmi_dbg(&client->dev, "ANX7150_Authentication failed\n");
//		ANX7150_hdcp_wait_100ms_needed = 1;
//		ANX7150_auth_fully_pass = 0;
        ANX7150_hdcp_auth_pass = 0;
        ANX7150_hdcp_auth_fail_counter ++;
        if (ANX7150_hdcp_auth_fail_counter >= ANX7150_HDCP_FAIL_THRESHOLD)
        {
            ANX7150_hdcp_auth_fail_counter = 0;
            //ANX7150_bksv_ready = 0;
            // TODO: Reset link;
            rc = anx7150_blue_screen(client, 1);
            rc = anx7150_hdcp_encryption(client, 0);
            //disable audio
            rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
			c &= (~ANX7150_HDMI_AUDCTRL1_IN_EN);
			rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
        }
    }
	return rc;
}

/* Authentication change process */
static int anx7150_auth_change(struct i2c_client *client)
{
	int rc = 0;
    char c;
	
//	int state = ANX7150_Get_System_State();
	
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_STATUS_REG, &c);
    if (c & ANX7150_HDCP_STATUS_AUTH_PASS)
    {
//        ANX7150_hdcp_auth_pass = 1;
        hdmi_dbg(&client->dev, "ANX7150_Authentication pass in Auth_Change\n");
    }
    else
    {
        rc = anx7150_set_avmute(client); //wen
        hdmi_dbg(&client->dev, "ANX7150_Authentication failed_by_Auth_change\n");
//        ANX7150_hdcp_auth_pass = 0;
//        ANX7150_hdcp_wait_100ms_needed = 1;
//        ANX7150_auth_fully_pass = 0;
//        ANX7150_hdcp_init_done=0;   //wen HDCP CTS
//        ANX7150_hdcp_auth_en=0;   //wen HDCP CTS
        rc = anx7150_hdcp_encryption(client, 0);
//        if (state == PLAY_BACK)
//        {
//            ANX7150_auth_fully_pass = 0;
//            //disable audio
//            rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
//			c &= (~ANX7150_HDMI_AUDCTRL1_IN_EN);
//			rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
//            rc = anx7150_clean_hdcp(client);							//wen updated for Changhong TV
//        }
    }
	return rc;
}

static int anx7150_get_interrupt_status(struct i2c_client *client, struct anx7150_interrupt_s *interrupt_staus)
{
	int rc = 0;
	u8 int_s1;
	u8 int_s2;
	u8 int_s3;
	
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR1_STATUS_REG, &int_s1);//jack wen, for spdif input from SD0.
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_INTR1_STATUS_REG, &int_s1);//power down all, 090630
	rc |= anx7150_i2c_read_p0_reg(client, ANX7150_INTR2_STATUS_REG, &int_s2);//jack wen, for spdif input from SD0.
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_INTR2_STATUS_REG, &int_s2);//power down all, 090630
	rc |= anx7150_i2c_read_p0_reg(client, ANX7150_INTR3_STATUS_REG, &int_s3);//jack wen, for spdif input from SD0.
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_INTR3_STATUS_REG, &int_s3);//power down all, 090630

	interrupt_staus->hotplug_change = (int_s1 & ANX7150_INTR1_STATUS_HP_CHG) ? 1 : 0;
	interrupt_staus->video_format_change = (int_s3 & ANX7150_INTR3_STATUS_VIDF_CHG) ? 1 : 0;
	interrupt_staus->bksv_ready = (int_s2 & ANX7150_INTR2_STATUS_BKSV_RDY) ? 1 : 0;
	interrupt_staus->auth_done = (int_s2 & ANX7150_INTR2_STATUS_AUTH_DONE) ? 1 : 0;
	interrupt_staus->auth_state_change = (int_s2 & ANX7150_INTR2_STATUS_AUTH_CHG) ? 1 : 0;
	interrupt_staus->pll_lock_change = (int_s2 & ANX7150_INTR2_STATUS_PLLLOCK_CHG) ? 1 : 0;
	interrupt_staus->rx_sense_change = (int_s3 & ANX7150_INTR3_STATUS_RXSEN_CHG) ? 1 : 0;
	interrupt_staus->HDCP_link_change = (int_s2 & ANX7150_INTR2_STATUS_HDCPLINK_CHK) ? 1 : 0;
	interrupt_staus->audio_clk_change = (int_s3 & ANX7150_INTR3_STATUS_AUDCLK_CHG) ? 1 : 0;
	interrupt_staus->audio_FIFO_overrun = (int_s1 & ANX7150_INTR1_STATUS_AFIFO_OVER) ? 1 : 0;
	interrupt_staus->SPDIF_error = (int_s1 & ANX7150_INTR1_STATUS_SPDIF_ERR) ? 1 : 0;
	interrupt_staus->SPDIF_bi_phase_error = ((int_s3 & ANX7150_INTR3_STATUS_SPDIFBI_ERR) ? 1 : 0) 
										|| ((int_s3 & ANX7150_INTR3_STATUS_SPDIF_UNSTBL) ? 1 : 0);
	return rc;
}

static int anx7150_set_avmute(struct i2c_client *client)
{
	int rc = 0;
	char c;

	// set mute flag
	c = 0x01;
	if((rc = anx7150_i2c_write_p1_reg(client, ANX7150_GNRL_CTRL_PKT_REG, &c)) < 0)
		return rc;
	// Enable control of Control Packet transmission.
	if((rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c)) < 0)
		return rc;
	c |= (0x0c);
	if((rc = anx7150_i2c_write_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c)) < 0)
		return rc;

	return rc;
}

static int anx7150_clear_avmute(struct i2c_client *client)
{
	int rc = 0;
	char c;
	
	c = 0x02;
	if((rc = anx7150_i2c_write_p1_reg(client, ANX7150_GNRL_CTRL_PKT_REG, &c)) < 0)
		return rc;
	
	if((rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c)) < 0)
		return rc;
	c |= (0x0c);
	if((rc = anx7150_i2c_write_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c)) < 0)
		return rc;
	return rc;
}

static int anx7150_color_space_conversion_config(struct i2c_client *client, int cspace_y2r, int y2r_sel, int range_y2r)
{
	int rc = 0;
	char c;
	
	if (cspace_y2r)
    {
        hdmi_dbg(&client->dev, "Color space Y2R enabled********\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
		c |= (ANX7150_VID_MODE_CSPACE_Y2R);
		// Set color space conversion mode.
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
        if (y2r_sel)
        {
            // Base on BT709
            hdmi_dbg(&client->dev, "Y2R_SEL!\n");
			rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
			c |= (ANX7150_VID_MODE_Y2R_SEL);
			rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
        }
        else
        {
        	// Base on BT601
        	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
			c &= (~ANX7150_VID_MODE_Y2R_SEL);
			rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);	
        }
        // Set YCbCr to RGB data range scaling
		if (range_y2r)
	    {
	    	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
			c |= (ANX7150_VID_MODE_RANGE_Y2R);
			rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
	    }
	    else
	    {
	    	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
			c &= (~ANX7150_VID_MODE_RANGE_Y2R);
			rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
	    }
    }
    else
    {
    	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
		c &= (~ANX7150_VID_MODE_CSPACE_Y2R);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
    }
    
    return rc;
}

static u8 anx7150_checksum(infoframe_struct *p)
{
    u8 checksum = 0x00;
    u8 i;

    checksum = p->type + p->length + p->version;
    for (i=1; i <= p->length; i++)
    {
        checksum += p->pb_u8[i];
    }
    checksum = ~checksum;
    checksum += 0x01;

    return checksum;
}

/***************** Config audio ***********************/
static int anx7150_config_i2s(	struct i2c_client *client, 
								unsigned char channel_num,
								unsigned char fs,
								unsigned char word_length)
{
	int rc = 0;
	char c = 0x00;
//    char c1 = 0x00;

//    hdmi_dbg(&client->dev,"ANX7150: config i2s audio.\n");
	
	// Config HDMI Audio Control Register 0
	if(channel_num < 3)
		c = 1 << 2;
	else if(channel_num < 5)
		c = 3 << 2;
	else if(channel_num < 7)
		c = 7 << 2;
	else
		c = 0x0F << 2;
		
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
//    hdmi_dbg(&client->dev,"config i2s channel, c = 0x%.2x\n",(u32)c);
	
    //config i2s format
    c = (ANX7150_I2S_SHIFT_CTRL << 3) | (ANX7150_I2S_DIR_CTRL << 2)  |
        (ANX7150_I2S_WS_POL << 1) | ANX7150_I2S_JUST_CTRL;
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_I2S_CTRL_REG, &c);
//    hdmi_dbg(&client->dev,"config i2s format, c = 0x%.2x\n",(u32)c);

    //config i2s channel status(5 regs)
    c = 0x00;
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_I2SCH_STATUS1_REG, &c);
    c = 0x00;
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_I2SCH_STATUS2_REG, &c);
    c = 0x00;
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_I2SCH_STATUS3_REG, &c);

    c = fs;
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_I2SCH_STATUS4_REG, &c);
//    hdmi_dbg(&client->dev,"@@@@@@@@config i2s channel status4, c = 0x%.2x\n",(unsigned int)c);//jack wen

    c = word_length;
	rc |= anx7150_i2c_write_p0_reg(client, ANX7150_I2SCH_STATUS5_REG, &c);
//    hdmi_dbg(&client->dev,"config i2s channel status, c = 0x%.2x\n",(u32)c);

    return rc;
}

/*	
	Detect ANX7150 chip.
 */ 
int ANX7150_hw_detect_device(struct i2c_client *client)
{
    int i, rc = 0; 
    char d1 = 0, d2 = 0, d3 = 0;
    
    for (i=0; i<10; i++) 
    {    
        if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_DEV_IDL_REG, &d1)) < 0) 
            continue;
        if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_DEV_IDH_REG, &d2)) < 0) 
            continue;
        if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_DEV_REV_REG, &d3)) < 0) 
            continue;
        if (d1 == 0x50 && d2 == 0x71)
        {    
            hdmi_dbg(&client->dev, "anx7150 revision 0x%02x detected!\n", d3);
            return 1;
        }    
    }    
     
    hdmi_dbg(&client->dev, "anx7150 not detected");
    return 0;
}

int ANX7150_hw_initial(struct i2c_client *client)
{
	int rc = 0;
    char c = 0;
	
	anx7150_in_pix_rpt_bkp = 0xff;
	anx7150_tx_pix_rpt_bkp = 0xff;
	ANX7150_hdcp_init_done = 0;
	ANX7150_hdcp_auth_fail_counter = 0;
	ANX7150_hdcp_auth_pass = 0;	
    //clear HDCP_HPD_RST
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);
	c |= (0x01);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);

   	mdelay(10);

	c &= (~0x01);
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);
	
    //Power on I2C
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL3_REG, &c);
	c |= (ANX7150_SYS_CTRL3_I2C_PWON);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL3_REG, &c);

	c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);
	c= 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SRST_REG, &c);

    //clear HDCP_HPD_RST
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);
	c &= (0xbf);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);

    //Power on Audio capture and Video capture module clock
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_PD_REG, &c);
	c |= (0x06);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_PD_REG, &c);

    //Enable auto set clock range for video PLL
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_CHIP_CTRL_REG, &c);
	c &= (0x00);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_CHIP_CTRL_REG, &c);

    //Set registers value of Blue Screen when HDCP authentication failed--RGB mode,green field
    c = 0x10;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN0_REG, &c);
	c = 0xeb;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN1_REG, &c);
	c = 0x10;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_BLUESCREEN2_REG, &c);

    //ANX7150_i2c_read_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
    //ANX7150_i2c_write_p0_reg(ANX7150_TMDS_CLKCH_CONFIG_REG, (c | 0x80));

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_PLL_CTRL0_REG, &c);
	c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_PLL_CTRL0_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_CHIP_DEBUG1_CTRL_REG, &c);
	c |= (0x08);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_CHIP_DEBUG1_CTRL_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_PLL_TX_AMP, &c);//jack wen
	c |= (0x01);

	rc = anx7150_i2c_write_p0_reg(client, ANX7150_PLL_TX_AMP, &c); //TMDS swing

	c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_PLL_CTRL1_REG, &c); //Added for PLL unlock issue in high temperature - Feiw
   //if (ANX7150_AUD_HW_INTERFACE == 0x02) //jack wen, spdif

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_I2S_CTRL_REG, &c);//jack wen, for spdif input from SD0.
	c &= (0xef);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_I2S_CTRL_REG, &c);

	c = 0xc7;
	rc = anx7150_i2c_write_p0_reg(client, 0xE1, &c);

    //ANX7150_i2c_read_p0_reg(ANX7150_SYS_CTRL1_REG, &c);
    c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);//power down HDCP, 090630

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL3_REG, &c);//jack wen, for spdif input from SD0.
	c &= (0xef);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL3_REG, &c);//power down all, 090630

	// disable hdcp added by zhy 20110413
	c = 0x00;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c); //simon
    if(rc < 0)
		dev_err(&client->dev, "%s>> i2c transfer err\n", __func__);

	// enabl INT pin output
#ifdef CONFIG_ANX7150_IRQ_USE_CHIP
	c = 0x04;//HOTPLUG_CHG
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR1_MASK_REG, &c);
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR_CTRL_REG, &c);
//	c = ANX7150_INTR_CTRL_POL | ANX7150_INTR_CTRL_TYPE | 0x08;
	c |= 0x08;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR_CTRL_REG, &c);
#endif
	
	return rc;
}

/*
	Detect HDMI hotplug.
 */
int ANX7150_get_hpd(struct i2c_client *client, int *hpd_state)
{
	int rc = 0;
	char c;
	
	if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL3_REG, &c)) < 0)
		return rc;
	if(c & ANX7150_SYS_CTRL3_PWON_ALL)
	{
		// ANX7150 power on
		if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_STATE_REG, &c)) < 0)
			return rc;
		*hpd_state = (c & ANX7150_SYS_STATE_HP)? 1:0;
	}
	else
	{
		// ANX7150 power down
		if((rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR_STATE_REG, &c)) < 0)
			return rc;
		*hpd_state = (c)? 1:0;
	} 
	
	return rc;
}

/*
	Dectect hdmi sink active.
 */
int ANX7150_get_sink_state(struct i2c_client *client, int *sink_status)
{
	int rc = 0;
	char c;

//	hdmi_dbg(&client->dev, "enter\n");
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_STATE_REG, &c);
	if(!rc)
		*sink_status = (c & ANX7150_SYS_STATE_RSV_DET) ? HDMI_RECEIVER_ACTIVE : HDMI_RECEIVER_INACTIVE;
	else
		*sink_status = HDMI_RECEIVER_INACTIVE;
	return rc;
}

/*
	HDMI plug
 */
int ANX7150_hw_plug(struct i2c_client *client)
{
	int rc = 0;
	char c;

	dev_info(&client->dev, "anx7150 plug\n");

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL3_REG, &c);
	c |= (0x01);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL3_REG, &c);//power up all, 090630

    //disable audio & video & hdcp & TMDS and init    begin
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
	c &= (~ANX7150_HDMI_AUDCTRL1_IN_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CTRL_REG, &c);
	c &= (~ANX7150_VID_CTRL_IN_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CTRL_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
	c &= (~ANX7150_TMDS_CLKCH_MUTE);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
	c &= (~ANX7150_HDCP_CTRL0_HW_AUTHEN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);

    //disable video & audio & hdcp & TMDS and init    end
    
    //Power on chip and select DVI mode
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);
	c |= (0x05);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);//  cwz change 0x01 -> 0x05

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);
	c &= (0xfd);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);

    //D("ANX7150 is set to DVI mode\n");
    rc = anx7150_rst_ddcchannel(client);
    //Initial Interrupt
    // disable video/audio CLK,Format change and before config video. 060713 xy

	c = 0x04;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR1_MASK_REG, &c);

	c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR2_MASK_REG, &c);
	
#ifdef CONFIG_ANX7150_IRQ_USE_CHIP
	// Enable Rx Sense Interrupt
	c = 0x02;
#else
	c = 0x00;
#endif
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR3_MASK_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR1_STATUS_REG, &c);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR1_STATUS_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR2_STATUS_REG, &c);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR2_STATUS_REG, &c);

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR3_STATUS_REG, &c);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR3_STATUS_REG, &c);

#ifndef CONFIG_ANX7150_IRQ_USE_CHIP
	c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR_CTRL_REG, &c);
#endif
	return rc;
}

int ANX7150_hw_read_edid(struct i2c_client *client, int block, unsigned char *buff)
{
	int rc = 0;
		
	if(client == NULL || buff == NULL)
		return HDMI_ERROR_FALSE;
	
    rc = anx7150_rst_ddcchannel(client);
//	msleep(5);

    rc =anx7150_initddc_read(client, 0xa0, block/2, 0x80 * (block%2), 0x80, 0x00);
//	msleep(10);
    rc = anx7150_ddc_mass_read(client, 128, buff);

//	int i;
//	for(i = 0; i < 128; i++)
//	{
//		printk("0x%02x, ",buff[i]);
//		if((i+1) % 16 == 0)
//			printk("\n");
//	}
	return rc;
}

/*	Video Config
 	@param	client					i2c client
	@param	anx7150_video_timing_id	input/output video timing id
	@param	output_color			output color mode:ANX7150_RGB, ANX7150_YCbCr422, ANX7150_YCbCr444
	@param	input_color				input color mode:ANX7150_RGB, ANX7150_YCbCr422, ANX7150_YCbCr444
	@param	anx7150_in_pix_rpt		input pixel clock repeat times
	@param	anx7150_tx_pix_rpt		output pixel clock repeat times
	@param	is_hdmi					output type: 1 hdmi; 0 dvi.
	@param	anx7150_bus_mode		input data bus mode
 */ 
int ANX7150_hw_config_video(struct i2c_client *client, int anx7150_video_timing_id, 
						int input_color, int output_color, 
						int anx7150_in_pix_rpt, int anx7150_tx_pix_rpt,
						int is_hdmi ,int anx7150_bus_mode)
{
	int rc = 0;
    char c, cspace_y2r = 0, y2r_sel, range_y2r, up_sample = 0;
    
	if(input_color != output_color)
	{
		if(output_color == ANX7150_RGB)	
			cspace_y2r = 1;
		else if(output_color == ANX7150_YCbCr444 && input_color == ANX7150_YCbCr422)
			up_sample = 1;
		else
		{
			hdmi_dbg(&client->dev, "Don't surport such color space conversion!\n");
			return HDMI_ERROR_FALSE;
		}
	}
	else
		cspace_y2r = 0;
	
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
	c &= ~(ANX7150_TMDS_CLKCH_MUTE);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
	
	// reset video mode
    c = 0x00;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
	// disable capture input video
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CTRL_REG, &c);
	c &= (~ANX7150_VID_CTRL_u8CTRL_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CTRL_REG, &c);
	// dectect input video clock 
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_STATE_REG, &c);
    if (!(c & 0x02))
    {
        hdmi_dbg(&client->dev, "No clock detected !\n");
        //ANX7150_i2c_write_p0_reg(ANX7150_SYS_CTRL2_REG, 0x02);
        return HDMI_ERROR_FALSE;
    }
    
    rc = anx7150_clean_hdcp(client);
    
    //Config the embeded blue screen format according to output video format.
    rc = anx7150_blue_screen_format_config(client, output_color);
    
    switch(anx7150_bus_mode)
    {
    	case ANX7150_RGB_YCrCb444_SepSync:
    		c = 0;
    		rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
    		// Set ESYNC type
			rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL4_REG, &c);
			c &= (0xfc);
			rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL4_REG, &c);
    		break;
    	default:
    		hdmi_dbg(&client->dev, "Not support such input bus mode!\n");
    		return HDMI_ERROR_FALSE;
    }
    /* Config input video mode. Restricted to ANX7150_RGB_YCrCb444_SepSync mode*/
/*	
	//Disable DE signal generation.
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	c &= (~ANX7150_VID_CAPCTRL0_DEGEN_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);

	//Disable embended sync signal.
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	c &= (~ANX7150_VID_CAPCTRL0_EMSYNC_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	
	// Diable demux YCmux format
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	c &= (~ANX7150_VID_CAPCTRL0_DEMUX_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);

	// Set video input bus mode.
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	c &= (~ANX7150_VID_CAPCTRL0_DV_BUSMODE);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	
	// Set IDCK polarity?
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	c &= (~ANX7150_VID_CAPCTRL0_DDR_EDGE);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CAPCTRL0_REG, &c);
	
	// Set ESYNC type
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL4_REG, &c);
	c &= (0xfc);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL4_REG, &c);
*/
 	/* Config video output. */
 	
 	//color space issue
    if(cspace_y2r)
    {
	    switch (anx7150_video_timing_id)
	    {
	        case ANX7150_V1280x720p_50Hz:
	        case ANX7150_V1280x720p_60Hz:
	        case ANX7150_V1920x1080i_60Hz:
	        case ANX7150_V1920x1080i_50Hz:
	        case ANX7150_V1920x1080p_60Hz:
	        case ANX7150_V1920x1080p_50Hz:
	            y2r_sel = ANX7150_CSC_BT709;
	            break;
	        default:
	            y2r_sel = ANX7150_CSC_BT601;
	            break;
	    }
	    //rang[0~255]/[16~235] select
	    if (anx7150_video_timing_id == ANX7150_V640x480p_60Hz)
	        range_y2r = 1;//range[0~255]
	    else
	        range_y2r = 0;//range[16~235]
    }
 	// set color space conversion
	rc = anx7150_color_space_conversion_config(client, cspace_y2r, y2r_sel, range_y2r);
	// set upsample mode
    if (up_sample)
    {
        hdmi_dbg(&client->dev, "UP_SAMPLE!\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
		c |= (ANX7150_VID_MODE_UPSAMPLE);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
    }
    else
    {
    	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
		c &= (~ANX7150_VID_MODE_UPSAMPLE);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
    }

	// set input pixel repeat times
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_MODE_REG, &c);
	c = ((c & 0xfc) | anx7150_in_pix_rpt);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_MODE_REG, &c);
    
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CTRL_REG, &c);
	c = ((c & 0xfc) | anx7150_tx_pix_rpt);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CTRL_REG, &c);
    
    // If input/output pix clock repeat time changed, reset misc
    if ((anx7150_in_pix_rpt != anx7150_in_pix_rpt_bkp)
            ||(anx7150_tx_pix_rpt != anx7150_tx_pix_rpt_bkp) )
    {
    	c = 0x02;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);
		c = 0x00;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL2_REG, &c);
        hdmi_dbg(&client->dev, "MISC_Reset!\n");
        anx7150_in_pix_rpt_bkp = anx7150_in_pix_rpt;
        anx7150_tx_pix_rpt_bkp = anx7150_tx_pix_rpt;
    }
    
    //enable video input
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_CTRL_REG, &c);
	c |= (ANX7150_VID_CTRL_IN_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_VID_CTRL_REG, &c);
    /******************** config video output end ******************/
    
    msleep(60);
	
	// check input video data stable
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_VID_STATUS_REG, &c);
    if (!(c & ANX7150_VID_STATUS_VID_STABLE))
    {
        hdmi_dbg(&client->dev,"Video not stable!\n");
        return HDMI_ERROR_FALSE;
    }
    
    //Enable video CLK,Format change after config video.
    // ANX7150_i2c_read_p0_reg(ANX7150_INTR1_MASK_REG, &c);
    // ANX7150_i2c_write_p0_reg(ANX7150_INTR1_MASK_REG, c |0x01);//3
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR2_MASK_REG, &c);
	c |= (0x48);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR2_MASK_REG, &c);
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR3_MASK_REG, &c);
	c |= (0x40);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR3_MASK_REG, &c);
		
	if (is_hdmi)
    {
    	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);
		c |= (0x02);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);
        hdmi_dbg(&client->dev,"ANX7150 is set to HDMI mode\n");
        
        anx7150_set_avmute(client);//wen
    }
    else
    	hdmi_dbg(&client->dev,"ANX7150 is set to DVI mode\n");
    	
    //reset TMDS link to align 4 channels  xy 061120
    hdmi_dbg(&client->dev,"reset TMDS link to align 4 channels\n");

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SRST_REG, &c);
	c |= (ANX7150_TX_RST);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SRST_REG, &c);
	c &= (~ANX7150_TX_RST);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SRST_REG, &c);
	
    //Enable TMDS clock output // just enable u87, and let the other u8s along to avoid overwriting.
//    hdmi_dbg(&client->dev,"Enable TMDS clock output\n");
//	rc = anx7150_i2c_read_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
//	c |= (ANX7150_TMDS_CLKCH_MUTE);
//	rc = anx7150_i2c_write_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
	
//	if(ANX7150_HDCP_enable)
//    	msleep(400);  //400ms only for HDCP CTS

    hdmi_dbg(&client->dev,"video config done.\n");
    
   	#ifdef CONFIG_HDMI_DEBUG
	anx7150_show_video_info(client);
	#endif
    return rc;
}

int ANX7150_hw_config_avi(struct i2c_client *client, infoframe_struct *p)
{
	int rc = 0, i;
	char c;
	
    // Disable repeater
    rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c);
    c &= ~ANX7150_INFO_PKTCTRL1_AVI_RPT;
	rc = anx7150_i2c_write_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c);

    // Enable wait:go
    i = 5;
    while(i--)
    {
	    rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c);
	    if (c & ANX7150_INFO_PKTCTRL1_AVI_EN)
	    {
	        hdmi_dbg(&client->dev, "wait disable, config avi infoframe packet.\n");
	        msleep(10);
	    }
	    else
	    	break;
	}

	if (c & ANX7150_INFO_PKTCTRL1_AVI_EN && i == 0)
	{
		hdmi_dbg(&client->dev, "wait avi repeater disable error\n");
		return HDMI_ERROR_FALSE;
	}
    p->pb_u8[0] = anx7150_checksum(p);

    // write infoframe to according regs
    c = p->type;
    rc = anx7150_i2c_write_p1_reg(client, 0, &c);
	c = p->version;
    rc = anx7150_i2c_write_p1_reg(client, 1, &c);
	c = p->length;
    rc = anx7150_i2c_write_p1_reg(client, 2, &c);

    for (i=0; i <= p->length; i++)
    {
    	c = p->pb_u8[i];
    	rc = anx7150_i2c_write_p1_reg(client, 3+i, &c);
    }
    
    // Enable and repeater
    rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c);
    c |= 0x30;
	rc = anx7150_i2c_write_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c);

    // complete aux packet
    hdmi_dbg(&client->dev, "config avi infoframe packet done.\n");
    return rc;
}

static int anx7150_config_spdif(struct i2c_client *client, 
								unsigned char channel_num,
								unsigned char fs,
								unsigned char word_length)
{
	int rc = 0;
    u8 exe_result = 0x00;
    char c = 0x00;
 //   u8 c2 = 0x00;
 //   u8 freq_mclk = 0x00;

    hdmi_dbg(&client->dev, "ANX7150: config SPDIF audio.\n");

	if(channel_num > 2)
		return -1;
		
    //Select MCLK
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
	c |= (ANX7150_HDMI_AUDCTRL1_CLK_SEL);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);

    //D("ANX7150: enable SPDIF audio.\n");
    //Enable SPDIF
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
	c |= (ANX7150_HDMI_AUDCTRL1_SPDIFIN_EN);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);

    //adjust MCLK phase in interrupt routine

    // adjust FS_FREQ   //FS_FREQ
	c = (fs << 4) | (word_length < 1);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_I2SCH_STATUS4_REG, &c);
	
    // spdif is stable
    hdmi_dbg(&client->dev, "config SPDIF audio done");
    exe_result = ANX7150_spdif_input;

    // open corresponding interrupt
    rc = anx7150_i2c_read_p0_reg(client, ANX7150_INTR1_MASK_REG, &c);
	c |= (0x32);
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_INTR1_MASK_REG, &c);
    //ANX7150_i2c_read_p0_reg(ANX7150_INTR3_MASK_REG, &c);
    //ANX7150_i2c_write_p0_reg(ANX7150_INTR3_MASK_REG, (c | 0xa1) );
    return exe_result;
}
int ANX7150_hw_config_audio(struct i2c_client *client, 
							unsigned char input_interface_type, 
							unsigned char channel_num,
							unsigned char clock_edge,
							unsigned char fs,
							unsigned char mclk_freq,
							unsigned char down_sample,
							unsigned char word_length)
{
	int rc = 0;
	char c = 0x00;
    u8 audio_layout = 0x00;
    u32 ACR_N = 0x0000;
    
    if ((input_interface_type & 0x07) == 0x00)
    {
        hdmi_dbg(&client->dev, "ANX7150 input no audio type.\n");
        return -1;
    }
    
    // Config HDMI Audio Control Register 0
    if(channel_num > 2)
    	audio_layout = 1;
    else
    	audio_layout = 0;
    c = (audio_layout << 7) | (down_sample << 5) | (clock_edge << 3) | mclk_freq;
    rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL0_REG, &c);
//    hdmi_dbg(&client->dev, "ANX7150_HDMI_AUDCTRL0_REG = 0x%.2x\n",(u32)c);
    
//    hdmi_dbg(&client->dev, "audio_type = 0x%.2x\n",(u32)input_interface_type);
    // confgi input interface
    if (input_interface_type & ANX7150_i2s_input)
    {
//    	hdmi_dbg(&client->dev, "Config I2s.\n");
        rc |= anx7150_config_i2s(client, channel_num, fs, word_length);
    }
    else
    {
        //disable I2S audio input
//        hdmi_dbg(&client->dev, "Disable I2S audio input.\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
        c &= 0xc3;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
    }
    /* After reset, spdif and super audio interface default is disabled.*/
    
    if (input_interface_type & ANX7150_spdif_input)
    {
        rc |= anx7150_config_spdif(client, channel_num, fs, word_length);
    }
    else
    {
        //disable SPDIF audio input
        hdmi_dbg(&client->dev, "Disable SPDIF audio input.\n");
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
        c &= ~ANX7150_HDMI_AUDCTRL1_SPDIFIN_EN;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
    }
#if 0
    if (input_interface_type & ANX7150_super_audio_input)
    {
//        exe_result |= anx7150_config_super_audio(client);
    }
    else
    {
        //disable super audio output
        hdmi_dbg(&client->dev, "ANX7150: disable super audio output.\n");
		c = 0x00;
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_ONEu8_AUD_CTRL_REG, &c);
}
#endif    
	if(input_interface_type)
    {
        //Initial N value
        // set default value to N
        ACR_N = ANX7150_N_48k;
        switch (fs)
        {
            case(0x00)://44.1k
                ACR_N = ANX7150_N_44k;
                break;
            case(0x02)://48k
                ACR_N = ANX7150_N_48k;
                break;
            case(0x03)://32k
                ACR_N = ANX7150_N_32k;
                break;
            case(0x08)://88k
                ACR_N = ANX7150_N_88k;
                break;
            case(0x0a)://96k
                ACR_N = ANX7150_N_96k;
                break;
            case(0x0c)://176k
                ACR_N = ANX7150_N_176k;
                break;
            case(0x0e)://192k
                ACR_N = ANX7150_N_192k;
                break;
            default:
                dev_err(&client->dev, "note wrong fs.\n");
                break;
        }
        // write N(ACR) to corresponding regs
        c = ACR_N;
		rc = anx7150_i2c_write_p1_reg(client, ANX7150_ACR_N1_SW_REG, &c);
        c = ACR_N>>8;
		rc = anx7150_i2c_write_p1_reg(client, ANX7150_ACR_N2_SW_REG, &c);
		c = 0x00;
		rc = anx7150_i2c_write_p1_reg(client, ANX7150_ACR_N3_SW_REG, &c);
	
//        // set the relation of MCLK and Fs  xy 070117
//        rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL0_REG, &c);
//		c = (c & 0xf8) | mclk_freq;
//		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL0_REG, &c);
//       
//       	hdmi_dbg(&client->dev, "Audio MCLK input mode is: %.2x\n",(u32)mclk_freq);

        //Enable control of ACR
        rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c);
		c |= (ANX7150_INFO_PKTCTRL1_ACR_EN);
		rc = anx7150_i2c_write_p1_reg(client, ANX7150_INFO_PKTCTRL1_REG, &c);

        //audio enable:
        rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
		c |= (ANX7150_HDMI_AUDCTRL1_IN_EN);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDMI_AUDCTRL1_REG, &c);
    }
	hdmi_dbg(&client->dev, "Configure audio done.\n");
	#ifdef CONFIG_HDMI_DEBUG
	anx7150_show_audio_info(client);
	#endif
    return rc;
}

int ANX7150_hw_config_aai(struct i2c_client *client, infoframe_struct *p)
{
	int rc = 0, i;
	char c;
	
    // Disable repeater
    rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL2_REG, &c);
    c &= ~ANX7150_INFO_PKTCTRL2_AIF_RPT;
	rc = anx7150_i2c_write_p1_reg(client, ANX7150_INFO_PKTCTRL2_REG, &c);

    // Enable wait:go
    i = 5;
    while(i--)
    {
	    rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL2_REG, &c);
	    if (c & ANX7150_INFO_PKTCTRL2_AIF_EN)
	    {
	        hdmi_dbg(&client->dev, "wait disable, config aai infoframe packet.\n");
	        msleep(10);
	    }
	    else
	    	break;
	}
	if( (i ==5) && (c & ANX7150_INFO_PKTCTRL2_AIF_EN) )
	{
		hdmi_dbg(&client->dev, "wait aai repeater disable error.\n");
		return HDMI_ERROR_FALSE; //jack wen
	}
	
    p->pb_u8[0] = anx7150_checksum(p);

    // write infoframe to according regs
    c = p->type;
    rc = anx7150_i2c_write_p1_reg(client, 0x20, &c);
	c = p->version;
    rc = anx7150_i2c_write_p1_reg(client, 0x21, &c);
	c = p->length;
    rc = anx7150_i2c_write_p1_reg(client, 0x22, &c);

    for (i=0; i <= p->length; i++)
    {
    	c = p->pb_u8[i];
    	rc = anx7150_i2c_write_p1_reg(client, 0x23+i, &c);
    }
    
	// Enable and repeater
    rc = anx7150_i2c_read_p1_reg(client, ANX7150_INFO_PKTCTRL2_REG, &c);
    c |= 0x03;
	rc = anx7150_i2c_write_p1_reg(client, ANX7150_INFO_PKTCTRL2_REG, &c);

    // complete aux packet
    hdmi_dbg(&client->dev, "config aai infoframe packet done.\n");
    return rc;
}

int ANX7150_hw_enalbe_output(struct i2c_client *client, int enable)
{
	int rc = 0;
    char c;

	if(enable)
	{
		anx7150_clear_avmute(client);
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
		c |= (ANX7150_TMDS_CLKCH_MUTE);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);	
		hdmi_dbg(&client->dev,"enable video&audio output.\n");
	}
	else
	{
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
		c &= ~(ANX7150_TMDS_CLKCH_MUTE);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_TMDS_CLKCH_CONFIG_REG, &c);
		hdmi_dbg(&client->dev,"Disable video&audio output.\n");
	}
	return rc;
}

int ANX7150_hw_config_hdcp(struct i2c_client *client, int is_hdmi, int enable)
{
	int rc;
	u8 c;
	
	if(!enable)
	{
		rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
        c &= ~ANX7150_HDCP_CTRL0_HW_AUTHEN;
		rc |= anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
		rc |= anx7150_hdcp_encryption(client, 0);
		
		return HDMI_ERROR_SUCESS;
	}

	mdelay(10);//let unencrypted video play a while, required by HDCP CTS. SY//wen HDCP CTS
//    anx7150_set_avmute(client);//before auth, set_avmute//wen
    mdelay(10);//wen HDCP CTS
    ANX7150_hdcp_auth_pass = 0;

	rc = anx7150_i2c_read_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);
    c |= ANX7150_SYS_CTRL1_HDCPMODE;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_SYS_CTRL1_REG, &c);
    if (is_hdmi)
        rc = anx7150_hardware_hdcp_auth_init(client);
    else
    {   //DVI, disable 1.1 feature and enable HDCP two special point check
    	rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c);
    	c = ((c & (~ANX7150_HDCP_CTRL1_HDCP11_EN)) | ANX7150_LINK_CHK_12_EN);
		rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL1_REG, &c);
    }
    
    // Enable hardware HDCP authentication.
    hdmi_dbg(&client->dev, "enable hw hdcp\n");
    anx7150_rst_ddcchannel(client);
	rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
    c |= ANX7150_HDCP_CTRL0_HW_AUTHEN;
	rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);

	return HDMI_ERROR_SUCESS;
}

void ANX7150_hw_process_interrupt(struct i2c_client *client)
{
	struct anx7150_interrupt_s interrupt_staus;
	char c;
	int rc;
	
	rc = anx7150_get_interrupt_status(client, &interrupt_staus);
	if(rc < 0)
		return;
	
//	if(interrupt_staus.video_format_change){
//		if(state > SYSTEM_CONFIG){
//			rc = anx7150_video_format_change(anx->client);
//			state = CONFIG_VIDEO;
//		}
//	}

	if(interrupt_staus.bksv_ready) {
		if (!anx7150_bksv_srm(client))
        {
        	//BKSV not correct, so do not send video frame.
            anx7150_blue_screen(client, 1);
//            anx7150_clear_avmute(client);
//            Bksv_valid=0;
        }
        else //SY.
        {
            // Correct BKSV, set BKSV SRM Check indicator status valid.
//            Bksv_valid=1;
			rc = anx7150_i2c_read_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
        	c |= 0x03;
			rc = anx7150_i2c_write_p0_reg(client, ANX7150_HDCP_CTRL0_REG, &c);
        }
	}
		
	
	if(interrupt_staus.auth_done && !ANX7150_hdcp_auth_pass) {
		rc = anx7150_auth_done(client);
//		state = CONFIG_AUDIO;
	}

	if(interrupt_staus.auth_state_change){
		rc = anx7150_auth_change(client);
//		if(state == PLAY_BACK){
//			state = HDCP_AUTHENTICATION;
//		}
	}

//	if(ANX7150_GET_RECIVER_TYPE() == 1){
//		if(interrupt_staus.audio_clk_change){
//			if(state > CONFIG_VIDEO){
//				rc = anx7150_audio_clk_change(anx->client);
//				state = SYSTEM_CONFIG;
//			}
//		}
//		
//		if(interrupt_staus.audio_FIFO_overrun){
//			if(state > CONFIG_VIDEO){
//				rc = anx7150_afifo_overrun(anx->client);
//				state = CONFIG_AUDIO;
//			}
//		}
//
//		rc = anx7150_spdif_error(anx->client, state, interrupt_staus.SPDIF_bi_phase_error, interrupt_staus.SPDIF_error);
//	}

//	if(interrupt_staus.pll_lock_change){
//		if(state > SYSTEM_CONFIG){
//			rc = anx7150_plllock(anx->client);
//			state = SYSTEM_CONFIG;
//		}
//	}
//
//	if(interrupt_staus.rx_sense_change){
//		anx7150_rx_sense_change(anx->client, state);
//		if(state > WAIT_RX_SENSE) 
//			state = WAIT_RX_SENSE;
//	}
}
