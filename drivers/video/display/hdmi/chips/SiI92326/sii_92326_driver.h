/***********************************************************************************/
/*  Copyright (c) 2002-2011, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#include "sii_92326_api.h"

#ifndef __SII_92326_DRIVER_H__
#define __SII_92326_DRIVER_H__

///////////////////////////////////////////////////////////////////////////////
//
// MHL Timings applicable to this driver.
//
//
#define	T_SRC_VBUS_CBUS_TO_STABLE	(200)	// 100 - 1000 milliseconds. Per MHL 1.0 Specs
#define	T_SRC_WAKE_PULSE_WIDTH_1	(20)	// 20 milliseconds. Per MHL 1.0 Specs
#define	T_SRC_WAKE_PULSE_WIDTH_2	(60)	// 60 milliseconds. Per MHL 1.0 Specs

#define	T_SRC_WAKE_TO_DISCOVER		(500)	// 100 - 1000 milliseconds. Per MHL 1.0 Specs

// Allow RSEN to stay low this much before reacting.
// Per specs between 100 to 200 ms
#define	T_SRC_RSEN_DEGLITCH			(100)	// (150)

// Wait this much after connection before reacting to RSEN (300-500ms)
// Per specs between 300 to 500 ms
#define	T_SRC_RXSENSE_CHK				(400)

// TIMER_SWWA_WRITE_STAT
#define	T_SWWA_WRITE_STAT			(50)

// Discover to MHL EST timeout
#define	T_SRC_DISCOVER_TO_MHL_EST	(500)

//====================================================
// VBUS power check for supply or charge
//====================================================
#define 	VBUS_POWER_CHK				(DISABLE) //(ENABLE)

// VBUS power check timer = 2s
#define	T_SRC_VBUS_POWER_CHK			(2000)

extern uint16_t MhlTxISRCounter;

//====================================================
// Macro for 92326 only!
//====================================================
//#define DEV_SUPPORT_EDID
#ifdef HDMI_HDCP_ENABLE
#define DEV_SUPPORT_HDCP
#endif

#define F_9022A_9334
#define INFOFRAMES_AFTER_TMDS
#define CLOCK_EDGE_RISING

#ifdef DEV_SUPPORT_EDID
#define IsHDMI_Sink()		(mhlTxEdid.HDMI_Sink)
#else
extern	uint8_t				HDMI_Sink;
#define IsHDMI_Sink()		HDMI_Sink//(TRUE)
#endif

// I2C Slave Addresses
//====================================================
#define TX_TPI_ADDR       		0x72
#define HDCP_SLAVE_ADDR     	0x74
#define EDID_ROM_ADDR       	0xA0
#define EDID_SEG_ADDR	    	0x60

// I2C Slave addresses for four indexed pages used by Sii 92326
//====================================================
#define PAGE_0_0X72			0x72
#define PAGE_1_0X7A			0x7A
#define PAGE_2_0X92			0x92
#define PAGE_CBUS_0XC8		0xC8

// Indexed Register Offsets, Constants
//====================================================
#define INDEXED_PAGE_0		0x01	//0x72
#define INDEXED_PAGE_1		0x02	//0x7A
#define INDEXED_PAGE_2		0x03	//0x92


//------------------------------------------------------------------------------
// Driver API typedefs
//------------------------------------------------------------------------------
//
// structure to hold operating information of MhlTx component
//
typedef struct 
{
    	bool_t		interruptDriven;	// Remember what app told us about interrupt availability.
    	uint8_t		pollIntervalMs;		// Remember what app set the polling frequency as.

	uint8_t		status_0;			// Received status from peer is stored here
	uint8_t		status_1;			// Received status from peer is stored here

    	uint8_t     	connectedReady;     // local MHL CONNECTED_RDY register value
    	uint8_t     	linkMode;           // local MHL LINK_MODE register value
    	uint8_t     	mhlHpdStatus;       // keep track of SET_HPD/CLR_HPD
    	uint8_t     	mhlRequestWritePending;

	bool_t		mhlConnectionEvent;
	uint8_t		mhlConnected;

    	uint8_t     	mhlHpdRSENflags;       // keep track of SET_HPD/CLR_HPD

	// mscMsgArrived == TRUE when a MSC MSG arrives, FALSE when it has been picked up
	bool_t		mscMsgArrived;
	uint8_t		mscMsgSubCommand;
	uint8_t		mscMsgData;

	// Remember FEATURE FLAG of the peer to reject app commands if unsupported
	uint8_t		mscFeatureFlag;

    	uint8_t     	cbusReferenceCount;  // keep track of CBUS requests
	// Remember last command, offset that was sent.
	// Mostly for READ_DEVCAP command and other non-MSC_MSG commands
	uint8_t		mscLastCommand;
	uint8_t		mscLastOffset;
	uint8_t		mscLastData;

	// Remember last MSC_MSG command (RCPE particularly)
	uint8_t		mscMsgLastCommand;
	uint8_t		mscMsgLastData;
	uint8_t		mscSaveRcpKeyCode;

    	//  support WRITE_BURST
    	uint8_t     	localScratchPad[16];
    	uint8_t     	miscFlags;          // such as SCRATCHPAD_BUSY
	//  uint8_t 	mscData[ 16 ]; 		// What we got back as message data

// for 92326 only! 
	bool_t 		hdmiCableConnected;
	bool_t		dsRxPoweredUp;
	
} mhlTx_config_t;

// bits for mhlHpdRSENflags:
typedef enum
{
     MHL_HPD            = 0x01
   , MHL_RSEN           = 0x02
}MhlHpdRSEN_e;

typedef enum
{
      FLAGS_SCRATCHPAD_BUSY         = 0x01
    , FLAGS_REQ_WRT_PENDING         = 0x02
    , FLAGS_WRITE_BURST_PENDING     = 0x04
    , FLAGS_RCP_READY               = 0x08
    , FLAGS_HAVE_DEV_CATEGORY       = 0x10
    , FLAGS_HAVE_DEV_FEATURE_FLAGS  = 0x20
    , FLAGS_SENT_DCAP_RDY           = 0x40
    , FLAGS_SENT_PATH_EN            = 0x80
}MiscFlags_e;


//
// structure to hold command details from upper layer to CBUS module
//
typedef struct
{
    uint8_t reqStatus;       // CBUS_IDLE, CBUS_PENDING
    uint8_t retryCount;
    uint8_t command;         // VS_CMD or RCP opcode
    uint8_t offsetData;      // Offset of register on CBUS or RCP data
    uint8_t length;          // Only applicable to write burst. ignored otherwise.
    union
    {
    	uint8_t msgData[ 16 ];   // Pointer to message data area.
	unsigned char	*pdatabytes;			// pointer for write burst or read many bytes
    }payload_u;

} cbus_req_t;


//
// Functions that driver exposes to the upper layer.
//
//////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxChipInitialize
//
// Chip specific initialization.
// This function is for SiI 92326 Initialization: HW Reset, Interrupt enable.
//
//
//////////////////////////////////////////////////////////////////////////////
bool_t 	SiiMhlTxChipInitialize( void );

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
void 	SiiMhlTxDeviceIsr( void );

///////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////
bool_t	SiiMhlTxDrvSendCbusCommand ( cbus_req_t *pReq  );

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvCBusBusy
//
//  returns FALSE when the CBus is ready for the next command
bool_t SiiMhlTxDrvCBusBusy(void);

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDriverTmdsControl
//
// Control the TMDS output. MhlTx uses this to support RAP content on and off.
//
void SiiMhlTxDrvTmdsControl( bool_t enable );

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvNotifyEdidChange
//
// MhlTx may need to inform upstream device of a EDID change. This can be
// achieved by toggling the HDMI HPD signal or by simply calling EDID read
// function.
//
//void	SiiMhlTxDrvNotifyEdidChange ( void );
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
bool_t SiiMhlTxReadDevcap( uint8_t offset );

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxDrvGetScratchPad
//
// This function reads the local scratchpad into a local memory buffer
//
void SiiMhlTxDrvGetScratchPad(uint8_t startReg,uint8_t *pData,uint8_t length);

//
// Functions that driver expects from the upper layer.
//

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxNotifyDsHpdChange
//
// Informs MhlTx component of a Downstream HPD change (when h/w receives
// SET_HPD or CLEAR_HPD).
//
extern	void	SiiMhlTxNotifyDsHpdChange( uint8_t dsHpdStatus );
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxNotifyConnection
//
// This function is called by the driver to inform of connection status change.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
extern	void	SiiMhlTxNotifyConnection( bool_t mhlConnected );

///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscCommandDone
//
// This function is called by the driver to inform of completion of last command.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
extern	void	SiiMhlTxMscCommandDone( uint8_t data1 );
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxMscWriteBurstDone
//
// This function is called by the driver to inform of completion of a write burst.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
extern	void	SiiMhlTxMscWriteBurstDone( uint8_t data1 );
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlIntr
//
// This function is called by the driver to inform of arrival of a MHL INTERRUPT.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
extern	void	SiiMhlTxGotMhlIntr( uint8_t intr_0, uint8_t intr_1 );
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlStatus
//
// This function is called by the driver to inform of arrival of a MHL STATUS.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
extern	void	SiiMhlTxGotMhlStatus( uint8_t status_0, uint8_t status_1 );
///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxGotMhlMscMsg
//
// This function is called by the driver to inform of arrival of a MHL STATUS.
//
// It is called in interrupt context to meet some MHL specified timings, therefore,
// it should not have to call app layer and do negligible processing, no printfs.
//
// Application shall not call this function.
//
extern	void	SiiMhlTxGotMhlMscMsg( uint8_t subCommand, uint8_t cmdData );
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
extern	void	SiiMhlTxGotMhlWriteBurst( uint8_t *spadArray );


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
//
// Parameters
// interruptDriven		Description
//						If TRUE, MhlTx component will not look at its status
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
void 	SiiMhlTxInitialize( bool_t interruptDriven, uint8_t pollIntervalMs );


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
void SiiMhlTxGetEvents( uint8_t *event, uint8_t *eventParameter );


//
// *event		MhlTx returns a value in this field when function completes execution.
// 				If this field is 0, the next parameter is undefined.
//				The following values may be returned.
//
//
#define		MHL_TX_EVENT_NONE			0x00	/* No event worth reporting.  */
#define		MHL_TX_EVENT_DISCONNECTION	0x01	/* MHL connection has been lost */
#define		MHL_TX_EVENT_CONNECTION		0x02	/* MHL connection has been established */
#define		MHL_TX_EVENT_RCP_READY		0x03	/* MHL connection is ready for RCP */
//
#define		MHL_TX_EVENT_RCP_RECEIVED	0x04	/* Received an RCP. Key Code in "eventParameter" */
#define		MHL_TX_EVENT_RCPK_RECEIVED	0x05	/* Received an RCPK message */
#define		MHL_TX_EVENT_RCPE_RECEIVED	0x06	/* Received an RCPE message .*/


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpSend
//
// This function checks if the peer device supports RCP and sends rcpKeyCode. The
// function will return a value of TRUE if it could successfully send the RCP
// subcommand and the key code. Otherwise FALSE.
//
bool_t SiiMhlTxRcpSend( uint8_t rcpKeyCode );


