//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- helper functions ------------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
#ifndef HELPERS_H
#define HELPERS_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "utility/Serial.h"

uint16_t freeMem(void);
uint16_t crc16(uint16_t crc, uint8_t a);

uint32_t byteTimeCvt(uint8_t tTime);
uint32_t intTimeCvt(uint16_t iTime);

#endif
