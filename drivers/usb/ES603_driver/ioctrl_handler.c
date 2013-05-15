
#include "ioctrl_handler.h" 
#include <linux/egistec/linux_driver.h>
#include "USBComm.h"
#include <linux/egistec/utility.h>

int Egis_IO_Reset_Device(usb_ss801u *dev)
{
	return Egis_Reset_device(dev);
}


int Egis_IO_SCSI_Read(usb_ss801u *dev, int arg)
{
	int retval;
	SCSICmd_Buf buf_scsi;
//  int index;
//  int bound;
//  unsigned char *pBuf;
	//info("=ioctl=SS801U_SCSI_READ ");
  	//_____Step1. get data from arg
	//-----1.1 get scsi_buff(CBW data) data
   	if(copy_from_user((void*)&buf_scsi, (void*)arg, sizeof(SCSICmd_Buf))){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_READ copy scsi from user fail\r\n");
		retval = -EFAULT; return retval;
   	}

	//log all data user sends
	//EgisMsg(dev->bPrintDbgunsigned, KERN_ERR, "\r\n=ioctl=EGIS_IOCTL_SCSI_READ list all data\r\n");
	//EgisMsg(dev->bPrintDbgunsigned, KERN_ERR, 
	//	"=ioctl=EGIS_IOCTL_SCSI_READ timeout of command is:%u\r\n", buf_scsi.timeout);
	//EgisMsg(dev->bPrintDbgunsigned, KERN_ERR, 
	//	"=ioctl=EGIS_IOCTL_SCSI_READ size of CBW is:%d\r\n", sizeof(CBW_Form));
	
	//pBuf = (void*)&buf_scsi;
	//bound = sizeof(SCSICmd_Buf);
	//for(index = 0; index < bound; index++)
	//{
	//	EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "%d", pBuf[index]);
	//}
	//EgisMsg(dev->bPrintDbgMsg, KERN_ERR, 
	//	"\r\n=ioctl=EGIS_IOCTL_SCSI_READ list data finish, data size = %u\r\n", buf_scsi.datasize);
	
   	//-----1.2 allocate and initial data buffer
   	buf_scsi.data = kzalloc(buf_scsi.datasize, GFP_KERNEL);
    if (!buf_scsi.data) {
   		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_READ Out of memory\r\n");
		retval = -EFAULT; return retval;
   	}

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=ioctl=EGIS_IOCTL_SCSI_READ begin to send SCSI command\r\n");
   	//_____Step2. send scsi cmd
   	retval = Egis_SndSCSICmd( 
				dev, 					  		// device infomation
     	       	&buf_scsi.CBW, 				  	// CBW
		     	buf_scsi.data, buf_scsi.datasize, dir_in, // data infomation
		     	buf_scsi.CSW,				  	// CSW
				buf_scsi.timeout
    		);

	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=ioctl=EGIS_IOCTL_SCSI_READ copy to user buffer\r\n");
   	//_____Step3. if send successfully, copy to user buffer and CSW
   	if(EGIS_SUCCESS(retval)){
    	if(copy_to_user(((SCSICmd_Buf*)arg)->data, 
						buf_scsi.data, 
						buf_scsi.datasize))
		{
		    kfree(buf_scsi.data); 
		    EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_READ copy to user fail\r\n");
		    retval = -EFAULT; return retval;
    	}
    	if(copy_to_user((void*)arg, &buf_scsi, sizeof(SCSICmd_Buf))){
		    kfree(buf_scsi.data); 
		    EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_READ copy CSW fail\r\n");
		    return retval;
    	}
    }

   	kfree(buf_scsi.data);
	return retval;
}

