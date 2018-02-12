//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Pin change interrupt handler ------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
#include "PinChangeIntHandler.h"

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)							// Arduino Mega

#elif defined(__AVR_AT90USB162__)														// Teensy 1.0

#elif defined(__AVR_ATmega32U4__)														// Teensy 2.0

#elif defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)						// Teensy++ 1.0 & 2.0

#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)							// Sanguino
	#define pinToSlot(p)   (((p) >= 0 && (p) <= 7) ? 1: ((p) >= 8 && (p) <= 14) ? 3: ((p) >= 15 && (p) <= 23) ? 2: ((p) >= 24 && (p) <= 31) ? 0: ((p) >= A0 && (p) <= A7) ? 0: 0xff)
	volatile uint8_t* pinPort[] = { &PINA, &PINB, &PINC, &PIND, };
	volatile uint8_t* intMask[] = { &PCMSK0, &PCMSK1, &PCMSK2, &PCMSK3, };
	volatile uint8_t portByte[4];

#elif defined(__AVR_ATmega8P__) || defined(__AVR_ATmega8__)								// Atmega8

#else																					// Arduino Duemilanove, Diecimila, LilyPad, Mini, Fio, etc
	#define pinToSlot(p)   (((p) >= 0 && (p) <= 7) ? 2: ((p) >= 8 && (p) <= 13) ? 0: ((p) >= A0 && (p) <= A5) ? 1: ((p) >= 14 && (p) <= 19) ? 1: 0xff)
	volatile uint8_t* intMask[] = { &PCMSK0, &PCMSK1, &PCMSK2, };
	volatile uint8_t* pinPort[] = { &PINB,   &PINC,   &PIND, };
	volatile uint8_t portByte[3];

#endif


s_pcIntH pcIntH[10];
uint8_t pcIntNbr = 0;

void registerInt(uint8_t tPin, s_dlgt tDelegate) {
	uint8_t pSlot = pinToSlot(tPin);													// get the position in port or PCMSK array
	
	pcIntNbr++;																			// increase counter because we have to add a new pin
	//pcIntH = (s_pcIntH*) realloc (pcIntH, pcIntNbr * sizeof(s_pcIntH));					// size the table
	
	pcIntH[pcIntNbr-1].dlgt = tDelegate;												// remember the address where we have to jump
	pcIntH[pcIntNbr-1].pcMask = pSlot;													// remember the PCMSK 
	pcIntH[pcIntNbr-1].byMask = digitalPinToBitMask(tPin);								// remember the pin position
	
	pinMode(tPin,INPUT_PULLUP);															// switch pin to input

	*intMask[pSlot] |= digitalPinToBitMask(tPin);										// set the pin in the respective PCMSK
	//Serial << F("iM:") << *intMask[pSlot] << "\n";
	PCICR |= (1 << pSlot);
	
	//PCICR |= _BV(pSlot);																// register PCMSK in interrupt change register

	portByte[pSlot] = *pinPort[pSlot] & *intMask[pSlot];								// remember the byte in the current port to figure out changes

	//Serial << F("pin:") << tPin << F(" pSlot:") << pSlot << F(" pB:") << portByte[pSlot] << F("\n");
	sei();

}

void collectPCINT(uint8_t vectInt) {
	cli();
	
	uint8_t intByte = *pinPort[vectInt] & *intMask[vectInt];							// get the input byte
	
	uint8_t msk = portByte[vectInt] ^ intByte;											// mask out the changes
	portByte[vectInt] = intByte;														// store pin byte for next time check

	if (msk == 0) {																		// seams to be a repeat, noting to do
		sei();
		return;
	}
	
	uint8_t bitStat = (intByte < *intMask[vectInt]) ? 0 : 1;							// prepare the bit status for the pin

	for (uint8_t i = 0; i < pcIntNbr; i++) {											// jump through the table and search for registered function
		if ((pcIntH[i].pcMask == vectInt) && (pcIntH[i].byMask == msk)) {
			pcIntH[i].dlgt(bitStat);
		}
	}

	sei();
}


ISR(PCINT0_vect) {
	collectPCINT(0);
}
ISR(PCINT1_vect) {
	collectPCINT(1);
}
ISR(PCINT2_vect) {
	collectPCINT(2);
}
ISR(PCINT3_vect) {
	collectPCINT(3);
}
