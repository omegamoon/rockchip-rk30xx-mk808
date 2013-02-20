#include <linux/console.h>
#include <linux/fb.h>
#include <linux/hdmi-itv.h>
#include <linux/completion.h>
#include <linux/rk_screen.h>
#include <linux/rk_fb.h>
#include "hdmi_local.h"

/* Base */
#define LCD_ACLK		500000000// 312000000

#define OUT_TYPE		SCREEN_HDMI
#define OUT_FACE		OUT_P888
#define DCLK_POL		1
#define SWAP_RB			0

static const struct fb_videomode hdmi_mode [] = {
	//name				refresh		xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw	polariry	PorI	flag(used for vic)
//{	"640x480p@60Hz",	60,			640,	480,	25175000,	48,		16,		33,		10,		96,		2,		0,			0,		1	},
//{	"720x480i@60Hz",	60,			720,	480,	27000000,	114,	38,		15,		4,		124,	3,		0,			1,		6	},
//{	"720x576i@50Hz",	50,			720,	576,	27000000,	138,	24,		19,		2,		126,	3,		0,			1,		21	},
{	"720x480p@60Hz",	60,			720,	480,	27000000,	60,		16,		30,		9,		62,		6,		0,			0,		2	},
{	"720x576p@50Hz",	50,			720,	576,	27000000,	68,		12,		39,		5,		64,		5,		0,			0,		17	},
//{	"1280x720p@24Hz",	24,			1280,	720,	59400000,	220,	1760,	20,		5,		40,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		60	},
//{	"1280x720p@25Hz",	25,			1280,	720,	74250000,	220,	2420,	20,		5,		40,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		61	},
//{	"1280x720p@30Hz",	30,			1280,	720,	74250000,	220,	1760,	20,		5,		40,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		62	},
{	"1280x720p@50Hz",	50,			1280,	720,	74250000,	220,	440,	20,		5,		40,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		19	},
{	"1280x720p@60Hz",	60,			1280,	720,	74250000,	220,	110,	20,		5,		40,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		4	},
//{	"1920x1080p@24Hz",	24,			1920,	1080,	74250000,	148,	638,	36,		4,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		32	},
//{	"1920x1080p@25Hz",	25,			1920,	1080,	74250000,	148,	528,	36,		4,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		33	},
//{	"1920x1080p@30Hz",	30,			1920,	1080,	74250000,	148,	88,		36,		4,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		34	},	
#ifndef CONFIG_SII92326
//{	"1920x1080i@50Hz_2",50,			1920,	1080,	72000000,	184,	32,		57,		23,		168,	5,		FB_SYNC_HOR_HIGH_ACT,			1,		39	},
//{	"1920x1080i@50Hz",	50,			1920,	1080,	74250000,	148,	528,	15,		2,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			1,		20	},
//{	"1920x1080i@60Hz",	60,			1920,	1080,	74250000,	148,	88,		15,		2,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			1,		5	},
{	"1920x1080p@50Hz",	50,			1920,	1080,	148500000,	148,	528,	36,		4,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		31	},
{	"1920x1080p@60Hz",	60,			1920,	1080,	148500000,	148,	88,		36,		4,		44,		5,		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,			0,		16	},
#endif
/*
{	"1440x288p@50Hz",	50,			720,	480,	27000000,	138,	24,		19,		2,		126,	3,		0,			0,		23	},
{	"2880x576i@50Hz",	50,			1440,	240,	54000000,	276,	48,		19,		2,		252,	3,		0,			1,		25	},
{	"2880x288p@50Hz",	50,			2880,	480,	54000000,	276,	48,		19,		3,		252,	3,		0,			0,		27	},
{	"1440x576p@50Hz",	50,			2880,	480,	54000000,	136,	24,		39,		5,		128,	5,		0,			0,		29	},
{	"2880x576p@50Hz",	50,			1920,	1080,	108000000,	272,	48,		39,		5,		256,	5,		0,			0,		37	},
{	"1440x240p@60Hz",	60,			1440,	240,	27000000,	114,	38,		15,		4,		124,	3,		0,			0,		8	},
{	"2880x480i@60Hz",	60,			2880,	480,	54000000,	228,	76,		15,		4,		248,	3,		0,			1,		10	},
{	"2880x480p@60Hz",	60,			2880,	480,	54000000,	228,	76,		15,		4,		248,	3,		0,			0,		12	},
{	"1440x480p@60Hz",	60,			1440,	480,	54000000,	120,	32,		30,		9,		124,	6,		0,			0,		14	},
{	"2880x480p@60Hz",	60,			2880,	480,	54000000,	240,	64,		30,		9,		248,	6,		0,			0,		35	},

{	"1920x1080i@100Hz",	100,		1920,	1080,	148500000,	148,	528,	15,		2,		44,		5,		1,			1,		40	},
{	"1280x720p@100Hz",	100,		1280,	720,	148500000,	220,	440,	20,		5,		40,		5,		1,			0,		41	},
{	"720x576p@100Hz",	100,		720,	576,	54000000,	68,		12,		39,		5,		64,		5,		0,			0,		42	},
{	"1440x576i@100Hz",	100,		1440,	576,	54000000,	138,	24,		19,		2,		12,		3,		0,			1,		44	},
{	"1920x1080p@100Hz",	100,		1920,	1080,	297000000,	148,	528,	36,		4,		44,		5,		1,			0,		64	},

{	"1920x1080i@120Hz",	120,		1920,	1080,	148500000,	148,	88,		15,		2,		44,		5,		1,			1,		46	},
{	"1280x720p@120Hz",	120,		1280,	720,	148500000,	220,	110,	20,		5,		40,		5,		1,			0,		47	},
{	"720x480p@120Hz",	120,		720,	480,	54000000,	60,		16,		30,		9,		62,		6,		0,			0,		48	},
{	"1440x480i@120Hz",	120,		1440,	480,	54000000,	114,	38,		15,		4,		12,		3,		0,			1,		50	},
{	"1920x1080p@120Hz",	120,		1920,	1080,	297000000,	148,	88,		36,		4,		44,		5,		1,			0,		63	},

{	"720x576p@200Hz",	200,		720,	576,	108000000,	68,		12,		39,		5,		64,		5,		0,			0,		52	},
{	"1440x576i@200Hz",	200,		1920,	1080,	108000000,	138,	24,		19,		2,		12,		3,		0,			1,		54	},

{	"720x480p@240Hz",	240,		720,	480,	108000000,	60,		16,		30,		9,		62,		6,		0,			0,		56	},
{	"1440x480i@240Hz",	240,		1440,	480,	108000000,	114,	38,		15,		4,		12,		3,		0,			1,		58	},
*/

};
#ifdef CONFIG_ARCH_RK29
extern int FB_Switch_Screen( struct rk29fb_screen *screen, u32 enable );
#endif