///////////////////////////////////////////////////////////////////////////////
//
// SiiMhlTxRcpkSend
//
// This function checks if the peer device supports RCP and sends RCPK response
// when application desires. 
// The function will return a value of TRUE if it could successfully send the RCPK
// subcommand. Otherwise FALSE.
//
bool_t SiiMhlTxRcpkSend( uint8_t rcpKeyCode );


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
bool_t SiiMhlTxRcpeSend( uint8_t rcpeErrorCode );



///////////////////////////////////////////////////////////////////////////////
//
// MHLPowerStatusCheck
//
// The function check dongle power status then to decide output power or not.
//
void MHLPowerStatusCheck (void);


///////////////////////////////////////////////////////////////////////////////
//
// CBUS register defintions
//
#define REG_CBUS_INTR_STATUS            0x08
#define BIT_DDC_ABORT                   (BIT2)    /* Responder aborted DDC command at translation layer */
#define BIT_MSC_MSG_RCV                 (BIT3)    /* Responder sent a VS_MSG packet (response data or command.) */
#define BIT_MSC_XFR_DONE                (BIT4)    /* Responder sent ACK packet (not VS_MSG) */
#define BIT_MSC_XFR_ABORT               (BIT5)    /* Command send aborted on TX side */
#define BIT_MSC_ABORT                   (BIT6)    /* Responder aborted MSC command at translation layer */

#define REG_CBUS_INTR_ENABLE            0x09

#define REG_DDC_ABORT_REASON        	  0x0C
#define REG_CBUS_BUS_STATUS              0x0A
#define BIT_BUS_CONNECTED                   0x01
#define BIT_LA_VAL_CHG                      	   0x02

#define REG_PRI_XFR_ABORT_REASON        0x0D

#define REG_CBUS_PRI_FWR_ABORT_REASON   		0x0E
#define	CBUSABORT_BIT_REQ_MAXFAIL			(0x01 << 0)
#define	CBUSABORT_BIT_PROTOCOL_ERROR		(0x01 << 1)
#define	CBUSABORT_BIT_REQ_TIMEOUT			(0x01 << 2)
#define	CBUSABORT_BIT_UNDEFINED_OPCODE		(0x01 << 3)
#define	CBUSSTATUS_BIT_CONNECTED			(0x01 << 6)
#define	CBUSABORT_BIT_PEER_ABORTED			(0x01 << 7)

#define REG_CBUS_PRI_START              				0x12
#define BIT_TRANSFER_PVT_CMD          				0x01
#define BIT_SEND_MSC_MSG                    			0x02
#define	MSC_START_BIT_MSC_CMD			 	(0x01 << 0)
#define	MSC_START_BIT_VS_CMD		        	 	(0x01 << 1)
#define	MSC_START_BIT_READ_REG		   	 	(0x01 << 2)
#define	MSC_START_BIT_WRITE_REG		        	(0x01 << 3)
#define	MSC_START_BIT_WRITE_BURST	        	(0x01 << 4)

#define REG_CBUS_PRI_ADDR_CMD           0x13
#define REG_CBUS_PRI_WR_DATA_1ST        0x14
#define REG_CBUS_PRI_WR_DATA_2ND        0x15
#define REG_CBUS_PRI_RD_DATA_1ST        0x16
#define REG_CBUS_PRI_RD_DATA_2ND        0x17


#define REG_CBUS_PRI_VS_CMD             0x18
#define REG_CBUS_PRI_VS_DATA            0x19

#define	REG_MSC_WRITE_BURST_LEN         0x20       // only for WRITE_BURST
#define	MSC_REQUESTOR_DONE_NACK         	(0x01 << 6)      

#define	REG_CBUS_MSC_RETRY_INTERVAL			0x1A		// default is 16
#define	REG_CBUS_DDC_FAIL_LIMIT				0x1C		// default is 5
#define	REG_CBUS_MSC_FAIL_LIMIT				0x1D		// default is 5
#define	REG_CBUS_MSC_INT2_STATUS        	0x1E
#define REG_CBUS_MSC_INT2_ENABLE             	0x1F
	#define	MSC_INT2_REQ_WRITE_MSC              (0x01 << 0)	// Write REG data written.
	#define	MSC_INT2_HEARTBEAT_MAXFAIL          (0x01 << 1)	// Retry threshold exceeded for sending the Heartbeat

#define	REG_MSC_WRITE_BURST_LEN         0x20       // only for WRITE_BURST

#define	REG_MSC_HEARTBEAT_CONTROL       0x21       // Periodic heart beat. TX sends GET_REV_ID MSC command
#define	MSC_HEARTBEAT_PERIOD_MASK		    0x0F	// bits 3..0
#define	MSC_HEARTBEAT_FAIL_LIMIT_MASK	    0x70	// bits 6..4
#define	MSC_HEARTBEAT_ENABLE			    0x80	// bit 7

#define REG_MSC_TIMEOUT_LIMIT           0x22
#define	MSC_TIMEOUT_LIMIT_MSB_MASK	        (0x0F)	        // default is 1
#define	MSC_LEGACY_BIT					    (0x01 << 7)	    // This should be cleared.

#define	REG_CBUS_LINK_CONTROL_1				0x30	// 
#define	REG_CBUS_LINK_CONTROL_2				0x31	// 
#define	REG_CBUS_LINK_CONTROL_3				0x32	// 
#define	REG_CBUS_LINK_CONTROL_4				0x33	// 
#define	REG_CBUS_LINK_CONTROL_5				0x34	// 
#define	REG_CBUS_LINK_CONTROL_6				0x35	// 
#define	REG_CBUS_LINK_CONTROL_7				0x36	// 
#define REG_CBUS_LINK_STATUS_1          			0x37
#define REG_CBUS_LINK_STATUS_2          			0x38
#define	REG_CBUS_LINK_CONTROL_8				0x39	// 
#define	REG_CBUS_LINK_CONTROL_9				0x3A	// 
#define	REG_CBUS_LINK_CONTROL_10				0x3B	// 
#define	REG_CBUS_LINK_CONTROL_11				0x3C	// 
#define	REG_CBUS_LINK_CONTROL_12				0x3D	// 


#define REG_CBUS_LINK_CTRL9_0           			0x3A
#define REG_CBUS_LINK_CTRL9_1           			0xBA

#define	REG_CBUS_DRV_STRENGTH_0				0x40	// 
#define	REG_CBUS_DRV_STRENGTH_1				0x41	// 
#define	REG_CBUS_ACK_CONTROL					0x42	// 
#define	REG_CBUS_CAL_CONTROL					0x43	// Calibration

#define REG_CBUS_SCRATCHPAD_0           			0xC0
#define REG_CBUS_DEVICE_CAP_0           			0x80
#define REG_CBUS_DEVICE_CAP_1           			0x81
#define REG_CBUS_DEVICE_CAP_2           			0x82
#define REG_CBUS_DEVICE_CAP_3           			0x83
#define REG_CBUS_DEVICE_CAP_4           			0x84
#define REG_CBUS_DEVICE_CAP_5           			0x85
#define REG_CBUS_DEVICE_CAP_6           			0x86
#define REG_CBUS_DEVICE_CAP_7           			0x87
#define REG_CBUS_DEVICE_CAP_8           			0x88
#define REG_CBUS_DEVICE_CAP_9           			0x89
#define REG_CBUS_DEVICE_CAP_A           			0x8A
#define REG_CBUS_DEVICE_CAP_B           			0x8B
#define REG_CBUS_DEVICE_CAP_C           			0x8C
#define REG_CBUS_DEVICE_CAP_D           			0x8D
#define REG_CBUS_DEVICE_CAP_E           			0x8E
#define REG_CBUS_DEVICE_CAP_F           			0x8F
#define REG_CBUS_SET_INT_0						0xA0
#define REG_CBUS_SET_INT_1						0xA1
#define REG_CBUS_SET_INT_2						0xA2
#define REG_CBUS_SET_INT_3						0xA3
#define REG_CBUS_WRITE_STAT_0        			0xB0
#define REG_CBUS_WRITE_STAT_1        			0xB1
#define REG_CBUS_WRITE_STAT_2        			0xB2
#define REG_CBUS_WRITE_STAT_3        			0xB3


