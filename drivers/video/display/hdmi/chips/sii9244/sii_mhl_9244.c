#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <mach/iomux.h>
#include <mach/board.h>

#include <linux/display-sys.h>

#include "sii_mhl_hal.h"
#include "sii_mhl_defs.h"
#include "sii_mhl_tx_api.h"
#include "sii_mhl_tx_base_drv_api.h"
#include "sii_mhl_9244.h"

struct sii9244 *sii9244 = NULL;

static const struct i2c_device_id mhl_sii_id[] = {
	{ "sii9244", 0 },	//0x72
	{ "sii9244_power", 0 },//0x7A
	{ "sii9244_signal", 0 },//0x92
	{ "sii9244_cbus", 0 },//0xC8
	{ }
};

#define	APP_DEMO_RCP_SEND_KEY_CODE 0x44  /* play */

static int16_t rcpKeyCode=-1;
static int16_t rcpkParam = -1;

///////////////////////////////////////////////////////////////////////////////
//
// AppNotifyMhlDownStreamHPDStatusChange
//
//  This function is invoked from the MhlTx component to notify the application about
//  changes to the Downstream HPD state of the MHL subsystem.
//
// Application module must provide this function.
//
void  AppNotifyMhlDownStreamHPDStatusChange(bool_t connected)
{
    // this need only be a placeholder for 9244
//    SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS,"App:%d AppNotifyMhlDownStreamHPDStatusChange connected:%s\n",(int)__LINE__,connected?"yes":"no");
}
///////////////////////////////////////////////////////////////////////////////
//
// AppNotifyMhlEvent
//
//  This function is invoked from the MhlTx component to notify the application
//  about detected events that may be of interest to it.
//
// Application module must provide this function.
//
MhlTxNotifyEventsStatus_e AppNotifyMhlEvent(uint8_t eventCode, uint8_t eventParam)
{
	MhlTxNotifyEventsStatus_e retVal = MHL_TX_EVENT_STATUS_PASSTHROUGH;
	switch(eventCode)
	{
	case MHL_TX_EVENT_DISCONNECTION:
//		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App: Got event = MHL_TX_EVENT_DISCONNECTION\n");
#ifdef	BYPASS_VBUS_HW_SUPPORT //(
        // turn off VBUS power here
#endif //)
		break;
	case MHL_TX_EVENT_CONNECTION:
//		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App: Got event = MHL_TX_EVENT_CONNECTION\n");
    	SiiMhlTxSetPreferredPixelFormat(MHL_STATUS_CLK_MODE_NORMAL);

#ifdef ENABLE_WRITE_BURST_TEST //(
        testCount=0;
        testStart=0;
//        SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS,"App:%d Reset Write Burst test counter\n",(int)__LINE__);
#endif //)
		break;
	case MHL_TX_EVENT_RCP_READY:
		if( (0 == (MHL_FEATURE_RCP_SUPPORT & eventParam)) )
		{
//			SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App:%d Peer does NOT support RCP\n",(int)__LINE__ );
		}
        else
        {
//			SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App:%d Peer supports RCP\n",(int)__LINE__ );
            // Demo RCP key code Volume Up
            rcpKeyCode = APP_DEMO_RCP_SEND_KEY_CODE;

        }
		if( (0 == (MHL_FEATURE_RAP_SUPPORT & eventParam)) )
		{
//			SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App:%d Peer does NOT support RAP\n",(int)__LINE__ );
		}
        else
        {
//			SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App:%d Peer supports RAP\n",(int)__LINE__ );
#ifdef ENABLE_WRITE_BURST_TEST //(
			testEnable = 1;
#endif //)
        }
		if( (0 == (MHL_FEATURE_SP_SUPPORT & eventParam)) )
		{
//			SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App:%d Peer does NOT support WRITE_BURST\n",(int)__LINE__ );
		}
        else
		{
//			SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App:%d Peer supports WRITE_BURST\n",(int)__LINE__ );
		}

		break;
	case MHL_TX_EVENT_RCP_RECEIVED :
        //
        // Check if we got an RCP. Application can perform the operation here
        // and send RCPK or RCPE. For now, we send the RCPK
        //
//        SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App: Received an RCP key code = %02X\n", (int)eventParam );
		rcpkParam = (int16_t) eventParam;
		break;
	case MHL_TX_EVENT_RCPK_RECEIVED:
//		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App: Received an RCPK = %02X\n", (int)eventParam);
#ifdef ENABLE_WRITE_BURST_TEST //(
		if ((APP_DEMO_RCP_SEND_KEY_CODE == eventParam)&& testEnable)
		{
            testStart=1;
//    		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS,"App:%d Write Burst test Starting:...\n",(int)__LINE__);
		}
#endif //)
		break;
	case MHL_TX_EVENT_RCPE_RECEIVED:
//		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App: Received an RCPE = %02X\n", (int)eventParam);
		break;
	case MHL_TX_EVENT_DCAP_CHG:
        {
        	uint8_t i,myData;
//    		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "App: MHL_TX_EVENT_DCAP_CHG: ",myData);
        	for(i=0;i<16;++i)
        	{
    			if (0 == SiiTxGetPeerDevCapEntry(i,&myData))
    			{
//    				SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "0x%02x ",(int)myData);
    			}
    			else
    			{
//    				SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "busy ");
    			}
        	}
//    		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "\n");
        }
		break;
	case MHL_TX_EVENT_DSCR_CHG:
		{
			ScratchPadStatus_e temp;
			uint8_t myData[16];
			temp = SiiGetScratchPadVector(0,sizeof(myData), myData);
			switch(temp)
			{
			case SCRATCHPAD_FAIL:
			case SCRATCHPAD_NOT_SUPPORTED:
			case SCRATCHPAD_BUSY:
//			    SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "SiiGetScratchPadVector returned 0x%02x\n",(int)temp);
				break;
			case SCRATCHPAD_SUCCESS:
				{
					uint8_t i;
//			    	SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "New ScratchPad: ",(int)temp);
					for (i=0;i<sizeof(myData);++i)
					{
//					    SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "(%02x, %c) \n",(int)temp,(char)temp);
					}
//			    	SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "\n",(int)temp);
				}
				break;
			}
		}
		break;
#ifdef BYPASS_VBUS_HW_SUPPORT //(
	case MHL_TX_EVENT_POW_BIT_CHG:
		if (eventParam) // power bit changed
		{
		    // turn OFF power here ;
		}
		retVal = MHL_TX_EVENT_STATUS_HANDLED;
		break;
	case MHL_TX_EVENT_RGND_MHL:

        // for OEM to do:  if sink is NOT supplying VBUS power then turn it on here
		retVal = MHL_TX_EVENT_STATUS_HANDLED;
		break;
#else //)(
	case MHL_TX_EVENT_POW_BIT_CHG:
	case MHL_TX_EVENT_RGND_MHL:
		// let the lower layers handle these.
		break;
#endif //)

	default:
//		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS, "Unknown event: 0x%02x\n",(int)eventCode);
		break;

	}
	return retVal;
}

