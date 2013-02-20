#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/err.h>
#include "hdmi_local.h"

static struct class *hdmi_class;
struct hdmi_id_ref_info {
	struct hdmi *hdmi;
	int id;
	int ref;
}ref_info[HDMI_MAX_ID];

void *hdmi_get_privdata(struct hdmi *hdmi)
{
	return hdmi->priv;
}
void hdmi_set_privdata(struct hdmi *hdmi, void *data)
{
	hdmi->priv = data;
	return;
}

#ifdef CONFIG_SII92326
#define HDMI_TASK_INTRVAL_MS	10
#else
#define HDMI_TASK_INTRVAL_MS	1000
#endif

int hdmi_task_interval(void)
{
	return msecs_to_jiffies(HDMI_TASK_INTRVAL_MS);
}

#ifdef HDMI_USE_IRQ
void hdmi_changed(struct hdmi *hdmi, unsigned int msec)
{
	hdmi_dbg(hdmi->dev, "%s\n", __FUNCTION__);
	queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, msecs_to_jiffies(msec));
}
#endif


static void hdmi_work_func(struct work_struct *work)
{
	struct hdmi *hdmi = container_of((void *)work, struct hdmi, delay_work);
	
	if(!HDMI_task(hdmi))
	{
		#if (!defined HDMI_USE_IRQ) || (defined CONFIG_ANX7150_IRQ_USE_RK29PIO) || (defined CONFIG_SII92326)
		if(hdmi->rate)
			queue_delayed_work(hdmi->workqueue, &hdmi->delay_work, hdmi->rate);
		#endif
	}
	
	return;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void hdmi_early_suspend(struct early_suspend *h)
{
	struct hdmi *hdmi = container_of(h, struct hdmi, early_suspend);
	hdmi_dbg(hdmi->dev, "hdmi enter early suspend\n");
	hdmi->enable = 0;
	hdmi->param_config = HDMI_CONFIG_ENABLE;
	/* wait for hdmi configuration finish */
	while(hdmi->wait)
		msleep(50);
	init_completion(&hdmi->complete);
	hdmi->wait = 1;
	#ifdef HDMI_USE_IRQ
	hdmi_changed(hdmi, 1);
	#endif
	wait_for_completion_interruptible_timeout(&hdmi->complete,
							msecs_to_jiffies(5000));
	flush_delayed_work(&hdmi->delay_work);
	return;
}
static char *envp[] = {"TRIGGER=WAKEUP", NULL};
static void hdmi_early_resume(struct early_suspend *h)
{
	struct hdmi *hdmi = container_of(h, struct hdmi, early_suspend);
	hdmi_dbg(hdmi->dev, "hdmi exit early resume\n");
	hdmi->enable = 1;
	hdmi->param_config = HDMI_CONFIG_ENABLE;
	kobject_uevent_env(&hdmi->dev->kobj, KOBJ_CHANGE, envp);
	#ifdef HDMI_USE_IRQ
	hdmi_changed(hdmi, 1);
	#endif
	return;
}
#endif

struct hdmi* hdmi_register(struct device *parent, struct hdmi_ops *ops, int video_source, int property)
{
	struct hdmi *hdmi;
	int  i;
	char name[8];

	if(	   !ops
		|| !ops->init
		|| !ops->detect_hpd
		|| !ops->read_edid
		|| !ops->config_video
		|| !ops->config_audio
		|| !ops->enable)
		return NULL;
	
	for(i = 0; i < HDMI_MAX_ID; i++) 
	{
		if(ref_info[i].ref == 0)
			break;
	}
	if(i == HDMI_MAX_ID)
		return NULL;
		
	hdmi = kzalloc(sizeof(struct hdmi), GFP_KERNEL);
    if (!hdmi)
    {
        printk("[%s] no memory for allocate hdmi\n", __FUNCTION__);
        return NULL;
    }
    
    memset(hdmi, 0, sizeof(struct hdmi));
   	hdmi->ops = ops;
   	hdmi->video_source = video_source;
   	hdmi->property = property;
	hdmi->enable = HDMI_ENABLE;
	hdmi->hpd_status = HDMI_RECEIVER_REMOVE;
#ifdef CONFIG_DISPLAY_AUTO_SWITCH
	hdmi->auto_switch = HDMI_ENABLE;
#else
	hdmi->auto_switch = HDMI_DISABLE;
#endif
	hdmi->auto_config = HDMI_AUTO_CONFIG;
	hdmi->param_config = HDMI_CONFIG_NONE;
	
	hdmi->config_set.display_on = HDMI_DISPLAY_HOLD;
	hdmi->config_set.hdcp_on = HDMI_HDCP_CONFIG;
	hdmi->config_set.resolution = HDMI_DEFAULT_RESOLUTION;
	hdmi->config_set.color = HDMI_DEFAULT_COLOR;
	hdmi->config_set.audio.type = HDMI_AUDIO_DEFAULT_TYPE;
	hdmi->config_set.audio.channel = HDMI_AUDIO_DEFAULT_CHANNEL_NUM;
	hdmi->config_set.audio.rate = HDMI_AUDIO_DEFAULT_Fs;
	hdmi->config_set.audio.word_length = HDMI_AUDIO_DEFAULT_WORD_LENGTH;
	
	hdmi->config_real.display_on = HDMI_DISPLAY_OFF;
	hdmi->config_real.hdcp_on = HDMI_HDCP_CONFIG;
	hdmi->config_real.resolution = HDMI_UNKOWN;
	hdmi->config_real.color = HDMI_UNKOWN;
	hdmi->config_real.audio.type = HDMI_UNKOWN;
	hdmi->config_real.audio.channel = HDMI_UNKOWN;
	hdmi->config_real.audio.rate = HDMI_UNKOWN;
	hdmi->config_real.audio.word_length = HDMI_UNKOWN;
	
	ext_hdmi_init_modelist(hdmi);
	
	sprintf(name, "hdmi-%d", hdmi->id);
	
	hdmi->ddev = ext_hdmi_register_display_sysfs(hdmi, NULL);
	if (IS_ERR(hdmi->dev)) {
		printk("[%s] register display system error\n", __FUNCTION__);
		kfree(hdmi);
		return NULL;
	}
	hdmi->dev = hdmi->ddev->dev;

	memset(name, 0, 8);
	sprintf(name, "hdmiwq%d", hdmi->id);
	hdmi->workqueue = create_singlethread_workqueue(name);
	INIT_DELAYED_WORK(&(hdmi->delay_work), hdmi_work_func);
#ifdef CONFIG_HAS_EARLYSUSPEND
	hdmi->early_suspend.suspend = hdmi_early_suspend;
	hdmi->early_suspend.resume = hdmi_early_resume;
	hdmi->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 1;
	register_early_suspend(&hdmi->early_suspend);
#endif

	hdmi->id = i;
	ref_info[i].hdmi = hdmi;
	ref_info[i].ref = 1;
	
	return hdmi;
}

void hdmi_unregister(struct hdmi *hdmi)
{
	flush_workqueue(hdmi->workqueue);
	destroy_workqueue(hdmi->workqueue);
	rk_display_device_unregister(hdmi->ddev);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&hdmi->early_suspend);
#endif
	ref_info[hdmi->id].ref = 0;
	ref_info[hdmi->id].hdmi = NULL;
	kfree(hdmi);
}

