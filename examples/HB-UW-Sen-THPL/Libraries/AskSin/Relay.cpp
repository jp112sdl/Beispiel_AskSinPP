//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Relay class -----------------------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------
#include "Relay.h"

// default settings for list3 or list4
const uint8_t peerOdd[] =    {
	// Default actor dual 1: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01 	
	// 0B:64 0C:66 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:64 8C:66 
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x64, 0x66, 
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x64, 0x66
};
const uint8_t peerEven[] =   {
	// Default actor dual 2: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01 
	// 0B:13 0C:33 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:13 8C:33 
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x13, 0x33, 
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x13, 0x33
};
const uint8_t peerSingle[] = {
	// Default actor single: 02:00 03:00 04:32 05:64 06:00 07:FF 08:00 09:FF 0A:01 
	// 0B:14 0C:63 82:00 83:00 84:32 85:64 86:00 87:FF 88:00 89:FF 8A:21 8B:14 8C:63 
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x01, 0x14, 0x63, 
	0x00, 0x00, 0x32, 0x64, 0x00, 0xFF, 0x00, 0xFF, 0x21, 0x14, 0x63,
};


//- user code here --------------------------------------------------------------------------------------------------------
// public function for setting the module
//void Relay::config(uint8_t type, uint8_t pinOn, uint8_t pinOff, uint8_t minDelay, uint8_t randomDelay) {
void Relay::config(void Init(), void Switch(uint8_t), uint8_t minDelay, uint8_t randomDelay) {
	// store config settings in class
	//hwType = type;																// 0 indicates a monostable, 1 a bistable relay
	//hwPin[0] = pinOn;															// first byte for on, second for off
	//hwPin[1] = pinOff;

	tInit = Init;
	tSwitch = Switch;

	mDel = minDelay;															// remember minimum delay for sending the status
	rDel = (randomDelay)?randomDelay:1;											// remember random delay for sending the status

	// set output pins
	tInit();

	// some basic settings for start
	nxtStat = 6;																// set relay status to off
	adjRly(0);																	// set relay to a defined status
	curStat = 6;																// set relay status to off
}

// private functions for triggering some action
void Relay::trigger11(uint8_t val, uint8_t *rampTime, uint8_t *duraTime) {
	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}

	rTime = (uint16_t)rampTime[0]<<8 | (uint16_t)rampTime[1];					// store ramp time
	dTime = (duraTime)?((uint16_t)duraTime[0]<<8 | (uint16_t)duraTime[1]):0;	// duration time if given
	
	if (rTime) nxtStat = (val == 0)?4:1;										// set next status
	else nxtStat = (val == 0)?6:3;

	lastTrig = 11;																// remember the trigger
	rlyTime = millis();															// changed some timers, activate poll function

	#ifdef DM_DBG
		Serial << F("RL:trigger11, val:") << val << F(", nxtS:") << nxtStat << F(", rampT:") << rTime << F(", duraT:") << dTime << F("\n");
	#endif

	hm->sendACKStatus(regCnl,getRlyStat(),getMovStat());						// send an status ACK
}
void Relay::trigger40(uint8_t lngIn, uint8_t cnt) {
	s_srly *s3 = ptrPeerList;													// copy list3 to pointer

	// check for repeated message
	if ((lngIn) && (s3->lgMultiExec == 0) && (cnt == msgRecvCnt)) return;		// trigger was long
	if ((lngIn == 0) && (cnt == msgRecvCnt)) return;							// repeated instruction
	msgRecvCnt = cnt;															// remember message counter

	// fill the respective variables
	uint8_t actTp = (lngIn)?s3->lgActionType:s3->shActionType;					// get actTp = {off=>0,jmpToTarget=>1,toggleToCnt=>2,toggleToCntInv=>3}

	if (actTp == 0) {															// off

	} else if ((actTp == 1) && (lngIn == 1)) {									// jmpToTarget
		// SwJtOn {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
		if      (curStat == 6) nxtStat = s3->lgSwJtOff;							// currently off
		else if (curStat == 3) nxtStat = s3->lgSwJtOn;							// on
		else if (curStat == 4) nxtStat = s3->lgSwJtDlyOff;						// delay off
		else if (curStat == 1) nxtStat = s3->lgSwJtDlyOn;						// delay on
		OnDly   = s3->lgOnDly;													// set timers
		OnTime  = s3->lgOnTime;
		OffDly  = s3->lgOffDly;
		OffTime = s3->lgOffTime;

	} else if ((actTp == 1) && (lngIn == 0)) {									// jmpToTarget
		if      (curStat == 6) nxtStat = s3->shSwJtOff;							// currently off
		else if (curStat == 3) nxtStat = s3->shSwJtOn;							// on
		else if (curStat == 4) nxtStat = s3->shSwJtDlyOff;						// delay off
		else if (curStat == 1) nxtStat = s3->shSwJtDlyOn;						// delay on
		OnDly   = s3->shOnDly;													// set timers
		OnTime  = s3->shOnTime;
		OffDly  = s3->shOffDly;
		OffTime = s3->shOffTime;

	} else if (actTp == 2) {													// toogleToCnt, if tCnt is even, then next state is on
		nxtStat = (cnt % 2 == 0)?3:6;											// even - relay dlyOn, otherwise dlyOff
		OnDly   = 0; OnTime  = 255; OffDly  = 0; OffTime = 255;					// set timers

	} else if (actTp == 3) {													// toggleToCntInv, if tCnt is even, then next state is off, while inverted
		nxtStat = (cnt % 2 == 0)?6:3;											// even - relay dlyOff, otherwise dlyOn
		OnDly   = 0; OnTime  = 255; OffDly  = 0; OffTime = 255;					// set timers
	}
	lastTrig = 40;																// set trigger
	rlyTime = millis();															// changed some timers, activate poll function

	#ifdef DM_DBG
		Serial << F("RL:trigger40, curS:") << curStat << F(", nxtS:") << nxtStat << F(", OnDly:") << OnDly << F(", OnTime:") << OnTime << F(", OffDly:") << OffDly << F(", OffTime:") << OffTime << F("\n");
	#endif

	hm->sendACKStatus(regCnl,getRlyStat(),getMovStat());
}
void Relay::trigger41(uint8_t lngIn, uint8_t val) {
	lastTrig = 41;																// set trigger
	rlyTime = millis();															// changed some timers, activate poll function
}


