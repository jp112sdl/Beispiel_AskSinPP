//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Remote Button ---------------------------------------------------------------------------------------------------------
//- with a lot of contribution from Dirk at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------
#ifndef BUTTONS_H
	#define BUTTONS_H

	#if defined(ARDUINO) && ARDUINO >= 100
		#include "Arduino.h"
	#else
		#include "WProgram.h"
	#endif

	#include "AskSinMain.h"
	#include "utility/Serial.h"
	#include "utility/Fastdelegate.h"
	#include "utility/PinChangeIntHandler.h"

	//#define BTN_DBG															// debug message flag
	//#define DM_DBG															// debug message flag

	#define DEBOUNCE_TIME                       10								// debounce tim in ms
	#define BTN_LONG_REPEAT_DELAY              250								// delay between repeated commands at long key press (ms)

	#define BTN_PRESSED                          0
	#define BTN_NOT_PRESSED                      1

	#define BTN_STATE_NONE                       0
	#define BTN_STATE_SHORT_FIRST                1
	#define BTN_STATE_LONG_FIRST                 2

	#define BTN_ACTION_SHORT_SINGLE              0
	#define BTN_ACTION_SHORT_DOUBLE              1
	#define BTN_ACTION_LONG_REPEAT               2
	#define BTN_ACTION_LONG_END                  3
	#define BTN_ACTION_LONG_DOUBLE               4
	#define BTN_ACTION_LONG_DOUBLE_TIMEOUT       5
	#define BTN_ACTION_STAY_AWAKE              255

	#define BTN_TIME_LONGPRESS_CONFIG         5000
	#define BTN_TIME_LONGPRESS_CONFIG_TIMEOUT 5000

	class Buttons {
	  //- user code here ------------------------------------------------------------------------------------------------------
	  public://----------------------------------------------------------------------------------------------------------------
		void     config(uint8_t Pin, void tCallBack(uint8_t, uint8_t));
		void     interrupt(uint8_t btnState);

	  private://---------------------------------------------------------------------------------------------------------------
		struct s_regChanL1 {
			// 0x04,0x08,0x09,
			uint8_t                      :4;     //       l:0, s:4
			uint8_t  longPress           :4;     // 0x04, s:4, e:8
			uint8_t  sign                :1;     // 0x08, s:0, e:1
			uint8_t                      :7;     //       l:7, s:0
			uint8_t  dblPress            :4;     // 0x09, s:0, e:4
			uint8_t                      :4;     //
		} *list1;																// pointer to list1 values

		uint16_t btnShortMaxDoubleTime;											// max time in with a double short key press is detected
		uint16_t btnLongPressTime;												// time key should be pressed to be recognized as a long key press
		uint16_t btnLongDoubleTimeout;											// maximum time between a double key press
		void (*callBack)(uint8_t, uint8_t);										// call back address for key display

		uint8_t  btnHasChanged;													// was there a change happened in key state
		uint8_t  btnStateCurrent;												// current status of the button

		uint8_t  btnStateMachineState;											// current state of button state machine
		uint16_t btnTimer;														// timer for counting times for key presses
		uint32_t btnRepeatTimer;												// timer for counting times for fireing repeated long press events

		uint32_t millisLongDoubleTimeout;
		uint32_t millisShortDoubleTimeout;

		uint32_t btnInterruptTimer;

		void     buttonAction(uint8_t btnEvent);								// function to call from inside of the poll function

	  //- mandatory functions for every new module to communicate within HM protocol stack ------------------------------------
	  public://----------------------------------------------------------------------------------------------------------------
		uint8_t  modStat;														// module status byte, needed for list3 modules to answer status requests
		uint8_t  regCnl;														// holds the channel for the module

		HM       *hm;															// pointer to HM class instance
		uint8_t  *ptrPeerList;													// pointer to list3/4 in regs struct
		uint8_t  *ptrMainList;													// pointer to list0/1 in regs struct

		void     configCngEvent(void);											// list1 on registered channel had changed
		void     pairSetEvent(uint8_t *data, uint8_t len);						// pair message to specific channel, handover information for value, ramp time and so on
		void     pairStatusReq(void);											// event on status request
		void     peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);		// peer message was received on the registered channel, handover the message bytes and length

		void     poll(void);													// poll function, driven by HM loop

		//- predefined, no reason to touch ------------------------------------------------------------------------------------
		void    regInHM(uint8_t cnl, HM *instPtr);								// register this module in HM on the specific channel
		void    hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len); // call back address for HM for informing on events
		void    peerAddEvent(uint8_t *data, uint8_t len);						// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
	};

#endif
