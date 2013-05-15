

#include "driver_usb.h"
#include "vendorcmd.h"
#include <linux/egistec/utility.h>


/* Support VID/PID */
#define USB_SS801U_VENDOR_ID	0x1c7a
#define USB_SS801U_PRODUCT_ID	0x0801

#define USB_SS801U2_VENDOR_ID	0x1307
#define USB_SS801U2_PRODUCT_ID	0x0172

#define USB_ES603_VENDOR_ID		0x1c7a
#define USB_ES603_PRODUCT_ID	0x0603

#ifndef info
#define info printk
#endif


//--------------- List all supported VID/PID here ------------------//
//------------------------------------------------------------------//
static struct usb_device_id ss801u_table [] = {
    { USB_DEVICE(USB_SS801U_VENDOR_ID ,	USB_SS801U_PRODUCT_ID) 	},
	{ USB_DEVICE(USB_SS801U2_VENDOR_ID, USB_SS801U2_PRODUCT_ID) },
	{ USB_DEVICE(USB_ES603_VENDOR_ID  ,	USB_ES603_PRODUCT_ID) 	},
    { }					/* Terminating entry */
};
//----- Register usb_device_id table to OS
MODULE_DEVICE_TABLE(usb, ss801u_table);


//------------------------- USB driver class -------------------------------//
//--------------------------------------------------------------------------//
/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver ss801u_class = {
    .name =			"esfp%d",
    .fops =			&ss801u_fops,
    .minor_base =	USB_SS801U_MINOR_BASE,
};

void ss801u_delete(struct kref *kref)
{
    usb_ss801u *dev = to_ss801u_dev(kref);

    //----- free udev
    usb_put_dev(dev->udev);

    //kfree(dev->bulk_in_buffer);
    kfree(dev);
}


static Device_Vendor Get_DeviceVendor(__le16 PID, __le16 VID)
{
    if( (USB_SS801U_VENDOR_ID == VID) && (USB_SS801U_PRODUCT_ID == PID) )
		return USBEST_801u;
	else if( (USB_SS801U2_VENDOR_ID == VID) && (USB_SS801U2_PRODUCT_ID == PID) )
		return USBEST_801u;
	else if( (USB_ES603_VENDOR_ID == VID) && (USB_ES603_PRODUCT_ID == PID) )
		return EGIS_603;

    return DEVICE_NOTSUPPORT;
}



static int ss801u_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    usb_ss801u *dev=NULL;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    size_t buffer_size;
    int i;
    int retval = -ENOMEM;


    /* allocate memory for our device state and initialize it */
    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev) {
        err("=PROBE=Out of memory\r\n");
        goto error;
    }
    kref_init(&dev->kref);
    mutex_init(&dev->io_mutex);
	mutex_init(&dev->SCSI_mutex);
	mutex_init(&dev->Bulk_Mutex);
    spin_lock_init(&dev->err_lock);

    dev->udev = usb_get_dev(interface_to_usbdev(interface));
    dev->interface = interface;


    /* set up the endpoint information */
    /* use only the first bulk-in and bulk-out endpoints */
    iface_desc = interface->cur_altsetting;

    info("There are %d endpoint. ", iface_desc->desc.bNumEndpoints);
    
    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i)
    {
        endpoint = &iface_desc->endpoint[i].desc;

        if (!dev->bulk_in_endpointAddr && usb_endpoint_is_bulk_in(endpoint))
        {
			info("bulk in endpoint %d\n", i);
            buffer_size = le16_to_cpu(endpoint->wMaxPacketSize);
            dev->bulk_in_size = buffer_size;
            info("The buffer_size is %d\n", buffer_size);
            dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
            //dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
            //if (!dev->bulk_in_buffer) {
            //    err("Could not allocate bulk_in_buffer");
            //    goto error;
            //}
        }

        if (!dev->bulk_out_endpointAddr &&
                usb_endpoint_is_bulk_out(endpoint)) {
            /* we found a bulk out endpoint */
			info("bulk out endpoint %d\n", i);
            dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
        }
    }
    if (!(dev->bulk_in_endpointAddr && dev->bulk_out_endpointAddr)) {
        err("Could not find both bulk-in and bulk-out endpoints");
        goto error;
    }


    //---- Get device vendor and sensor type ----//
    dev->dev_vendor = Get_DeviceVendor(dev->udev->descriptor.idProduct, dev->udev->descriptor.idVendor);

    //Wayne todo:check if is needed
    //Wayne for test: useless for ES603
    //---- init contact mode and normal mode command ----//
 #ifdef BEFORE_603
    Egis_Fill_Contactmode_Cmd(dev->dev_vendor, &dev->contactCBW);
    Egis_Fill_Normalmode_Cmd(dev->dev_vendor, &dev->normalCBW);
 #endif
    //Wayne todo:check if is needed !END

    //----- Fill version information -----//
    dev->version.vnumber = VERSION_DIGIT_EGISDRIVER;
    strcpy(dev->version.vstring, VERSION_EGISDRIVER);

	//----- Set auto-suspend mechanism -----//
	dev->idle_delay_time = 5;				// 5 second
//	dev->udev->autosuspend_disabled = 0;	// enable auto-suspend
//  dev->udev->autoresume_disabled = 0;		// enable auto-resume
	dev->udev->do_remote_wakeup = 1;		// enable remote wakeup