// private functions for setting relay and getting current status
void Relay::adjRly(uint8_t tValue) {
	if (curStat == nxtStat) return;												// nothing to do
	tSwitch(tValue);
	
	modStat = (tValue)?0xc8:0x00;

	#ifdef DM_DBG
		Serial << F("RL:adjRly, curS:") << curStat << F(", nxtS:") << nxtStat << F("\n");
	#endif

	cbsTme = millis() + ((uint32_t)mDel*1000) + random(((uint32_t)rDel*1000));	// set the timer for sending the status
	//Serial << "cbsT:" << cbsTme << F("\n");
}
uint8_t Relay::getMovStat(void) {
	// curStat could be {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if ((nxtStat == 1) || (nxtStat == 4)) return 0x40;
	else return 0x00;
}
uint8_t Relay::getRlyStat(void) {
	// curStat could be {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if ((nxtStat == 6) || (nxtStat == 1)) return 0x00;
	else return 0xC8;
}


//- mandatory functions for every new module to communicate within HM protocol stack --------------------------------------
void Relay::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef DM_DBG
	Serial << F("configCngEvent\n");
	#endif
}
void Relay::pairSetEvent(uint8_t *data, uint8_t len) {
	// we received a message from master to set a new value, typical you will find three bytes in data
	// 1st byte = value; 2nd byte = ramp time; 3rd byte = duration time;
	// after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
	#ifdef DM_DBG
		Serial << F("pairSetEvent, value:"); pHexB(data[0]);
		if (len > 1) Serial << F(", rampTime: "); pHexB(data[1]);
		if (len > 3) Serial << F(", duraTime: "); pHexB(data[3]);
		Serial << F("\n");
	#endif
	
	//hm->sendACKStatus(regCnl,modStat,0);
}
void Relay::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef DM_DBG
	Serial << F("pairStatusReq\n");
	#endif
	
	hm->sendInfoActuatorStatus(regCnl, getRlyStat(), getMovStat());
}
void Relay::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef DM_DBG
		Serial << F("peerMsgEvent, type: "); pHexB(type)
		Serial << F(", data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LF);
	#endif
	
	//hm->send_ACK();
}

void Relay::poll(void) {
	// just polling, as the function name said
	if ((cbsTme != 0) && (millis() > cbsTme)) {									// check if we have to send a status message
		hm->sendInfoActuatorStatus(regCnl, getRlyStat(), 0);					// send status
		cbsTme = 0;																// nothing to do any more
	}

	// relay timer functions
	if ((rlyTime == 0) || (rlyTime > millis())) return;							// timer set to 0 or time for action not reached, leave
	rlyTime = 0;																// freeze per default

	// set relay - {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (nxtStat == 3) {															// set relay on
		adjRly(1); curStat = 3;													// adjust relay, status will send from adjRly()

	} else if (nxtStat == 6) {													// set relay off
		adjRly(0); curStat = 6;													// adjust relay, status will send from adjRly()
	}

	// adjust nxtStat for trigger11 - {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (lastTrig == 11) {
		if (nxtStat == 1) {														// dlyOn -> on
			nxtStat = 3;														// next status is on
			rlyTime = millis() + intTimeCvt(rTime);								// set respective timer
			
		} else if ((nxtStat == 3) && (dTime > 0)) {								// on - > off
			nxtStat = 6;														// next status is off
			rlyTime = millis() + intTimeCvt(dTime);								// set the respective timer
		}
	}

	// adjust nxtStat for trigger40 - {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (lastTrig == 40) {
		if        (nxtStat == 1) {
			nxtStat = 3;
			rlyTime = millis() + byteTimeCvt(OnDly);

		} else if ((nxtStat == 3) && (OnTime < 255)) {
			nxtStat = 4;
			if (OnTime) rlyTime = millis() + byteTimeCvt(OnTime);

		} else if (nxtStat == 4) {
			nxtStat = 6;
			rlyTime = millis() + byteTimeCvt(OffDly);

		} else if ((nxtStat == 6) && (OffTime < 255)) {
			nxtStat = 1;
			if (OffTime) rlyTime = millis() + byteTimeCvt(OffTime);
		}
	}

	//cbM(cnlAss, curStat, nxtStat);

}

//- predefined, no reason to touch ----------------------------------------------------------------------------------------
void Relay::regInHM(uint8_t cnl, HM *instPtr) {
	hm = instPtr;																		// set pointer to the HM module
	hm->regCnlModule(cnl,s_mod_dlgt(this,&Relay::hmEventCol),(uint16_t*)&ptrMainList,(uint16_t*)&ptrPeerList);
	regCnl = cnl;																		// stores the channel we are responsible fore
}
void Relay::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	if  (by3 == 0x00)                    poll();
	if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);

	if ((by3 == 0x11) && (by10 == 0x02)) trigger11(data[0], &data[1], &data[3]);	
	if  (by3 == 0x40)                    trigger40((by10 & 0x40), data[0]);
}
void Relay::peerAddEvent(uint8_t *data, uint8_t len) {
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
