///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >hdmitx.h<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/

#ifndef _HDMITX_H_
#define _HDMITX_H_

// #define SUPPORT_DEGEN
// #define SUPPORT_SYNCEMB

#define SUPPORT_EDID
#define SUPPORT_HDCP
#define SUPPORT_INPUTRGB
#define SUPPORT_INPUTYUV444
#define SUPPORT_INPUTYUV422


#if defined(SUPPORT_INPUTYUV444) || defined(SUPPORT_INPUTYUV422)
#define SUPPORT_INPUTYUV
#endif

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hdmi-itv.h>
#include <linux/i2c.h>
#include "../../hdmi_local.h"

#include "cat6611.h"
#include "cat6611_sys.h"
#include "cat6611_drv.h"
#include "edid.h"
#include "typedef.h"



//////////////////////////////////////////////////////////////////////
// Function Prototype
//////////////////////////////////////////////////////////////////////


// dump
#ifdef DEBUG
void DumpCat6611Reg() ;
#endif
// I2C

//////////////////////////////////////////////////////////////////////////////
// main.c
//////////////////////////////////////////////////////////////////////////////

#endif // _HDMITX_H_

