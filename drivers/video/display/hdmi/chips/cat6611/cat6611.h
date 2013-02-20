#ifndef _CAT6611_H
#define _CAT6611_H


#define CAT6611_I2C_ADDR0	0X98	//0x9a
#define CAT6611_SCL_RATE	100 * 1000


struct cat6611_dev_s{
	struct i2c_driver *i2c_driver;
	int cat6611_detect;
	struct hdmi *hdmi;
};

struct cat6611_pdata {
	int irq;
	int io_irq_pin;
	int io_pwr_pin;
	int io_rst_pin;
	struct i2c_client *client;
	struct cat6611_dev_s dev;
};

#endif
