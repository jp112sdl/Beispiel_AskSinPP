//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Remote Button ---------------------------------------------------------------------------------------------------------
//- with a lot of contribution from Dirk at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------
#include "Buttons.h"

const uint8_t peerOdd[] =    {0};															// default settings for list3 or list4
const uint8_t peerEven[] =   {0};
const uint8_t peerSingle[] = {0};

//- user code here --------------------------------------------------------------------------------------------------------
void Buttons::config(uint8_t Pin, void tCallBack(uint8_t, uint8_t)) {

	// settings while setup
	pinMode(Pin, INPUT_PULLUP);															// setting the pin to input mode
	callBack = tCallBack;																// call back address for button state display

	btnHasChanged = 0;

	// todo: button state at init time
	btnStateCurrent = BTN_NOT_PRESSED;													// btn is connected to gnd, so current state at init time should 1

	registerInt(Pin,s_dlgt(this,&Buttons::interrupt));									// setting the interrupt and port mask

	if (regCnl) {
		list1 = (s_regChanL1*)ptrMainList;												// assign the pointer to our class struct
	} else {
		const uint8_t temp[3] = {0,0,0};
		list1 = (s_regChanL1*)temp;
	}
	configCngEvent();																	// call the config change event for setting up the timing variables
}

void Buttons::interrupt(uint8_t btnState) {
	btnStateCurrent = btnState;															// getting the pin status
	btnInterruptTimer = millis();
	btnHasChanged = 1;																	// jump in poll routine next time
}

/**
 * execute button actions
 *
 * @see Bdefines TN_ACTION_...
 */
void Buttons::buttonAction(uint8_t btnEvent) {
	hm->stayAwake(1000);																// stay awake for 1000ms
	if (btnEvent == BTN_ACTION_STAY_AWAKE) return;										// was only a wake up message

	#ifdef DM_DBG
		Serial << F("i:") << regCnl << F(", s:") << btnEvent << F("\n");				// some debug message
	#endif
	
	if (regCnl == 0) {																	// config button mode
		if (btnEvent == BTN_ACTION_SHORT_SINGLE) {
			hm->startPairing();															// short key press, send pairing string
			hm->stayAwake(20000);														// configuration mode, stay alive for a long time

		} else if (btnEvent == BTN_ACTION_LONG_REPEAT) {
			hm->statusLed.set(STATUSLED_2, STATUSLED_MODE_BLINKFAST);					// long key press could mean, you like to go for reseting the device
			hm->stayAwake(BTN_TIME_LONGPRESS_CONFIG_TIMEOUT + 500);

		} else if (btnEvent == BTN_ACTION_LONG_DOUBLE_TIMEOUT) {
			hm->statusLed.set(STATUSLED_2, STATUSLED_MODE_OFF);							// time out for double long, stop slow blinking

		} else if (btnEvent == BTN_ACTION_LONG_DOUBLE) {
			hm->reset();																// double long key press, reset the device
		}

	} else {
		if ((btnEvent == BTN_ACTION_SHORT_SINGLE) || (btnEvent == BTN_ACTION_SHORT_DOUBLE)) {
			hm->sendPeerREMOTE(regCnl, 0, 0);											// short key or double short key press detected

		} else if (btnEvent == BTN_ACTION_LONG_REPEAT) {
			hm->sendPeerREMOTE(regCnl, 1, 0);											// long or continuous long key press detected

		} else if (btnEvent == BTN_ACTION_LONG_END) {
			hm->sendPeerREMOTE(regCnl, 2, 0);											// end of long or continuous long key press detected
		}
	}
	
	if (callBack) {
		callBack(regCnl, btnEvent);														// call the callback function
	}
}