///	dev->udev->autosuspend_delay = dev->idle_delay_time*HZ;	// Set idle-delay time in second

	dev->bPrintDbgMsg = true;

    /* save our data pointer in this interface device */
    usb_set_intfdata(interface, dev);

    /* we can register the device now, as it is ready */
    retval = usb_register_dev(interface, &ss801u_class);
    if (retval) {
        /* something prevented us from registering this driver */
        err("Not able to get a minor for this device.\r\n");
        usb_set_intfdata(interface, NULL);
        goto error;
    }
	info("name of ss801u_class is %s.\n", ss801u_class.name);

    /* let the user know what node this device is now attached to */
    info("SS801U device now attached to #%d", interface->minor);
    return 0;

error:
    if (dev)
        /* this frees allocated memory */
        kref_put(&dev->kref, ss801u_delete);
    return retval;
}

static void ss801u_disconnect(struct usb_interface *interface)
{
    usb_ss801u *dev;
    int minor = interface->minor;

    dev = usb_get_intfdata(interface);

	if(dev->async_disconnect)
	    kill_fasync(&dev->async_disconnect, SIGIO, POLL_IN);


    usb_set_intfdata(interface, NULL);

    /* give back our minor */
    usb_deregister_dev(interface, &ss801u_class);

    /* prevent more I/O from starting */
    mutex_lock(&dev->io_mutex);
    dev->interface = NULL;
    mutex_unlock(&dev->io_mutex);

    //usb_kill_anchored_urbs(&dev->submitted);

    /* decrement our usage count */
    kref_put(&dev->kref, ss801u_delete);

    info("SS801U device #%d now disconnected", minor);

	//remove_proc_entry("driver/801udriver", NULL);
}



static int ss801u_suspend(struct usb_interface *intf, pm_message_t message)
{
    int retval = 0;
    usb_ss801u *dev = usb_get_intfdata(intf);
    int minor = intf->minor;
    unsigned char CSW[CSW_SIZE];

    if (!dev){
 		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=SUSPEND= dev is NULL\r\n");
        return 0;
    } 

    EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "SS801U device #%d now suspended", minor);

    //ss801u_draw_down(dev);

	if(/*dev->bSetSuspendAsync &&*/ dev->async_suspend)
	    kill_fasync(&dev->async_suspend, SIGIO, POLL_IN);

    retval = Egis_SndContactModeCmd(dev->dev_vendor, dev, CSW);

    return retval;
}

static int ss801u_resume (struct usb_interface *intf)
{
    usb_ss801u *dev = usb_get_intfdata(intf);
    int minor = intf->minor;

    if (!dev){
 		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=RESUME= dev is NULL\r\n");
        return 0;
    } 

    EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "SS801U device #%d now resume", minor);

	//----- Send signal to SDK
	if(dev->async_resume)
	    kill_fasync(&dev->async_resume, SIGIO, POLL_IN);

	//----- restore idle-delay time to user setting
	///dev->udev->autosuspend_delay = dev->idle_delay_time*HZ;

    //retval = Egis_SndNormalModeCmd(dev->dev_vendor, dev, CSW);
    return 0;
}

static int ss801u_reset_resume (struct usb_interface *intf)
{
    usb_ss801u *dev = usb_get_intfdata(intf);
    int minor = intf->minor;

    EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "SS801U device #%d now reset resumed", minor);

    if (!dev){
 		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=RESUME= dev is NULL\r\n");
        return 0;
    } 

	if(dev->async_resume)
	    kill_fasync(&dev->async_resume, SIGIO, POLL_IN);

	///dev->udev->autosuspend_delay = dev->idle_delay_time*HZ;

    //retval = Egis_SndNormalModeCmd(dev, CSW);

    return 0;
}

static int ss801u_pre_reset(struct usb_interface *intf)
{
    usb_ss801u *dev = usb_get_intfdata(intf);

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=PRERESET=\r\n");

    //mutex_lock(&dev->io_mutex);
    //ss801u_draw_down(dev);

    return 0;
}

static int ss801u_post_reset(struct usb_interface *intf)
{
    usb_ss801u *dev = usb_get_intfdata(intf);

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=POSTRESET=\r\n");
    /* we are sure no URBs are active - no locking needed */
    //dev->errors = -EPIPE;
    //mutex_unlock(&dev->io_mutex);

    return 0;
}

//--------------------------- USB driver --- -------------------------------//
//--------------------------------------------------------------------------//
struct usb_driver ss801u_driver = {

	.name       =       "esfp",   // global unique name, appear in /sys/bus/usb/drivers
	.probe      =       ss801u_probe,
	.disconnect =       ss801u_disconnect,

	.suspend    =       ss801u_suspend,
	.resume     =       ss801u_resume,
	.reset_resume =     ss801u_reset_resume,

	.pre_reset  =       ss801u_pre_reset,
	.post_reset =       ss801u_post_reset,

	.id_table   =       ss801u_table,       // VID, PID
	.supports_autosuspend = 1,
};



static int __init usb_ss801u_init(void)
{
    int result;

    info(" v" VERSION_EGISDRIVER);

    result = usb_register(&ss801u_driver);
    if (result)
        err("usb_register failed. Error number %d", result);

    return result;
}

static void __exit usb_ss801u_exit(void)
{
	info("\r\n ***** \r\n Driver Exit \r\n *****\r\n");
    usb_deregister(&ss801u_driver);
}

module_init(usb_ss801u_init);
module_exit(usb_ss801u_exit);

MODULE_VERSION(VERSION_EGISDRIVER);
