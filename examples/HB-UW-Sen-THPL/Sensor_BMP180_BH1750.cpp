/**
   SHT10, BMP085, BH1750 Sensor class

   (c) 2014 Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
   Author: Horst Bittner <horst@diebittners.de>
           Dirk Hoffmann dirk@fhem.forum
*/
#include "Sensor_BMP180_BH1750.h"
#include <Wire.h>

//- user code here --------------------------------------------------------------------------------------------------------
/**
   @params	data	pointer to data port pin
   @params	sck		pointer to sck port pin
   @params	timing	0: use send slot: >0: ms send interval
   @params	pPtr	Pointer so BMP180 class
   @params	lPtr	Pointer so BH1750 class
*/
void BMP180_BH1750::config(uint8_t data, uint8_t sck, uint16_t timing, BMP085 *pPtr, BH1750 *lPtr) {
  tTiming = timing;
  nTime = 1000;																// set the first time we like to measure
  startTime = millis();

  nAction = BMP180_BH1750_nACTION_MEASURE_INIT;
  bm180 = pPtr;

  bh1750 = lPtr;
  bh1750->begin();
}

void BMP180_BH1750::poll_transmit(void) {
  if (tTiming) {
    nTime = tTiming;																// there is a given timing
  } else {
    nTime = (calcSendSlot() * 250) - BMP180_BH1750_MAX_MEASURE_TIME; 		// calculate the next send slot by multiplying with 250ms to get the time in millis
  }
  startTime = millis();

  hm->sendPeerWEATHER(regCnl, tTemp, tHum, tPres, tLux);								// send out the weather event

  nAction = BMP180_BH1750_nACTION_MEASURE_INIT;								// next time we want to measure again
}

uint32_t BMP180_BH1750::calcSendSlot(void) {
  uint32_t result = (((hm->getHMID() << 8) | hm->getMsgCnt()) * 1103515245 + 12345) >> 16;
  return (result & 0xFF) + BMP180_BH1750_MINIMAL_CYCLE_LENGTH;
}

//- mandatory functions for every new module to communicate within HM protocol stack --------------------------------------
void BMP180_BH1750::configCngEvent(void) {
  // it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
#ifdef DM_DBG
  Serial << F("configCngEvent\n");
#endif
}

void BMP180_BH1750::pairSetEvent(uint8_t *data, uint8_t len) {
  // we received a message from master to set a new value, typical you will find three bytes in data
  // 1st byte = value; 2nd byte = ramp time; 3rd byte = duration time;
  // after setting the new value we have to send an enhanced ACK (<- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54)
#ifdef DM_DBG
  Serial << F("pairSetEvent, value:"); pHexB(data[0]);
  if (len > 1) Serial << F(", rampTime: "); pHexB(data[1]);
  if (len > 2) Serial << F(", duraTime: "); pHexB(data[2]);
  Serial << F("\n");
#endif

  hm->sendACKStatus(regCnl, modStat, 0);
}

void BMP180_BH1750::pairStatusReq(void) {
  // we received a status request, appropriate answer is an InfoActuatorStatus message
#ifdef DM_DBG
  Serial << F("pairStatusReq\n");
#endif

  hm->sendInfoActuatorStatus(regCnl, modStat, 0);
}

void BMP180_BH1750::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
  // we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
  // appropriate answer is an ACK
#ifdef DM_DBG
  Serial << F("peerMsgEvent, type: "); pHexB(type);
  Serial << F(", data: "); pHex(data, len, SERIAL_DBG_PHEX_MODE_LF);
#endif

  hm->send_ACK();
}

void BMP180_BH1750::poll(void) {
  unsigned long mills = millis();

  // just polling, as the function name said
  if (nTime == 0 || (mills - startTime < nTime)) {								// check if it is time to jump in
    return;
  }

  nTime = BMP180_BH1750_MAX_MEASURE_TIME;
  startTime = mills;

  //Serial.print("nAction: "); Serial.println(nAction); _delay_ms(50);

  if (nAction == BMP180_BH1750_nACTION_MEASURE_INIT) {
    nAction = BMP180_BH1750_nACTION_MEASURE_L;
  } else if (nAction == BMP180_BH1750_nACTION_MEASURE_L) {
    poll_measureCalcLight();
  } else if (nAction == BMP180_BH1750_nACTION_CALC_L) {
    poll_measureCalcLight();
  } else if (nAction == BMP180_BH1750_nACTION_MEASURE_THP) {
    poll_measureTHP();
  } else if (nAction == BMP180_BH1750_nACTION_TRANSMIT) {
    poll_transmit();													// transmit
  }
}