///////////////////////////////////////////////////////////////////////////////
//
// This file contains MHL Specs related definitions.
//

// Device Power State
#define MHL_DEV_UNPOWERED		0x00
#define MHL_DEV_INACTIVE		0x01
#define MHL_DEV_QUIET			0x03
#define MHL_DEV_ACTIVE			0x04

// Version that this chip supports
#define MHL_VER_MAJOR			(0x01 << 4)	// bits 4..7
#define MHL_VER_MINOR			0x00		// bits 0..3
#define MHL_VERSION				(MHL_VER_MAJOR | MHL_VER_MINOR)

//Device Category
#define	MHL_DEV_CATEGORY_OFFSET				0x02
#define	MHL_DEV_CATEGORY_POW_BIT			(BIT4)

#define	MHL_DEV_CAT_SOURCE					0x00
#define	MHL_DEV_CAT_SINGLE_INPUT_SINK		0x01
#define	MHL_DEV_CAT_MULTIPLE_INPUT_SINK		0x02
#define	MHL_DEV_CAT_UNPOWERED_DONGLE		0x03
#define	MHL_DEV_CAT_SELF_POWERED_DONGLE	0x04
#define	MHL_DEV_CAT_HDCP_REPEATER			0x05
#define	MHL_DEV_CAT_NON_DISPLAY_SINK		0x06
#define	MHL_DEV_CAT_POWER_NEUTRAL_SINK		0x07
#define	MHL_DEV_CAT_OTHER					0x80

//Video Link Mode
#define	MHL_DEV_VID_LINK_SUPPRGB444			0x01
#define	MHL_DEV_VID_LINK_SUPPYCBCR444		0x02
#define	MHL_DEV_VID_LINK_SUPPYCBCR422		0x04
#define	MHL_DEV_VID_LINK_PPIXEL				0x08
#define	MHL_DEV_VID_LINK_SUPP_ISLANDS		0x10

//Audio Link Mode Support
#define	MHL_DEV_AUD_LINK_2CH					0x01
#define	MHL_DEV_AUD_LINK_8CH					0x02


//Feature Flag in the devcap
#define	MHL_DEV_FEATURE_FLAG_OFFSET			0x0A
#define	MHL_FEATURE_RCP_SUPPORT				BIT0	// Dongles have freedom to not support RCP
#define	MHL_FEATURE_RAP_SUPPORT				BIT1	// Dongles have freedom to not support RAP
#define	MHL_FEATURE_SP_SUPPORT				BIT2	// Dongles have freedom to not support SCRATCHPAD

/*
#define		MHL_POWER_SUPPLY_CAPACITY		16		// 160 mA current
#define		MHL_POWER_SUPPLY_PROVIDED		16		// 160mA 0r 0 for Wolverine.
#define		MHL_HDCP_STATUS					0		// Bits set dynamically
*/

// VIDEO TYPES
#define  MHL_VT_GRAPHICS						0x00		
#define  MHL_VT_PHOTO							0x02		
#define  MHL_VT_CINEMA							0x04		
#define  MHL_VT_GAMES							0x08		
#define  MHL_SUPP_VT							0x80		

//Logical Dev Map
#define  MHL_DEV_LD_DISPLAY					(0x01 << 0)
#define  MHL_DEV_LD_VIDEO						(0x01 << 1)
#define  MHL_DEV_LD_AUDIO						(0x01 << 2)
#define  MHL_DEV_LD_MEDIA						(0x01 << 3)
#define  MHL_DEV_LD_TUNER						(0x01 << 4)
#define  MHL_DEV_LD_RECORD						(0x01 << 5)
#define  MHL_DEV_LD_SPEAKER					(0x01 << 6)
#define  MHL_DEV_LD_GUI							(0x01 << 7)

//Bandwidth
#define  MHL_BANDWIDTH_LIMIT					22		// 225 MHz


#define  MHL_STATUS_REG_CONNECTED_RDY   	0x30
#define  MHL_STATUS_REG_LINK_MODE            	0x31

#define  MHL_STATUS_DCAP_RDY				BIT0

#define  MHL_STATUS_CLK_MODE_MASK            	0x07
#define  MHL_STATUS_CLK_MODE_PACKED_PIXEL    0x02
#define  MHL_STATUS_CLK_MODE_NORMAL        	0x03
#define  MHL_STATUS_PATH_EN_MASK             	0x08
#define  MHL_STATUS_PATH_ENABLED             	0x08
#define  MHL_STATUS_PATH_DISABLED            	0x00
#define  MHL_STATUS_MUTED_MASK               	0x10

#define  MHL_RCHANGE_INT                     	0x20
#define  MHL_DCHANGE_INT                     	0x21

#define  MHL_INT_DCAP_CHG				BIT0
#define  MHL_INT_DSCR_CHG               		BIT1
#define  MHL_INT_REQ_WRT                     	BIT2
#define  MHL_INT_GRT_WRT                     	BIT3

// On INTR_1 the EDID_CHG is located at BIT 0
#define	MHL_INT_EDID_CHG				BIT1

#define	MHL_INT_AND_STATUS_SIZE		0x03		// This contains one nibble each - max offset
#define	MHL_SCRATCHPAD_SIZE			16
#define	MHL_MAX_BUFFER_SIZE			MHL_SCRATCHPAD_SIZE	// manually define highest number

// DEVCAP we will initialize to
#define	MHL_LOGICAL_DEVICE_MAP		(MHL_DEV_LD_AUDIO | MHL_DEV_LD_VIDEO | MHL_DEV_LD_MEDIA | MHL_DEV_LD_GUI )

enum
{
    MHL_MSC_MSG_RCP             = 0x10,     // RCP sub-command
    MHL_MSC_MSG_RCPK            = 0x11,     // RCP Acknowledge sub-command
    MHL_MSC_MSG_RCPE            = 0x12,     // RCP Error sub-command
    MHL_MSC_MSG_RAP             = 0x20,     // Mode Change Warning sub-command
    MHL_MSC_MSG_RAPK            = 0x21,     // MCW Acknowledge sub-command
};

#define	RCPE_NO_ERROR					0x00
#define	RCPE_INEEFECTIVE_KEY_CODE	0x01
#define	RCPE_BUSY						0x02
//
// MHL spec related defines
//
enum
{
	MHL_ACK						= 0x33,	// Command or Data uint8_t acknowledge
	MHL_NACK						= 0x34,	// Command or Data uint8_t not acknowledge
	MHL_ABORT						= 0x35,	// Transaction abort
	MHL_WRITE_STAT				= 0x60 | 0x80,	// Write one status register strip top bit
	MHL_SET_INT					= 0x60,	// Write one interrupt register
	MHL_READ_DEVCAP				= 0x61,	// Read one register
	MHL_GET_STATE					= 0x62,	// Read CBUS revision level from follower
	MHL_GET_VENDOR_ID			= 0x63,	// Read vendor ID value from follower.
	MHL_SET_HPD					= 0x64,	// Set Hot Plug Detect in follower
	MHL_CLR_HPD					= 0x65,	// Clear Hot Plug Detect in follower
	MHL_SET_CAP_ID				= 0x66,	// Set Capture ID for downstream device.
	MHL_GET_CAP_ID				= 0x67,	// Get Capture ID from downstream device.
	MHL_MSC_MSG					= 0x68,	// VS command to send RCP sub-commands
	MHL_GET_SC1_ERRORCODE		= 0x69,	// Get Vendor-Specific command error code.
	MHL_GET_DDC_ERRORCODE		= 0x6A,	// Get DDC channel command error code.
	MHL_GET_MSC_ERRORCODE		= 0x6B,	// Get MSC command error code.
	MHL_WRITE_BURST				= 0x6C,	// Write 1-16 bytes to responder’s scratchpad.
	MHL_GET_SC3_ERRORCODE		= 0x6D,	// Get channel 3 command error code.
};

enum
{
    	MHL_RCP_CMD_SELECT          = 0x00,
    	MHL_RCP_CMD_UP              = 0x01,
    	MHL_RCP_CMD_DOWN            = 0x02,
    	MHL_RCP_CMD_LEFT            = 0x03,
    	MHL_RCP_CMD_RIGHT           = 0x04,
    	MHL_RCP_CMD_RIGHT_UP        = 0x05,
    	MHL_RCP_CMD_RIGHT_DOWN      = 0x06,
    	MHL_RCP_CMD_LEFT_UP         = 0x07,
    	MHL_RCP_CMD_LEFT_DOWN       = 0x08,
    	MHL_RCP_CMD_ROOT_MENU       = 0x09,
    	MHL_RCP_CMD_SETUP_MENU      = 0x0A,
    	MHL_RCP_CMD_CONTENTS_MENU   = 0x0B,
    	MHL_RCP_CMD_FAVORITE_MENU   = 0x0C,
    	MHL_RCP_CMD_EXIT            = 0x0D,
	
