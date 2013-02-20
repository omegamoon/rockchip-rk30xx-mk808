/***********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
// Standard C Library
//#include <stdio.h>

//#include "hal_mcu.h"
#include "sii92326.h"
#include "sii_92326_driver.h"
#include "sii_92326_api.h"

///////////////////////////////////////////////////////////////////////////////
// I2C functions used by the driver.
//
extern void	DelayMS ( uint16_t m_sec );
extern uint8_t I2CReadByte(uint8_t SlaveAddr, uint8_t RegAddr);
extern void 	I2CWriteByte(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Data);
extern uint8_t I2CReadBlock(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t NBytes, uint8_t * Data);
extern uint8_t I2CWriteBlock(uint8_t SlaveAddr, uint8_t RegAddr, uint8_t NBytes, uint8_t * Data);
extern uint8_t I2CReadSegmentBlockEDID(uint8_t SlaveAddr, uint8_t Segment, uint8_t Offset, uint8_t *Buffer, uint8_t Length);

///////////////////////////////////////////////////////////////////////////////
// GPIO for chip reset.
//
//sbit pinMHLTxHwResetN = P0^2;

// GPIO for CbusWakeUpGenerator.
//
//sbit pinMHLTxCbusWakeUpPulse = P0^1;

// GPIO for hardware interrupt.
//
//sbit pinMHLTxInt = P3^3;

///////////////////////////////////////////////////////////////////////////////
//
// Store global config info here. This is shared by the driver.
//
//
//
// structure to hold operating information of MhlTx component
//
mhlTx_config_t	mhlTxConfig={0};

///////////////////////////////////////////////////////////////////////////////
// for 92326 only!
//
bool_t	TPImode = FALSE;
uint8_t	tpivmode[3] ={0};  // saved TPI Reg0x08/Reg0x09/Reg0x0A values.

mhlTx_AVSetting  mhlTxAv = {0};

#ifdef DEV_SUPPORT_EDID
mhlTx_Edid     mhlTxEdid = {0};
#else
uint8_t	HDMI_Sink = FALSE;
uint8_t HDMI_YCbCr_4_4_4 = FALSE;
uint8_t HDMI_YCbCr_4_2_2 = FALSE;
#endif

#ifdef DEV_SUPPORT_HDCP
mhlTx_Hdcp    mhlTxHdcp = {0};
#endif


//------------------------------------------------------------------------------
// Function Name: ReadByteTPI()
// Function Description: I2C read
//------------------------------------------------------------------------------
uint8_t ReadByteTPI (uint8_t Offset)
{
    	return I2CReadByte(TX_TPI_ADDR, Offset);
}

//------------------------------------------------------------------------------
// Function Name: WriteByteTPI()
// Function Description: I2C write
//------------------------------------------------------------------------------
void WriteByteTPI (uint8_t Offset, uint8_t Data)
{
    	I2CWriteByte(TX_TPI_ADDR, Offset, Data);
}

//------------------------------------------------------------------------------
// Function Name: ReadSetWriteTPI()
// Function Description: Write "1" to all bits in TPI offset "Offset" that are set
//                  to "1" in "Pattern"; Leave all other bits in "Offset"
//                  unchanged.
//------------------------------------------------------------------------------
void ReadSetWriteTPI (uint8_t Offset, uint8_t Pattern)
{
    	uint8_t Tmp;

    	Tmp = ReadByteTPI(Offset);

    	Tmp |= Pattern;
    	WriteByteTPI(Offset, Tmp);
}

//------------------------------------------------------------------------------
// Function Name: ReadSetWriteTPI()
// Function Description: Write "0" to all bits in TPI offset "Offset" that are set
//                  to "1" in "Pattern"; Leave all other bits in "Offset"
//                  unchanged.
//------------------------------------------------------------------------------
void ReadClearWriteTPI (uint8_t Offset, uint8_t Pattern)
{
    	uint8_t Tmp;

    	Tmp = ReadByteTPI(Offset);

    	Tmp &= ~Pattern;
    	WriteByteTPI(Offset, Tmp);
}

//------------------------------------------------------------------------------
// Function Name: ReadSetWriteTPI()
// Function Description: Write "Value" to all bits in TPI offset "Offset" that are set
//                  to "1" in "Mask"; Leave all other bits in "Offset"
//                  unchanged.
//------------------------------------------------------------------------------
void ReadModifyWriteTPI (uint8_t Offset, uint8_t Mask, uint8_t Value)
{
	uint8_t Tmp;
	
	Tmp = ReadByteTPI(Offset);
	Tmp &= ~Mask;
	Tmp |= (Value & Mask);
	WriteByteTPI(Offset, Tmp);
}

//------------------------------------------------------------------------------
// Function Name: ReadBlockTPI()
// Function Description: Read NBytes from offset Addr of the TPI slave address
//                      into a uint8_t Buffer pointed to by Data
//------------------------------------------------------------------------------
void ReadBlockTPI (uint8_t TPI_Offset, uint8_t NBytes, uint8_t * pData)
{
	I2CReadBlock(TX_TPI_ADDR, TPI_Offset, NBytes, pData);
}

//------------------------------------------------------------------------------
// Function Name: WriteBlockTPI()
// Function Description: Write NBytes from a uint8_t Buffer pointed to by Data to
//                      the TPI I2C slave starting at offset Addr
//------------------------------------------------------------------------------
void WriteBlockTPI (uint8_t TPI_Offset, uint8_t NBytes, uint8_t * pData)
{
	I2CWriteBlock(TX_TPI_ADDR, TPI_Offset, NBytes, pData);
}

//------------------------------------------------------------------------------
// Function Name: ReadIndexedRegister()
// Function Description: Read an indexed register value
//
//                  Write:
//                      1. 0xBC => Internal page num
//                      2. 0xBD => Indexed register offset
//
//                  Read:
//                      3. 0xBE => Returns the indexed register value
//------------------------------------------------------------------------------
uint8_t ReadIndexedRegister(uint8_t PageNum, uint8_t RegOffset) 
{
	WriteByteTPI(TPI_INTERNAL_PAGE_REG, PageNum);		// Internal page
	WriteByteTPI(TPI_INDEXED_OFFSET_REG, RegOffset);	// Indexed register
	return ReadByteTPI(TPI_INDEXED_VALUE_REG); 		// Return read value
}

//------------------------------------------------------------------------------
// Function Name: WriteIndexedRegister()
// Function Description: Write a value to an indexed register
//
//                  Write:
//                      1. 0xBC => Internal page num
//                      2. 0xBD => Indexed register offset
//                      3. 0xBE => Set the indexed register value
//------------------------------------------------------------------------------
void WriteIndexedRegister (uint8_t PageNum, uint8_t RegOffset, uint8_t RegValue) 
{
	WriteByteTPI(TPI_INTERNAL_PAGE_REG, PageNum);  // Internal page
	WriteByteTPI(TPI_INDEXED_OFFSET_REG, RegOffset);  // Indexed register
	WriteByteTPI(TPI_INDEXED_VALUE_REG, RegValue);    // Read value into buffer
}

//------------------------------------------------------------------------------
// Function Name: ReadModifyWriteIndexedRegister()
// Function Description: Write "Value" to all bits in TPI offset "Offset" that are set
//                  to "1" in "Mask"; Leave all other bits in "Offset"
//                  unchanged.
//------------------------------------------------------------------------------
void ReadModifyWriteIndexedRegister(uint8_t PageNum, uint8_t RegOffset, uint8_t Mask, uint8_t Value)
{
	uint8_t Tmp;
	
	WriteByteTPI(TPI_INTERNAL_PAGE_REG, PageNum);
	WriteByteTPI(TPI_INDEXED_OFFSET_REG, RegOffset);
	Tmp = ReadByteTPI(TPI_INDEXED_VALUE_REG);
	
	Tmp &= ~Mask;
	Tmp |= (Value & Mask);
	
	WriteByteTPI(TPI_INDEXED_VALUE_REG, Tmp);
}

//------------------------------------------------------------------------------
// Function Name: sii_I2CReadByte()
// Function Description: I2C read
//------------------------------------------------------------------------------
uint8_t sii_I2CReadByte (uint8_t SlaveAddr, uint8_t RegAddr)
{
	uint8_t regVal;

	if (!TPImode)
		regVal = I2CReadByte(SlaveAddr, RegAddr);
	else
	{
		if( SlaveAddr == PAGE_0_0X72 )
    			regVal = ReadIndexedRegister(INDEXED_PAGE_0, RegAddr);
		else if( SlaveAddr == PAGE_1_0X7A )
			regVal = ReadIndexedRegister(INDEXED_PAGE_1, RegAddr);
		else if( SlaveAddr == PAGE_2_0X92 )
			regVal = ReadIndexedRegister(INDEXED_PAGE_2, RegAddr);
		else
			regVal = I2CReadByte(SlaveAddr, RegAddr);
	}
	
	return regVal;
}

//------------------------------------------------------------------------------
// Function Name: sii_I2CWriteByte()
// Function Description: I2C write
//------------------------------------------------------------------------------
void sii_I2CWriteByte (uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Data)
{
	if (!TPImode)
		I2CWriteByte(SlaveAddr, RegAddr, Data);
	else
	{
		if( SlaveAddr == PAGE_0_0X72 )
    			WriteIndexedRegister(INDEXED_PAGE_0, RegAddr, Data);
		else if( SlaveAddr == PAGE_1_0X7A )
			WriteIndexedRegister(INDEXED_PAGE_1, RegAddr, Data);
		else if( SlaveAddr == PAGE_2_0X92 )
			WriteIndexedRegister(INDEXED_PAGE_2, RegAddr, Data);
		else
			I2CWriteByte(SlaveAddr, RegAddr, Data);
	}
}


///////////////////////////////////////////////////////////////////////////
//
// StartTPI
//
static bool_t StartTPI( void )
{
	uint8_t devID = 0x00;
	uint16_t wID = 0x0000;

	TX_DEBUG_PRINT(("[MHL]: >>StartTPI()\n"));

	WriteByteTPI(TPI_ENABLE, 0x00);	// Write "0" to 72:C7 to start HW TPI mode
	DelayMS(100);
	devID = ReadIndexedRegister(INDEXED_PAGE_0, 0x03);
	wID = devID;
	wID <<= 8;
	devID = ReadIndexedRegister(INDEXED_PAGE_0, 0x02);
	wID |= devID;

	devID = ReadByteTPI(TPI_DEVICE_ID);
	TX_DEBUG_PRINT(("[MHL]: SiI%04X, SiI_DEVICE_ID: %02X\n", (int)wID, (int)devID));

	return (TPImode = TRUE);
}

/*
//------------------------------------------------------------------------------
// Function Name: EnableTMDS()
// Function Description: Enable TMDS
//------------------------------------------------------------------------------
void EnableTMDS (void)
{
    TX_DEBUG_PRINT(("[MHL]: TMDS -> Enabled\n"));
    ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, TMDS_OUTPUT_CONTROL_MASK, TMDS_OUTPUT_CONTROL_ACTIVE);
    WriteByteTPI(TPI_PIX_REPETITION, tpivmode[0]);      		// Write register 0x08
}

//------------------------------------------------------------------------------
// Function Name: DisableTMDS()
// Function Description: Disable TMDS
//------------------------------------------------------------------------------
void DisableTMDS (void)
{
    TX_DEBUG_PRINT(("[MHL]: TMDS -> Disabled\n"));
    ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK,
					   TMDS_OUTPUT_CONTROL_POWER_DOWN | AV_MUTE_MUTED);
}
*/

//------------------------------------------------------------------------------
// Function Name: EnableInterrupts()
// Function Description: Enable the interrupts specified in the input parameter
//
// Accepts: A bit pattern with "1" for each interrupt that needs to be
//                  set in the Interrupt Enable Register (TPI offset 0x3C)
// Returns: TRUE
// Globals: none
//------------------------------------------------------------------------------
uint8_t EnableInterrupts(uint8_t Interrupt_Pattern)
{
	TX_DEBUG_PRINT(("[MHL]: >>EnableInterrupts()\n"));
	ReadSetWriteTPI(TPI_INTERRUPT_ENABLE_REG, Interrupt_Pattern);
	
	return TRUE;
}

#define ClearInterrupt(x)       WriteByteTPI(TPI_INTERRUPT_STATUS_REG, x)   // write "1" to clear interrupt bit

//------------------------------------------------------------------------------
// Function Name: DisableInterrupts()
// Function Description: Disable the interrupts specified in the input parameter
//
// Accepts: A bit pattern with "1" for each interrupt that needs to be
//                  cleared in the Interrupt Enable Register (TPI offset 0x3C)
// Returns: TRUE
// Globals: none
//------------------------------------------------------------------------------
uint8_t DisableInterrupts (uint8_t Interrupt_Pattern)
{
	TX_DEBUG_PRINT(("[MHL]: >>DisableInterrupts()\n"));
	ReadClearWriteTPI(TPI_INTERRUPT_ENABLE_REG, Interrupt_Pattern);
	
	return TRUE;
}


static void SiiMhlTxTmdsEnable(void);

#ifdef DEV_SUPPORT_EDID
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////                   EDID                 ///////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static uint8_t g_CommData [EDID_BLOCK_SIZE];
#endif
#define ReadBlockEDID(a,b,c)				I2CReadBlock(EDID_ROM_ADDR, a, b, c)
#define ReadSegmentBlockEDID(a,b,c,d)		I2CReadSegmentBlockEDID(EDID_ROM_ADDR, a, b, d, c)

//------------------------------------------------------------------------------
// Function Name: GetDDC_Access()
// Function Description: Request access to DDC bus from the receiver
//
// Accepts: none
// Returns: TRUE or FLASE
// Globals: none
//------------------------------------------------------------------------------
#define T_DDC_ACCESS    50

uint8_t GetDDC_Access (uint8_t* SysCtrlRegVal)
{
	uint8_t sysCtrl;
	uint8_t DDCReqTimeout = T_DDC_ACCESS;
	uint8_t TPI_ControlImage;

	TX_DEBUG_PRINT(("[MHL]: >>GetDDC_Access()\n"));

	sysCtrl = ReadByteTPI (TPI_SYSTEM_CONTROL_DATA_REG);			// Read and store original value. Will be passed into ReleaseDDC()
	*SysCtrlRegVal = sysCtrl;

	sysCtrl |= DDC_BUS_REQUEST_REQUESTED;
	WriteByteTPI (TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);

	while (DDCReqTimeout--)											// Loop till 0x1A[1] reads "1"
	{
		TPI_ControlImage = ReadByteTPI(TPI_SYSTEM_CONTROL_DATA_REG);

		if (TPI_ControlImage & DDC_BUS_GRANT_MASK)					// When 0x1A[1] reads "1"
		{
			sysCtrl |= DDC_BUS_GRANT_GRANTED;
			WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);		// lock host DDC bus access (0x1A[2:1] = 11)
			return TRUE;
		}
		WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);			// 0x1A[2] = "1" - Requst the DDC bus
		DelayMS(200);
	}

	WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, sysCtrl);				// Failure... restore original value.
	return FALSE;
}