static int hdmi_init(void)
{
    return 0;
}

static int hdmi_standby(u8 enable)
{
    return 0;
}

static int hdmi_set_info(struct rk29fb_screen *screen, unsigned int VIC)
{
    int i;
    
    if(screen == NULL || VIC == 0)
    	return -1;
    
    for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
    {
    	if(hdmi_mode[i].flag == VIC)
    		break;
    }
    if(i == ARRAY_SIZE(hdmi_mode))
    	return -1;
    	
    /* screen type & face */
    screen->type = OUT_TYPE;
    screen->face = OUT_FACE;

    /* Screen size */
    screen->x_res = hdmi_mode[i].xres;
    screen->y_res = hdmi_mode[i].yres;
//	screen->xpos = 0;
//    screen->ypos = 0;
    /* Timing */
    screen->pixclock = hdmi_mode[i].pixclock;
	screen->lcdc_aclk = LCD_ACLK;
	screen->left_margin = hdmi_mode[i].left_margin;
	screen->right_margin = hdmi_mode[i].right_margin;
	screen->hsync_len = hdmi_mode[i].hsync_len;
	screen->upper_margin = hdmi_mode[i].upper_margin;
	screen->lower_margin = hdmi_mode[i].lower_margin;
	screen->vsync_len = hdmi_mode[i].vsync_len;

	/* Pin polarity */
	if(FB_SYNC_HOR_HIGH_ACT & hdmi_mode[i].sync)
		screen->pin_hsync = 1;
	else
		screen->pin_hsync = 0;
	if(FB_SYNC_VERT_HIGH_ACT & hdmi_mode[i].sync)
		screen->pin_vsync = 1;
	else
		screen->pin_vsync = 0;
	screen->pin_den = 0;
	screen->pin_dclk = DCLK_POL;

	/* Swap rule */
    screen->swap_rb = SWAP_RB;
    screen->swap_rg = 0;
    screen->swap_gb = 0;
    screen->swap_delta = 0;
    screen->swap_dumy = 0;

    /* Operation function*/
    screen->init = hdmi_init;
    screen->standby = hdmi_standby;
    
    return 0;
}

