#include <linux/kernel.h>
#include <linux/delay.h>

#include <linux/hdmi-itv.h>
#include "hdmi_local.h"

/**********************
 * Global Data
 **********************/
//#define HDMI_DEBUG_TIME;
#ifdef HDMI_DEBUG_TIME
#include <linux/jiffies.h>
static unsigned long hdmi_time_start, hdmi_time_end;
#endif

#define HDMI_MAX_EDID_TRY_TIMES	1
#define HDMI_MAX_TRY_TIMES	1
#define HDMI_TASK_INTERVAL	hdmi_task_interval()

static int g_state = HDMI_UNKNOWN;
static int g_lcdcstatus = 0;
/**********************
 * Funtions
 *********************/

static void hdmi_sys_show_state(struct hdmi *hdmi, int state)
{
	switch(state)
	{
		case HDMI_INITIAL:
			dev_printk(KERN_INFO, hdmi->dev, "HDMI_INITIAL\n");
			break;
		case WAIT_HOTPLUG:
			dev_printk(KERN_INFO, hdmi->dev, "WAIT_HOTPLUG\n");
			break;
		case READ_PARSE_EDID:
			dev_printk(KERN_INFO, hdmi->dev, "READ_PARSE_EDID\n");
			break;
		case WAIT_RX_SENSE:
			dev_printk(KERN_INFO, hdmi->dev, "WAIT_RX_SENSE\n");
			break;
		case WAIT_HDMI_ENABLE:
			dev_printk(KERN_INFO, hdmi->dev, "WAIT_HDMI_ENABLE\n");
			break;
		case SYSTEM_CONFIG:
			dev_printk(KERN_INFO, hdmi->dev, "SYSTEM_CONFIG\n");
			break;
		case CONFIG_VIDEO:
			dev_printk(KERN_INFO, hdmi->dev, "CONFIG_VIDEO\n");
			break;
		case CONFIG_AUDIO:
			dev_printk(KERN_INFO, hdmi->dev, "CONFIG_AUDIO\n");
			break;
		case CONFIG_PACKETS:
			dev_printk(KERN_INFO, hdmi->dev, "CONFIG_PACKETS\n");
			break;
		case HDCP_AUTHENTICATION:
			dev_printk(KERN_INFO, hdmi->dev, "HDCP_AUTHENTICATION\n");
			break;
		case PLAY_BACK:
			dev_printk(KERN_INFO, hdmi->dev, "PLAY_BACK\n");
			break;
		default:
			dev_printk(KERN_INFO, hdmi->dev, "Unkown State\n");
			break;
	}
}

static void hdmi_sys_send_uevent(struct hdmi *hdmi, int uevent)
{
	char *envp[3];
	
	envp[0] = "INTERFACE=HDMI";
	envp[1] = kmalloc(32, GFP_KERNEL);
	if(envp[1] == NULL)	return;
	sprintf(envp[1], "SCREEN=%d", hdmi->ddev->property);
	envp[2] = NULL;
	kobject_uevent_env(&hdmi->ddev->dev->kobj, uevent, envp);
	kfree(envp[1]);
}

static int hdmi_sys_init(struct hdmi *hdmi)
{
	int rc;
	
	rc = hdmi->ops->init(hdmi);
	if(!rc)
	{
		hdmi->config_real.display_on = HDMI_DISPLAY_OFF;
		hdmi->config_real.hdcp_on = HDMI_HDCP_CONFIG;
		hdmi->config_real.resolution = HDMI_UNKOWN;
		hdmi->config_real.audio.channel = HDMI_UNKOWN;
		hdmi->config_real.audio.rate = HDMI_UNKOWN;
		hdmi->config_real.audio.word_length = HDMI_UNKOWN;
	}
	return rc;
}

