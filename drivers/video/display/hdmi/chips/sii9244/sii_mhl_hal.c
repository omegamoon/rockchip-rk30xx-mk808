#include <linux/kernel.h>
#include <linux/i2c.h> 
#include <linux/delay.h>
#include <linux/jiffies.h>
#include "sii_mhl_hal.h"
#include "sii_mhl_9244.h"


//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------
static uint32_t g_timerCountersTime[ TIMER_COUNT ];
static uint16_t g_timerCounters[ TIMER_COUNT ];

static uint16_t g_timerElapsed;
static uint32_t g_elapsedTick;
static uint16_t g_timerElapsedGranularity;

static uint16_t g_timerElapsed1;
static uint32_t g_elapsedTick1;
static uint16_t g_timerElapsedGranularity1;

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
        g_timerCountersTime[ i] = 0;
    }
    g_timerElapsed  = 0;
    g_timerElapsed1 = 0;
    g_elapsedTick   = 0;
    g_elapsedTick1  = 0;
    g_timerElapsedGranularity   = 0;
    g_timerElapsedGranularity1  = 0;

//    // Set up Timer 0 for timer tick
//
//    TMOD |= 0x01;   // put timer 0 in Mode 1 (16-bit timer)
//
//    TL0 = 0x66;     // set timer count for interrupt every 1ms (based on 11Mhz crystal)
//    TH0 = 0xFC;     // each count = internal clock/12
//
//    TR0 = 1;        // start the timer
//    ET0 = 1;        // timer interrupts enable

}

//------------------------------------------------------------------------------
// Function:    HalTimerSet
// Description:
//------------------------------------------------------------------------------

void HalTimerSet (uint8_t index, uint16_t m_sec)
{
//    EA = 0;                             // Disable interrupts while updating

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

//    EA = 1;
}

//------------------------------------------------------------------------------
// Function:    HalTimerExpired
// Description: Returns > 0 if specified timer has expired.
//------------------------------------------------------------------------------

uint8_t HalTimerExpired (uint8_t timer)
{
//    if (timer < TIMER_COUNT)
//    {
//        return(g_timerCounters[timer] == 0);
//    }
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
    }

    return(0);
}

//------------------------------------------------------------------------------
// Function:    HalTimerWait
// Description: Waits for the specified number of milliseconds.
//------------------------------------------------------------------------------

void HalTimerWait ( uint16_t ms )
{

	//
	// For generating wake up and other pulses, we need a CPU spin instead of
	// interrupt based routines.
	// We would disable the interrupts to have more predictable time.
	//
//    EA = 0;             // Disable interrupts while waiting
	msleep(ms);
	// restore interrupts after waiting
//    EA = 1;
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
    	if(time_after((unsigned long)curtime,(unsigned long)g_elapsedTick1))
		{
			if(curtime > g_elapsedTick1)
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

//------------------------------------------------------------------------------
// Function: I2C_WriteByte
// Description:
//------------------------------------------------------------------------------
void I2C_WriteByte(uint8_t deviceID, uint8_t offset, uint8_t value)
{
//    SendAddr(deviceID, WRITE);
//    SendByte(offset);
//    SendByte(value);
//    SendStop();
    
	sii9244->client->addr = deviceID >> 1;
	i2c_master_reg8_send(sii9244->client, offset, &value, 1, SII9244_SCL_RATE);
}

//------------------------------------------------------------------------------
// Function: I2C_ReadByte
// Description:
//------------------------------------------------------------------------------
uint8_t I2C_ReadByte(uint8_t deviceID, uint8_t offset)
{
    uint8_t abyte;

//    SendAddr(deviceID, WRITE);
//    SendByte(offset);
//    SendAddr(deviceID, READ);
//
//    abyte = GetByte(LAST_BYTE);  //get single byte, same as last byte, no ACK
//    SendStop();

	sii9244->client->addr = deviceID >> 1;
	i2c_master_reg8_recv(sii9244->client, offset, &abyte, 1, SII9244_SCL_RATE);
    
    return (abyte);
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadBytePage0 ()
//
// PURPOSE		:	Read the value from a Page0 register.
//
// INPUT PARAMS	:	Offset	-	the offset of the Page0 register to be read.
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	The value read from the Page0 register.
//
//////////////////////////////////////////////////////////////////////////////

uint8_t ReadBytePage0 (uint8_t Offset)
{
	return I2C_ReadByte(PAGE_0_0X72, Offset);
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	WriteBytePage0 ()
//
// PURPOSE		:	Write a value to a Page0 register.
//
// INPUT PARAMS	:	Offset	-	the offset of the Page0 register to be written.
//					Data	-	the value to be written.
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	void
//
//////////////////////////////////////////////////////////////////////////////

void WriteBytePage0 (uint8_t Offset, uint8_t Data)
{
	I2C_WriteByte(PAGE_0_0X72, Offset, Data);
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadModifyWritePage0 ()
//
// PURPOSE		:	Set or clear individual bits in a Page0 register.
//
// INPUT PARAMS	:	Offset	-	the offset of the Page0 register to be modified.
//					Mask	-	"1" for each Page0 register bit that needs to be
//								modified
//					Data	-	The desired value for the register bits in their
//								proper positions
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	void
//
// EXAMPLE		:	If Mask of 0x0C and a 
//
//////////////////////////////////////////////////////////////////////////////

void ReadModifyWritePage0(uint8_t Offset, uint8_t Mask, uint8_t Data)
{

	uint8_t Temp;

	Temp = ReadBytePage0(Offset);		// Read the current value of the register.
	Temp &= ~Mask;					// Clear the bits that are set in Mask.
	Temp |= (Data & Mask);			// OR in new value. Apply Mask to Value for safety.
	WriteBytePage0(Offset, Temp);		// Write new value back to register.
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadByteCBUS ()
//
// PURPOSE		:	Read the value from a CBUS register.
//
// INPUT PARAMS	:	Offset - the offset of the CBUS register to be read.
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	The value read from the CBUS register.
//
//////////////////////////////////////////////////////////////////////////////

uint8_t ReadByteCBUS (uint8_t Offset)
{
	return I2C_ReadByte(PAGE_CBUS_0XC8, Offset);
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	WriteByteCBUS ()
//
// PURPOSE		:	Write a value to a CBUS register.
//
// INPUT PARAMS	:	Offset	-	the offset of the Page0 register to be written.
//					Data	-	the value to be written.
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	void
//
//////////////////////////////////////////////////////////////////////////////

void WriteByteCBUS(uint8_t Offset, uint8_t Data)
{
	I2C_WriteByte(PAGE_CBUS_0XC8, Offset, Data);
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadModifyWriteCBUS ()
//
// PURPOSE		:	Set or clear individual bits on CBUS page.
//
// INPUT PARAMS	:	Offset	-	the offset of the CBUS register to be modified.
//					Mask	-	"1" for each CBUS register bit that needs to be
//								modified
//					Data	-	The desired value for the register bits in their
//								proper positions
//
// OUTPUT PARAMS:	None
//
// GLOBALS USED	:	None
//
// RETURNS		:	void
//
//
//////////////////////////////////////////////////////////////////////////////

void ReadModifyWriteCBUS(uint8_t Offset, uint8_t Mask, uint8_t Value)
{
    uint8_t Temp;

    Temp = ReadByteCBUS(Offset);
    Temp &= ~Mask;
	Temp |= (Value & Mask);
    WriteByteCBUS(Offset, Temp);
}