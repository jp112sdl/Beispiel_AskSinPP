//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Serial interface functions --------------------------------------------------------------------------------------------
//- Parser sketch from: http://jeelabs.org/2010/10/24/parsing-input-commands/
//- -----------------------------------------------------------------------------------------------------------------------
#ifndef SERIAL_H
#define SERIAL_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <avr/eeprom.h>


//- input parser class
class InputParser {
  public:
	typedef struct {
		char code;																		// one-letter command code
		byte bytes;																		// number of bytes required as input
		void (*fun)();																	// code to call for this command
	} Commands;
	
	InputParser (byte size, const Commands*, Stream& =Serial);							// set up with a buffer of specified size
	
	byte count() { return fill; }														// number of data bytes
	//byte *buffer;																		// holds the read data
	byte buffer[50];																		// holds the read data
	
	void poll();																		// call this frequently to check for incoming data
	
	InputParser& operator >> (char& v)      { return get(&v, 1); }
	InputParser& operator >> (byte& v)      { return get(&v, 1); }
	InputParser& operator >> (int& v)       { return get(&v, 2); }
	InputParser& operator >> (word& v)      { return get(&v, 2); }
	InputParser& operator >> (long& v)      { return get(&v, 4); }
	InputParser& operator >> (uint32_t& v)  { return get(&v, 4); }
	InputParser& operator >> (const char*& v);

  private:
	InputParser& get(void*, byte);
	void reset();
	
	byte limit, fill, top, next;
	byte instring, hexmode, hasvalue;
	uint32_t value;
	const Commands* cmds;
	Stream& io;
};
extern const InputParser::Commands cmdTab[];

#define SERIAL_DBG_PHEX_MODE_NONE    0
#define SERIAL_DBG_PHEX_MODE_LEN     1
#define SERIAL_DBG_PHEX_MODE_LF      2
#define SERIAL_DBG_PHEX_MODE_TIME    4

//- some support for serial function
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

void pCharPGM(const uint8_t *buf);												// print the content from PROGMEM
void pHexPGM(const uint8_t *buf, uint8_t len);									// print a couple of bytes in HEX format
void pHexB(uint8_t val);														// print one byte in HEX format
void pHex(uint8_t *buf, uint8_t len, uint8_t mode);								// print a couple of bytes in HEX format
void pTime(void);																// print a time stamp in brackets

#endif