typedef volatile struct _SiiOsSchedulerCounterPair_t
{
	uint8_t tail;
	uint8_t head;
}SiiOsSchedulerCounterPair_t,*PSiiOsSchedulerCounterPair_t;

static SiiOsSchedulerCounterPair_t g_interruptPair={0,0},g_bumpPair={0,0};


static void SiiOsScheduler(void)
{
	int pinTxInt = gpio_get_value(sii9244->io_irq_pin);
	while((g_interruptPair.tail != g_interruptPair.head)
			||(g_bumpPair.tail != g_bumpPair.head)
			||(0==pinTxInt)
			)
	{
		if (g_interruptPair.tail != g_interruptPair.head)
		{
			g_interruptPair.head++;
//			SII_SCHEDULER_DEBUG_PRINT(("Sch: intr:%3bu sched:%3bu\n",g_interruptPair.tail,g_interruptPair.head));
//			printk("Sch: intr:%3bu sched:%3bu\n",g_interruptPair.tail,g_interruptPair.head);
		}
		if (g_bumpPair.tail != g_bumpPair.head)
		{
			g_bumpPair.head++;
//			SII_SCHEDULER_DEBUG_PRINT(("Sch: bump:%3bu sched:%3bu\n",g_bumpPair.tail,g_bumpPair.head));
//			printk(("Sch: bump:%3bu sched:%3bu\n",g_bumpPair.tail,g_bumpPair.head));
		}
		SiiMhlTxDeviceIsr( );
		pinTxInt = gpio_get_value(sii9244->io_irq_pin);
	}
}