static void hdmi_show_sink_info(struct hdmi *hdmi)
{
	struct list_head *pos, *head = &hdmi->edid.modelist;
	struct fb_modelist *modelist;
	struct fb_videomode *m;
	int i;
	struct hdmi_audio *audio;

	hdmi_dbg(hdmi->dev, "******** Show Sink Info ********\n");
	hdmi_dbg(hdmi->dev, "Support video mode: \n");
	list_for_each(pos, head) {
		modelist = list_entry(pos, struct fb_modelist, list);
		m = &modelist->mode;
		hdmi_dbg(hdmi->dev, "	%s.\n", m->name);
	}
	
	for(i = 0; i < hdmi->edid.audio_num; i++)
	{
		audio = &(hdmi->edid.audio[i]);
		switch(audio->type)
		{
			case HDMI_AUDIO_LPCM:
				hdmi_dbg(hdmi->dev, "Support audio type: LPCM\n");
				break;
			case HDMI_AUDIO_AC3:
				hdmi_dbg(hdmi->dev, "Support audio type: AC3\n");
				break;
			case HDMI_AUDIO_MPEG1:
				hdmi_dbg(hdmi->dev, "Support audio type: MPEG1\n");
				break;
			case HDMI_AUDIO_MP3:
				hdmi_dbg(hdmi->dev, "Support audio type: MP3\n");
				break;
			case HDMI_AUDIO_MPEG2:
				hdmi_dbg(hdmi->dev, "Support audio type: MPEG2\n");
				break;
			case HDMI_AUDIO_AAC_LC:
				hdmi_dbg(hdmi->dev, "Support audio type: AAC\n");
				break;
			case HDMI_AUDIO_DTS:
				hdmi_dbg(hdmi->dev, "Support audio type: DTS\n");
				break;
			case HDMI_AUDIO_ATARC:
				hdmi_dbg(hdmi->dev, "Support audio type: ATARC\n");
				break;
			case HDMI_AUDIO_DSD:
				hdmi_dbg(hdmi->dev, "Support audio type: DSD\n");
				break;
			case HDMI_AUDIO_E_AC3:
				hdmi_dbg(hdmi->dev, "Support audio type: E-AC3\n");
				break;
			case HDMI_AUDIO_DTS_HD:
				hdmi_dbg(hdmi->dev, "Support audio type: DTS-HD\n");
				break;
			case HDMI_AUDIO_MLP:
				hdmi_dbg(hdmi->dev, "Support audio type: MLP\n");
				break;
			case HDMI_AUDIO_DST:
				hdmi_dbg(hdmi->dev, "Support audio type: DST\n");
				break;
			case HDMI_AUDIO_WMA_PRO:
				hdmi_dbg(hdmi->dev, "Support audio type: WMP-PRO\n");
				break;
			default:
				hdmi_dbg(hdmi->dev, "Support audio type: Unkown\n");
				break;
		}
		
		hdmi_dbg(hdmi->dev, "Support audio sample rate: \n");
		if(audio->rate & HDMI_AUDIO_FS_32000)
			hdmi_dbg(hdmi->dev, "	32000\n");
		if(audio->rate & HDMI_AUDIO_FS_44100)
			hdmi_dbg(hdmi->dev, "	44100\n");
		if(audio->rate & HDMI_AUDIO_FS_48000)
			hdmi_dbg(hdmi->dev, "	48000\n");
		if(audio->rate & HDMI_AUDIO_FS_88200)
			hdmi_dbg(hdmi->dev, "	88200\n");
		if(audio->rate & HDMI_AUDIO_FS_96000)
			hdmi_dbg(hdmi->dev, "	96000\n");
		if(audio->rate & HDMI_AUDIO_FS_176400)
			hdmi_dbg(hdmi->dev, "	176400\n");
		if(audio->rate & HDMI_AUDIO_FS_192000)
			hdmi_dbg(hdmi->dev, "	192000\n");
		
		hdmi_dbg(hdmi->dev, "Support audio word lenght: \n");
		if(audio->rate & HDMI_AUDIO_WORD_LENGTH_16bit)
			hdmi_dbg(hdmi->dev, "	16bit\n");
		if(audio->rate & HDMI_AUDIO_WORD_LENGTH_20bit)
			hdmi_dbg(hdmi->dev, "	20bit\n");
		if(audio->rate & HDMI_AUDIO_WORD_LENGTH_24bit)
			hdmi_dbg(hdmi->dev, "	24bit\n");
	}
	hdmi_dbg(hdmi->dev, "******** Show Sink Info ********\n");
}

