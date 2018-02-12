//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------
#ifndef _ASKSINMAIN_H
#define _ASKSINMAIN_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define AS_DBG																	// activate to see asksin debug infos's
//#define AS_DBG_Explain														// activate to see more asksin debug infos's

#define USE_ADRESS_SECTION    0													// see Register.h
#define RESET_MODE_SOFT       0

#define LED_MODE_CONFIG       0													// led flashes only at config button actions
#define LED_MODE_EVERYTIME    1													// led flashes at every actions

#define POWER_MODE_ON         1													// Device is on every time
#define POWER_MODE_BURST      2													// Device can wake up with burst
#define POWER_MODE_SLEEP_WDT  3													// Device sleeps. The watchdog wake up the device every 250 ms
#define POWER_MODE_SLEEP_DEEP 4													// Device sleeps. Only level interrupt wake up the device (e.g. external key press)

#define TIMEOUT_WDT_RESET     300
#define TIMEOUT_AFTER_SENDING 300

#ifdef AS_DBG || AS_DBG_Explain
	#include "utility/Serial.h"
#endif

#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include "utility/cc110x.h"
#include "utility/Helpers.h"
#include "utility/StatusLed.h"
#include "utility/Battery.h"
#include "utility/Fastdelegate.h"
#include <util/delay.h>


//- some structures for device definition in Register.h -------------------------------------------------------------------
struct s_cnlDefType {			// list object for channel definition
	uint8_t       cnl;																	// channel number
	uint8_t       lst;																	// list number
	uint8_t       pMax;																	// amount of peers
	uint8_t       sIdx;																	// index in slice string
	uint8_t       sLen;																	// len of slice
	uint16_t      pAddr;																// physical address in eeprom
	uint16_t      pPeer;																// physical peer address in eeprom
	void          *pRegs;																// pointer to regs structure
};
struct s_defaultRegsTbl {		// struct for storing some defaults
	uint8_t       prIn;																	// peer or regs indicator, 0 = peer, 1 = regs
	uint8_t       cnl;																	// channel number
	uint8_t       lst;																	// list number
	uint8_t       pIdx;																	// peer index
	uint8_t       len;																	// len of payload
	const uint8_t *b;																	// payload byte array
};
//- -----------------------------------------------------------------------------------------------------------------------

static volatile uint8_t resetWdt_flag;

//- interrupt handling
static volatile uint8_t int0_flag;
static volatile uint8_t wd_flag;
extern volatile unsigned long timer0_millis;											// make millis timer available for correcting after deep sleep

//- typedef for delegate to module function
using namespace fastdelegate;
typedef FastDelegate5<uint8_t, uint8_t, uint8_t, uint8_t*, uint8_t> s_mod_dlgt;			// void    hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len)

//- -----------------------------------------------------------------------------------------------------------------------
//- Homematic communication functions -------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
class HM {
  public://----------------------------------------------------------------------------------------------------------------
	//- jumptable for calling functions in the main sketch
	struct s_jumptable {
		uint8_t by3;																	// one byte command code
		uint8_t by10;																	// one byte command specifier
		uint8_t by11;																	// one byte command specifier
		void (*fun)(uint8_t*, uint8_t);													// pointer to function
	};

	//- typedef for delegate to module function
	//s_modtable *modTbl;																	// size the module object
	struct s_modtable {
		uint8_t use;																	// channel where the module is registered to
		uint8_t msgCnt;																	// channel message counter
		s_mod_dlgt mDlgt;																// delegate to the module function
	};																					// pointer, we size it in init function


	//- address definition per channel for navigation in slice string table
	struct s_devDef {			// device definition table
		const uint8_t      cnlNbr;														// number of channels
		const uint8_t      lstNbr;														// number of lists
		uint8_t	           *slPtr;														// pointer to slice string
		const s_cnlDefType *chPtr;														// pointer to array of elements
	};
	struct s_dtRegs {			// structure to find the default settings
		uint8_t nbr;
		s_defaultRegsTbl   *ptr;
	};
	struct s_eeprom {			// eeprom definition
		uint16_t           magicNr;														// address and len of magic number
		uint16_t           peerDB; 														// start address and len of peer database
		uint16_t           regsDB;														// start address and len of register settings database
		uint16_t           userSpace;													// user space for storing own values
	};

