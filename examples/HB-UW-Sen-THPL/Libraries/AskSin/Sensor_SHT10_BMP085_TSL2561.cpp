/**
 * SHT10, BMP085, TSL2561 Sensor class
 *
 * (c) 2014 Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
 * Author: Horst Bittner <horst@diebittners.de>
 *         Dirk Hoffmann dirk@fhem.forum
 */
#include "Sensor_SHT10_BMP085_TSL2561.h"
#include <Wire.h>

//- user code here --------------------------------------------------------------------------------------------------------
/**
 * @params	data	pointer to data port pin
 * @params	sck		pointer to sck port pin
 * @params	timing	0: use send slot: >0: ms send interval
 * @params	tPtr	Pointer so SHT10 class
 * @params	pPtr	Pointer so BMP180 class
 * @params	lPtr	Pointer so TSL2561 class
 */
void SHT10_BMP085_TSL2561::config(uint8_t data, uint8_t sck, uint16_t timing, Sensirion *tPtr, BMP085 *pPtr, TSL2561 *lPtr) {
	tTiming = timing;
	nTime = 1000;																// set the first time we like to measure
	startTime = millis();

	nAction = SHT10_BMP085_TSL2561_nACTION_MEASURE_INIT;
	tsl2561InitCount = 0;

	sht10 = tPtr;
	//sht10->config(data,sck);													// configure the sensor
	sht10->writeSR(LOW_RES);													// low resolution is enough

	if (pPtr != NULL) {															// only if there is a valid module defined
		bm180 = pPtr;
	}

	if (lPtr != NULL) {															// only if there is a valid module defined
		tsl2561 = lPtr;

		tsl2561->begin(TSL2561_ADDR_0);

		// Check if tsl2561 available
		if (tsl2561->setPowerUp()) {

			// config tsl2561 interrupt pin
			pinMode(A0, INPUT_PULLUP);													// setting the pin to input mode
			registerInt(A0,s_dlgt(this,&SHT10_BMP085_TSL2561::tsl2561_ISR));			// setting the interrupt and port mask
		}
	}
}

void SHT10_BMP085_TSL2561::poll_transmit(void) {
	if (tTiming) {
		nTime = tTiming;																// there is a given timing
	} else {
		nTime = (calcSendSlot() * 250) - SHT10_BMP085_TSL2561_MAX_MEASURE_TIME; 		// calculate the next send slot by multiplying with 250ms to get the time in millis
	}
	startTime = millis();

	hm->sendPeerWEATHER(regCnl, tTemp, tHum, tPres, tLux);								// send out the weather event

	nAction = SHT10_BMP085_TSL2561_nACTION_MEASURE_INIT;								// next time we want to measure again
}

uint32_t SHT10_BMP085_TSL2561::calcSendSlot(void) {
	uint32_t result = (((hm->getHMID() << 8) | hm->getMsgCnt()) * 1103515245 + 12345) >> 16;
	return (result & 0xFF) + SHT10_BMP085_TSL2561_MINIMAL_CYCLE_LENGTH;
}

//- mandatory functions for every new module to communicate within HM protocol stack --------------------------------------
void SHT10_BMP085_TSL2561::configCngEvent(void) {
	// it's only for information purpose while something in the channel config was changed (List0/1 or List3/4)
	#ifdef DM_DBG
		Serial << F("configCngEvent\n");
	#endif
}

