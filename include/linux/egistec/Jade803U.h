




// Bit Map
#define BIT0			0x01
#define BIT1			0x02
#define BIT2			0x04
#define BIT3			0x08
#define BIT4			0x10
#define BIT5			0x20
#define BIT6			0x40
#define BIT7			0x80











//******************************************************************************************

//==============================================================
//				Sensor Image Size
//==============================================================
#define PIXELSHIFTPERBYTE         1
#define FRAME1                    1
#define FRAMEBURST                60
#define FRAMESTRIPES              6000
#define COL_NO                    192
#define ROW_NO                    4
#define TOTAL_PIXEL               COL_NO*ROW_NO

//==============================================================
//				Sensor Registers
//==============================================================
#define  FRAME_DATA_ADDR               0x00
#define  SOFT_RESET_ADDR               0x01
#define  MODE_SELECT_ADDR              0x02  
#define  GLOBAL_STATUS_REG_ADDR        0x03
#define  FPS_COLUMN_SELECT_ADDR        0x04
#define  CLOCK_SETTING                 0x05
#define  MVS_FRAMEBUF_CONTROL_ADDR     0x10
#define  NAVIGATION_WEIGHT_REGISTER    0x11
#define  NAVIGATIONVECTOR2             0x12

#define  UP_RECONSTRUCTION_RESULT_1						0x14
#define  DOWN_RECONSTRUCTION_RESULT_1				0x17

#define  FIL_RECON_VALUE_ADDR          0x1A
#define  GAIN_INFORMATION              0x20
#define  GAIN_READ_1_ADDR              0x21 // JACKY_201006010_FOR_TUNE_DC
#define  GAIN_READ_2_ADDR              0x22 // JACKY_201006010_FOR_TUNE_DC
#define  GAIN_READ_3_ADDR              0x23 // JACKY_201006010_FOR_TUNE_DC
#define  VRB_READ_1_ADDR               0x26
#define  VRB_READ_2_ADDR               0x27
#define  VRB_READ_3_ADDR               0x28
#define  DES_KEY_1_ADDR                0x41
#define  CNT_CTRL_1                    0x50
#define  CNT_CTRL_2                    0x51
#define  INTEN_TH_H                    0x54
#define  INTEN_TH_L                    0x55
#define  INTEN_TH2_H                   0x56
#define  INTEN_TH2_L                   0x57
#define  INTEN_FRM_TH                  0x58
#define  PCNT_TH_H_DET                 0x59
#define  PCNT_TH_L_DET                 0x5A
#define  PCNT_FRM_TH_DET               0x5B
#define  PCNT_TH_H                     0x5C
#define  PCNT_TH_L                     0x5D
#define  PCNT_FRM_TH                   0x5E
#define  LEVEL_TH                      0x5F
#define  NO_MV_TH                      0x60
#define  FIRMWARE_VERSION              0x70
#define  OPAMP_GAIN_CONTROL            0xE0
#define  VRT_VOLTAGE_CONTROL           0xE1
#define  VRB_VOLTAGE_CONTROL           0xE2
#define  DET_VRT_VOLTAGE_CONTROL       0xE3 // JACKY_20100812_FOR_DETECTMODE_TUNING
#define  VCO_CONTROL                   0xE5
#define  DC_OFFSET_CANCELING_VOLTAGE   0xE6
#define  ENGINEER_MODE_ENABLE          0xF0
#define  ANALOG_TEST_MODE              0xF2
#define  LED_SETTING_1                 0XF793
#define  LED_SETTING_2                 0XF794