int Egis_IO_SCSI_Write(usb_ss801u *dev, int arg)
{
	int retval;
	SCSICmd_Buf buf_scsi;
   	//info("=ioctl=SS801U_SCSI_WRITE ");
   	//_____Step1. get data from arg
   	//-----1.1 get scsi_buff(CBW data) data
   	if(copy_from_user(&buf_scsi, (void*)arg, sizeof(SCSICmd_Buf))){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_WRITE copy scsi from user fail\r\n");
		retval = -EFAULT; return retval;
   	}

   	//-----1.2 check data allocation is needed
   	if(0 < buf_scsi.datasize){
		//----- allocate, initial data buffer
   		buf_scsi.data = kzalloc(buf_scsi.datasize, GFP_KERNEL);
   		if (!buf_scsi.data) {
       		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_WRITE Out of memory\r\n");
    		retval = -EFAULT; return retval;
   		}
		//----- get transfer data
		if(copy_from_user(buf_scsi.data, 
						  ((SCSICmd_Buf*)arg)->data, 
						  ((SCSICmd_Buf*)arg)->datasize))
		{
    		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_WRITE copy data fail\r\n");
   	    	retval = EFAULT; return retval;
   		}
   	}
   	else{
		buf_scsi.data = (char*) 0;
		buf_scsi.datasize = 0;
   	}

   	//_____Step2. send scsi cmd
   	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=ioctl=EGIS_IOCTL_SCSI_WRITE begin to send SCSI command\r\n");
   	retval = Egis_SndSCSICmd( 
				dev, 					  		// device infomation
	     		&buf_scsi.CBW, 				  	// CBW
		      	buf_scsi.data, buf_scsi.datasize, dir_out,// data infomation
		      	buf_scsi.CSW,				  	// CSW
				buf_scsi.timeout
	    	);


   	//_____Step3. if send successfully, copy CSW
   	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=ioctl=EGIS_IOCTL_SCSI_WRITE copy to user buffer\r\n");
   	if(EGIS_SUCCESS(retval)){
		//----- if copy fail, free data buffer
   		if(copy_to_user((void*)arg, &buf_scsi, sizeof(SCSICmd_Buf))){
    		if(0 < buf_scsi.datasize){
    			kfree(buf_scsi.data); 
    		}
    		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_SCSI_READ copy CSW fail\r\n");
    		return retval;
   		}
   	}

   	if(0 < buf_scsi.datasize){
   		kfree(buf_scsi.data);
   	}	
	return retval;    
}

/***********************************For ES603******************************************/

int es603_response_verify(u8 *data)
{
	u8 target[6] = 
	{
		0x53, 0x49, 0x47, 0x45, 0x0a, 0x01
	};
	int i = 0;

	for (i = ARRAY_SIZE(target) - 1; i >= 0; --i)
	{
//  	pr_debug("es603_response_verify i = %d", i);
		if (data[i] != target[i] )
		{
			pr_debug("%s:%s:%d Fail at data = %2X i = %d\n", __FILE__, __FUNCTION__, __LINE__, data[i], i);
			break;
		}
	}
	++i;
	pr_debug("es603_response_verify = %d", i);
	return i;
}

int ES603_Write_Register(usb_ss801u	*dev, int nRegLen, u8 *RegBuf, u8 *DataBuf)
{

	// Init
	unsigned		n_ioc = 0;
	struct egis_ioc_transfer *SIT;
	int i = 0;
   	u8	*RegDataBuf = NULL;
	RegDataBuf =  kmalloc(nRegLen * 2, GFP_KERNEL);
	SIT = kmalloc(sizeof(struct egis_ioc_transfer), GFP_KERNEL);

	// Set RegDataBuf
	for (i = 0;i < nRegLen;i++)
	{
		RegDataBuf[i * 2] = RegBuf[i];
		RegDataBuf[i * 2 + 1] = DataBuf[i];
	}
	

	// Set SIT
	SIT->tx_buf = (unsigned long)RegDataBuf;
	SIT->rx_buf = 0;
	SIT->len = nRegLen * 2;
	SIT->delay_usecs = 0;
	SIT->speed_hz = 14000000;
	SIT->bits_per_word = 8;
	SIT->cs_change = 1;
	SIT->opcode = 0x02;

	
	// Call ES603_IO_Bulk_Write 
	ES603_IO_Bulk_Write(dev, SIT, n_ioc);

	kfree(SIT);
	kfree(RegDataBuf);
	
	return 0;

}

int ES603_IO_Bulk_Read(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers)
{
	int retval;
	int bytes_read = 0;
	struct egis_ioc_transfer *u_tmp;
	u8	*tx_buf = NULL;
	unsigned int uiRet = 0;

 	pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);

	u_tmp = u_xfers;

//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	tx_buf =  kmalloc(u_tmp->len + USB_LEADING_TAG_OFFSET, GFP_KERNEL);
	if (!tx_buf)
	{
		return -ENOMEM;
	}
