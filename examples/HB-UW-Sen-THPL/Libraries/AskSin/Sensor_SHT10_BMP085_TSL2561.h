/**
 * SHT10, BMP085, TSL2561 Sensor class
 *
 * (c) 2014 Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
 * Author: Horst Bittner <horst@diebittners.de>
 *         Dirk Hoffmann dirk@fhem.forum
 */

#ifndef SENSOR_SHT10_BMP085_TSL2561_H
#define SENSOR_SHT10_BMP085_TSL2561_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "AskSinMain.h"
#include "utility/Serial.h"
#include "utility/Fastdelegate.h"
#include "utility/PinChangeIntHandler.h"

#include <Wire.h>
#include <Sensirion.h>
#include <BMP085.h>
#include <TSL2561.h>

//#define US_100
#ifdef US_100
	#define US_100_PIN_VCC         5
	#define US_100_PIN_GND         9
	#define US_100_PIN_TRIGGER     6
	#define US_100_PIN_ECHO        7
#endif

#define SHT10_BMP085_TSL2561 Sensors_SHT10_BMP085_TSL2561						// module name as macro to overcome the problem of renaming functions all the time
//#define DM_DBG																// debug message flag

#define SHT10_BMP085_TSL2561_nACTION_MEASURE_INIT    1
#define SHT10_BMP085_TSL2561_nACTION_MEASURE_L       2
#define SHT10_BMP085_TSL2561_nACTION_CALC_L          3
#define SHT10_BMP085_TSL2561_nACTION_MEASURE_THP     4
#define SHT10_BMP085_TSL2561_nACTION_TRANSMIT        5

#define SHT10_BMP085_TSL2561_MAX_MEASURE_TIME       50
#define SHT10_BMP085_TSL2561_MINIMAL_CYCLE_LENGTH  480							// minimal cycle length in 250ms units

const uint8_t peerOdd[] =    {};												// default settings for list3 or list4
const uint8_t peerEven[] =   {};
const uint8_t peerSingle[] = {};

class SHT10_BMP085_TSL2561 {
	// user code here
	public:
		void config(uint8_t data, uint8_t sck, uint16_t timing, Sensirion *tPtr, BMP085 *pPtr, TSL2561 *lPtr);

		// mandatory functions for every new module to communicate within HM protocol stack
		uint8_t  modStat;														// module status byte, needed for list3 modules to answer status requests
		uint8_t  regCnl;														// holds the channel for the module

		HM      *hm;															// pointer to HM class instance
		uint8_t *ptrPeerList;													// pointer to list3/4 in regs struct
		uint8_t *ptrMainList;													// pointer to list0/1 in regs struct

		long     counter;

		void    configCngEvent(void);											// list1 on registered channel had changed
		void    pairSetEvent(uint8_t *data, uint8_t len);						// pair message to specific channel, handover information for value, ramp time and so on
		void    pairStatusReq(void);											// event on status request
		void    peerMsgEvent(uint8_t type, uint8_t *data, uint8_t len);			// peer message was received on the registered channel, handover the message bytes and length
		void    poll(void);														// poll function, driven by HM loop

		//- predefined, no reason to touch ------------------------------------------------------------------------------------
		void    regInHM(uint8_t cnl, HM *instPtr);											// register this module in HM on the specific channel
		void    hmEventCol(uint8_t by3, uint8_t by10, uint8_t by11, uint8_t *data, uint8_t len); // call back address for HM for informing on events
		void    peerAddEvent(uint8_t *data, uint8_t len);									// peer was added to the specific channel, 1st and 2nd byte shows peer channel, third and fourth byte shows peer index
		void    setAltitude(int16_t altitude);

	protected:

	private:
		Sensirion *sht10;
		BMP085 *bm180;
		TSL2561 *tsl2561;

		uint8_t  nAction;
		uint32_t nTime;
		uint32_t startTime;
		uint16_t tTiming;

		int16_t tTemp;
		uint8_t  tHum;
		uint16_t tPres;
		int16_t  tAltitude;														// altitude for calculating air pressure at sea level

		uint8_t tsl2561InitCount;
		unsigned int tsl2561Data0;
		unsigned int tsl2561Data1;
		uint32_t tLux;

		uint8_t tsl2561IntFlag;

		boolean gain;    // Gain setting, 0 = X1, 1 = X16;

		void tsl2561_ISR(uint8_t pinState);

		uint32_t calcSendSlot(void);
		uint8_t  poll_measureLightInit();
		void     poll_measureCalcLight(void);
		void     poll_measureTHP(void);
		void     poll_transmit(void);

		uint32_t us100Measuer();
};


#endif