/**
 * hdmi_ouputmode_select - select hdmi transmitter output mode: hdmi or dvi?
 * @hdmi: handle of hdmi
 * @edid_ok: get EDID data success or not, HDMI_ERROR_SUCESS means success.
 */
int ext_hdmi_ouputmode_select(struct hdmi *hdmi, int edid_ok)
{
	struct list_head *head = &hdmi->edid.modelist;
	struct fb_monspecs	*specs = hdmi->edid.specs;
	struct fb_videomode *modedb = NULL;
	int i, pixclock;
	
	#ifdef CONFIG_HDMI_MODE_ALWAYS_HDMI
	hdmi->edid.is_hdmi = 1;
	#endif
	#ifdef CONFIG_HDMI_MODE_ALWAYS_DVI
	hdmi->edid.is_hdmi = 0;
	#endif
	#ifdef CONFIG_HDMI_MODE_DEPEND_EDID
	if(edid_ok != HDMI_ERROR_SUCESS) {
		dev_info(hdmi->dev, "warning: EDID error, assume sink as HDMI !!!!");
		hdmi->edid.is_hdmi = 1;
	}
	#endif
	if(edid_ok != HDMI_ERROR_SUCESS) {
		hdmi->edid.ycbcr444 = 0;
		hdmi->edid.ycbcr422 = 0;
		hdmi->auto_config = HDMI_DISABLE;
	}
	if(head->next == head) {
		dev_info(hdmi->dev, "warning: no CEA video mode parsed from EDID !!!!");
		// If EDID get error, list all system supported mode.
		// If output mode is set to DVI and EDID is ok, check
		// the output timing.
		
		if(hdmi->edid.is_hdmi == 0 && specs && specs->modedb_len) {
			/* Get max resolution timing */
			modedb = &specs->modedb[0];
			for (i = 0; i < specs->modedb_len; i++) {
				if(specs->modedb[i].xres > modedb->xres)
					modedb = &specs->modedb[i];
				else if(specs->modedb[i].yres > modedb->yres)
					modedb = &specs->modedb[i];
			}
			// For some monitor, the max pixclock read from EDID is smaller
			// than the clock of max resolution mode supported. We fix it.
			pixclock = PICOS2KHZ(modedb->pixclock);
			pixclock /= 250;
			pixclock *= 250;
			pixclock *= 1000;
			if(pixclock == 148250000)
				pixclock = 148500000;
			if(pixclock > specs->dclkmax)
				specs->dclkmax = pixclock;
		}
		
		for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++) {
			if(modedb) {
				if( (hdmi_mode[i].pixclock < specs->dclkmin) || 
					(hdmi_mode[i].pixclock > specs->dclkmax) || 
					(hdmi_mode[i].refresh < specs->vfmin) ||
					(hdmi_mode[i].refresh > specs->vfmax) ||
					(hdmi_mode[i].xres > modedb->xres) ||
					(hdmi_mode[i].yres > modedb->yres) )
				continue;
			}
			ext_hdmi_add_videomode(&hdmi_mode[i], head);
		}
	}
	
	#ifdef CONFIG_HDMI_DEBUG
	hdmi_show_sink_info(hdmi);
	#endif
	return HDMI_ERROR_SUCESS;
}
/**
 * hdmi_videomode_compare - compare 2 videomodes
 * @mode1: first videomode
 * @mode2: second videomode
 *
 * RETURNS:
 * 1 if mode1 > mode2, 0 if mode1 = mode2, -1 mode1 < mode2
 */