//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	
	tx_buf[0] = 0x45;
	tx_buf[1] = 0x47;
	tx_buf[2] = 0x49;
	tx_buf[3] = 0x53;
	tx_buf[4] = 0x09;
	tx_buf[5] = JADE_REGISTER_MASSREAD;
	tx_buf[6] = u_tmp->len;

	retval = copy_from_user(tx_buf + USB_LEADING_TAG_OFFSET, 
								(const u8 __user *)(uintptr_t) u_tmp->tx_buf,
								u_tmp->len);
	if (retval)
	{
		kfree(tx_buf);
		return -ENOMEM;
	}
	pr_debug("u_tmp->len = %d\n", u_tmp->len);


	//_____Step2. Write Addrs
	retval = Egis_BulkXfer(
				dev,
	  			dir_out,
       			(void*)tx_buf,
       			u_tmp->len + USB_LEADING_TAG_OFFSET,
       			&bytes_read, 
				5000
			 );
	if (retval)
	{
		pr_debug("%s:%d  Write Address Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
		pr_debug("%s:%d  Write Address Success length = %d!!\n", __FUNCTION__, __LINE__, bytes_read);

	//_____Step 3. Read Data
	bytes_read = 0;
	memset(tx_buf, 0, u_tmp->len + USB_LEADING_TAG_OFFSET);
	retval = Egis_BulkXfer(
				dev,
	  			dir_in,
       			(void*)tx_buf,
       			u_tmp->len + USB_LEADING_TAG_OFFSET - 1,
       			&bytes_read, 
				5000
			 );

	if (retval)
	{
		pr_debug("%s:%d  Read Data Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
	{
		pr_debug("%s:%d  Read Data Success length = %d!!\n",  __FUNCTION__, __LINE__, bytes_read);
		if (es603_response_verify(tx_buf))
		{
			pr_debug("%s:%d  es603_response_verify Faile!!!!\n", __FUNCTION__, __LINE__);
			retval = -EPROTO;
			goto done;
		}
		else
		{
//  		for (i = 0; i < bytes_read; i++)
//  		{
//  			pr_debug("Data = %2x ",tx_buf[i]);
//  		}
//  		pr_debug("\n");
			uiRet = copy_to_user((u8 __user *)(uintptr_t) u_tmp->rx_buf, tx_buf + (USB_LEADING_TAG_OFFSET - 1), u_tmp->len);
		}
	}
	//_____Step4. if send successfully, copy read data to user.
	
done:
	if(tx_buf)
    	kfree(tx_buf);

	return retval;
}

int ES603_IO_Bulk_Write(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers)
{
	int retval;
	int bytes_read = 0;
	struct egis_ioc_transfer *u_tmp;
	u8	*tx_buf = NULL;

 	pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);

	u_tmp = u_xfers;

//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	tx_buf =  kmalloc(u_tmp->len + USB_LEADING_TAG_OFFSET, GFP_KERNEL);
	if (!tx_buf)
	{
		return -ENOMEM;
	}
//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	
	tx_buf[0] = 0x45;
	tx_buf[1] = 0x47;
	tx_buf[2] = 0x49;
	tx_buf[3] = 0x53;
	tx_buf[4] = 0x09;
	tx_buf[5] = JADE_REGISTER_MASSWRITE;
	tx_buf[6] = u_tmp->len / 2;

	retval = copy_from_user(tx_buf + USB_LEADING_TAG_OFFSET, 
								(const u8 __user *)(uintptr_t) u_tmp->tx_buf,
								u_tmp->len);
	if (retval)
	{
		kfree(tx_buf);
		return -ENOMEM;
	}
	pr_debug("u_tmp->len = %d\n", u_tmp->len);
	


	//_____Step2. Write Addrs
	retval = Egis_BulkXfer(
				dev,
	  			dir_out,
       			(void*)tx_buf,
       			u_tmp->len + USB_LEADING_TAG_OFFSET,
       			&bytes_read, 
				5000
			 );
	if (retval)
	{
		pr_debug("%s:%d  Write Address Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
		pr_debug("%s:%d  Write Address Success length = %d!!\n", __FUNCTION__, __LINE__, bytes_read);

	//_____Step 3. Read Data
	bytes_read = 0;
	memset(tx_buf, 0, u_tmp->len + USB_LEADING_TAG_OFFSET);
	retval = Egis_BulkXfer(
				dev,
	  			dir_in,
       			(void*)tx_buf,
       			USB_LEADING_TAG_OFFSET - 1,
       			&bytes_read, 
				5000
			 );

	if (retval)
	{
		pr_debug("%s:%d  Read Data Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
	{
		pr_debug("%s:%d  Read Data Success length = %d!!\n",  __FUNCTION__, __LINE__, bytes_read);
		if (es603_response_verify(tx_buf))
		{
			pr_debug("%s:%d  es603_response_verify Faile!!!!\n", __FUNCTION__, __LINE__);
			retval = -EPROTO;
			goto done;
		}
	}
	
done:
	if(tx_buf)
    	kfree(tx_buf);

	return retval;
}

int es603_io_bulk_get_image(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers)
{
	int retval;
	int bytes_read = 0, image_size = 0;
	struct egis_ioc_transfer *u_tmp;
	u8	*tx_buf = NULL, *rx_buf = NULL;
	unsigned int uiRet = 0;

 	pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);

	u_tmp = u_xfers;
//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	tx_buf =  kmalloc(u_tmp->len + USB_LEADING_TAG_OFFSET_IMAGE, GFP_KERNEL);
	if (!tx_buf)
	{
		return -ENOMEM;
	}
//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	
	tx_buf[0] = 0x45;
	tx_buf[1] = 0x47;
	tx_buf[2] = 0x49;
	tx_buf[3] = 0x53;
	tx_buf[4] = 0x09;
	tx_buf[5] = JADE_GET_ONE_IMG;

	retval = copy_from_user(tx_buf + USB_LEADING_TAG_OFFSET_IMAGE, 
								(const u8 __user *)(uintptr_t) u_tmp->tx_buf,
								u_tmp->len);
	if (retval)
	{
		kfree(tx_buf);
		tx_buf = NULL;
		retval =  -ENOMEM;
		goto done;
	}
	image_size = tx_buf[USB_LEADING_TAG_OFFSET_IMAGE + 1] * 2 * tx_buf[USB_LEADING_TAG_OFFSET_IMAGE];
	pr_debug("u_tmp->len = %d image_size = %d\n", u_tmp->len, image_size);


	//_____Step2. Write Addrs
	retval = Egis_BulkXfer(
				dev,
	  			dir_out,
       			(void*)tx_buf,
       			u_tmp->len + USB_LEADING_TAG_OFFSET_IMAGE,
       			&bytes_read, 
				5000
			 );
	if (retval)
	{
		pr_debug("%s:%d  Write Address Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
		pr_debug("%s:%d  Write Address Success length = %d!!\n", __FUNCTION__, __LINE__, bytes_read);

	//_____Step 3. Read Data
	bytes_read = 0;
	kfree(tx_buf);
	tx_buf = NULL;
	rx_buf = kmalloc(image_size, GFP_KERNEL);
	if (!rx_buf)
	{
		pr_debug("%s:%d  rx_buf alloc fail!!\n", __FUNCTION__, __LINE__);
		retval =  -ENOMEM;
		goto done;
	}

//  memset(rx_buf, 0, u_tmp->len + USB_LEADING_TAG_OFFSET_IMAGE);
	retval = Egis_BulkXfer(
				dev,
	  			dir_in,
       			(void*)rx_buf,
       			image_size,
       			&bytes_read, 
				5000
			 );
	
	if (retval)
	{
		pr_debug("%s:%d  Read Data Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
	{
		pr_debug("%s:%d  Read Data Success length = %d!!\n",  __FUNCTION__, __LINE__, bytes_read);
//  	if (!es603_response_verify(rx_buf))
//  	{
//  		pr_debug("%s:%d  es603_response_verify Faile!!!!\n", __FUNCTION__, __LINE__);
//  		retval = -EPROTO;
//  		goto done;
//  	}
//  	else
//  	{
//  		for (i = 0; i < bytes_read; i++)
//  		{
//  			if (i && !(i % 32))
//  			{
//  				pr_debug("\n");
//  			}
//  			pr_debug("Data = %2x ",rx_buf[i]);
//  		}
//  		pr_debug("\n");
			pr_debug("%s copy_to_user Start!!\n",  __FUNCTION__);
			uiRet = __copy_to_user((u8 __user *)(uintptr_t) u_tmp->rx_buf, rx_buf, image_size);
			pr_debug("%s copy_to_user Finish!!\n", __FUNCTION__);
//  	}
	}
	//_____Step4. if send successfully, copy read data to user.
	
done:
	if(tx_buf)
    	kfree(tx_buf);
	if (rx_buf)
	{
		kfree(rx_buf);
	}

	return retval;
}

int es603_io_bulk_get_full_image(usb_ss801u *dev, struct egis_ioc_transfer *u_xfers, unsigned n_xfers)
{
	int retval;
	int bytes_read = 0, image_size = 0;
	struct egis_ioc_transfer *u_tmp;
	u8	*tx_buf = NULL, *rx_buf = NULL;
	unsigned int uiRet = 0;

 	pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);

	u_tmp = u_xfers;
//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	tx_buf =  kmalloc(u_tmp->len + USB_LEADING_TAG_OFFSET_IMAGE, GFP_KERNEL);
	if (!tx_buf)
	{
		return -ENOMEM;
	}
//  pr_debug("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
	
	tx_buf[0] = 0x45;
	tx_buf[1] = 0x47;
	tx_buf[2] = 0x49;
	tx_buf[3] = 0x53;
	tx_buf[4] = 0x09;
	tx_buf[5] = JADE_GET_FULL_IMAGE2;

	retval = copy_from_user(tx_buf + USB_LEADING_TAG_OFFSET_IMAGE, 
								(const u8 __user *)(uintptr_t) u_tmp->tx_buf,
								u_tmp->len);
	if (retval)
	{
		kfree(tx_buf);
		tx_buf = NULL;
		return -ENOMEM;
	}
	
	image_size = 256 / 2 * ((tx_buf[USB_LEADING_TAG_OFFSET_IMAGE] << 8) + tx_buf[USB_LEADING_TAG_OFFSET_IMAGE + 1]);
//  info("image_size = %d direction = %d padding = %d timeout = %d", image_size, tx_buf[8], tx_buf[9], tx_buf[10]);
	pr_debug("u_tmp->len = %d image_size = %d\n", u_tmp->len, image_size);

	//Wayne for test
	tx_buf[8] = 2;
	tx_buf[9] = 1;
	tx_buf[10] = 120;
	


	//_____Step2. Write Addrs
	retval = Egis_BulkXfer(
				dev,
	  			dir_out,
       			(void*)tx_buf,
       			u_tmp->len + USB_LEADING_TAG_OFFSET_IMAGE,
       			&bytes_read, 
				5000
			 );
	if (retval)
	{
		pr_debug("%s:%d  Write Address Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
		pr_debug("%s:%d  Write Address Success length = %d!!\n", __FUNCTION__, __LINE__, bytes_read);

	//_____Step 3. Read Data
	bytes_read = 0;
	kfree(tx_buf);
	tx_buf = NULL;
	rx_buf = kmalloc(image_size, GFP_KERNEL);

//  memset(rx_buf, 0, u_tmp->len + USB_LEADING_TAG_OFFSET_IMAGE);
	retval = Egis_BulkXfer(
				dev,
	  			dir_in,
       			(void*)rx_buf,
       			image_size,
       			&bytes_read, 
				5000
			 );
	
	if (retval)
	{
		pr_debug("%s:%d  Read Data Fail!!\n", __FUNCTION__, __LINE__);
		goto done;
	}
	else
	{
		pr_debug("%s:%d  Read Data Success length = %d!!\n",  __FUNCTION__, __LINE__, bytes_read);
//  	if (!es603_response_verify(rx_buf))
//  	{
//  		pr_debug("%s:%d  es603_response_verify Faile!!!!\n", __FUNCTION__, __LINE__);
//  		retval = -EPROTO;
//  		goto done;
//  	}
//  	else
//  	{
//  		for (i = 0; i < bytes_read; i++)
//  		{
//  			if (i && !(i % 32))
//  			{
//  				pr_debug("\n");
//  			}
//  			pr_debug("Data = %2x ",rx_buf[i]);
//  		}
//  		pr_debug("\n");
			pr_debug("%s copy_to_user Start!!\n", __FUNCTION__);
			uiRet = __copy_to_user((u8 __user *)(uintptr_t) u_tmp->rx_buf, rx_buf, image_size);
			pr_debug("%s copy_to_user Finish!!\n", __FUNCTION__);
//  	}
	}
	//_____Step4. if send successfully, copy read data to user.
	
done:
	if(tx_buf)
    	kfree(tx_buf);
	if (rx_buf)
	{
		kfree(rx_buf);
	}

	return retval;
}
/*===========================For ES603==============================*/

int Egis_IO_Bulk_Read(usb_ss801u *dev, int arg)
{
	int retval;
	BULK_BUF Bulk_buffer;
	int bytes_read;
	unsigned char dataBuf[_MaxBulkRead];
	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=ioctl=EGIS_IOCTL_BULK_READ \r\n");
   	//_____Step1. get data from arg
   	//-----1.1 get  data
   	if(copy_from_user(&Bulk_buffer, (void*)arg, sizeof(BULK_BUF))){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ copy scsi from user fail\r\n");
		retval = -EFAULT; return retval;
   	}

	//-----1.2 allocate and initial data buffer
   	Bulk_buffer.Data = kzalloc(Bulk_buffer.DataSize, GFP_KERNEL);
    if (!Bulk_buffer.Data) {
   		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ Out of memory\r\n");
		retval = -EFAULT; return retval;
   	}

	//_____Step2. Bulk transfer
	retval = Egis_BulkXfer(
				dev,
	  			dir_in,
       			(void*)Bulk_buffer.Data,
       			Bulk_buffer.DataSize,
       			&bytes_read, 
				Bulk_buffer.Timeout
			 );

	//_____Step3. if send successfully, copy read data to user.
	
	
   	if(EGIS_SUCCESS(retval)){
		retval = copy_to_user(((void*)((BULK_BUF*)arg)->Data), 
						Bulk_buffer.Data, 
						Bulk_buffer.DataSize);
		if(!access_ok(VERIFY_WRITE, ((BULK_BUF*)arg)->Data, Bulk_buffer.DataSize))
		{
			EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ the buffer is invalid!\r\n");
		}
		else
		{
			EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ the buffer is OK to write!\r\n");
		}
		if(0 != retval)
		{
			if(_MaxBulkRead > Bulk_buffer.DataSize) memcpy(dataBuf, Bulk_buffer.Data, Bulk_buffer.DataSize);
			else EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ data too large\r\n");
			retval = copy_to_user(((BULK_BUF*)arg)->Data,dataBuf,Bulk_buffer.DataSize);
		}
    	if(0 != retval)
		{
			//----- if copy fail, free data buffer
		    EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ copy data to user fail\r\n");
			EgisMsg(dev->bPrintDbgMsg, KERN_ERR, 
				"=ioctl=EGIS_IOCTL_BULK_READ %u bytes can't be copied, data size is %u\r\n", retval, Bulk_buffer.DataSize);
		    kfree(Bulk_buffer.Data); 				    
			Bulk_buffer.DataSize = 0;
			Bulk_buffer.Data = (unsigned char*)0;
			retval = -EFAULT; //break;
    	}
		else{
			Bulk_buffer.DataSize = bytes_read;
		}
    }
   	else{
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ bulk xfer fail %d -> reset\r\n", retval);
		Bulk_buffer.DataSize = 0;
		Bulk_buffer.Data = (unsigned char*)0;
   	}
    if(copy_to_user((void*)arg, &Bulk_buffer, sizeof(BULK_BUF))){
	    EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_READ copy Bulk_buffer fail\r\n");
    }

	if(Bulk_buffer.Data)
    	kfree(Bulk_buffer.Data);
	return retval;
}

int Egis_IO_Bulk_Write(usb_ss801u *dev, int arg)
{
	int retval;
	BULK_BUF Bulk_buffer;
	int bytes_write;
	EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_WRITE \r\n");			

	//_____Step1. get data from arg
   	//-----1.1 get  data
   	if(copy_from_user(&Bulk_buffer, (void*)arg, sizeof(BULK_BUF))){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_WRITE copy scsi from user fail\r\n");
		retval = -EFAULT; return retval;
   	}
	    	
   	//-----1.2 check data allocation is needed
   	if(0 < Bulk_buffer.DataSize){
		//----- allocate, initial data buffer
   		Bulk_buffer.Data = kzalloc(Bulk_buffer.DataSize, GFP_KERNEL);
    	if (!Bulk_buffer.Data) {
   			EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_WRITE Out of memory\r\n");
			retval = -EFAULT; return retval;
   		}
		//----- get transfer data
		if(copy_from_user(Bulk_buffer.Data, 
						  ((BULK_BUF*)arg)->Data, 
						  ((BULK_BUF*)arg)->DataSize))
		{
    		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=SS801U_SCSI_WRITE copy data from user fail\r\n");
   	    	retval = EFAULT; return retval;
   		}
   	}
   	else{
		Bulk_buffer.Data = (char*) 0;
		Bulk_buffer.DataSize = 0;
   	}


   	//_____Step2. bulk transfer out
	retval = Egis_BulkXfer(
				dev,
	  			dir_out,
       			(void*)Bulk_buffer.Data,
       			Bulk_buffer.DataSize,
       			&bytes_write, 
				Bulk_buffer.Timeout
			 );


   	//_____Step3. if send successfully, copy CSW
   	if(EGIS_SUCCESS(retval)){
		Bulk_buffer.DataSize = bytes_write;
   	}
   	else{
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_WRITE bulk xfer fail %d -> reset\r\n", retval);
		Bulk_buffer.DataSize = 0;
   	}

	//_____Step4. Copy actual write size to user
	if(copy_to_user( &((BULK_BUF*)arg)->DataSize, 
						&Bulk_buffer.DataSize, 
						sizeof(Bulk_buffer.DataSize)))
	{
    	EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_BULK_WRITE copy written dataszie fail\r\n");
    	//break;
   	}

   	if(Bulk_buffer.Data){
   		kfree(Bulk_buffer.Data);
   	}	    
	return retval;
}


int Egis_IO_CtrlXfer(usb_ss801u *dev, int arg, CtrlXfer_DIR CtrlDir)
{
	int retval;
	CTRLXFER_PACKAGE Ctrl_Package;
	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "=ioctl=EGIS_IOCTL_CTRLXFER %d\r\n", (int)CtrlDir);
	
	//_____Step1. get data from arg
   	//-----1.1 get  data
   	if(copy_from_user((void*)&Ctrl_Package, (void*)arg, sizeof(CTRLXFER_PACKAGE))){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_CTRLXFER arg data from user fail\r\n");
		retval = -EFAULT; return retval;
   	}
	EgisMsg(dev->bPrintDbgMsg, KERN_INFO, "Setup packet %2x %2x %2x %2x %2x\r\n", 
				Ctrl_Package.SetupPacket.bRequest, Ctrl_Package.SetupPacket.bRequestType,
				Ctrl_Package.SetupPacket.wValue  , Ctrl_Package.SetupPacket.wIndex, 
				Ctrl_Package.SetupPacket.wLength);

	//____Step2. Ctrl xfer in
	retval = usb_control_msg(
				dev->udev,
				(Ctrl_IN == CtrlDir)?
					usb_rcvctrlpipe(dev->udev, 0) : 
					usb_sndctrlpipe(dev->udev, 0),
				Ctrl_Package.SetupPacket.bRequest, 
				Ctrl_Package.SetupPacket.bRequestType, 
				Ctrl_Package.SetupPacket.wValue, 					
				Ctrl_Package.SetupPacket.wIndex, 
				Ctrl_Package.DataPacket,	
				Ctrl_Package.SetupPacket.wLength, 
				Ctrl_Package.Timeout
			);


	//____Step3. copy data to user
   	if( (0<retval) && (Ctrl_IN == CtrlDir) ){
    	if(copy_to_user( ((CTRLXFER_PACKAGE*)arg)->DataPacket, 
						 Ctrl_Package.DataPacket, 
						 Ctrl_Package.SetupPacket.wLength) )
		{
			//----- if copy fail, free data buffer
		    EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_CTRLXFER copy data to user fail\r\n");		    
			Ctrl_Package.SetupPacket.wLength = 0;
			retval = -EFAULT; //break;
    	}
    }
   	else if(EGIS_FAIL(retval)){
		EgisMsg(dev->bPrintDbgMsg, KERN_ERR, "=ioctl=EGIS_IOCTL_CTRLXFER ctrl xfer fail %d -> reset\r\n", retval);
		Egis_Reset_device(dev);
   	}
	return retval;
}

