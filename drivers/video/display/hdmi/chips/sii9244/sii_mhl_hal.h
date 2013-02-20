#ifndef __SII_MHL_HAL_H__
#define __SII_MHL_HAL_H__

typedef bool bool_t;

// Direct register access
uint8_t     I2C_ReadByte(uint8_t deviceID, uint8_t offset);
void        I2C_WriteByte(uint8_t deviceID, uint8_t offset, uint8_t value);

uint8_t     ReadBytePage0 (uint8_t Offset);
void        WriteBytePage0 (uint8_t Offset, uint8_t Data);
void        ReadModifyWritePage0 (uint8_t Offset, uint8_t Mask, uint8_t Data);

uint8_t     ReadByteCBUS (uint8_t Offset);
void        WriteByteCBUS (uint8_t Offset, uint8_t Data);
void        ReadModifyWriteCBUS(uint8_t Offset, uint8_t Mask, uint8_t Value);

//
// Slave addresses used in Sii 9244.
//
#define	PAGE_0_0X72			0x72
#define	PAGE_1_0x7A			0x7A
#define	PAGE_2_0x92			0x92
#define	PAGE_CBUS_0XC8		0xC8

//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------
// Timers - Target system uses these timers
#define ELAPSED_TIMER               0xFF
#define ELAPSED_TIMER1              0xFE

typedef enum TimerId
{
    TIMER_FOR_MONITORING= 0,		// HalTimerWait() is implemented using busy waiting
    TIMER_POLLING,		// Reserved for main polling loop
    TIMER_2,			// Available
    TIMER_SWWA_WRITE_STAT,
    TIMER_TO_DO_RSEN_CHK,
    TIMER_TO_DO_RSEN_DEGLITCH,
    TIMER_COUNT			// MUST BE LAST!!!!
} timerId_t;

extern void 	HalTimerInit ( void );
extern void 	HalTimerSet ( uint8_t index, uint16_t m_sec );
extern uint8_t 	HalTimerExpired ( uint8_t index );
extern void		HalTimerWait ( uint16_t m_sec );
extern uint16_t	HalTimerElapsed( uint8_t index );


#define BIT0                    0x01
#define BIT1                    0x02
#define BIT2                    0x04
#define BIT3                    0x08
#define BIT4                    0x10
#define BIT5                    0x20
#define BIT6                    0x40
#define BIT7                    0x80

#define LOW                     0
#define HIGH                    1

// Definition of DEVCAP values for 9244.
#define DEVCAP_VAL_DEV_STATE       0
#define DEVCAP_VAL_MHL_VERSION     MHL_VERSION
#define DEVCAP_VAL_DEV_CAT         (MHL_DEV_CAT_SOURCE)
#define DEVCAP_VAL_ADOPTER_ID_H    (uint8_t)(SILICON_IMAGE_ADOPTER_ID >>   8)
#define DEVCAP_VAL_ADOPTER_ID_L    (uint8_t)(SILICON_IMAGE_ADOPTER_ID & 0xFF)
#define DEVCAP_VAL_VID_LINK_MODE   MHL_DEV_VID_LINK_SUPPRGB444
#define DEVCAP_VAL_AUD_LINK_MODE   MHL_DEV_AUD_LINK_2CH
#define DEVCAP_VAL_VIDEO_TYPE      0
#define DEVCAP_VAL_LOG_DEV_MAP     MHL_LOGICAL_DEVICE_MAP
#define DEVCAP_VAL_BANDWIDTH       0
#define DEVCAP_VAL_FEATURE_FLAG    (MHL_FEATURE_RCP_SUPPORT | MHL_FEATURE_RAP_SUPPORT |MHL_FEATURE_SP_SUPPORT)
#define DEVCAP_VAL_DEVICE_ID_H     (uint8_t)(TRANSCODER_DEVICE_ID>>   8)
#define DEVCAP_VAL_DEVICE_ID_L     (uint8_t)(TRANSCODER_DEVICE_ID& 0xFF)
#define DEVCAP_VAL_SCRATCHPAD_SIZE MHL_SCRATCHPAD_SIZE
#define DEVCAP_VAL_INT_STAT_SIZE   MHL_INT_AND_STATUS_SIZE
#define DEVCAP_VAL_RESERVED        0

// DEVCAP we will initialize to
#define	MHL_LOGICAL_DEVICE_MAP		(MHL_DEV_LD_AUDIO | MHL_DEV_LD_VIDEO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_GUI )

#define settingMode9290		0
#define settingMode938x		1
#define pin9290_938x 		settingMode938x

#define settingMode3X	0
#define settingMode1X	1
#define pinMode1x3x		settingMode1X

#endif//__SII_MHL_HAL_H__