	//0x0E - 0x1F are reserved

    	MHL_RCP_CMD_NUM_0           = 0x20,
    	MHL_RCP_CMD_NUM_1           = 0x21,
    	MHL_RCP_CMD_NUM_2           = 0x22,
    	MHL_RCP_CMD_NUM_3           = 0x23,
    	MHL_RCP_CMD_NUM_4           = 0x24,
    	MHL_RCP_CMD_NUM_5           = 0x25,
    	MHL_RCP_CMD_NUM_6           = 0x26,
    	MHL_RCP_CMD_NUM_7           = 0x27,
    	MHL_RCP_CMD_NUM_8           = 0x28,
    	MHL_RCP_CMD_NUM_9           = 0x29,

    	MHL_RCP_CMD_DOT             = 0x2A,
    	MHL_RCP_CMD_ENTER           = 0x2B,
    	MHL_RCP_CMD_CLEAR           = 0x2C,

	//0x2D - 0x2F are reserved

    	MHL_RCP_CMD_CH_UP           = 0x30,
    	MHL_RCP_CMD_CH_DOWN         = 0x31,
    	MHL_RCP_CMD_PRE_CH          = 0x32,
    	MHL_RCP_CMD_SOUND_SELECT    = 0x33,
    	MHL_RCP_CMD_INPUT_SELECT    = 0x34,
    	MHL_RCP_CMD_SHOW_INFO       = 0x35,
    	MHL_RCP_CMD_HELP            = 0x36,
    	MHL_RCP_CMD_PAGE_UP         = 0x37,
    	MHL_RCP_CMD_PAGE_DOWN       = 0x38,

	//0x39 - 0x40 are reserved

    	MHL_RCP_CMD_VOL_UP	        = 0x41,
    	MHL_RCP_CMD_VOL_DOWN        = 0x42,
    	MHL_RCP_CMD_MUTE            = 0x43,
    	MHL_RCP_CMD_PLAY            = 0x44,
    	MHL_RCP_CMD_STOP            = 0x45,
    	MHL_RCP_CMD_PAUSE           = 0x46,
    	MHL_RCP_CMD_RECORD          = 0x47,
    	MHL_RCP_CMD_REWIND          = 0x48,
    	MHL_RCP_CMD_FAST_FWD        = 0x49,
    	MHL_RCP_CMD_EJECT           = 0x4A,
    	MHL_RCP_CMD_FWD             = 0x4B,
    	MHL_RCP_CMD_BKWD            = 0x4C,

	//0x4D - 0x4F are reserved

    	MHL_RCP_CMD_ANGLE            = 0x50,
    	MHL_RCP_CMD_SUBPICTURE       = 0x51,

	//0x52 - 0x5F are reserved

    	MHL_RCP_CMD_PLAY_FUNC       = 0x60,
    	MHL_RCP_CMD_PAUSE_PLAY_FUNC = 0x61,
    	MHL_RCP_CMD_RECORD_FUNC     = 0x62,
    	MHL_RCP_CMD_PAUSE_REC_FUNC  = 0x63,
    	MHL_RCP_CMD_STOP_FUNC       = 0x64,
    	MHL_RCP_CMD_MUTE_FUNC       = 0x65,
    	MHL_RCP_CMD_UN_MUTE_FUNC    = 0x66,
    	MHL_RCP_CMD_TUNE_FUNC       = 0x67,
    	MHL_RCP_CMD_MEDIA_FUNC      = 0x68,

	//0x69 - 0x70 are reserved

    	MHL_RCP_CMD_F1              = 0x71,
    	MHL_RCP_CMD_F2              = 0x72,
    	MHL_RCP_CMD_F3              = 0x73,
    	MHL_RCP_CMD_F4              = 0x74,
    	MHL_RCP_CMD_F5              = 0x75,

	//0x76 - 0x7D are reserved

    	MHL_RCP_CMD_VS              = 0x7E,
    	MHL_RCP_CMD_RSVD            = 0x7F,
};

#define	MHL_RAP_CONTENT_ON		0x10	// Turn content streaming ON.
#define	MHL_RAP_CONTENT_OFF		0x11	// Turn content streaming OFF.


//====================================================
//====================================================
//====================================================
//====================================================
#define MAX_V_DESCRIPTORS				20
#define MAX_A_DESCRIPTORS				10
#define MAX_SPEAKER_CONFIGURATIONS	4
#define AUDIO_DESCR_SIZE			 	3

#define RGB					0
#define YCBCR444			1
#define YCBCR422_16BITS		2
#define YCBCR422_8BITS		3
#define XVYCC444			4

#define EXTERNAL_HSVSDE	0
#define INTERNAL_DE			1
#define EMBEDDED_SYNC		2

#define COLORIMETRY_601	0
#define COLORIMETRY_709	1

//====================================================
#define MCLK128FS 			0
#define MCLK256FS 			1
#define MCLK384FS 			2
#define MCLK512FS 			3
#define MCLK768FS 			4
#define MCLK1024FS 			5
#define MCLK1152FS 			6
#define MCLK192FS 			7

#define SCK_SAMPLE_FALLING_EDGE 	0x00
#define SCK_SAMPLE_RISING_EDGE 	0x80

//====================================================
// Video mode define ( = VIC code, please see CEA-861 spec)
#define HDMI_640X480P		1
#define HDMI_480I60_4X3	6
#define HDMI_480I60_16X9	7
#define HDMI_576I50_4X3	21
#define HDMI_576I50_16X9	22
#define HDMI_480P60_4X3	2
#define HDMI_480P60_16X9	3
#define HDMI_576P50_4X3	17
#define HDMI_576P50_16X9	18
#define HDMI_720P60			4
#define HDMI_720P50			19
#define HDMI_1080I60		5
#define HDMI_1080I50		20
#define HDMI_1080P24		32
#define HDMI_1080P25		33
#define HDMI_1080P30		34
//#define HDMI_1080P60		16 //MHL doesn't supported
//#define HDMI_1080P50		31 //MHL doesn't supported

//====================================================
#define AMODE_I2S 			0
#define AMODE_SPDIF 		1
#define AMODE_HBR 			2
#define AMODE_DSD 			3

#define ACHANNEL_2CH		1
#define ACHANNEL_3CH 		2
#define ACHANNEL_4CH 		3
#define ACHANNEL_5CH 		4
#define ACHANNEL_6CH 		5
#define ACHANNEL_7CH 		6
#define ACHANNEL_8CH 		7

#define AFS_44K1			0x00
#define AFS_48K				0x02
#define AFS_32K				0x03
#define AFS_88K2			0x08
#define AFS_768K			0x09
#define AFS_96K				0x0a
#define AFS_176K4			0x0c
#define AFS_192K			0x0e

#define ALENGTH_16BITS		0x02
#define ALENGTH_17BITS		0x0c
#define ALENGTH_18BITS		0x04
#define ALENGTH_19BITS		0x08
#define ALENGTH_20BITS		0x0a
#define ALENGTH_21BITS		0x0d
#define ALENGTH_22BITS		0x05
#define ALENGTH_23BITS		0x09
#define ALENGTH_24BITS		0x0b

//====================================================
typedef struct
{
    	uint8_t HDMIVideoFormat;	// 0 = CEA-861 VIC; 1 = HDMI_VIC; 2 = 3D
    	uint8_t VIC;				// VIC or the HDMI_VIC
    	uint8_t AspectRatio;			// 4x3 or 16x9
    	uint8_t ColorSpace;			// 0 = RGB; 1 = YCbCr4:4:4; 2 = YCbCr4:2:2_16bits; 3 = YCbCr4:2:2_8bits; 4 = xvYCC4:4:4
    	uint8_t ColorDepth;			// 0 = 8bits; 1 = 10bits; 2 = 12bits
    	uint8_t Colorimetry;			// 0 = 601; 1 = 709
	uint8_t SyncMode;			// 0 = external HS/VS/DE; 1 = external HS/VS and internal DE; 2 = embedded sync  
	uint8_t TclkSel;				// 0 = x0.5CLK; 1 = x1CLK; 2 = x2CLK; 3 = x4CLK
    	uint8_t ThreeDStructure;	// Valid when (HDMIVideoFormat == VMD_HDMIFORMAT_3D)
	uint8_t ThreeDExtData;		// Valid when (HDMIVideoFormat == VMD_HDMIFORMAT_3D) && (ThreeDStructure == VMD_3D_SIDEBYSIDEHALF)

	uint8_t AudioMode;			// 0 = I2S; 1 = S/PDIF; 2 = HBR; 3 = DSD;
	uint8_t AudioChannels;		// 1 = 2chs; 2 = 3chs; 3 = 4chs; 4 = 5chs; 5 = 6chs; 6 = 7chs; 7 = 8chs;
	uint8_t AudioFs;			// 0-44.1kHz; 2-48kHz; 3-32kHz; 8-88.2kHz; 9-768kHz; A-96kHz; C-176.4kHz; E-192kHz; 1/4/5/6/7/B/D/F-not indicated
     	uint8_t AudioWordLength; 	// 0/1-not available; 2-16 bit; 4-18 bit; 8-19 bit; A-20 bit; C-17 bit; 5-22 bit; 9-23 bit; B-24 bit; D-21 bit
     	uint8_t AudioI2SFormat;   	// Please refer to TPI reg0x20 for detailed. 
     						 	//[7]_SCK Sample Edge: 0 = Falling; 1 = Rising
     						 	//[6:4]_MCLK Multiplier: 000:MCLK=128Fs; 001:MCLK=256Fs; 010:MCLK=384Fs; 011:MCLK=512Fs; 100:MCLK=768Fs; 101:MCLK=1024Fs; 110:MCLK=1152Fs; 111:MCLK=192Fs;   
     						 	//[3]_WS Polarity-Left when: 0 = WS is low when Left; 1 = WS is high when Left
     						 	//[2]_SD Justify Data is justified: 0 = Left; 1 = Right
     						 	//[1]_SD Direction Byte shifted first: 0 = MSB; 1 = LSB
     						 	//[0]_WS to SD First Bit Shift: 0 = Yes; 1 = No

}mhlTx_AVSetting;