void SiiOsBumpMhlTxScheduler()
{
	g_bumpPair.tail++;
}

static irqreturn_t sii9244_detect_irq(int irq, void *dev_id)
{
	printk("\nsii9244_detect_irq %d\n\n", g_interruptPair.tail);
	g_interruptPair.tail++;
	return IRQ_HANDLED;
}

void SiiOsMhlTxInterruptEnable()
{
	if(sii9244->io_irq_pin != INVALID_GPIO) {
		if(gpio_request(sii9244->io_irq_pin, "mhl gpio") < 0) {
	    	dev_err(&sii9244->client->dev, "fail to request gpio %d\n", sii9244->io_irq_pin);
	    }
	    else {
		    gpio_direction_input(sii9244->io_irq_pin);
	    	sii9244->irq = gpio_to_irq(sii9244->io_irq_pin);
	    	if(request_irq(sii9244->irq, sii9244_detect_irq, IRQF_TRIGGER_FALLING, NULL, sii9244) < 0) {
	        	dev_err(&sii9244->client->dev, "fail to request sii9244 irq\n");
	    	}
    	}
	}
}

static void mhl_work(struct work_struct *work)
{
	int T_Monitor = T_MONITORING_PERIOD;
	while(1) {
		SiiOsScheduler();
		if (!T_Monitor)
		{
			if (rcpKeyCode >= 0)
			{
	            if (!MhlTxCBusBusy())
	            {
	
	//            	SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS,"App:%d Sending RCP (%02X)\n",(int)__LINE__, (int) rcpKeyCode);
	             	//
	             	// If RCP engine is ready, send one code
	             	//
	             	if( SiiMhlTxRcpSend( (uint8_t)rcpKeyCode ))
	             	{
	//             		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS,"App:%d SiiMhlTxRcpSend (%02X)\n",(int)__LINE__, (int) rcpKeyCode);
	             	}
	             	else
	             	{
	//             		SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS,"App:%d SiiMhlTxRcpSend (%02X) Returned Failure.\n",(int)__LINE__, (int) rcpKeyCode);
	             	}
	             	rcpKeyCode = -1;
	            }
			}
	#ifdef ENABLE_WRITE_BURST_TEST
	        else if ( (!MhlTxCBusBusy()) && (testStart) )
	        {
	        #define TEST_COUNT_LIMIT 100
	            if (testCount < TEST_COUNT_LIMIT)
	            {
	            static uint8_t testData[17]="MyDogHasFleas\0\0\0";
	                if (0 == SiiMhlTxRequestWriteBurst(0,sizeof(testData)-1,testData))
	                {
	                    testCount++;
	                }
	            }
	            else
	            {
	                testStart = 0;
	                SiiOsDebugPrintSimple(SII_OSAL_DEBUG_TRACE_ALWAYS,"App:%d Write Burst test complete after %d iterations\n",(int)__LINE__,testCount);
	            }
	        }
	#endif //)
			if (rcpkParam >= 0)
			{
		        SiiMhlTxRcpkSend((uint8_t)rcpkParam);
		        rcpkParam = -1;
			}
			T_Monitor = T_MONITORING_PERIOD;
		}
		else
		{
			T_Monitor--;
		}
		msleep(10);
	}
}

