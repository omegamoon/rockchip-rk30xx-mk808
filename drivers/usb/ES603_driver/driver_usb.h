#ifndef DRIVER_USB_H_
#define DRIVER_USB_H_


#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <asm/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/time.h>

#include <linux/fs.h>
#include <linux/egistec/linux_driver.h>
#include "versioninfo.h"


//======================== DEFINITION ================================//
//====================================================================//
/* version information */
//#define DRIVER_VERSION_STR  "1.0.4"
//#define DRIVER_VERSION_NUM  0x01000400
#define DRIVER_SHORT    "ss801u"
#define DRIVER_AUTHOR   "Egistec"
#define DRIVER_DESC     "SS801U FingerPrint Sensor Driver"

/* driver minor code */
#define USB_SS801U_MINOR_BASE	211



#define MAX_TRANSFER		(PAGE_SIZE - 512)
/* MAX_TRANSFER is chosen so that the VM is not stressed by
   allocations > PAGE_SIZE and the number of packets in a page
   is an integer 512 is the largest possible packet on EHCI */
#define WRITES_IN_FLIGHT	8

//========================= FUNCTION =================================//
//====================================================================//
void ss801u_delete(struct kref *kref);



//========================= EMUERATION ===============================//
//====================================================================//
typedef enum _DEVICE_VENDOR
{
    DEVICE_NOTSUPPORT = 0,

    USBEST_801u = 1,
    MOAI_802u   = 2,
    EGIS_603	= 3
}Device_Vendor;



//=========================== VARIABLE ===============================//
//====================================================================//
extern const struct file_operations ss801u_fops;

//struct usb_driver ss801u_driver;

/*
 * data stored in usb_interface is like device extension in WDM
 */
typedef struct _USB_SS801U {
    struct usb_device       *udev;					/* the usb device for this device */
    struct usb_interface    *interface;				/* the interface for this device */

	//----- bulk transfer ------//
    //unsigned char           *bulk_in_buffer;		/* the buffer to receive data */
    size_t                  bulk_in_size;			/* the size of the receive buffer */
    __u8                    bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
    __u8                    bulk_out_endpointAddr;	/* the address of the bulk out endpoint */

	//----- ctrl transfer -----//
    //unsigned char           *ctrl_in_buffer;		/* the buffer to receive data */
   // size_t                  ctrl_xfer_size;			/* the size of the receive buffer */
    //__u8                    ctrl_endpointAddr;	/* the address of the ctrl in endpoint */
    //__u8                    ctrl_out_endpointAddr;	/* the address of the ctrl out endpoint */

    int                     errors;				/* the last request tanked */
    int                     open_count;			/* count the number of openers */
    spinlock_t              err_lock;			/* lock for errors */
    struct kref             kref;				// a counter to record the open times 
    struct mutex            io_mutex;			/* synchronize I/O with disconnect */
												// usd in open, release, read, write
	struct mutex			SCSI_mutex;			// Package scsi cmd
	struct mutex			Bulk_Mutex;

    CBW_Form	    		contactCBW;
    CBW_Form	    		normalCBW;  

    FPDriver_Version  		version;

    Device_Vendor   		dev_vendor;

	Sig_Type sig_type;
    struct fasync_struct* 	async_resume;
    struct fasync_struct* 	async_suspend;
	struct fasync_struct* 	async_disconnect;

	int						idle_delay_time;	// in second

	bool					bPrintDbgMsg;

	bool					bReadTwice;

}usb_ss801u;

#define to_ss801u_dev(d) container_of(d, usb_ss801u, kref)
#define to_usb_ss801u(d) container_of(d, usb_ss801u, udev)


#endif