#define EDID_BLOCK_LENGTH	128
static int hdmi_sys_parse_edid(struct hdmi* hdmi)
{
	struct hdmi_edid *pedid;
	unsigned char *buff = NULL;
	int rc = HDMI_ERROR_SUCESS, extendblock = 0, i;
	
	if(hdmi == NULL)
		return HDMI_ERROR_FALSE;
	fb_destroy_modelist(&hdmi->edid.modelist);
	pedid = &(hdmi->edid);
	memset(pedid, 0, sizeof(struct hdmi_edid));
	INIT_LIST_HEAD(&pedid->modelist);
	
	buff = kmalloc(EDID_BLOCK_LENGTH, GFP_KERNEL);
	if(buff == NULL)
	{		
		hdmi_dbg(hdmi->dev, "[%s] can not allocate memory for edid buff.\n", __FUNCTION__);
		return -1;
	}
	// Read base block edid.
	memset(buff, 0 , EDID_BLOCK_LENGTH);
	rc = hdmi->ops->read_edid(hdmi, 0, buff);
	if(rc)
	{
		dev_err(hdmi->dev, "[HDMI] read edid base block error\n");
		goto out;
	}
	rc = HDMI_EDID_parse_base(buff, &extendblock, pedid);
	if(rc)
	{
		dev_err(hdmi->dev, "[HDMI] parse edid base block error\n");
		goto out;
	}
	for(i = 1; i < extendblock + 1; i++)
	{
		memset(buff, 0 , EDID_BLOCK_LENGTH);
		rc = hdmi->ops->read_edid(hdmi, i, buff);
		if(rc)
		{
			printk("[HDMI] read edid block %d error\n", i);	
			goto out;
		}
		rc = HDMI_EDID_parse_extensions(buff, pedid);
		if(rc)
		{
			dev_err(hdmi->dev, "[HDMI] parse edid block %d error\n", i);
			continue;
		}
	}
out:
	if(buff)
		kfree(buff);
	rc = ext_hdmi_ouputmode_select(hdmi, rc);
	return rc;
}


static void hdmi_sys_unplug(struct hdmi* hdmi)
{
	if(hdmi->ops->remove)
		hdmi->ops->remove(hdmi);
	fb_destroy_modelist(&hdmi->edid.modelist);
	if(hdmi->edid.audio)
		kfree(hdmi->edid.audio);
	if(hdmi->edid.specs)
	{
		if(hdmi->edid.specs->modedb)
			kfree(hdmi->edid.specs->modedb);
		kfree(hdmi->edid.specs);
	}
	memset(&hdmi->edid, 0, sizeof(struct hdmi_edid));
	ext_hdmi_init_modelist(hdmi);
	hdmi->hpd_status = HDMI_RECEIVER_REMOVE;
//	hdmi->config_set.display_on = HDMI_DISPLAY_OFF;	
//	hdmi->auto_config = HDMI_AUTO_CONFIG;
}

static int hdmi_sys_change(struct hdmi* hdmi)
{
	int change, state = g_state;
	
	change = hdmi->param_config;
	if(change != HDMI_CONFIG_NONE)	
	{		
		hdmi->param_config = HDMI_CONFIG_NONE;
		switch(change)
		{	
			case HDMI_CONFIG_ENABLE:
				/* System suspended */
				if(!hdmi->enable)
				{
					if(hdmi->hpd_status)
						hdmi_sys_unplug(hdmi);
					state = HDMI_UNKNOWN;
//					if(hdmi->auto_switch)
//						rk_display_device_enable_other(hdmi->ddev);
				}
				if(hdmi->wait == 1) {
					complete(&hdmi->complete);
					hdmi->wait = 0;	
				}
				break;	
			case HDMI_CONFIG_COLOR:
				if(state > CONFIG_VIDEO)
					state = CONFIG_VIDEO;	
				break;
			case HDMI_CONFIG_HDCP:
//				if(state > HDCP_AUTHENTICATION)
//					state = HDCP_AUTHENTICATION;	
				break;
			case HDMI_CONFIG_DISPLAY:
				break;
			case HDMI_CONFIG_AUDIO:
				if(state > CONFIG_AUDIO)
					state = CONFIG_AUDIO;
				break;
			case HDMI_CONFIG_VIDEO:
			default:
				if(state > SYSTEM_CONFIG)
					state = SYSTEM_CONFIG;
				else
					if(hdmi->wait == 1) {
					complete(&hdmi->complete);
					hdmi->wait = 0;	
				}					
				break;
		}
		#ifdef HDMI_DEBUG_TIME
		hdmi_time_start = jiffies;
		#endif
	}
	
	return state;
}