// [0x02] Mode Select
#define SLEEP_MODE                     0x30 // JACKY_20100720_FOR_IJ_LED , Org = 0x10
#define CONTACT_MODE                   0x31 // JACKY_20100720_FOR_IJ_LED , Org = 0x11
#define NAVI_MODE                      0x32 // JACKY_20100720_FOR_IJ_LED , Org = 0x12
#define SENSOR_MODE                    0x33 // JACKY_20100720_FOR_IJ_LED , Org = 0x13
#define SENSOR_AND_DES_MODE            (SENSOR_MODE | BIT3)
#define RECON_MODE                     0x34 // JACKY_20100720_FOR_IJ_LED , Org = 0x14
#define RECON_AND_DES_MODE             (RECON_MODE | BIT3)
//==============================================================
//				InitSensor Define
//==============================================================
#define  JADE_STR                      "JDXX"
#define  SENSOR_JADE_D                 0x0D
#define  SENSOR_JADE_E                 0x0E


#define ERR_TIME_DURATION              10*1000
#define RECONSTRUCTION_TIMEOUT         5*1000
#define FINGER_TOUCH_INTENSITY         0xDF	// JACKY_20100812_FOR_FINGERDETECT_PARAMETER , Org = 0xEF
#define FINGER_LEAVE_INTENSITY         0xD0
#define READYBIT_TIMEOUT               0x64
#define FINGERDETECT_TIMEOUT           15*1000
#define CSVOUTDETECT_TIMEOUT           5*1000


#define INIT_SNR_REG02                 0x30 // JACKY_20100720_FOR_IJ_LED , Org = 0x10
#define INIT_SNR_REG50                 0x0F
#define INIT_SNR_REGE0                 0x04
#define INIT_SNR_REGE1                 0x08
#define INIT_SNR_REGE2                 0x0D
#define INIT_SNR_REGE5                 0x14 // JACKY_20100812_FOR_CORRECT_REGVALUE , VCO = 12M , Org = 0x1C
#define INIT_SNR_REGE6                 0x36
#define INIT_SNR_REGF0                 0x00
#define INIT_SNR_REGF2                 0x00

//==============================================================
//				LED Define
//==============================================================
#define  LED_BREATH_1                  0x40
#define  LED_BREATH_2                  0x0F
#define  LED_BREATH_MASK_1             0x80
#define  LED_BREATH_MASK_2             0x10

//==============================================================
//				Reconstruction Define
//==============================================================
//enum ReconDir
//{
//    UP_RECON   = 1,
//    DOWN_RECON = 2,
//    DUAL_RECON = 3
//};

enum ReconDirNum
{
	SINGLE_DIR = 1,
	DUAL_DIR   = 2,
};

#define SENSOR_SNR_REG_INIT_NUM               9

#define  FULLROWNO                            320
#define  JADE_START_IDX                       0
#define  JADE_NEXT_IDX                        1
#define  JADE_TAG_0                           0x5A
#define  JADE_TAG_1                           0xA5

// MVS/FRMBUF Control Register
#define MV_COLTH                              2
#define MV_ROWTH                              0  
#define RECPAD                                1
#define NEW_REC                               1
// New_Reconstruction_Parameters
#define CNT_BASE                              0
#define BASE_DIV                              1
// New_Reconstruction_Parameters[1:0]
#define CNT_BASE_00                           0
#define CNT_BASE_01                           1
#define CNT_BASE_02                           2
#define CNT_BASE_03                           3
// New_Reconstruction_Parameters[3:2]
#define BASE_DIV_00                           0
#define BASE_DIV_01                           1
#define BASE_DIV_02                           2
#define BASE_DIV_03                           3
// padding reconstruction or non-padding reconstruction
#define RECON_PADDING_ON                      1
#define RECON_PADDING_OFF                     0

//==============================================================
//				Detect Define
//==============================================================
#define FREQUENCY_RESUME                      0x03
#define DEFAULT_CLOCK_SETTING                 0x10			// JACKY_20100812_FOR_CORRECT_REGVALUE , Use Internal Clocker, Org = 0x3F
#define DEFAULT_PCNT_TH_H_DET                 0x18
#define DEFAULT_PCNT_TH_L_DET                 0x08
#define DEFAULT_PCNT_FRM_TH_DET               0x10