void BMP180_BH1750::poll_measureCalcLight(void) {
  // read light data
  double lux = bh1750->readLightLevel();


  boolean luxValid = (lux != 54612);
  tLux = ((luxValid) ? lux : 65535) * 100;

  //Serial.print("Lux: "); Serial.println(tLux);
  //	_delay_ms (50);

  nAction = BMP180_BH1750_nACTION_MEASURE_THP;
}

void BMP180_BH1750::poll_measureTHP(void) {
  // Disable I2C

  uint16_t rawData;

  if (bm180 != NULL) {														// only if we have a valid module
    bm180->begin(BMP085_ULTRAHIGHRES);	// BMP085_ULTRALOWPOWER, BMP085_STANDARD, BMP085_HIGHRES, BMP085_ULTRAHIGHRES

    // simple barometric formula
    tPres = (uint16_t)((bm180->readPressure() / 10) + (tAltitude / 0.85));
    //Serial << F("tPres: ") << tPres << F("\n");

    if (tPres > 300) {
      // get temperature from bmp180 if sht10 no present

      tTemp = bm180->readTemperature() * 10;
    } else {
      tPres = 0;
    }
  }

  nAction = BMP180_BH1750_nACTION_TRANSMIT;							// next time we would send the data
}

//- predefined, no reason to touch ----------------------------------------------------------------------------------------
void BMP180_BH1750::regInHM(uint8_t cnl, HM *instPtr) {
  hm = instPtr;																		// set pointer to the HM module
  hm->regCnlModule(cnl, s_mod_dlgt(this, &BMP180_BH1750::hmEventCol), (uint16_t*)&ptrMainList, (uint16_t*)&ptrPeerList);
  regCnl = cnl;																		// stores the channel we are responsible fore
}

void BMP180_BH1750::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
  if       (by3 == 0x00)                    poll();
  else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
  else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
  else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
  else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
  else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
  else return;
}

void BMP180_BH1750::peerAddEvent(uint8_t *data, uint8_t len) {
  // we received an peer add event, which means, there was a peer added in this respective channel
  // 1st byte and 2nd byte shows the peer channel, 3rd and 4th byte gives the peer index
  // no need for sending an answer, but we could set default data to the respective list3/4
#ifdef DM_DBG
  Serial << F("peerAddEvent: pCnl1: "); pHexB(data[0]);
  Serial << F(", pCnl2: "); pHexB(data[1]);
  Serial << F(", pIdx1: "); pHexB(data[2]);
  Serial << F(", pIdx2: "); pHexB(data[3], SERIAL_DBG_PHEX_MODE_LF);
#endif

  if ((data[0]) && (data[1])) {																		// dual peer add
    if (data[0] % 2) {																				// odd
      hm->setListFromModule(regCnl, data[2], (uint8_t*)peerOdd, sizeof(peerOdd));
      hm->setListFromModule(regCnl, data[3], (uint8_t*)peerEven, sizeof(peerEven));
    } else {																						// even
      hm->setListFromModule(regCnl, data[2], (uint8_t*)peerEven, sizeof(peerEven));
      hm->setListFromModule(regCnl, data[3], (uint8_t*)peerOdd, sizeof(peerOdd));
    }
  } else {																							// single peer add
    if (data[0]) hm->setListFromModule(regCnl, data[2], (uint8_t*)peerSingle, sizeof(peerSingle));
    if (data[1]) hm->setListFromModule(regCnl, data[3], (uint8_t*)peerSingle, sizeof(peerSingle));
  }

}

void BMP180_BH1750::setAltitude(int16_t altitude) {
  tAltitude = altitude;
}
