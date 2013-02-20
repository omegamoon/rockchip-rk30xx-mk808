/*****************************************************************************
*
*	Copyright(c) O2Micro, 2012. All rights reserved.
*	
*	Description:
*		
*   Qualification Level: 
*		Sample Code Release (Sample Code Release | Production Release)
*   
*	Release Purpose: 
*		Reference design for OZ8806 access
*   
*	Release Target: O2MICRO Customer
*   Re-distribution Allowed: 
*				No (Yes, under agreement | No)
*   Author: Eason.yuan
*	$Source: /data/code/CVS
*	$Revision: 2.00.00 $
*
*****************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>


#include <linux/i2c.h>



#include "parameter.h"

/*****************************************************************************
* Define section
* add all #define here
*****************************************************************************/
#define OCV_DATA_NUM  11



/*****************************************************************************
* Global variables section - Exported
* add declaration of global variables that will be exported here
* e.g.
*	int8_t foo;
****************************************************************************/

ocv_data_t ocv_data[OCV_DATA_NUM] = {
				{3500, 00},{3695, 10},{3746, 20},
				{3777, 30},{3795, 40},{3825, 50}, 
				{3887, 60},{3946, 70},{4020, 80}, 
				{4104, 90},{4180, 100},
};

config_data_t config_data = {22,5,391,250,7200,4160,100,3500,0};

/*
	int32_t		fRsense;		//= 20;			//Rsense value of chip, in mini ohm
	int32_t		dbCARLSB;		//= 5.0;		//LSB of CAR, comes from spec
	int32_t		dbCurrLSB;		//391 (3.90625*100);	//LSB of Current, comes from spec
	int32_t		fVoltLSB;		//250 (2.5*100);	//LSB of Voltage, comes from spec

	int32_t		design_capacity;	//= 7000;		//design capacity of the battery
 	int32_t		charge_cv_voltage;	//= 4200;		//CV Voltage at fully charged
	int32_t		charge_end_current;	//= 100;		//the current threshold of End of Charged
	int32_t		discharge_end_voltage;	//= 3550;		//mV
	uint8_t      	debug;
}
*/
parameter_data_t parameter_customer;

/*****************************************************************************
 * Description:
 *		bmu_init_chip
 * Parameters:
 *		description for each argument, new argument starts at new line
 * Return:
 *		what does this function returned?
 *****************************************************************************/
void bmu_init_parameter(struct i2c_client *client)
{
	parameter_customer.config = &config_data;
	parameter_customer.ocv = &ocv_data;
	parameter_customer.client = client;
	parameter_customer.ocv_data_num = OCV_DATA_NUM;
	parameter_customer.charge_pursue_step = 5;		
 	parameter_customer.discharge_pursue_step = 10;		
	parameter_customer.discharge_pursue_th = 8;
}










