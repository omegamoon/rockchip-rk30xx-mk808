#ifndef __SII9244_H__
#define __SII9244_H__

#include <linux/workqueue.h>

#define SII9244_SCL_RATE		90000

#define T_MONITORING_PERIOD		100

#define MONITORING_PERIOD		50

#define SiI_DEVICE_ID			0xB0

#define TX_HW_RESET_PERIOD		10
#define TX_HW_RESET_DELAY		100

struct sii9244 {
	int irq;
	int io_irq_pin;
	int io_pwr_pin;
	int io_rst_pin;
	struct i2c_client *client;
	struct workqueue_struct *workqueue;
	struct delayed_work delay_work;
};

extern struct sii9244 *sii9244;

extern void SiiOsBumpMhlTxScheduler(void);
extern void SiiOsMhlTxInterruptEnable(void);

#endif //__SII9244_H__
