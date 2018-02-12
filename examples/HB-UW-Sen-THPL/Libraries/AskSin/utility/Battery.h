//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Low battery alarm -----------------------------------------------------------------------------------------------------
//- with a lot of contribution from Dirk at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef BATTERY_H
#define BATTERY_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <util/delay.h>
#include <Wire.h>
#include "Serial.h"

class Battery {
  public://----------------------------------------------------------------------------------------------------------------
	
	uint8_t state;																		// status byte for message handling
	uint16_t voltage;																	// last measured battery voltage

	void config(uint8_t mode, uint8_t enablePin, uint8_t adcPin, uint8_t fact, uint16_t time);
	void setMinVoltage(uint8_t tenthVolts);												// set the reference voltage in 0.1 volts

	void poll(void);																	// poll for periodic check

  private://---------------------------------------------------------------------------------------------------------------
	uint8_t  tMode;																		// remember the mode
	uint8_t  tEnablePin;
	uint8_t  tAdcPin;
	float    tFact;
	uint16_t tTime;																		// remember the time for periodic check
	uint32_t nTime;																		// timer for periodic check
	uint8_t  tTenthVolts;																// remember the tenth volts set
	uint8_t oldVoltage;																	// old last measured battery voltage

	#define BATTERY_STATE_HYSTERESIS          1											// battery state should reset only if voltage rise greater than given value
	#define BATTERY_NUM_MESS_ADC              64
	#define BATTERY_DUMMY_NUM_MESS_ADC        10
	#define AVR_BANDGAP_VOLTAGE               1100UL									// Band gap reference for Atmega328p

	#define BATTERY_MODE_BANDGAP_MESSUREMENT  1
	#define BATTERY_MODE_EXTERNAL_MESSUREMENT 2


	uint8_t getBatteryVoltageInternal();
	uint8_t getBatteryVoltageExternal();

	uint16_t getAdcValue(uint8_t voltageReference, uint8_t inputChannel);
};

#endif