static int hdmi_videomode_compare(const struct fb_videomode *mode1,
		     const struct fb_videomode *mode2)
{
	if(mode1->xres > mode2->xres)
		return 1;
	else if(mode1->xres == mode2->xres)
	{ 
		if(mode1->yres > mode2->yres)
			return 1;
		else if(mode1->yres == mode2->yres)
		{
			if(mode1->pixclock > mode2->pixclock)	
				return 1;
			else if(mode1->pixclock == mode2->pixclock)
			{	
				if(mode1->refresh > mode2->refresh)
					return 1;
				else if(mode1->refresh == mode2->refresh) 
					return 0;
			}
		}
	}
	return -1;		
}
/**
 * hdmi_add_videomode: adds videomode entry to modelist
 * @mode: videomode to add
 * @head: struct list_head of modelist
 *
 * NOTES:
 * Will only add unmatched mode entries
 */
int ext_hdmi_add_videomode(const struct fb_videomode *mode, struct list_head *head)
{
	struct list_head *pos;
	struct fb_modelist *modelist, *modelist_new;
	struct fb_videomode *m;
	int i, found = 0;

	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
    {
    	m =(struct fb_videomode*) &hdmi_mode[i];
    	if (fb_mode_is_equal(m, mode)) {
			found = 1;
			break;
		}
    }

	if (found) {
		list_for_each(pos, head) {
			modelist = list_entry(pos, struct fb_modelist, list);
			m = &modelist->mode;
			if (fb_mode_is_equal(m, mode)) {
			// m == mode	
				return 0;
			}
			else
			{ 
				if(hdmi_videomode_compare(m, mode) == -1) {
					break;
				}
			}
		}

		modelist_new = kmalloc(sizeof(struct fb_modelist),
				  GFP_KERNEL);					
		if (!modelist_new)
			return -ENOMEM;	
		modelist_new->mode = hdmi_mode[i];
		list_add_tail(&modelist_new->list, pos);
	}
	
	return 0;
}

/**
 * hdmi_videomode_to_vic: transverse video mode to vic
 * @vmode: videomode to transverse
 * 
 */