//====================================================
typedef struct
{
	bool_t HDCP_TxSupports;
	bool_t HDCP_AksvValid;
	bool_t HDCP_Started;
	uint8_t HDCP_LinkProtectionLevel;
	bool_t HDCP_Override;
	bool_t HDCPAuthenticated;
	
}mhlTx_Hdcp;

//====================================================
typedef struct
{												// for storing EDID parsed data
	bool_t edidDataValid;
	uint8_t VideoDescriptor[MAX_V_DESCRIPTORS];	// maximum number of video descriptors
	uint8_t AudioDescriptor[MAX_A_DESCRIPTORS][3];	// maximum number of audio descriptors
	uint8_t SpkrAlloc[MAX_SPEAKER_CONFIGURATIONS];	// maximum number of speaker configurations
	uint8_t UnderScan;								// "1" if DTV monitor underscans IT video formats by default
	uint8_t BasicAudio;								// Sink supports Basic Audio
	uint8_t YCbCr_4_4_4;							// Sink supports YCbCr 4:4:4
	uint8_t YCbCr_4_2_2;							// Sink supports YCbCr 4:2:2
	bool_t HDMI_Sink;								// "1" if HDMI signature found
	uint8_t CEC_A_B;								// CEC Physical address. See HDMI 1.3 Table 8-6
	uint8_t CEC_C_D;
	uint8_t ColorimetrySupportFlags;				// IEC 61966-2-4 colorimetry support: 1 - xvYCC601; 2 - xvYCC709 
	uint8_t MetadataProfile;
	bool_t _3D_Supported;
	
} mhlTx_Edid;

enum EDID_ErrorCodes
{
	EDID_OK,
	EDID_INCORRECT_HEADER,
	EDID_CHECKSUM_ERROR,
	EDID_NO_861_EXTENSIONS,
	EDID_SHORT_DESCRIPTORS_OK,
	EDID_LONG_DESCRIPTORS_OK,
	EDID_EXT_TAG_ERROR,
	EDID_REV_ADDR_ERROR,
	EDID_V_DESCR_OVERFLOW,
	EDID_UNKNOWN_TAG_CODE,
	EDID_NO_DETAILED_DESCRIPTORS,
	EDID_DDC_BUS_READ_FAILURE,
	EDID_DDC_BUS_REQ_FAILURE,
	EDID_DDC_BUS_RELEASE_FAILURE
};

enum AV_ConfigErrorCodes
{
    	DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC,
    	V_MODE_NOT_SUPPORTED,
    	SET_EMBEDDED_SYC_FAILURE,
    	I2S_MAPPING_SUCCESSFUL,
    	I2S_INPUT_CONFIG_SUCCESSFUL,
    	I2S_HEADER_SET_SUCCESSFUL,
   	EHDMI_ARC_SINGLE_SET_SUCCESSFUL,
    	EHDMI_ARC_COMMON_SET_SUCCESSFUL,
    	EHDMI_HEC_SET_SUCCESSFUL,
    	EHDMI_ARC_CM_WITH_HEC_SET_SUCCESSFUL,
    	AUD_MODE_NOT_SUPPORTED,
    	I2S_NOT_SET,
    	DE_SET_OK,
    	VIDEO_MODE_SET_OK,
    	AUDIO_MODE_SET_OK,
    	GBD_SET_SUCCESSFULLY,
    	DE_CANNOT_BE_SET_WITH_3D_MODE,
};

// Generic Masks
//====================================================
#define LOW_BYTE      0x00FF

#define LOW_NIBBLE	0x0F
#define HI_NIBBLE  	0xF0

#define MSBIT       	0x80
#define LSBIT          	0x01

#define TWO_LSBITS        	0x03
#define THREE_LSBITS   	0x07
#define FOUR_LSBITS    	0x0F
#define FIVE_LSBITS    	0x1F
#define SEVEN_LSBITS    	0x7F
#define TWO_MSBITS     	0xC0
#define EIGHT_BITS      	0xFF
#define BYTE_SIZE        	0x08
#define BITS_1_0          	0x03
#define BITS_2_1          	0x06
#define BITS_2_1_0        	0x07
#define BITS_3_2              	0x0C
#define BITS_4_3_2       	0x1C  
#define BITS_5_4              	0x30
#define BITS_5_4_3		0x38
#define BITS_6_5             	0x60
#define BITS_6_5_4        	0x70
#define BITS_7_6            	0xC0

#define TPI_INTERNAL_PAGE_REG		0xBC
#define TPI_INDEXED_OFFSET_REG	0xBD
#define TPI_INDEXED_VALUE_REG		0xBE

// Interrupt Masks
//====================================================
#define HOT_PLUG_EVENT          0x01
#define RX_SENSE_EVENT          0x02
#define HOT_PLUG_STATE          0x04
#define RX_SENSE_STATE          0x08

#define AUDIO_ERROR_EVENT       0x10
#define SECURITY_CHANGE_EVENT   0x20
#define V_READY_EVENT           0x40
#define HDCP_CHANGE_EVENT       0x80

#define NON_MASKABLE_INT		0xFF

// TPI Control Masks
//====================================================

#define CS_HDMI_RGB         0x00
#define CS_DVI_RGB          0x03

#define ENABLE_AND_REPEAT	0xC0
#define EN_AND_RPT_MPEG	0xC3
#define DISABLE_MPEG		0x03	// Also Vendor Specific InfoFrames

// Pixel Repetition Masks
//====================================================
#define BIT_BUS_24          0x20
#define BIT_EDGE_RISE       0x10

//Audio Maps
//====================================================
#define BIT_AUDIO_MUTE      0x10

// Input/Output Format Masks
//====================================================
#define BITS_IN_RGB         0x00
#define BITS_IN_YCBCR444    0x01
#define BITS_IN_YCBCR422    0x02

#define BITS_IN_AUTO_RANGE  0x00
#define BITS_IN_FULL_RANGE  0x04
#define BITS_IN_LTD_RANGE   0x08

#define BIT_EN_DITHER_10_8  0x40
#define BIT_EXTENDED_MODE   0x80

#define BITS_OUT_RGB        0x00
#define BITS_OUT_YCBCR444   0x01
#define BITS_OUT_YCBCR422   0x02

#define BITS_OUT_AUTO_RANGE 0x00
#define BITS_OUT_FULL_RANGE 0x04
#define BITS_OUT_LTD_RANGE  0x08

#define BIT_BT_709          0x10


// DE Generator Masks
//====================================================
#define BIT_EN_DE_GEN       0x40
#define DE 					0x00
#define DeDataNumBytes 		12

// Embedded Sync Masks
//====================================================
#define BIT_EN_SYNC_EXTRACT 0x40
#define EMB                 0x80
#define EmbDataNumBytes     8


// Audio Modes
//====================================================
#define AUD_PASS_BASIC      0x00
#define AUD_PASS_ALL        0x01
#define AUD_DOWN_SAMPLE     0x02
#define AUD_DO_NOT_CHECK    0x03

#define REFER_TO_STREAM_HDR     0x00
#define TWO_CHANNELS            	0x00
#define EIGHT_CHANNELS          	0x01
#define AUD_IF_SPDIF            		0x40
#define AUD_IF_I2S              		0x80
#define AUD_IF_DSD				0xC0
#define AUD_IF_HBR				0x04

#define TWO_CHANNEL_LAYOUT      0x00
#define EIGHT_CHANNEL_LAYOUT    0x20


// DDC Bus Addresses
//====================================================
#define DDC_BSTATUS_ADDR_L  0x41
#define DDC_BSTATUS_ADDR_H  0x42
#define DDC_KSV_FIFO_ADDR   0x43
#define KSV_ARRAY_SIZE      128

// DDC Bus Bit Masks
//====================================================
#define BIT_DDC_HDMI        0x80
#define BIT_DDC_REPEATER    0x40
#define BIT_DDC_FIFO_RDY    0x20
#define DEVICE_COUNT_MASK   0x7F

// KSV Buffer Size
//====================================================
#define DEVICE_COUNT         128    // May be tweaked as needed

// InfoFrames
//====================================================
#define SIZE_AVI_INFOFRAME      0x0E     // including checksum uint8_t
#define BITS_OUT_FORMAT         0x60    // Y1Y0 field

#define _4_To_3                 0x10    // Aspect ratio - 4:3  in InfoFrame DByte 1
#define _16_To_9                0x20    // Aspect ratio - 16:9 in InfoFrame DByte 1
#define SAME_AS_AR              0x08    // R3R2R1R0 - in AVI InfoFrame DByte 2

