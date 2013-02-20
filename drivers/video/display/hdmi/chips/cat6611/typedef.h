///*****************************************
//  Copyright (C) 2009-2014
//  ITE Tech. Inc. All Rights Reserved
//  Proprietary and Confidential
///*****************************************
//   @file   >typedef.h<
//   @author Jau-Chih.Tseng@ite.com.tw
//   @date   2009/08/24
//   @fileversion: CAT6611_SAMPLEINTERFACE_1.12
//******************************************/

#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

//////////////////////////////////////////////////
// data type
//////////////////////////////////////////////////
#ifdef _MCU_
typedef bit BOOL ;
#define _CODE code
#define _IDATA idata
#define _XDATA xdata
#else
typedef int BOOL ;
//#define BOOL int
#define _CODE
#define _IDATA
#define _XDATA
#endif // _MCU_



typedef char CHAR, *PCHAR ;
//typedef unsigned char uchar, *puchar ;
typedef unsigned char UCHAR, *PUCHAR ;
typedef unsigned char byte, *pbyte ;
typedef unsigned char BYTE, *PBYTE ;

typedef short SHORT, *PSHORT ;
//typedef unsigned short ushort, *pushort ;
typedef unsigned short USHORT, *PUSHORT ;	// gModify.Ques.char
typedef unsigned short word, *pword ;
typedef unsigned short WORD, *PWORD ;

typedef long LONG, *PLONG ;
//typedef unsigned long ulong, *pulong ;
typedef unsigned long ULONG, *PULONG ;
typedef unsigned long dword, *pdword ;
typedef unsigned long DWORD, *PDWORD ;

#define FALSE 0
#define TRUE 1

#define SUCCESS 0
#define FAIL -1

#define ON 1
#define OFF 0

typedef enum _SYS_STATUS {
    ER_SUCCESS = 0,
    ER_FAIL,
    ER_RESERVED
} SYS_STATUS ;

//#define abs(x) (((x)>0)?(x):(-(x)))

///////////////////////////////////////////////////////////////////////
// Video Data Type
///////////////////////////////////////////////////////////////////////
#define F_MODE_RGB444  0
#define F_MODE_YUV422 1
#define F_MODE_YUV444 2
#define F_MODE_CLRMOD_MASK 3


#define F_MODE_INTERLACE  1

#define F_VIDMODE_ITU709  (1<<4)
#define F_VIDMODE_ITU601  0

#define F_VIDMODE_0_255   0
#define F_VIDMODE_16_235  (1<<5)

#define F_VIDMODE_EN_UDFILT (1<<6) // output mode only, and loaded from EEPROM
#define F_VIDMODE_EN_DITHER (1<<7) // output mode only, and loaded from EEPROM


typedef union _VideoFormatCode {
    struct _VFC
    {
        BYTE colorfmt:2 ;
        BYTE interlace:1 ;
        BYTE Colorimetry:1 ;
        BYTE Quantization:1 ;
        BYTE UpDownFilter:1 ;
        BYTE Dither:1 ;
    } VFCCode ;
    unsigned char VFCByte ;
} VideoFormatCode ;

#define T_MODE_CCIR656 (1<<0)
#define T_MODE_SYNCEMB (1<<1)
#define T_MODE_INDDR (1<<2)
#define T_MODE_DEGEN (1<<3)
#define T_MODE_SYNCGEN (1<<4)

//////////////////////////////////////////////////////////////////
// Audio relate definition and macro.
//////////////////////////////////////////////////////////////////

// for sample clock
#define FS_22K05  4
#define FS_44K1 0
#define FS_88K2 8
#define FS_176K4    12

#define FS_24K  6
#define FS_48K  2
#define FS_96K  10
#define FS_192K 14

#define FS_32K  3
#define FS_OTHER    1

// Audio Enable
#define ENABLE_SPDIF    (1<<4)
#define ENABLE_I2S_SRC3  (1<<3)
#define ENABLE_I2S_SRC2  (1<<2)
#define ENABLE_I2S_SRC1  (1<<1)
#define ENABLE_I2S_SRC0  (1<<0)

#define AUD_SWL_NOINDICATE  0x0
#define AUD_SWL_16          0x2
#define AUD_SWL_17          0xC
#define AUD_SWL_18          0x4
#define AUD_SWL_20          0xA // for maximum 20 bit
#define AUD_SWL_21          0xD
#define AUD_SWL_22          0x5
#define AUD_SWL_23          0x9
#define AUD_SWL_24          0xB