int ext_hdmi_videomode_to_vic(struct fb_videomode *vmode)
{
	unsigned char vic = 0;
	int i = 0;
	
	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
	{
		if(	vmode->vmode == hdmi_mode[i].vmode &&
			vmode->refresh == hdmi_mode[i].refresh &&
			vmode->xres == hdmi_mode[i].xres && 
			vmode->left_margin == hdmi_mode[i].left_margin &&
			vmode->right_margin == hdmi_mode[i].right_margin &&
			vmode->upper_margin == hdmi_mode[i].upper_margin &&
			vmode->lower_margin == hdmi_mode[i].lower_margin && 
			vmode->hsync_len == hdmi_mode[i].hsync_len && 
			vmode->vsync_len == hdmi_mode[i].vsync_len)
		{
			if( (vmode->vmode == FB_VMODE_NONINTERLACED && vmode->yres == hdmi_mode[i].yres) || 
				(vmode->vmode == FB_VMODE_INTERLACED && vmode->yres == hdmi_mode[i].yres/2))
			{								
				vic = hdmi_mode[i].flag;
				break;
			}
		}
	}
	return vic;
}

/**
 * hdmi_vic_to_videomode: transverse vic mode to video mode
 * @vmode: vic to transverse
 * 
 */
const struct fb_videomode* ext_hdmi_vic_to_videomode(int vic)
{
	int i;
	
	if(vic == 0)
		return NULL;
	
	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
	{
		if(hdmi_mode[i].flag == vic)
			return &hdmi_mode[i];
	}
	return NULL;
}

/**
 * hdmi_find_best_mode: find the video mode nearest to input vic
 * @hdmi: 
 * @vic: input vic
 * 
 * NOTES:
 * If vic is zero, return the high resolution video mode vic.
 */
int ext_hdmi_find_best_mode(struct hdmi* hdmi, int vic)
{
	struct list_head *pos, *head = &hdmi->edid.modelist;
	struct fb_modelist *modelist;
	struct fb_videomode *m = NULL;
	int found = 0;
	
	if(vic)
	{
		list_for_each(pos, head) {
			modelist = list_entry(pos, struct fb_modelist, list);
			m = &modelist->mode;
			if(m->flag == vic)
			{
				found = 1;	
				break;
			}
		}
	}
	if( (vic == 0 || found == 0) && head->next != head)
	{
		modelist = list_entry(head->next, struct fb_modelist, list);
		m = &modelist->mode;
	}
	if(m != NULL)
		return m->flag;
	else
		return 0;
}

const char *ext_hdmi_get_video_mode_name(unsigned char vic)
{
	int i;
	
	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
	{
		if(vic == hdmi_mode[i].flag)
			break;
	}
	if(i == ARRAY_SIZE(hdmi_mode))
		return NULL;
	else
		return hdmi_mode[i].name;
}

/**
 * hdmi_switch_fb: switch lcdc mode to required video mode
 * @hdmi: 
 * @type:
 * 
 * NOTES:
 * 
 */
int ext_hdmi_switch_fb(struct hdmi *hdmi, int type)
{
	int rc = 0;
	struct rk29fb_screen hdmi_info;

	if(hdmi->config_set.resolution == 0)
		hdmi->config_set.resolution = HDMI_DEFAULT_RESOLUTION;
	memset(&hdmi_info, 0, sizeof(struct rk29fb_screen));
	rc = hdmi_set_info(&hdmi_info, hdmi->config_set.resolution);

	if(rc == 0) {
	#ifdef CONFIG_ARCH_RK29    
	   	FB_Switch_Screen(&hdmi_info, type);
	#else
		rk_fb_switch_screen(&hdmi_info, type , hdmi->video_source);
	#endif
	}
	if(hdmi->wait == 1) {
		complete(&hdmi->complete);
		hdmi->wait = 0;
	}
	return rc;
}

void ext_hdmi_init_modelist(struct hdmi *hdmi)
{
	int i;
	struct list_head *head = &hdmi->edid.modelist;
	
	INIT_LIST_HEAD(&hdmi->edid.modelist);
	for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++) {
		ext_hdmi_add_videomode(&hdmi_mode[i], head);
	}
}
