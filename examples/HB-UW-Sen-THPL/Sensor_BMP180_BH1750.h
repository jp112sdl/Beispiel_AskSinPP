/**
 * SHT10, BMP085, TSL2561 Sensor class
 *
 * (c) 2014 Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
 * Author: Horst Bittner <horst@diebittners.de>
 *         Dirk Hoffmann dirk@fhem.forum
 */

#ifndef SENSOR_BMP180_BH1750_H
#define SENSOR_BMP180_BH1750_H

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
#include <BMP085.h>
#include <BH1750.h>


#define BMP180_BH1750 Sensors_BMP180_BH1750						// module name as macro to overcome the problem of renaming functions all the time
//#define DM_DBG																// debug message flag

#define BMP180_BH1750_nACTION_MEASURE_INIT    1
#define BMP180_BH1750_nACTION_MEASURE_L       2
#define BMP180_BH1750_nACTION_CALC_L          3
#define BMP180_BH1750_nACTION_MEASURE_THP     4
#define BMP180_BH1750_nACTION_TRANSMIT        5

#define BMP180_BH1750_MAX_MEASURE_TIME       50
#define BMP180_BH1750_MINIMAL_CYCLE_LENGTH  480							// minimal cycle length in 250ms units

const uint8_t peerOdd[] =    {};												// default settings for list3 or list4
const uint8_t peerEven[] =   {};
const uint8_t peerSingle[] = {};

class BMP180_BH1750 {
	// user code here
	public:
		void config(uint8_t data, uint8_t sck, uint16_t timing, BMP085 *pPtr, BH1750 *lPtr);

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
		BMP085 *bm180;
		BH1750 *bh1750;

		uint8_t  nAction;
		uint32_t nTime;
		uint32_t startTime;
		uint16_t tTiming;

		int16_t tTemp;
		uint8_t  tHum;
		uint16_t tPres;
		int16_t  tAltitude;														// altitude for calculating air pressure at sea level

		uint32_t tLux;

		uint8_t bh1750IntFlag;

		uint32_t calcSendSlot(void);
		void     poll_measureCalcLight(void);
		void     poll_measureTHP(void);
		void     poll_transmit(void);
};


#endif