int hdmi_enable(struct hdmi *hdmi)
{
	rk_display_device_enable(hdmi->ddev);
	return 0;
}
EXPORT_SYMBOL(hdmi_enable);

int hdmi_config_audio(struct hdmi_audio	*audio)
{
	int i, j;
	struct hdmi *hdmi;
	if(audio == NULL)
		return HDMI_ERROR_FALSE;
		
	for(i = 0; i < HDMI_MAX_ID; i++)
	{
		if(ref_info[i].ref ==0)
			continue;
		hdmi = ref_info[i].hdmi;
		
		// Same as current audio setting, return.
		if(memcmp(audio, &hdmi->config_set.audio, sizeof(struct hdmi_audio)) == 0)
			continue;

		for(j = 0; j < hdmi->edid.audio_num; j++)
		{
			if(audio->type == hdmi->edid.audio_num)
				break;
		}
		
		if( (j == hdmi->edid.audio_num) ||
			(audio->channel > hdmi->edid.audio[j].channel) ||
			((audio->rate & hdmi->edid.audio[j].rate) == 0)||
			((audio->type == HDMI_AUDIO_LPCM) &&
			((audio->word_length & hdmi->edid.audio[j].word_length) == 0)) )
		{
			printk("[%s] warning : input audio type not supported in hdmi sink\n", __FUNCTION__);
//			continue;
		}
		
		memcpy(&hdmi->config_set.audio, audio, sizeof(struct hdmi_audio));
		hdmi->param_config = HDMI_CONFIG_AUDIO;
		init_completion(&hdmi->complete);
		hdmi->wait = 1;
		#ifdef HDMI_USE_IRQ
		hdmi_changed(hdmi, 1);
		#endif
		wait_for_completion_interruptible_timeout(&hdmi->complete,
								msecs_to_jiffies(10000));
	}
	return 0;
}

struct hdmi *get_hdmi_struct(int id)
{
	if(ref_info[id].ref == 0)
		return NULL;
	else
		return ref_info[id].hdmi;
}

static int __init hdmi_class_init(void)
{
	int i;
	
	hdmi_class = class_create(THIS_MODULE, "hdmi");

	if (IS_ERR(hdmi_class))
		return PTR_ERR(hdmi_class);
	for(i = 0; i < HDMI_MAX_ID; i++) {
		ref_info[i].id = i;
		ref_info[i].ref = 0;
		ref_info[i].hdmi = NULL;
	}
	return 0;
}

static void __exit hdmi_class_exit(void)
{
	class_destroy(hdmi_class);
}
//EXPORT_SYMBOL(hdmi_changed);
EXPORT_SYMBOL(hdmi_register);
EXPORT_SYMBOL(hdmi_unregister);
EXPORT_SYMBOL(get_hdmi_struct);
subsys_initcall(hdmi_class_init);
module_exit(hdmi_class_exit);