	//- homematic communication specific declarations
	#define send_payLoad		(send.data + 10)										// payload for send queue
	#define recv_payLoad		(recv.data + 10)										// payload for receive queue

	struct s_devParm {
		uint8_t  maxRetr;																// max send retries
		uint16_t timeOut;																// timeout for ACK sending
		const uint8_t  *p;																// pointer to PROGMEM serial number, etc
		//uint8_t  *cnlCnt;																// message counter, will sized in init
		uint8_t  MAID[3];																// master id for further use
	};
	struct s_send {																		// send queue structure
		uint8_t  data[60];																// buffer for send string
		uint8_t  mCnt;																	// message counter
		uint8_t  counter;																// send try counter, has to be 0 if send queue is empty
		uint8_t  retries;																// set max. retries, if message requires an ACK, retries will set to 3
		uint32_t timer;																	// timer variable used for store next check time
		uint32_t startMillis;
		uint8_t  burst;																	// receiver needs burst signal
	} send;
	struct s_recv {																		// receive queue structure
		uint8_t data[60];																// buffer for received string
		uint8_t forUs :1;																// for us indication flag, is set while the received message was addressed to us
		uint8_t bCast :1;																// broadcast indication flag
		uint8_t mCnt;																	// message counter for pair communication
	} recv;
	struct s_powr {
		uint8_t  mode;																	// indicate the power mode, TX enabled in all modes; 0 = RX enabled,  1 = RX in burst mode, 2 = RX off
		uint8_t  state;																	// current state of TRX868 module
		uint16_t wdTme;																	// clock cycle of watch dog in ms
		uint32_t nxtTO;																	// check millis() timer against, if millis() >= nextTimeout go in powerdown
		uint32_t startMillis;
	} powr;

	CC110x cc;																			// declaration of communication class
	StatusLed statusLed;																// declaration of status led
	Battery battery;

	uint8_t hmId[3];																	// own HMID

	unsigned long wdtResetTimer;														// timer to count timeout before watchdog reset

	//- homematic public protocol functions
	void     init(void);																// Ok, initialize the HomeMatic module
	void     poll(void);																// OK, main task to manage TX and RX messages
	void     send_out(void);															// OK, send function

	void     reset(void);																// clear peer database and register content, do a reset of the device
	void     resetWdt(void);															// initiate a watchdog reset
	void     setPowerMode(uint8_t mode);												// set power mode for HM device
	void     stayAwake(uint32_t millisAwake);											// switch TRX module in RX mode for x milliseconds
	void     setLedMode(uint8_t ledMode);

	//- some functions for checking the config, preparing eeprom and load defaults to eeprom or in regs structure
	void     printConfig(void);															// Ok, show the config on serial console
	void     prepEEprom(void);															// Ok, check chDefType[] for changes and format eeprom space accordingly
	void     loadDefaults(void);														// Ok, load defaults for regs or peer to eeprom
	void     loadRegs(void);															// Ok, load regs table from eeprom
	void     regCnlModule(uint8_t cnl, s_mod_dlgt Delegate, uint16_t *mainList, uint16_t *peerList); // register a module to specific channel
	uint32_t getHMID(void);
	uint8_t  getMsgCnt(void);

	//- external functions for pairing and communicating with the module
	void     startPairing(void);														// , start pairing with master
	void     sendInfoActuatorStatus(uint8_t cnl, uint8_t status, uint8_t flag);			// , send status function
	void     sendACKStatus(uint8_t cnl, uint8_t status, uint8_t douolo);				// , send ACK with status
	void     sendPeerREMOTE(uint8_t button, uint8_t longPress, uint8_t lowBat);			// (0x40) send REMOTE event to all peers
//	void     sendPeerWEATHER(uint8_t cnl, uint16_t temp, uint8_t hum, uint16_t pres, uint32_t lux);	// (0x70) send WEATHER event
	// debugging
	void     sendPeerWEATHER(uint8_t cnl, int16_t temp, uint8_t hum, uint16_t pres, uint32_t lux);
	void     send_ACK(void);															// , ACK sending function
	void     send_NACK(void);															// , NACK sending function

	
  //protected://-------------------------------------------------------------------------------------------------------------
	//- some structs for configuration loops
	struct s_conf {				// structure for handling configuration requests
		uint8_t mCnt;																	// message counter
		uint8_t reID[3];																// requester id
		uint8_t channel;																// requested channel
		uint8_t list;																	// requested list
		uint8_t peer[4];																// requested peer id and peer channel
		uint8_t type;																	// message type for answer
		uint8_t wrEn;																	// write enabled
		uint8_t act;																	// active, 1 = yes, 0 = no
	} conf;
	struct s_pevt {				// structure for handling send peer events
		uint8_t reqACK;																	// ack requested, if it's an repeated long then not
		uint8_t type;																	// message type, 0x40 for remote
		uint8_t data[20];																// data to send
		uint8_t len;																	// len of data to send
		s_cnlDefType *t;																// pointer to channel line
		uint8_t msgCnt;																	// message counter, when already active we can recognize that a new job in the same channel index has happens
		uint8_t act;																	// active, 1 = yes, 0 = no
	} pevt;