#define BT_601                  0x40
#define BT_709                  0x80

//#define EN_AUDIO_INFOFRAMES         0xC2
#define TYPE_AUDIO_INFOFRAMES       0x84
#define AUDIO_INFOFRAMES_VERSION    0x01
#define AUDIO_INFOFRAMES_LENGTH     0x0A

#define TYPE_GBD_INFOFRAME       	0x0A

#define ENABLE_AND_REPEAT			0xC0

#define EN_AND_RPT_MPEG				0xC3
#define DISABLE_MPEG				0x03	// Also Vendor Specific InfoFrames

#define EN_AND_RPT_AUDIO			0xC2
#define DISABLE_AUDIO				0x02

#define EN_AND_RPT_AVI				0xC0	// Not normally used.  Write to TPI 0x19 instead
#define DISABLE_AVI					0x00	// But this is used to Disable

#define NEXT_FIELD					0x80
#define GBD_PROFILE					0x00
#define AFFECTED_GAMUT_SEQ_NUM		0x01

#define ONLY_PACKET					0x30
#define CURRENT_GAMUT_SEQ_NUM		0x01

// FPLL Multipliers:
//====================================================

#define X0d5                      				0x00
#define X1                      					0x01
#define X2                      					0x02
#define X4                      					0x03

// 3D Constants
//====================================================

#define _3D_STRUC_PRESENT				0x02

// 3D_Stucture Constants
//====================================================
#define FRAME_PACKING					0x00
#define FIELD_ALTERNATIVE				0x01
#define LINE_ALTERNATIVE				0x02
#define SIDE_BY_SIDE_FULL				0x03
#define L_PLUS_DEPTH					0x04
#define L_PLUS_DEPTH_PLUS_GRAPHICS	0x05
#define SIDE_BY_SIDE_HALF				0x08

// 3D_Ext_Data Constants
//====================================================
#define HORIZ_ODD_LEFT_ODD_RIGHT		0x00
#define HORIZ_ODD_LEFT_EVEN_RIGHT		0x01
#define HORIZ_EVEN_LEFT_ODD_RIGHT  		0x02
#define HORIZ_EVEN_LEFT_EVEN_RIGHT		0x03

#define QUINCUNX_ODD_LEFT_EVEN_RIGHT	0x04
#define QUINCUNX_ODD_LEFT_ODD_RIGHT		0x05
#define QUINCUNX_EVEN_LEFT_ODD_RIGHT	0x06
#define QUINCUNX_EVEN_LEFT_EVEN_RIGHT	0x07

#define NO_3D_SUPPORT					0x0F

// InfoFrame Type Code
//====================================================
#define AVI  						0x00 
#define SPD  						0x01  
#define AUDIO 						0x02   
#define MPEG 						0x03 
#define GEN_1	 					0x04
#define GEN_2 						0x05  
#define HDMI_VISF 					0x06 
#define GBD 						0x07 

// Size of InfoFrame Data types
#define MAX_SIZE_INFOFRAME_DATA     0x22
#define SIZE_AVI_INFOFRAME			0x0E	// 14 bytes
#define SIZE_SPD_INFOFRAME 			0x19	// 25 bytes
#define SISE_AUDIO_INFOFRAME_IFORM  0x0A    // 10 bytes
#define SIZE_AUDIO_INFOFRAME		0x0F	// 15 bytes
#define SIZE_MPRG_HDMI_INFOFRAME    0x1B	// 27 bytes		
#define SIZE_MPEG_INFOFRAME 		0x0A	// 10 bytes
#define SIZE_GEN_1_INFOFRAME    	0x1F	// 31 bytes
#define SIZE_GEN_2_INFOFRAME    	0x1F	// 31 bytes
#define SIZE_HDMI_VISF_INFOFRAME    0x1E	// 31 bytes
#define SIZE_GBD_INFOFRAME     		0x1C  	// 28 bytes

#define AVI_INFOFRM_OFFSET          0x0C
#define OTHER_INFOFRM_OFFSET        0xC4
#define TPI_INFOFRAME_ACCESS_REG    0xBF

// Serial Communication Buffer constants
#define MAX_COMMAND_ARGUMENTS       50
#define GLOBAL_BYTE_BUF_BLOCK_SIZE  131


// Video Mode Constants
//====================================================
#define VMD_ASPECT_RATIO_4x3			0x01
#define VMD_ASPECT_RATIO_16x9			0x02

#define VMD_COLOR_SPACE_RGB			0x00
#define VMD_COLOR_SPACE_YCBCR422		0x01
#define VMD_COLOR_SPACE_YCBCR444		0x02

#define VMD_COLOR_DEPTH_8BIT			0x00
#define VMD_COLOR_DEPTH_10BIT			0x01
#define VMD_COLOR_DEPTH_12BIT			0x02
#define VMD_COLOR_DEPTH_16BIT			0x03

#define VMD_HDCP_NOT_AUTHENTICATED	0
#define VMD_HDCP_AUTHENTICATED		1

#define VMD_HDMIFORMAT_CEA_VIC          	0x00
#define VMD_HDMIFORMAT_HDMI_VIC         	0x01
#define VMD_HDMIFORMAT_3D               	0x02
#define VMD_HDMIFORMAT_PC               	0x03

// These values are from HDMI Spec 1.4 Table H-2
#define VMD_3D_FRAMEPACKING			0
#define VMD_3D_FIELDALTERNATIVE		1
#define VMD_3D_LINEALTERNATIVE		2              
#define VMD_3D_SIDEBYSIDEFULL			3
#define VMD_3D_LDEPTH					4
#define VMD_3D_LDEPTHGRAPHICS			5
#define VMD_3D_SIDEBYSIDEHALF			8


//--------------------------------------------------------------------
// System Macro Definitions
//--------------------------------------------------------------------
#define TX_HW_RESET_PERIOD      10
#define SII92326_DEVICE_ID         0xB0

#define T_HPD_DELAY    			10

//--------------------------------------------------------------------
// HDCP Macro Definitions
//--------------------------------------------------------------------
#define AKSV_SIZE              		5
#define NUM_OF_ONES_IN_KSV	20

//--------------------------------------------------------------------
// EDID Constants Definition
//--------------------------------------------------------------------
#define EDID_BLOCK_0_OFFSET 0x00
#define EDID_BLOCK_1_OFFSET 0x80

#define EDID_BLOCK_SIZE      128
#define EDID_HDR_NO_OF_FF   0x06
#define NUM_OF_EXTEN_ADDR   0x7E

#define EDID_TAG_ADDR       0x00
#define EDID_REV_ADDR       0x01
#define EDID_TAG_IDX        0x02
#define LONG_DESCR_PTR_IDX  0x02
#define MISC_SUPPORT_IDX    0x03

#define ESTABLISHED_TIMING_INDEX        35      // Offset of Established Timing in EDID block
#define NUM_OF_STANDARD_TIMINGS          8
#define STANDARD_TIMING_OFFSET          38
#define LONG_DESCR_LEN                  18
#define NUM_OF_DETAILED_DESCRIPTORS      4

#define DETAILED_TIMING_OFFSET        0x36

// Offsets within a Long Descriptors Block
//====================================================
#define PIX_CLK_OFFSET                	 	0
#define H_ACTIVE_OFFSET               	2
#define H_BLANKING_OFFSET          	3
#define V_ACTIVE_OFFSET                  	5
#define V_BLANKING_OFFSET                6
#define H_SYNC_OFFSET                    	8
#define H_SYNC_PW_OFFSET                 9
#define V_SYNC_OFFSET                   	10
#define V_SYNC_PW_OFFSET                	10
#define H_IMAGE_SIZE_OFFSET            	12
#define V_IMAGE_SIZE_OFFSET           	13
#define H_BORDER_OFFSET                 	15
#define V_BORDER_OFFSET                 	16
#define FLAGS_OFFSET                    	17

#define AR16_10                          		0
#define AR4_3                            		1
#define AR5_4                            		2
#define AR16_9                           		3

// Data Block Tag Codes
//====================================================
#define AUDIO_D_BLOCK       0x01
#define VIDEO_D_BLOCK       0x02
#define VENDOR_SPEC_D_BLOCK 0x03
#define SPKR_ALLOC_D_BLOCK  0x04
#define USE_EXTENDED_TAG    0x07

// Extended Data Block Tag Codes
//====================================================
#define COLORIMETRY_D_BLOCK 0x05

#define HDMI_SIGNATURE_LEN  0x03

#define CEC_PHYS_ADDR_LEN   0x02
#define EDID_EXTENSION_TAG  0x02
#define EDID_REV_THREE      0x03
#define EDID_DATA_START     0x04

#define EDID_BLOCK_0        0x00
#define EDID_BLOCK_2_3      0x01

#define VIDEO_CAPABILITY_D_BLOCK 0x00


//--------------------------------------------------------------------
// TPI Register Definition
//--------------------------------------------------------------------

// TPI Video Mode Data
#define TPI_PIX_CLK_LSB							(0x00)
#define TPI_PIX_CLK_MSB							(0x01)
#define TPI_VERT_FREQ_LSB						(0x02)
#define TPI_VERT_FREQ_MSB						(0x03)
#define TPI_TOTAL_PIX_LSB						(0x04)
#define TPI_TOTAL_PIX_MSB						(0x05)
#define TPI_TOTAL_LINES_LSB					(0x06)
#define TPI_TOTAL_LINES_MSB					(0x07)

