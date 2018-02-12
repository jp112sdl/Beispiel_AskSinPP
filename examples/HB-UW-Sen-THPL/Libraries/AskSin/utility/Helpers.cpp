//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- helper functions ------------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

#include "Helpers.h"

extern uint16_t __bss_end, _pHeap_start;
extern void *__brkval;
uint16_t freeMem() {															// shows free memory
	uint16_t free_memory;

	if((uint16_t)__brkval == 0) {
		free_memory = ((uint16_t)&free_memory) - ((uint16_t)&__bss_end);
	} else {
		free_memory = ((uint16_t)&free_memory) - ((uint16_t)__brkval);
	}

	return free_memory;
}

uint16_t crc16(uint16_t crc, uint8_t a) {
	crc ^= a;
	for (uint8_t i = 0; i < 8; ++i) {
		if (crc & 1) {
			crc = (crc >> 1) ^ 0xA001;
		} else {
			crc = (crc >> 1);
		}
	}

	return crc;
}

uint32_t byteTimeCvt(uint8_t tTime) {
	const uint16_t c[8] = {1,10,50,100,600,3000,6000,36000};
	return (uint32_t)(tTime & 0x1f) * c[tTime >> 5] * 100;
}

uint32_t intTimeCvt(uint16_t iTime) {
	if (iTime == 0) {
		return 0;
	}

	uint8_t tByte;
	if ((iTime & 0x1F) != 0) {
		tByte = 2;
		for (uint8_t i = 1; i < (iTime & 0x1F); i++) {
			tByte *= 2;
		}
	} else {
		tByte = 1;
	}

	return (uint32_t)tByte * (iTime >> 5) * 100;
}