//==============================================================
//				DES Define
//==============================================================
#define SENSOR_DES_REG_INIT_NUM               8

//======================================================================
//				DVR Initialize
//======================================================================
#define SENSOR_DVR_REG_INIT_NUM           24
#define VCO_CONTROL_DEFAULT               INIT_SNR_REGE5	// JACKY_20100812_FOR_CORRECT_REGVALUE , VCO = 12M , Org = 0x1C
#define VCO_CONTROL_DETECTMODE            0x13				// JACKY_20100812_FOR_CORRECT_REGVALUE
#define DETECT_VRT_DEFAULT                0x00				// JACKY_20100812_FOR_CORRECT_REGVALUE
#define DC_OFFSET_DEFAULT                 0x2F

// DVR Define
//#define VRB_HWTUNING					  // VRB hardware turning
#define VRT_HWTUNING					  // VRT hardware turning

#define TUNNING_GAINS                     0x00
									      // 00: 1 gain(s) 
										  // 01: 2 gain(s) 
										  // 10: 3 gain(s) 
										  // 11: 4 gain(s) 
#define FRAME                             20
#define SETVRB_DIV_2_THEN_PLUS_6          6
#define VRTTEMP_MUL_2_THEN_MINUS_10       10	// JACKY_20100701_FOR_REGISTER_RECHECK, should be 10(b) not 10(dex)
#define IN_SENSOR_SMALL_GAIN              0x23  // JACKY_20100809_FOR_LIMITE_GAIN , def is 0x43 // x12
#define IN_SENSOR_NORMAL_GAIN             0x21  // JACKY_20100809_FOR_LIMITE_GAIN , def is 0x04 // x5
#define IN_SENSOR_LARGE_GAIN              0x20  // JACKY_20100809_FOR_LIMITE_GAIN , def is 0x05 // x6
#define OUT_SENSORDVR_SMALL_VRB           0x00
#define OUT_SENSORDVR_NORMAL_VRB          0x00
#define OUT_SENSORDVR_LARGE_VRB           0x00
#define FRAMEWIDTH                        COL_NO
#define NLEFTPIXEL_TO_NRIGHTPIXEL         80  //VRB
#define NWIDHTSTARTCOL_TO_NWIDTHENDCOL    80  //VRT
#define RANGEIMGPIXELS                    double(DVR_nLeftPixel_to_nRightPixel*ROW_NO)		//VRB
#define NAREAPIXEL                        double(DVR_nWidthStartCol_to_nWidthEndCol*ROW_NO)	//VRT
//======================================================================
//							VRB Thresholds
//======================================================================
#define DBLACKTHRESHOLD1                  double(0.970)
#define DBLACKTHRESHOLD2                  double(0.010)
#define DWHITETHRESHOLD                   double(0.020)
#define DWHOLEWHITETH                     double(0.020)
#define DBINARYTH                         double(0.200)
#define DBLACKGRAYRATE                    2
										  // 0000: 0  , 0001: 1  , 0010: 2  , 0011: 3
										  // 0100: 4  , 0101: 5  , 0110: 6  , 0111: 7
										  // 1000: 8  , 1001: 9  , 1010: 10 , 1011: 11
										  // 1100: 12 , 1101: 13 , 1110: 14 , 1111: 15
//======================================================================
//							VRT Thresholds
//======================================================================
#define DTH1                              double(0.250)
#define DTH2                              double(0.020)
#define DTH3_SLOPE_HIGH                   2				
#define DTH3_SLOPE_LOW                    1
#define DTH1_1                            double(0.650)  // increasing => darker
#define DTH3_SLOPE_BLACKRATE              double(0.100)  // increasing => darker
#define DTH3_SLOPE_BWRATE                 double(0.750)	 // decreasing => darker
										  // 00: Reserved 
										  // 01: 0.250
										  // 10: 0.500
										  // 11: 0.750