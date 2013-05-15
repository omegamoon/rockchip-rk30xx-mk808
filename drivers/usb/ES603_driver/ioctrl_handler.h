#ifndef _IOCTRL_HANDLER_H
#define _IOCTRL_HANDLER_H

#include "driver_usb.h"

//For ES603
//#include <linux/spi/spidev.h>
#include <linux/egistec/Define_JadeOpc.h>
#include <linux/egistec/es603_sensor.h>

typedef enum _CTRLXFER_DIR{
	Ctrl_OUT = 0,
	Ctrl_IN
}CtrlXfer_DIR;



int Egis_IO_SCSI_Read(usb_ss801u *dev, int arg);
int Egis_IO_SCSI_Write(usb_ss801u *dev, int arg);

int Egis_IO_Bulk_Read(usb_ss801u *dev, int arg);
int Egis_IO_Bulk_Write(usb_ss801u *dev, int arg);

int Egis_IO_CtrlXfer(usb_ss801u *dev, int arg, CtrlXfer_DIR CtrlDir);
//int Egis_IO_Ctrl_Write(usb_ss801u *dev, void* arg);

int Egis_IO_Reset_Device(usb_ss801u *dev);

int es603_response_verify(u8 *data);

int ES603_Write_Register(usb_ss801u	*dev, int nRegLen, u8 *RegBuf, u8 *DataBuf);
int ES603_IO_Bulk_Read(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers);
int ES603_IO_Bulk_Write(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers);
int es603_io_bulk_get_image(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers);
int es603_io_bulk_get_full_image(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers);

#endif
