/***********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
//#include <stdio.h>

#ifndef __SII_92326_API_H__
#define __SII_92326_API_H__

/* C99 defined data types.  */
/*
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;

typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed long    int32_t;
*/
#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif
typedef enum
{
    FALSE   = 0,
    TRUE    = !(FALSE)
} bool_t;


#define LOW                     0
#define HIGH                    1

// Generic Masks
//==============
#define _ZERO		    0x00
#define BIT0                   0x01
#define BIT1                   0x02
#define BIT2                   0x04
#define BIT3                   0x08
#define BIT4                   0x10
#define BIT5                   0x20
#define BIT6                   0x40
#define BIT7                   0x80

// Timers - Target system uses these timers
#define ELAPSED_TIMER               0xFF
#define ELAPSED_TIMER1             0xFE	// For from discovery to MHL est timeout

typedef enum TimerId
{
    TIMER_FOR_MONITORING= 0,		// HalTimerWait() is implemented using busy waiting
    TIMER_POLLING,		// Reserved for main polling loop
    TIMER_2,			// Available
    TIMER_SWWA_WRITE_STAT,
    TIMER_TO_DO_RSEN_CHK,
    TIMER_TO_DO_RSEN_DEGLITCH,
    TIMER_COUNT			// MUST BE LAST!!!!
} timerId_t;

//
// This is the time in milliseconds we poll what we poll.
//
#define MONITORING_PERIOD		50

#define SiI_DEVICE_ID			0xB0

#define TX_HW_RESET_PERIOD	10

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Debug Definitions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define DISABLE 0x00
#define ENABLE  0xFF

// Compile debug prints inline or not
#define CONF__TX_API_PRINT   	(ENABLE)
#define CONF__TX_DEBUG_PRINT   (ENABLE)//(DISABLE)
#define CONF__TX_EDID_PRINT   	(ENABLE)

/*\
| | Debug Print Macro
| |
| | Note: TX_DEBUG_PRINT Requires double parenthesis
| | Example:  TX_DEBUG_PRINT(("hello, world!\n"));
\*/

#if (CONF__TX_DEBUG_PRINT == ENABLE)
    #define TX_DEBUG_PRINT(x)	printk x
#else
    #define TX_DEBUG_PRINT(x)
#endif

#if (CONF__TX_API_PRINT == ENABLE)
    #define TX_API_PRINT(x)	printk x
#else
    #define TX_API_PRINT(x)
#endif

#if (CONF__TX_EDID_PRINT == ENABLE)
    #define TX_EDID_PRINT(x)	printk x
#else
    #define TX_EDID_PRINT(x)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern bool_t	vbusPowerState;
//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------

void HalTimerInit ( void );
void 	HalTimerSet ( uint8_t index, uint16_t m_sec );
uint8_t 	HalTimerExpired ( uint8_t index );
void		DelayMS ( uint16_t m_sec );
uint16_t	HalTimerElapsed( uint8_t index );


///////////////////////////////////////////////////////////////////////////////
//
// AppMhlTxDisableInterrupts
//
// This function or macro is invoked from MhlTx driver to secure the processor
// before entering into a critical region.
//
// Application module must provide this function.
//
extern	void	AppMhlTxDisableInterrupts( void );


///////////////////////////////////////////////////////////////////////////////
//
// AppMhlTxRestoreInterrupts
//
// This function or macro is invoked from MhlTx driver to secure the processor
// before entering into a critical region.
//
// Application module must provide this function.
//
extern	void	AppMhlTxRestoreInterrupts( void );


///////////////////////////////////////////////////////////////////////////////
//
// AppVbusControl
//
// This function or macro is invoked from MhlTx driver to ask application to
// control the VBUS power. If powerOn is sent as non-zero, one should assume
// peer does not need power so quickly remove VBUS power.
//
// if value of "powerOn" is 0, then application must turn the VBUS power on
// within 50ms of this call to meet MHL specs timing.
//
// Application module must provide this function.
//
extern	void	AppVbusControl( bool_t powerOn );

#endif  // __SII_92326_API_H__