	//- some short hands for receive string handling
	#define  recv_len			recv.data[0]											// length of received bytes
	#define  recv_rCnt			recv.data[1]											// requester message counter, important for ACK
	#define  recv_reID			recv.data+4												// requester ID - who had send the message
	#define  recv_msgTp			recv.data[3]											// message type
	#define  recv_by10			recv.data[10]											// byte 10 type
	#define  recv_by11			recv.data[11]											// byte 11 type

	#define  recv_isMsg			bitRead(recv.data[2],7)									// is message, true if 0x80 was set
	#define  recv_isRpt			bitRead(recv.data[2],6)									// is a repeated message, true if 0x40 was set
	#define  recv_ackRq			bitRead(recv.data[2],5)									// ACK requested, true if 0x20 was set
	#define  recv_isCfg			bitRead(recv.data[2],2)									// configuration message, true if 0x04 was set
	#define  recv_toMst			bitRead(recv.data[2],1)									// message to master, true if 0x02 was set

	//- hm communication functions
	void     cc1101Recv_poll(void);
	void     recv_poll(void);															// handles the received string
	void     send_poll(void);															// handles the send queue
	void     send_conf_poll(void);														// handles information requests
	void     send_peer_poll(void);														// handle send events to peers
	void     power_poll(void);															// handles the power modes of the TRX868 module
	void     module_poll(void);															// polling function for the registered modules

	void     hm_enc(uint8_t *buf);														// Ok, encode homematic strings
	void     hm_dec(uint8_t *buf);														// Ok, decode homematic strings
	void     exMsg(uint8_t *buf);														// Ok, explains the send and received messages

	//- receive message handling
	void     recv_PairConfig(void);														// , 01
	void     recv_PairEvent(void);														// , 11
//	void     recv_UpdateEvent(void);													// , 30
	void     recv_PeerEvent(void);														// , >=12
	uint8_t  main_Jump(void);
	uint8_t  module_Jump(uint8_t *by3, uint8_t *by10, uint8_t *by11, uint8_t *cnl, uint8_t *data, uint8_t len);
	
	//- internal send functions
	void     send_prep(uint8_t msgCnt, uint8_t comBits, uint8_t msgType, uint8_t *targetID, uint8_t *payLoad, uint8_t payLen);

	uint8_t  ledMode;

	unsigned long startTime;

	//- to check incoming messages if sender is known
	uint8_t  isPeerKnown(uint8_t *peer);												// , check 3 byte peer against peer database, return 1 if found, otherwise 0
	uint8_t  isPairKnown(uint8_t *pair);												// check 3 byte pair against List0, return 1 if pair is known, otherwise 0

	//- peer specific public functions
	uint8_t  addPeerFromMsg(uint8_t cnl, uint8_t *peer);								// Ok, add a peers to the peer database in the respective channel, returns 0xff on failure, 0 on full and 1 if everything went ok
	uint8_t  remPeerFromMsg(uint8_t cnl, uint8_t *peer);								// Ok, remove a 5 byte peer from peer database in the respective channel, returns 0 on not available and 1 if everything is ok
	uint8_t  valPeerFromMsg(uint8_t *peer);												// Ok, validates a peer, return 0 if not known, 1 if known
	uint8_t  getPeerForMsg(uint8_t cnl, uint8_t *buf);									// Ok, delivers a peer string for a message