/////////////////////////////////////////////////////////////////////
// Packet and Info Frame definition and datastructure.
/////////////////////////////////////////////////////////////////////

#define VENDORSPEC_INFOFRAME_TYPE 0x01
#define AVI_INFOFRAME_TYPE  0x02
#define SPD_INFOFRAME_TYPE 0x03
#define AUDIO_INFOFRAME_TYPE 0x04
#define MPEG_INFOFRAME_TYPE 0x05

#define VENDORSPEC_INFOFRAME_VER 0x01
#define AVI_INFOFRAME_VER  0x02
#define SPD_INFOFRAME_VER 0x01
#define AUDIO_INFOFRAME_VER 0x01
#define MPEG_INFOFRAME_VER 0x01

#define VENDORSPEC_INFOFRAME_LEN 8
#define AVI_INFOFRAME_LEN 13
#define SPD_INFOFRAME_LEN 25
#define AUDIO_INFOFRAME_LEN 10
#define MPEG_INFOFRAME_LEN 10

#define ACP_PKT_LEN 9
#define ISRC1_PKT_LEN 16
#define ISRC2_PKT_LEN 16

struct VideoTiming {
    ULONG VideoPixelClock ;
    BYTE VIC ;
    BYTE pixelrep ;
} ;

typedef union _AVI_InfoFrame
{
    struct {
        BYTE Type ;
        BYTE Ver ;
        BYTE Len ;

        BYTE Scan:2 ;
        BYTE BarInfo:2 ;
        BYTE ActiveFmtInfoPresent:1 ;
        BYTE ColorMode:2 ;
        BYTE FU1:1 ;

        BYTE ActiveFormatAspectRatio:4 ;
        BYTE PictureAspectRatio:2 ;
        BYTE Colorimetry:2 ;

        BYTE Scaling:2 ;
        BYTE FU2:6 ;

        BYTE VIC:7 ;
        BYTE FU3:1 ;

        BYTE PixelRepetition:4 ;
        BYTE FU4:4 ;

        SHORT Ln_End_Top ;
        SHORT Ln_Start_Bottom ;
        SHORT Pix_End_Left ;
        SHORT Pix_Start_Right ;
    } info ;
    struct {
        BYTE AVI_HB[3] ;
        BYTE AVI_DB[AVI_INFOFRAME_LEN] ;
    } pktbyte ;
} AVI_InfoFrame ;

typedef union _Audio_InfoFrame {

    struct {
        BYTE Type ;
        BYTE Ver ;
        BYTE Len ;

        BYTE AudioChannelCount:3 ;
        BYTE RSVD1:1 ;
        BYTE AudioCodingType:4 ;

        BYTE SampleSize:2 ;
        BYTE SampleFreq:3 ;
        BYTE Rsvd2:3 ;

        BYTE FmtCoding ;

        BYTE SpeakerPlacement ;

        BYTE Rsvd3:3 ;
        BYTE LevelShiftValue:4 ;
        BYTE DM_INH:1 ;
    } info ;

    struct {
        BYTE AUD_HB[3] ;
        BYTE AUD_DB[AUDIO_INFOFRAME_LEN] ;
    } pktbyte ;

} Audio_InfoFrame ;

typedef union _MPEG_InfoFrame {
    struct {
        BYTE Type ;
        BYTE Ver ;
        BYTE Len ;

        ULONG MpegBitRate ;

        BYTE MpegFrame:2 ;
        BYTE Rvsd1:2 ;
        BYTE FieldRepeat:1 ;
        BYTE Rvsd2:3 ;
    } info ;
    struct {
        BYTE MPG_HB[3] ;
        BYTE MPG_DB[MPEG_INFOFRAME_LEN] ;
    } pktbyte ;
} MPEG_InfoFrame ;

// Source Product Description
typedef union _SPD_InfoFrame {
    struct {
        BYTE Type ;
        BYTE Ver ;
        BYTE Len ;

        char VN[8] ; // vendor name character in 7bit ascii characters
        char PD[16] ; // product description character in 7bit ascii characters
        BYTE SourceDeviceInfomation ;
    } info ;
    struct {
        BYTE SPD_HB[3] ;
        BYTE SPD_DB[SPD_INFOFRAME_LEN] ;
    } pktbyte ;
} SPD_InfoFrame ;


#endif // _TYPEDEF_H_