//- mandatory functions for every new module to communicate within HM protocol stack --------------------------------------
void Buttons::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)

	if (regCnl == 0) {																	// we are in config button mode
		btnShortMaxDoubleTime = 0;
		btnLongPressTime = BTN_TIME_LONGPRESS_CONFIG;									// time key should be pressed to be recognized as a long key press in config mode
		btnLongDoubleTimeout = BTN_TIME_LONGPRESS_CONFIG_TIMEOUT;						// maximum time between a double key press in config mode

	} else {																			// we are in normal button mode
		btnShortMaxDoubleTime = list1->dblPress * 100;									// max time in with a double short key press is detected
		btnLongPressTime = (list1->longPress * 100);									// time the button must pressed to be recognized as a long button press in normal mode
		btnLongDoubleTimeout = 0;														// maximum time between a double key press in normal mode
	}

	#ifdef DM_DBG
		Serial << F("config change; cnl: ") << regCnl << F(", longPress: ") << list1->longPress << F(", sign: ") << list1->sign << F(", dblPress: ") << list1->dblPress  << F("\n");
	#endif
}

void Buttons::pairSetEvent(uint8_t *data, uint8_t len) {
	// we received a message from master to set a new value, typical you will find three bytes in data
	// 1st byte = value; 2nd byte = ramp time; 3rd byte = duration time;
	// after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
	#ifdef DM_DBG
		Serial << F("pairSetEvent, value:"); pHexB(data[0]);
		if (len > 1) Serial << F(", rampTime: "); pHexB(data[1]);
		if (len > 2) Serial << F(", duraTime: "); pHexB(data[2]);
		Serial << F("\n");
	#endif
		
	hm->sendACKStatus(regCnl,modStat,0);
}

void Buttons::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef DM_DBG
	Serial << F("pairStatusReq\n");
	#endif
	
	hm->sendInfoActuatorStatus(regCnl, modStat, 0);
}

void Buttons::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef DM_DBG
		Serial << F("peerMsgEvent, type: "); pHexB(type);
		Serial << F(", data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LF);
	#endif
	
	hm->send_ACK();
}

/**
 * possible events of this function:
 *   0: short key press
 *   1: double short key press
 *   2: long key press
 *   3: repeated long key press
 *   4: end of long key press
 *   5: double long key press
 *   6: time out for double long
 * 255: key press, for stay awake issues
 */
void Buttons::poll(void) {
	unsigned long mills = millis();

	if (btnStateCurrent == BTN_NOT_PRESSED &&
	   btnStateMachineState == BTN_STATE_LONG_FIRST &&
	   (mills - millisLongDoubleTimeout) >= btnLongDoubleTimeout) {						// double long keypress within double timeout expected

		btnStateMachineState = BTN_STATE_NONE;
		buttonAction(BTN_ACTION_LONG_DOUBLE_TIMEOUT);									// fire double timeout action

		#ifdef BTN_DBG
			Serial << F("bt: LONG_DOUBLE_TIMEOUT");	buttonAction(BTN_ACTION_STAY_AWAKE);
		#endif
	}

	if ((!btnHasChanged && !btnTimer) || (mills - btnInterruptTimer) < DEBOUNCE_TIME) {
		return;																			// no need for do any thing
	}

	btnHasChanged = 0;
	#ifdef BTN_DBG
		Serial << F("bt: ") << btnTimer << F("\n");
	#endif

	if (btnStateCurrent == BTN_PRESSED) {												// detect key press
		btnTimer = mills - btnInterruptTimer ;											// start button timer

		if (btnTimer >= btnLongPressTime) {												// button timer detect long key press
			if (btnStateMachineState == BTN_STATE_LONG_FIRST) {							// was there a long key press before
				btnStateMachineState = BTN_STATE_NONE;
				btnTimer = 0;
				buttonAction(BTN_ACTION_LONG_DOUBLE);									// fire double long action

				#ifdef BTN_DBG
					Serial << F("bt: LONG_DOUBLE");
				#endif

			} else {																	// long repeated key press detected
				millisLongDoubleTimeout = mills;
				if (btnRepeatTimer == 0) {
					btnRepeatTimer = mills;
				} else {
					if ((mills - btnRepeatTimer) >= BTN_LONG_REPEAT_DELAY) {
						buttonAction(BTN_ACTION_LONG_REPEAT);							// fire double repeat action
						btnRepeatTimer = mills;

						#ifdef BTN_DBG
							Serial << F("bt: LONG_REPEAT");
						#endif
					}
				}
			}
		}
		buttonAction(BTN_ACTION_STAY_AWAKE);											// while a key was pressed fire stay awake action

	} else if (btnStateCurrent == BTN_NOT_PRESSED && btnTimer > 0) {					// detect key release
		btnRepeatTimer = 0;

		if (btnStateMachineState == BTN_STATE_LONG_FIRST) {								// was there a long key press before
			btnStateMachineState = BTN_STATE_NONE;
			buttonAction(BTN_ACTION_LONG_DOUBLE_TIMEOUT);								// we fire double timeout action

			#ifdef BTN_DBG
				Serial << F("bt: LONG_DOUBLE_TIMEOUT");
			#endif

		} else {
			if (btnTimer >= btnLongPressTime) {											// check button timer after key release
				if (btnLongDoubleTimeout > 0) {											// if btnLongDoubleTimeout set? we in config mode
					btnStateMachineState = BTN_STATE_LONG_FIRST;
				} else {
					buttonAction(BTN_ACTION_LONG_END);									// fire long end action

					#ifdef BTN_DBG
						Serial << F("bt: LONG_END");
					#endif
				}

			} else if (btnTimer >= 0) {													// is there a running button timer?
				if (btnStateMachineState == BTN_STATE_SHORT_FIRST &&					// there a short key press before and
				   (mills - millisShortDoubleTimeout) < btnShortMaxDoubleTime) {		// second short key press is within btnShortMaxDoubleTime

					btnStateMachineState = BTN_STATE_NONE;
					buttonAction(BTN_ACTION_SHORT_DOUBLE);								// double short key press

					#ifdef BTN_DBG
						Serial << F("bt: SHORT_DOUBLE");
					#endif

				} else {
					btnStateMachineState = BTN_STATE_SHORT_FIRST;
					millisShortDoubleTimeout = mills;
					if (btnShortMaxDoubleTime == 0) {									// single action only available if btnShortMaxDoubleTime not set
						buttonAction(BTN_ACTION_SHORT_SINGLE);							// fire single short action

						#ifdef BTN_DBG
							Serial << F("bt: SHORT_SINGLE");
						#endif
					}
				}
			}
		}

		btnTimer = 0;
	}

	#ifdef BTN_DBG
		_delay_ms(150);
	#endif
}

