#include <linux/i2c.h>
#include <linux/delay.h>
#include <mach/board.h>
#include <mach/gpio.h>
#include "sii902x.h"

#define sii902x_SCL_RATE	100*1000

//------------------------------------------------------------------------------
// Function Name: DelayMS()
// Function Description: Introduce a busy-wait delay equal, in milliseconds, to the input parameter.
//
// Accepts: Length of required delay in milliseconds (max. 65535 ms)
inline void DelayMS (word MS)
{
	msleep(MS);
}

byte I2CReadByte ( byte SlaveAddr, byte RegAddr )
{
	byte val = 0;
	
	sii902x->client->addr = SlaveAddr >> 1;
	i2c_master_reg8_recv(sii902x->client, RegAddr, &val, 1, sii902x_SCL_RATE);
	return val;
}

//------------------------------------------------------------------------------
// Function Name: I2CReadBlock
// Function Description: Reads block of data from HDMI Device
//------------------------------------------------------------------------------
byte I2CReadBlock( byte SlaveAddr, byte RegAddr, byte NBytes, byte * Data )
{
	int ret;
	sii902x->client->addr = SlaveAddr >> 1;
	ret = i2c_master_reg8_recv(sii902x->client, RegAddr, Data, NBytes, sii902x_SCL_RATE);
	if(ret > 0)
		return 0;
	else
		return 1;
}

void I2CWriteByte ( byte SlaveAddr, byte RegAddr, byte Data )
{
	sii902x->client->addr = SlaveAddr >> 1;
	i2c_master_reg8_send(sii902x->client, RegAddr, &Data, 1, sii902x_SCL_RATE);
}

//------------------------------------------------------------------------------
// Function Name:  I2CWriteBlock
// Function Description: Writes block of data from HDMI Device
//------------------------------------------------------------------------------
byte I2CWriteBlock( byte SlaveAddr, byte RegAddr, byte NBytes, byte * Data )
{
	int ret;
	sii902x->client->addr = SlaveAddr >> 1;
	ret = i2c_master_reg8_send(sii902x->client, RegAddr, Data, NBytes, sii902x_SCL_RATE);
	if(ret > 0)
		return 0;
	else
		return -1;
}

//------------------------------------------------------------------------------
// Function Name:  siiReadSegmentBlockEDID
// Function Description: Reads segment block of EDID from HDMI Downstream Device
//------------------------------------------------------------------------------
byte siiReadSegmentBlockEDID(byte SlaveAddr, byte Segment, byte Offset, byte *Buffer, byte Length)
{
	int ret;
	struct i2c_client *client = sii902x->client;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msgs[3];

	msgs[0].addr = EDID_SEG_ADDR >> 1;
	msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	msgs[0].len = 1;
	msgs[0].buf = (char *)&Segment;
	msgs[0].scl_rate = sii902x_SCL_RATE;
	msgs[0].udelay = client->udelay;

	msgs[1].addr = SlaveAddr >> 1;
	msgs[1].flags = client->flags;
	msgs[1].len = 1;
	msgs[1].buf = &Offset;
	msgs[1].scl_rate = sii902x_SCL_RATE;
	msgs[1].udelay = client->udelay;

	msgs[2].addr = SlaveAddr >> 1;
	msgs[2].flags = client->flags | I2C_M_RD;
	msgs[2].len = Length;
	msgs[2].buf = (char *)Buffer;
	msgs[2].scl_rate = sii902x_SCL_RATE;
	msgs[2].udelay = client->udelay;
	
	ret = i2c_transfer(adap, msgs, 3);
	return (ret == 3) ? 0 : -1;
}

void SII902x_Reset(void)
{
	if(sii902x->io_rst_pin != INVALID_GPIO) {
		gpio_set_value(sii902x->io_rst_pin, 0);
		msleep(10);
		gpio_set_value(sii902x->io_rst_pin, 1);        //reset
	}
}