//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Dummy class for own developments --------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
#include "Dummy.h"



//- mandatory functions for every new module to communicate within HM protocol stack --------------------------------------
void Dummy::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef DM_DBG
	Serial << F("configCngEvent\n");
	#endif
}
void Dummy::pairSetEvent(uint8_t *data, uint8_t len) {
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
void Dummy::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef DM_DBG
	Serial << F("pairStatusReq\n");
	#endif
	
	hm->sendInfoActuatorStatus(regCnl, modStat, 0);
}
void Dummy::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef DM_DBG
		Serial << F("peerMsgEvent, type: "); pHexB(type);
		Serial << F(", data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LF);
	#endif
	
	hm->send_ACK();
}

void Dummy::poll(void) {
	// just polling, as the function name said
}

//- predefined, no reason to touch ----------------------------------------------------------------------------------------
void Dummy::regInHM(uint8_t cnl, HM *instPtr) {
	hm = instPtr;																		// set pointer to the HM module
	hm->regCnlModule(cnl,s_mod_dlgt(this,&Dummy::hmEventCol),(uint16_t*)&ptrMainList,(uint16_t*)&ptrPeerList);
	regCnl = cnl;																		// stores the channel we are responsible fore
}
void Dummy::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	if       (by3 == 0x00)                    poll();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}
void Dummy::peerAddEvent(uint8_t *data, uint8_t len) {
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