static int hdmi_sys_detect_status(struct hdmi *hdmi, int cur_status)
{
	int rc, hpd, state = cur_status;
	
	rc = hdmi->ops->detect_hpd(hdmi, &hpd);
//	hdmi_dbg(hdmi->dev, "hpd is %d\n", hpd);
	if(!rc && (hpd != hdmi->hpd_status))
	{
		if(hpd == HDMI_RECEIVER_REMOVE)
		{
			hdmi_dbg(hdmi->dev, "HDMI_UNPLUG\n");
			hdmi_sys_unplug(hdmi);
			if(hdmi->auto_switch)
			    rk_display_device_enable_other(hdmi->ddev);
			state = HDMI_INITIAL;
			if(hdmi->wait == 1) {	
				complete(&hdmi->complete);
				hdmi->wait = 0;
			}
			hdmi_sys_send_uevent(hdmi, KOBJ_REMOVE);
		}
		else if(hdmi->hpd_status == HDMI_RECEIVER_REMOVE)
			hdmi->hpd_status = HDMI_RECEIVER_INACTIVE;
	}
	if(state > SYSTEM_CONFIG && hdmi->ops->detect_sink)
	{
		rc = hdmi->ops->detect_sink(hdmi, &hpd);
//		hdmi_dbg(hdmi->dev, "sink is %d\n", hpd);
		if(hpd == HDMI_RECEIVER_INACTIVE)
		{
			hdmi->hpd_status = hpd;	
			hdmi_dbg(hdmi->dev, "HDMI_UNLINK\n");
			if(state > WAIT_RX_SENSE)
			{	
				if(hdmi->auto_switch)
				    rk_display_device_enable_other(hdmi->ddev);
	
				if(hdmi->wait == 1) {	
					complete(&hdmi->complete);
					hdmi->wait = 0;
				}
				if(state == PLAY_BACK)
					hdmi_sys_send_uevent(hdmi, KOBJ_REMOVE);
				state = WAIT_RX_SENSE;
			}
		}		
	}
	return state;
}

