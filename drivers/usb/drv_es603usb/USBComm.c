
#include "USBComm.h"
#include <linux/egistec/utility.h>

int Egis_Reset_device(usb_ss801u *dev) 
{
	int lock, ret;

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "\r\n=RESET_DEVICE=\r\n");		
		
	lock = usb_lock_device_for_reset(dev->udev, dev->interface); 	
	if (lock < 0) {					
	    EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=RESET_DEVICE= locking device failed: %d\r\n", lock);	
	    return lock;
	}						

	ret = usb_reset_device(dev->udev);			
	if (ret < 0)					
	    EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=RESET_DEVICE= reset device failed: %d\r\n", ret);	

	if (lock)					
	    usb_unlock_device(dev->udev);			

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "\r\n=RESET_DEVICE= Finish\r\n");
	return ret;
}

int Egis_BulkXfer(
		usb_ss801u *dev,
		trans_dir 	dir,
		void* 		Data,
		int			DataSize,
		int*		pActualSize,
		int			Timeout
)
{
	int retval = -1, i;
	int iActualRWSize;
	bool bRestore=false;

//Wayne for test
//  dev->bPrintDbgMsg = true;

//  pr_debug("%s:%d bulk In endpoint = %d Out endpoint = %d\n",  __FUNCTION__, __LINE__, dev->bulk_in_endpointAddr, dev->bulk_out_endpointAddr);
	if (!dev->udev)
	{
		pr_debug("dev->udev = NULL\n");
	}
	else
	{
		pr_debug("dev->udev->state = %d dev->udev->speed = %d\n", dev->udev->state, dev->udev->speed);
	}
	
//  for (i = 0; i < DataSize; i++)
//  {
//  	pr_debug("%2x ", (u8)Data[i]);
//  }
//  pr_debug("\n");

	mutex_lock(&dev->Bulk_Mutex);
	//----- if EPROTO happens, we will call clear feature and re-send again
	do{
		
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"Do usb_bulk_msg\n");
		retval = usb_bulk_msg(
					dev->udev,
		  			(dir_in == dir) ? 
						usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr) :
						usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
		   			Data,
		   			DataSize,
		   			&iActualRWSize, 
					Timeout
				 );
		// show data
//  	EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"following are data block of bulk transfer:\r\n");
//  	int iLoopIndex = 0;
//  	unsigned char *pData = Data;
//  	for(iLoopIndex = 0; iLoopIndex<DataSize; iLoopIndex++)
//  	{
//  		EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"%2x", pData[iLoopIndex]);
//  	}
//  	EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"\r\ndata block finish\r\n");
		
		if(EGIS_FAIL(retval)){
			//----- if error == EPROTO && not be restored --> clear halt
			if(EOVERFLOW == retval)
			{
				EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"=Egis_BulkXfer= bulk transfer error, EOVERFLOW, data size = %d"
					, DataSize);
			}
			if(EPROTO == retval && !bRestore){
				EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=Egis_BulkXfer= bulk xfer fail clear feature\r\n");
				retval = usb_clear_halt(dev->udev, (dir_in == dir) ? 
								usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr) :
								usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr)
						 );
				bRestore = true;
			}
			//----- this if include "else" above and check clear halt fail
			if(EGIS_FAIL(retval)){
				EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=Egis_BulkXfer= bulk xfer fail %d\r\n", retval);
				break;
			}
		}
		else{
			//----- if xfer is success, set bRestore=false to leave while loop
			bRestore = false;
		}
		
	}while(EGIS_SUCCESS(retval) && bRestore);

	memcpy(pActualSize, &iActualRWSize, sizeof(int));
	EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=Egis_BulkXfer= bulk xfer actual data size: %d\r\n", iActualRWSize);

	mutex_unlock(&dev->Bulk_Mutex);

	//----- If xfer fails, reset device.
	if(EGIS_FAIL(retval)){
		Egis_Reset_device(dev);
	}
	//Wayne for test
	dev->bPrintDbgMsg = false;
	return retval;
}


int Egis_SndSCSICmd(usb_ss801u *dev, 					// device infomation
		    PCBW_Form CBW, 								// CBW
		    char* data, size_t datasize, trans_dir dir, // data infomation
 		    unsigned char* CSW,							// CSW
			unsigned long timeout
		   )
{
    int retval=0;
    int bytes_read;
    unsigned char tmpCSW[CSW_SIZE];

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=Egis_sndSCSIcmd= Start\r\n");
	mutex_lock(&dev->SCSI_mutex);
    //================================== CBW =========================================//
    //================================================================================//
    EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=Egis_sndSCSIcmd= send CBW using bulk transfer\r\n");
	retval = Egis_BulkXfer(
				dev,
	  			dir_out,
       			CBW,
				CBW_SIZE,
			    &bytes_read, 
				timeout
			 );	
	if(EGIS_FAIL(retval)){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=Egis_sndSCSIcmd= snd CBW fail %d\r\n", retval);
		goto SCSI_EXIT;
	}

    //================================== DATA ========================================//
    //================================================================================//

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, 
		"=Egis_sndSCSIcmd= send or get data block using bulk transfer, data size is:%u\r\n", datasize);
    if(0 < datasize){
		retval = Egis_BulkXfer(
					dev,
			  		dir,
			   		data,
			    	datasize,
			    	&bytes_read, 
					timeout
				 );
	/*EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"following are data block:\r\n");
	int iLoopIndex = 0;
	for(iLoopIndex = 0; iLoopIndex<datasize; iLoopIndex++)
	{
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"%d", data[iLoopIndex]);
	}
	EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"\r\ndata block finish\r\n");*/
	
		if(EGIS_FAIL(retval)){
			EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=Egis_sndSCSIcmd= data xfer fail %d\r\n", retval);
			goto SCSI_EXIT;
		}
	}
	else
	{
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR,"=Egis_sndSCSIcmd= data size <= 0\r\n");
	}


    //================================== CSW =========================================//
    //================================================================================//

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=Egis_sndSCSIcmd= read CSW using bulk transfer\r\n");
	retval = Egis_BulkXfer(
				dev,
			  	dir_in,
				tmpCSW,
				CSW_SIZE,
				&bytes_read, 
				timeout
			 );
	if(EGIS_FAIL(retval)){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=Egis_sndSCSIcmd= rcv CSW fail %d\r\n", retval);
		goto SCSI_EXIT;
	}
	else{
		if(CSW){
			memcpy(CSW, tmpCSW, CSW_SIZE);
		}
	}

SCSI_EXIT:
	mutex_unlock(&dev->SCSI_mutex);
	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=Egis_sndSCSIcmd= END\r\n");
    return retval;
}



