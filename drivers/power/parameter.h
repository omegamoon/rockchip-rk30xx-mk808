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
*	$Revision: 2.00.01 $
*
*****************************************************************************/
#ifndef _PARAMETER_H_
#define _PARAMETER_H_


 /****************************************************************************
  * #include section
  *  add #include here if any
  ***************************************************************************/
  
 /****************************************************************************
  * #define section
  *  add constant #define here if any
  ***************************************************************************/
#define MUTEX_TIMEOUT                         5000
#define MYDRIVER				"oz8806"  //"OZ8806"
#define OZ8806Addr				0x2F


 
/****************************************************************************
* Struct section
*  add struct #define here if any
***************************************************************************/
typedef struct tag_ocv_data{
	int32_t			cell_voltage;
	int32_t			rsoc;				
}ocv_data_t;


//config struct
typedef struct	 tag_config_data {
	int32_t		fRsense;		//= 20;			//Rsense value of chip, in mini ohm
	int32_t		dbCARLSB;		//= 5.0;		//LSB of CAR, comes from spec
	int32_t		dbCurrLSB;		//391 (3.90625*100);	//LSB of Current, comes from spec
	int32_t		fVoltLSB;		//250 (2.5*100);	//LSB of Voltage, comes from spec

	int32_t		design_capacity;	//= 7000;		//design capacity of the battery
 	int32_t		charge_cv_voltage;	//= 4200;		//CV Voltage at fully charged
	int32_t		charge_end_current;	//= 100;		//the current threshold of End of Charged
	int32_t		discharge_end_voltage;	//= 3550;		//mV
	uint8_t         debug;                                          // enable or disable O2MICRO debug information

}config_data_t;



typedef struct	 tag_parameter_data {
	int32_t            	 ocv_data_num;
	ocv_data_t    		*ocv;
	config_data_t 		*config;
		
	struct i2c_client 	*client;
	
	uint8_t 	charge_pursue_step;		
 	uint8_t  	discharge_pursue_step;		
	uint8_t  	discharge_pursue_th;			

}parameter_data_t;


typedef struct tag_bmu {
	int32_t		PowerStatus;	
	int32_t		fRC;			//= 0;		//Remaining Capacity, indicates how many mAhr in battery
	int32_t		fRSOC;			//50 = 50%;	//Relative State Of Charged, present percentage of battery capacity
	int32_t		fVolt;			//= 0;						//Voltage of battery, in mV
	int32_t		fCurr;			//= 0;		//Current of battery, in mA; plus value means charging, minus value means discharging
	int32_t		fPrevCurr;		//= 0;						//last one current reading
	int32_t		fOCVVolt;		//= 0;						//Open Circuit Voltage
	int32_t		fCellTemp;		//= 0;						//Temperature of battery
	int32_t	    	fRCPrev;
	int32_t		sCaMAH;			//= 0;						//adjusted residual capacity				
	int32_t     	i2c_error_times;
}bmu_data_t;



/****************************************************************************
* extern variable declaration section
***************************************************************************/
//extern config_data_t config_data;
//extern ocv_data_t ocv_data[];
extern bmu_data_t batt_info;
extern parameter_data_t parameter_customer;




/****************************************************************************
*  section
*  add function prototype here if any
***************************************************************************/
extern void bmu_init_chip(parameter_data_t *paramter_data);
extern void bmu_wake_up_chip(void);
extern void bmu_power_down_chip(void);
extern void bmu_polling_loop(void);
extern void bmu_init_parameter(struct i2c_client *client);

#endif