int HDMI_task(struct hdmi *hdmi)
{
	int rc, state, state_last, hpd, trytimes;
//	state = g_state;
//	hdmi_sys_show_state(hdmi, state);

	/* Process parametre changed */
	state = hdmi_sys_change(hdmi);
	
	/* Initialize hdmi chips when system start.*/
	if(state == HDMI_UNKNOWN)
	{
		rc = hdmi_sys_init(hdmi);
		state = WAIT_HOTPLUG;
	}
	
	if(!hdmi->enable)
		goto exit;

	/* Detect hdmi status */
	state = hdmi_sys_detect_status(hdmi, state);
	/* Process HDMI status machine */
	rc = 0;
	trytimes = 0;
	do
	{	
		state_last = state;

		switch(state)
		{
			case HDMI_INITIAL:
				rc = hdmi_sys_init(hdmi);
				if(rc == HDMI_ERROR_SUCESS)
				{
					hdmi->rate = 0;	
					state = WAIT_HOTPLUG;
				}
				else
					hdmi->rate = HDMI_TASK_INTERVAL;
				break;
			case WAIT_HOTPLUG:
				hdmi->rate = HDMI_TASK_INTERVAL;
				if(hdmi->hpd_status > HDMI_RECEIVER_REMOVE)
				{
					#ifdef HDMI_DEBUG_TIME
					hdmi_time_start = jiffies;
					#endif
							
					if(hdmi->ops->insert)
					{
						rc = hdmi->ops->insert(hdmi);
					}
					
					if(rc == HDMI_ERROR_SUCESS)
					{	
						state = READ_PARSE_EDID;
					}
					else
						hdmi->rate = HDMI_TASK_INTERVAL;	
				}
				break;
			case READ_PARSE_EDID:
				rc = hdmi_sys_parse_edid(hdmi);
				#ifdef HDMI_DEBUG_TIME
					hdmi_time_end = jiffies;
					if(hdmi_time_end > hdmi_time_start)
						hdmi_time_end -= hdmi_time_start;
					else
						hdmi_time_end += 0xFFFFFFFF - hdmi_time_start;
					dev_printk(KERN_INFO, hdmi->dev, "HDMI EDID parse cost %u ms\n", jiffies_to_msecs(hdmi_time_end));
				#endif
				if(rc == HDMI_ERROR_SUCESS)
				{
					hdmi->rate = 0;	
					state = WAIT_RX_SENSE;
					hdmi_sys_send_uevent(hdmi, KOBJ_ADD);
				}
				else
					hdmi->rate = HDMI_TASK_INTERVAL;
				break;
			case WAIT_RX_SENSE:
				if(hdmi->ops->detect_sink)
				{
					rc = hdmi->ops->detect_sink(hdmi, &hpd);
					hdmi->hpd_status = hpd;
					if(hpd == HDMI_RECEIVER_ACTIVE)
					{
						//Only when HDMI receiver is actived,
						//HDMI communication is successful.		
						state = WAIT_HDMI_ENABLE;		
						hdmi->rate = 0;
					}
					else
					{
						rc = HDMI_ERROR_FALSE;	
						hdmi->rate = HDMI_TASK_INTERVAL;	
					}	
				}
				else
				{
					hdmi->hpd_status = HDMI_RECEIVER_ACTIVE;
					hdmi->rate = 0;
					state = WAIT_HDMI_ENABLE;
					if(hdmi->auto_switch)
						hdmi_enable(hdmi);
				}
				#ifdef HDMI_DEBUG_TIME
				hdmi_time_end = jiffies;
				if(hdmi_time_end > hdmi_time_start)
					hdmi_time_end -= hdmi_time_start;
				else
					hdmi_time_end += 0xFFFFFFFF - hdmi_time_start;
				dev_printk(KERN_INFO, hdmi->dev, "HDMI sink enable cost %u ms\n", jiffies_to_msecs(hdmi_time_end));
				#endif
			
				break;
			case WAIT_HDMI_ENABLE:
				if(hdmi->config_set.display_on == HDMI_DISPLAY_OFF)
					hdmi->rate = HDMI_TASK_INTERVAL;
				else
					state = SYSTEM_CONFIG;
				break;
			case SYSTEM_CONFIG:
				if(hdmi->auto_switch)
				{
		    		rk_display_device_disable_other(hdmi->ddev);
		    		g_lcdcstatus = 1;
				}
				if(hdmi->auto_config)	
					hdmi->config_set.resolution = ext_hdmi_find_best_mode(hdmi, 0);
				else
					hdmi->config_set.resolution = ext_hdmi_find_best_mode(hdmi, hdmi->config_set.resolution);
				rc = ext_hdmi_switch_fb(hdmi, 1);
				#ifdef HDMI_DEBUG_TIME
					hdmi_time_end = jiffies;
					if(hdmi_time_end > hdmi_time_start)
						hdmi_time_end -= hdmi_time_start;
					else
						hdmi_time_end += 0xFFFFFFFF - hdmi_time_start;
					dev_printk(KERN_INFO, hdmi->dev, "HDMI configure LCD cost %u ms\n", jiffies_to_msecs(hdmi_time_end));
				#endif
				if(rc == HDMI_ERROR_SUCESS)
				{
					state = CONFIG_VIDEO;
					hdmi->rate = 0;
				}
				else
					hdmi->rate = HDMI_TASK_INTERVAL;
				break;
			case CONFIG_VIDEO:
				{
					hdmi->config_real.display_on = HDMI_DISABLE;
					if(hdmi->edid.is_hdmi) {
						if(hdmi->edid.ycbcr444)
							hdmi->config_set.color = HDMI_VIDEO_YCbCr444;
						else if(hdmi->edid.ycbcr422)
							hdmi->config_set.color = HDMI_VIDEO_YCbCr422;
						else
							hdmi->config_set.color = HDMI_VIDEO_RGB;
					}
					else
						hdmi->config_set.color = HDMI_VIDEO_RGB;
					if(hdmi->config_set.color > HDMI_VIDEO_RGB && (!hdmi->support_r2y))
						hdmi->config_set.color = HDMI_VIDEO_RGB;
					rc = hdmi->ops->config_video(hdmi, hdmi->config_set.resolution, HDMI_VIDEO_RGB, /*HDMI_VIDEO_RGB*/hdmi->config_set.color);
					#ifdef HDMI_DEBUG_TIME
						hdmi_time_end = jiffies;
						if(hdmi_time_end > hdmi_time_start)
							hdmi_time_end -= hdmi_time_start;
						else
							hdmi_time_end += 0xFFFFFFFF - hdmi_time_start;
						dev_printk(KERN_INFO, hdmi->dev, "HDMI configure VIDEO cost %u ms\n", jiffies_to_msecs(hdmi_time_end));
					#endif
				}
				if(rc == HDMI_ERROR_SUCESS)
				{
					hdmi->rate = 0;	
					if(hdmi->edid.is_hdmi)
						state = CONFIG_AUDIO;
					else
						state = PLAY_BACK;
				}
				else	
					hdmi->rate = HDMI_TASK_INTERVAL;
				break;
			case CONFIG_AUDIO:
				rc = hdmi->ops->config_audio(hdmi, &(hdmi->config_set.audio));
				#ifdef HDMI_DEBUG_TIME
					hdmi_time_end = jiffies;
					if(hdmi_time_end > hdmi_time_start)
						hdmi_time_end -= hdmi_time_start;
					else
						hdmi_time_end += 0xFFFFFFFF - hdmi_time_start;
					dev_printk(KERN_INFO, hdmi->dev, "HDMI configure Audio cost %u ms\n", jiffies_to_msecs(hdmi_time_end));
				#endif
							
				if(rc == HDMI_ERROR_SUCESS)
				{
					hdmi->rate = 0;	
					state = PLAY_BACK;
				}
				else	
					hdmi->rate = HDMI_TASK_INTERVAL;
				break;
#if 0	
			case HDCP_AUTHENTICATION:
				if(hdmi->ops->config_hdcp)
				{
					rc = hdmi->ops->config_hdcp(hdmi, hdmi->config_set.hdcp_on);	
					#ifdef HDMI_DEBUG_TIME
					hdmi_time_end = jiffies;
					if(hdmi_time_end > hdmi_time_start)
						hdmi_time_end -= hdmi_time_start;
					else
						hdmi_time_end += 0xFFFFFFFF - hdmi_time_start;
					dev_printk(KERN_INFO, hdmi->dev, "HDMI configure HDCP cost %u ms\n", jiffies_to_msecs(hdmi_time_end));
					#endif
				}
				if(rc == HDMI_ERROR_SUCESS)
				{	
					hdmi->rate = 0;	
					state = PLAY_BACK;
				}
				else
					hdmi->rate = HDMI_TASK_INTERVAL;
				break;
#endif
			case PLAY_BACK:				
//				if(hdmi->config_real.display_on != hdmi->config_set.display_on)
				if( memcmp(&(hdmi->config_real), &(hdmi->config_set), sizeof(struct hdmi_configs)) )
				{
					if(hdmi->config_set.display_on == HDMI_DISPLAY_ON)
					{	
						if(hdmi->ops->config_hdcp)
							rc = hdmi->ops->config_hdcp(hdmi, hdmi->config_set.hdcp_on);
						rc |= hdmi->ops->enable(hdmi, HDMI_ENABLE);
					}
					else
						rc = hdmi->ops->enable(hdmi, HDMI_DISABLE);
					if(rc == HDMI_ERROR_SUCESS)
					{
						#ifdef HDMI_DEBUG_TIME
						hdmi_time_end = jiffies;
						if(hdmi_time_end > hdmi_time_start)
							hdmi_time_end -= hdmi_time_start;
						else
							hdmi_time_end += 0xFFFFFFFF - hdmi_time_start;
						dev_printk(KERN_INFO, hdmi->dev, "HDMI configuration cost %u ms\n", jiffies_to_msecs(hdmi_time_end));
						#endif
						memcpy(&(hdmi->config_real), &(hdmi->config_set), sizeof(struct hdmi_configs));
					}
					if(hdmi->config_set.display_on == HDMI_DISABLE)	
						state = WAIT_HDMI_ENABLE;
				}
				if(hdmi->wait == 1) {	
					complete(&hdmi->complete);
					hdmi->wait = 0;						
				}
				hdmi->rate = HDMI_TASK_INTERVAL;
				break;
			default:
				state = HDMI_INITIAL;
				hdmi->rate = HDMI_TASK_INTERVAL;
				break;
		}
		if(rc != HDMI_ERROR_SUCESS)
		{
			trytimes++;
			msleep(10);
		}
		if(state != state_last)
		{
			trytimes = 0;	
			hdmi_sys_show_state(hdmi, state);
		}
	}while((state != state_last || (rc != HDMI_ERROR_SUCESS) ) && trytimes < HDMI_MAX_TRY_TIMES);	
	
	if(trytimes == HDMI_MAX_TRY_TIMES)
	{
		if(hdmi->hpd_status)
		{
			if(state == CONFIG_AUDIO)
				state = HDCP_AUTHENTICATION;
			else if(state > WAIT_RX_SENSE)
			{	
				hdmi_sys_unplug(hdmi);
				if(hdmi->auto_switch && g_lcdcstatus)
				{
				    rk_display_device_enable_other(hdmi->ddev);
				    g_lcdcstatus = 0;
				}
				state = HDMI_UNKNOWN;
			}
		}
	}
	
exit:
	if(state != g_state)
		g_state = state;
	
	return 0;
}