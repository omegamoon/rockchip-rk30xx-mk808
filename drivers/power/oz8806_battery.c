/*
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <linux/power_supply.h>
#include <mach/board.h>

#include <linux/string.h>
#include <asm/irq.h>


//you must add this code here for O2MICRO
#include "parameter.h"


#define VERSION						"2012.8.6/2.00.01"	
	


struct OZ8806_data {
	struct power_supply bat;
	struct power_supply ac;
	struct delayed_work work;
	struct work_struct dcwakeup_work;
	unsigned int interval;
	unsigned int dc_det_pin;

	struct i2c_client	*myclient;
	struct mutex		update_lock;

	u32					valid;
	unsigned long		last_updated;
	u8					control;
	u16					aout16;
};

static struct OZ8806_data *the_OZ8806;

static enum power_supply_property oz8806_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_HEALTH,
	//POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	//POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG,
	//POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
};

static enum power_supply_property oz8806_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};



/*-------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------*/

static int OZ8806_detect(struct i2c_client *client, int kind,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WRITE_BYTE_DATA
				     | I2C_FUNC_SMBUS_READ_BYTE))
		return -ENODEV;

	strlcpy(info->type, MYDRIVER, I2C_NAME_SIZE);

	return 0;
}

static int oz8806_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	int ret = 0;
	
	struct OZ8806_data *data = container_of(psy, struct OZ8806_data, bat);
	switch (psp) {
	
	case POWER_SUPPLY_PROP_STATUS:
		if(gpio_get_value(data->dc_det_pin))
			val->intval = 3; //0;	/*discharging*/
		else
			val->intval = 1;	/*charging*/
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = batt_info.fVolt;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;//batt_info.fRSOC<0 ? 0:1;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = batt_info.fCurr;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = batt_info.fRSOC;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;	
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static int oz8806_ac_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
	int ret = 0;
	struct OZ8806_data *data = container_of(psy, struct OZ8806_data, ac);
	
      if( parameter_customer.config->debug )
	      printk("--------------------->> batt_info.fRSOC=%d\n",batt_info.fRSOC);
	      
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS){
			if( gpio_get_value(data->dc_det_pin) ||  batt_info.fRSOC >= 100 )//wh,充电满后不再显示充电图标
				val->intval = 0;	/*discharging*/
			else
				val->intval = 1;	/*charging*/
		}
		break;
		
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static void oz8806_powersupply_init(struct OZ8806_data *data)
{
	data->bat.name = "battery";
	data->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	data->bat.properties = oz8806_battery_props;
	data->bat.num_properties = ARRAY_SIZE(oz8806_battery_props);
	data->bat.get_property = oz8806_battery_get_property;
	
	data->ac.name = "ac";
	data->ac.type = POWER_SUPPLY_TYPE_MAINS;
	data->ac.properties = oz8806_ac_props;
	data->ac.num_properties = ARRAY_SIZE(oz8806_ac_props);
	data->ac.get_property = oz8806_ac_get_property;
}

static void oz8806_battery_work(struct work_struct *work)
{
	struct OZ8806_data *data = container_of(work, struct OZ8806_data, work.work); 
	
	//you must add this code here for O2MICRO
	mutex_lock(&data->update_lock);
	bmu_polling_loop();
	mutex_unlock(&data->update_lock);
	
	
	if((gpio_get_value(data->dc_det_pin) == 0) && (batt_info.fRSOC <= 0))
		 batt_info.fRSOC = 1;
	power_supply_changed(&data->bat);
	/* reschedule for the next time */
	schedule_delayed_work(&data->work, data->interval);
}

static irqreturn_t OZ8806_dc_wakeup(int irq, void *dev_id)
{   
    schedule_work(&the_OZ8806->dcwakeup_work);
    return IRQ_HANDLED;
}

static void OZ8806_dcdet_delaywork(struct work_struct *work)
{
    int ret;

	struct OZ8806_data *data = container_of(work, struct OZ8806_data, dcwakeup_work); 
    
	int irq      = gpio_to_irq(data->dc_det_pin);
    int irq_flag = gpio_get_value (data->dc_det_pin) ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;
    
    rk28_send_wakeup_key();
    
    free_irq(irq, NULL);
    ret = request_irq(irq, OZ8806_dc_wakeup, irq_flag, "oz8806_dc_det", NULL);
	if (ret) {
		free_irq(irq, NULL);
	}
	
	power_supply_changed(&data->bat);
}