	//- regs specific public functions
	uint8_t  getListForMsg2(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf);		// Ok, create the answer of a info request by filling *buf, returns len of buffer, 0 if done and ff on failure
	uint8_t  getListForMsg3(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf);		// not defined yet
	uint8_t  setListFromMsg(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t len, uint8_t *buf); // Ok, writes the register information in *buf to eeprom, 1 if all went ok, 0 on failure
	uint8_t  getRegAddr(uint8_t cnl, uint8_t lst, uint8_t pIdx, uint8_t addr, uint8_t len, uint8_t *buf); // get len reg bytes from specific list by searching for the address byte, returns byte in buffer, 0 if not found and 1 if successfully
	uint8_t  doesListExist(uint8_t cnl, uint8_t lst);									// check if a list exist
	void     getCnlListByPeerIdx(uint8_t cnl, uint8_t peerIdx);							// fill regs struct with respective list3
	void     setListFromModule(uint8_t cnl, uint8_t peerIdx, uint8_t *data, uint8_t len); // write defaults to regs while a peer was added, works only for list3/4

	//- peer specific private functions
	uint8_t  getCnlByPeer(uint8_t *peer);												// OK, find the respective channel of a 4 byte peer in the peer database, returns the channel 1 to x if the peer is known, 0 if unknown
	uint8_t  getIdxByPeer(uint8_t cnl, uint8_t *peer);									// Ok, find the appropriate index of a 4 byte peer in peer database by selecting the channel and searching for the peer, returns peer index, if not found 0xff
	uint8_t  getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer);						// Ok, get the respective 4 byte peer from peer database by selecting channel and index, returns the respective peer in the _tr, function returns 1 if everything is fine, 0 if not
	uint8_t  getFreePeerSlot(uint8_t cnl);												// Ok, search for a free slot in peer database, return the index for a free slot, or 0xff if there is no free slot
	uint8_t  cntFreePeerSlot(uint8_t cnl);												// Ok, count free slots in peer database by channel
	uint8_t  addPeer(uint8_t cnl, uint8_t *peer);										// Ok, add a single peer to database, returns 1 if ok, 0 for failure
	uint8_t  remPeer(uint8_t cnl, uint8_t *peer);										// Ok, remove a peer from peer database

	//- cnlDefType specific functions
	//- cnlDefType specific functions
	uint16_t cdListAddrByCnlLst(uint8_t cnl, uint8_t lst, uint8_t peerIdx);				// Ok, get the eeprom address of register database by index and peerIdx
	uint16_t cdListAddrByPtr(s_cnlDefType *ptr, uint8_t peerIdx);
	s_cnlDefType* cnlDefbyList(uint8_t cnl, uint8_t lst);

	uint16_t cdPeerAddrByCnlIdx(uint8_t cnl, uint8_t peerIdx);							// Ok, get the eeprom address of peers database by index and peerIdx
	s_cnlDefType* cnlDefbyPeer(uint8_t cnl);

	//- pure eeprom handling, i2c to be implemented
	uint8_t  getEeBy(uint16_t addr);
	void     setEeBy(uint16_t addr, uint8_t payload);
	uint16_t getEeWo(uint16_t addr);
	void     setEeWo(uint16_t addr, uint16_t payload);
	uint32_t getEeLo(uint16_t addr);
	void     setEeLo(uint16_t addr, uint32_t payload);
	void     getEeBl(uint16_t addr,uint8_t len,void *ptr);
	void     setEeBl(uint16_t addr,uint8_t len,void *ptr);
	void     clrEeBl(uint16_t addr, uint16_t len);

  private://---------------------------------------------------------------------------------------------------------------
};
extern HM::s_devDef    dDef;															// structure for chDefType and number of elements
extern HM::s_dtRegs    dtRegs;															// number and pointer to array
extern HM::s_eeprom    ee[];															// eeprom definition
extern HM::s_devParm   dParm;															// hm settings
extern HM::s_jumptable jTbl[];															// jump table for calls to the main sketch
extern HM::s_modtable  modTbl[];
extern HM hm;
//- -----------------------------------------------------------------------------------------------------------------------

//- interrupt handling for interface communication module to AskSin library 
struct s_intGDO0 {
	uint8_t nbr;
};

void isrGDO0(void);

ISR( WDT_vect );

#endif
