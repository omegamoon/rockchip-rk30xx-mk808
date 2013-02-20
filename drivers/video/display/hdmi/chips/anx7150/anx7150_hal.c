#include <linux/i2c.h>
#include "anx7150.h"
#include "anx7150_hw.h"

int anx7150_i2c_read_p0_reg(struct i2c_client *client, char reg, char *val)
{
	client->addr = ANX7150_I2C_ADDR0;
	return i2c_master_reg8_recv(client, reg, val, 1, ANX7150_SCL_RATE) > 0? 0: HDMI_ERROR_I2C;
}
int anx7150_i2c_write_p0_reg(struct i2c_client *client, char reg, char *val)
{
	client->addr = ANX7150_I2C_ADDR0;
	return i2c_master_reg8_send(client, reg, val, 1, ANX7150_SCL_RATE) > 0? 0: HDMI_ERROR_I2C;
}
int anx7150_i2c_read_p1_reg(struct i2c_client *client, char reg, char *val)
{
	client->addr = ANX7150_I2C_ADDR1;
	return i2c_master_reg8_recv(client, reg, val, 1, ANX7150_SCL_RATE) > 0? 0: HDMI_ERROR_I2C;
}
int anx7150_i2c_write_p1_reg(struct i2c_client *client, char reg, char *val)
{
	client->addr = ANX7150_I2C_ADDR1;
	return i2c_master_reg8_send(client, reg, val, 1, ANX7150_SCL_RATE) > 0? 0: HDMI_ERROR_I2C;
}

//void inline DRVDelayMs(unsigned int ms)
//{
//	msleep(ms);
//}