void SHT10_BMP085_TSL2561::pairSetEvent(uint8_t *data, uint8_t len) {
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

void SHT10_BMP085_TSL2561::pairStatusReq(void) {
	// we received a status request, appropriate answer is an InfoActuatorStatus message
	#ifdef DM_DBG
		Serial << F("pairStatusReq\n");
	#endif

	hm->sendInfoActuatorStatus(regCnl, modStat, 0);
}

void SHT10_BMP085_TSL2561::peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len) {
	// we received a peer event, in type you will find the marker if it was a switch(3E), remote(40) or sensor(41) event
	// appropriate answer is an ACK
	#ifdef DM_DBG
		Serial << F("peerMsgEvent, type: "); pHexB(type);
		Serial << F(", data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LF);
	#endif

	hm->send_ACK();
}

void SHT10_BMP085_TSL2561::poll(void) {
	if (tsl2561IntFlag) {
		tsl2561IntFlag = 0;
		if (nAction == SHT10_BMP085_TSL2561_nACTION_MEASURE_L) {
			nAction = SHT10_BMP085_TSL2561_nACTION_CALC_L;
		}

/*
		tsl2561->getData(tsl2561Data0, tsl2561Data1);

		uint16_t low = tsl2561Data0;
		uint16_t heigh = tsl2561Data0;
		low = (low > 300) ? (low -300) : low;
		heigh = (heigh < 65000) ? (heigh + 300) : heigh;

		Serial.print(low); Serial.print(" < "); Serial.print(tsl2561Data0); Serial.print(" < "); Serial.println(heigh);
		_delay_ms(50);

		// Setup interrupt pesistence fo rinterrupt driven light detection
		tsl2561->setInterruptThreshold(low, heigh);
		tsl2561->setInterruptControl(TSL2561_INTERRUPT_CONTROL_LEVEL, TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_1);
*/
		// reset the interrupt line
		tsl2561->clearInterrupt();
	}

	unsigned long mills = millis();

	// just polling, as the function name said
	if (nTime == 0 || (mills - startTime < nTime)) {								// check if it is time to jump in
		return;
	}

	nTime = SHT10_BMP085_TSL2561_MAX_MEASURE_TIME;
	startTime = mills;

	//Serial.print("nAction: "); Serial.println(nAction); _delay_ms(50);

	#ifdef US_100
		poll_measureTHP();

		tLux = us100Measuer();
		Serial.print("Distance: "); Serial.print(tLux); Serial.println(" mm");

		poll_transmit();													// transmit

	#else
		if (nAction == SHT10_BMP085_TSL2561_nACTION_MEASURE_INIT) {

			if (poll_measureLightInit()){
				nAction = SHT10_BMP085_TSL2561_nACTION_MEASURE_L;

			} else {
				tLux = 6553800;														// send 65538 Lux if no sensor available
				nAction = SHT10_BMP085_TSL2561_nACTION_MEASURE_THP;
			}

		} else if (nAction == SHT10_BMP085_TSL2561_nACTION_MEASURE_L) {
			// next nTime should set via tsl2561_ISR only

		} else if (nAction == SHT10_BMP085_TSL2561_nACTION_CALC_L) {
			poll_measureCalcLight();

		} else if (nAction == SHT10_BMP085_TSL2561_nACTION_MEASURE_THP) {
			poll_measureTHP();

		} else if (nAction == SHT10_BMP085_TSL2561_nACTION_TRANSMIT) {
			poll_transmit();													// transmit
		}
#endif

}

#ifdef US_100
	uint32_t SHT10_BMP085_TSL2561::us100Measuer() {
		unsigned long echoTime   = 0;
		uint32_t distanceMM = 0;

		digitalWrite(US_100_PIN_VCC, HIGH);										// power on the US-100
		_delay_ms(500);

		digitalWrite(US_100_PIN_TRIGGER, HIGH);									// Send pulses begin by trigger pin
		_delay_us(50);															// Set the pulse width of 50us
		digitalWrite(US_100_PIN_TRIGGER, LOW);									// The end of the pulse and start measure

		echoTime = pulseIn(US_100_PIN_ECHO, HIGH);								// A pulse width calculating US-100 returned
		if((echoTime < 60000) && (echoTime > 1)) {								// Pulse effective range (1, 60000).

			// distanceMM = (echoTime * 0.34mm/us) / 2 (mm)
			distanceMM = (echoTime*34/100)/2;                   // Calculating the distance by a pulse width.

			// debug output
		}

		digitalWrite(US_100_PIN_VCC, LOW);										// power off the US-100
		_delay_ms(50);

		return distanceMM;
	}
#endif

uint8_t SHT10_BMP085_TSL2561::poll_measureLightInit() {
	// check if TSL2561 available
	uint8_t initOk = tsl2561->setPowerUp();

	if (initOk) {
		tsl2561->setInterruptControl(TSL2561_INTERRUPT_CONTROL_LEVEL, TSL2561_INTERRUPT_PSELECT_EVERY_ADC);

		uint8_t gain = 0;
		uint8_t integrationTime = INTEGATION_TIME_14;

		if (tsl2561InitCount == 0) {
			// first set to low sensity

			tsl2561->clearInterrupt();
			tsl2561Data0 = 0;
			tsl2561Data1 = 0;

		} else {

			if ((tsl2561Data0 < 1000) && (tsl2561Data1 < 1000)) {
				gain = ((tsl2561Data0 < 100) && (tsl2561Data1 < 100)) ? true : false;
				integrationTime = INTEGATION_TIME_402;
			} else {
				integrationTime = INTEGATION_TIME_101;
			}
		}

		tsl2561->setTiming(gain, integrationTime);
	}

	return initOk;
}

void SHT10_BMP085_TSL2561::poll_measureCalcLight(void) {
	// read light data
	tsl2561->getData(tsl2561Data0, tsl2561Data1);

//	Serial.print("tsl2561InitCount :"); Serial.println(tsl2561InitCount);
//	Serial.print("data0 :"); Serial.print(tsl2561Data0); Serial.print(", data1 :"); Serial.println(tsl2561Data1);
//	_delay_ms (50);

	if (tsl2561InitCount == 0 && tsl2561Data0 < 2000 && tsl2561Data1 < 2000) {
		// we mesure again
		nAction = SHT10_BMP085_TSL2561_nACTION_MEASURE_INIT;
		tsl2561InitCount++;

	} else {
		double lux = 0;
		boolean luxValid = tsl2561->getLux(tsl2561Data0, tsl2561Data1, lux);
		tLux = ((luxValid) ? lux : 65535) * 100;

//		Serial.print("Lux: "); Serial.println(tLux);
//		_delay_ms (50);

		nAction = SHT10_BMP085_TSL2561_nACTION_MEASURE_THP;
	}

	tsl2561->setPowerDown();
}

void SHT10_BMP085_TSL2561::poll_measureTHP(void) {
	// Disable I2C
	TWCR = 0;

	uint16_t rawData;
	uint8_t shtError = sht10->measTemp(&rawData);

	// Measure temperature and humidity from Sensor only if no error
	if (!shtError) {
		float temp = sht10->calcTemp(rawData);
		tTemp = temp * 10;

		sht10->measHumi(&rawData);
		tHum = sht10->calcHumi(rawData, temp);
		//	Serial << F("raw: ") << rawData << F("  mH: ") << tHum << (F("\n");
	}

	if (bm180 != NULL) {														// only if we have a valid module
		bm180->begin(BMP085_ULTRAHIGHRES);	// BMP085_ULTRALOWPOWER, BMP085_STANDARD, BMP085_HIGHRES, BMP085_ULTRAHIGHRES

		// simple barometric formula
		tPres = (uint16_t)((bm180->readPressure() / 10) + (tAltitude / 0.85));
		//Serial << F("tPres: ") << tPres << F("\n");

		if (tPres > 300) {
			// get temperature from bmp180 if sht10 no present
			if (shtError) {
				// temp from BMP180
				tTemp = bm180->readTemperature() * 10;
			}
		} else {
			tPres = 0;
		}
	}

	nAction = SHT10_BMP085_TSL2561_nACTION_TRANSMIT;							// next time we would send the data
}

//- predefined, no reason to touch ----------------------------------------------------------------------------------------
void SHT10_BMP085_TSL2561::regInHM(uint8_t cnl, HM *instPtr) {
	hm = instPtr;																		// set pointer to the HM module
	hm->regCnlModule(cnl,s_mod_dlgt(this,&SHT10_BMP085_TSL2561::hmEventCol),(uint16_t*)&ptrMainList,(uint16_t*)&ptrPeerList);
	regCnl = cnl;																		// stores the channel we are responsible fore
}

void SHT10_BMP085_TSL2561::hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len) {
	if       (by3 == 0x00)                    poll();
	else if ((by3 == 0x01) && (by11 == 0x06)) configCngEvent();
	else if ((by3 == 0x11) && (by10 == 0x02)) pairSetEvent(data, len);
	else if ((by3 == 0x01) && (by11 == 0x0E)) pairStatusReq();
	else if ((by3 == 0x01) && (by11 == 0x01)) peerAddEvent(data, len);
	else if  (by3 >= 0x3E)                    peerMsgEvent(by3, data, len);
	else return;
}

void SHT10_BMP085_TSL2561::peerAddEvent(uint8_t *data, uint8_t len) {
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

void SHT10_BMP085_TSL2561::tsl2561_ISR(uint8_t pinState) {
	if (pinState == 0) {
		tsl2561IntFlag = 1;
	}
}

void SHT10_BMP085_TSL2561::setAltitude(int16_t altitude) {
	tAltitude = altitude;
}
