//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Pin change interrupt handler ------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
#ifndef _PINCHANGEINTHANDLER_H
#define _PINCHANGEINTHANDLER_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Serial.h"
#include "Fastdelegate.h"

using namespace fastdelegate;
typedef FastDelegate1<uint8_t> s_dlgt;

struct s_pcIntH {
	uint8_t pcMask;
	uint8_t byMask;
	s_dlgt  dlgt;
};

void registerInt(uint8_t pin, s_dlgt Delegate);
void collectPCINT(uint8_t vectInt);

ISR(PCINT0_vect);
ISR(PCINT1_vect);
ISR(PCINT2_vect);
ISR(PCINT3_vect);



#endif