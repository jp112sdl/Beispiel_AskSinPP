//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Relay class -----------------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------
#ifndef RELAY_H
#define RELAY_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "AskSinMain.h"
#include "utility/Serial.h"
#include "utility/Helpers.h"
#include "utility/Fastdelegate.h"

//#define DM_DBG																		// debug message flag

class Relay {
  //- user code here ------------------------------------------------------------------------------------------------------
  public://----------------------------------------------------------------------------------------------------------------
	//void     config(uint8_t type, uint8_t pinOn, uint8_t pinOff, uint8_t minDelay, uint8_t randomDelay);
	void     config(void Init(), void Switch(uint8_t), uint8_t minDelay, uint8_t randomDelay);

  protected://-------------------------------------------------------------------------------------------------------------
  private://---------------------------------------------------------------------------------------------------------------
	struct s_srly {
		uint8_t shCtDlyOn           :4;
		uint8_t shCtDlyOff          :4;
		uint8_t shCtOn              :4;
		uint8_t shCtOff             :4;
		uint8_t shCtValLo;
		uint8_t shCtValHi;
		uint8_t shOnDly;
		uint8_t shOnTime;
		uint8_t shOffDly;
		uint8_t shOffTime;
		uint8_t shActionType        :2;
		uint8_t                     :4;
		uint8_t shOffTimeMode       :1;
		uint8_t shOnTimeMode        :1;
		uint8_t shSwJtOn            :4;
		uint8_t shSwJtOff           :4;
		uint8_t shSwJtDlyOn         :4;
		uint8_t shSwJtDlyOff        :4;
		uint8_t lgCtDlyOn           :4;
		uint8_t lgCtDlyOff          :4;
		uint8_t lgCtOn              :4;
		uint8_t lgCtOff             :4;
		uint8_t lgCtValLo;
		uint8_t lgCtValHi;
		uint8_t lgOnDly;
		uint8_t lgOnTime;
		uint8_t lgOffDly;
		uint8_t lgOffTime;
		uint8_t lgActionType        :2;
		uint8_t                     :3;
		uint8_t lgMultiExec         :1;
		uint8_t lgOffTimeMode       :1;
		uint8_t lgOnTimeMode        :1;
		uint8_t lgSwJtOn            :4;
		uint8_t lgSwJtOff           :4;
		uint8_t lgSwJtDlyOn         :4;
		uint8_t lgSwJtDlyOff        :4;
	};

	void (*tInit)(void);
	void (*tSwitch)(uint8_t);
	
	uint8_t hwType:1;																	// 0 indicates a monostable, 1 a bistable relay
	uint8_t hwPin[2];																	// first byte for on, second for off
	uint8_t msgRecvCnt;																	// receive counter to avoid double action on one message
	
	uint8_t curStat:4, nxtStat:4;														// current state and next state
	uint8_t OnDly, OnTime, OffDly, OffTime;												// trigger 40/41 timer variables
	uint8_t lastTrig;																	// store for the last trigger
	uint16_t rTime, dTime;																// trigger 11 timer variables
	uint32_t rlyTime;																	// timer for poll routine

	uint8_t mDel, rDel;																	// store for the call back delay
	uint32_t cbsTme;																	// timer for call back poll routine

	void    trigger11(uint8_t val, uint8_t *rampTime, uint8_t *duraTime);				// FHEM event
	void    trigger41(uint8_t lngIn, uint8_t val);										// sensor event called
	void    trigger40(uint8_t lngIn, uint8_t cnt);										// remote event called
 
	void    adjRly(uint8_t tValue);														// set relay to tValue, 0 is off, 1 is on
	uint8_t getMovStat(void);															// provide movement status byte for ACK enhanced - 40 if status is not set, otherwise 00
	uint8_t getRlyStat(void);															// provide relay status byte for ACK enhanced - 40 if status is not set, otherwise 00

  
  //- mandatory functions for every new module to communicate within HM protocol stack ------------------------------------ 
  public://----------------------------------------------------------------------------------------------------------------
	uint8_t modStat;																	// module status byte, needed for list3 modules to answer status requests
	uint8_t regCnl;																		// holds the channel for the module

	HM      *hm;																		// pointer to HM class instance
	s_srly  *ptrPeerList;																// pointer to list3/4 in regs struct
	//uint8_t *ptrPeerList;																// pointer to list3/4 in regs struct
	uint8_t *ptrMainList;																// pointer to list0/1 in regs struct

	void     configCngEvent(void);														// list1 on registered channel had changed
	void     pairSetEvent(uint8_t *data, uint8_t len);									// pair message to specific channel, handover information for value, ramp time and so on
	void     pairStatusReq(void);														// event on status request
	void     peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);					// peer message was received on the registered channel, handover the message bytes and length

	void     poll(void);																// poll function, driven by HM loop

	//- predefined, no reason to touch ------------------------------------------------------------------------------------
	void     regInHM(uint8_t cnl, HM *instPtr);											// register this module in HM on the specific channel
	void     hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len); // call back address for HM for informing on events
	void     peerAddEvent(uint8_t *data, uint8_t len);									// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
};


#endif