static int sii9244_i2c_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int ret = 0;
	struct rkdisplay_platform_data *mhl_data = client->dev.platform_data;
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk("%s i2c check function err!\n",__func__); 
		return -ENODEV;
	}
	
	if(strcmp(client->name, mhl_sii_id[0].name) == 0) {
		sii9244 = kzalloc(sizeof(struct sii9244), GFP_KERNEL);
		if(!sii9244) {
	        dev_err(&client->dev, "no memory for state\n");
	        goto err_kzalloc_sii9244;
	    }
	    
	    memset(sii9244, 0 ,sizeof(struct sii9244));
		sii9244->client = client;
		
		// Register HDMI device
		if(mhl_data) {
			sii9244->io_pwr_pin = mhl_data->io_pwr_pin;
			sii9244->io_rst_pin = mhl_data->io_reset_pin;
		}
		else {
			sii9244->io_pwr_pin = INVALID_GPIO;
			sii9244->io_rst_pin = INVALID_GPIO;	
		}
		
		//Power on sii902x
		if(sii9244->io_pwr_pin != INVALID_GPIO) {
			ret = gpio_request(sii9244->io_pwr_pin, NULL);
			if(ret) {
				gpio_free(sii9244->io_pwr_pin);
				sii9244->io_pwr_pin = INVALID_GPIO;
	        	printk(KERN_ERR "request sii9244 power control gpio error\n ");
	        	goto err_request_gpio; 
			}
			else
				gpio_direction_output(sii9244->io_pwr_pin, GPIO_HIGH);
		}
		
		if(sii9244->io_rst_pin != INVALID_GPIO) {
			ret = gpio_request(sii9244->io_rst_pin, NULL);
			if(ret) {
				gpio_free(sii9244->io_rst_pin);
				sii9244->io_rst_pin = INVALID_GPIO;
	        	printk(KERN_ERR "request sii9244 reset control gpio error\n ");
	        	goto err_request_gpio; 
			}
			else
				gpio_direction_output(sii9244->io_rst_pin, GPIO_HIGH);
		}
		sii9244->io_irq_pin = client->irq;
		sii9244->workqueue = create_singlethread_workqueue("mhl");
		INIT_DELAYED_WORK(&(sii9244->delay_work), mhl_work);
		
		//
		// Initialize the registers as required. Setup firmware vars.
		//
		SiiMhlTxInitialize( MONITORING_PERIOD );
		
		queue_delayed_work(sii9244->workqueue, &sii9244->delay_work, 0);
			
		dev_info(&client->dev, "sii9244 probe ok\n");
	}
	
	return 0;
	
err_request_gpio:
	if(sii9244->io_rst_pin != INVALID_GPIO)
		gpio_free(sii9244->io_rst_pin);
	if(sii9244->io_pwr_pin != INVALID_GPIO)
		gpio_free(sii9244->io_pwr_pin);
	kfree(sii9244);
	sii9244 = NULL;
err_kzalloc_sii9244:
	dev_err(&client->dev, "sii9244 probe error\n");
	return ret;
}

static int __devexit sii9244_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver sii9244_i2c_driver  = {
    .driver = {
        .name  = "sii9244",
        .owner = THIS_MODULE,
    },
    .probe 		= &sii9244_i2c_probe,
    .remove     = &sii9244_i2c_remove,
    .id_table	= mhl_sii_id,
};

static int __init sii9244_init(void)
{
    return i2c_add_driver(&sii9244_i2c_driver);
}

static void __exit sii9244_exit(void)
{
    i2c_del_driver(&sii9244_i2c_driver);
}

module_init(sii9244_init);
module_exit(sii9244_exit);