// Pixel Repetition Data
#define TPI_PIX_REPETITION						(0x08)

// TPI AVI Input and Output Format Data
/// AVI Input Format Data
#define TPI_INPUT_FORMAT_REG					(0x09)
#define INPUT_COLOR_SPACE_MASK				(BIT1 | BIT0)
#define INPUT_COLOR_SPACE_RGB					(0x00)
#define INPUT_COLOR_SPACE_YCBCR444			(0x01)
#define INPUT_COLOR_SPACE_YCBCR422			(0x02)
#define INPUT_COLOR_SPACE_BLACK_MODE			(0x03)

/// AVI Output Format Data
#define TPI_OUTPUT_FORMAT_REG					(0x0A)
#define TPI_YC_Input_Mode						(0x0B)

// TPI InfoFrame related constants
#define TPI_AVI_INFO_REG_ADDR					(0x0C) // AVI InfoFrame Checksum
#define TPI_OTHER_INFO_REG_ADDR		  		(0xBF)
#define TPI_INFO_FRAME_REG_OFFSET				(0xC4)

// TPI AVI InfoFrame Data
#define TPI_AVI_BYTE_0							(0x0C)
#define TPI_AVI_BYTE_1							(0x0D)
#define TPI_AVI_BYTE_2							(0x0E)
#define TPI_AVI_BYTE_3							(0x0F)
#define TPI_AVI_BYTE_4							(0x10)
#define TPI_AVI_BYTE_5							(0x11)

#define TPI_INFO_FRM_DBYTE5					(0xC8)
#define TPI_INFO_FRM_DBYTE6					(0xC9)

#define TPI_END_TOP_BAR_LSB					(0x12)
#define TPI_END_TOP_BAR_MSB					(0x13)

#define TPI_START_BTM_BAR_LSB					(0x14)
#define TPI_START_BTM_BAR_MSB					(0x15)

#define TPI_END_LEFT_BAR_LSB					(0x16)
#define TPI_END_LEFT_BAR_MSB					(0x17)

#define TPI_END_RIGHT_BAR_LSB					(0x18)
#define TPI_END_RIGHT_BAR_MSB					(0x19)

// Colorimetry
#define SET_EX_COLORIMETRY						(0x0C)	// Set TPI_AVI_BYTE_2 to extended colorimetry and use 
													//TPI_AVI_BYTE_3

#define TPI_SYSTEM_CONTROL_DATA_REG			(0x1A)

#define LINK_INTEGRITY_MODE_MASK				(BIT6)
#define LINK_INTEGRITY_STATIC					(0x00)
#define LINK_INTEGRITY_DYNAMIC					(0x40)

#define TMDS_OUTPUT_CONTROL_MASK				(BIT4)
#define TMDS_OUTPUT_CONTROL_ACTIVE			(0x00)
#define TMDS_OUTPUT_CONTROL_POWER_DOWN	(0x10)

#define AV_MUTE_MASK							(BIT3)
#define AV_MUTE_NORMAL						(0x00)
#define AV_MUTE_MUTED							(0x08)

#define DDC_BUS_REQUEST_MASK					(BIT2)
#define DDC_BUS_REQUEST_NOT_USING			(0x00)
#define DDC_BUS_REQUEST_REQUESTED			(0x04)

#define DDC_BUS_GRANT_MASK					(BIT1)
#define DDC_BUS_GRANT_NOT_AVAILABLE			(0x00)
#define DDC_BUS_GRANT_GRANTED				(0x02)

#define OUTPUT_MODE_MASK						(BIT0)
#define OUTPUT_MODE_DVI						(0x00)
#define OUTPUT_MODE_HDMI						(0x01)

// TPI Identification Registers
#define TPI_DEVICE_ID							(0x1B)
#define TPI_DEVICE_REV_ID						(0x1C)

#define TPI_RESERVED2							(0x1D)

#define TPI_DEVICE_POWER_STATE_CTRL_REG		(0x1E)

#define CTRL_PIN_CONTROL_MASK					(BIT4)
#define CTRL_PIN_TRISTATE						(0x00)
#define CTRL_PIN_DRIVEN_TX_BRIDGE				(0x10)

#define TX_POWER_STATE_MASK					(BIT1 | BIT0)
#define TX_POWER_STATE_D0						(0x00)
#define TX_POWER_STATE_D1						(0x01)
#define TX_POWER_STATE_D2						(0x02)
#define TX_POWER_STATE_D3						(0x03)

// Configuration of I2S Interface
#define TPI_I2S_EN								(0x1F)
#define TPI_I2S_IN_CFG							(0x20)
#define SCK_SAMPLE_EDGE						(BIT7)

// Available only when TPI 0x26[7:6]=10 to select I2S input
#define TPI_I2S_CHST_0							(0x21)
#define TPI_I2S_CHST_1							(0x22)
#define TPI_I2S_CHST_2							(0x23)
#define TPI_I2S_CHST_3							(0x24)
#define TPI_I2S_CHST_4							(0x25)

#define AUDIO_INPUT_LENGTH					(0x24)

// Available only when 0x26[7:6]=01
#define TPI_SPDIF_HEADER						(0x24)
#define TPI_AUDIO_HANDLING						(0x25)

// Audio Configuration Regiaters
#define TPI_AUDIO_INTERFACE_REG				(0x26)
#define AUDIO_MUTE_MASK						(BIT4)
#define AUDIO_MUTE_NORMAL						(0x00)
#define AUDIO_MUTE_MUTED						(0x10)

#define AUDIO_SEL_MASK							(BITS_7_6)


#define TPI_AUDIO_SAMPLE_CTRL					(0x27)
#define TPI_SPEAKER_CFG						(0xC7)
#define TPI_CODING_TYPE_CHANNEL_COUNT		(0xC4)

//--------------------------------------------------------------------
// HDCP Implementation
// HDCP link security logic is implemented in certain transmitters; unique
//   keys are embedded in each chip as part of the solution. The security 
//   scheme is fully automatic and handled completely by the hardware.
//--------------------------------------------------------------------

/// HDCP Query Data Register
#define TPI_HDCP_QUERY_DATA_REG				(0x29)

#define EXTENDED_LINK_PROTECTION_MASK		(BIT7)
#define EXTENDED_LINK_PROTECTION_NONE		(0x00)
#define EXTENDED_LINK_PROTECTION_SECURE		(0x80)

#define LOCAL_LINK_PROTECTION_MASK			(BIT6)
#define LOCAL_LINK_PROTECTION_NONE			(0x00)
#define LOCAL_LINK_PROTECTION_SECURE			(0x40)

#define LINK_STATUS_MASK						(BIT5 | BIT4)
#define LINK_STATUS_NORMAL					(0x00)
#define LINK_STATUS_LINK_LOST					(0x10)
#define LINK_STATUS_RENEGOTIATION_REQ		(0x20)
#define LINK_STATUS_LINK_SUSPENDED			(0x30)

#define HDCP_REPEATER_MASK					(BIT3)
#define HDCP_REPEATER_NO						(0x00)
#define HDCP_REPEATER_YES						(0x08)

#define CONNECTOR_TYPE_MASK					(BIT2 | BIT0)
#define CONNECTOR_TYPE_DVI						(0x00)
#define CONNECTOR_TYPE_RSVD					(0x01)
#define CONNECTOR_TYPE_HDMI					(0x04)
#define CONNECTOR_TYPE_FUTURE					(0x05)

#define PROTECTION_TYPE_MASK					(BIT1)
#define PROTECTION_TYPE_NONE					(0x00)
#define PROTECTION_TYPE_HDCP					(0x02)

/// HDCP Control Data Register
#define TPI_HDCP_CONTROL_DATA_REG			(0x2A)
#define PROTECTION_LEVEL_MASK					(BIT0)
#define PROTECTION_LEVEL_MIN					(0x00)
#define PROTECTION_LEVEL_MAX					(0x01)

#define KSV_FORWARD_MASK						(BIT4)
#define KSV_FORWARD_ENABLE					(0x10)
#define KSV_FORWARD_DISABLE					(0x00)

/// HDCP BKSV Registers
#define TPI_BKSV_1_REG							(0x2B)
#define TPI_BKSV_2_REG							(0x2C)
#define TPI_BKSV_3_REG							(0x2D)
#define TPI_BKSV_4_REG							(0x2E)
#define TPI_BKSV_5_REG							(0x2F)

/// HDCP Revision Data Register
#define TPI_HDCP_REVISION_DATA_REG			(0x30)

#define HDCP_MAJOR_REVISION_MASK				(BIT7 | BIT6 | BIT5 | BIT4)
#define HDCP_MAJOR_REVISION_VALUE				(0x10)

#define HDCP_MINOR_REVISION_MASK				(BIT3 | BIT2 | BIT1 | BIT0)
#define HDCP_MINOR_REVISION_VALUE				(0x02)

/// HDCP KSV and V' Value Data Register
#define TPI_V_PRIME_SELECTOR_REG				(0x31)

/// V' Value Readback Registers
#define TPI_V_PRIME_7_0_REG					(0x32)
#define TPI_V_PRIME_15_9_REG					(0x33)
#define TPI_V_PRIME_23_16_REG					(0x34)
#define TPI_V_PRIME_31_24_REG					(0x35)

