/***********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
//#include <stdio.h>
//#include "hal_mcu.h"
#include "sii92326.h"
#include "sii_92326_driver.h"
#include "sii_92326_api.h"

//sbit pinMHLTxVbus_CTRL = P2^1;	// VDD 5V to MHL VBUS switch control

#define	APP_DEMO_RCP_SEND_KEY_CODE 0x41

//static   void	AppRcpDemo( uint8_t event, uint8_t eventParameter);

bool_t	vbusPowerState = TRUE;		// FALSE: 0 = vbus output on; TRUE: 1 = vbus output off;
//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------
#include <linux/jiffies.h>
static uint32_t g_timerCountersTime[ TIMER_COUNT ];
static uint16_t g_timerCounters[ TIMER_COUNT ];

static uint16_t g_timerElapsed;
static uint32_t g_elapsedTick;
static uint16_t g_timerElapsedGranularity;

static uint16_t g_timerElapsed1;
static uint32_t g_elapsedTick1;
static uint16_t g_timerElapsedGranularity1;

#if 0
//------------------------------------------------------------------------------
// Function: TimerTickHandler
// Description:
//------------------------------------------------------------------------------
static void TimerTickHandler ( void ) interrupt 1
{
    uint8_t i;

    //decrement all active timers in array

    for ( i = 0; i < TIMER_COUNT; i++ )
    {
        if ( g_timerCounters[ i ] > 0 )
        {
            g_timerCounters[ i ]--;
        }
    }
    g_elapsedTick++;
    if ( g_elapsedTick == g_timerElapsedGranularity )
    {
        g_timerElapsed++;
        g_elapsedTick = 0;
    }
    g_elapsedTick1++;
    if ( g_elapsedTick1 == g_timerElapsedGranularity1 )
    {
        g_timerElapsed1++;
        g_elapsedTick1 = 0;
    }
}

//------------------------------------------------------------------------------
// Function: MhlTxISR
// Description:
//------------------------------------------------------------------------------
static void MhlTxISR( void ) interrupt 2 
{
	MhlTxISRCounter += 30;	//15;
}
#endif
//------------------------------------------------------------------------------
// Function: HalTimerInit
// Description:
//------------------------------------------------------------------------------

void HalTimerInit ( void )
{
    uint8_t i;

    //initializer timer counters in array
    for ( i = 0; i < TIMER_COUNT; i++ )
    {
        g_timerCounters[ i ] = 0;
		g_timerCountersTime[ i ] = 0;
    }
    g_timerElapsed  = 0;
    g_timerElapsed1 = 0;
    g_elapsedTick   = 0;
    g_elapsedTick1  = 0;
    g_timerElapsedGranularity   = 0;
    g_timerElapsedGranularity1  = 0;
}

//------------------------------------------------------------------------------
// Function:    HalTimerSet
// Description:
//------------------------------------------------------------------------------

void HalTimerSet (uint8_t index, uint16_t m_sec)
{

    switch (index)
    {
    	case ELAPSED_TIMER:
        	g_timerElapsedGranularity = m_sec;
        	g_timerElapsed = 0;
        	g_elapsedTick = jiffies;
        	break;

    	case ELAPSED_TIMER1:
        	g_timerElapsedGranularity1 = m_sec;
        	g_timerElapsed1 = 0;
        	g_elapsedTick1 = jiffies;
        	break;
        default:
        	g_timerCounters[index] = m_sec;
        	g_timerCountersTime[index] = jiffies;
        	break;
    }

}

//------------------------------------------------------------------------------
// Function:    HalTimerExpired
// Description: Returns > 0 if specified timer has expired.
//------------------------------------------------------------------------------

uint8_t HalTimerExpired (uint8_t timer)
{
    uint32_t	curtime, deltatime;
    if (timer < TIMER_COUNT)
    {
    	curtime = jiffies;
    	if(time_after((unsigned long)curtime,(unsigned long)g_timerCountersTime[timer]))
    	{
    		if(curtime > g_timerCountersTime[timer])
    			deltatime = (curtime - g_timerCountersTime[timer]);
    		else
    			deltatime = (0xffffffff + curtime - g_timerCountersTime[timer]);

    		return (jiffies_to_msecs(deltatime)/g_timerCounters[timer]);
    	}
//		return(g_timerCounters[timer] == 0);
    }

    return(0);
}

//------------------------------------------------------------------------------
// Function:    HalTimerElapsed
// Description: Returns current timer tick.  Rollover depends on the
//              granularity specified in the SetTimer() call.
//------------------------------------------------------------------------------

uint16_t HalTimerElapsed ( uint8_t index )
{
	uint32_t curtime, deltatime = 0;
    uint16_t elapsedTime;
    
	curtime = jiffies;
    if ( index == ELAPSED_TIMER ) {
//		elapsedTime = g_timerElapsed;
    	if(time_after((unsigned long)curtime,(unsigned long)g_elapsedTick))
		{
			if(curtime > g_elapsedTick)
				deltatime = (curtime - g_elapsedTick);
			else
				deltatime = (0xffffffff + curtime - g_elapsedTick);
		}
		if(g_timerElapsedGranularity)
			elapsedTime = (jiffies_to_msecs(deltatime)/g_timerElapsedGranularity);
		else
			elapsedTime = 0;
	}
    else {
 //		elapsedTime = g_timerElapsed1;
    	if(time_after((unsigned long)curtime,(unsigned long)g_elapsedTick1))
		{
			if(curtime > g_elapsedTick)
				deltatime = (curtime - g_elapsedTick1);
			else
				deltatime = (0xffffffff + curtime - g_elapsedTick1);
		}
		if(g_timerElapsedGranularity1)
			elapsedTime = (jiffies_to_msecs(deltatime)/g_timerElapsedGranularity1);
		else
			elapsedTime = 0;
	}
    return( elapsedTime );
}

#if 0
///////////////////////////////////////////////////////////////////////////////
//
// main
//
// This is part of application layer code. This file simply has samples on
// how to call the APIs of the MhlTx component. This is a functioning module
// for the 92326 starter kit.
//
// 1. This code is not meant for FPGA emulation board or any starter kit earlier than X04
// 2. MHL wake up pulse has been implemented.
// 3. SWWA 20005 has been commented out.
// 4. New API list for RCP and other MHL sideband channel commands. See PR.
// 5. MHL 1.0 specification related timing and logic has been added.
// 6. Code can be run using pure polling or interrupt driven.
//
void main(void)
{
	bool_t 	interruptDriven;
	uint8_t 	pollIntervalMs;
	uint8_t	event;
	uint8_t	eventParameter;
	
	HalTimerInit();
	HalTimerSet (TIMER_POLLING, MONITORING_PERIOD);

	// Announce on RS232c port.
	//
	TX_API_PRINT(("\n============================================\n"));
	TX_API_PRINT(("Copyright 2011 Silicon Image\n"));
	TX_API_PRINT(("SiI-92326 Starter Kit Firmware Version 1.00.94_App V1.2\n"));
	TX_API_PRINT(("[Built:  %s-%s]\n", __DATE2__, __TIME__));
	TX_API_PRINT(("============================================\n"));

	//
	// Initialize the registers as required. Setup firmware vars.
	//
	SiiMhlTxInitialize( interruptDriven = FALSE, pollIntervalMs = MONITORING_PERIOD);
	
	siMhlTx_VideoSel( HDMI_720P60 );
	siMhlTx_AudioSel( AFS_48K );
    
	//
	// Loop forever looking for interrupts
	//
	while (TRUE)
	{
	
		//
		// Look for any events that might have occurred.
		//
		SiiMhlTxGetEvents( &event, &eventParameter );

		if( MHL_TX_EVENT_NONE != event )
		{
			AppRcpDemo( event, eventParameter);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// AppRcpDemo
//
// This function is supposed to provide a demo code to elicit how to call RCP
// API function.
//
void	AppRcpDemo( uint8_t event, uint8_t eventParameter)
{
	uint8_t		rcpKeyCode;

//	printf("App:%d Got event = %02X, eventParameter = %02X\n",(int)__LINE__, (int)event, (int)eventParameter);

	switch( event )
	{
		case	MHL_TX_EVENT_DISCONNECTION:
			TX_API_PRINT(("[MHL]App: Got event = MHL_TX_EVENT_DISCONNECTION\n"));
			break;

		case	MHL_TX_EVENT_CONNECTION:
			TX_API_PRINT(("[MHL]App: Got event = MHL_TX_EVENT_CONNECTION\n"));
			break;

		case	MHL_TX_EVENT_RCP_READY:
    			TX_API_PRINT(("[MHL]App: Got event = MHL_TX_EVENT_RCP_READY...\n"));

			if( (0 == (MHL_FEATURE_RCP_SUPPORT & eventParameter)) )
			{
				TX_API_PRINT(("[MHL]App: Peer does NOT support RCP\n"));
			}
            		else
            		{
				TX_API_PRINT(("[MHL]App: Peer supports RCP\n"));
			}
			if( (0 == (MHL_FEATURE_RAP_SUPPORT & eventParameter)) )
			{
				TX_API_PRINT(("[MHL]App: Peer does NOT support RAP\n"));
			}
            		else
            		{
				TX_API_PRINT(("[MHL]App: Peer supports RAP\n"));
			}
			if( (0 == (MHL_FEATURE_SP_SUPPORT & eventParameter)) )
			{
				TX_API_PRINT(("[MHL]App: Peer does NOT support WRITE_BURST\n"));
			}
            		else
			{
				TX_API_PRINT(("[MHL]App: Peer supports WRITE_BURST\n"));
			}

#if 0	//RCP key send demo in here
              	if (MHL_FEATURE_RCP_SUPPORT & eventParameter)
            		{
                		if (!MhlTxCBusBusy())
	                	{
	    				// Demo RCP key code Volume Up
	    				rcpKeyCode = APP_DEMO_RCP_SEND_KEY_CODE;

	        			TX_API_PRINT(("[MHL]App: Sending RCP (%02X)\n",(int) rcpKeyCode));
					//
					// If RCP engine is ready, send one code
					//
					if( SiiMhlTxRcpSend( rcpKeyCode ))
					{
	        				TX_API_PRINT(("[MHL]App: SiiMhlTxRcpSend (%02X)\n",(int) rcpKeyCode));
					}
					else
					{
	        				TX_API_PRINT(("[MHL]App: SiiMhlTxRcpSend (%02X) Returned Failure.\n",(int) rcpKeyCode));
	        			}
				}
			}
#endif
			break;

		case	MHL_TX_EVENT_RCP_RECEIVED:
			//
			// Check if we got an RCP. Application can perform the operation here
			// and send RCPK or RCPE. For now, we send the RCPK
			//
			rcpKeyCode = eventParameter;
			TX_API_PRINT(("[MHL]App: Received an RCP key code = %02X\n",(int)rcpKeyCode));
			SiiMhlTxRcpkSend(rcpKeyCode);

			// Added RCP key printf and interface with UI. //by oscar 20101217
		    	switch ( rcpKeyCode )
		    	{
		        	case MHL_RCP_CMD_SELECT:
					TX_API_PRINT(( "\n[MHL]App: Select received\n\n" ));
					break;
		        	case MHL_RCP_CMD_UP:
					TX_API_PRINT(( "\n[MHL]App: Up received\n\n" ));
					break;
		        	case MHL_RCP_CMD_DOWN:
					TX_API_PRINT(( "\n[MHL]App: Down received\n\n" ));
					break;
		        	case MHL_RCP_CMD_LEFT:
					TX_API_PRINT(( "\n[MHL]App: Left received\n\n" ));
					break;
		        	case MHL_RCP_CMD_RIGHT:
					TX_API_PRINT(( "\n[MHL]App: Right received\n\n" ));
					break;
		        	case MHL_RCP_CMD_ROOT_MENU:
					TX_API_PRINT(( "\n[MHL]App: Root Menu received\n\n" ));
					break;
		        	case MHL_RCP_CMD_EXIT:
					TX_API_PRINT(( "\n[MHL]App: Exit received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_0:
					TX_API_PRINT(( "\n[MHL]App: Number 0 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_1:
					TX_API_PRINT(( "\n[MHL]App: Number 1 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_2:
					TX_API_PRINT(( "\n[MHL]App: Number 2 received\n\n" ));
					break;	
		        	case MHL_RCP_CMD_NUM_3:
					TX_API_PRINT(( "\n[MHL]App: Number 3 received\n\n" ));
					break;	
		        	case MHL_RCP_CMD_NUM_4:
					TX_API_PRINT(( "\n[MHL]App: Number 4 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_5:
					TX_API_PRINT(( "\n[MHL]App: Number 5 received\n\n" ));
					break;	
		        	case MHL_RCP_CMD_NUM_6:
					TX_API_PRINT(( "\n[MHL]App: Number 6 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_7:
					TX_API_PRINT(( "\n[MHL]App: Number 7 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_8:
					TX_API_PRINT(( "\n[MHL]App: Number 8 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_9:
					TX_API_PRINT(( "\n[MHL]App: Number 9 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_DOT:
					TX_API_PRINT(( "\n[MHL]App: Dot received\n\n" ));
					break;
		        	case MHL_RCP_CMD_ENTER:
					TX_API_PRINT(( "\n[MHL]App: Enter received\n\n" ));
					break;
		        	case MHL_RCP_CMD_CLEAR:
					TX_API_PRINT(( "\n[MHL]App: Clear received\n\n" ));
					break;
				case MHL_RCP_CMD_SOUND_SELECT:
					TX_API_PRINT(( "\n[MHL]App: Sound Select received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PLAY:
					TX_API_PRINT(( "\n[MHL]App: Play received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PAUSE:
					TX_API_PRINT(( "\n[MHL]App: Pause received\n\n" ));
					break;
		        	case MHL_RCP_CMD_STOP:
					TX_API_PRINT(( "\n[MHL]App: Stop received\n\n" ));
					break;
		        	case MHL_RCP_CMD_FAST_FWD:
					TX_API_PRINT(( "\n[MHL]App: Fastfwd received\n\n" ));
					break;
		        	case MHL_RCP_CMD_REWIND:
					TX_API_PRINT(( "\n[MHL]App: Rewind received\n\n" ));
					break;
				case MHL_RCP_CMD_EJECT:
					TX_API_PRINT(( "\n[MHL]App: Eject received\n\n" ));
					break;
				case MHL_RCP_CMD_FWD:
		 			TX_API_PRINT(( "\n[MHL]App: Forward received\n\n" ));
					break;
		   		case MHL_RCP_CMD_BKWD:
		   			TX_API_PRINT(( "\n[MHL]App: Backward received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PLAY_FUNC:
					TX_API_PRINT(( "\n[MHL]App: Play Function received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PAUSE_PLAY_FUNC:
					TX_API_PRINT(( "\n[MHL]App: Pause_Play Function received\n\n" ));
					break;
		   		case MHL_RCP_CMD_STOP_FUNC:
		   			TX_API_PRINT(( "\n[MHL]App: Stop Function received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F1:
		   			TX_API_PRINT(( "\n[MHL]App: F1 received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F2:
		   			TX_API_PRINT(( "\n[MHL]App: F2 received\n\n" ));
					break;
			   	case MHL_RCP_CMD_F3:
		   			TX_API_PRINT(( "\n[MHL]App: F3 received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F4:
		   			TX_API_PRINT(( "\n[MHL]App: F4 received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F5:
		   			TX_API_PRINT(( "\n[MHL]App: F5 received\n\n" ));
					break;
        			default:
        				break;
      			}
				
			break;

		case	MHL_TX_EVENT_RCPK_RECEIVED:
			TX_API_PRINT(("[MHL]App: Received an RCPK = %02X\n", (int)eventParameter));
			break;

		case	MHL_TX_EVENT_RCPE_RECEIVED:
			TX_API_PRINT(("[MHL]App: Received an RCPE = %02X\n", (int)eventParameter));
			break;

		default:
			break;
	}
}
#endif

#if (VBUS_POWER_CHK == ENABLE)
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
void	AppVbusControl( bool_t powerOn )
{
	struct hdmi_platform_data *mhl_data = NULL;
	
	if(sii92326->client->dev.platform_data)
		mhl_data = sii92326->client->dev.platform_data;
	
	if( powerOn )
	{
//		pinMHLTxVbus_CTRL = 1;
		if(mhl_data->power)
			mhl_data->power(1);
		TX_API_PRINT(("[MHL]App: Peer's POW bit is set. Turn the VBUS power OFF here.\n"));
	}
	else
	{
//		pinMHLTxVbus_CTRL = 0;
		if(mhl_data->power)
			mhl_data->power(0);
		TX_API_PRINT(("[MHL]App: Peer's POW bit is cleared. Turn the VBUS power ON here.\n"));
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////
//
// Following function is added for rk29 itv driver. 
//
///////////////////////////////////////////////////////////////////////////////
#include <linux/i2c.h>
#include <linux/delay.h>
#include "sii92326.h"
#include <mach/board.h>

#define I2C_SCL_RATE	100*1000

//------------------------------------------------------------------------------
// Function Name: DelayMS()
// Function Description: Introduce a busy-wait delay equal, in milliseconds, to the input parameter.
//
// Accepts: Length of required delay in milliseconds (max. 65535 ms)
inline void	DelayMS ( uint16_t m_sec )
{
	msleep(m_sec);
}

uint8_t I2CReadByte(uint8_t SlaveAddr, uint8_t RegAddr)
{
	uint8_t val = 0;
	
	sii92326->client->addr = SlaveAddr >> 1;
	i2c_master_reg8_recv(sii92326->client, RegAddr, &val, 1, I2C_SCL_RATE);
	return val;
}

//------------------------------------------------------------------------------
// Function Name: I2CReadBlock
// Function Description: Reads block of data from HDMI Device
//------------------------------------------------------------------------------
uint8_t I2CReadBlock(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t NBytes, uint8_t * Data)
{
	int ret;
	sii92326->client->addr = SlaveAddr >> 1;
	ret = i2c_master_reg8_recv(sii92326->client, RegAddr, Data, NBytes, I2C_SCL_RATE);
	if(ret > 0)
		return 0;
	else
		return 1;
}

void I2CWriteByte(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Data)
{
	sii92326->client->addr = SlaveAddr >> 1;
	i2c_master_reg8_send(sii92326->client, RegAddr, &Data, 1, I2C_SCL_RATE);
}

//------------------------------------------------------------------------------
// Function Name:  I2CWriteBlock
// Function Description: Writes block of data from HDMI Device
//------------------------------------------------------------------------------
extern uint8_t I2CWriteBlock(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t NBytes, uint8_t * Data)
{
	int ret;
	sii92326->client->addr = SlaveAddr >> 1;
	ret = i2c_master_reg8_send(sii92326->client, RegAddr, Data, NBytes, I2C_SCL_RATE);
	if(ret > 0)
		return 0;
	else
		return -1;
}

//------------------------------------------------------------------------------
// Function Name:  I2CReadSegmentBlockEDID
// Function Description: Reads segment block of EDID from HDMI Downstream Device
//------------------------------------------------------------------------------
uint8_t I2CReadSegmentBlockEDID(uint8_t SlaveAddr, uint8_t Segment, uint8_t Offset, uint8_t *Buffer, uint8_t Length)
{

	int ret;
	struct i2c_client *client = sii92326->client;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msgs[3];

	msgs[0].addr = EDID_SEG_ADDR >> 1;
	msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	msgs[0].len = 1;
	msgs[0].buf = (char *)&Segment;
	msgs[0].scl_rate = I2C_SCL_RATE;
	msgs[0].udelay = client->udelay;

	msgs[1].addr = SlaveAddr >> 1;
	msgs[1].flags = client->flags;
	msgs[1].len = 1;
	msgs[1].buf = &Offset;
	msgs[1].scl_rate = I2C_SCL_RATE;
	msgs[1].udelay = client->udelay;

	msgs[2].addr = SlaveAddr >> 1;
	msgs[2].flags = client->flags | I2C_M_RD;
	msgs[2].len = Length;
	msgs[2].buf = (char *)Buffer;
	msgs[2].scl_rate = I2C_SCL_RATE;
	msgs[2].udelay = client->udelay;
	
	ret = i2c_transfer(adap, msgs, 3);
	return (ret == 3) ? 0 : -1;
}

void SiI92326_Reset(void)
{
	if(sii92326->io_rst_pin != INVALID_GPIO) {
		gpio_set_value(sii92326->io_rst_pin, 0);
		msleep(10);
		gpio_set_value(sii92326->io_rst_pin, 1);        //reset
	}
}