//------------------------------------------------------------------------------
// Function Name: ReleaseDDC()
// Function Description: Release DDC bus
//
// Accepts: none
// Returns: TRUE if bus released successfully. FALSE if failed.
// Globals: none
//------------------------------------------------------------------------------
uint8_t ReleaseDDC (uint8_t SysCtrlRegVal)
{
	uint8_t DDCReqTimeout = T_DDC_ACCESS;
	uint8_t TPI_ControlImage;

	TX_DEBUG_PRINT(("[MHL]: >>ReleaseDDC()\n"));

	SysCtrlRegVal &= ~BITS_2_1;					// Just to be sure bits [2:1] are 0 before it is written

	while (DDCReqTimeout--)						// Loop till 0x1A[1] reads "0"
	{
		// Cannot use ReadClearWriteTPI() here. A read of TPI_SYSTEM_CONTROL is invalid while DDC is granted.
		// Doing so will return 0xFF, and cause an invalid value to be written back.
		//ReadClearWriteTPI(TPI_SYSTEM_CONTROL,BITS_2_1); // 0x1A[2:1] = "0" - release the DDC bus

		WriteByteTPI(TPI_SYSTEM_CONTROL_DATA_REG, SysCtrlRegVal);
		TPI_ControlImage = ReadByteTPI(TPI_SYSTEM_CONTROL_DATA_REG);

		if (!(TPI_ControlImage & BITS_2_1))		// When 0x1A[2:1] read "0"
			return TRUE;
	}

	return FALSE;								// Failed to release DDC bus control
}
#ifdef DEV_SUPPORT_EDID
//------------------------------------------------------------------------------
// Function Name: CheckEDID_Header()
// Function Description: Checks if EDID header is correct per VESA E-EDID standard
//
// Accepts: Pointer to 1st EDID block
// Returns: TRUE or FLASE
// Globals: EDID data
//------------------------------------------------------------------------------
uint8_t CheckEDID_Header (uint8_t *Block)
{
	uint8_t i = 0;
	
	if (Block[i])               // uint8_t 0 must be 0
		return FALSE;
	
	for (i = 1; i < 1 + EDID_HDR_NO_OF_FF; i++)
	{
		if(Block[i] != 0xFF)    // bytes [1..6] must be 0xFF
	    	return FALSE;
	}
	
	if (Block[i])               // uint8_t 7 must be 0
		return FALSE;
	
	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: DoEDID_Checksum()
// Function Description: Calculte checksum of the 128 uint8_t block pointed to by the
//                  pointer passed as parameter
//
// Accepts: Pointer to a 128 uint8_t block whose checksum needs to be calculated
// Returns: TRUE or FLASE
// Globals: EDID data
//------------------------------------------------------------------------------
uint8_t DoEDID_Checksum (uint8_t *Block)
{
	uint8_t i;
	uint8_t CheckSum = 0;
	
	for (i = 0; i < EDID_BLOCK_SIZE; i++)
		CheckSum += Block[i];
	
	if (CheckSum)
		return FALSE;
	
	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: ParseEstablishedTiming()
// Function Description: Parse the established timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// Accepts: Pointer to the 128 uint8_t array where the data read from EDID Block0 is stored.
// Returns: none
// Globals: EDID data
//------------------------------------------------------------------------------
#if (CONF__TX_EDID_PRINT == ENABLE)
void ParseEstablishedTiming (uint8_t *Data)
{
	TX_EDID_PRINT(("Parsing Established Timing:\n"));
	TX_EDID_PRINT(("===========================\n"));
	
	// Parse Established Timing Byte #0
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT7)
		TX_EDID_PRINT(("720 x 400 @ 70Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT6)
		TX_EDID_PRINT(("720 x 400 @ 88Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT5)
		TX_EDID_PRINT(("640 x 480 @ 60Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT4)
		TX_EDID_PRINT(("640 x 480 @ 67Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT3)
		TX_EDID_PRINT(("640 x 480 @ 72Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT2)
		TX_EDID_PRINT(("640 x 480 @ 75Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT1)
		TX_EDID_PRINT(("800 x 600 @ 56Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX] & BIT0)
		TX_EDID_PRINT(("800 x 400 @ 60Hz\n"));
	
	// Parse Established Timing Byte #1:
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT7)
		TX_EDID_PRINT(("800 x 600 @ 72Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT6)
		TX_EDID_PRINT(("800 x 600 @ 75Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT5)
		TX_EDID_PRINT(("832 x 624 @ 75Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT4)
		TX_EDID_PRINT(("1024 x 768 @ 87Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT3)
		TX_EDID_PRINT(("1024 x 768 @ 60Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT2)
		TX_EDID_PRINT(("1024 x 768 @ 70Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT1)
		TX_EDID_PRINT(("1024 x 768 @ 75Hz\n"));
	if(Data[ESTABLISHED_TIMING_INDEX + 1]  & BIT0)
		TX_EDID_PRINT(("1280 x 1024 @ 75Hz\n"));
	
	// Parse Established Timing Byte #2:
	if(Data[ESTABLISHED_TIMING_INDEX + 2] & 0x80)
		TX_EDID_PRINT(("1152 x 870 @ 75Hz\n"));
	
	if((!Data[0])&&(!Data[ESTABLISHED_TIMING_INDEX + 1]  )&&(!Data[2]))
		TX_EDID_PRINT(("No established video modes\n"));
}

//------------------------------------------------------------------------------
// Function Name: ParseStandardTiming()
// Function Description: Parse the standard timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// Accepts: Pointer to the 128 uint8_t array where the data read from EDID Block0 is stored.
// Returns: none
// Globals: EDID data
//------------------------------------------------------------------------------
void ParseStandardTiming (uint8_t *Data)
{
	uint8_t i;
	uint8_t AR_Code;
	
	TX_EDID_PRINT(("Parsing Standard Timing:\n"));
	TX_EDID_PRINT(("========================\n"));
	
	for (i = 0; i < NUM_OF_STANDARD_TIMINGS; i += 2)
	{
		if ((Data[STANDARD_TIMING_OFFSET + i] == 0x01) && ((Data[STANDARD_TIMING_OFFSET + i +1]) == 1))
		{
	    		TX_EDID_PRINT(("Standard Timing Undefined\n")); // per VESA EDID standard, Release A, Revision 1, February 9, 2000, Sec. 3.9
	      }
		else
		{
	    		TX_EDID_PRINT(("Horizontal Active pixels: %i\n", (int)((Data[STANDARD_TIMING_OFFSET + i] + 31)*8)));    // per VESA EDID standard, Release A, Revision 1, February 9, 2000, Table 3.15
	
	    		AR_Code = (Data[STANDARD_TIMING_OFFSET + i +1] & TWO_MSBITS) >> 6;
	    		TX_EDID_PRINT(("Aspect Ratio: "));
	
	    		switch(AR_Code)
	    		{
	        		case AR16_10:
	            			TX_EDID_PRINT(("16:10\n"));
	            			break;
	
	        		case AR4_3:
	            			TX_EDID_PRINT(("4:3\n"));
	            			break;
	
	        		case AR5_4:
	            			TX_EDID_PRINT(("5:4\n"));
	            			break;
	
	        		case AR16_9:
	            			TX_EDID_PRINT(("16:9\n"));
	            			break;
	    		}
		}
	}
}

//------------------------------------------------------------------------------
// Function Name: ParseDetailedTiming()
// Function Description: Parse the detailed timing section of EDID Block 0 and
//                  print their decoded meaning to the screen.
//
// Accepts: Pointer to the 128 uint8_t array where the data read from EDID Block0 is stored.
//              Offset to the beginning of the Detailed Timing Descriptor data.
//
//              Block indicator to distinguish between block #0 and blocks #2, #3
// Returns: none
// Globals: EDID data
//------------------------------------------------------------------------------
uint8_t ParseDetailedTiming (uint8_t *Data, uint8_t DetailedTimingOffset, uint8_t Block)
{
	uint8_t TmpByte;
	uint8_t i;
	uint16_t TmpWord;
	
	TmpWord = Data[DetailedTimingOffset + PIX_CLK_OFFSET] +
	            256 * Data[DetailedTimingOffset + PIX_CLK_OFFSET + 1];
	
	if (TmpWord == 0x00)            // 18 uint8_t partition is used as either for Monitor Name or for Monitor Range Limits or it is unused
	{
		if (Block == EDID_BLOCK_0)      // if called from Block #0 and first 2 bytes are 0 => either Monitor Name or for Monitor Range Limits
	    	{
	       	if (Data[DetailedTimingOffset + 3] == 0xFC) // these 13 bytes are ASCII coded monitor name
	        	{
	             	TX_EDID_PRINT(("Monitor Name: "));
	
	                	for (i = 0; i < 13; i++)
	                   {
	                		TX_EDID_PRINT(("%c", Data[DetailedTimingOffset + 5 + i])); // Display monitor name on SiIMon
	                    }
	                    TX_EDID_PRINT(("\n"));
	        	}
	
	             else if (Data[DetailedTimingOffset + 3] == 0xFD) // these 13 bytes contain Monitor Range limits, binary coded
	             {
	                    TX_EDID_PRINT(("Monitor Range Limits:\n\n"));
	
	                     i = 0;
	                    TX_EDID_PRINT(("Min Vertical Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++])); //
	                	TX_EDID_PRINT(("Max Vertical Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++])); //
	                    TX_EDID_PRINT(("Min Horizontal Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++])); //
	                    TX_EDID_PRINT(("Max Horizontal Rate in Hz: %d\n", (int) Data[DetailedTimingOffset + 5 + i++])); //
	                    TX_EDID_PRINT(("Max Supported pixel clock rate in MHz/10: %d\n", (int) Data[DetailedTimingOffset + 5 + i++])); //
	                	TX_EDID_PRINT(("Tag for secondary timing formula (00h=not used): %d\n", (int) Data[DetailedTimingOffset + 5 + i++])); //
	                    TX_EDID_PRINT(("Min Vertical Rate in Hz %d\n", (int) Data[DetailedTimingOffset + 5 + i])); //
	                    TX_EDID_PRINT(("\n"));
	             }
	     }
	
	     else if (Block == EDID_BLOCK_2_3)                          // if called from block #2 or #3 and first 2 bytes are 0x00 (padding) then this
	     {                                                                                          // descriptor partition is not used and parsing should be stopped
	      	TX_EDID_PRINT(("No More Detailed descriptors in this block\n"));
	             TX_EDID_PRINT(("\n"));
	             return FALSE;
	      }
	}
	
	else                                            // first 2 bytes are not 0 => this is a detailed timing descriptor from either block
	{
		if((Block == EDID_BLOCK_0) && (DetailedTimingOffset == 0x36))
	      {
	      	TX_EDID_PRINT(("\n\n\nParse Results, EDID Block #0, Detailed Descriptor Number 1:\n"));
	             TX_EDID_PRINT(("===========================================================\n\n"));
	      }
	      else if((Block == EDID_BLOCK_0) && (DetailedTimingOffset == 0x48))
	      {
	             TX_EDID_PRINT(("\n\n\nParse Results, EDID Block #0, Detailed Descriptor Number 2:\n"));
	             TX_EDID_PRINT(("===========================================================\n\n"));
	       }
	
	   	TX_EDID_PRINT(("Pixel Clock (MHz * 100): %d\n", (int)TmpWord));
	
	    	TmpWord = Data[DetailedTimingOffset + H_ACTIVE_OFFSET] +
	            256 * ((Data[DetailedTimingOffset + H_ACTIVE_OFFSET + 2] >> 4) & FOUR_LSBITS);
	    	TX_EDID_PRINT(("Horizontal Active Pixels: %d\n", (int)TmpWord));
	
	    	TmpWord = Data[DetailedTimingOffset + H_BLANKING_OFFSET] +
	            256 * (Data[DetailedTimingOffset + H_BLANKING_OFFSET + 1] & FOUR_LSBITS);
	    	TX_EDID_PRINT(("Horizontal Blanking (Pixels): %d\n", (int)TmpWord));
	
	    	TmpWord = (Data[DetailedTimingOffset + V_ACTIVE_OFFSET] )+
	            256 * ((Data[DetailedTimingOffset + (V_ACTIVE_OFFSET) + 2] >> 4) & FOUR_LSBITS);
	    	TX_EDID_PRINT(("Vertical Active (Lines): %d\n", (int)TmpWord));
	
	    	TmpWord = Data[DetailedTimingOffset + V_BLANKING_OFFSET] +
	            256 * (Data[DetailedTimingOffset + V_BLANKING_OFFSET + 1] & LOW_NIBBLE);
	    	TX_EDID_PRINT(("Vertical Blanking (Lines): %d\n", (int)TmpWord));
	
	    	TmpWord = Data[DetailedTimingOffset + H_SYNC_OFFSET] +
	            256 * ((Data[DetailedTimingOffset + (H_SYNC_OFFSET + 3)] >> 6) & TWO_LSBITS);
	    	TX_EDID_PRINT(("Horizontal Sync Offset (Pixels): %d\n", (int)TmpWord));
	
	    	TmpWord = Data[DetailedTimingOffset + H_SYNC_PW_OFFSET] +
	            256 * ((Data[DetailedTimingOffset + (H_SYNC_PW_OFFSET + 2)] >> 4) & TWO_LSBITS);
	    	TX_EDID_PRINT(("Horizontal Sync Pulse Width (Pixels): %d\n", (int)TmpWord));
	
	    	TmpWord = (Data[DetailedTimingOffset + V_SYNC_OFFSET] >> 4) & FOUR_LSBITS +
	            256 * ((Data[DetailedTimingOffset + (V_SYNC_OFFSET + 1)] >> 2) & TWO_LSBITS);
	    	TX_EDID_PRINT(("Vertical Sync Offset (Lines): %d\n", (int)TmpWord));
	
	    	TmpWord = (Data[DetailedTimingOffset + V_SYNC_PW_OFFSET]) & FOUR_LSBITS +
	            256 * (Data[DetailedTimingOffset + (V_SYNC_PW_OFFSET + 1)] & TWO_LSBITS);
	    	TX_EDID_PRINT(("Vertical Sync Pulse Width (Lines): %d\n", (int)TmpWord));
	
	    	TmpWord = Data[DetailedTimingOffset + H_IMAGE_SIZE_OFFSET] +
	            256 * (((Data[DetailedTimingOffset + (H_IMAGE_SIZE_OFFSET + 2)]) >> 4) & FOUR_LSBITS);
	    	TX_EDID_PRINT(("Horizontal Image Size (mm): %d\n", (int)TmpWord));
	
	    	TmpWord = Data[DetailedTimingOffset + V_IMAGE_SIZE_OFFSET] +
	            256 * (Data[DetailedTimingOffset + (V_IMAGE_SIZE_OFFSET + 1)] & FOUR_LSBITS);
	    	TX_EDID_PRINT(("Vertical Image Size (mm): %d\n", (int)TmpWord));
	
	    	TmpByte = Data[DetailedTimingOffset + H_BORDER_OFFSET];
	    	TX_EDID_PRINT(("Horizontal Border (Pixels): %d\n", (int)TmpByte));
	
	    	TmpByte = Data[DetailedTimingOffset + V_BORDER_OFFSET];
	    	TX_EDID_PRINT(("Vertical Border (Lines): %d\n", (int)TmpByte));
	
	    	TmpByte = Data[DetailedTimingOffset + FLAGS_OFFSET];
	    	if (TmpByte & BIT7)
	        	TX_EDID_PRINT(("Interlaced\n"));
	    	else
	        	TX_EDID_PRINT(("Non-Interlaced\n"));
	
	    	if (!(TmpByte & BIT5) && !(TmpByte & BIT6))
	        	TX_EDID_PRINT(("Normal Display, No Stereo\n"));
	    	else
	        	TX_EDID_PRINT(("Refer to VESA E-EDID Release A, Revision 1, table 3.17\n"));
	
	    	if (!(TmpByte & BIT3) && !(TmpByte & BIT4))
	        	TX_EDID_PRINT(("Analog Composite\n"));
	    	if ((TmpByte & BIT3) && !(TmpByte & BIT4))
	        	TX_EDID_PRINT(("Bipolar Analog Composite\n"));
	    	else if (!(TmpByte & BIT3) && (TmpByte & BIT4))
	        	TX_EDID_PRINT(("Digital Composite\n"));
	
	    	else if ((TmpByte & BIT3) && (TmpByte & BIT4))
	        	TX_EDID_PRINT(("Digital Separate\n"));
	
	      TX_EDID_PRINT(("\n"));
	}
	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: ParseBlock_0_TimingDescripors()
// Function Description: Parse EDID Block 0 timing descriptors per EEDID 1.3
//                  standard. printf() values to screen.
//
// Accepts: Pointer to the 128 uint8_t array where the data read from EDID Block0 is stored.
// Returns: none
// Globals: EDID data
//------------------------------------------------------------------------------
void ParseBlock_0_TimingDescripors (uint8_t *Data)
{
	uint8_t i;
	uint8_t Offset;
	
	ParseEstablishedTiming(Data);
	ParseStandardTiming(Data);
	
	for (i = 0; i < NUM_OF_DETAILED_DESCRIPTORS; i++)
	{
		Offset = DETAILED_TIMING_OFFSET + (LONG_DESCR_LEN * i);
		ParseDetailedTiming(Data, Offset, EDID_BLOCK_0);
	}
}
#endif

//------------------------------------------------------------------------------
// Function Name: ParseEDID()
// Function Description: Extract sink properties from its EDID file and save them in
//                  global structure mhlTxEdid.
//
// Accepts: none
// Returns: TRUE or FLASE
// Globals: EDID data
// NOTE: Fields that are not supported by the 9022/4 (such as deep color) were not parsed.
//------------------------------------------------------------------------------
uint8_t ParseEDID (uint8_t *pEdid, uint8_t *numExt)
{
	uint8_t i, j, k;

	TX_EDID_PRINT(("\n"));
	TX_EDID_PRINT(("EDID DATA (Segment = 0 Block = 0 Offset = %d):\n", (int) EDID_BLOCK_0_OFFSET));

	for (j = 0, i = 0; j < 128; j++)
	{
		k = pEdid[j];
		TX_EDID_PRINT(("%2.2X ", (int) k));
		i++;

		if (i == 0x10)
		{
			TX_EDID_PRINT(("\n"));
			i = 0;
		}
	}
	TX_EDID_PRINT(("\n"));

	if (!CheckEDID_Header(pEdid))
	{
		// first 8 bytes of EDID must be {0, FF, FF, FF, FF, FF, FF, 0}
		TX_DEBUG_PRINT(("EDID -> Incorrect Header\n"));
		return EDID_INCORRECT_HEADER;
	}

	if (!DoEDID_Checksum(pEdid))
	{
		// non-zero EDID checksum
		TX_DEBUG_PRINT(("EDID -> Checksum Error\n"));
		return EDID_CHECKSUM_ERROR;
	}
	
#if (CONF__TX_EDID_PRINT == ENABLE)
	ParseBlock_0_TimingDescripors(pEdid);			// Parse EDID Block #0 Desctiptors
#endif

	*numExt = pEdid[NUM_OF_EXTEN_ADDR];	// read # of extensions from offset 0x7E of block 0
	TX_EDID_PRINT(("EDID -> 861 Extensions = %d\n", (int) *numExt));

	if (!(*numExt))
	{
		// No extensions to worry about
		return EDID_NO_861_EXTENSIONS;
	}

	//return Parse861Extensions(NumOfExtensions);			// Parse 861 Extensions (short and long descriptors);
	return EDID_OK;
}

//------------------------------------------------------------------------------
// Function Name: Parse861ShortDescriptors()
// Function Description: Parse CEA-861 extension short descriptors of the EDID block
//                  passed as a parameter and save them in global structure mhlTxEdid.
//
// Accepts: A pointer to the EDID 861 Extension block being parsed.
// Returns: EDID_PARSED_OK if EDID parsed correctly. Error code if failed.
// Globals: EDID data
// NOTE: Fields that are not supported by the 9022/4 (such as deep color) were not parsed.
//------------------------------------------------------------------------------
uint8_t Parse861ShortDescriptors (uint8_t *Data)
{
    uint8_t LongDescriptorOffset;
    uint8_t DataBlockLength;
    uint8_t DataIndex;
    uint8_t ExtendedTagCode;
    uint8_t VSDB_BaseOffset = 0;

    uint8_t V_DescriptorIndex = 0;  // static to support more than one extension
    uint8_t A_DescriptorIndex = 0;  // static to support more than one extension

    uint8_t TagCode;

    uint8_t i;
    uint8_t j;

    if (Data[EDID_TAG_ADDR] != EDID_EXTENSION_TAG)
    {
        TX_EDID_PRINT(("EDID -> Extension Tag Error\n"));
        return EDID_EXT_TAG_ERROR;
    }

    if (Data[EDID_REV_ADDR] != EDID_REV_THREE)
    {
        TX_EDID_PRINT(("EDID -> Revision Error\n"));
        return EDID_REV_ADDR_ERROR;
    }

    LongDescriptorOffset = Data[LONG_DESCR_PTR_IDX];    // block offset where long descriptors start

    mhlTxEdid.UnderScan = ((Data[MISC_SUPPORT_IDX]) >> 7) & LSBIT;  // uint8_t #3 of CEA extension version 3
    mhlTxEdid.BasicAudio = ((Data[MISC_SUPPORT_IDX]) >> 6) & LSBIT;
    mhlTxEdid.YCbCr_4_4_4 = ((Data[MISC_SUPPORT_IDX]) >> 5) & LSBIT;
    mhlTxEdid.YCbCr_4_2_2 = ((Data[MISC_SUPPORT_IDX]) >> 4) & LSBIT;

    DataIndex = EDID_DATA_START;            // 4

    while (DataIndex < LongDescriptorOffset)
    {
        TagCode = (Data[DataIndex] >> 5) & THREE_LSBITS;
        DataBlockLength = Data[DataIndex++] & FIVE_LSBITS;
        if ((DataIndex + DataBlockLength) > LongDescriptorOffset)
        {
            TX_EDID_PRINT(("EDID -> V Descriptor Overflow\n"));
            return EDID_V_DESCR_OVERFLOW;
        }

        i = 0;                                  // num of short video descriptors in current data block

        switch (TagCode)
        {
            case VIDEO_D_BLOCK:
                while ((i < DataBlockLength) && (i < MAX_V_DESCRIPTORS))        // each SVD is 1 uint8_t long
                {
                    mhlTxEdid.VideoDescriptor[V_DescriptorIndex++] = Data[DataIndex++];
                    i++;
                }
                DataIndex += DataBlockLength - i;   // if there are more STDs than MAX_V_DESCRIPTORS, skip the last ones. Update DataIndex

                TX_EDID_PRINT(("EDID -> Short Descriptor Video Block\n"));
                break;

            case AUDIO_D_BLOCK:
                while (i < DataBlockLength/3)       // each SAD is 3 bytes long
                {
                    j = 0;
                    while (j < AUDIO_DESCR_SIZE)    // 3
                    {
                        mhlTxEdid.AudioDescriptor[A_DescriptorIndex][j++] = Data[DataIndex++];
                    }
                    A_DescriptorIndex++;
                    i++;
                }
                TX_EDID_PRINT(("EDID -> Short Descriptor Audio Block\n"));
                break;

            case  SPKR_ALLOC_D_BLOCK:
                mhlTxEdid.SpkrAlloc[i++] = Data[DataIndex++];       // although 3 bytes are assigned to Speaker Allocation, only
                DataIndex += 2;                                     // the first one carries information, so the next two are ignored by this code.
                TX_EDID_PRINT(("EDID -> Short Descriptor Speaker Allocation Block\n"));
                break;

            case USE_EXTENDED_TAG:
                ExtendedTagCode = Data[DataIndex++];

                switch (ExtendedTagCode)
                {
                    case VIDEO_CAPABILITY_D_BLOCK:
                        TX_EDID_PRINT(("EDID -> Short Descriptor Video Capability Block\n"));

                        // TO BE ADDED HERE: Save "video capability" parameters in mhlTxEdid data structure
                        // Need to modify that structure definition
                        // In the meantime: just increment DataIndex by 1
                        DataIndex += 1;    // replace with reading and saving the proper data per CEA-861 sec. 7.5.6 while incrementing DataIndex
                        break;

                    case COLORIMETRY_D_BLOCK:
                        mhlTxEdid.ColorimetrySupportFlags = Data[DataIndex++] & BITS_1_0;
                        mhlTxEdid.MetadataProfile = Data[DataIndex++] & BITS_2_1_0;

                        TX_EDID_PRINT(("EDID -> Short Descriptor Colorimetry Block\n"));
                        break;
                }
                break;

            case VENDOR_SPEC_D_BLOCK:
                VSDB_BaseOffset = DataIndex - 1;

                if ((Data[DataIndex++] == 0x03) &&    // check if sink is HDMI compatible
                    (Data[DataIndex++] == 0x0C) &&
                    (Data[DataIndex++] == 0x00))

                    mhlTxEdid.HDMI_Sink = TRUE;
                else
                    mhlTxEdid.HDMI_Sink = FALSE;

                mhlTxEdid.CEC_A_B = Data[DataIndex++];  // CEC Physical address
                mhlTxEdid.CEC_C_D = Data[DataIndex++];

                if ((DataIndex + 7) > VSDB_BaseOffset + DataBlockLength)        // Offset of 3D_Present bit in VSDB
                        mhlTxEdid._3D_Supported = FALSE;
                else if (Data[DataIndex + 7] >> 7)
                        mhlTxEdid._3D_Supported = TRUE;
                else
                        mhlTxEdid._3D_Supported = FALSE;

                DataIndex += DataBlockLength - HDMI_SIGNATURE_LEN - CEC_PHYS_ADDR_LEN; // Point to start of next block
                TX_EDID_PRINT(("EDID -> Short Descriptor Vendor Block\n"));
                TX_EDID_PRINT(("\n"));
                break;

            default:
                TX_EDID_PRINT(("EDID -> Unknown Tag Code\n"));
                return EDID_UNKNOWN_TAG_CODE;

        }                   // End, Switch statement
    }                       // End, while (DataIndex < LongDescriptorOffset) statement

    return EDID_SHORT_DESCRIPTORS_OK;
}

//------------------------------------------------------------------------------
// Function Name: Parse861LongDescriptors()
// Function Description: Parse CEA-861 extension long descriptors of the EDID block
//                  passed as a parameter and printf() them to the screen.
//
// Accepts: A pointer to the EDID block being parsed
// Returns: An error code if no long descriptors found; EDID_PARSED_OK if descriptors found.
// Globals: none
//------------------------------------------------------------------------------
uint8_t Parse861LongDescriptors (uint8_t *Data)
{
    uint8_t LongDescriptorsOffset;
    uint8_t DescriptorNum = 1;

    LongDescriptorsOffset = Data[LONG_DESCR_PTR_IDX];   // EDID block offset 2 holds the offset

    if (!LongDescriptorsOffset)                         // per CEA-861-D, table 27
    {
        TX_DEBUG_PRINT(("EDID -> No Detailed Descriptors\n"));
        return EDID_NO_DETAILED_DESCRIPTORS;
    }

    // of the 1st 18-uint8_t descriptor
    while (LongDescriptorsOffset + LONG_DESCR_LEN < EDID_BLOCK_SIZE)
    {
        TX_EDID_PRINT(("Parse Results - CEA-861 Long Descriptor #%d:\n", (int) DescriptorNum));
        TX_EDID_PRINT(("===============================================================\n"));

#if (CONF__TX_EDID_PRINT == ENABLE)
        if (!ParseDetailedTiming(Data, LongDescriptorsOffset, EDID_BLOCK_2_3))
                        break;
#endif
        LongDescriptorsOffset +=  LONG_DESCR_LEN;
        DescriptorNum++;
    }

    return EDID_LONG_DESCRIPTORS_OK;
}

//------------------------------------------------------------------------------
// Function Name: Parse861Extensions()
// Function Description: Parse CEA-861 extensions from EDID ROM (EDID blocks beyond
//                  block #0). Save short descriptors in global structure
//                  mhlTxEdid. printf() long descriptors to the screen.
//
// Accepts: The number of extensions in the EDID being parsed
// Returns: EDID_PARSED_OK if EDID parsed correctly. Error code if failed.
// Globals: EDID data
// NOTE: Fields that are not supported by the 9022/4 (such as deep color) were not parsed.
//------------------------------------------------------------------------------
uint8_t Parse861Extensions (uint8_t NumOfExtensions)
{
    uint8_t i,j,k;

    uint8_t ErrCode;

    uint8_t V_DescriptorIndex = 0;
    uint8_t A_DescriptorIndex = 0;

    uint8_t Segment = 0;
    uint8_t Block = 0;
    uint8_t Offset = 0;

    mhlTxEdid.HDMI_Sink = FALSE;

    do
    {
        Block++;

        Offset = 0;
        if ((Block % 2) > 0)
        {
            Offset = EDID_BLOCK_SIZE;
        }

        Segment = (uint8_t) (Block / 2);

        if (Block == 1)
        {
            if (ReadBlockEDID(EDID_BLOCK_1_OFFSET, EDID_BLOCK_SIZE, g_CommData))    // read first 128 bytes of EDID ROM
            {
            	  TX_DEBUG_PRINT (("EDID -> DDC Block1 read failed\n"));
            	  return EDID_DDC_BUS_READ_FAILURE;
            }
        }
        else
        {
            if (ReadSegmentBlockEDID(Segment, Offset, EDID_BLOCK_SIZE, g_CommData))     // read next 128 bytes of EDID ROM
            {
                TX_DEBUG_PRINT (("EDID -> DDC Extension Block read failed\n"));
                return EDID_DDC_BUS_READ_FAILURE;
            }
        }

        TX_EDID_PRINT(("\n"));
        TX_EDID_PRINT(("EDID DATA (Segment = %d Block = %d Offset = %d):\n", (int) Segment, (int) Block, (int) Offset));
        for (j=0, i=0; j<128; j++)
        {
            k = g_CommData[j];
            TX_EDID_PRINT(("%2.2X ", (int) k));
            i++;

            if (i == 0x10)
            {
                TX_EDID_PRINT(("\n"));
                i = 0;
            }
        }
        TX_EDID_PRINT(("\n"));

        if ((NumOfExtensions > 1) && (Block == 1))
        {
            continue;
        }

        ErrCode = Parse861ShortDescriptors(g_CommData);
        if (ErrCode != EDID_SHORT_DESCRIPTORS_OK)
        {
            return ErrCode;
        }

        ErrCode = Parse861LongDescriptors(g_CommData);
        if (ErrCode != EDID_LONG_DESCRIPTORS_OK)
        {
            return ErrCode;
        }

    } while (Block < NumOfExtensions);

    return EDID_OK;
}

//------------------------------------------------------------------------------
// Function Name: DoEdidRead()
// Function Description: EDID processing
//
// Accepts: none
// Returns: TRUE or FLASE
// Globals: none
//------------------------------------------------------------------------------
uint8_t DoEdidRead (void)
{
	uint8_t SysCtrlReg;
	uint8_t Result = EDID_OK;
	uint8_t NumOfExtensions;

	// If we already have valid EDID data, ship this whole thing
	if (mhlTxEdid.edidDataValid == TRUE)
		return Result;
	
	else
	{
		// Request access to DDC bus from the receiver
		if (GetDDC_Access(&SysCtrlReg))
		{
			if (ReadBlockEDID(EDID_BLOCK_0_OFFSET, EDID_BLOCK_SIZE, g_CommData))	// read first 128 bytes of EDID ROM
			{  // edid read error
				TX_DEBUG_PRINT (("EDID -> DDC Block0 read failed\n"));
				return EDID_DDC_BUS_READ_FAILURE;
			}
			
			Result = ParseEDID(g_CommData, &NumOfExtensions);
			if (Result == EDID_OK)
			{
				TX_DEBUG_PRINT (("EDID -> Block0 Parse OK\n"));
				Result = Parse861Extensions(NumOfExtensions);		// Parse 861 Extensions (short and long descriptors);
				
				if (Result == EDID_DDC_BUS_READ_FAILURE)
					return EDID_DDC_BUS_READ_FAILURE;
				
				if (Result != EDID_OK)
					TX_DEBUG_PRINT(("EDID -> Extension Parse FAILED\n"));
			}
			else if (Result == EDID_NO_861_EXTENSIONS)
				TX_DEBUG_PRINT (("EDID -> No 861 Extensions\n"));
			else
				TX_DEBUG_PRINT (("EDID -> Parse FAILED\n"));

			if (!ReleaseDDC(SysCtrlReg))				// Host must release DDC bus once it is done reading EDID
			{
				TX_DEBUG_PRINT (("EDID -> DDC bus release failed\n"));
				Result = EDID_DDC_BUS_RELEASE_FAILURE;
			}
		}
		else
		{
			TX_DEBUG_PRINT (("EDID -> DDC bus request failed\n"));
			Result = EDID_DDC_BUS_REQ_FAILURE;
		}

		if (Result == EDID_NO_861_EXTENSIONS)
		{
			mhlTxEdid.HDMI_Sink = FALSE;
			mhlTxEdid.YCbCr_4_4_4 = FALSE;
			mhlTxEdid.YCbCr_4_2_2 = FALSE;
			mhlTxEdid.CEC_A_B = 0x00;
			mhlTxEdid.CEC_C_D = 0x00;
		}
		else if (Result != EDID_OK)
		{
			mhlTxEdid.HDMI_Sink = TRUE;
			mhlTxEdid.YCbCr_4_4_4 = FALSE;
			mhlTxEdid.YCbCr_4_2_2 = FALSE;
			mhlTxEdid.CEC_A_B = 0x00;
			mhlTxEdid.CEC_C_D = 0x00;
		}
			
		TX_DEBUG_PRINT(("EDID -> mhlTxEdid.HDMI_Sink = %d\n", (int)mhlTxEdid.HDMI_Sink));
		TX_DEBUG_PRINT(("EDID -> mhlTxEdid.YCbCr_4_4_4 = %d\n", (int)mhlTxEdid.YCbCr_4_4_4));
		TX_DEBUG_PRINT(("EDID -> mhlTxEdid.YCbCr_4_2_2 = %d\n", (int)mhlTxEdid.YCbCr_4_2_2));
		TX_DEBUG_PRINT(("EDID -> mhlTxEdid.CEC_A_B = 0x%x\n", (int)mhlTxEdid.CEC_A_B));
		TX_DEBUG_PRINT(("EDID -> mhlTxEdid.CEC_C_D = 0x%x\n", (int)mhlTxEdid.CEC_C_D));

		mhlTxEdid.edidDataValid = TRUE;
		
		return Result;
	}
}
#endif

#ifdef DEV_SUPPORT_HDCP
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////                  HDCP                 ///////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
// Function Name: IsHDCP_Supported()
// Function Description: Check Tx revision number to find if this Tx supports HDCP
//                  by reading the HDCP revision number from TPI register 0x30.
//
// Accepts: none
// Returns: TRUE if Tx supports HDCP. FALSE if not.
// Globals: none
//------------------------------------------------------------------------------
uint8_t IsHDCP_Supported (void)
{
    	uint8_t HDCP_Rev;
	uint8_t HDCP_Supported;

	TX_DEBUG_PRINT(("[MHL]: >> IsHDCP_Supported()\n"));

	HDCP_Supported = TRUE;

	// Check Device ID
    	HDCP_Rev = ReadByteTPI(TPI_HDCP_REVISION_DATA_REG);

    	if (HDCP_Rev != (HDCP_MAJOR_REVISION_VALUE | HDCP_MINOR_REVISION_VALUE))
	{
    		HDCP_Supported = FALSE;
	}

	return HDCP_Supported;
}

//------------------------------------------------------------------------------
// Function Name: AreAKSV_OK()
// Function Description: Check if AKSVs contain 20 '0' and 20 '1'
//
// Accepts: none
// Returns: TRUE if 20 zeros and 20 ones found in AKSV. FALSE OTHERWISE
// Globals: none
//------------------------------------------------------------------------------
uint8_t AreAKSV_OK (void)
{
	uint8_t B_Data[AKSV_SIZE];
	uint8_t NumOfOnes = 0;
	uint8_t i, j;
	
	TX_DEBUG_PRINT(("[MHL]: >> AreAKSV_OK()\n"));
	
	ReadBlockTPI(TPI_AKSV_1_REG, AKSV_SIZE, B_Data);
	
	for (i=0; i<AKSV_SIZE; i++)
	{
		for (j=0; j<BYTE_SIZE; j++)
		{
	    		if (B_Data[i] & 0x01)
	    		{
	        		NumOfOnes++;
	    		}
	    		B_Data[i] >>= 1;
		}
	}
	if (NumOfOnes != NUM_OF_ONES_IN_KSV)
		return FALSE;
	
	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: HDCP_Off()
// Function Description: Switch hdcp off.
//------------------------------------------------------------------------------
void HDCP_Off (void)
{
	TX_DEBUG_PRINT(("[MHL]: >> HDCP_Off()\n"));

	// AV MUTE
	ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, AV_MUTE_MASK, AV_MUTE_MUTED);
	WriteByteTPI(TPI_HDCP_CONTROL_DATA_REG, PROTECTION_LEVEL_MIN);

	mhlTxHdcp.HDCP_Started = FALSE;
	mhlTxHdcp.HDCP_Override = FALSE;
	mhlTxHdcp.HDCP_LinkProtectionLevel = EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE;
}

//------------------------------------------------------------------------------
// Function Name: HDCP_On()
// Function Description: Switch hdcp on.
//------------------------------------------------------------------------------
void HDCP_On (void)
{
	if (mhlTxHdcp.HDCP_Override == FALSE)
	{
		TX_DEBUG_PRINT(("[MHL]: >> HDCP_On()\n"));

		WriteByteTPI(TPI_HDCP_CONTROL_DATA_REG, PROTECTION_LEVEL_MAX);

		mhlTxHdcp.HDCP_Started = TRUE;
	}
	else
	{
		mhlTxHdcp.HDCP_Started = FALSE;
	}
}

//------------------------------------------------------------------------------
// Function Name: RestartHDCP()
// Function Description: Restart HDCP.
//------------------------------------------------------------------------------
void RestartHDCP (void)
{
	TX_DEBUG_PRINT (("[MHL]: >> RestartHDCP()\n"));

	SiiMhlTxDrvTmdsControl( FALSE );
	SiiMhlTxTmdsEnable();
}

//------------------------------------------------------------------------------
// Function Name: HDCP_Init()
// Function Description: Tests Tx and Rx support of HDCP. If found, checks if
//                  and attempts to set the security level accordingly.
//
// Accepts: none
// Returns: TRUE if HW TPI started successfully. FALSE if failed to.
// Globals: HDCP_TxSupports - initialized to FALSE, set to TRUE if supported by this device
//		   HDCP_AksvValid - initialized to FALSE, set to TRUE if valid AKSVs are read from this device
//		   HDCP_Started - initialized to FALSE
//		   HDCP_LinkProtectionLevel - initialized to (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE)
//------------------------------------------------------------------------------
void HDCP_Init (void)
{
	TX_DEBUG_PRINT(("[MHL]: >> HDCP_Init()\n"));

	mhlTxHdcp.HDCP_TxSupports = FALSE;
	mhlTxHdcp.HDCP_AksvValid = FALSE;
	mhlTxHdcp.HDCP_Started = FALSE;
	mhlTxHdcp.HDCP_LinkProtectionLevel = EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE;

	// This is TX-related... need only be done once.
    	if (!IsHDCP_Supported())
    	{
		// The TX does not support HDCP, so authentication will never be attempted.
		// Video will be shown as soon as TMDS is enabled.
		TX_DEBUG_PRINT(("HDCP -> TX does not support HDCP\n"));
		return;
	}
	mhlTxHdcp.HDCP_TxSupports = TRUE;

	// This is TX-related... need only be done once.
    	if (!AreAKSV_OK())
    	{
		// The TX supports HDCP, but does not have valid AKSVs.
		// Video will not be shown.
        	TX_DEBUG_PRINT(("HDCP -> Illegal AKSV\n"));
        	return;
    	}
	mhlTxHdcp.HDCP_AksvValid = TRUE;

#ifdef KSVFORWARD
	// Enable the KSV Forwarding feature and the KSV FIFO Intererrupt
	ReadModifyWriteTPI(TPI_HDCP_CONTROL_DATA_REG, KSV_FORWARD_MASK, KSV_FORWARD_ENABLE);
	ReadModifyWriteTPI(TPI_KSV_FIFO_READY_INT_EN, KSV_FIFO_READY_EN_MASK, KSV_FIFO_READY_ENABLE);
#endif

	TX_DEBUG_PRINT(("HDCP -> Supported by TX, AKSVs valid\n"));
}

#ifdef READKSV
//------------------------------------------------------------------------------
// Function Name: IsRepeater()
// Function Description: Test if sink is a repeater.
//
// Accepts: none
// Returns: TRUE if sink is a repeater. FALSE if not.
// Globals: none
//------------------------------------------------------------------------------
uint8_t IsRepeater (void)
{
    	uint8_t RegImage;

	TX_DEBUG_PRINT(("[MHL]: >> IsRepeater()\n"));

    	RegImage = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);

   	 if (RegImage & HDCP_REPEATER_MASK)
        	return TRUE;

    	return FALSE;           // not a repeater
}

//------------------------------------------------------------------------------
// Function Name: ReadBlockHDCP()
// Function Description: Read NBytes from offset Addr of the HDCP slave address
//                      into a uint8_t Buffer pointed to by Data
//
// Accepts: HDCP port offset, number of bytes to read and a pointer to the data buffer where 
//               the data read will be saved
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void ReadBlockHDCP (uint8_t TPI_Offset, uint8_t NBytes, uint8_t * pData)
{
	I2CReadBlock(HDCP_SLAVE_ADDR, TPI_Offset, NBytes, pData);
}

//------------------------------------------------------------------------------
// Function Name: GetKSV()
// Function Description: Collect all downstrean KSV for verification.
//
// Accepts: none
// Returns: TRUE if KSVs collected successfully. False if not.
// Globals: KSV_Array[], The buffer is limited to KSV_ARRAY_SIZE due to the 8051 implementation.
//------------------------------------------------------------------------------
uint8_t GetKSV (void)
{
	uint8_t i;
    	uint16_t KeyCount;
    	uint8_t KSV_Array[KSV_ARRAY_SIZE];

	TX_DEBUG_PRINT(("[MHL]: >> GetKSV()\n"));
    	ReadBlockHDCP(DDC_BSTATUS_ADDR_L, 1, &i);
    	KeyCount = (i & DEVICE_COUNT_MASK) * 5;
	if (KeyCount != 0)
	{
		ReadBlockHDCP(DDC_KSV_FIFO_ADDR, KeyCount, KSV_Array);
	}

	/*
	TX_DEBUG_PRINT(("KeyCount = %d\n", (int) KeyCount));
	for (i=0; i<KeyCount; i++)
	{
		TX_DEBUG_PRINT(("KSV[%2d] = %2.2X\n", (int) i, (int) KSV_Array[i]));
	}
	*/

	 return TRUE;
}
#endif

//------------------------------------------------------------------------------
// Function Name: HDCP_CheckStatus()
// Function Description: Check HDCP status.
//
// Accepts: InterruptStatus
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void HDCP_CheckStatus (uint8_t InterruptStatusImage)
{
	uint8_t QueryData;
	uint8_t LinkStatus;
	uint8_t RegImage;
	uint8_t NewLinkProtectionLevel;
	
#ifdef READKSV
	uint8_t RiCnt;
#endif
#ifdef KSVFORWARD
	uint8_t ksv;
#endif

	if ((mhlTxHdcp.HDCP_TxSupports == TRUE) && (mhlTxHdcp.HDCP_AksvValid == TRUE))
	{
		if ((mhlTxHdcp.HDCP_LinkProtectionLevel == (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE)) && (mhlTxHdcp.HDCP_Started == FALSE))
		{
			QueryData = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);

			if (QueryData & PROTECTION_TYPE_MASK)   // Is HDCP avaialable
			{
				HDCP_On();
			}
		}

		// Check if Link Status has changed:
		if (InterruptStatusImage & SECURITY_CHANGE_EVENT) 
		{
			TX_DEBUG_PRINT (("HDCP -> "));

			LinkStatus = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);
			LinkStatus &= LINK_STATUS_MASK;

			ClearInterrupt(SECURITY_CHANGE_EVENT);

			switch (LinkStatus)
			{
				case LINK_STATUS_NORMAL:
					TX_DEBUG_PRINT (("Link = Normal\n"));
					break;

				case LINK_STATUS_LINK_LOST:
					TX_DEBUG_PRINT (("Link = Lost\n"));
					RestartHDCP();
					break;

				case LINK_STATUS_RENEGOTIATION_REQ:
					TX_DEBUG_PRINT (("Link = Renegotiation Required\n"));
					HDCP_Off();
					HDCP_On();
					break;

				case LINK_STATUS_LINK_SUSPENDED:
					TX_DEBUG_PRINT (("Link = Suspended\n"));
					HDCP_On();
					break;
			}
		}

		// Check if HDCP state has changed:
		if (InterruptStatusImage & HDCP_CHANGE_EVENT)
		{
			RegImage = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);

			NewLinkProtectionLevel = RegImage & (EXTENDED_LINK_PROTECTION_MASK | LOCAL_LINK_PROTECTION_MASK);
			if (NewLinkProtectionLevel != mhlTxHdcp.HDCP_LinkProtectionLevel)
			{
				TX_DEBUG_PRINT (("HDCP -> "));

				mhlTxHdcp.HDCP_LinkProtectionLevel = NewLinkProtectionLevel;

				switch (mhlTxHdcp.HDCP_LinkProtectionLevel)
				{
					case (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE):
						TX_DEBUG_PRINT (("Protection = None\n"));
						RestartHDCP();
						break;

					case LOCAL_LINK_PROTECTION_SECURE:

						if (IsHDMI_Sink())
						{
							ReadModifyWriteTPI(TPI_AUDIO_INTERFACE_REG, AUDIO_MUTE_MASK, AUDIO_MUTE_NORMAL);
						}

						ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, AV_MUTE_MASK, AV_MUTE_NORMAL);
						TX_DEBUG_PRINT (("Protection = Local, Video Unmuted\n"));
						break;

					case (EXTENDED_LINK_PROTECTION_SECURE | LOCAL_LINK_PROTECTION_SECURE):
						TX_DEBUG_PRINT (("Protection = Extended\n"));
#ifdef READKSV
 						if (IsRepeater())
						{
							RiCnt = ReadIndexedRegister(INDEXED_PAGE_0, 0x25);
							while (RiCnt > 0x70)  // Frame 112
							{
								RiCnt = ReadIndexedRegister(INDEXED_PAGE_0, 0x25);
							}
							ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, 0x06, 0x06);
							GetKSV();
							RiCnt = ReadByteTPI(TPI_SYSTEM_CONTROL_DATA_REG);
							ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, 0x08, 0x00);
						}
#endif
						break;

					default:
						TX_DEBUG_PRINT (("Protection = Extended but not Local?\n"));
						RestartHDCP();
						break;
				}
			}

#ifdef KSVFORWARD			
			// Check if KSV FIFO is ready and forward - Bug# 17892
			// If interrupt never goes off:
		 	//   a) KSV formwarding is not enabled
			//   b) not a repeater
			//   c) a repeater with device count == 0
			// and therefore no KSV list to forward
			if ((ReadByteTPI(TPI_KSV_FIFO_READY_INT) & KSV_FIFO_READY_MASK) == KSV_FIFO_READY_YES)
			{
				ReadModifyWriteTPI(TPI_KSV_FIFO_READY_INT, KSV_FIFO_READY_MASK, KSV_FIFO_READY_YES);
				TX_DEBUG_PRINT (("KSV Fwd: KSV FIFO has data...\n"));
				{
					// While !(last uint8_t has been read from KSV FIFO)
					// if (count = 0) then a uint8_t is not in the KSV FIFO yet, do not read
					//   else read a uint8_t from the KSV FIFO and forward it or keep it for revocation check
					do 
					{
						ksv = ReadByteTPI(TPI_KSV_FIFO_STATUS_REG);
						if (ksv & KSV_FIFO_COUNT_MASK)
						{
							TX_DEBUG_PRINT (("KSV Fwd: KSV FIFO Count = %d, ", (int)(ksv & KSV_FIFO_COUNT_MASK)));
							ksv = ReadByteTPI(TPI_KSV_FIFO_VALUE_REG);	// Forward or store for revocation check
							TX_DEBUG_PRINT (("Value = %d\n", (int)ksv));
						}
					} while ((ksv & KSV_FIFO_LAST_MASK) == KSV_FIFO_LAST_NO);
					TX_DEBUG_PRINT (("KSV Fwd: Last KSV FIFO forward complete\n"));
				}
			}
#endif
			ClearInterrupt(HDCP_CHANGE_EVENT);
		}
	}
}
#endif



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////             AV CONFIG              ///////////////////////////////
///////////////////////*************************///////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Video mode table
//------------------------------------------------------------------------------
typedef struct
{
    uint8_t Mode_C1;
    uint8_t Mode_C2;
    uint8_t SubMode;
} ModeIdType;

typedef struct
{
    uint16_t Pixels;
    uint16_t Lines;
} PxlLnTotalType;

typedef struct
{
    uint16_t H;
    uint16_t V;
} HVPositionType;

typedef struct
{
    uint16_t H;
    uint16_t V;
} HVResolutionType;

typedef struct
{
    uint8_t           	RefrTypeVHPol;
    uint16_t           	VFreq;
    PxlLnTotalType 	Total;
} TagType;

typedef struct
{
    uint8_t IntAdjMode;
    uint16_t HLength;
    uint8_t VLength;
    uint16_t Top;
    uint16_t Dly;
    uint16_t HBit2HSync;
    uint8_t VBit2VSync;
    uint16_t Field2Offset;
}  _656Type;

typedef struct
{
    uint8_t VactSpace1;
    uint8_t VactSpace2;
    uint8_t Vblank1;
    uint8_t Vblank2;
    uint8_t Vblank3;
} Vspace_Vblank;

//
// WARNING!  The entries in this enum must remian in the samre order as the PC Codes part
// of the VideoModeTable[].
//
typedef	enum
{
    PC_640x350_85_08 = 0,
    PC_640x400_85_08,
    PC_720x400_70_08,
    PC_720x400_85_04,
    PC_640x480_59_94,
    PC_640x480_72_80,
    PC_640x480_75_00,
    PC_640x480_85_00,
    PC_800x600_56_25,
    PC_800x600_60_317,
    PC_800x600_72_19,
    PC_800x600_75,
    PC_800x600_85_06,
    PC_1024x768_60,
    PC_1024x768_70_07,
    PC_1024x768_75_03,
    PC_1024x768_85,
    PC_1152x864_75,
    PC_1600x1200_60,
    PC_1280x768_59_95,
    PC_1280x768_59_87,
    PC_280x768_74_89,
    PC_1280x768_85,
    PC_1280x960_60,
    PC_1280x960_85,
    PC_1280x1024_60,
    PC_1280x1024_75,
    PC_1280x1024_85,
    PC_1360x768_60,
    PC_1400x105_59_95,
    PC_1400x105_59_98,
    PC_1400x105_74_87,
    PC_1400x105_84_96,
    PC_1600x1200_65,
    PC_1600x1200_70,
    PC_1600x1200_75,
    PC_1600x1200_85,
    PC_1792x1344_60,
    PC_1792x1344_74_997,
    PC_1856x1392_60,
    PC_1856x1392_75,
    PC_1920x1200_59_95,
    PC_1920x1200_59_88,
    PC_1920x1200_74_93,
    PC_1920x1200_84_93,
    PC_1920x1440_60,
    PC_1920x1440_75,
    PC_12560x1440_60,
    PC_SIZE			// Must be last
} PcModeCode_t;

typedef struct
{
    ModeIdType       	ModeId;
    uint16_t             		PixClk;
    TagType          		Tag;
    HVPositionType  	Pos;
    HVResolutionType 	Res;
    uint8_t             		AspectRatio;
    _656Type         		_656;
    uint8_t             		PixRep;
    Vspace_Vblank 		VsVb;
    uint8_t             		_3D_Struct;
} VModeInfoType;

#define NSM                     0   // No Sub-Mode

#define	DEFAULT_VIDEO_MODE		0	// 640  x 480p @ 60 VGA

#define ProgrVNegHNeg           0x00
#define ProgrVNegHPos           	0x01
#define ProgrVPosHNeg           	0x02
#define ProgrVPosHPos           	0x03

#define InterlaceVNegHNeg   	0x04
#define InterlaceVPosHNeg      0x05
#define InterlaceVNgeHPos    	0x06
#define InterlaceVPosHPos     	0x07

#define VIC_BASE                	0
#define HDMI_VIC_BASE           43
#define VIC_3D_BASE             	47
#define PC_BASE                 	64

// Aspect ratio
//=================================================
#define R_4                      		0   // 4:3
#define R_4or16                  	1   // 4:3 or 16:9
#define R_16                     		2   // 16:9

//
// These are the VIC codes that we support in a 3D mode
//
#define VIC_FOR_480P_60Hz_4X3			2		// 720p x 480p @60Hz
#define VIC_FOR_480P_60Hz_16X9			3		// 720p x 480p @60Hz
#define VIC_FOR_720P_60Hz				4		// 1280 x 720p @60Mhz
#define VIC_FOR_1080i_60Hz				5		// 1920 x 1080i @60Mhz
#define VIC_FOR_1080p_60Hz				16		// 1920 x 1080i @60hz
#define VIC_FOR_720P_50Hz				19		// 1280 x 720p @50Mhz
#define VIC_FOR_1080i_50Hz				20		// 1920 x 1080i @50Mhz
#define VIC_FOR_1080p_50Hz				31		// 1920 x 720p @50Hz
#define VIC_FOR_1080p_24Hz				32		// 1920 x 720p @24Hz


static const VModeInfoType VModesTable[] =
{
	//===================================================================================================================================================================================================================================
    //         VIC                  Refresh type Refresh-Rate Pixel-Totals  Position     Active     Aspect   Int  Length          Hbit  Vbit  Field  Pixel          Vact Space/Blank
    //        1   2  SubM   PixClk  V/H Position       VFreq   H      V      H    V       H    V    Ratio    Adj  H   V  Top  Dly HSync VSync Offset Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{        1,  0, NSM},  2517,  {ProgrVNegHNeg,     6000, { 800,  525}}, {144, 35}, { 640, 480}, R_4,     {0,  96, 2, 33,  48,  16,  10,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 0 - 1.       640  x 480p @ 60 VGA
    {{        2,  3, NSM},  2700,  {ProgrVNegHNeg,     6000, { 858,  525}}, {122, 36}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  19,   9,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 1 - 2,3      720  x 480p
    {{        4,  0, NSM},  7425,  {ProgrVPosHPos,     6000, {1650,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 110,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 2 - 4        1280 x 720p@60Hz
    {{        5,  0, NSM},  7425,  {InterlaceVPosHPos, 6000, {2200,  562}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 148,  88,   2, 1100},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 3 - 5        1920 x 1080i
    {{        6,  7, NSM},  2700,  {InterlaceVNegHNeg, 6000, {1716,  264}}, {119, 18}, { 720, 480}, R_4or16, {3,  62, 3, 15, 114,  17,   5,  429},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 4 - 6,7      1440 x 480i,pix repl
    {{        8,  9,   1},  2700,  {ProgrVNegHNeg,     6000, {1716,  262}}, {119, 18}, {1440, 240}, R_4or16, {0, 124, 3, 15, 114,  38,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 5 - 8,9(1)   1440 x 240p
    {{        8,  9,   2},  2700,  {ProgrVNegHNeg,     6000, {1716,  263}}, {119, 18}, {1440, 240}, R_4or16, {0, 124, 3, 15, 114,  38,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 6 - 8,9(2)   1440 x 240p
    {{       10, 11, NSM},  5400,  {InterlaceVNegHNeg, 6000, {3432,  525}}, {238, 18}, {2880, 480}, R_4or16, {0, 248, 3, 15, 228,  76,   4, 1716},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 7 - 10,11    2880 x 480i
    {{       12, 13,   1},  5400,  {ProgrVNegHNeg,     6000, {3432,  262}}, {238, 18}, {2880, 240}, R_4or16, {0, 248, 3, 15, 228,  76,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 8 - 12,13(1) 2880 x 240p
    {{       12, 13,   2},  5400,  {ProgrVNegHNeg,     6000, {3432,  263}}, {238, 18}, {2880, 240}, R_4or16, {0, 248, 3, 15, 228,  76,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 9 - 12,13(2) 2880 x 240p
    {{       14, 15, NSM},  5400,  {ProgrVNegHNeg,     6000, {1716,  525}}, {244, 36}, {1440, 480}, R_4or16, {0, 124, 6, 30, 120,  32,   9,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 10 - 14,15    1440 x 480p
    {{       16,  0, NSM}, 14835,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 11 - 16       1920 x 1080p
    {{       17, 18, NSM},  2700,  {ProgrVNegHNeg,     5000, { 864,  625}}, {132, 44}, { 720, 576}, R_4or16, {0,  64, 5, 39,  68,  12,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 12 - 17,18    720  x 576p
    {{       19,  0, NSM},  7425,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 13 - 19       1280 x 720p@50Hz
    {{       20,  0, NSM},  7425,  {InterlaceVPosHPos, 5000, {2640, 1125}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1320},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 14 - 20       1920 x 1080i
    {{       21, 22, NSM},  2700,  {InterlaceVNegHNeg, 5000, {1728,  625}}, {132, 22}, { 720, 576}, R_4,     {3,  63, 3, 19, 138,  24,   2,  432},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 15 - 21,22    1440 x 576i
    {{       23, 24,   1},  2700,  {ProgrVNegHNeg,     5000, {1728,  312}}, {132, 22}, {1440, 288}, R_4or16, {0, 126, 3, 19, 138,  24,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 16 - 23,24(1) 1440 x 288p
    {{       23, 24,   2},  2700,  {ProgrVNegHNeg,     5000, {1728,  313}}, {132, 22}, {1440, 288}, R_4or16, {0, 126, 3, 19, 138,  24,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 17 - 23,24(2) 1440 x 288p
    {{       23, 24,   3},  2700,  {ProgrVNegHNeg,     5000, {1728,  314}}, {132, 22}, {1440, 288}, R_4or16, {0, 126, 3, 19, 138,  24,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 18 - 23,24(3) 1440 x 288p
    {{       25, 26, NSM},  5400,  {InterlaceVNegHNeg, 5000, {3456,  625}}, {264, 22}, {2880, 576}, R_4or16, {0, 252, 3, 19, 276,  48,   2, 1728},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 19 - 25,26    2880 x 576i
    {{       27, 28,   1},  5400,  {ProgrVNegHNeg,     5000, {3456,  312}}, {264, 22}, {2880, 288}, R_4or16, {0, 252, 3, 19, 276,  48,   2,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 20 - 27,28(1) 2880 x 288p
    {{       27, 28,   2},  5400,  {ProgrVNegHNeg,     5000, {3456,  313}}, {264, 22}, {2880, 288}, R_4or16, {0, 252, 3, 19, 276,  48,   3,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 21 - 27,28(2) 2880 x 288p
    {{       27, 28,   3},  5400,  {ProgrVNegHNeg,     5000, {3456,  314}}, {264, 22}, {2880, 288}, R_4or16, {0, 252, 3, 19, 276,  48,   4,    0},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 22 - 27,28(3) 2880 x 288p
    {{       29, 30, NSM},  5400,  {ProgrVPosHNeg,     5000, {1728,  625}}, {264, 44}, {1440, 576}, R_4or16, {0, 128, 5, 39, 136,  24,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 23 - 29,30    1440 x 576p
    {{       31,  0, NSM}, 14850,  {ProgrVPosHPos,     5000, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 24 - 31(1)    1920 x 1080p
    {{       32,  0, NSM},  7417,  {ProgrVPosHPos,     2400, {2750, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 638,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 25 - 32(2)    1920 x 1080p@24Hz
    {{       33,  0, NSM},  7425,  {ProgrVPosHPos,     2500, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 26 - 33(3)    1920 x 1080p
    {{       34,  0, NSM},  7417,  {ProgrVPosHPos,     3000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 27 - 34(4)    1920 x 1080p
    {{       35, 36, NSM}, 10800,  {ProgrVNegHNeg,     5994, {3432,  525}}, {488, 36}, {2880, 480}, R_4or16, {0, 248, 6, 30, 240,  64,  10,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 28 - 35, 36   2880 x 480p@59.94/60Hz
    {{       37, 38, NSM}, 10800,  {ProgrVNegHNeg,     5000, {3456,  625}}, {272, 39}, {2880, 576}, R_4or16, {0, 256, 5, 40, 272,  48,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 29 - 37, 38   2880 x 576p@50Hz
    {{       39,  0, NSM},  7200,  {InterlaceVNegHNeg, 5000, {2304, 1250}}, {352, 62}, {1920,1080}, R_16,    {0, 168, 5, 87, 184,  32,  24,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 30 - 39       1920 x 1080i@50Hz
    {{       40,  0, NSM}, 14850,  {InterlaceVPosHPos, 10000,{2640, 1125}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1320},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 31 - 40       1920 x 1080i@100Hz
    {{       41,  0, NSM}, 14850,  {InterlaceVPosHPos, 10000,{1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 400,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 32 - 41       1280 x 720p@100Hz
    {{       42, 43, NSM},  5400,  {ProgrVNegHNeg,     10000,{ 864,  144}}, {132, 44}, { 720, 576}, R_4or16, {0,  64, 5, 39,  68,  12,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 33 - 42, 43,  720p x 576p@100Hz
    {{       44, 45, NSM},  5400,  {InterlaceVNegHNeg, 10000,{ 864,  625}}, {132, 22}, { 720, 576}, R_4or16, {0,  63, 3, 19,  69,  12,   2,  432},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 34 - 44, 45,  720p x 576i@100Hz, pix repl
    {{       46,  0, NSM}, 14835,  {InterlaceVPosHPos, 11988,{2200, 1125}}, {192, 20}, {1920,1080}, R_16,    {0,  44, 5, 15, 149,  88,   2, 1100},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 35 - 46,      1920 x 1080i@119.88/120Hz
    {{       47,  0, NSM}, 14835,  {ProgrVPosHPos,     11988,{1650,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 110,   5, 1100},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 36 - 47,      1280 x 720p@119.88/120Hz
    {{       48, 49, NSM},  5400,  {ProgrVNegHNeg,     11988,{ 858,  525}}, {122, 36}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  16,  10,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 37 - 48, 49   720  x 480p@119.88/120Hz
    {{       50, 51, NSM},  5400,  {InterlaceVNegHNeg, 11988,{ 858,  525}}, {119, 18}, { 720, 480}, R_4or16, {0,  62, 3, 15,  57,  19,   4,  429},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 38 - 50, 51   720  x 480i@119.88/120Hz
    {{       52, 53, NSM}, 10800,  {ProgrVNegHNeg,     20000,{ 864,  625}}, {132, 44}, { 720, 576}, R_4or16, {0,  64, 5, 39,  68,  12,   5,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 39 - 52, 53,  720  x 576p@200Hz
    {{       54, 55, NSM}, 10800,  {InterlaceVNegHNeg, 20000,{ 864,  625}}, {132, 22}, { 720, 576}, R_4or16, {0,  63, 3, 19,  69,  12,   2,  432},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 40 - 54, 55,  1440 x 576i @200Hz, pix repl
    {{       56, 57, NSM}, 10800,  {ProgrVNegHNeg,     24000,{ 858,  525}}, {122, 42}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  16,   9,    0},    0,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 41 - 56, 57,  720  x 480p @239.76/240Hz
    {{       58, 59, NSM}, 10800,  {InterlaceVNegHNeg, 24000,{ 858,  525}}, {119, 18}, { 720, 480}, R_4or16, {0,  62, 3, 15,  57,  19,   4,  429},    1,    {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 42 - 58, 59,  1440 x 480i @239.76/240Hz, pix repl

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 4K x 2K Modes:
	//===================================================================================================================================================================================================================================
    //                                                                                                            Pulse
    //         VIC                  Refresh type Refresh-Rate Pixel-Totals   Position     Active    Aspect   Int  Width           Hbit  Vbit  Field  Pixel          Vact Space/Blank
    //        1   2  SubM   PixClk  V/H Position       VFreq   H      V      H    V       H    V    Ratio    Adj  H   V  Top  Dly HSync VSync Offset Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{        1,  0, NSM}, 29700, {ProgrVNegHNeg,     30000,{4400, 2250}}, {384, 82}, {3840,2160}, R_16,    {0,  88, 10, 72, 296, 176,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 43 - 4k x 2k 29.97/30Hz (297.000 MHz)
    {{        2,  0, NSM}, 29700, {ProgrVNegHNeg,     29700,{5280, 2250}}, {384, 82}, {3840,2160}, R_16,    {0,  88, 10, 72, 296,1056,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 44 - 4k x 2k 25Hz
    {{        3,  0, NSM}, 29700, {ProgrVNegHNeg,     24000,{5500, 2250}}, {384, 82}, {3840,2160}, R_16,    {0,  88, 10, 72, 296,1276,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 45 - 4k x 2k 24Hz (297.000 MHz)
    {{        4  ,0, NSM}, 29700, {ProgrVNegHNeg,     24000,{6500, 2250}}, {384, 82}, {4096,2160}, R_16,    {0,  88, 10, 72, 296,1020,  8,    0},   0,     {0,     0,     0,     0,    0},    NO_3D_SUPPORT}, // 46 - 4k x 2k 24Hz (SMPTE)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 3D Modes:
	//===================================================================================================================================================================================================================================
    //                                                                                                            Pulse
    //         VIC                  Refresh type Refresh-Rate Pixel-Totals  Position      Active    Aspect   Int  Width           Hbit  Vbit  Field  Pixel          Vact Space/Blank
    //        1   2  SubM   PixClk  V/H Position       VFreq   H      V      H    V       H    V    Ratio    Adj  H   V  Top  Dly HSync VSync Offset Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{        2,  3, NSM},  2700,  {ProgrVPosHPos,     6000, {858,   525}}, {122, 36}, { 720, 480}, R_4or16, {0,  62, 6, 30,  60,  16,   9,    0},    0,    {0,     0,     0,     0,    0},     8}, // 47 - 3D, 2,3 720p x 480p /60Hz, Side-by-Side (Half)
    {{        4,  0, NSM}, 14850,  {ProgrVPosHPos,     6000, {1650,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 110,   5,    0},    0,    {0,     0,     0,     0,    0},     0}, // 48 - 3D  4   1280 x 720p@60Hz,  Frame Packing
    {{        5,  0, NSM}, 14850,  {InterlaceVPosHPos, 6000, {2200,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148,  88,   2, 1100},    0,    {23,   22,     0,     0,    0},     0}, // 49 - 3D, 5,  1920 x 1080i/60Hz, Frame Packing
    {{        5,  0, NSM}, 14850,  {InterlaceVPosHPos, 6000, {2200,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148,  88,   2, 1100},    0,    {0,     0,    22,    22,   23},     1}, // 50 - 3D, 5,  1920 x 1080i/60Hz, Field Alternative
    {{       16,  0, NSM}, 29700,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     0}, // 51 - 3D, 16, 1920 x 1080p/60Hz, Frame Packing
    {{       16,  0, NSM}, 29700,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     2}, // 52 - 3D, 16, 1920 x 1080p/60Hz, Line Alternative
    {{       16,  0, NSM}, 29700,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     3}, // 53 - 3D, 16, 1920 x 1080p/60Hz, Side-by-Side (Full)
    {{       16,  0, NSM}, 14850,  {ProgrVPosHPos,     6000, {2200, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148,  88,   4,    0},    0,    {0,     0,     0,     0,    0},     8}, // 54 - 3D, 16, 1920 x 1080p/60Hz, Side-by-Side (Half)
    {{       19,  0, NSM}, 14850,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},     0}, // 55 - 3D, 19, 1280 x 720p@50Hz,  Frame Packing
    {{       19,  0, NSM}, 14850,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},     4}, // 56 - 3D, 19, 1280 x 720p/50Hz,  (L + depth)
    {{       19,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {1980,  750}}, {260, 25}, {1280, 720}, R_16,    {0,  40, 5, 20, 220, 440,   5,    0},    0,    {0,     0,     0,     0,    0},     5}, // 57 - 3D, 19, 1280 x 720p/50Hz,  (L + depth + Gfx + G-depth)
    {{       20,  0, NSM}, 14850,  {InterlaceVPosHPos, 5000, {2640,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1220},    0,    {23,   22,     0,     0,    0},     0}, // 58 - 3D, 20, 1920 x 1080i/50Hz, Frame Packing
    {{       20,  0, NSM}, 14850,  {InterlaceVPosHPos, 5000, {2640,  562}}, {192, 20}, {1920, 540}, R_16,    {0,  44, 5, 15, 148, 528,   2, 1220},    0,    {0,     0,    22,    22,   23},     1}, // 59 - 3D, 20, 1920 x 1080i/50Hz, Field Alternative
    {{       31,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},     0}, // 60 - 3D, 31, 1920 x 1080p/50Hz, Frame Packing
    {{       31,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {2640, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},     2}, // 61 - 3D, 31, 1920 x 1080p/50Hz, Line Alternative
    {{       31,  0, NSM}, 29700,  {ProgrVPosHPos,     5000, {2650, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 528,   4,    0},    0,    {0,     0,     0,     0,    0},     3}, // 62 - 3D, 31, 1920 x 1080p/50Hz, Side-by-Side (Full)
    {{       32,  0, NSM}, 14850,  {ProgrVPosHPos,     2400, {2750, 1125}}, {192, 41}, {1920,1080}, R_16,    {0,  44, 5, 36, 148, 638,   4,    0},    0,    {0,     0,     0,     0,    0},     0}, // 63 - 3D, 32, 1920 x 1080p@24Hz, Frame Packing

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// NOTE: DO NOT ATTEMPT INPUT RESOLUTIONS THAT REQUIRE PIXEL CLOCK FREQUENCIES HIGHER THAN THOSE SUPPORTED BY THE TRANSMITTER CHIP

	//===================================================================================================================================================================================================================================
	//                                                                                                              Sync Pulse
    //  VIC                          Refresh type   fresh-Rate  Pixel-Totals    Position    Active     Aspect   Int  Width            Hbit  Vbit  Field  Pixel          Vact Space/Blank
    // 1   2  SubM         PixClk    V/H Position       VFreq   H      V        H    V       H    V     Ratio   {Adj  H   V  Top  Dly HSync VSync Offset} Repl  Space1 Space2 Blank1 Blank2 Blank3  3D
    //===================================================================================================================================================================================================================================
    {{PC_BASE  , 0,NSM},    3150,   {ProgrVNegHPos,     8508,   {832, 445}},    {160,63},   {640,350},   R_16,  {0,  64,  3,  60,  96,  32,  32,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 64 - 640x350@85.08
    {{PC_BASE+1, 0,NSM},    3150,   {ProgrVPosHNeg,     8508,   {832, 445}},    {160,44},   {640,400},   R_16,  {0,  64,  3,  41,  96,  32,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 65 - 640x400@85.08
    {{PC_BASE+2, 0,NSM},    2700,   {ProgrVPosHNeg,     7008,   {900, 449}},    {0,0},      {720,400},   R_16,  {0,   0,  0,   0,   0,   0,   0,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 66 - 720x400@70.08
    {{PC_BASE+3, 0,NSM},    3500,   {ProgrVPosHNeg,     8504,   {936, 446}},    {20,45},    {720,400},   R_16,  {0,  72,  3,  42, 108,  36,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 67 - 720x400@85.04
    {{PC_BASE+4, 0,NSM},    2517,   {ProgrVNegHNeg,     5994,   {800, 525}},    {144,35},   {640,480},   R_4,   {0,  96,  2,  33,  48,  16,  10,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 68 - 640x480@59.94
    {{PC_BASE+5, 0,NSM},    3150,   {ProgrVNegHNeg,     7281,   {832, 520}},    {144,31},   {640,480},   R_4,   {0,  40,  3,  28, 128, 128,   9,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 69 - 640x480@72.80
    {{PC_BASE+6, 0,NSM},    3150,   {ProgrVNegHNeg,     7500,   {840, 500}},    {21,19},    {640,480},   R_4,   {0,  64,  3,  28, 128,  24,   9,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 70 - 640x480@75.00
    {{PC_BASE+7,0,NSM},     3600,   {ProgrVNegHNeg,     8500,   {832, 509}},    {168,28},   {640,480},   R_4,   {0,  56,  3,  25, 128,  24,   9,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 71 - 640x480@85.00
    {{PC_BASE+8,0,NSM},     3600,   {ProgrVPosHPos,     5625,   {1024, 625}},   {200,24},   {800,600},   R_4,   {0,  72,  2,  22, 128,  24,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 72 - 800x600@56.25
    {{PC_BASE+9,0,NSM},     4000,   {ProgrVPosHPos,     6032,   {1056, 628}},   {216,27},   {800,600},   R_4,   {0, 128,  4,  23,  88,  40,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 73 - 800x600@60.317
    {{PC_BASE+10,0,NSM},    5000,   {ProgrVPosHPos,     7219,   {1040, 666}},   {184,29},   {800,600},   R_4,   {0, 120,  6,  23,  64,  56,  37,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 74 - 800x600@72.19
    {{PC_BASE+11,0,NSM},    4950,   {ProgrVPosHPos,     7500,   {1056, 625}},   {240,24},   {800,600},   R_4,   {0,  80,  3,  21, 160,  16,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 75 - 800x600@75
    {{PC_BASE+12,0,NSM},    5625,   {ProgrVPosHPos,     8506,   {1048, 631}},   {216,30},   {800,600},   R_4,   {0,  64,  3,  27, 152,  32,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 76 - 800x600@85.06
    {{PC_BASE+13,0,NSM},    6500,   {ProgrVNegHNeg,     6000,   {1344, 806}},   {296,35},   {1024,768},  R_4,   {0, 136,  6,  29, 160,  24,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 77 - 1024x768@60
    {{PC_BASE+14,0,NSM},    7500,   {ProgrVNegHNeg,     7007,   {1328, 806}},   {280,35},   {1024,768},  R_4,   {0, 136,  6,  19, 144,  24,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 78 - 1024x768@70.07
    {{PC_BASE+15,0,NSM},    7875,   {ProgrVPosHPos,     7503,   {1312, 800}},   {272,31},   {1024,768},  R_4,   {0,  96,  3,  28, 176,  16,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 79 - 1024x768@75.03
    {{PC_BASE+16,0,NSM},    9450,   {ProgrVPosHPos,     8500,   {1376, 808}},   {304,39},   {1024,768},  R_4,   {0,  96,  3,  36, 208,  48,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 80 - 1024x768@85
    {{PC_BASE+17,0,NSM},   10800,   {ProgrVPosHPos,     7500,   {1600, 900}},   {384,35},   {1152,864},  R_4,   {0, 128,  3,  32, 256,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 81 - 1152x864@75
    {{PC_BASE+18,0,NSM},   16200,   {ProgrVPosHPos,     6000,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 304,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 82 - 1600x1200@60
    {{PC_BASE+19,0,NSM},    6825,   {ProgrVNegHPos,     6000,   {1440, 790}},   {112,19},   {1280,768},  R_16,  {0,  32,  7,  12,  80,  48,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 83 - 1280x768@59.95
    {{PC_BASE+20,0,NSM},    7950,   {ProgrVPosHNeg,     5987,   {1664, 798}},   {320,27},   {1280,768},  R_16,  {0, 128,  7,  20, 192,  64,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 84 - 1280x768@59.87
    {{PC_BASE+21,0,NSM},   10220,   {ProgrVPosHNeg,     6029,   {1696, 805}},   {320,27},   {1280,768},  R_16,  {0, 128,  7,  27, 208,  80,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 85 - 1280x768@74.89
    {{PC_BASE+22,0,NSM},   11750,   {ProgrVPosHNeg,     8484,   {1712, 809}},   {352,38},   {1280,768},  R_16,  {0, 136,  7,  31, 216,  80,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 86 - 1280x768@85
    {{PC_BASE+23,0,NSM},   10800,   {ProgrVPosHPos,     6000,   {1800, 1000}},  {424,39},   {1280,960},  R_4,   {0, 112,  3,  36, 312,  96,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 87 - 1280x960@60
    {{PC_BASE+24,0,NSM},   14850,   {ProgrVPosHPos,     8500,   {1728, 1011}},  {384,50},   {1280,960},  R_4,   {0, 160,  3,  47, 224,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 88 - 1280x960@85
    {{PC_BASE+25,0,NSM},   10800,   {ProgrVPosHPos,     6002,   {1688, 1066}},  {360,41},   {1280,1024}, R_4,   {0, 112,  3,  38, 248,  48,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 89 - 1280x1024@60
    {{PC_BASE+26,0,NSM},   13500,   {ProgrVPosHPos,     7502,   {1688, 1066}},  {392,41},   {1280,1024}, R_4,   {0, 144,  3,  38, 248,  16,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 90 - 1280x1024@75
    {{PC_BASE+27,0,NSM},   15750,   {ProgrVPosHPos,     8502,   {1728, 1072}},  {384,47},   {1280,1024}, R_4,   {0, 160,  3,   4, 224,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 91 - 1280x1024@85
    {{PC_BASE+28,0,NSM},    8550,   {ProgrVPosHPos,     6002,   {1792, 795}},   {368,24},   {1360,768},  R_16,  {0, 112,  6,  18, 256,  64,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 92 - 1360x768@60
    {{PC_BASE+29,0,NSM},   10100,   {ProgrVNegHPos,     5995,   {1560, 1080}},  {112,27},   {1400,1050}, R_4,   {0,  32,  4,  23,  80,  48,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 93 - 1400x105@59.95
    {{PC_BASE+30,0,NSM},   12175,   {ProgrVPosHNeg,     5998,   {1864, 1089}},  {376,36},   {1400,1050}, R_4,   {0, 144,  4,  32, 232,  88,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 94 - 1400x105@59.98
    {{PC_BASE+31,0,NSM},   15600,   {ProgrVPosHNeg,     7487,   {1896, 1099}},  {392,46},   {1400,1050}, R_4,   {0, 144,  4,  22, 248, 104,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 95 - 1400x105@74.87
    {{PC_BASE+32,0,NSM},   17950,   {ProgrVPosHNeg,     8496,   {1912, 1105}},  {408,52},   {1400,1050}, R_4,   {0, 152,  4,  48, 256, 104,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 96 - 1400x105@84.96
    {{PC_BASE+33,0,NSM},   17550,   {ProgrVPosHPos,     6500,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 97 - 1600x1200@65
    {{PC_BASE+34,0,NSM},   18900,   {ProgrVPosHPos,     7000,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 98 - 1600x1200@70
    {{PC_BASE+35,0,NSM},   20250,   {ProgrVPosHPos,     7500,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 99 - 1600x1200@75
    {{PC_BASE+36,0,NSM},   22950,   {ProgrVPosHPos,     8500,   {2160, 1250}},  {496,49},   {1600,1200}, R_4,   {0, 192,  3,  46, 304,  64,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 100 - 1600x1200@85
    {{PC_BASE+37,0,NSM},   20475,   {ProgrVPosHNeg,     6000,   {2448, 1394}},  {528,49},   {1792,1344}, R_4,   {0, 200,  3,  46, 328, 128,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 101 - 1792x1344@60
    {{PC_BASE+38,0,NSM},   26100,   {ProgrVPosHNeg,     7500,   {2456, 1417}},  {568,72},   {1792,1344}, R_4,   {0, 216,  3,  69, 352,  96,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 102 - 1792x1344@74.997
    {{PC_BASE+39,0,NSM},   21825,   {ProgrVPosHNeg,     6000,   {2528, 1439}},  {576,46},   {1856,1392}, R_4,   {0, 224,  3,  43, 352,  96,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 103 - 1856x1392@60
    {{PC_BASE+40,0,NSM},   28800,   {ProgrVPosHNeg,     7500,   {2560, 1500}},  {576,107},  {1856,1392}, R_4,   {0, 224,  3, 104, 352, 128,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 104 - 1856x1392@75
    {{PC_BASE+41,0,NSM},   15400,   {ProgrVNegHPos,     5995,   {2080, 1235}},  {112,32},   {1920,1200}, R_16,  {0,  32,  6,  26,  80,  48,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 106 - 1920x1200@59.95
    {{PC_BASE+42,0,NSM},   19325,   {ProgrVPosHNeg,     5988,   {2592, 1245}},  {536,42},   {1920,1200}, R_16,  {0, 200,  6,  36, 336, 136,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 107 - 1920x1200@59.88
    {{PC_BASE+43,0,NSM},   24525,   {ProgrVPosHNeg,     7493,   {2608, 1255}},  {552,52},   {1920,1200}, R_16,  {0, 208,  6,  46, 344, 136,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 108 - 1920x1200@74.93
    {{PC_BASE+44,0,NSM},   28125,   {ProgrVPosHNeg,     8493,   {2624, 1262}},  {560,59},   {1920,1200}, R_16,  {0, 208,  6,  53, 352, 144,   3,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 109 - 1920x1200@84.93
    {{PC_BASE+45,0,NSM},   23400,   {ProgrVPosHNeg,     6000,   {2600, 1500}},  {552,59},   {1920,1440}, R_4,   {0, 208,  3,  56, 344, 128,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 110 - 1920x1440@60
    {{PC_BASE+46,0,NSM},   29700,   {ProgrVPosHNeg,     7500,   {2640, 1500}},  {576,59},   {1920,1440}, R_4,   {0, 224,  3,  56, 352, 144,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 111 - 1920x1440@75
    {{PC_BASE+47,0,NSM},   24150,   {ProgrVPosHNeg,     6000,   {2720, 1481}},  {48,  3},   {2560,1440}, R_16,  {0,  32,  5,  56, 352, 144,   1,       0},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 112 - 2560x1440@60 // %%% need work
    {{PC_BASE+48,0,NSM},    2700,   {InterlaceVNegHNeg, 6000,   {1716,  264}},  {244,18},   {1440, 480},R_4or16,{3, 124,  3,  15, 114,  17,   5,     429},  0,  {0,     0,     0,     0,    0},   NO_3D_SUPPORT}, // 113 - 1440 x 480i 
};

//------------------------------------------------------------------------------
// Aspect Ratio table defines the aspect ratio as function of VIC. This table
// should be used in conjunction with the 861-D part of VModeInfoType VModesTable[]
// (formats 0 - 59) because some formats that differ only in their AR are grouped
// together (e.g., formats 2 and 3).
//------------------------------------------------------------------------------
static const uint8_t AspectRatioTable[] =
{
       R_4,  R_4, R_16, R_16, R_16,  R_4, R_16,  R_4, R_16,  R_4,
	R_16,  R_4, R_16,  R_4, R_16, R_16,  R_4, R_16, R_16, R_16,
	R_4, R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16,
	R_16, R_16, R_16, R_16,  R_4, R_16,  R_4, R_16, R_16, R_16,
	R_16,  R_4, R_16,  R_4, R_16, R_16, R_16,  R_4, R_16,  R_4,
	R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16,  R_4, R_16
};

//------------------------------------------------------------------------------
// VIC to Indexc table defines which VideoModeTable entry is appropreate for this VIC code. 
// Note: This table is valid ONLY for VIC codes in 861-D formats, NOT for HDMI_VIC codes
// or 3D codes!
//------------------------------------------------------------------------------
static const uint8_t VIC2Index[] =
{
	0,  0,  1,  1,  2,  3,  4,  4,  5,  5,
	7,  7,  8,  8, 10, 10, 11, 12, 12, 13,
	14, 15, 15, 16, 16, 19, 19, 20, 20, 23,
	23, 24, 25, 26, 27, 28, 28, 29, 29, 30,
	31, 32, 33, 33, 34, 34, 35, 36, 37, 37,
	38, 38, 39, 39, 40, 40, 41, 41, 42, 42
};

//------------------------------------------------------------------------------
// Function Name: ConvertVIC_To_VM_Index()
// Function Description: Convert Video Identification Code to the corresponding
//                  		index of VModesTable[]. Conversion also depends on the 
//					value of the 3D_Structure parameter in the case of 3D video format.
// Accepts: VIC to be converted; 3D_Structure value
// Returns: Index into VModesTable[] corrsponding to VIC
// Globals: VModesTable[] mhlTxAv
// Note: Conversion is for 861-D formats, HDMI_VIC or 3D
//------------------------------------------------------------------------------
uint8_t ConvertVIC_To_VM_Index (void)
{
	uint8_t index;

	//
	// The global VideoModeDescription contains all the information we need about
	// the Video mode for use to find its entry in the Videio mode table.
	//
	// The first issue.  The "VIC" may be a 891-D VIC code, or it might be an
	// HDMI_VIC code, or it might be a 3D code.  Each require different handling
	// to get the proper video mode table index.
	//
	if (mhlTxAv.HDMIVideoFormat == VMD_HDMIFORMAT_CEA_VIC)
	{
		//
		// This is a regular 861-D format VIC, so we use the VIC to Index
		// table to look up the index.
		//
        index = VIC2Index[mhlTxAv.VIC];
	}
	else if (mhlTxAv.HDMIVideoFormat == VMD_HDMIFORMAT_HDMI_VIC)
	{
		//
		// HDMI_VIC conversion is simple.  We need to subtract one because the codes start
		// with one instead of zero.  These values are from HDMI 1.4 Spec Table 8-13. 
		//
		if ((mhlTxAv.VIC < 1) || (mhlTxAv.VIC > 4))
		{
			index = DEFAULT_VIDEO_MODE;
		}
		else
		{
			index = (HDMI_VIC_BASE - 1) + mhlTxAv.VIC;
		}
	}
	else if (mhlTxAv.HDMIVideoFormat == VMD_HDMIFORMAT_3D)
	{
		//
		// Currently there are only a few VIC modes that we can do in 3D.  If the VIC code is not
		// one of these OR if the packing type is not supported for that VIC code, then it is an
		// error and we go to the default video mode.  See HDMI Spec 1.4 Table H-6.
		//
		switch (mhlTxAv.VIC)
		{
			case VIC_FOR_480P_60Hz_4X3:
			case VIC_FOR_480P_60Hz_16X9:
				// We only support Side-by-Side (Half) for these modes
				if (mhlTxAv.ThreeDStructure == SIDE_BY_SIDE_HALF)
					index = VIC_3D_BASE + 0;
				else
					index = DEFAULT_VIDEO_MODE;
				break;

			case VIC_FOR_720P_60Hz:
				switch(mhlTxAv.ThreeDStructure)
				{
					case FRAME_PACKING:
						index = VIC_3D_BASE + 1;
						break;
					default:
						index = DEFAULT_VIDEO_MODE;
						break;
				}
				break;

			case VIC_FOR_1080i_60Hz:
				switch(mhlTxAv.ThreeDStructure)
				{
					case FRAME_PACKING:
						index = VIC_3D_BASE + 2;
						break;
					case VMD_3D_FIELDALTERNATIVE:
						index = VIC_3D_BASE + 3;
						break;
					default:
						index = DEFAULT_VIDEO_MODE;
						break;
				}
				break;

			case VIC_FOR_1080p_60Hz:
				switch(mhlTxAv.ThreeDStructure)
				{
					case FRAME_PACKING:
						index = VIC_3D_BASE + 4;
						break;
					case VMD_3D_LINEALTERNATIVE:
						index = VIC_3D_BASE + 5;
						break;
					case SIDE_BY_SIDE_FULL:
						index = VIC_3D_BASE + 6;
						break;
					case SIDE_BY_SIDE_HALF:
						index = VIC_3D_BASE + 7;
						break;
					default:
						index = DEFAULT_VIDEO_MODE;
						break;
				}
				break;

			case VIC_FOR_720P_50Hz:
				switch(mhlTxAv.ThreeDStructure)
				{
					case FRAME_PACKING:
						index = VIC_3D_BASE + 8;
						break;
					case VMD_3D_LDEPTH:
						index = VIC_3D_BASE + 9;
						break;
					case VMD_3D_LDEPTHGRAPHICS:
						index = VIC_3D_BASE + 10;
						break;
					default:
						index = DEFAULT_VIDEO_MODE;
						break;
				}
				break;

			case VIC_FOR_1080i_50Hz:
				switch(mhlTxAv.ThreeDStructure)
				{
					case FRAME_PACKING:
						index = VIC_3D_BASE + 11;			
						break;
					case VMD_3D_FIELDALTERNATIVE:
						index = VIC_3D_BASE + 12;			
						break;
					default:
						index = DEFAULT_VIDEO_MODE;
						break;
				}
			break;

			case VIC_FOR_1080p_50Hz:
				switch(mhlTxAv.ThreeDStructure)
				{
					case FRAME_PACKING:
						index = VIC_3D_BASE + 13;
						break;
					case VMD_3D_LINEALTERNATIVE:
						index = VIC_3D_BASE + 14;
						break;
					case SIDE_BY_SIDE_FULL:
						index = VIC_3D_BASE + 15;
						break;
					default:
						index = DEFAULT_VIDEO_MODE;
						break;
				}
				break;

			case VIC_FOR_1080p_24Hz:
				switch(mhlTxAv.ThreeDStructure)
				{
					case FRAME_PACKING:
						index = VIC_3D_BASE + 16;
						break;
					default:
						index = DEFAULT_VIDEO_MODE;
						break;
				}
				break;

			default:
				index = DEFAULT_VIDEO_MODE;
				break;
		}
	}
	else if (mhlTxAv.HDMIVideoFormat == VMD_HDMIFORMAT_PC)
	{
		if (mhlTxAv.VIC < PC_SIZE)
		{
			index = mhlTxAv.VIC + PC_BASE;
		}
		else
		{
			index = DEFAULT_VIDEO_MODE;
		}
	}
	else
	{
		// This should never happen!  If so, default to first table entry
		index = DEFAULT_VIDEO_MODE;
	}
	
    	return index;
}


// Patches
//========
uint8_t TPI_REG0x63_SAVED = 0;

//------------------------------------------------------------------------------
// Function Name: SetEmbeddedSync()
// Function Description: Set the 9022/4 registers to extract embedded sync.
//
// Accepts: Index of video mode to set
// Returns: TRUE
// Globals: VModesTable[]
//------------------------------------------------------------------------------
uint8_t SetEmbeddedSync (void)
{
	uint8_t	ModeTblIndex;
	uint16_t H_Bit_2_H_Sync;
	uint16_t Field2Offset;
	uint16_t H_SyncWidth;
	
	uint8_t V_Bit_2_V_Sync;
	uint8_t V_SyncWidth;
	uint8_t B_Data[8];
	
	TX_DEBUG_PRINT(("[MHL]: >>SetEmbeddedSync()\n"));
	
	ReadModifyWriteIndexedRegister(INDEXED_PAGE_0, 0x0A, 0x01, 0x01);
	
	ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);	 // set 0x60[7] = 0 for DE mode
	WriteByteTPI(TPI_DE_CTRL, 0x30);
	ReadSetWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);       // set 0x60[7] = 1 for Embedded Sync
	
	ModeTblIndex = ConvertVIC_To_VM_Index(); 
	
	H_Bit_2_H_Sync = VModesTable[ModeTblIndex]._656.HBit2HSync;
	Field2Offset = VModesTable[ModeTblIndex]._656.Field2Offset;
	H_SyncWidth = VModesTable[ModeTblIndex]._656.HLength;
	V_Bit_2_V_Sync = VModesTable[ModeTblIndex]._656.VBit2VSync;
	V_SyncWidth = VModesTable[ModeTblIndex]._656.VLength;
	
	B_Data[0] = H_Bit_2_H_Sync & LOW_BYTE;                  // Setup HBIT_TO_HSYNC 8 LSBits (0x62)
	
	B_Data[1] = (H_Bit_2_H_Sync >> 8) & TWO_LSBITS;         // HBIT_TO_HSYNC 2 MSBits
	//B_Data[1] |= BIT_EN_SYNC_EXTRACT;                     // and Enable Embedded Sync to 0x63
	TPI_REG0x63_SAVED = B_Data[1];
	
	B_Data[2] = Field2Offset & LOW_BYTE;                    // 8 LSBits of "Field2 Offset" to 0x64
	B_Data[3] = (Field2Offset >> 8) & LOW_NIBBLE;           // 2 MSBits of "Field2 Offset" to 0x65
	
	B_Data[4] = H_SyncWidth & LOW_BYTE;
	B_Data[5] = (H_SyncWidth >> 8) & TWO_LSBITS;                    // HWIDTH to 0x66, 0x67
	B_Data[6] = V_Bit_2_V_Sync;                                     // VBIT_TO_VSYNC to 0x68
	B_Data[7] = V_SyncWidth;                                        // VWIDTH to 0x69
	
	WriteBlockTPI(TPI_HBIT_TO_HSYNC_7_0, 8, &B_Data[0]);
	
	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: EnableEmbeddedSync()
// Function Description: EnableEmbeddedSync
//------------------------------------------------------------------------------
void EnableEmbeddedSync (void)
{
    	TX_DEBUG_PRINT(("[MHL]: >>EnableEmbeddedSync()\n"));

	ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);	 // set 0x60[7] = 0 for DE mode
	WriteByteTPI(TPI_DE_CTRL, 0x30);
    	ReadSetWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);       // set 0x60[7] = 1 for Embedded Sync
	ReadSetWriteTPI(TPI_DE_CTRL, BIT6);
}

//------------------------------------------------------------------------------
// Function Name: SetDE()
// Function Description: Set the 9022/4 internal DE generator parameters
//
// Accepts: none
// Returns: DE_SET_OK
// Globals: none
//
// NOTE: 0x60[7] must be set to "0" for the follwing settings to take effect
//------------------------------------------------------------------------------
uint8_t SetDE (void)
{
	uint8_t RegValue;
	uint8_t ModeTblIndex;
	
	uint16_t H_StartPos, V_StartPos;
	uint16_t Htotal, Vtotal;
	uint16_t H_Res, V_Res;
	
	uint8_t Polarity;
	uint8_t B_Data[12];
	
	TX_DEBUG_PRINT(("[MHL]: >>SetDE()\n"));
	
	ModeTblIndex = ConvertVIC_To_VM_Index();
	
	if (VModesTable[ModeTblIndex]._3D_Struct != NO_3D_SUPPORT)
	{
		return DE_CANNOT_BE_SET_WITH_3D_MODE;
		TX_DEBUG_PRINT((">>SetDE() not allowed with 3D video format\n"));
	}
	
	// Make sure that External Sync method is set before enableing the DE Generator:
	RegValue = ReadByteTPI(TPI_SYNC_GEN_CTRL);
	
	if (RegValue & BIT7)
	{
		return DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC;
	}
	
	H_StartPos = VModesTable[ModeTblIndex].Pos.H;
	V_StartPos = VModesTable[ModeTblIndex].Pos.V;

   	Htotal = VModesTable[ModeTblIndex].Tag.Total.Pixels;
	Vtotal = VModesTable[ModeTblIndex].Tag.Total.Lines;

	Polarity = (~VModesTable[ModeTblIndex].Tag.RefrTypeVHPol) & TWO_LSBITS;
	
	H_Res = VModesTable[ModeTblIndex].Res.H;
	
	if ((VModesTable[ModeTblIndex].Tag.RefrTypeVHPol & 0x04))
	{
		V_Res = (VModesTable[ModeTblIndex].Res.V) >> 1;
	}
	else
	{
	    V_Res = (VModesTable[ModeTblIndex].Res.V);
	}
	
	B_Data[0] = H_StartPos & LOW_BYTE;              // 8 LSB of DE DLY in 0x62
	
	B_Data[1] = (H_StartPos >> 8) & TWO_LSBITS;     // 2 MSBits of DE DLY to 0x63
	B_Data[1] |= (Polarity << 4);                   // V and H polarity
	B_Data[1] |= BIT_EN_DE_GEN;                     // enable DE generator
	
	B_Data[2] = V_StartPos & SEVEN_LSBITS;      // DE_TOP in 0x64
	B_Data[3] = 0x00;                           // 0x65 is reserved
	B_Data[4] = H_Res & LOW_BYTE;               // 8 LSBits of DE_CNT in 0x66
	B_Data[5] = (H_Res >> 8) & LOW_NIBBLE;      // 4 MSBits of DE_CNT in 0x67
	B_Data[6] = V_Res & LOW_BYTE;               // 8 LSBits of DE_LIN in 0x68
	B_Data[7] = (V_Res >> 8) & THREE_LSBITS;    // 3 MSBits of DE_LIN in 0x69
	B_Data[8] = Htotal & LOW_BYTE;				// 8 LSBits of H_RES in 0x6A
	B_Data[9] =	(Htotal >> 8) & LOW_NIBBLE;		// 4 MSBITS of H_RES in 0x6B
	B_Data[10] = Vtotal & LOW_BYTE;				// 8 LSBits of V_RES in 0x6C
	B_Data[11] = (Vtotal >> 8) & BITS_2_1_0;	// 3 MSBITS of V_RES in 0x6D

	WriteBlockTPI(TPI_DE_DLY, 12, &B_Data[0]);
	TPI_REG0x63_SAVED = B_Data[1];
	
	return DE_SET_OK;                               // Write completed successfully
}

//------------------------------------------------------------------------------
// Function Name: SetFormat()
// Function Description: Set the 9022/4 format
//
// Accepts: Data
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void SetFormat (uint8_t *Data)
{
	ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPUT_MODE_MASK, OUTPUT_MODE_HDMI); // Set HDMI mode to allow color space conversion
	
	WriteBlockTPI(TPI_INPUT_FORMAT_REG, 2, Data);   // Program TPI AVI Input and Output Format
	WriteByteTPI(TPI_END_RIGHT_BAR_MSB, 0x00);	  // Set last uint8_t of TPI AVI InfoFrame for TPI AVI I/O Format to take effect

	if (!IsHDMI_Sink()) 
	{
		ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPUT_MODE_MASK, OUTPUT_MODE_DVI);
	}

	if (mhlTxAv.SyncMode == EMBEDDED_SYNC)
		EnableEmbeddedSync();					// Last uint8_t of TPI AVI InfoFrame resets Embedded Sync Extraction
}

//------------------------------------------------------------------------------
// Function Name: printVideoMode()
// Function Description: print video mode
//------------------------------------------------------------------------------
void printVideoMode (void)
{
	TX_DEBUG_PRINT(("[MHL]: >>Video mode = "));
	
	switch (mhlTxAv.VIC)
	{
		case HDMI_640X480P:
			TX_DEBUG_PRINT(("HDMI_640X480P \n"));
			break;
		case HDMI_480I60_4X3:
			TX_DEBUG_PRINT(("HDMI_480I60_4X3 \n"));
			break;
		case HDMI_480I60_16X9:
			TX_DEBUG_PRINT(("HDMI_480I60_16X9 \n"));
			break;
		case HDMI_576I50_4X3:
			TX_DEBUG_PRINT(("HDMI_576I50_4X3 \n"));
			break;
		case HDMI_576I50_16X9:
			TX_DEBUG_PRINT(("HDMI_576I50_16X9 \n"));
			break;
		case HDMI_480P60_4X3:
			TX_DEBUG_PRINT(("HDMI_480P60_4X3 \n"));
			break;
		case HDMI_480P60_16X9:
			TX_DEBUG_PRINT(("HDMI_480P60_16X9 \n"));
			break;
		case HDMI_576P50_4X3:
			TX_DEBUG_PRINT(("HDMI_576P50_4X3 \n"));
			break;
		case HDMI_576P50_16X9:
			TX_DEBUG_PRINT(("HDMI_576P50_16X9 \n"));
			break;
		case HDMI_720P60:
			TX_DEBUG_PRINT(("HDMI_720P60 \n"));
			break;
		case HDMI_720P50:
			TX_DEBUG_PRINT(("HDMI_720P50 \n"));
			break;
		case HDMI_1080I60:
			TX_DEBUG_PRINT(("HDMI_1080I60 \n"));
			break;
		case HDMI_1080I50:
			TX_DEBUG_PRINT(("HDMI_1080I50 \n"));
			break;
		case HDMI_1080P24:
			TX_DEBUG_PRINT(("HDMI_1080P24 \n"));
			break;
		case HDMI_1080P25:
			TX_DEBUG_PRINT(("HDMI_1080P25 \n"));
			break;
		case HDMI_1080P30:
			TX_DEBUG_PRINT(("HDMI_1080P30 \n"));
			break;
		default:
			break;
	}
}

//------------------------------------------------------------------------------
// Function Name: InitVideo()
// Function Description: Set the 9022/4 to the video mode determined by GetVideoMode()
//
// Accepts: Index of video mode to set; Flag that distinguishes between
//                  calling this function after power up and after input
//                  resolution change
// Returns: TRUE
// Globals: VModesTable, VideoCommandImage
//------------------------------------------------------------------------------
uint8_t InitVideo (uint8_t TclkSel)
{
	uint8_t	ModeTblIndex;
	
#ifdef DEEP_COLOR
	uint8_t temp;
#endif
	uint8_t B_Data[8];

	uint8_t EMB_Status;
	uint8_t DE_Status;
	uint8_t Pattern;

	TX_DEBUG_PRINT(("[MHL]: >>InitVideo()"));
	printVideoMode();
	TX_DEBUG_PRINT((" HF:%d", (int) mhlTxAv.HDMIVideoFormat));
	TX_DEBUG_PRINT((" VIC:%d", (int) mhlTxAv.VIC));
	TX_DEBUG_PRINT((" A:%x", (int) mhlTxAv.AspectRatio));
	TX_DEBUG_PRINT((" CS:%x", (int) mhlTxAv.ColorSpace));
	TX_DEBUG_PRINT((" CD:%x", (int) mhlTxAv.ColorDepth));
	TX_DEBUG_PRINT((" CR:%x", (int) mhlTxAv.Colorimetry));
	TX_DEBUG_PRINT((" SM:%x", (int) mhlTxAv.SyncMode));
	TX_DEBUG_PRINT((" TCLK:%x", (int) mhlTxAv.TclkSel)); 
	TX_DEBUG_PRINT((" 3D:%d", (int) mhlTxAv.ThreeDStructure));
	TX_DEBUG_PRINT((" 3Dx:%d\n", (int) mhlTxAv.ThreeDExtData));

	ModeTblIndex = (uint8_t)ConvertVIC_To_VM_Index();

	Pattern = (TclkSel << 6) & TWO_MSBITS;							// Use TPI 0x08[7:6] for 9022A/24A video clock multiplier
	ReadSetWriteTPI(TPI_PIX_REPETITION, Pattern);

	// Take values from VModesTable[]:
	if( (mhlTxAv.VIC == 6) || (mhlTxAv.VIC == 7) ||	//480i
	     (mhlTxAv.VIC == 21) || (mhlTxAv.VIC == 22) )	//576i
	{
		if( mhlTxAv.ColorSpace == YCBCR422_8BITS)	//27Mhz pixel clock
		{
			B_Data[0] = VModesTable[ModeTblIndex].PixClk & 0x00FF;
			B_Data[1] = (VModesTable[ModeTblIndex].PixClk >> 8) & 0xFF;
		}
		else											//13.5Mhz pixel clock
		{
			B_Data[0] = (VModesTable[ModeTblIndex].PixClk /2) & 0x00FF;
			B_Data[1] = ((VModesTable[ModeTblIndex].PixClk /2) >> 8) & 0xFF;
		}
	
	}
	else
	{
		B_Data[0] = VModesTable[ModeTblIndex].PixClk & 0x00FF;			// write Pixel clock to TPI registers 0x00, 0x01
		B_Data[1] = (VModesTable[ModeTblIndex].PixClk >> 8) & 0xFF;
	}
	
	B_Data[2] = VModesTable[ModeTblIndex].Tag.VFreq & 0x00FF;		// write Vertical Frequency to TPI registers 0x02, 0x03
	B_Data[3] = (VModesTable[ModeTblIndex].Tag.VFreq >> 8) & 0xFF;

	if( (mhlTxAv.VIC == 6) || (mhlTxAv.VIC == 7) ||	//480i
	     (mhlTxAv.VIC == 21) || (mhlTxAv.VIC == 22) )	//576i
	{
		B_Data[4] = (VModesTable[ModeTblIndex].Tag.Total.Pixels /2) & 0x00FF;	// write total number of pixels to TPI registers 0x04, 0x05
		B_Data[5] = ((VModesTable[ModeTblIndex].Tag.Total.Pixels /2) >> 8) & 0xFF;
	}
	else
	{
		B_Data[4] = VModesTable[ModeTblIndex].Tag.Total.Pixels & 0x00FF;	// write total number of pixels to TPI registers 0x04, 0x05
		B_Data[5] = (VModesTable[ModeTblIndex].Tag.Total.Pixels >> 8) & 0xFF;
	}
	
	B_Data[6] = VModesTable[ModeTblIndex].Tag.Total.Lines & 0x00FF;	// write total number of lines to TPI registers 0x06, 0x07
	B_Data[7] = (VModesTable[ModeTblIndex].Tag.Total.Lines >> 8) & 0xFF;

	WriteBlockTPI(TPI_PIX_CLK_LSB, 8, B_Data);						// Write TPI Mode data.

	// TPI Input Bus and Pixel Repetition Data
	// B_Data[0] = Reg0x08;
	
	B_Data[0] = 0;  // Set to default 0 for use again
	//B_Data[0] = (VModesTable[ModeTblIndex].PixRep) & LOW_BYTE;		// Set pixel replication field of 0x08
	B_Data[0] |= BIT_BUS_24;										// Set 24 bit bus
	B_Data[0] |= (TclkSel << 6) & TWO_MSBITS;

#ifdef CLOCK_EDGE_RISING
	B_Data[0] |= BIT_EDGE_RISE;									// Set to rising edge
#else
	B_Data[0] &= ~BIT_EDGE_RISE;									// Set to falling edge
#endif

	tpivmode[0] = B_Data[0]; // saved TPI Reg0x08 value.
	WriteByteTPI(TPI_PIX_REPETITION, B_Data[0]);					// 0x08

	// TPI AVI Input and Output Format Data
	// B_Data[0] = Reg0x09;
	// B_Data[1] = Reg0x0A;
	
	B_Data[0] = 0;  // Set to default 0 for use again
	B_Data[1] = 0;  // Set to default 0 for use again
	
	if (mhlTxAv.SyncMode == EMBEDDED_SYNC)
	{
		EMB_Status = SetEmbeddedSync();
		EnableEmbeddedSync();
	}

	if (mhlTxAv.SyncMode == INTERNAL_DE)
	{
		ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);	// set 0x60[7] = 0 for External Sync
		DE_Status = SetDE();								// Call SetDE() with Video Mode as a parameter
	}

	if (mhlTxAv.ColorSpace == RGB)
		B_Data[0] = (((BITS_IN_RGB | BITS_IN_AUTO_RANGE) & ~BIT_EN_DITHER_10_8) & ~BIT_EXTENDED_MODE); // 0x09
	
	else if (mhlTxAv.ColorSpace == YCBCR444)
		B_Data[0] = (((BITS_IN_YCBCR444 | BITS_IN_AUTO_RANGE) & ~BIT_EN_DITHER_10_8) & ~BIT_EXTENDED_MODE); // 0x09
		
	else if ((mhlTxAv.ColorSpace == YCBCR422_16BITS) ||(mhlTxAv.ColorSpace == YCBCR422_8BITS))
		B_Data[0] = (((BITS_IN_YCBCR422 | BITS_IN_AUTO_RANGE) & ~BIT_EN_DITHER_10_8) & ~BIT_EXTENDED_MODE); // 0x09
	
#ifdef DEEP_COLOR
	switch (mhlTxAv.ColorDepth)
	{
		case 0:  temp = 0x00; ReadModifyWriteTPI(TPI_DEEP_COLOR_GCP, BIT2, 0x00); break;
		case 1:  temp = 0x80; ReadModifyWriteTPI(TPI_DEEP_COLOR_GCP, BIT2, BIT2); break;
		case 2:  temp = 0xC0; ReadModifyWriteTPI(TPI_DEEP_COLOR_GCP, BIT2, BIT2); break;
		case 3:  temp = 0x40; ReadModifyWriteTPI(TPI_DEEP_COLOR_GCP, BIT2, BIT2); break;
		default: temp = 0x00; ReadModifyWriteTPI(TPI_DEEP_COLOR_GCP, BIT2, 0x00); break;
	}
	B_Data[0] = ((B_Data[0] & 0x3F) | temp);
#endif

	B_Data[1] = (BITS_OUT_RGB | BITS_OUT_AUTO_RANGE);  //Reg0x0A

	if ((mhlTxAv.VIC == 6) || (mhlTxAv.VIC == 7) ||	//480i
	     (mhlTxAv.VIC == 21) || (mhlTxAv.VIC == 22) ||//576i
	     (mhlTxAv.VIC == 2) || (mhlTxAv.VIC == 3) ||	//480p
	     (mhlTxAv.VIC == 17) ||(mhlTxAv.VIC == 18))	//576p
	{
		B_Data[1] &= ~BIT_BT_709;
	}
	else
	{
		B_Data[1] |= BIT_BT_709;
	}
	
#ifdef DEEP_COLOR
	B_Data[1] = ((B_Data[1] & 0x3F) | temp);
#endif

#ifdef DEV_SUPPORT_EDID
	if (!IsHDMI_Sink()) 
	{
		B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_RGB);
	}
	else 
	{
		// Set YCbCr color space depending on EDID
		if (mhlTxEdid.YCbCr_4_4_4) 
		{
			B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_YCBCR444);
		}
		else 
		{
			if (mhlTxEdid.YCbCr_4_2_2)
			{
				B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_YCBCR422);
			}
			else
			{
				B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_RGB);
			}
		}
	}
	
#else
	if (!IsHDMI_Sink()) 
	{
		B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_RGB);
	}
	else 
	{
		// Set YCbCr color space depending on EDID
		if (HDMI_YCbCr_4_4_4) 
		{
			B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_YCBCR444);
		}
		else 
		{
			if (HDMI_YCbCr_4_2_2)
			{
				B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_YCBCR422);
			}
			else
			{
				B_Data[1] = ((B_Data[1] & 0xFC) | BITS_OUT_RGB);
			}
		}
	}
#endif

	tpivmode[1] = B_Data[0];	// saved TPI Reg0x09 value.
	tpivmode[2] = B_Data[1];  // saved TPI Reg0x0A value.
	SetFormat(B_Data);

	ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, BIT2);	// Number HSync pulses from VSync active edge to Video Data Period should be 20 (VS_TO_VIDEO)

	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: SetAVI_InfoFrames()
// Function Description: Load AVI InfoFrame data into registers and send to sink
//
// Accepts: An API_Cmd parameter that holds the data to be sent in the InfoFrames
// Returns: TRUE
// Globals: none
//
// Note:     Infoframe contents are from spec CEA-861-D
//
//------------------------------------------------------------------------------
uint8_t SetAVI_InfoFrames (void)
{
	uint8_t B_Data[SIZE_AVI_INFOFRAME];
	uint8_t i;
	uint8_t TmpVal;
	uint8_t VModeTblIndex;
	
	TX_DEBUG_PRINT(("[MHL]: >>SetAVI_InfoFrames()\n"));
	
	for (i = 0; i < SIZE_AVI_INFOFRAME; i++)
		B_Data[i] = 0;
	
#ifdef DEV_SUPPORT_EDID
	if (mhlTxEdid.YCbCr_4_4_4)
		TmpVal = 2;
	else if (mhlTxEdid.YCbCr_4_2_2)
		TmpVal = 1;
	else
		TmpVal = 0;
#else
	if(HDMI_YCbCr_4_4_4)
		TmpVal = 2;
	else if(HDMI_YCbCr_4_2_2)
		TmpVal = 1;
	else		
		TmpVal = 0;
#endif

	B_Data[1] = (TmpVal << 5) & BITS_OUT_FORMAT;	// AVI Byte1: Y1Y0 (output format)
	B_Data[1] |= 0x11;  // A0 = 1; Active format identification data is present in the AVI InfoFrame. // S1:S0 = 01; Overscanned (television).

	if (mhlTxAv.ColorSpace == XVYCC444)                // Extended colorimetry - xvYCC
   	{
       	B_Data[2] = 0xC0;                                          // Extended colorimetry info (B_Data[3] valid (CEA-861D, Table 11)

              if (mhlTxAv.Colorimetry == COLORIMETRY_601)   		// xvYCC601
                    	B_Data[3] &= ~BITS_6_5_4;

              else if (mhlTxAv.Colorimetry == COLORIMETRY_709)	// xvYCC709
                     B_Data[3] = (B_Data[3] & ~BITS_6_5_4) | BIT4;
	}

	else if (mhlTxAv.Colorimetry == COLORIMETRY_709)		// BT.709
		B_Data[2] = 0x80;		// AVI Byte2: C1C0

	else if (mhlTxAv.Colorimetry == COLORIMETRY_601)		// BT.601
		B_Data[2] = 0x40;		// AVI Byte2: C1C0

	else													// Carries no data
	{													// AVI Byte2: C1C0
		B_Data[2] &= ~BITS_7_6;							// colorimetry = 0
		B_Data[3] &= ~BITS_6_5_4;						// Extended colorimetry = 0
	}

	VModeTblIndex = ConvertVIC_To_VM_Index();

	B_Data[4] = mhlTxAv.VIC;

	//  Set the Aspect Ration info into the Infoframe Byte 2
	if (mhlTxAv.AspectRatio == VMD_ASPECT_RATIO_16x9)
	{
		B_Data[2] |= _16_To_9;                          // AVI Byte2: M1M0
		// If the Video Mode table says this mode can be 4x3 OR 16x9, and we are pointing to the
		// table entry that is 4x3, then we bump to the next Video Table entry which will be for 16x9.
		if ((VModesTable[VModeTblIndex].AspectRatio == R_4or16) && (AspectRatioTable[mhlTxAv.VIC - 1] == R_4))
		{
			mhlTxAv.VIC++;
			B_Data[4]++;
		}
	}
	else
	{
		B_Data[2] |= _4_To_3;                       // AVI Byte4: VIC
	}

		B_Data[2] |= SAME_AS_AR;                        // AVI Byte2: R3..R1 - Set to "Same as Picture Aspect Ratio"
		B_Data[5] = VModesTable[VModeTblIndex].PixRep;      // AVI Byte5: Pixel Replication - PR3..PR0

	// Calculate AVI InfoFrame ChecKsum
	B_Data[0] = 0x82 + 0x02 +0x0D;
	for (i = 1; i < SIZE_AVI_INFOFRAME; i++)
	{
		B_Data[0] += B_Data[i];
	}
	B_Data[0] = 0x100 - B_Data[0];

	// Write the Inforframe data to the TPI Infoframe registers
	WriteBlockTPI(TPI_AVI_BYTE_0, SIZE_AVI_INFOFRAME, B_Data);

	if (mhlTxAv.SyncMode == EMBEDDED_SYNC)
		EnableEmbeddedSync();

	return TRUE;
}

void siMhlTx_PowerStateD0 (void);
void SetAudioMute (uint8_t audioMute);
uint8_t siMhlTx_AudioSet (void);
#ifdef APPLY_PLL_RECOVERY
void SiiMhlTxDrvRecovery( void );
#endif
//------------------------------------------------------------------------------
// Function Name: siMhlTx_Init()
// Function Description: Set the 9022/4 video and video.
//------------------------------------------------------------------------------
void siMhlTx_Init (void) 
{
	TX_DEBUG_PRINT(("[MHL]: >>siMhlTx_Init()\n"));
	
	// workaround for Bug#18128
	if (mhlTxAv.ColorDepth == VMD_COLOR_DEPTH_8BIT)
	{
		// Yes it is, so force 16bpps first!
		mhlTxAv.ColorDepth = VMD_COLOR_DEPTH_16BIT;
		InitVideo(mhlTxAv.TclkSel);
		// Now put it back to 8bit and go do the expected InitVideo() call
		mhlTxAv.ColorDepth = VMD_COLOR_DEPTH_8BIT;
	}
	// end workaround

	InitVideo(mhlTxAv.TclkSel);			// Set PLL Multiplier to x1 upon power up

	siMhlTx_PowerStateD0();
	
	if (IsHDMI_Sink())					// Set InfoFrames only if HDMI output mode
	{
		SetAVI_InfoFrames();
		siMhlTx_AudioSet();			// set audio interface to basic audio (an external command is needed to set to any other mode
	}
	else
	{
		SetAudioMute(AUDIO_MUTE_MUTED);
	}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!

// THIS PATCH IS NEEDED BECAUSE SETTING UP AVI InfoFrames CLEARS 0x63 and 0x60[5]
	if (mhlTxAv.ColorSpace == YCBCR422_8BITS)
		ReadSetWriteTPI(TPI_SYNC_GEN_CTRL, BIT5);       // Set 0x60[5] according to input color space.
                                               
// THIS PATCH IS NEEDED BECAUSE SETTING UP AVI InfoFrames CLEARS 0x63
    	WriteByteTPI(TPI_DE_CTRL, TPI_REG0x63_SAVED);

// PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    	//==========================================================
    	WriteByteTPI(TPI_YC_Input_Mode, 0x00);


	WriteIndexedRegister(INDEXED_PAGE_0, SRST, MHL_FIFO_RST_MASK | SW_RST_AUTO_MASK);	// Assert Mobile HD FIFO Reset
	DelayMS(1);
	WriteIndexedRegister(INDEXED_PAGE_0, SRST, SW_RST_AUTO_MASK);			// Deassert Mobile HD FIFO Reset


#ifdef DEV_SUPPORT_HDCP
	if ((mhlTxHdcp.HDCP_TxSupports == TRUE) && (mhlTxHdcp.HDCPAuthenticated == VMD_HDCP_AUTHENTICATED))
	{
		if (mhlTxHdcp.HDCP_AksvValid == TRUE)
		{
			// AV MUTE
			TX_DEBUG_PRINT (("[MHL]: TMDS -> Enabled (Video Muted)\n"));
			SiiMhlTxTmdsEnable();
			
			//ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, LINK_INTEGRITY_MODE_MASK | TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK,
				//LINK_INTEGRITY_DYNAMIC | TMDS_OUTPUT_CONTROL_ACTIVE | AV_MUTE_MUTED);
			
			WriteByteTPI(TPI_PIX_REPETITION, tpivmode[0]);      		// Write register 0x08
			EnableInterrupts(HOT_PLUG_EVENT | RX_SENSE_EVENT | AUDIO_ERROR_EVENT | SECURITY_CHANGE_EVENT | V_READY_EVENT | HDCP_CHANGE_EVENT);
		}
	}
	else
#endif
	{
		TX_DEBUG_PRINT (("[MHL]: TMDS -> Enabled\n"));
		SiiMhlTxTmdsEnable();
		
		//ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, LINK_INTEGRITY_MODE_MASK | TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK, 
			//LINK_INTEGRITY_DYNAMIC | TMDS_OUTPUT_CONTROL_ACTIVE | AV_MUTE_NORMAL);
			
		WriteByteTPI(TPI_PIX_REPETITION, tpivmode[0]);      		// Write register 0x08
		EnableInterrupts(HOT_PLUG_EVENT | RX_SENSE_EVENT | AUDIO_ERROR_EVENT);
	}

#ifdef INFOFRAMES_AFTER_TMDS
	WriteByteTPI(0xCD, 0x00);	    				// Set last uint8_t of Audio InfoFrame
	WriteByteTPI(TPI_END_RIGHT_BAR_MSB, 0x00);	    // Set last uint8_t of AVI InfoFrame
#endif
}

//------------------------------------------------------------------------------
// Function Name: siMhlTx_VideoSet()
// Function Description: Set the 9022/4 video resolution
//
// Accepts: none
// Returns: Success message if video resolution changed successfully.
//                  Error Code if resolution change failed
// Globals: mhlTxAv
//------------------------------------------------------------------------------
//============================================================
#define T_RES_CHANGE_DELAY      128         // delay between turning TMDS bus off and changing output resolution

uint8_t siMhlTx_VideoSet (void)   
{
  	TX_DEBUG_PRINT(("[MHL]: >>siMhlTx_VideoSet()\n"));

	mhlTxConfig.hdmiCableConnected = TRUE;
	mhlTxConfig.dsRxPoweredUp = TRUE;

	SiiMhlTxDrvTmdsControl( FALSE );
	DelayMS(T_RES_CHANGE_DELAY);	// allow control InfoFrames to pass through to the sink device.
	
	siMhlTx_Init();
	
	return VIDEO_MODE_SET_OK;
}

//------------------------------------------------------------------------------
// Function Name: SetAudioInfoFrames()
// Function Description: Load Audio InfoFrame data into registers and send to sink
//
// Accepts: (1) Channel count 
//              (2) speaker configuration per CEA-861D Tables 19, 20 
//              (3) Coding type: 0x09 for DSD Audio. 0 (refer to stream header) for all the rest 
//              (4) Sample Frequency. Non zero for HBR only 
//              (5) Audio Sample Length. Non zero for HBR only.
// Returns: TRUE
// Globals: none
//------------------------------------------------------------------------------
uint8_t SetAudioInfoFrames(uint8_t ChannelCount, uint8_t CodingType, uint8_t SS, uint8_t Fs, uint8_t SpeakerConfig)
{
    	uint8_t B_Data[SIZE_AUDIO_INFOFRAME];  // 14
    	uint8_t i;
    	uint8_t TmpVal = 0;

    	TX_DEBUG_PRINT(("[MHL]: >>SetAudioInfoFrames()\n"));

    	for (i = 0; i < SIZE_AUDIO_INFOFRAME; i++)
        	B_Data[i] = 0;

    	WriteByteTPI(MISC_INFO_FRAMES_CTRL, DISABLE_AUDIO);         //  Disbale MPEG/Vendor Specific InfoFrames

    	B_Data[0] = TYPE_AUDIO_INFOFRAMES;      // 0x84
    	B_Data[1] = AUDIO_INFOFRAMES_VERSION;   // 0x01
    	B_Data[2] = AUDIO_INFOFRAMES_LENGTH;    // 0x0A
    	B_Data[3] = TYPE_AUDIO_INFOFRAMES+ 		// Calculate checksum - 0x84 + 0x01 + 0x0A
				AUDIO_INFOFRAMES_VERSION+ 
				AUDIO_INFOFRAMES_LENGTH;

    	B_Data[4] = ChannelCount;               // 0 for "Refer to Stream Header" or for 2 Channels. 0x07 for 8 Channels
	B_Data[4] |= (CodingType << 4);                 // 0xC7[7:4] == 0b1001 for DSD Audio

	B_Data[5] = ((Fs & THREE_LSBITS) << 2) | (SS & TWO_LSBITS);

	B_Data[7] = SpeakerConfig;

    	for (i = 4; i < SIZE_AUDIO_INFOFRAME; i++)
        	B_Data[3] += B_Data[i];

    	B_Data[3] = 0x100 - B_Data[3];

	WriteByteTPI(MISC_INFO_FRAMES_CTRL, EN_AND_RPT_AUDIO);         // Re-enable Audio InfoFrame transmission and repeat

    	WriteBlockTPI(MISC_INFO_FRAMES_TYPE, SIZE_AUDIO_INFOFRAME, B_Data);
		
	if (mhlTxAv.SyncMode == EMBEDDED_SYNC)
		EnableEmbeddedSync();

    	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: SetAudioMute()
// Function Description: Mute audio
//
// Accepts: Mute or unmute.
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void SetAudioMute (uint8_t audioMute)
{
	ReadModifyWriteTPI(TPI_AUDIO_INTERFACE_REG, AUDIO_MUTE_MASK, audioMute);
}

#ifndef F_9022A_9334
//------------------------------------------------------------------------------
// Function Name: SetChannelLayout()
// Function Description: Set up the Channel layout field of internal register 0x2F (0x2F[1])
//
// Accepts: Number of audio channels: "0 for 2-Channels ."1" for 8.
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void SetChannelLayout (uint8_t Count)
{
    	// Indexed register 0x7A:0x2F[1]:
   	WriteByteTPI(TPI_INTERNAL_PAGE_REG, 0x02); // Internal page 2
   	WriteByteTPI(TPI_INDEXED_OFFSET_REG, 0x2F);

    	Count &= THREE_LSBITS;

    	if (Count == TWO_CHANNEL_LAYOUT)
    	{
        	// Clear 0x2F[1]:
        	ReadClearWriteTPI(TPI_INDEXED_VALUE_REG, BIT1);
    	}

    	else if (Count == EIGHT_CHANNEL_LAYOUT)
    	{
        	// Set 0x2F[1]:
        	ReadSetWriteTPI(TPI_INDEXED_VALUE_REG, BIT1);
    	}
}
#endif

//------------------------------------------------------------------------------
// Function Name: siMhlTx_AudioSet()
// Function Description: Set the 9022/4 audio interface to basic audio.
//
// Accepts: none
// Returns: Success message if audio changed successfully.
//                  Error Code if resolution change failed
// Globals: mhlTxAv
//------------------------------------------------------------------------------
uint8_t siMhlTx_AudioSet (void)
{
	TX_DEBUG_PRINT(("[MHL]: >>siMhlTx_AudioSet()\n"));
	   
	SetAudioMute(AUDIO_MUTE_MUTED);  // mute output

	if (mhlTxAv.AudioMode == AMODE_I2S)  	// I2S input
	{
		ReadModifyWriteTPI(TPI_AUDIO_INTERFACE_REG, AUDIO_SEL_MASK, AUD_IF_I2S);	// 0x26 = 0x80
		WriteByteTPI(TPI_AUDIO_HANDLING, 0x08 | AUD_DO_NOT_CHECK);	  // 0x25
	}
	else									// SPDIF input
	{
		ReadModifyWriteTPI(TPI_AUDIO_INTERFACE_REG, AUDIO_SEL_MASK, AUD_IF_SPDIF);	// 0x26 = 0x40
		WriteByteTPI(TPI_AUDIO_HANDLING, AUD_PASS_BASIC);                   // 0x25 = 0x00
	}

#ifndef F_9022A_9334
	if (mhlTxAv.AudioChannels == ACHANNEL_2CH)
       	SetChannelLayout(TWO_CHANNELS);             // Always 2 channesl in S/PDIF
       else
       	SetChannelLayout(EIGHT_CHANNELS);
#else
	if (mhlTxAv.AudioChannels == ACHANNEL_2CH)
       	ReadClearWriteTPI(TPI_AUDIO_INTERFACE_REG, BIT5); // Use TPI 0x26[5] for 9022A/24A and 9334 channel layout
       else
       	ReadSetWriteTPI(TPI_AUDIO_INTERFACE_REG, BIT5); // Use TPI 0x26[5] for 9022A/24A and 9334 channel layout
#endif

	if (mhlTxAv.AudioMode == AMODE_I2S)  	// I2S input
	{
       	// I2S - Map channels - replace with call to API MAPI2S
       	WriteByteTPI(TPI_I2S_EN, 0x80); // 0x1F

		if (mhlTxAv.AudioChannels > ACHANNEL_2CH)
			WriteByteTPI(TPI_I2S_EN, 0x91);
		
		if (mhlTxAv.AudioChannels > ACHANNEL_4CH)
			WriteByteTPI(TPI_I2S_EN, 0xA2);
		
		if (mhlTxAv.AudioChannels > ACHANNEL_6CH)
			WriteByteTPI(TPI_I2S_EN, 0xB3);

       	// I2S - Stream Header Settings - replace with call to API SetI2S_StreamHeader
       	WriteByteTPI(TPI_I2S_CHST_0, 0x00); // 0x21
       	WriteByteTPI(TPI_I2S_CHST_1, 0x00);
       	WriteByteTPI(TPI_I2S_CHST_2, 0x00);
       	WriteByteTPI(TPI_I2S_CHST_3, mhlTxAv.AudioFs);
       	WriteByteTPI(TPI_I2S_CHST_4, (mhlTxAv.AudioFs << 4) |mhlTxAv.AudioWordLength);

		// Oscar 20100929 added for 16bit auido noise issue
		WriteIndexedRegister(INDEXED_PAGE_1, AUDIO_INPUT_LENGTH, mhlTxAv.AudioWordLength);

       	// I2S - Input Configuration
       	WriteByteTPI(TPI_I2S_IN_CFG, mhlTxAv.AudioI2SFormat); //TPI_Reg0x20
	}

    	WriteByteTPI(TPI_AUDIO_SAMPLE_CTRL, REFER_TO_STREAM_HDR);
	SetAudioInfoFrames(mhlTxAv.AudioChannels & THREE_LSBITS, REFER_TO_STREAM_HDR, REFER_TO_STREAM_HDR, REFER_TO_STREAM_HDR, 0x00); 	

	SetAudioMute(AUDIO_MUTE_NORMAL);  // unmute output

	return AUDIO_MODE_SET_OK;
}

//------------------------------------------------------------------------------
// Function Name: siMhlTx_PowerStateD0()
// Function Description: Set TX to D0 mode.
//------------------------------------------------------------------------------
void siMhlTx_PowerStateD0 (void)
{	
	ReadModifyWriteTPI(TPI_DEVICE_POWER_STATE_CTRL_REG, TX_POWER_STATE_MASK, TX_POWER_STATE_D0);
	TX_DEBUG_PRINT(("[MHL]: TX Power State D0\n"));
}

//------------------------------------------------------------------------------
// Function Name: HotPlugService()
// Function Description: Implement Hot Plug Service Loop activities
//
// Accepts: none
// Returns: An error code that indicates success or cause of failure
// Globals: LinkProtectionLevel
//------------------------------------------------------------------------------
void HotPlugService (void)
{
	TX_DEBUG_PRINT(("[MHL]: >>HotPlugService()\n"));
	DisableInterrupts(0xFF);
	
	//siMhlTx.VIC = g_edid.VideoDescriptor[0];	// use 1st mode supported by sink
	//mhlTxAv.AudioFs = Afs;
	siMhlTx_Init();
}

//------------------------------------------------------------------------------
// Function Name: OnDownstreamRxPoweredDown()
// Function Description: HDMI cable unplug handle.
//------------------------------------------------------------------------------
void OnDownstreamRxPoweredDown (void)
{
	TX_DEBUG_PRINT (("[MHL]: DSRX -> Powered Down\n"));
	mhlTxConfig.dsRxPoweredUp = FALSE;
	SiiMhlTxDrvTmdsControl( FALSE );
	ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPUT_MODE_MASK,OUTPUT_MODE_DVI);	// Set to DVI output mode to reset HDCP
}

//------------------------------------------------------------------------------
// Function Name: OnDownstreamRxPoweredUp()
// Function Description: DSRX power up handle.
//------------------------------------------------------------------------------
void OnDownstreamRxPoweredUp (void)
{
	TX_DEBUG_PRINT (("[MHL]: DSRX -> Powered Up\n"));
	mhlTxConfig.dsRxPoweredUp = TRUE;

	HotPlugService();
}

//------------------------------------------------------------------------------
// Function Name: OnHdmiCableDisconnected()
// Function Description: HDMI cable unplug handle.
//------------------------------------------------------------------------------
void OnHdmiCableDisconnected (void)
{
	TX_DEBUG_PRINT (("[MHL]: HDMI Disconnected\n"));

	mhlTxConfig.hdmiCableConnected = FALSE;

#ifdef DEV_SUPPORT_EDID
	mhlTxEdid.edidDataValid = FALSE;
#endif

	OnDownstreamRxPoweredDown();
}

//------------------------------------------------------------------------------
// Function Name: OnHdmiCableConnected()
// Function Description: HDMI cable plug in handle.
//
// Accepts: none
// Returns: TRUE or FALSE
// Globals: none
//------------------------------------------------------------------------------
uint8_t OnHdmiCableConnected (void)
{
	TX_DEBUG_PRINT (("[MHL]: HDMI Cable Connected\n"));
	
	mhlTxConfig.hdmiCableConnected = TRUE;
#if 0	//Disabled by zhy for itv hdmi driver 2011.11.01
#ifdef DEV_SUPPORT_HDCP
	if ((mhlTxHdcp.HDCP_TxSupports == TRUE) 
		&& (mhlTxHdcp.HDCP_AksvValid == TRUE) 
		&& (mhlTxHdcp.HDCPAuthenticated == VMD_HDCP_AUTHENTICATED)) 
	{
		WriteIndexedRegister(INDEXED_PAGE_0, 0xCE, 0x00); // Clear BStatus
		WriteIndexedRegister(INDEXED_PAGE_0, 0xCF, 0x00);
	}
#endif
	
#ifdef DEV_SUPPORT_EDID
	TX_DEBUG_PRINT (("[MHL]: Reading EDID\n"));
	if( DoEdidRead() == EDID_DDC_BUS_READ_FAILURE )
		return FALSE;
#endif

#ifdef READKSV
	ReadModifyWriteTPI(0xBB, 0x08, 0x08);
#endif

	if (IsHDMI_Sink())              // select output mode (HDMI/DVI) according to sink capabilty
	{
		TX_DEBUG_PRINT (("[MHL]: HDMI Sink Detected\n"));
		ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPUT_MODE_MASK, OUTPUT_MODE_HDMI);
	}
	else
	{
		TX_DEBUG_PRINT (("[MHL]: DVI Sink Detected\n"));
		ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPUT_MODE_MASK, OUTPUT_MODE_DVI);
	}

	OnDownstreamRxPoweredUp();		// RX power not determinable? Force to on for now.
#endif
	return TRUE;
}

//------------------------------------------------------------------------------
// Function Name: siMhlTx_VideoSel()
// Function Description: Select output video mode
//
// Accepts: Video mode
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void siMhlTx_VideoSel (uint8_t vmode)
{
	mhlTxAv.HDMIVideoFormat		= VMD_HDMIFORMAT_CEA_VIC;
	mhlTxAv.VIC 				= vmode;
	mhlTxAv.ColorSpace 			= RGB;
	mhlTxAv.ColorDepth			= VMD_COLOR_DEPTH_8BIT;
	mhlTxAv.SyncMode 			= EXTERNAL_HSVSDE;
		
	switch (vmode)
	{
		case HDMI_480I60_4X3:
		case HDMI_576I50_4X3:
			mhlTxAv.AspectRatio 	= VMD_ASPECT_RATIO_4x3;
			mhlTxAv.Colorimetry	= COLORIMETRY_601;
			mhlTxAv.TclkSel		= X2;
			break;
			
		case HDMI_480I60_16X9:
		case HDMI_576I50_16X9:
			mhlTxAv.AspectRatio 	= VMD_ASPECT_RATIO_16x9;
			mhlTxAv.Colorimetry	= COLORIMETRY_601;
			mhlTxAv.TclkSel		= X2;
			break;
			
		case HDMI_480P60_4X3:
		case HDMI_576P50_4X3:
		case HDMI_640X480P:
			mhlTxAv.AspectRatio 	= VMD_ASPECT_RATIO_4x3;
			mhlTxAv.Colorimetry	= COLORIMETRY_601;
			mhlTxAv.TclkSel		= X1;
			break;

		case HDMI_480P60_16X9:
		case HDMI_576P50_16X9:
			mhlTxAv.AspectRatio 	= VMD_ASPECT_RATIO_16x9;
			mhlTxAv.Colorimetry	= COLORIMETRY_601;
			mhlTxAv.TclkSel		= X1;
			break;
			
		case HDMI_720P60:
		case HDMI_720P50:
		case HDMI_1080I60:
		case HDMI_1080I50:
		case HDMI_1080P24:
		case HDMI_1080P25:
		case HDMI_1080P30:				
			mhlTxAv.AspectRatio 	= VMD_ASPECT_RATIO_16x9;
			mhlTxAv.Colorimetry	= COLORIMETRY_709;
			mhlTxAv.TclkSel		= X1;
			break;
			
		default:
			break;
	}
}

//------------------------------------------------------------------------------
// Function Name: siMhlTx_AudioSel()
// Function Description: Select output audio mode
//
// Accepts: Audio Fs
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void siMhlTx_AudioSel (uint8_t Afs)
{
	mhlTxAv.AudioMode			= AMODE_I2S;
	mhlTxAv.AudioChannels		= ACHANNEL_2CH;
	mhlTxAv.AudioFs				= Afs;
	mhlTxAv.AudioWordLength		= ALENGTH_16BITS;//ALENGTH_24BITS;
	mhlTxAv.AudioI2SFormat		= (MCLK256FS << 4) |SCK_SAMPLE_RISING_EDGE |0x00;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define SILICON_IMAGE_ADOPTER_ID 322
#define TRANSCODER_DEVICE_ID 0x9232
//
// Software power states are a little bit different than the hardware states but
// a close resemblance exists.
//
// D3 matches well with hardware state. In this state we receive RGND interrupts
// to initiate wake up pulse and device discovery
//
// Chip wakes up in D2 mode and interrupts MCU for RGND. Firmware changes the 92326
// into D0 mode and sets its own operation mode as POWER_STATE_D0_NO_MHL because
// MHL connection has not yet completed.
//
// For all practical reasons, firmware knows only two states of hardware - D0 and D3.
//
// We move from POWER_STATE_D0_NO_MHL to POWER_STATE_D0_MHL only when MHL connection
// is established.
/*
//
//                             S T A T E     T R A N S I T I O N S
//
//
//                    POWER_STATE_D3                      POWER_STATE_D0_NO_MHL
//                   /--------------\                        /------------\
//                  /                \                      /     D0       \
//                 /                  \                \   /                \
//                /   DDDDDD  333333   \     RGND       \ /   NN  N  OOO     \
//                |   D     D     33   |-----------------|    N N N O   O     |
//                |   D     D  3333    |      IRQ       /|    N  NN  OOO      |
//                \   D     D      33  /               /  \                  /
//                 \  DDDDDD  333333  /                    \   CONNECTION   /
//                  \                /\                     /\             /
//                   \--------------/  \  TIMEOUT/         /  -------------
//                         /|\          \-------/---------/        ||
//                        / | \            500ms\                  ||
//                          |                     \                ||
//                          |  RSEN_LOW                            || MHL_EST
//                           \ (STATUS)                            ||  (IRQ)
//                            \                                    ||
//                             \      /------------\              //
//                              \    /              \            //
//                               \  /                \          //
//                                \/                  \ /      //
//                                 |    CONNECTED     |/======//    
//                                 |                  |\======/
//                                 \   (OPERATIONAL)  / \
//                                  \                /
//                                   \              /
//                                    \-----------/
//                                   POWER_STATE_D0_MHL
//
//
//
*/
#define	POWER_STATE_D3				3
#define	POWER_STATE_D0_NO_MHL		2
#define	POWER_STATE_D0_MHL			0
#define	POWER_STATE_FIRST_INIT		0xFF

//
// To remember the current power state.
//
static	uint8_t	fwPowerState = POWER_STATE_FIRST_INIT;

//
// This flag is set to TRUE as soon as a INT1 RSEN CHANGE interrupt arrives and
// a deglitch timer is started.
//
// We will not get any further interrupt so the RSEN LOW status needs to be polled
// until this timer expires.
//
static	bool_t	deglitchingRsenNow = FALSE;

//
// To serialize the RCP commands posted to the CBUS engine, this flag
// is maintained by the function SiiMhlTxDrvSendCbusCommand()
//
static	bool_t	mscCmdInProgress;	// FALSE when it is okay to send a new command
//
// Preserve Downstream HPD status
//
static	uint8_t	dsHpdStatus = 0;


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadModifyWritePage0 ()
//
// PURPOSE		:	Set or clear individual bits in a Page0 register.
//
// INPUT PARAMS	:	Offset	-	the offset of the Page0 register to be modified.
//					Mask	-	"1" for each Page0 register bit that needs to be
//								modified
//					Data	-	The desired value for the register bits in their
//								proper positions
//
void ReadModifyWritePage0(uint8_t Offset, uint8_t Mask, uint8_t Data)
{
	uint8_t Temp;

	Temp = sii_I2CReadByte(PAGE_0_0X72, Offset);		// Read the current value of the register.
	Temp &= ~Mask;					// Clear the bits that are set in Mask.
	Temp |= (Data & Mask);			// OR in new value. Apply Mask to Value for safety.
	sii_I2CWriteByte(PAGE_0_0X72, Offset, Temp);		// Write new value back to register.
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadByteCBUS ()
//
// PURPOSE		:	Read the value from a CBUS register.
//
// INPUT PARAMS	:	Offset - the offset of the CBUS register to be read.
//
// RETURNS		:	The value read from the CBUS register.
//
uint8_t ReadByteCBUS (uint8_t Offset)
{
	return I2CReadByte(PAGE_CBUS_0XC8, Offset);
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	WriteByteCBUS ()
//
// PURPOSE		:	Write a value to a CBUS register.
//
// INPUT PARAMS	:	Offset	-	the offset of the Page0 register to be written.
//					Data	-	the value to be written.
//
void WriteByteCBUS(uint8_t Offset, uint8_t Data)
{
	I2CWriteByte(PAGE_CBUS_0XC8, Offset, Data);
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION		:	ReadModifyWriteCBUS ()
//
// PURPOSE		:	Set or clear individual bits on CBUS page.
//
// INPUT PARAMS	:	Offset	-	the offset of the CBUS register to be modified.
//					Mask	-	"1" for each CBUS register bit that needs to be
//								modified
//					Data	-	The desired value for the register bits in their
//								proper positions
//
void ReadModifyWriteCBUS(uint8_t Offset, uint8_t Mask, uint8_t Value)
{
    uint8_t Temp;

    Temp = ReadByteCBUS(Offset);
    Temp &= ~Mask;
    Temp |= (Value & Mask);
    WriteByteCBUS(Offset, Temp);
}


#define	I2C_READ_MODIFY_WRITE(saddr,offset,mask)	sii_I2CWriteByte(saddr, offset, sii_I2CReadByte(saddr, offset) | (mask));
#define ReadModifyWriteByteCBUS(offset,andMask,orMask)  WriteByteCBUS(offset,(ReadByteCBUS(offset)&andMask) | orMask)

#define	SET_BIT(saddr,offset,bitnumber)		I2C_READ_MODIFY_WRITE(saddr,offset, (1<<bitnumber))
#define	CLR_BIT(saddr,offset,bitnumber)		sii_I2CWriteByte(saddr, offset, sii_I2CReadByte(saddr, offset) & ~(1<<bitnumber))
//
// 90[0] = Enable / Disable MHL Discovery on MHL link
//
#define	DISABLE_DISCOVERY				CLR_BIT(PAGE_0_0X72, 0x90, 0);
#define	ENABLE_DISCOVERY				SET_BIT(PAGE_0_0X72, 0x90, 0);

#define STROBE_POWER_ON					CLR_BIT(PAGE_0_0X72, 0x90, 1);
//
//	Look for interrupts on INTR4 (Register 0x74)
//		7 = RSVD		(reserved)
//		6 = RGND Rdy	(interested)
//		5 = VBUS Low	(ignore)	
//		4 = CBUS LKOUT	(interested)
//		3 = USB EST		(interested)
//		2 = MHL EST		(interested)
//		1 = RPWR5V Change	(ignore)
//		0 = SCDT Change	(only if necessary)
//
#define	INTR_4_DESIRED_MASK				(BIT0 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define	UNMASK_INTR_4_INTERRUPTS		sii_I2CWriteByte(PAGE_0_0X72, 0x78, INTR_4_DESIRED_MASK)
#define	MASK_INTR_4_INTERRUPTS			sii_I2CWriteByte(PAGE_0_0X72, 0x78, 0x00)

//	Look for interrupts on INTR_2 (Register 0x72)
//		7 = bcap done			(ignore)
//		6 = parity error		(ignore)
//		5 = ENC_EN changed		(ignore)
//		4 = no premable			(ignore)
//		3 = ACR CTS changed		(ignore)
//		2 = ACR Pkt Ovrwrt		(ignore)
//		1 = TCLK_STBL changed	(interested)
//		0 = Vsync				(ignore)
#define	INTR_2_DESIRED_MASK				(BIT1)
#define	UNMASK_INTR_2_INTERRUPTS		sii_I2CWriteByte(PAGE_0_0X72, 0x76, INTR_2_DESIRED_MASK)
#define	MASK_INTR_2_INTERRUPTS			sii_I2CWriteByte(PAGE_0_0X72, 0x76, 0x00)

//	Look for interrupts on INTR_1 (Register 0x71)
//		7 = RSVD		(reserved)
//		6 = MDI_HPD		(interested)
//		5 = RSEN CHANGED(interested)	
//		4 = RSVD		(reserved)
//		3 = RSVD		(reserved)
//		2 = RSVD		(reserved)
//		1 = RSVD		(reserved)
//		0 = RSVD		(reserved)

#define	INTR_1_DESIRED_MASK				(BIT5 | BIT6)
#define	UNMASK_INTR_1_INTERRUPTS		sii_I2CWriteByte(PAGE_0_0X72, 0x75, INTR_1_DESIRED_MASK)
#define	MASK_INTR_1_INTERRUPTS			sii_I2CWriteByte(PAGE_0_0X72, 0x75, 0x00)

//	Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
//		7 = RSVD			(reserved)
//		6 = MSC_RESP_ABORT	(interested)
//		5 = MSC_REQ_ABORT	(interested)	
//		4 = MSC_REQ_DONE	(interested)
//		3 = MSC_MSG_RCVD	(interested)
//		2 = DDC_ABORT		(interested)
//		1 = RSVD			(reserved)
//		0 = rsvd				(reserved)
#define	INTR_CBUS1_DESIRED_MASK			(BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define	UNMASK_CBUS1_INTERRUPTS			WriteByteCBUS(0x09, INTR_CBUS1_DESIRED_MASK)
#define	MASK_CBUS1_INTERRUPTS			WriteByteCBUS(0x09, 0x00)

//	Look for interrupts on CBUS:CBUS_INTR_STATUS2 [0xC8:0x1E]
//		7 = RSVD			(reserved)
//		6 = RSVD			(reserved)
//		5 = RSVD			(reserved)
//		4 = RSVD			(reserved)
//		3 = WRT_STAT_RECD	(interested)
//		2 = SET_INT_RECD	(interested)
//		1 = RSVD			(reserved)
//		0 = WRT_BURST_RECD (interested)
#define	INTR_CBUS2_DESIRED_MASK			(BIT0 | BIT2 | BIT3)
#define	UNMASK_CBUS2_INTERRUPTS			WriteByteCBUS(0x1F, INTR_CBUS2_DESIRED_MASK)
#define	MASK_CBUS2_INTERRUPTS			WriteByteCBUS(0x1F, 0x00)

//
// Local scope functions.
//
static	void	Int4Isr( void );
static	void	Int1RsenIsr( void );
static	void	MhlCbusIsr( void );
static	void	DeglitchRsenLow( void );

static	void	CbusReset(void);
static	void	SwitchToD0( void );
static	void	SwitchToD3( void );
static	void	WriteInitialRegisterValues ( void );
static	void	InitCBusRegs( void );
static	void	ForceUsbIdSwitchOpen ( void );
static	void	ReleaseUsbIdSwitchOpen ( void );
static	void	MhlTxDrvProcessConnection ( void );
static	void	MhlTxDrvProcessDisconnection ( void );
static	void	ApplyDdcAbortSafety(void);


#define	APPLY_PLL_RECOVERY

#ifdef APPLY_PLL_RECOVERY
static  void    SiiMhlTxDrvRecovery( void );
#endif

//////////////////////////////////////
//  internal utility functions
//


////////////////////////////////////////////////////////////////////
//
// E X T E R N A L L Y    E X P O S E D   A P I    F U N C T I O N S
//
////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxChipInitialize
//
// Chip specific initialization.
// This function is for SiI 92326 Initialization: HW Reset, Interrupt enable.
//
//
//////////////////////////////////////////////////////////////////////////////

bool_t SiiMhlTxChipInitialize ( void )
{
	// Toggle TX reset pin
//	pinMHLTxHwResetN = LOW;
//	DelayMS(TX_HW_RESET_PERIOD);
//	pinMHLTxHwResetN = HIGH;
	SiI92326_Reset();
	//
	// Setup our own timer for now. 50ms.
	//
	HalTimerSet( ELAPSED_TIMER, MONITORING_PERIOD );

	TPImode = FALSE;
	dsHpdStatus = 0;
	   
	TX_DEBUG_PRINT( ("[MHL]: SiiMhlTxChipInitialize: 92%02X\n", (int)sii_I2CReadByte(PAGE_0_0X72, 0x02)) );

	// setup device registers. Ensure RGND interrupt would happen.
	WriteInitialRegisterValues();

	sii_I2CWriteByte(PAGE_0_0X72, 0x71, INTR_1_DESIRED_MASK); // [dave] clear HPD & RSEN interrupts

	// Setup interrupt masks for all those we are interested.
	UNMASK_INTR_4_INTERRUPTS;
	UNMASK_INTR_1_INTERRUPTS;
	
	// CBUS interrupts are unmasked after performing the reset.
	// UNMASK_CBUS1_INTERRUPTS;
	// UNMASK_CBUS2_INTERRUPTS;

	SwitchToD3();

	return TRUE;
}
///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxDeviceIsr
//
// This function must be called from a master interrupt handler or any polling
// loop in the host software if during initialization call the parameter
// interruptDriven was set to TRUE. SiiMhlTxGetEvents will not look at these
// events assuming firmware is operating in interrupt driven mode. MhlTx component
// performs a check of all its internal status registers to see if a hardware event
// such as connection or disconnection has happened or an RCP message has been
// received from the connected device. Due to the interruptDriven being TRUE,
// MhlTx code will ensure concurrency by asking the host software and hardware to
// disable interrupts and restore when completed. Device interrupts are cleared by
// the MhlTx component before returning back to the caller. Any handling of
// programmable interrupt controller logic if present in the host will have to
// be done by the caller after this function returns back.

// This function has no parameters and returns nothing.
//
// This is the master interrupt handler for 92326. It calls sub handlers
// of interest. Still couple of status would be required to be picked up
// in the monitoring routine (Sii92326TimerIsr)
//
// To react in least amount of time hook up this ISR to processor's
// interrupt mechanism.
//
// Just in case environment does not provide this, set a flag so we
// call this from our monitor (Sii92326TimerIsr) in periodic fashion.
//
// Device Interrupts we would look at
//		RGND		= to wake up from D3
//		MHL_EST 	= connection establishment
//		CBUS_LOCKOUT= Service USB switch
//		RSEN_LOW	= Disconnection deglitcher
//		CBUS 		= responder to peer messages
//					  Especially for DCAP etc time based events
//
void SiiMhlTxDeviceIsr( void )
{
	uint8_t InterruptStatusImage;
	//
	// Look at discovery interrupts if not yet connected.
	//
	if( POWER_STATE_D0_MHL != fwPowerState )
	{
		//
		// Check important RGND, MHL_EST, CBUS_LOCKOUT and SCDT interrupts
		// During D3 we only get RGND but same ISR can work for both states
		//
        	//if (0==pinMHLTxInt)
		{
			Int4Isr();
		}
	}
	else if( POWER_STATE_D0_MHL == fwPowerState )
	{		
		//
		// Check RSEN LOW interrupt and apply deglitch timer for transition
		// from connected to disconnected state.
		//
		if(HalTimerExpired( TIMER_TO_DO_RSEN_CHK ))
		{
			//
			// If no MHL cable is connected, we may not receive interrupt for RSEN at all
			// as nothing would change. Poll the status of RSEN here.
			//
			// Also interrupt may come only once who would have started deglitch timer.
			// The following function will look for expiration of that before disconnection.
			//
			if(deglitchingRsenNow)
			{
				TX_DEBUG_PRINT(("[MHL]: [%d]: deglitchingRsenNow.\n", (int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD)));
				DeglitchRsenLow();
			}
			else
			{
				Int1RsenIsr();
			}
		}
#ifdef APPLY_PLL_RECOVERY
		//
		// Trigger a PLL recovery if SCDT is high or FIFO overflow has happened.
		//
		SiiMhlTxDrvRecovery();

#endif // APPLY_PLL_RECOVERY

		//
		// TPI interrupt status check
		//
		InterruptStatusImage = ReadByteTPI(TPI_INTERRUPT_STATUS_REG);
		// Check if Audio Error event has occurred:
		if (InterruptStatusImage & AUDIO_ERROR_EVENT)
		{
			// The hardware handles the event without need for host intervention (PR, p. 31)
			ClearInterrupt(AUDIO_ERROR_EVENT);
		}
		//
		// Check for HDCP interrupt
		//
#ifdef DEV_SUPPORT_HDCP
		if ((mhlTxConfig.hdmiCableConnected == TRUE)
			&& (mhlTxConfig.dsRxPoweredUp == TRUE)
			&& (mhlTxHdcp.HDCPAuthenticated == VMD_HDCP_AUTHENTICATED))
		{
			HDCP_CheckStatus(InterruptStatusImage);
		}
#endif
		
		//
		// Check for any peer messages for DCAP_CHG etc
		// Dispatch to have the CBUS module working only once connected.
		//
		MhlCbusIsr();
	}
}
/*
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvAcquireUpstreamHPDControl
//
// Acquire the direct control of Upstream HPD.
//
void SiiMhlTxDrvAcquireUpstreamHPDControl(void)
{
	// set reg_hpd_out_ovr_en to first control the hpd
	SET_BIT(PAGE_0_0X72, 0x79, 4);
	TX_DEBUG_PRINT(("[MHL]: Upstream HPD Acquired.\n"));
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow
//
// Acquire the direct control of Upstream HPD.
//
void SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow(void)
{
	// set reg_hpd_out_ovr_en to first control the hpd and clear reg_hpd_out_ovr_val
 	ReadModifyWritePage0(0x79, BIT5 | BIT4, BIT4);	// Force upstream HPD to 0 when not in MHL mode.
	TX_DEBUG_PRINT(("[MHL]: Upstream HPD Acquired - driven low.\n"));
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvReleaseUpstreamHPDControl
//
// Release the direct control of Upstream HPD.
//
void SiiMhlTxDrvReleaseUpstreamHPDControl(void)
{
   	// Un-force HPD (it was kept low, now propagate to source
	// let HPD float by clearing reg_hpd_out_ovr_en
   	CLR_BIT(PAGE_0_0X72, 0x79, 4);
	TX_DEBUG_PRINT(("[MHL]: Upstream HPD released.\n"));
}
*/
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvTmdsControl
//
// Control the TMDS output. MhlTx uses this to support RAP content on and off.
//
void SiiMhlTxDrvTmdsControl( bool_t enable )
{
	uint8_t Mask,Value;
	
	if( enable )
	{
		SET_BIT(PAGE_0_0X72, 0x80, 4);
		TX_DEBUG_PRINT(("[MHL]: MHL Output Enabled\n"));
		//SiiMhlTxDrvReleaseUpstreamHPDControl();  // this triggers an EDID read

		Mask  = LINK_INTEGRITY_MODE_MASK | TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK;
#ifdef DEV_SUPPORT_HDCP
       	Value = LINK_INTEGRITY_DYNAMIC   | TMDS_OUTPUT_CONTROL_ACTIVE | ( mhlTxHdcp.HDCPAuthenticated ? AV_MUTE_NORMAL : AV_MUTE_MUTED);
#else
       	Value = LINK_INTEGRITY_DYNAMIC   | TMDS_OUTPUT_CONTROL_ACTIVE | AV_MUTE_NORMAL;
#endif
		ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, Mask, Value);

#ifdef DEV_SUPPORT_HDCP
		if (mhlTxHdcp.HDCP_Started == FALSE)
		{
#ifdef APPLY_PLL_RECOVERY
			SiiMhlTxDrvRecovery();
			DelayMS(200);
#endif
			HDCP_On();
		}
#endif
	}
	else
	{
		CLR_BIT(PAGE_0_0X72, 0x80, 4);
		TX_DEBUG_PRINT(("[MHL]: MHL Ouput Disabled\n"));

		ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK,
			TMDS_OUTPUT_CONTROL_POWER_DOWN   | AV_MUTE_MUTED);

#ifdef DEV_SUPPORT_HDCP
		HDCP_Off();
#endif
	}
}
/*
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvNotifyEdidChange
//
// MhlTx may need to inform upstream device of an EDID change. This can be
// achieved by toggling the HDMI HPD signal or by simply calling EDID read
// function.
//
void	SiiMhlTxDrvNotifyEdidChange ( void )
{
    	TX_DEBUG_PRINT(("[MHL]: SiiMhlTxDrvNotifyEdidChange\n"));
	//
	// Prepare to toggle HPD to upstream
	//
    	SiiMhlTxDrvAcquireUpstreamHPDControl();

	// reg_hpd_out_ovr_val = LOW to force the HPD low
	CLR_BIT(PAGE_0_0X72, 0x79, 5);

	// wait a bit
	DelayMS(110);

    	// force upstream HPD back to high by reg_hpd_out_ovr_val = HIGH
	SET_BIT(PAGE_0_0X72, 0x79, 5);
}
*/

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvSendCbusCommand
//
// Write the specified Sideband Channel command to the CBUS.
// Command can be a MSC_MSG command (RCP/RAP/RCPK/RCPE/RAPK), or another command 
// such as READ_DEVCAP, SET_INT, WRITE_STAT, etc.
//
// Parameters:  
//              pReq    - Pointer to a cbus_req_t structure containing the 
//                        command to write
// Returns:     TRUE    - successful write
//              FALSE   - write failed
//
bool_t SiiMhlTxDrvSendCbusCommand ( cbus_req_t *pReq  )
{
    bool_t  success = TRUE;

    uint8_t i, startbit;

    //
    // If not connected, return with error
    //
    if( (POWER_STATE_D0_MHL != fwPowerState ) || (mscCmdInProgress))
    {
	    TX_DEBUG_PRINT(("[MHL]: Error: fwPowerState: %02X, or CBUS(0x0A):%02X mscCmdInProgress = %d\n",
				(int) fwPowerState,
				(int) ReadByteCBUS(0x0a),
				(int) mscCmdInProgress));
		
   		return FALSE;
    }
    // Now we are getting busy
	mscCmdInProgress = TRUE;

    TX_DEBUG_PRINT(("[MHL]: Sending MSC command %02X, %02X, %02X, %02X\n", 
			(int)pReq->command, 
			(int)(pReq->offsetData),
		 	(int)pReq->payload_u.msgData[0],
		 	(int)pReq->payload_u.msgData[1]));

    /****************************************************************************************/
    /* Setup for the command - write appropriate registers and determine the correct        */
    /*                         start bit.                                                   */
    /****************************************************************************************/

    // Set the offset and outgoing data byte right away
	WriteByteCBUS( (REG_CBUS_PRI_ADDR_CMD    & 0xFF), pReq->offsetData); 	// set offset
	WriteByteCBUS( (REG_CBUS_PRI_WR_DATA_1ST & 0xFF), pReq->payload_u.msgData[0]);
	
    startbit = 0x00;
    switch ( pReq->command )
    {
		case MHL_SET_INT:	// Set one interrupt register = 0x60
			startbit = MSC_START_BIT_WRITE_REG;
			break;

       case MHL_WRITE_STAT:	// Write one status register = 0x60 | 0x80
          	startbit = MSC_START_BIT_WRITE_REG;
          	break;

       case MHL_READ_DEVCAP:	// Read one device capability register = 0x61
			startbit = MSC_START_BIT_READ_REG;
			break;

		case MHL_GET_STATE:			// 0x62 -
		case MHL_GET_VENDOR_ID:		// 0x63 - for vendor id	
		case MHL_SET_HPD:			// 0x64	- Set Hot Plug Detect in follower
		case MHL_CLR_HPD:			// 0x65	- Clear Hot Plug Detect in follower
		case MHL_GET_SC1_ERRORCODE:		// 0x69	- Get channel 1 command error code
		case MHL_GET_DDC_ERRORCODE:		// 0x6A	- Get DDC channel command error code.
		case MHL_GET_MSC_ERRORCODE:		// 0x6B	- Get MSC command error code.
		case MHL_GET_SC3_ERRORCODE:		// 0x6D	- Get channel 3 command error code.
			WriteByteCBUS( (REG_CBUS_PRI_ADDR_CMD & 0xFF), pReq->command );
			startbit = MSC_START_BIT_MSC_CMD;
			break;

       case MHL_MSC_MSG:
			WriteByteCBUS( (REG_CBUS_PRI_WR_DATA_2ND & 0xFF), pReq->payload_u.msgData[1] );
			WriteByteCBUS( (REG_CBUS_PRI_ADDR_CMD & 0xFF), pReq->command );
			startbit = MSC_START_BIT_VS_CMD;
			break;

       case MHL_WRITE_BURST:
			ReadModifyWriteCBUS((REG_MSC_WRITE_BURST_LEN & 0xFF),0x0F,pReq->length -1);
			
			// Now copy all bytes from array to local scratchpad
			if (NULL == pReq->payload_u.pdatabytes)
			{
				TX_DEBUG_PRINT(("[MHL]: Put pointer to WRITE_BURST data in req.pdatabytes!!!\n\n"));
			}
			else
			{
				uint8_t *pData = pReq->payload_u.pdatabytes;
				TX_DEBUG_PRINT(("[MHL]: Writing data into scratchpad\n\n"));
				for ( i = 0; i < pReq->length; i++ )
				{
			    		WriteByteCBUS( (REG_CBUS_SCRATCHPAD_0 & 0xFF) + i, *pData++ );
				}
			}
			startbit = MSC_START_BIT_WRITE_BURST;
            break;

       default:
            success = FALSE;
            	break;
    }

    /****************************************************************************************/
    /* Trigger the CBUS command transfer using the determined start bit.                    */
    /****************************************************************************************/

    if ( success )
    {
        WriteByteCBUS( REG_CBUS_PRI_START & 0xFF, startbit );
    }
    else
    {
        TX_DEBUG_PRINT(("[MHL]: SiiMhlTxDrvSendCbusCommand failed\n\n"));
    }

    return( success );
}
bool_t SiiMhlTxDrvCBusBusy(void)
{
    return mscCmdInProgress ? TRUE :FALSE;
}
////////////////////////////////////////////////////////////////////
//
// L O C A L    F U N C T I O N S
//
////////////////////////////////////////////////////////////////////
// Int1RsenIsr
//
// This interrupt is used only to decide if the MHL is disconnected
// The disconnection is determined by looking at RSEN LOW and applying
// all MHL compliant disconnect timings and deglitch logic.
//
//	Look for interrupts on INTR_1 (Register 0x71)
//		7 = RSVD		(reserved)
//		6 = MDI_HPD		(interested)
//		5 = RSEN CHANGED(interested)	
//		4 = RSVD		(reserved)
//		3 = RSVD		(reserved)
//		2 = RSVD		(reserved)
//		1 = RSVD		(reserved)
//		0 = RSVD		(reserved)
////////////////////////////////////////////////////////////////////
void	Int1RsenIsr( void )
{
	uint8_t	reg71 = sii_I2CReadByte(PAGE_0_0X72, 0x71);
	uint8_t	rsen  = sii_I2CReadByte(PAGE_0_0X72, 0x09) & BIT2;
	
	// Look at RSEN interrupt.
	// If RSEN interrupt is lost, check if we should deglitch using the RSEN status only.
	if( (reg71 & BIT5) ||
		((FALSE == deglitchingRsenNow) && (rsen == 0x00)) )
	{
		TX_DEBUG_PRINT (("[MHL]: Got INTR_1: reg71 = %02X, rsen = %02X\n", (int) reg71, (int) rsen));
		//
		// RSEN becomes LOW in SYS_STAT register 0x72:0x09[2]
		// SYS_STAT	==> bit 7 = VLOW, 6:4 = MSEL, 3 = TSEL, 2 = RSEN, 1 = HPD, 0 = TCLK STABLE
		//
		// Start countdown timer for deglitch
		// Allow RSEN to stay low this much before reacting
		//
		if(rsen == 0x00)
		{
			TX_DEBUG_PRINT (("[MHL]: Int1RsenIsr: Start T_SRC_RSEN_DEGLITCH (%d ms) before disconnection\n",
									 (int)(T_SRC_RSEN_DEGLITCH) ) );
			//
			// We got this interrupt due to cable removal
			// Start deglitch timer
			//
			HalTimerSet(TIMER_TO_DO_RSEN_DEGLITCH, T_SRC_RSEN_DEGLITCH);

			deglitchingRsenNow = TRUE;
		}
		else if( deglitchingRsenNow )
		{
			TX_DEBUG_PRINT(("[MHL]: [%d]: Ignore now, RSEN is high. This was a glitch.\n", (int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD)));
			//
			// Ignore now, this was a glitch
			//
			deglitchingRsenNow = FALSE;
		}
		// Clear MDI_RSEN interrupt
		sii_I2CWriteByte(PAGE_0_0X72, 0x71, BIT5);
	}
	else if( deglitchingRsenNow )
	{
		TX_DEBUG_PRINT(("[MHL]: [%d]: Ignore now coz (reg71 & BIT5) has been cleared. This was a glitch.\n", (int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD)));
		//
		// Ignore now, this was a glitch
		//
		deglitchingRsenNow = FALSE;
	}
}
//////////////////////////////////////////////////////////////////////////////
//
// DeglitchRsenLow
//
// This function looks at the RSEN signal if it is low.
//
// The disconnection will be performed only if we were in fully MHL connected
// state for more than 400ms AND a 150ms deglitch from last interrupt on RSEN
// has expired.
//
// If MHL connection was never established but RSEN was low, we unconditionally
// and instantly process disconnection.
//
static void DeglitchRsenLow( void )
{
	TX_DEBUG_PRINT(("[MHL]: DeglitchRsenLow RSEN <72:09[2]> = %02X\n", (int) (sii_I2CReadByte(PAGE_0_0X72, 0x09)) ));

	if((sii_I2CReadByte(PAGE_0_0X72, 0x09) & BIT2) == 0x00)
	{
		TX_DEBUG_PRINT(("[MHL]: [%d]: RSEN is Low.\n", (int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD)));
		//
		// If no MHL cable is connected or RSEN deglitch timer has started,
		// we may not receive interrupts for RSEN.
		// Monitor the status of RSEN here.
		//
		// 
		// First check means we have not received any interrupts and just started
		// but RSEN is low. Case of "nothing" connected on MHL receptacle
		//
		if((POWER_STATE_D0_MHL == fwPowerState)    && HalTimerExpired(TIMER_TO_DO_RSEN_DEGLITCH) )
		{
			// Second condition means we were fully operational, then a RSEN LOW interrupt
			// occured and a DEGLITCH_TIMER per MHL specs started and completed.
			// We can disconnect now.
			//
			TX_DEBUG_PRINT(("[MHL]: Disconnection due to RSEN Low\n"));

			deglitchingRsenNow = FALSE;

			// FP1226: Toggle MHL discovery to level the voltage to deterministic vale.
			DISABLE_DISCOVERY;
			ENABLE_DISCOVERY;
			//
			// We got here coz cable was never connected
			//
			dsHpdStatus &= ~BIT6;  //cable disconnect implies downstream HPD low
			WriteByteCBUS(0x0D, dsHpdStatus);
			SiiMhlTxNotifyDsHpdChange( 0 );
			MhlTxDrvProcessDisconnection();
		}
	}
	else
	{
		//
		// Deglitch here:
		// RSEN is not low anymore. Reset the flag.
		// This flag will be now set on next interrupt.
		//
		// Stay connected
		//
		deglitchingRsenNow = FALSE;
	}
}
///////////////////////////////////////////////////////////////////////////
// WriteInitialRegisterValues
//
//
///////////////////////////////////////////////////////////////////////////
static void WriteInitialRegisterValues ( void )
{
	TX_DEBUG_PRINT(("[MHL]: WriteInitialRegisterValues\n"));
	
	// Power Up
	sii_I2CWriteByte(PAGE_1_0X7A, 0x3D, 0x3F);	// Power up CVCC 1.2V core
	sii_I2CWriteByte(PAGE_2_0X92, 0x11, 0x01);	// Enable TxPLL Clock
	sii_I2CWriteByte(PAGE_2_0X92, 0x12, 0x15);	// Enable Tx Clock Path & Equalizer
	sii_I2CWriteByte(PAGE_0_0X72, 0x08, 0x35);	// Power Up TMDS Tx Core

	sii_I2CWriteByte(PAGE_0_0X72, 0xA0, 0xD0);
	sii_I2CWriteByte(PAGE_0_0X72, 0xA1, 0xFC);	// Disable internal MHL driver

       // 1x mode
	sii_I2CWriteByte(PAGE_0_0X72, 0xA3, 0xD9);
	sii_I2CWriteByte(PAGE_0_0X72, 0xA6, 0x0C);

	sii_I2CWriteByte(PAGE_0_0X72, 0x2B, 0x01);	// Enable HDCP Compliance safety

	//
	// CBUS & Discovery
	// CBUS discovery cycle time for each drive and float = 100us
	//
	ReadModifyWritePage0(0x90, BIT3 | BIT2, BIT2);

	// Do not perform RGND impedance detection if connected to SiI 9290
	//
	sii_I2CWriteByte(PAGE_0_0X72, 0x91, 0xA5);		// Clear bit 6 (reg_skip_rgnd)

	// Changed from 66 to 77 for 94[1:0] = 11 = 5k reg_cbusmhl_pup_sel
	// and bits 5:4 = 11 rgnd_vth_ctl
	//
	sii_I2CWriteByte(PAGE_0_0X72, 0x94, 0x77);			// 1.8V CBUS VTH & GND threshold

	//set bit 2 and 3, which is Initiator Timeout
	WriteByteCBUS(0x31, ReadByteCBUS(0x31) | 0x0c);

	// Establish if connected to 9290 or any other legacy product
	sii_I2CWriteByte(PAGE_0_0X72, 0xA5, 0xA0);	// bits 4:2. rgnd_res_ctl = 3'b000.

	TX_DEBUG_PRINT(("[MHL]: MHL 1.0 Compliant Clock\n"));

	// RGND & single discovery attempt (RGND blocking) , Force USB ID switch to open
	sii_I2CWriteByte(PAGE_0_0X72, 0x95, 0x71);

	// Use only 1K for MHL impedance. Set BIT5 for No-open-drain.
	// Default is good.
	//
	// Use 1k and 2k commented.
//	sii_I2CWriteByte(PAGE_0_0X72, 0x96, 0x22);

	// Use VBUS path of discovery state machine
	sii_I2CWriteByte(PAGE_0_0X72, 0x97, 0x00);


	//
	// For MHL compliance we need the following settings for register 93 and 94
	// Bug 20686
	//
	// To allow RGND engine to operate correctly.
	//
	// When moving the chip from D2 to D0 (power up, init regs) the values should be
	// 94[1:0] = 11  reg_cbusmhl_pup_sel[1:0] should be set for 5k
	// 93[7:6] = 10  reg_cbusdisc_pup_sel[1:0] should be set for 10k
	// 93[5:4] = 00  reg_cbusidle_pup_sel[1:0] = open (default)
	//
	sii_I2CWriteByte(PAGE_0_0X72, 0x92, 0x86);
	// change from CC to 8C to match 10K
	// 0b11 is 5K, 0b10 is 10K, 0b01 is 20k and 0b00 is off
	sii_I2CWriteByte(PAGE_0_0X72, 0x93, 0x8C);				// Disable CBUS pull-up during RGND measurement

	//Jiangshanbin HPD BIT6 for push pull
 	//ReadModifyWritePage0(0x79, BIT6, 0x00);

       //SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();

	DelayMS(25);
	ReadModifyWritePage0(0x95, BIT6, 0x00);	// Release USB ID switch

	ReadModifyWritePage0(0x78, BIT5, 0x00);  //per bug 21045

	sii_I2CWriteByte(PAGE_0_0X72, 0x90, 0x27);	// Enable CBUS discovery

	// Reset CBus to clear state
	CbusReset();

	InitCBusRegs();

	// Enable Auto soft reset on SCDT = 0
	sii_I2CWriteByte(PAGE_0_0X72, 0x05, 0x04);

	// HW debounce to 64ms (0x14)
	sii_I2CWriteByte(PAGE_0_0X72, 0x7A, 0x14);

	// Disable DDC stall feature
	//sii_I2CWriteByte(PAGE_0_0X72, 0xF5, 0x00);		//xding
	
	UNMASK_INTR_1_INTERRUPTS;
	UNMASK_INTR_2_INTERRUPTS;
	UNMASK_INTR_4_INTERRUPTS;
}
///////////////////////////////////////////////////////////////////////////
// InitCBusRegs
//
///////////////////////////////////////////////////////////////////////////
static void InitCBusRegs( void )
{
	uint8_t	regval;

	TX_DEBUG_PRINT(("[MHL]: InitCBusRegs\n"));
	// Increase DDC translation layer timer
	WriteByteCBUS(0x07, 0x32);          // new default is for MHL mode
	WriteByteCBUS(0x40, 0x03); 			// CBUS Drive Strength
	WriteByteCBUS(0x42, 0x06); 			// CBUS DDC interface ignore segment pointer
	WriteByteCBUS(0x36, 0x0C);

	WriteByteCBUS(0x3D, 0xFD);
	WriteByteCBUS(0x1C, 0x01);
	WriteByteCBUS(0x1D, 0x0F);          // MSC_RETRY_FAIL_LIM

	WriteByteCBUS(0x44, 0x02);

	// Setup our devcap
	WriteByteCBUS(0x80, MHL_DEV_ACTIVE);
	WriteByteCBUS(0x81, MHL_VERSION);
	WriteByteCBUS(0x82, (MHL_DEV_CAT_SOURCE));
	WriteByteCBUS(0x83, (uint8_t)(SILICON_IMAGE_ADOPTER_ID >>   8));
	WriteByteCBUS(0x84, (uint8_t)(SILICON_IMAGE_ADOPTER_ID & 0xFF));
	WriteByteCBUS(0x85, MHL_DEV_VID_LINK_SUPPRGB444);
	WriteByteCBUS(0x86, MHL_DEV_AUD_LINK_2CH);
	WriteByteCBUS(0x87, 0);										// not for source
	WriteByteCBUS(0x88, MHL_LOGICAL_DEVICE_MAP);
	WriteByteCBUS(0x89, 0);										// not for source
	WriteByteCBUS(0x8A, (MHL_FEATURE_RCP_SUPPORT | MHL_FEATURE_RAP_SUPPORT |MHL_FEATURE_SP_SUPPORT));
	WriteByteCBUS(0x8B, (uint8_t)(TRANSCODER_DEVICE_ID>>   8));
	WriteByteCBUS(0x8C, (uint8_t)(TRANSCODER_DEVICE_ID& 0xFF));										// reserved
	WriteByteCBUS(0x8D, MHL_SCRATCHPAD_SIZE);
	WriteByteCBUS(0x8E, MHL_INT_AND_STATUS_SIZE);
	WriteByteCBUS(0x8F, 0);										//reserved

	// Make bits 2,3 (initiator timeout) to 1,1 for register CBUS_LINK_CONTROL_2
	regval = ReadByteCBUS( REG_CBUS_LINK_CONTROL_2 );
	regval = (regval | 0x0C);
	WriteByteCBUS(REG_CBUS_LINK_CONTROL_2, regval);

	// Clear legacy bit on Wolverine TX.
	regval = ReadByteCBUS( REG_MSC_TIMEOUT_LIMIT);
	WriteByteCBUS(REG_MSC_TIMEOUT_LIMIT, (regval & MSC_TIMEOUT_LIMIT_MSB_MASK));

	// Set NMax to 1
	WriteByteCBUS(REG_CBUS_LINK_CONTROL_1, 0x01);
}


///////////////////////////////////////////////////////////////////////////
//
// ForceUsbIdSwitchOpen
//
static void ForceUsbIdSwitchOpen ( void )
{
	DISABLE_DISCOVERY 		// Disable CBUS discovery
	ReadModifyWritePage0(0x95, BIT6, BIT6);	// Force USB ID switch to open

	sii_I2CWriteByte(PAGE_0_0X72, 0x92, 0x86);

	// Force HPD to 0 when not in MHL mode.
	//SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();
}


///////////////////////////////////////////////////////////////////////////
//
// ReleaseUsbIdSwitchOpen
//
static void ReleaseUsbIdSwitchOpen ( void )
{
	DelayMS(50); // per spec

	// Release USB ID switch
	ReadModifyWritePage0(0x95, BIT6, 0x00);

	ENABLE_DISCOVERY;
}


//#define CbusWakeUpPulse_GPIO

/////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   CbusWakeUpPulseGenerator ()
//
// PURPOSE      :   Generate Cbus Wake up pulse sequence using GPIO or I2C method.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   None
//
static void CbusWakeUpPulseGenerator( void )
{	
#ifdef CbusWakeUpPulse_GPIO
	//
	// GPIO method
	//
	TX_DEBUG_PRINT(("[MHL]: CbusWakeUpPulseGenerator: GPIO mode\n"));
	 // put reg_cbus_dbgmode = 1
	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) | 0x08));
	
	// Start the pulse
	 pinMHLTxCbusWakeUpPulse = 1;
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// no adjustmebt for code path in GPIO mode
	
	pinMHLTxCbusWakeUpPulse = 0;
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// no adjustmebt for code path in GPIO mode
	
	pinMHLTxCbusWakeUpPulse = 1;
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// no adjustmebt for code path in GPIO mode
	
	pinMHLTxCbusWakeUpPulse = 0;
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_2);	// no adjustmebt for code path in GPIO mode
	
	pinMHLTxCbusWakeUpPulse = 1;
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// no adjustmebt for code path in GPIO mode
	
	pinMHLTxCbusWakeUpPulse = 0;
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// no adjustmebt for code path in GPIO mode
	
	pinMHLTxCbusWakeUpPulse = 1;
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// no adjustmebt for code path in GPIO mode
	
	pinMHLTxCbusWakeUpPulse = 0;
	DelayMS(T_SRC_WAKE_TO_DISCOVER);
	
	// put reg_cbus_dbgmode = 0
	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) & 0xF7));
	
	sii_I2CWriteByte(PAGE_0_0X72, 0x90, (sii_I2CReadByte(PAGE_0_0X72, 0x90) & 0xFE));
	sii_I2CWriteByte(PAGE_0_0X72, 0x90, (sii_I2CReadByte(PAGE_0_0X72, 0x90) | 0x01));

#else
	//
	// I2C method
	//
	TX_DEBUG_PRINT(("[MHL]: CbusWakeUpPulseGenerator: I2C mode\n"));
	// Start the pulse
	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) | 0xC0));
    	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// adjust for code path

	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) & 0x3F));
    	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// adjust for code path

	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) | 0xC0));
    	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// adjust for code path

	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) & 0x3F));
    	DelayMS(T_SRC_WAKE_PULSE_WIDTH_2);	// adjust for code path

	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) | 0xC0));
    	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// adjust for code path

	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) & 0x3F));
    	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// adjust for code path

	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) | 0xC0));
	DelayMS(T_SRC_WAKE_PULSE_WIDTH_1);	// adjust for code path

	sii_I2CWriteByte(PAGE_0_0X72, 0x96, (sii_I2CReadByte(PAGE_0_0X72, 0x96) & 0x3F));

	DelayMS(T_SRC_WAKE_TO_DISCOVER);
#endif

	//
	// Toggle MHL discovery bit
	// 
//	DISABLE_DISCOVERY;
//	ENABLE_DISCOVERY;

}
///////////////////////////////////////////////////////////////////////////
//
// ApplyDdcAbortSafety
//
static void ApplyDdcAbortSafety( void )
{
	uint8_t	bTemp, bPost;

/*	TX_DEBUG_PRINT(("[MHL]: [%d]: Do we need DDC Abort Safety\n",
								(int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD)));*/

	WriteByteCBUS(0x29, 0xFF);  // clear the ddc abort counter
	bTemp = ReadByteCBUS(0x29);  // get the counter
	DelayMS(3);
	bPost = ReadByteCBUS(0x29);  // get another value of the counter

	TX_DEBUG_PRINT(("[MHL]: bTemp: 0x%X bPost: 0x%X\n",(int)bTemp,(int)bPost));

	if (bPost > (bTemp + 50))
	{
		TX_DEBUG_PRINT(("[MHL]: Applying DDC Abort Safety(SWWA 18958)\n"));
		
		CbusReset();
		
		InitCBusRegs();
		
		ForceUsbIdSwitchOpen();
		ReleaseUsbIdSwitchOpen();
		
		MhlTxDrvProcessDisconnection();
	}
}
///////////////////////////////////////////////////////////////////////////
// ProcessRgnd
//
// H/W has detected impedance change and interrupted.
// We look for appropriate impedance range to call it MHL and enable the
// hardware MHL discovery logic. If not, disable MHL discovery to allow
// USB to work appropriately.
//
// In current chip a firmware driven slow wake up pulses are sent to the
// sink to wake that and setup ourselves for full D0 operation.
///////////////////////////////////////////////////////////////////////////
void	ProcessRgnd( void )
{
	uint8_t		reg99RGNDRange;
	//
	// Impedance detection has completed - process interrupt
	//
	reg99RGNDRange = sii_I2CReadByte(PAGE_0_0X72, 0x99) & 0x03;
	TX_DEBUG_PRINT(("[MHL]: RGND Reg 99 = %02X\n", (int)reg99RGNDRange));

	//
	// Reg 0x99
	// 00, 01 or 11 means USB.
	// 10 means 1K impedance (MHL)
	//
	// If 1K, then only proceed with wake up pulses
	if(0x02 == reg99RGNDRange)
	{
		// Switch to full power mode.	// oscar 20110211 for UTG ID=GND endless loop
		SwitchToD0();
				
		// Select CBUS drive float.
		SET_BIT(PAGE_0_0X72, 0x95, 5);

		//The sequence of events during MHL discovery is as follows:
		//	(i) SiI9244 blocks on RGND interrupt (Page0:0x74[6]).
		//	(ii) System firmware turns off its own VBUS if present.
		//	(iii) System firmware waits for about 200ms (spec: TVBUS_CBUS_STABLE, 100 - 1000ms), then checks for the presence of
		//		VBUS from the Sink.
		//	(iv) If VBUS is present then system firmware proceed to drive wake pulses to the Sink as described in previous
		//		section.
		//	(v) If VBUS is absent the system firmware turns on its own VBUS, wait for an additional 200ms (spec:
		//		TVBUS_OUT_TO_STABLE, 100 - 1000ms), and then proceed to drive wake pulses to the Sink as described in above.

		// AP need to check VBUS power present or absent in here 	// by oscar 20110527
		
#if (VBUS_POWER_CHK == ENABLE)			// Turn on VBUS output.
		AppVbusControl( vbusPowerState = FALSE );
#endif

		TX_DEBUG_PRINT(("[MHL]: Waiting T_SRC_VBUS_CBUS_TO_STABLE (%d ms)\n", (int)T_SRC_VBUS_CBUS_TO_STABLE));
		DelayMS(T_SRC_VBUS_CBUS_TO_STABLE);

		//
		// Send slow wake up pulse using GPIO or I2C
		//
		CbusWakeUpPulseGenerator();

		HalTimerSet( ELAPSED_TIMER1, T_SRC_DISCOVER_TO_MHL_EST );	//xding
	}
	else
	{
		TX_DEBUG_PRINT(("[MHL]: USB impedance. Set for USB Established.\n"));
		
		CLR_BIT(PAGE_0_0X72, 0x95, 5);
	}
}

////////////////////////////////////////////////////////////////////
// SwitchToD0
//
// This function performs s/w as well as h/w state transitions.
//
// Chip comes up in D2. Firmware must first bring it to full operation
// mode in D0.
////////////////////////////////////////////////////////////////////
void	SwitchToD0( void )
{
	TX_DEBUG_PRINT(("[MHL]: [%d]: Switch To Full power mode (D0)\n",
							(int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD)) );

	//
	// WriteInitialRegisterValues switches the chip to full power mode.
	//
	WriteInitialRegisterValues();
	
	// Force Power State to ON
	STROBE_POWER_ON
		
	fwPowerState = POWER_STATE_D0_NO_MHL;
}
////////////////////////////////////////////////////////////////////
// SwitchToD3
//
// This function performs s/w as well as h/w state transitions.
//
static void SwitchToD3( void )
{
	//if(POWER_STATE_D3 != fwPowerState)	//by oscar 20110125 for USB OTG
	{
		TX_DEBUG_PRINT(("[MHL]: Switch To D3\n"));

		ForceUsbIdSwitchOpen();

		//
		// To allow RGND engine to operate correctly.
		// So when moving the chip from D0 MHL connected to D3 the values should be
		// 94[1:0] = 00  reg_cbusmhl_pup_sel[1:0] should be set for open
		// 93[7:6] = 00  reg_cbusdisc_pup_sel[1:0] should be set for open
		// 93[5:4] = 00  reg_cbusidle_pup_sel[1:0] = open (default)
		//
		// Disable CBUS pull-up during RGND measurement
		ReadModifyWritePage0(0x93, BIT7 | BIT6 | BIT5 | BIT4, 0);

		ReadModifyWritePage0(0x94, BIT1 | BIT0, 0);

		// 1.8V CBUS VTH & GND threshold

		ReleaseUsbIdSwitchOpen();

		// Force HPD to 0 when not in MHL mode.
       	//SiiMhlTxDrvAcquireUpstreamHPDControlDriveLow();

		// Change TMDS termination to high impedance on disconnection
		// Bits 1:0 set to 11
		sii_I2CWriteByte(PAGE_2_0X92, 0x01, 0x03);

		//
		// Change state to D3 by clearing bit 0 of 3D (SW_TPI, Page 1) register
		// ReadModifyWriteIndexedRegister(INDEXED_PAGE_1, 0x3D, BIT0, 0x00);
		//
		CLR_BIT(PAGE_1_0X7A, 0x3D, 0);

		fwPowerState = POWER_STATE_D3;
	}

#if (VBUS_POWER_CHK == ENABLE)		// Turn VBUS power off when switch to D3(cable out)
	if( vbusPowerState == FALSE )
	{
		AppVbusControl( vbusPowerState = TRUE );
	}
#endif

	TPImode = FALSE;
}


////////////////////////////////////////////////////////////////////
// Int4Isr
//
//
//	Look for interrupts on INTR4 (Register 0x74)
//		7 = RSVD		(reserved)
//		6 = RGND Rdy	(interested)
//		5 = VBUS Low	(ignore)	
//		4 = CBUS LKOUT	(interested)
//		3 = USB EST		(interested)
//		2 = MHL EST		(interested)
//		1 = RPWR5V Change	(ignore)
//		0 = SCDT Change	(interested during D0)
////////////////////////////////////////////////////////////////////
static void Int4Isr( void )
{
	uint8_t reg74;

	reg74 = sii_I2CReadByte(PAGE_0_0X72, (0x74));	// read status

	// When I2C is inoperational (say in D3) and a previous interrupt brought us here, do nothing.
	if(0xFF == reg74)
	{
		return;
	}

#if 1
	if(reg74)
	{
		TX_DEBUG_PRINT(("[MHL]: >Got INTR_4. [reg74 = %02X]\n", (int)reg74));
	}
#endif

	// process MHL_EST interrupt
	if(reg74 & BIT2) // MHL_EST_INT
	{
		HalTimerSet( ELAPSED_TIMER1, 0 );	//xding
		MhlTxDrvProcessConnection();
	}

	// process USB_EST interrupt
	else if(reg74 & BIT3) // MHL_DISC_FAIL_INT
	{
		MhlTxDrvProcessDisconnection();
//		return;
	}

	if((POWER_STATE_D3 == fwPowerState) && (reg74 & BIT6))
	{
		// process RGND interrupt

		// Switch to full power mode.
//		SwitchToD0();
		
		//
		// If a sink is connected but not powered on, this interrupt can keep coming
		// Determine when to go back to sleep. Say after 1 second of this state.
		//
		// Check RGND register and send wake up pulse to the peer
		//
		ProcessRgnd();
	}

	// CBUS Lockout interrupt?
	if (reg74 & BIT4)
	{
		TX_DEBUG_PRINT(("[MHL]: CBus Lockout\n"));

		ForceUsbIdSwitchOpen();
		ReleaseUsbIdSwitchOpen();
	}
	sii_I2CWriteByte(PAGE_0_0X72, (0x74), reg74);	// clear all interrupts

}
#ifdef APPLY_PLL_RECOVERY
///////////////////////////////////////////////////////////////////////////
// FUNCTION:	ApplyPllRecovery
//
// PURPOSE:		This function helps recover PLL.
//
static void ApplyPllRecovery ( void )
{
	// Disable TMDS
	CLR_BIT(PAGE_0_0X72, 0x80, 4);

	// Enable TMDS
	SET_BIT(PAGE_0_0X72, 0x80, 4);

	// followed by a 10ms settle time
	DelayMS(10);

	// MHL FIFO Reset here 
	SET_BIT(PAGE_0_0X72, 0x05, 4);

	CLR_BIT(PAGE_0_0X72, 0x05, 4);

	TX_DEBUG_PRINT(("[MHL]: Applied PLL Recovery\n"));

#ifdef INFOFRAMES_AFTER_TMDS
	WriteByteTPI(0xCD, 0x00);	    				// Set last uint8_t of Audio InfoFrame
	WriteByteTPI(TPI_END_RIGHT_BAR_MSB, 0x00);	    // Set last uint8_t of AVI InfoFrame
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SiiMhlTxDrvRecovery ()
//
// PURPOSE      :   Check SCDT interrupt and PSTABLE interrupt
//
//
// DESCRIPTION :  If SCDT interrupt happened and current status
// is HIGH, irrespective of the last status (assuming we can miss an interrupt)
// go ahead and apply PLL recovery.
//
// When a PSTABLE interrupt happens, it is an indication of a possible
// FIFO overflow condition. Apply a recovery method.
//
//////////////////////////////////////////////////////////////////////////////
void SiiMhlTxDrvRecovery( void )
{
/*// removed for 92326, only for 9244 use
	//
	// Detect Rising Edge of SCDT
        //
	// Check if SCDT interrupt came
	if((sii_I2CReadByte(PAGE_0_0X72, (0x74)) & BIT0))
       {
       	TX_DEBUG_PRINT(("[MHL]: SCDT Interrupt\n"));
	        //
		// Clear this interrupt and then check SCDT.
		// if the interrupt came irrespective of what SCDT was earlier
		// and if SCDT is still high, apply workaround.
	        //
		// This approach implicitly takes care of one lost interrupt.
		//
		SET_BIT(PAGE_0_0X72, (0x74), 0);


		// Read status, if it went HIGH
		if (((sii_I2CReadByte(PAGE_0_0X72, 0x81)) & BIT1) >> 1)
	        {
			// Toggle TMDS and reset MHL FIFO.
			ApplyPllRecovery();
		}
        }
*/

        //
	// Check PSTABLE interrupt...reset FIFO if so.
        //
	if((sii_I2CReadByte(PAGE_0_0X72, (0x72)) & BIT1))
	{

		TX_DEBUG_PRINT(("[MHL]: PSTABLE Interrupt\n"));

		// Toggle TMDS and reset MHL FIFO.
		ApplyPllRecovery();

		// clear PSTABLE interrupt. Do not clear this before resetting the FIFO.
		SET_BIT(PAGE_0_0X72, (0x72), 1);

	}
}
#endif // APPLY_PLL_RECOVERY
///////////////////////////////////////////////////////////////////////////
//
// MhlTxDrvProcessConnection
//
///////////////////////////////////////////////////////////////////////////
static void MhlTxDrvProcessConnection ( void )
{
	bool_t	mhlConnected = TRUE;

	// double check RGND impedance for USB_ID deglitching.	//by oscar 20110412
	if(0x02 != (sii_I2CReadByte(PAGE_0_0X72, 0x99) & 0x03) )
	{
		TX_DEBUG_PRINT (("[MHL]: MHL_EST interrupt but not MHL impedance\n"));
		SwitchToD0();
		SwitchToD3();
		return;
	}
	
	TX_DEBUG_PRINT (("[MHL]: MHL Cable Connected. CBUS:0x0A = %02X\n", (int) ReadByteCBUS(0x0a)));

	if( POWER_STATE_D0_MHL == fwPowerState )
	{
		return;
	}
	
	//
	// Discovery over-ride: reg_disc_ovride	
	//
	sii_I2CWriteByte(PAGE_0_0X72, 0xA0, 0x10);

	fwPowerState = POWER_STATE_D0_MHL;

	// Setup interrupt masks for all those we are interested.
	// Clear MDI_RSEN interrupt before enable it	// by oscar 20101231
	sii_I2CWriteByte(PAGE_0_0X72, 0x71, BIT5);
	UNMASK_INTR_1_INTERRUPTS;

	//
	// Increase DDC translation layer timer (uint8_t mode)
	// Setting DDC Byte Mode
	//
	WriteByteCBUS(0x07, 0x32);  // CBUS DDC byte handshake mode
	// Doing it this way causes problems with playstation: ReadModifyWriteByteCBUS(0x07, BIT2,0);

	// Enable segment pointer safety
	SET_BIT(PAGE_CBUS_0XC8, 0x44, 1);

	// upstream HPD status should not be allowed to rise until HPD from downstream is detected.

	//by oscar 20110125 for USB OTG
	// Change TMDS termination to 50 ohm termination (default)
	// Bits 1:0 set to 00
	sii_I2CWriteByte(PAGE_2_0X92, 0x01, 0x00);
	
	// TMDS should not be enabled until RSEN is high, and HPD and PATH_EN are received

	// Keep the discovery enabled. Need RGND interrupt
	ENABLE_DISCOVERY;

	// Wait T_SRC_RXSENSE_CHK ms to allow connection/disconnection to be stable (MHL 1.0 specs)
	TX_DEBUG_PRINT (("[MHL]: [%d]: Wait T_SRC_RXSENSE_CHK (%d ms) before checking RSEN\n",
							(int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD),
							(int) T_SRC_RXSENSE_CHK) );

	//
	// Ignore RSEN interrupt for T_SRC_RXSENSE_CHK duration.
	// Get the timer started
	//
	HalTimerSet(TIMER_TO_DO_RSEN_CHK, T_SRC_RXSENSE_CHK);

	// Notify upper layer of cable connection
	SiiMhlTxNotifyConnection(mhlConnected = TRUE);

	// Enable HW TPI mode, check device ID
	if (StartTPI())
	{
	
#ifdef DEV_SUPPORT_HDCP
		mhlTxHdcp.HDCP_Override = FALSE;
		mhlTxHdcp.HDCPAuthenticated = VMD_HDCP_AUTHENTICATED;
		HDCP_Init();
#endif
	}
}
///////////////////////////////////////////////////////////////////////////
//
// MhlTxDrvProcessDisconnection
//
///////////////////////////////////////////////////////////////////////////
static void MhlTxDrvProcessDisconnection ( void )
{
	bool_t	mhlConnected = FALSE;

	TX_DEBUG_PRINT (("[MHL]: [%d]: MhlTxDrvProcessDisconnection\n", (int) (HalTimerElapsed( ELAPSED_TIMER ) * MONITORING_PERIOD)));

	// clear all interrupts
//	sii_I2CWriteByte(PAGE_0_0X72, (0x74), sii_I2CReadByte(PAGE_0_0X72, (0x74)));

	sii_I2CWriteByte(PAGE_0_0X72, 0xA0, 0xD0);

	//
	// Reset CBus to clear register contents
	// This may need some key reinitializations
	//
//	CbusReset();

	// Disable TMDS
	SiiMhlTxDrvTmdsControl( FALSE );

	if( POWER_STATE_D0_MHL == fwPowerState )
	{
		// Notify upper layer of cable connection
		SiiMhlTxNotifyConnection(mhlConnected = FALSE);
	}

	// Now put chip in sleep mode
	SwitchToD3();
}
///////////////////////////////////////////////////////////////////////////
//
// CbusReset
//
///////////////////////////////////////////////////////////////////////////

void CbusReset()
{
	uint8_t	idx;
	SET_BIT(PAGE_0_0X72, 0x05, 3);
	DelayMS(2);
	CLR_BIT(PAGE_0_0X72, 0x05, 3);

	mscCmdInProgress = FALSE;

	// Adjust interrupt mask everytime reset is performed.
	UNMASK_CBUS1_INTERRUPTS;
	UNMASK_CBUS2_INTERRUPTS;
	
	for(idx=0; idx < 4; idx++)
	{
		// Enable WRITE_STAT interrupt for writes to all 4 MSC Status registers.
		WriteByteCBUS(0xE0 + idx, 0xFF);

		// Enable SET_INT interrupt for writes to all 4 MSC Interrupt registers.
		WriteByteCBUS(0xF0 + idx, 0xFF);
	}
}


///////////////////////////////////////////////////////////////////////////
//
// CBusProcessErrors
//
//
///////////////////////////////////////////////////////////////////////////
static uint8_t CBusProcessErrors( uint8_t intStatus )
{
    uint8_t result          = 0;
    uint8_t mscAbortReason  = 0;
    uint8_t ddcAbortReason  = 0;

    /* At this point, we only need to look at the abort interrupts. */

    intStatus &=  (BIT_MSC_ABORT | BIT_MSC_XFR_ABORT);

    if ( intStatus )
    {
//      result = ERROR_CBUS_ABORT;		// No Retry will help

		/* If transfer abort or MSC abort, clear the abort reason register. */
		if( intStatus & BIT_DDC_ABORT )
		{
			result = ddcAbortReason = ReadByteCBUS( REG_DDC_ABORT_REASON );
			TX_DEBUG_PRINT( ("[MHL]: CBUS:: DDC ABORT happened, reason:: %02X\n", (int)(ddcAbortReason)));
		}
		
		if ( intStatus & BIT_MSC_XFR_ABORT )
		{
		     	result = mscAbortReason = ReadByteCBUS( REG_PRI_XFR_ABORT_REASON );
		
		    	TX_DEBUG_PRINT( ("[MHL]: CBUS:: MSC Transfer ABORTED. Clearing 0x0D\n"));
		    	WriteByteCBUS( REG_PRI_XFR_ABORT_REASON, 0xFF );
		}
		if ( intStatus & BIT_MSC_ABORT )
		{
		    TX_DEBUG_PRINT( ("[MHL]: CBUS:: MSC Peer sent an ABORT. Clearing 0x0E\n"));
		    	WriteByteCBUS( REG_CBUS_PRI_FWR_ABORT_REASON, 0xFF );
		}

        // Now display the abort reason.

        if ( mscAbortReason != 0 )
        {
            TX_DEBUG_PRINT( ("[MHL]: CBUS:: Reason for ABORT is ....0x%02X\n", (int)mscAbortReason ));

            if ( mscAbortReason & CBUSABORT_BIT_REQ_MAXFAIL)
            {
                TX_DEBUG_PRINT( ("[MHL]: CBUS:: Requestor MAXFAIL - retry threshold exceeded\n"));
            }
            if ( mscAbortReason & CBUSABORT_BIT_PROTOCOL_ERROR)
            {
                TX_DEBUG_PRINT( ("[MHL]: CBUS:: Protocol Error\n"));
            }
            if ( mscAbortReason & CBUSABORT_BIT_REQ_TIMEOUT)
            {
                TX_DEBUG_PRINT( ("[MHL]: CBUS:: Requestor translation layer timeout\n"));
            }
            if ( mscAbortReason & CBUSABORT_BIT_PEER_ABORTED)
            {
                TX_DEBUG_PRINT( ("[MHL]: CBUS:: Peer sent an abort\n"));
            }
            if ( mscAbortReason & CBUSABORT_BIT_UNDEFINED_OPCODE)
            {
                TX_DEBUG_PRINT( ("[MHL]: CBUS:: Undefined opcode\n"));
            }
        }
    }
    return( result );
}

void SiiMhlTxDrvGetScratchPad(uint8_t startReg,uint8_t *pData,uint8_t length)
{
    int i;
	uint8_t regOffset;

    for (regOffset= 0xC0 + startReg,i = 0; i < length;++i,++regOffset)
    {
        *pData++ = ReadByteCBUS( regOffset );
    }
}
///////////////////////////////////////////////////////////////////////////
//
// MhlCbusIsr
//
// Only when MHL connection has been established. This is where we have the
// first looks on the CBUS incoming commands or returned data bytes for the
// previous outgoing command.
//
// It simply stores the event and allows application to pick up the event
// and respond at leisure.
//
// Look for interrupts on CBUS:CBUS_INTR_STATUS [0xC8:0x08]
//		7 = RSVD			(reserved)
//		6 = MSC_RESP_ABORT	(interested)
//		5 = MSC_REQ_ABORT	(interested)	
//		4 = MSC_REQ_DONE	(interested)
//		3 = MSC_MSG_RCVD	(interested)
//		2 = DDC_ABORT		(interested)
//		1 = RSVD			(reserved)
//		0 = rsvd			(reserved)
///////////////////////////////////////////////////////////////////////////
static void MhlCbusIsr( void )
{
	uint8_t	cbusInt;
	uint8_t gotData[4];	// Max four status and int registers.
	uint8_t	i;
	uint8_t	reg71 = sii_I2CReadByte(PAGE_0_0X72, 0x71);

	//
	// Main CBUS interrupts on CBUS_INTR_STATUS
	//
	cbusInt = ReadByteCBUS(0x08);

	// When I2C is inoperational (say in D3) and a previous interrupt brought us here, do nothing.
	if(cbusInt == 0xFF)
	{
		return;
	}
	
	cbusInt &= (~(BIT1|BIT0));	 //don't check Reserved bits // by oscar added 20101207
	if( cbusInt )
	{
		//
		// Clear all interrupts that were raised even if we did not process
		//
		WriteByteCBUS(0x08, cbusInt);
		
		TX_DEBUG_PRINT(("[MHL]: Clear CBUS INTR_1: %02X\n", (int) cbusInt));
	}
	
	// Look for DDC_ABORT
	if (cbusInt & BIT2)
	{
		ApplyDdcAbortSafety();
	}
	// MSC_MSG (RCP/RAP)
	if((cbusInt & BIT3))
	{
		uint8_t mscMsg[2];
		TX_DEBUG_PRINT(("[MHL]: MSC_MSG Received\n"));
		//
		// Two bytes arrive at registers 0x18 and 0x19
		//
		mscMsg[0] = ReadByteCBUS( 0x18 );
		mscMsg[1] = ReadByteCBUS( 0x19 );

		TX_DEBUG_PRINT(("[MHL]: MSC MSG: %02X %02X\n", (int)mscMsg[0], (int)mscMsg[1] ));
		SiiMhlTxGotMhlMscMsg( mscMsg[0], mscMsg[1] );
	}
	if((cbusInt & BIT5) || (cbusInt & BIT6))	// MSC_REQ_ABORT or MSC_RESP_ABORT
	{
		gotData[0] = CBusProcessErrors(cbusInt);
		// Ignore CBUS error and release CBUS status for new command. // by oscar 20101215
		mscCmdInProgress = FALSE;
	}
	// MSC_REQ_DONE received.
	if(cbusInt & BIT4)
	{
		TX_DEBUG_PRINT(("[MHL]: MSC_REQ_DONE\n"));

		mscCmdInProgress = FALSE;
        	// only do this after cBusInt interrupts are cleared above
		SiiMhlTxMscCommandDone( ReadByteCBUS( 0x16 ) );
	}

	if (BIT7 & cbusInt)
	{
		#define CBUS_LINK_STATUS_2 0x38
    	TX_DEBUG_PRINT(("[MHL]: Clearing CBUS_link_hard_err_count\n"));
    	// reset the CBUS_link_hard_err_count field
    	WriteByteCBUS(CBUS_LINK_STATUS_2,(uint8_t)(ReadByteCBUS(CBUS_LINK_STATUS_2) & 0xF0));  
	}
	//
	// Now look for interrupts on register 0x1E. CBUS_MSC_INT2
	// 7:4 = Reserved
	//   3 = msc_mr_write_state = We got a WRITE_STAT
	//   2 = msc_mr_set_int. We got a SET_INT
	//   1 = reserved
	//   0 = msc_mr_write_burst. We received WRITE_BURST
	//
	cbusInt = ReadByteCBUS(0x1E);
	if( cbusInt )
	{
		//
		// Clear all interrupts that were raised even if we did not process
		//
		WriteByteCBUS(0x1E, cbusInt);

		TX_DEBUG_PRINT(("[MHL]: Clear CBUS INTR_2: %02X\n",(int) cbusInt));
	}
	if ( BIT0 & cbusInt)
	{
		// WRITE_BURST complete
		SiiMhlTxMscWriteBurstDone( cbusInt );
	}
	if (cbusInt & BIT2)
	{
		uint8_t intr[4];
		uint8_t address;
		
		TX_DEBUG_PRINT(("[MHL]: MHL INTR Received\n"));
   		for(i = 0,address=0xA0; i < 4; ++i,++address)
		{
			// Clear all, recording as we go
			intr[i] = ReadByteCBUS( address );
			WriteByteCBUS( address, intr[i]);
		}
		// We are interested only in first two bytes.
		SiiMhlTxGotMhlIntr( intr[0], intr[1] );

	}
	if ((cbusInt & BIT3)||HalTimerExpired(TIMER_SWWA_WRITE_STAT))
	{
		uint8_t status[4];
		uint8_t address;
		
		for (i = 0,address=0xB0; i < 4;++i,++address)
		{
			// Clear all, recording as we go
			status[i] = ReadByteCBUS( address );
			WriteByteCBUS( address , 0xFF /* future status[i]*/ );
		}
		SiiMhlTxGotMhlStatus( status[0], status[1] );
		HalTimerSet(TIMER_SWWA_WRITE_STAT, T_SWWA_WRITE_STAT);
	}
	if(reg71)
	{
		//TX_DEBUG_PRINT(("[MHL]: INTR_1 @72:71 = %02X\n", (int) reg71));
		// Clear MDI_HPD interrupt
		sii_I2CWriteByte(PAGE_0_0X72, 0x71, reg71);  /*INTR_1_DESIRED_MASK*/
	}
	//
	// Check if a SET_HPD came from the downstream device.
	//
	cbusInt = ReadByteCBUS(0x0D);

	// CBUS_HPD status bit
	if( BIT6 & (dsHpdStatus ^ cbusInt))
	{
		// Remember
       	dsHpdStatus = cbusInt;
				
		TX_DEBUG_PRINT(("[MHL]: Downstream HPD changed to: %02X\n", (int) cbusInt));
		// Inform upper layer of change in Downstream HPD
       	SiiMhlTxNotifyDsHpdChange( BIT6 & cbusInt );
	}
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
/*
queue implementation
*/
#define NUM_CBUS_EVENT_QUEUE_EVENTS 5
typedef struct _CBusQueue_t
{
    uint8_t head;   // queue empty condition head == tail
    uint8_t tail;
    cbus_req_t queue[NUM_CBUS_EVENT_QUEUE_EVENTS];
}CBusQueue_t,*PCBusQueue_t;


#define QUEUE_SIZE(x) (sizeof(x.queue)/sizeof(x.queue[0]))
#define MAX_QUEUE_DEPTH(x) (QUEUE_SIZE(x) -1)
#define QUEUE_DEPTH(x) ((x.head <= x.tail)?(x.tail-x.head):(QUEUE_SIZE(x)-x.head+x.tail))
#define QUEUE_FULL(x) (QUEUE_DEPTH(x) >= MAX_QUEUE_DEPTH(x))

#define ADVANCE_QUEUE_HEAD(x) { x.head = (x.head < MAX_QUEUE_DEPTH(x))?(x.head+1):0; }
#define ADVANCE_QUEUE_TAIL(x) { x.tail = (x.tail < MAX_QUEUE_DEPTH(x))?(x.tail+1):0; }

#define RETREAT_QUEUE_HEAD(x) { x.head = (x.head > 0)?(x.head-1):MAX_QUEUE_DEPTH(x); }


// Because the Linux driver can be opened multiple times it can't
// depend on one time structure initialization done by the compiler.
//CBusQueue_t CBusQueue={0,0,{0}};
CBusQueue_t CBusQueue;

cbus_req_t *GetNextCBusTransactionImpl(void)
{
    if (0==QUEUE_DEPTH(CBusQueue))
    {
        return NULL;
    }
    else
    {
    	cbus_req_t *retVal;
       retVal = &CBusQueue.queue[CBusQueue.head];
       ADVANCE_QUEUE_HEAD(CBusQueue)
       return retVal;
    }
}
cbus_req_t *GetNextCBusTransactionWrapper(char *pszFunction,int iLine)
{
    TX_DEBUG_PRINT(("[MHL]:%d %s\n",iLine,pszFunction));
    return  GetNextCBusTransactionImpl();
}
#define GetNextCBusTransaction(func) GetNextCBusTransactionWrapper(#func,__LINE__)

bool_t PutNextCBusTransactionImpl(cbus_req_t *pReq)
{
    if (QUEUE_FULL(CBusQueue))
    {
        //queue is full
        return FALSE;
    }
    // at least one slot available
    CBusQueue.queue[CBusQueue.tail] = *pReq;
    ADVANCE_QUEUE_TAIL(CBusQueue)
    return TRUE;
}
// use this wrapper to do debugging output for the routine above.
bool_t PutNextCBusTransactionWrapper(cbus_req_t *pReq,int iLine)
{
    bool_t retVal;

    TX_DEBUG_PRINT(("[MHL]:%d PutNextCBusTransaction %02X %02X %02X depth:%d head: %d tail:%d\n"
                ,iLine
                ,(int)pReq->command
                ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[0]:pReq->offsetData)
                ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[1]:pReq->payload_u.msgData[0])
                ,(int)QUEUE_DEPTH(CBusQueue)
                ,(int)CBusQueue.head
                ,(int)CBusQueue.tail
                ));
    retVal = PutNextCBusTransactionImpl(pReq);

    if (!retVal)
    {
        TX_DEBUG_PRINT(("[MHL]:%d PutNextCBusTransaction queue full, when adding event %d\n",iLine,(int)pReq->command));
    }
    return retVal;
}
#define PutNextCBusTransaction(req) PutNextCBusTransactionWrapper(req,__LINE__)

bool_t PutPriorityCBusTransactionImpl(cbus_req_t *pReq)
{
    if (QUEUE_FULL(CBusQueue))
    {
        //queue is full
        return FALSE;
    }
    // at least one slot available
    RETREAT_QUEUE_HEAD(CBusQueue)
    CBusQueue.queue[CBusQueue.head] = *pReq;
    return TRUE;
}
bool_t PutPriorityCBusTransactionWrapper(cbus_req_t *pReq,int iLine)
{
    bool_t retVal;
    TX_DEBUG_PRINT(("[MHL]:%d: PutPriorityCBusTransaction %02X %02X %02X depth:%d head: %d tail:%d\n"
                ,iLine
                ,(int)pReq->command
                ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[0]:pReq->offsetData)
                ,(int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[1]:pReq->payload_u.msgData[0])
                ,(int)QUEUE_DEPTH(CBusQueue)
                ,(int)CBusQueue.head
                ,(int)CBusQueue.tail
                ));
    retVal = PutPriorityCBusTransactionImpl(pReq);
    if (!retVal)
    {
        TX_DEBUG_PRINT(("[MHL]:%d: PutPriorityCBusTransaction queue full, when adding event 0x%02X\n",iLine,(int)pReq->command));
    }
    return retVal;
}
#define PutPriorityCBusTransaction(pReq) PutPriorityCBusTransactionWrapper(pReq,__LINE__)

#define IncrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount++; TX_DEBUG_PRINT(("[MHL]:%d %s cbusReferenceCount:%d\n",(int)__LINE__,#func,(int)mhlTxConfig.cbusReferenceCount)); }
#define DecrementCBusReferenceCount(func) {mhlTxConfig.cbusReferenceCount--; TX_DEBUG_PRINT(("[MHL]:%d %s cbusReferenceCount:%d\n",(int)__LINE__,#func,(int)mhlTxConfig.cbusReferenceCount)); }

#define SetMiscFlag(func,x) { mhlTxConfig.miscFlags |=  (x); TX_DEBUG_PRINT(("[MHL]:%d %s set %s\n",(int)__LINE__,#func,#x)); }
#define ClrMiscFlag(func,x) { mhlTxConfig.miscFlags &= ~(x); TX_DEBUG_PRINT(("[MHL]:%d %s clr %s\n",(int)__LINE__,#func,#x)); }

//
// Functions used internally.
//
static bool_t SiiMhlTxSetDCapRdy( void );
static bool_t SiiMhlTxClrDCapRdy( void );
static bool_t SiiMhlTxSetPathEn(void );
static bool_t SiiMhlTxClrPathEn( void );
static bool_t SiiMhlTxRapkSend( void );
static void    MhlTxDriveStates( void );
static void    MhlTxResetStates( void );
static bool_t MhlTxSendMscMsg ( uint8_t command, uint8_t cmdData );
extern uint8_t	rcpSupportTable [];

bool_t MhlTxCBusBusy(void)
{
    return ((QUEUE_DEPTH(CBusQueue) > 0)||SiiMhlTxDrvCBusBusy() || mhlTxConfig.cbusReferenceCount)?TRUE:FALSE;
}
///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxTmdsEnable
//
// Implements conditions on enabling TMDS output stated in MHL spec section 7.6.1
//
//
static void SiiMhlTxTmdsEnable(void)
{
	TX_DEBUG_PRINT( ("[MHL]: SiiMhlTxTmdsEnable\n"));
	if (MHL_RSEN & mhlTxConfig.mhlHpdRSENflags)
	{
		TX_DEBUG_PRINT( ("\tMHL_RSEN\n"));
		if (MHL_HPD & mhlTxConfig.mhlHpdRSENflags)
		{
			TX_DEBUG_PRINT( ("\t\tMHL_HPD\n"));
	    		if (MHL_STATUS_PATH_ENABLED & mhlTxConfig.status_1)
	    		{
	    			TX_DEBUG_PRINT(("\t\t\tMHL_STATUS_PATH_ENABLED\n"));
	        		SiiMhlTxDrvTmdsControl( TRUE );
	    		}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetInt
//
// Set MHL defined INTERRUPT bits in peer's register set.
//
// This function returns TRUE if operation was successfully performed.
//
//  regToWrite      Remote interrupt register to write
//
//  mask            the bits to write to that register
//
//  priority        0:  add to head of CBusQueue
//                  1:  add to tail of CBusQueue
//
static bool_t SiiMhlTxSetInt( uint8_t regToWrite,uint8_t  mask, uint8_t priorityLevel )
{
	cbus_req_t	req;
	bool_t retVal;

	// find the offset and bit position
	// and feed
    	req.retryCount  = 2;
	req.command     = MHL_SET_INT;
	req.offsetData  = regToWrite;
	req.payload_u.msgData[0]  = mask;
	if (0 == priorityLevel)
	{
		retVal = PutPriorityCBusTransaction(&req);
	}
	else
	{
		retVal = PutNextCBusTransaction(&req);
	}
	return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDoWriteBurst
//
static bool_t SiiMhlTxDoWriteBurst( uint8_t startReg, uint8_t *pData,uint8_t length )
{
    if (FLAGS_WRITE_BURST_PENDING & mhlTxConfig.miscFlags)
    {
		cbus_req_t	req;
		bool_t retVal;
		
		TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxDoWriteBurst startReg:%d length:%d\n",(int)__LINE__,(int)startReg,(int)length) );
		
		req.retryCount  = 1;
		req.command     = MHL_WRITE_BURST;
		req.length      = length;
		req.offsetData  = startReg;
		req.payload_u.pdatabytes  = pData;
		
		retVal = PutPriorityCBusTransaction(&req);
		ClrMiscFlag(MhlTxDriveStates, FLAGS_WRITE_BURST_PENDING)
		return retVal;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////
// SiiMhlTxRequestWriteBurst
//
bool_t SiiMhlTxRequestWriteBurst(void)
{
    bool_t retVal = FALSE;

    if ((FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)||MhlTxCBusBusy())
    {
        TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxRequestWriteBurst failed FLAGS_SCRATCHPAD_BUSY \n",(int)__LINE__) );
    }
    else
    {
    	 TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxRequestWriteBurst, request sent\n",(int)__LINE__) );
        retVal =  SiiMhlTxSetInt(MHL_RCHANGE_INT,MHL_INT_REQ_WRT, 1);
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
// SiiMhlTxInitialize
//
// Sets the transmitter component firmware up for operation, brings up chip
// into power on state first and then back to reduced-power mode D3 to conserve
// power until an MHL cable connection has been established. If the MHL port is
// used for USB operation, the chip and firmware continue to stay in D3 mode.
// Only a small circuit in the chip observes the impedance variations to see if
// processor should be interrupted to continue MHL discovery process or not.
//
// interruptDriven		If TRUE, MhlTx component will not look at its status
//						registers in a polled manner from timer handler 
//						(SiiMhlTxGetEvents). It will expect that all device 
//						events will result in call to the function 
//						SiiMhlTxDeviceIsr() by host's hardware or software 
//						(a master interrupt handler in host software can call
//						it directly). interruptDriven == TRUE also implies that
//						the MhlTx component shall make use of AppDisableInterrupts()
//						and AppRestoreInterrupts() for any critical section work to
//						prevent concurrency issues.
//
//						When interruptDriven == FALSE, MhlTx component will do
//						all chip status analysis via looking at its register
//						when called periodically into the function
//						SiiMhlTxGetEvents() described below.
//
// pollIntervalMs		This number should be higher than 0 and lower than 
//						51 milliseconds for effective operation of the firmware.
//						A higher number will only imply a slower response to an 
//						event on MHL side which can lead to violation of a 
//						connection disconnection related timing or a slower 
//						response to RCP messages.
//
//
//
//
void SiiMhlTxInitialize( bool_t interruptDriven, uint8_t pollIntervalMs )
{
	// Initialize queue of pending CBUS requests.
	CBusQueue.head = 0;
	CBusQueue.tail = 0;

	//
	// Remember mode of operation.
	//
	mhlTxConfig.interruptDriven = interruptDriven;
	mhlTxConfig.pollIntervalMs  = pollIntervalMs;

	MhlTxResetStates();
	SiiMhlTxChipInitialize();
}


///////////////////////////////////////////////////////////////////////////////
// 
// rcpSupportTable
//
#define	MHL_MAX_RCP_KEY_CODE	(0x7F + 1)	// inclusive

uint8_t		rcpSupportTable [MHL_MAX_RCP_KEY_CODE] = {
	(MHL_DEV_LD_GUI),		// 0x00 = Select
	(MHL_DEV_LD_GUI),		// 0x01 = Up
	(MHL_DEV_LD_GUI),		// 0x02 = Down
	(MHL_DEV_LD_GUI),		// 0x03 = Left
	(MHL_DEV_LD_GUI),		// 0x04 = Right
	0, 0, 0, 0,				// 05-08 Reserved
	(MHL_DEV_LD_GUI),		// 0x09 = Root Menu
	0, 0, 0,				// 0A-0C Reserved
	(MHL_DEV_LD_GUI),		// 0x0D = Select
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// 0E-1F Reserved
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Numeric keys 0x20-0x29
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),
	0,						// 0x2A = Dot
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Enter key = 0x2B
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_TUNER),	// Clear key = 0x2C
	0, 0, 0,				// 2D-2F Reserved
	(MHL_DEV_LD_TUNER),		// 0x30 = Channel Up
	(MHL_DEV_LD_TUNER),		// 0x31 = Channel Dn
	(MHL_DEV_LD_TUNER),		// 0x32 = Previous Channel
	(MHL_DEV_LD_AUDIO),		// 0x33 = Sound Select
	0,						// 0x34 = Input Select
	0,						// 0x35 = Show Information
	0,						// 0x36 = Help
	0,						// 0x37 = Page Up
	0,						// 0x38 = Page Down
	0, 0, 0, 0, 0, 0, 0,	// 0x39-0x3F Reserved
	0,						// 0x40 = Undefined

	(MHL_DEV_LD_SPEAKER),	// 0x41 = Volume Up
	(MHL_DEV_LD_SPEAKER),	// 0x42 = Volume Down
	(MHL_DEV_LD_SPEAKER),	// 0x43 = Mute
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x44 = Play
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x45 = Stop
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x46 = Pause
	(MHL_DEV_LD_RECORD),	// 0x47 = Record
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x48 = Rewind
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x49 = Fast Forward
	(MHL_DEV_LD_MEDIA),		// 0x4A = Eject
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	// 0x4B = Forward
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_MEDIA),	// 0x4C = Backward
	0, 0, 0,				// 4D-4F Reserved
	0,						// 0x50 = Angle
	0,						// 0x51 = Subpicture
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 52-5F Reserved
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x60 = Play Function
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO),	// 0x61 = Pause the Play Function
	(MHL_DEV_LD_RECORD),	// 0x62 = Record Function
	(MHL_DEV_LD_RECORD),	// 0x63 = Pause the Record Function
	(MHL_DEV_LD_VIDEO | MHL_DEV_LD_AUDIO | MHL_DEV_LD_RECORD),	// 0x64 = Stop Function

	(MHL_DEV_LD_SPEAKER),	// 0x65 = Mute Function
	(MHL_DEV_LD_SPEAKER),	// 0x66 = Restore Mute Function
	0, 0, 0, 0, 0, 0, 0, 0, 0, 	                        // 0x67-0x6F Undefined or reserved
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 		// 0x70-0x7F Undefined or reserved
};

///////////////////////////////////////////////////////////////////////////////
// 
// SiiMhlTxGetEvents
//
// This is a function in MhlTx that must be called by application in a periodic
// fashion. The accuracy of frequency (adherence to the parameter pollIntervalMs)
// will determine adherence to some timings in the MHL specifications, however,
// MhlTx component keeps a tolerance of up to 50 milliseconds for most of the
// timings and deploys interrupt disabled mode of operation (applicable only to
// Sii 92326) for creating precise pulse of smaller duration such as 20 ms.
//
// This function does not return anything but it does modify the contents of the
// two pointers passed as parameter.
//
// It is advantageous for application to call this function in task context so
// that interrupt nesting or concurrency issues do not arise. In addition, by
// collecting the events in the same periodic polling mechanism prevents a call
// back from the MhlTx which can result in sending yet another MHL message.
//
// An example of this is responding back to an RCP message by another message
// such as RCPK or RCPE.
//
//
// *event		MhlTx returns a value in this field when function completes execution.
// 				If this field is 0, the next parameter is undefined.
//				The following values may be returned.
//
//
uint16_t MhlTxISRCounter = 0;

void SiiMhlTxGetEvents( uint8_t *event, uint8_t *eventParameter )
{
#if 1 // oscar 20101223
	//
	// If interrupts have not been routed to our ISR, manually call it here.
	//
	//if(FALSE == mhlTxConfig.interruptDriven)
	if( MhlTxISRCounter )
	{	
		MhlTxISRCounter --;
		SiiMhlTxDeviceIsr();
		DelayMS(10);	// zhy 20111101
	}
#endif

	MhlTxDriveStates( );

	*event = MHL_TX_EVENT_NONE;
	*eventParameter = 0;

	if( mhlTxConfig.mhlConnectionEvent )
	{
		TX_DEBUG_PRINT(("[MHL]: SiiMhlTxGetEvents mhlConnectionEvent\n"));

		// Consume the message
		mhlTxConfig.mhlConnectionEvent = FALSE;

		//
		// Let app know the connection went away.
		//
		*event = mhlTxConfig.mhlConnected;
		*eventParameter = mhlTxConfig.mscFeatureFlag;

		// If connection has been lost, reset all state flags.
		if(MHL_TX_EVENT_DISCONNECTION == mhlTxConfig.mhlConnected)
		{
			MhlTxResetStates( );
		}
		else if (MHL_TX_EVENT_CONNECTION == mhlTxConfig.mhlConnected)
		{
			SiiMhlTxSetDCapRdy();
		}
	}
	else if( mhlTxConfig.mscMsgArrived )
	{
		TX_DEBUG_PRINT( ("[MHL]: SiiMhlTxGetEvents MSC MSG <%02X, %02X>\n",
							(int) ( mhlTxConfig.mscMsgSubCommand ),
							(int) ( mhlTxConfig.mscMsgData )) );

		// Consume the message
		mhlTxConfig.mscMsgArrived = FALSE;

		//
		// Map sub-command to an event id
		//
		switch(mhlTxConfig.mscMsgSubCommand)
		{
			case	MHL_MSC_MSG_RAP:
				// RAP is fully handled here.
				//
				// Handle RAP sub-commands here itself
				//
				if( MHL_RAP_CONTENT_ON == mhlTxConfig.mscMsgData)
				{
					SiiMhlTxTmdsEnable();
				}
				else if( MHL_RAP_CONTENT_OFF == mhlTxConfig.mscMsgData)
				{
					SiiMhlTxDrvTmdsControl( FALSE );
				}
				// Always RAPK to the peer
				SiiMhlTxRapkSend( );
				break;

			case	MHL_MSC_MSG_RCP:
				// If we get a RCP key that we do NOT support, send back RCPE
				// Do not notify app layer.
				if(MHL_LOGICAL_DEVICE_MAP & rcpSupportTable [mhlTxConfig.mscMsgData & 0x7F] )
				{
					*event          = MHL_TX_EVENT_RCP_RECEIVED;
					*eventParameter = mhlTxConfig.mscMsgData; // key code
				}
				else
				{
					// Save keycode to send a RCPK after RCPE.
					mhlTxConfig.mscSaveRcpKeyCode = mhlTxConfig.mscMsgData; // key code
					SiiMhlTxRcpeSend( RCPE_INEEFECTIVE_KEY_CODE );
				}
				break;

			case	MHL_MSC_MSG_RCPK:
				*event = MHL_TX_EVENT_RCPK_RECEIVED;
				*eventParameter = mhlTxConfig.mscMsgData; // key code
				DecrementCBusReferenceCount(SiiMhlTxGetEvents)
				mhlTxConfig.mscLastCommand = 0;
				mhlTxConfig.mscMsgLastCommand = 0;
				
				TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxGetEvents RCPK\n",(int)__LINE__) );
				break;

			case	MHL_MSC_MSG_RCPE:
				*event = MHL_TX_EVENT_RCPE_RECEIVED;
				*eventParameter = mhlTxConfig.mscMsgData; // status code
				break;

			case	MHL_MSC_MSG_RAPK:
				// Do nothing if RAPK comes, except decrement the reference counter
				DecrementCBusReferenceCount(SiiMhlTxGetEvents)
				mhlTxConfig.mscLastCommand = 0;
				mhlTxConfig.mscMsgLastCommand = 0;
				TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxGetEvents RAPK\n",(int)__LINE__) );
				break;

			default:
				// Any freak value here would continue with no event to app
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// MhlTxDriveStates
//
// This is an internal function to move the MSC engine to do the next thing
// before allowing the application to run RCP APIs.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
static void MhlTxDriveStates( void )
{

/*	// Polling POW bit at here only for old MHL dongle which used 9292 rev1.0 silicon (not used for the latest rev1.1 silicon)
#if (VBUS_POWER_CHK == ENABLE)
	// Source VBUS power check	// by oscar 20110118
	static uint8_t vbusCheckTime = 0;

	vbusCheckTime ++;
	if( vbusCheckTime == (uint8_t)(T_SRC_VBUS_POWER_CHK / MONITORING_PERIOD))  // 2s period
	{
		vbusCheckTime = 0;
		MHLPowerStatusCheck();
	}
#endif
*/

    // Discover timeout check	//xding
    if ((POWER_STATE_D0_MHL != fwPowerState) && HalTimerElapsed( ELAPSED_TIMER1 ) )
    {
		HalTimerSet( ELAPSED_TIMER1, 0 );
		MhlTxDrvProcessDisconnection();
    }
    
    // process queued CBus transactions
    if (QUEUE_DEPTH(CBusQueue) > 0)
    {
        if (!SiiMhlTxDrvCBusBusy())
        {
        	int reQueueRequest = 0;
        	cbus_req_t *pReq = GetNextCBusTransaction(MhlTxDriveStates);
			// coordinate write burst requests and grants.
			if (MHL_SET_INT == pReq->command)
			{
				if (MHL_RCHANGE_INT == pReq->offsetData)
				{
				    if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
				    {
						if (MHL_INT_REQ_WRT == pReq->payload_u.msgData[0])
						{
							reQueueRequest= 1;
						}
						else if (MHL_INT_GRT_WRT == pReq->payload_u.msgData[0])
						{
							reQueueRequest= 0;
						}
				    }
				    else
					{
						if (MHL_INT_REQ_WRT == pReq->payload_u.msgData[0])
						{
						    IncrementCBusReferenceCount(MhlTxDriveStates)
						    SetMiscFlag(MhlTxDriveStates, FLAGS_SCRATCHPAD_BUSY)
						    SetMiscFlag(MhlTxDriveStates, FLAGS_WRITE_BURST_PENDING)
						}
						else if (MHL_INT_GRT_WRT == pReq->payload_u.msgData[0])
						{
						    SetMiscFlag(MhlTxDriveStates, FLAGS_SCRATCHPAD_BUSY)
						}
					}
				}
			}
            if (reQueueRequest)
            {
                // send this one to the back of the line for later attempts
                if (pReq->retryCount-- > 0)
                {
                    PutNextCBusTransaction(pReq);
                }
            }
            else
            {
                if (MHL_MSC_MSG == pReq->command)
                {
                    mhlTxConfig.mscMsgLastCommand = pReq->payload_u.msgData[0];
                    mhlTxConfig.mscMsgLastData    = pReq->payload_u.msgData[1];
            	  }
                else
                {
                    mhlTxConfig.mscLastOffset  = pReq->offsetData;
                    mhlTxConfig.mscLastData    = pReq->payload_u.msgData[0];
        	  }
                mhlTxConfig.mscLastCommand = pReq->command;

                IncrementCBusReferenceCount(MhlTxDriveStates)
                SiiMhlTxDrvSendCbusCommand( pReq  );
            }
        }
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscCommandDone
//
// This function is called by the driver to inform of completion of last command.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
#define FLAG_OR_NOT(x) (FLAGS_HAVE_##x & mhlTxConfig.miscFlags)?#x:""
#define SENT_OR_NOT(x) (FLAGS_SENT_##x & mhlTxConfig.miscFlags)?#x:""

void	SiiMhlTxMscCommandDone( uint8_t data1 )
{
	TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone. data1 = %02X\n",(int)__LINE__, (int) data1) );

	DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)
	if ( MHL_READ_DEVCAP == mhlTxConfig.mscLastCommand )
	{
		if(MHL_DEV_CATEGORY_OFFSET == mhlTxConfig.mscLastOffset)
		{
			mhlTxConfig.miscFlags |= FLAGS_HAVE_DEV_CATEGORY;
			TX_DEBUG_PRINT(("[MHL]:%d SiiMhlTxMscCommandDone FLAGS_HAVE_DEV_CATEGORY\n",(int)__LINE__));

#if (VBUS_POWER_CHK == ENABLE)
			if( vbusPowerState != (bool_t) ( data1 & MHL_DEV_CATEGORY_POW_BIT) )
			{
				vbusPowerState = (bool_t) ( data1 & MHL_DEV_CATEGORY_POW_BIT);
				AppVbusControl( vbusPowerState );
			}
#endif

            		// OK to call this here, since requests always get queued and processed in the "foreground"
			SiiMhlTxReadDevcap( MHL_DEV_FEATURE_FLAG_OFFSET );
		}
		else if(MHL_DEV_FEATURE_FLAG_OFFSET == mhlTxConfig.mscLastOffset)
		{
			mhlTxConfig.miscFlags |= FLAGS_HAVE_DEV_FEATURE_FLAGS;
			TX_DEBUG_PRINT(("[MHL]:%d SiiMhlTxMscCommandDone FLAGS_HAVE_DEV_FEATURE_FLAGS\n",(int)__LINE__));

			// Remember features of the peer
			mhlTxConfig.mscFeatureFlag	= data1;

			// These variables are used to remember if we issued a READ_DEVCAP
		   		//    or other MSC command
			// Since we are done, reset them.
			mhlTxConfig.mscLastCommand = 0;
			mhlTxConfig.mscLastOffset  = 0;

			TX_DEBUG_PRINT( ("[MHL]:%d Peer's Feature Flag = %02X\n\n",(int)__LINE__, (int) data1) );
		}
	}
	else if(MHL_WRITE_STAT == mhlTxConfig.mscLastCommand)
	{

		TX_DEBUG_PRINT( ("[MHL]: WRITE_STAT miscFlags: %02X\n\n", (int) mhlTxConfig.miscFlags) );
		if (MHL_STATUS_REG_CONNECTED_RDY == mhlTxConfig.mscLastOffset)
		{
		    if (MHL_STATUS_DCAP_RDY & mhlTxConfig.mscLastData)
		    {
				mhlTxConfig.miscFlags |= FLAGS_SENT_DCAP_RDY;
				TX_DEBUG_PRINT(("[MHL]:%d SiiMhlTxMscCommandDone FLAGS_SENT_DCAP_RDY\n",(int)__LINE__));
		    }
		}
		else if (MHL_STATUS_REG_LINK_MODE == mhlTxConfig.mscLastOffset)
		{
		    if ( MHL_STATUS_PATH_ENABLED & mhlTxConfig.mscLastData)
		    {
				mhlTxConfig.miscFlags |= FLAGS_SENT_PATH_EN;
				TX_DEBUG_PRINT(("[MHL]:%d SiiMhlTxMscCommandDone FLAGS_SENT_PATH_EN\n",(int)__LINE__));
		    }
		}

		mhlTxConfig.mscLastCommand = 0;
		mhlTxConfig.mscLastOffset  = 0;
	}
	else if (MHL_MSC_MSG == mhlTxConfig.mscLastCommand)
	{
		if(MHL_MSC_MSG_RCPE == mhlTxConfig.mscMsgLastCommand)
		{
			//
			// RCPE is always followed by an RCPK with original key code that came.
			//
			if( SiiMhlTxRcpkSend( mhlTxConfig.mscSaveRcpKeyCode ) )
			{
			}
		}
		else
		{
			/*TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone default\n"
			    "\tmscLastCommand: 0x%02X \n"
			    "\tmscMsgLastCommand: 0x%02X mscMsgLastData: 0x%02X\n"
			    "\tcbusReferenceCount: %d\n"
			    ,(int)__LINE__
			    ,(int)mhlTxConfig.mscLastCommand
			    ,(int)mhlTxConfig.mscMsgLastCommand
			    ,(int)mhlTxConfig.mscMsgLastData
			    ,(int)mhlTxConfig.cbusReferenceCount
			    ) );*/
		}
		mhlTxConfig.mscLastCommand = 0;
    }
    else if (MHL_WRITE_BURST == mhlTxConfig.mscLastCommand)
    {
        TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone MHL_WRITE_BURST\n",(int)__LINE__ ) );
        mhlTxConfig.mscLastCommand = 0;
        mhlTxConfig.mscLastOffset  = 0;
        mhlTxConfig.mscLastData    = 0;

        // all CBus request are queued, so this is OK to call here
        // use priority 0 so that other queued commands don't interfere
        SiiMhlTxSetInt( MHL_RCHANGE_INT,MHL_INT_DSCR_CHG,0 );
    }
    else if (MHL_SET_INT == mhlTxConfig.mscLastCommand)
    {
        TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone MHL_SET_INT\n",(int)__LINE__ ) );
        if (MHL_RCHANGE_INT == mhlTxConfig.mscLastOffset)
        {
        	TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone MHL_RCHANGE_INT\n",(int)__LINE__) );
            if (MHL_INT_DSCR_CHG == mhlTxConfig.mscLastData)
            {
                DecrementCBusReferenceCount(SiiMhlTxMscCommandDone)  // this one is for the write burst request
                TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone MHL_INT_DSCR_CHG\n",(int)__LINE__) );
                ClrMiscFlag(SiiMhlTxMscCommandDone, FLAGS_SCRATCHPAD_BUSY)
            }
        }
			// Once the command has been sent out successfully, forget this case.
        mhlTxConfig.mscLastCommand = 0;
        mhlTxConfig.mscLastOffset  = 0;
        mhlTxConfig.mscLastData    = 0;
    }
    else
    {
    	/* TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone default\n"
    	            "\tmscLastCommand: 0x%02X mscLastOffset: 0x%02X\n"
                    "\tcbusReferenceCount: %d\n"
    	            ,(int)__LINE__
    	            ,(int)mhlTxConfig.mscLastCommand
    	            ,(int)mhlTxConfig.mscLastOffset
                    ,(int)mhlTxConfig.cbusReferenceCount
    	            ) );*/
   }
    if (!(FLAGS_RCP_READY & mhlTxConfig.miscFlags))
    {
    	TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscCommandDone. have(%s %s) sent(%s %s)\n"
    	                    , (int) __LINE__
                            , FLAG_OR_NOT(DEV_CATEGORY)
                            , FLAG_OR_NOT(DEV_FEATURE_FLAGS)
                            , SENT_OR_NOT(PATH_EN)
                            , SENT_OR_NOT(DCAP_RDY)
    	));
        if (FLAGS_HAVE_DEV_CATEGORY & mhlTxConfig.miscFlags)
        {
            if (FLAGS_HAVE_DEV_FEATURE_FLAGS& mhlTxConfig.miscFlags)
            {
                if (FLAGS_SENT_PATH_EN & mhlTxConfig.miscFlags)
                {
                    if (FLAGS_SENT_DCAP_RDY & mhlTxConfig.miscFlags)
                    {
                        mhlTxConfig.miscFlags |= FLAGS_RCP_READY;
                		// Now we can entertain App commands for RCP
                		// Let app know this state
                		mhlTxConfig.mhlConnectionEvent = TRUE;
                		mhlTxConfig.mhlConnected = MHL_TX_EVENT_RCP_READY;
                    }
                }
            }
		}
    }
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscWriteBurstDone
//
// This function is called by the driver to inform of completion of a write burst.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxMscWriteBurstDone( uint8_t data1 )
{
#define WRITE_BURST_TEST_SIZE 16
	uint8_t temp[WRITE_BURST_TEST_SIZE];
	uint8_t i;
    	TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxMscWriteBurstDone(%02X) \"",(int)__LINE__,(int)data1 ) );
    	SiiMhlTxDrvGetScratchPad(0,temp,WRITE_BURST_TEST_SIZE);
    	for (i = 0; i < WRITE_BURST_TEST_SIZE ; ++i)
    	{
        	if (temp[i]>=' ')
        	{
            		TX_DEBUG_PRINT(("%02X %c ",(int)temp[i],temp[i]));
        	}
        	else
        	{
            		TX_DEBUG_PRINT(("%02X . ",(int)temp[i]));
        	}
    	}
    	TX_DEBUG_PRINT(("\"\n"));
}


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlMscMsg
//
// This function is called by the driver to inform of arrival of a MHL MSC_MSG
// such as RCP, RCPK, RCPE. To quickly return back to interrupt, this function
// remembers the event (to be picked up by app later in task context).
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing of its own,
//
// No printfs.
//
// Application shall not call this function.
//
void	SiiMhlTxGotMhlMscMsg( uint8_t subCommand, uint8_t cmdData )
{
	// Remeber the event.
	mhlTxConfig.mscMsgArrived		= TRUE;
	mhlTxConfig.mscMsgSubCommand	= subCommand;
	mhlTxConfig.mscMsgData			= cmdData;
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlIntr
//
// This function is called by the driver to inform of arrival of a MHL INTERRUPT.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxGotMhlIntr( uint8_t intr_0, uint8_t intr_1 )
{
	TX_DEBUG_PRINT( ("[MHL]: INTERRUPT Arrived. %02X, %02X\n", (int) intr_0, (int) intr_1) );

	//
	// Handle DCAP_CHG INTR here
	//
	if(MHL_INT_DCAP_CHG & intr_0)
	{
        	// OK to call this here, since all requests are queued
		SiiMhlTxReadDevcap( MHL_DEV_CATEGORY_OFFSET );
	}

	if( MHL_INT_DSCR_CHG & intr_0)
	{
		SiiMhlTxDrvGetScratchPad(0,mhlTxConfig.localScratchPad,sizeof(mhlTxConfig.localScratchPad));
		// remote WRITE_BURST is complete
		ClrMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
	}
	if( MHL_INT_REQ_WRT  & intr_0)
	{

    	// this is a request from the sink device.
    	if (FLAGS_SCRATCHPAD_BUSY & mhlTxConfig.miscFlags)
    	{
        		// use priority 1 to defer sending grant until
       	 	//  local traffic is done
        		SiiMhlTxSetInt( MHL_RCHANGE_INT, MHL_INT_GRT_WRT,1);
    	}
	   	else
	   	{
			SetMiscFlag(SiiMhlTxGotMhlIntr, FLAGS_SCRATCHPAD_BUSY)
			// OK to call this here, since all requests are queued
			// use priority 0 to respond immediately
			SiiMhlTxSetInt( MHL_RCHANGE_INT, MHL_INT_GRT_WRT,0);
	   	}
	}
	if( MHL_INT_GRT_WRT  & intr_0)
	{
		uint8_t length =sizeof(mhlTxConfig.localScratchPad);
		TX_DEBUG_PRINT(("[MHL]:%d MHL_INT_GRT_WRT length:%d\n",(int)__LINE__,(int)length));
		SiiMhlTxDoWriteBurst(0x40, mhlTxConfig.localScratchPad, length);
	}

    // removed "else", since interrupts are not mutually exclusive of each other.
	if(MHL_INT_EDID_CHG & intr_1)
	{
		// force upstream source to read the EDID again.
		// Most likely by appropriate togggling of HDMI HPD
		//SiiMhlTxDrvNotifyEdidChange ( );

		if( OnHdmiCableConnected() == FALSE )
		{
			TX_DEBUG_PRINT(("[MHL]: **********EDID read fail**********\n"));
			SiiMhlTxInitialize( FALSE, MONITORING_PERIOD);
			return;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlStatus
//
// This function is called by the driver to inform of arrival of a MHL STATUS.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
void	SiiMhlTxGotMhlStatus( uint8_t status_0, uint8_t status_1 )
{
//	TX_DEBUG_PRINT( ("[MHL]: STATUS Arrived. %02X, %02X\n", (int) status_0, (int) status_1) );
	//
	// Handle DCAP_RDY STATUS here itself
	//
	uint8_t StatusChangeBitMask0,StatusChangeBitMask1;
	StatusChangeBitMask0 = status_0 ^ mhlTxConfig.status_0;
	StatusChangeBitMask1 = status_1 ^ mhlTxConfig.status_1;
	// Remember the event.   (other code checks the saved values, so save the values early, but not before the XOR operations above)
	mhlTxConfig.status_0 = status_0;
	mhlTxConfig.status_1 = status_1;

	if(MHL_STATUS_DCAP_RDY & StatusChangeBitMask0)
	{
		TX_DEBUG_PRINT( ("[MHL]: DCAP_RDY changed\n") );
		if(MHL_STATUS_DCAP_RDY & status_0)
		{
            // OK to call this here since all requests are queued
    		SiiMhlTxReadDevcap( MHL_DEV_CATEGORY_OFFSET );
		}
	}

    	// did PATH_EN change?
	if(MHL_STATUS_PATH_ENABLED & StatusChangeBitMask1)
	{
		TX_DEBUG_PRINT(("[MHL]: PATH_EN changed\n"));
		if(MHL_STATUS_PATH_ENABLED & status_1)
		{
			// OK to call this here since all requests are queued
			SiiMhlTxSetPathEn();
		}
		else
		{
			// OK to call this here since all requests are queued
			SiiMhlTxClrPathEn();
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpSend
//
// This function checks if the peer device supports RCP and sends rcpKeyCode. The
// function will return a value of TRUE if it could successfully send the RCP
// subcommand and the key code. Otherwise FALSE.
//
// The followings are not yet utilized.
// 
// (MHL_FEATURE_RAP_SUPPORT & mhlTxConfig.mscFeatureFlag))
// (MHL_FEATURE_SP_SUPPORT & mhlTxConfig.mscFeatureFlag))
//
//
bool_t SiiMhlTxRcpSend( uint8_t rcpKeyCode )
{
	bool_t retVal;
	//
	// If peer does not support do not send RCP or RCPK/RCPE commands
	//

	if((0 == (MHL_FEATURE_RCP_SUPPORT & mhlTxConfig.mscFeatureFlag))
	    ||
        !(FLAGS_RCP_READY & mhlTxConfig.miscFlags)
		)
	{
		TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxRcpSend failed\n",(int)__LINE__) );
		retVal=FALSE;
	}

	retVal=MhlTxSendMscMsg ( MHL_MSC_MSG_RCP, rcpKeyCode );
	if(retVal)
	{
		TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxRcpSend\n",(int)__LINE__) );
		IncrementCBusReferenceCount(SiiMhlTxRcpSend)
	}
	return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpkSend
//
// This function sends RCPK to the peer device. 
//
bool_t SiiMhlTxRcpkSend( uint8_t rcpKeyCode )
{
	return ( MhlTxSendMscMsg ( MHL_MSC_MSG_RCPK, rcpKeyCode ) );
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRapkSend
//
// This function sends RAPK to the peer device. 
//
static bool_t SiiMhlTxRapkSend( void )
{
	return ( MhlTxSendMscMsg ( MHL_MSC_MSG_RAPK, 0 ) );
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpeSend
//
// The function will return a value of TRUE if it could successfully send the RCPE
// subcommand. Otherwise FALSE.
//
// When successful, MhlTx internally sends RCPK with original (last known)
// keycode.
//
bool_t SiiMhlTxRcpeSend( uint8_t rcpeErrorCode )
{
	return ( MhlTxSendMscMsg ( MHL_MSC_MSG_RCPE, rcpeErrorCode ) );
}

/*
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRapSend
//
// This function checks if the peer device supports RAP and sends rcpKeyCode. The
// function will return a value of TRUE if it could successfully send the RCP
// subcommand and the key code. Otherwise FALSE.
//

bool_t SiiMhlTxRapSend( uint8_t rapActionCode )
{
bool_t retVal;
    if (!(FLAGS_RCP_READY & mhlTxConfig.miscFlags))
    {
    	TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxRapSend failed\n",(int)__LINE__) );
        retVal = FALSE;
    }
    else
    {
    	retVal = MhlTxSendMscMsg ( MHL_MSC_MSG_RAP, rapActionCode );
        if(retVal)
        {
            IncrementCBusReferenceCount
            TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxRapSend\n",(int)__LINE__) );
        }
    }
    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlWriteBurst
//
// This function is called by the driver to inform of arrival of a scratchpad data.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
// Application shall not call this function.
//
void	SiiMhlTxGotMhlWriteBurst( uint8_t *spadArray )
{
}
*/
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetStatus
//
// Set MHL defined STATUS bits in peer's register set.
//
// register	    MHLRegister to write
//
// value        data to write to the register
//
static bool_t SiiMhlTxSetStatus( uint8_t regToWrite, uint8_t value )
{
	cbus_req_t	req;
 	bool_t retVal;

	// find the offset and bit position
	// and feed
	req.retryCount  = 2;
	req.command     = MHL_WRITE_STAT;
	req.offsetData  = regToWrite;
	req.payload_u.msgData[0]  = value;

	TX_DEBUG_PRINT( ("[MHL]:%d SiiMhlTxSetStatus\n",(int)__LINE__) );
	retVal = PutNextCBusTransaction(&req);
	return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetDCapRdy
//
static bool_t SiiMhlTxSetDCapRdy( void )
{
	mhlTxConfig.connectedReady |= MHL_STATUS_DCAP_RDY;   // update local copy
	return SiiMhlTxSetStatus( MHL_STATUS_REG_CONNECTED_RDY, mhlTxConfig.connectedReady);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxClrDCapRdy
//
static bool_t SiiMhlTxClrDCapRdy( void )
{
	mhlTxConfig.connectedReady &= ~MHL_STATUS_DCAP_RDY;  // update local copy
	return SiiMhlTxSetStatus( MHL_STATUS_REG_CONNECTED_RDY, mhlTxConfig.connectedReady);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxSetPathEn
//
static bool_t SiiMhlTxSetPathEn(void )
{
	TX_DEBUG_PRINT( ("[MHL]: SiiMhlTxSetPathEn\n"));
	SiiMhlTxTmdsEnable();
	mhlTxConfig.linkMode |= MHL_STATUS_PATH_ENABLED;     // update local copy
	return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxClrPathEn
//
static bool_t SiiMhlTxClrPathEn( void )
{
	TX_DEBUG_PRINT( ("[MHL]: SiiMhlTxClrPathEn\n"));
	SiiMhlTxDrvTmdsControl( FALSE );
	mhlTxConfig.linkMode &= ~MHL_STATUS_PATH_ENABLED;    // update local copy
	return SiiMhlTxSetStatus( MHL_STATUS_REG_LINK_MODE, mhlTxConfig.linkMode);
}

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxReadDevcap
//
// This function sends a READ DEVCAP MHL command to the peer.
// It  returns TRUE if successful in doing so.
//
// The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
//
// offset		Which byte in devcap register is required to be read. 0..0x0E
//
bool_t SiiMhlTxReadDevcap( uint8_t offset )
{
	cbus_req_t	req;
	TX_DEBUG_PRINT( ("[MHL]: SiiMhlTxReadDevcap\n"));
	//
	// Send MHL_READ_DEVCAP command
	//
	req.retryCount  = 2;
	req.command     = MHL_READ_DEVCAP;
	req.offsetData  = offset;
	req.payload_u.msgData[0]  = 0;  // do this to avoid confusion
	
	return PutNextCBusTransaction(&req);
}

///////////////////////////////////////////////////////////////////////////////
//
// MhlTxSendMscMsg
//
// This function sends a MSC_MSG command to the peer.
// It  returns TRUE if successful in doing so.
//
// The value of devcap should be obtained by making a call to SiiMhlTxGetEvents()
//
// offset		Which byte in devcap register is required to be read. 0..0x0E
//
static bool_t MhlTxSendMscMsg ( uint8_t command, uint8_t cmdData )
{
	cbus_req_t	req;
	uint8_t		ccode;

	//
	// Send MSC_MSG command
	//
	// Remember last MSC_MSG command (RCPE particularly)
	//
	req.retryCount  = 2;
	req.command     = MHL_MSC_MSG;
	req.payload_u.msgData[0]  = command;
	req.payload_u.msgData[1]  = cmdData;
	ccode = PutNextCBusTransaction(&req);
	return( (bool_t) ccode );
}
///////////////////////////////////////////////////////////////////////////////
// 
// SiiMhlTxNotifyConnection
//
//
void	SiiMhlTxNotifyConnection( bool_t mhlConnected )
{
	mhlTxConfig.mhlConnectionEvent = TRUE;

	TX_DEBUG_PRINT(("[MHL]: SiiMhlTxNotifyConnection MSC_STATE_IDLE %01X\n", (int) mhlConnected ));

	if(mhlConnected)
	{
		mhlTxConfig.mhlConnected = MHL_TX_EVENT_CONNECTION;
		mhlTxConfig.mhlHpdRSENflags |= MHL_RSEN;
		SiiMhlTxTmdsEnable();
	}
	else
	{
		mhlTxConfig.mhlConnected = MHL_TX_EVENT_DISCONNECTION;
		mhlTxConfig.mhlHpdRSENflags &= ~MHL_RSEN;
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxNotifyDsHpdChange
// Driver tells about arrival of SET_HPD or CLEAR_HPD by calling this function.
//
// Turn the content off or on based on what we got.
//
void	SiiMhlTxNotifyDsHpdChange( uint8_t dsHpdStatus )
{
	if( 0 == dsHpdStatus )
	{
	    TX_DEBUG_PRINT(("[MHL]: Disable TMDS\n"));
	    TX_DEBUG_PRINT(("[MHL]: DsHPD OFF\n"));
            mhlTxConfig.mhlHpdRSENflags &= ~MHL_HPD;

	    if (mhlTxConfig.hdmiCableConnected == TRUE)
	    {
		  OnHdmiCableDisconnected();
		  return;
	    }
		
	     SiiMhlTxDrvTmdsControl( FALSE );
	}
	else
	{
	    TX_DEBUG_PRINT(("[MHL]: Enable TMDS\n"));
	    TX_DEBUG_PRINT(("[MHL]: DsHPD ON\n"));
		mhlTxConfig.mhlHpdRSENflags |= MHL_HPD;

	    if (mhlTxConfig.hdmiCableConnected == FALSE)
	    {
			if( OnHdmiCableConnected() == FALSE )
			{
				TX_DEBUG_PRINT(("[MHL]: **********EDID read fail**********\n"));
				SiiMhlTxInitialize( FALSE, MONITORING_PERIOD);
				return;
			}
			else
		  		return;
	    }
		
		SiiMhlTxTmdsEnable();
	}
}
///////////////////////////////////////////////////////////////////////////////
//
// MhlTxResetStates
//
// Application picks up mhl connection and rcp events at periodic intervals.
// Interrupt handler feeds these variables. Reset them on disconnection. 
//
static void	MhlTxResetStates( void )
{
	mhlTxConfig.mhlConnectionEvent	= FALSE;
	mhlTxConfig.mhlConnected		= MHL_TX_EVENT_DISCONNECTION;
	mhlTxConfig.mhlHpdRSENflags	   &= ~(MHL_RSEN | MHL_HPD);
	mhlTxConfig.mscMsgArrived		= FALSE;
	
	mhlTxConfig.status_0				= 0;
	mhlTxConfig.status_1				= 0;
	mhlTxConfig.connectedReady    		= 0;
	mhlTxConfig.linkMode           		= 0;
	mhlTxConfig.cbusReferenceCount  	= 0;
	mhlTxConfig.miscFlags				= 0;
	mhlTxConfig.mscLastCommand      	= 0;
	mhlTxConfig.mscMsgLastCommand   	= 0;

	mhlTxConfig.hdmiCableConnected = FALSE;
	mhlTxConfig.dsRxPoweredUp = FALSE;

#ifdef DEV_SUPPORT_EDID
	mhlTxEdid.edidDataValid = FALSE;
#endif
}


#if (VBUS_POWER_CHK == ENABLE)
///////////////////////////////////////////////////////////////////////////////
//
// Function Name: MHLPowerStatusCheck()
//
// Function Description: Check MHL device (dongle or sink) power status.
//
void MHLPowerStatusCheck (void)
{
	static uint8_t DevCatPOWValue = 0;
	uint8_t RegValue;

	if( POWER_STATE_D0_MHL == fwPowerState )
	{
		WriteByteCBUS( REG_CBUS_PRI_ADDR_CMD, MHL_DEV_CATEGORY_OFFSET ); 	// DevCap 0x02
		WriteByteCBUS( REG_CBUS_PRI_START, MSC_START_BIT_READ_REG ); // execute DevCap reg read command

		RegValue = ReadByteCBUS( REG_CBUS_PRI_RD_DATA_1ST );
	
		if( DevCatPOWValue != (RegValue & MHL_DEV_CATEGORY_POW_BIT) )
		{
			DevCatPOWValue = RegValue & MHL_DEV_CATEGORY_POW_BIT;
			TX_DEBUG_PRINT(("[MHL]: DevCapReg0x02=0x%02X, POW bit Changed...\n", (int)RegValue));

			if( vbusPowerState != (bool_t) ( DevCatPOWValue ) )
			{
				vbusPowerState = (bool_t) ( DevCatPOWValue );
				AppVbusControl( vbusPowerState );
			}
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Following function is added for rk29 itv driver. 
//
///////////////////////////////////////////////////////////////////////////////

int sii92326_detect_device(struct sii92326_pdata *sii92326)
{
	HalTimerInit();
	HalTimerSet (TIMER_POLLING, MONITORING_PERIOD);
	return TRUE;
}

int sii92326_sys_init(struct hdmi *hdmi)
{
	SiiMhlTxInitialize(FALSE, MONITORING_PERIOD);

//	siMhlTx_VideoSel( HDMI_720P60 );
//	siMhlTx_AudioSel( AFS_44K1 );
	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_detect_hpd(struct hdmi *hdmi, int *hpdstatus)
{
	uint8_t	event;
	uint8_t	eventParameter;
	
	//
	// Look for any events that might have occurred.
	//
	SiiMhlTxGetEvents( &event, &eventParameter );
	*hpdstatus = mhlTxConfig.hdmiCableConnected;

	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_insert(struct hdmi *hdmi)
{
	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_remove(struct hdmi *hdmi)
{
	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_read_edid(struct hdmi *hdmi, int block, unsigned char *buff)
{
	uint8_t SysCtrlReg;
	uint8_t Offset, Segment;

	// Request access to DDC bus from the receiver
	if (GetDDC_Access(&SysCtrlReg) != TRUE)
	{
		TX_DEBUG_PRINT (("EDID -> DDC bus request failed\n"));
		return HDMI_ERROR_FALSE;
	}
	Offset = 0;
    if ((block % 2) > 0)
    {
        Offset = EDID_BLOCK_SIZE;
    }

    Segment = (uint8_t) (block / 2);

    if (block < 2)
    {
        ReadBlockEDID(Offset, EDID_BLOCK_SIZE, buff);    // read first Segment of EDID ROM
    }
    else
    {
        ReadSegmentBlockEDID(Segment, Offset, EDID_BLOCK_SIZE, buff);     // read next Segment of EDID ROM
    }
	
	if (!ReleaseDDC(SysCtrlReg))				// Host must release DDC bus once it is done reading EDID
	{
		TX_DEBUG_PRINT (("EDID -> DDC bus release failed\n"));
		return HDMI_ERROR_FALSE;
	}
	
	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_config_video(struct hdmi *hdmi, int vic, int input_color, int output_color)
{
	// ÅäÖÃÊÓÆµÊä³ö
	uint8_t vmode;
	#ifdef DEV_SUPPORT_EDID
	mhlTxEdid.HDMI_Sink = hdmi->edid.is_hdmi;
	mhlTxEdid.YCbCr_4_4_4 = hdmi->edid.ycbcr444;
	mhlTxEdid.YCbCr_4_2_2 = hdmi->edid.ycbcr422;
	#else
	HDMI_Sink = hdmi->edid.is_hdmi;
	HDMI_YCbCr_4_4_4 = hdmi->edid.ycbcr444;
	HDMI_YCbCr_4_2_2 = hdmi->edid.ycbcr422;
	#endif
	
	switch(vic)
	{
		case HDMI_720x480p_60Hz_4_3:
			vmode = HDMI_480P60_4X3;
			break;
		case HDMI_720x576p_50Hz_4_3:
			vmode = HDMI_576P50_4X3;
			break;
		case HDMI_1280x720p_60Hz:
			vmode = HDMI_720P60;
			break;
		case HDMI_1280x720p_50Hz:
			vmode = HDMI_720P50;
			break;
		case HDMI_1920x1080p_30Hz:
			vmode = HDMI_1080P30;
			break;
		case HDMI_1920x1080p_25Hz:
			vmode = HDMI_1080P25;
			break;
		case HDMI_1920x1080p_24Hz:
			vmode = HDMI_1080P24;
			break;
		default:
			return HDMI_ERROR_FALSE;
	}
	siMhlTx_VideoSel( vmode );
//	siMhlTx_VideoSet();
	return HDMI_ERROR_SUCESS;
}
int sii92326_sys_config_audio(struct hdmi *hdmi, struct hdmi_audio *audio)
{
	uint8_t MCLK_FS=0;
	
	if(audio == NULL)
    	return HDMI_ERROR_FALSE;
    	
   	mhlTxAv.AudioMode			= AMODE_I2S;
	mhlTxAv.AudioChannels		= audio->channel - 1;
	
    switch (audio->rate)
    {      
        case HDMI_AUDIO_FS_32000:
			mhlTxAv.AudioFs = AFS_32K;
			MCLK_FS = MCLK384FS;
			break;
		case HDMI_AUDIO_FS_44100:
			mhlTxAv.AudioFs = AFS_44K1;
			MCLK_FS = MCLK256FS;
			break;
		case HDMI_AUDIO_FS_48000:
			mhlTxAv.AudioFs = AFS_48K;
			MCLK_FS = MCLK256FS;
			break;
		case HDMI_AUDIO_FS_88200:
			mhlTxAv.AudioFs = AFS_88K2;
			MCLK_FS = MCLK128FS;
			break;
		case HDMI_AUDIO_FS_96000:
			mhlTxAv.AudioFs = AFS_96K;
			MCLK_FS = MCLK128FS;
			break;
		case HDMI_AUDIO_FS_176400:
			mhlTxAv.AudioFs = AFS_176K4;
			MCLK_FS = MCLK128FS;
			break;
		case HDMI_AUDIO_FS_192000:
			mhlTxAv.AudioFs = AFS_192K;
			MCLK_FS = MCLK128FS;
			break;
		default:
			printk("[%s] not support such sample rate", __FUNCTION__);
			return HDMI_ERROR_FALSE;
	}

    switch(audio->word_length)
	{
		case HDMI_AUDIO_WORD_LENGTH_16bit:
			mhlTxAv.AudioWordLength = ALENGTH_16BITS;
			break;
		case HDMI_AUDIO_WORD_LENGTH_20bit:
			mhlTxAv.AudioWordLength = ALENGTH_20BITS;
			break;
		case HDMI_AUDIO_WORD_LENGTH_24bit:
			mhlTxAv.AudioWordLength = ALENGTH_24BITS;
			break;
		default:
			printk("[%s] not support such word length", __FUNCTION__);
			return HDMI_ERROR_FALSE;
	}
	mhlTxAv.AudioI2SFormat		= (MCLK_FS << 4) |SCK_SAMPLE_RISING_EDGE |0x00;
//	siMhlTx_AudioSet();
	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_config_hdcp(struct hdmi *hdmi, int enable)
{
	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_enalbe_output(struct hdmi *hdmi, int enable)
{
	if(enable)
		OnDownstreamRxPoweredUp();
	else
		OnDownstreamRxPoweredDown();
	return HDMI_ERROR_SUCESS;
}

int sii92326_sys_detect_sink(struct hdmi *hdmi, int *sinkstatus)
{
	*sinkstatus = TRUE;
	return HDMI_ERROR_SUCESS;
}