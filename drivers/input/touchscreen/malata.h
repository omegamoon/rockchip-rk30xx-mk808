/*
 *  "BQ" Cypress touchscreen driver
 *
 *  Copyright (C) 2008 LocoLabs LLC
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
 #define VERSION_1
#ifdef VERSION_1
/* I2C slave address */
#define BQ_I2C_SLAVE_ADDR		0x5c

/* I2C registers */
#define BQ_TOUCH_NUM			0x00
#define BQ_TOUCH_OLD_NUM			0x01
#define BQ_DATA_INFO			0x02
#define BQ_POS_X0_LO			0x02 	/* 16-bit register, MSB */
#define BQ_POS_X0_HI			0x03 	/* 16-bit register, LSB */
#define BQ_POS_Y0_LO			0x04 	/* 16-bit register, MSB */
#define BQ_POS_Y0_HI			0x05 	/* 16-bit register, LSB */
#define BQ_POS_X1_LO			0x06 	/* 16-bit register, MSB */
#define BQ_POS_X1_HI			0x07 	/* 16-bit register, LSB */
#define BQ_POS_Y1_LO			0x08 	/* 16-bit register, MSB */
#define BQ_POS_Y1_HI			0x09 	/* 16-bit register, LSB */
#define BQ_POS_PRESSURE			0x12
#define BQ_DATA_END				0x12
#define BQ_POWER_MODE			0x14
#define BQ_INT_MODE				0x15
#define BQ_SPECOP				0x37
#define BQ_EEPROM_READ			0x01
#define BQ_EEPROM_WRITE			0x02
#define BQ_EEPROM_READ_ADDR			0x38	/* 16-bit register */
#define BQ_RESOLUTION_X_LO			0x3d 	/* 8-bit register */
#define BQ_RESOLUTION_X_HI			0x01 	/* 8-bit register */
#define BQ_RESOLUTION_Y_LO			0x3f 	/* 8-bit register */
#define BQ_RESOLUTION_Y_HI			0x01 	/* 8-bit register */

#else

#define BQ_I2C_SLAVE_ADDR		0x34

/* I2C registers */
#define BQ_TOUCH-NUM			0x00
#define BQ_DATA_INFO			0x00
#define BQ_POS_X0_LO			0x03 	/* 16-bit register, MSB */
#define BQ_POS_X0_HI			0x04 	/* 16-bit register, LSB */
#define BQ_POS_Y0_LO			0x05 	/* 16-bit register, MSB */
#define BQ_POS_Y0_HI			0x06 	/* 16-bit register, LSB */
#define BQ_POS_X1_LO			0x09 	/* 16-bit register, MSB */
#define BQ_POS_X1_HI			0x0a 	/* 16-bit register, LSB */
#define BQ_POS_Y1_LO			0x0b 	/* 16-bit register, MSB */
#define BQ_POS_Y1_HI			0x0c 	/* 16-bit register, LSB */
#define BQ_DATA_END				0x0c

#endif


#define BQ_IRQ_PERIOD		(26 * 1000000) /* ns delay between interrupts */

struct BQ_data {
	struct i2c_client		*client;
	struct workqueue_struct		*workq;
	struct input_dev		*input;
	struct hrtimer    		timer;
	int				irq;
	u32 				dx;
	u32 				dy;
	u32				width;
	u16				x0;
	u16				y0;
	u16				x1;
	u16				y1;
	u8				z;
};
