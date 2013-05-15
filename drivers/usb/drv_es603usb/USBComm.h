#ifndef _USBCOMM_H_
#define _USBCOMM_H_

#include "driver_usb.h"
#include <linux/egistec/linux_driver.h>

typedef enum _TRANS_DIR
{
    dir_in  = 0,
    dir_out = 1
}trans_dir;

int Egis_Reset_device(usb_ss801u *dev) ;

int Egis_SndSCSICmd(
			usb_ss801u *dev, 							// device infomation
		    PCBW_Form CBW, 								// CBW
		    char* data, size_t datasize, trans_dir dir, // data infomation
 		    unsigned char* CSW,							// CSW
			unsigned long timeout  						// in jiffies, 0 means INIFINTE
		   );

int Egis_BulkXfer(
		usb_ss801u *dev,
		trans_dir 	dir,
		void* 		Data,
		int			DataSize,
		int*		pActualSize,
		int			Timeout
	);


#endif