static int OZ8806_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret, irq, irq_flag;
	struct OZ8806_data *data;

	if (!(data = kzalloc(sizeof(struct OZ8806_data), GFP_KERNEL)))
		return -ENOMEM;

	//Note that mainboard definition file, ex: arch/arm/mach-msm/board-xxx.c, must has declared
	// static struct i2c_board_info xxx_i2c_devs[] __initdata = {....}
	// and it must add including this "I2C_BOARD_INFO("OZ8806", 0x2F)," 
	// otherwise, probe will occur error
	// string is matching with definition in OZ8806_id id table

	// Init real i2c_client 
	i2c_set_clientdata(client, data);

	the_OZ8806 = data;
	data->myclient = client;
	data->interval = msecs_to_jiffies(4 * 1000);
	data->dc_det_pin = RK30_PIN6_PA5;

	if (data->dc_det_pin != INVALID_GPIO)
	{
		ret = gpio_request(data->dc_det_pin, "oz8806_dc_det");
		if (ret != 0) {
			gpio_free(data->dc_det_pin);
			printk("fail to request dc_det_pin\n");
			return -EIO;
		}

		INIT_WORK(&data->dcwakeup_work, OZ8806_dcdet_delaywork);
		irq = gpio_to_irq(data->dc_det_pin);
	        
		irq_flag = gpio_get_value (data->dc_det_pin) ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;
	    	ret = request_irq(irq, OZ8806_dc_wakeup, irq_flag, "oz8806_dc_det", NULL);
	    	if (ret) {
	    		printk("failed to request dc det irq\n");
				return -EIO;
	    	}
	    	enable_irq_wake(irq);	
	}

	mutex_init(&data->update_lock);

	INIT_DELAYED_WORK(&data->work, oz8806_battery_work);

	// Init OZ8806 chip
	//you must add this code here for O2MICRO
	bmu_init_parameter(client);
	bmu_init_chip(&parameter_customer);


	printk("AAAAAAAAAAAAAAAAAAAA CUSTOMER SAMPLE OZ8806 POWER MANAGEMENT DRIVER VERSION is %s\n",VERSION);
	oz8806_powersupply_init(data);
	ret = power_supply_register(&client->dev, &the_OZ8806->bat);
	if (ret) {
		printk(KERN_ERR "failed to register battery\n");
		return ret;
	}
	ret = power_supply_register(&client->dev, &the_OZ8806->ac);
	if (ret) {
		printk(KERN_ERR "failed to register ac\n");
		return ret;
	}

	printk("%s %d",__FUNCTION__,__LINE__);
	schedule_delayed_work(&data->work, data->interval);
	return 0;					//return Ok
}

static int OZ8806_remove(struct i2c_client *client)
{
	struct OZ8806_data *data = i2c_get_clientdata(client);
	//printk("yyyyyyyyyyyyyyyyyyyyyyyyyyyy OZ8806 remove");
	mutex_lock(&data->update_lock);
	bmu_power_down_chip();
	mutex_unlock(&data->update_lock);
	power_supply_unregister(&data->bat);
	power_supply_unregister(&data->ac);
	cancel_delayed_work(&data->work);
	gpio_free(data->dc_det_pin);
	kfree(data);

	return 0;
}

static int OZ8806_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct OZ8806_data *data = i2c_get_clientdata(client);
	//printk("yyyyyyyyyyyyyyyyyyyyyyyyyyyy OZ8806 suspend");
	cancel_delayed_work(&data->work);
	mutex_lock(&data->update_lock);
	mutex_unlock(&data->update_lock);
	return 0;
}

static int OZ8806_resume(struct i2c_client *client)
{
	struct OZ8806_data *data = i2c_get_clientdata(client);
	u16			ADCValue;
	int				tmpfRSOC;
	int ret = 0;
	//printk("yyyyyyyyyyyyyyyyyyyyyyyyyyyy OZ8806 resume");
	//you must add this code here for O2MICRO
	mutex_lock(&data->update_lock);
	bmu_wake_up_chip();
	mutex_unlock(&data->update_lock);

	schedule_delayed_work(&data->work, data->interval);
	return 0;
}


/*-------------------------------------------------------------------------*/

static const struct i2c_device_id OZ8806_id[] = {
	{ MYDRIVER, 0 },							//string, id??
	{ }
};
MODULE_DEVICE_TABLE(i2c, OZ8806_id);

static struct i2c_driver OZ8806_driver = {
	.driver = {
		.name	= MYDRIVER,
	},
	.probe			= OZ8806_probe,
	.remove			= OZ8806_remove,
	.resume			= OZ8806_resume,
	.id_table		= OZ8806_id,

	//auto-detection function
	//.class			= I2C_CLASS_HWMON,			// Nearest choice
	//.detect			= OZ8806_detect,
	//.address_data	= &addr_data,
};

/*-------------------------------------------------------------------------*/

static int __init OZ8806_init(void)
{
	return i2c_add_driver(&OZ8806_driver);
}

static void __exit OZ8806_exit(void)
{
	i2c_del_driver(&OZ8806_driver);
}

/*-------------------------------------------------------------------------*/

#define	DRIVER_VERSION	"24 June 2009"
#define	DRIVER_NAME	(OZ8806_driver.driver.name)

MODULE_DESCRIPTION("OZ8806 Battery Monitor IC Driver");
MODULE_LICENSE("O2Micro");

module_init(OZ8806_init);
module_exit(OZ8806_exit);