//- pre defined, no reason to touch ---------------------------------------------------------------------------------------
void Buttons::regInHM(uint8_t cnl, HM *instPtr) {
	hm = instPtr;																		// set pointer to the HM module
	hm->regCnlModule(cnl,s_mod_dlgt(this,&Buttons::hmEventCol),(uint16_t*)&ptrMainList,(uint16_t*)&ptrPeerList);
	regCnl = cnl;																		// stores the channel we are responsible fore
}
void Buttons::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	if       (by3 == 0x00)                    poll();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}
void Buttons::peerAddEvent(uint8_t *data, uint8_t len) {
	// we received an peer add event, which means, there was a peer added in this respective channel
	// 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
	// no need for sending an answer, but we could set default data to the respective list3/4
	#ifdef DM_DBG
		Serial << F("peerAddEvent: pCnl1: "); pHexB(data[0]);
		Serial << F(", pCnl2: "); pHexB(data[1]);
		Serial << F(", pIdx1: "); pHexB(data[2]);
		Serial << F(", pIdx2: "); pHexB(data[3]);
		Serial << F("\n");
	#endif
	
	if ((data[0]) && (data[1])) {																		// dual peer add
		if (data[0]%2) {																				// odd
			hm->setListFromModule(regCnl,data[2],(uint8_t*)peerOdd,sizeof(peerOdd));
			hm->setListFromModule(regCnl,data[3],(uint8_t*)peerEven,sizeof(peerEven));
		} else {																						// even
			hm->setListFromModule(regCnl,data[2],(uint8_t*)peerEven,sizeof(peerEven));
			hm->setListFromModule(regCnl,data[3],(uint8_t*)peerOdd,sizeof(peerOdd));
		}
	} else {																							// single peer add
		if (data[0]) hm->setListFromModule(regCnl,data[2],(uint8_t*)peerSingle,sizeof(peerSingle));
		if (data[1]) hm->setListFromModule(regCnl,data[3],(uint8_t*)peerSingle,sizeof(peerSingle));
	}
	
}