/// HDCP AKSV Registers
#define TPI_AKSV_1_REG							(0x36)
#define TPI_AKSV_2_REG							(0x37)
#define TPI_AKSV_3_REG							(0x38)
#define TPI_AKSV_4_REG							(0x39)
#define TPI_AKSV_5_REG							(0x3A)

#define TPI_DEEP_COLOR_GCP						(0x40)

//--------------------------------------------------------------------
// Interrupt Service
// TPI can be configured to generate an interrupt to the host to notify it of
//   various events. The host can either poll for activity or use an interrupt
//   handler routine. TPI generates on a single interrupt (INT) to the host.
//--------------------------------------------------------------------

/// Interrupt Enable Register
#define TPI_INTERRUPT_ENABLE_REG				(0x3C)

#define HDCP_AUTH_STATUS_CHANGE_EN_MASK	(BIT7)
#define HDCP_AUTH_STATUS_CHANGE_DISABLE		(0x00)
#define HDCP_AUTH_STATUS_CHANGE_ENABLE		(0x80)

#define HDCP_VPRIME_VALUE_READY_EN_MASK		(BIT6)
#define HDCP_VPRIME_VALUE_READY_DISABLE		(0x00)
#define HDCP_VPRIME_VALUE_READY_ENABLE		(0x40)

#define HDCP_SECURITY_CHANGE_EN_MASK		(BIT5)
#define HDCP_SECURITY_CHANGE_DISABLE		    	(0x00)
#define HDCP_SECURITY_CHANGE_ENABLE			(0x20)

#define AUDIO_ERROR_EVENT_EN_MASK			(BIT4)
#define AUDIO_ERROR_EVENT_DISABLE				(0x00)
#define AUDIO_ERROR_EVENT_ENABLE				(0x10)

#define CPI_EVENT_NO_RX_SENSE_MASK			(BIT3)
#define CPI_EVENT_NO_RX_SENSE_DISABLE		(0x00)
#define CPI_EVENT_NO_RX_SENSE_ENABLE			(0x08)

#define RECEIVER_SENSE_EVENT_EN_MASK			(BIT1)
#define RECEIVER_SENSE_EVENT_DISABLE			(0x00)
#define RECEIVER_SENSE_EVENT_ENABLE			(0x02)

#define HOT_PLUG_EVENT_EN_MASK				(BIT0)
#define HOT_PLUG_EVENT_DISABLE				(0x00)
#define HOT_PLUG_EVENT_ENABLE					(0x01)

/// Interrupt Status Register
#define TPI_INTERRUPT_STATUS_REG				(0x3D)

#define HDCP_AUTH_STATUS_CHANGE_EVENT_MASK	(BIT7)
#define HDCP_AUTH_STATUS_CHANGE_EVENT_NO	(0x00)
#define HDCP_AUTH_STATUS_CHANGE_EVENT_YES	(0x80)

#define HDCP_VPRIME_VALUE_READY_EVENT_MASK	(BIT6)
#define HDCP_VPRIME_VALUE_READY_EVENT_NO	(0x00)
#define HDCP_VPRIME_VALUE_READY_EVENT_YES	(0x40)

#define HDCP_SECURITY_CHANGE_EVENT_MASK		(BIT5)
#define HDCP_SECURITY_CHANGE_EVENT_NO		(0x00)
#define HDCP_SECURITY_CHANGE_EVENT_YES		(0x20)

#define AUDIO_ERROR_EVENT_MASK				(BIT4)
#define AUDIO_ERROR_EVENT_NO					(0x00)
#define AUDIO_ERROR_EVENT_YES					(0x10)

#define CPI_EVENT_MASK							(BIT3)
#define CPI_EVENT_NO							(0x00)
#define CPI_EVENT_YES							(0x08)
#define RX_SENSE_MASK							(BIT3)		// This bit is dual purpose depending on the value of 0x3C[3]
#define RX_SENSE_NOT_ATTACHED					(0x00)
#define RX_SENSE_ATTACHED						(0x08)

#define HOT_PLUG_PIN_STATE_MASK				(BIT2)
#define HOT_PLUG_PIN_STATE_LOW				(0x00)
#define HOT_PLUG_PIN_STATE_HIGH				(0x04)

#define RECEIVER_SENSE_EVENT_MASK				(BIT1)
#define RECEIVER_SENSE_EVENT_NO				(0x00)
#define RECEIVER_SENSE_EVENT_YES				(0x02)

#define HOT_PLUG_EVENT_MASK					(BIT0)
#define HOT_PLUG_EVENT_NO						(0x00)
#define HOT_PLUG_EVENT_YES					(0x01)

/// KSV FIFO First Status Register
#define TPI_KSV_FIFO_READY_INT					(0x3E)

#define KSV_FIFO_READY_MASK					(BIT1)
#define KSV_FIFO_READY_NO						(0x00)
#define KSV_FIFO_READY_YES						(0x02)

#define TPI_KSV_FIFO_READY_INT_EN				(0x3F)

#define KSV_FIFO_READY_EN_MASK				(BIT1)
#define KSV_FIFO_READY_DISABLE				(0x00)
#define KSV_FIFO_READY_ENABLE					(0x02)

/// KSV FIFO Last Status Register
#define TPI_KSV_FIFO_STATUS_REG				(0x41)
#define TPI_KSV_FIFO_VALUE_REG					(0x42)

#define KSV_FIFO_LAST_MASK						(BIT7)
#define KSV_FIFO_LAST_NO						(0x00)
#define KSV_FIFO_LAST_YES						(0x80)

#define KSV_FIFO_COUNT_MASK					(BIT4 | BIT3 | BIT2 | BIT1 | BIT0)

// Sync Register Configuration and Sync Monitoring Registers
#define TPI_SYNC_GEN_CTRL						(0x60)
#define TPI_SYNC_POLAR_DETECT					(0x61)

// Explicit Sync DE Generator Registers (TPI 0x60[7]=0)
#define TPI_DE_DLY								(0x62)
#define TPI_DE_CTRL								(0x63)
#define TPI_DE_TOP								(0x64)

#define TPI_RESERVED4							(0x65)

#define TPI_DE_CNT_7_0							(0x66)
#define TPI_DE_CNT_11_8						(0x67)

#define TPI_DE_LIN_7_0							(0x68)
#define TPI_DE_LIN_10_8							(0x69)

#define TPI_DE_H_RES_7_0						(0x6A)
#define TPI_DE_H_RES_10_8						(0x6B)

#define TPI_DE_V_RES_7_0						(0x6C)
#define TPI_DE_V_RES_10_8						(0x6D)

// Embedded Sync Register Set (TPI 0x60[7]=1)
#define TPI_HBIT_TO_HSYNC_7_0					(0x62)
#define TPI_HBIT_TO_HSYNC_9_8					(0x63)
#define TPI_FIELD_2_OFFSET_7_0					(0x64)
#define TPI_FIELD_2_OFFSET_11_8				(0x65)
#define TPI_HWIDTH_7_0							(0x66)
#define TPI_HWIDTH_8_9							(0x67)
#define TPI_VBIT_TO_VSYNC						(0x68)
#define TPI_VWIDTH								(0x69)

// H/W Optimization Control Registers
#define TPI_HW_OPT_CTRL_1						(0xB9)
#define TPI_HW_OPT_CTRL_2						(0xBA)
#define TPI_HW_OPT_CTRL_3						(0xBB)

// H/W Optimization Control Register #3 Set 
#define DDC_DELAY_BIT9_MASK					(BIT7)
#define DDC_DELAY_BIT9_NO						(0x00)
#define DDC_DELAY_BIT9_YES						(0x80)
#define RI_CHECK_SKIP_MASK						(BIT3)
#define RI_CHECK_SKIP_NO						(0x00)
#define RI_CHECK_SKIP_YES						(0x08)

// TPI Enable Register
#define TPI_ENABLE								(0xC7)

// Misc InfoFrames
#define MISC_INFO_FRAMES_CTRL					(0xBF)
#define MISC_INFO_FRAMES_TYPE					(0xC0)
#define MISC_INFO_FRAMES_VER					(0xC1)
#define MISC_INFO_FRAMES_LEN					(0xC2)
#define MISC_INFO_FRAMES_CHKSUM				(0xC3)
//--------------------------------------------------------------------

//--------------------------------------------------------------------
// Dev0x72 Indexed Register Definition
//--------------------------------------------------------------------
#define SRST										(0x05)
#define MHL_FIFO_RST_MASK						(BIT4)
#define CBUS_RST_MASK							(BIT3)
#define SW_RST_AUTO_MASK						(BIT2)
#define SW_RST_MASK							(BIT0)
//--------------------------------------------------------------------

uint8_t ReadIndexedRegister(uint8_t PageNum, uint8_t RegOffset) ;
void WriteIndexedRegister (uint8_t PageNum, uint8_t RegOffset, uint8_t RegValue);
void siMhlTx_VideoSel (uint8_t vmode);
void siMhlTx_AudioSel (uint8_t Afs);
uint8_t siMhlTx_VideoSet (void);
bool_t SiiMhlTxRequestWriteBurst(void);
bool_t MhlTxCBusBusy(void);

extern mhlTx_config_t mhlTxConfig;
//--------------------------------------------------------------------
//--------------------------------------------------------------------
#endif //__SII_92326_DRIVER_H__
