#ifndef _COLORADO_H
#define _COLORADO_H
#include <linux/gpio.h>
#include <linux/hdmi-itv.h>
#include "../../hdmi_local.h"

/*
 * Below five GPIOs are example  for AP to control the Slimport chip ANX7805. 
 * Different AP needs to configure these control pins to corresponding GPIOs of AP.
 */

/*******************Slimport Control************************/
#define INVALID_GPIO        -1
#define SP_TX_PWR_V12_CTRL              INVALID_GPIO//(139)//AP IO Control - Power+V12
#define SP_TX_HW_RESET                  RK29_PIN4_PD3    //(136)//AP IO Control - Reset 
#define SLIMPORT_CABLE_DETECT         	RK29_PIN6_PD1//(132)//AP IO Input - Cable detect 
#define VBUS_PWR_CTRL                   INVALID_GPIO//      (155)//AP IO Control - VBUS+5V  
#define SP_TX_CHIP_PD_CTRL              RK29_PIN6_PB6  //(137)//AP IO Control - CHIP_PW_HV 

#define SSC_ON
//#define SSC_1    
//#define DISBALBE_SCRAMBLE
#define LVTTL_EN                  //0811
//#define SPDIF_EN                  //0811
#define MIPI_LANE_COUNT 3   //0811
#define MIPI_DEBUG



#define AUX_ERR  1
#define AUX_OK   0
#define SOURCE_AUX_OK	1
#define SOURCE_AUX_ERR	0
#define SOURCE_REG_OK	1
#define SOURCE_REG_ERR	0

int  Colorado_System_Init (void);
unsigned char SP_TX_Write_Reg(unsigned char dev,unsigned char offset, unsigned char d);
unsigned char SP_TX_Read_Reg(unsigned char dev,unsigned char offset, unsigned char *d);

struct anx7805_dev_s{
	struct i2c_driver *i2c_driver;
	int detect;
	struct hdmi *hdmi;
};

struct anx7805_pdata {
	int irq;
	int gpio;
	struct i2c_client *client;
	struct anx7805_dev_s dev;
};

/* HDMI system control interface */
int ANX7805_sys_init(struct hdmi *hdmi);
int ANX7805_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus);
int ANX7805_sys_plug(struct hdmi *hdmi);
int ANX7805_sys_detect_sink(struct hdmi *hdmi, int *sink_status);
int ANX7805_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff);
int ANX7805_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color);
int ANX7805_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio);
int ANX7805_sys_config_hdcp(struct hdmi *hdmi, int enable);
int ANX7805_sys_enalbe_output(struct hdmi *hdmi, int enable);
#endif
