//    /linux/egistec/Define_JadeOpc.h

#ifndef DEFINE_JADEOPC_H_INCLUDED
#define DEFINE_JADEOPC_H_INCLUDED



enum enum_FlashType  {NoFlash = 0, MXIC, SST, ST, Winbond};

//#define READ        0x01
//#define WRITE       0x02
#define DES_ON      0x01
#define DES_OFF     0x00
//==========================================================
#define JADE_REGISTER_MASSREAD             0x01
#define JADE_REGISTER_MASSWRITE            0x02
#define JADE_GET_ONE_IMG                   0x03
#define JADE_GET_FULL_IMAGE                0x05
#define JADE_GET_FULL_IMAGE2               0x06

#define	FLASH_READID                       0X20
#define	FLASH_WRITE                        0X21
#define	FLASH_READ                         0X22
#define	FLASH_ERASE                        0X23
#define	FLASH_WRITE_STATUS                 0X24
#define	FLASH_READ_STATUS                  0X25
#define	FLASH_WRITE_EN                     0X26
#define	FLASH_WRITE_DN                     0X27

#define	ISP_BY_USB                         0X30
#define	READ_ISP_BY_USB                    0X31

#define	EXTERNAL_FUNCTION1                 0X51
#define	EXTERNAL_FUNCTION2                 0X52
#define	EXTERNAL_FUNCTION3                 0X53
#define	EXTERNAL_FUNCTION4                 0X54
#define	EXTERNAL_FUNCTION5                 0X55

#define	JADE_GPIO_READ_WRITE               0X60
#define	USB_SFR_READ_WRITE                 0X61

#define MARCH_TEST_FUNC                    0x99


//===========================================================

#define RESPONSE_DATA_OFFSET        	6
#define USB_LEADING_TAG_OFFSET			7
#define USB_LEADING_TAG_OFFSET_IMAGE			6

#define JADE_FIRMWARE_OK            1
#define JADE_FIRMWARE_TIMEOUT       3

#endif