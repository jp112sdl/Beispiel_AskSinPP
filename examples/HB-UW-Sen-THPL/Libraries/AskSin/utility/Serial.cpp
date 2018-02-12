//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Serial interface functions --------------------------------------------------------------------------------------------
//- Parser sketch from: http://jeelabs.org/2010/10/24/parsing-input-commands/
//- -----------------------------------------------------------------------------------------------------------------------
#include "Serial.h"

//- input parser class
InputParser::InputParser (byte size, const Commands *ctab, Stream& stream)
: limit (size), cmds (ctab), io (stream) {
	//buffer = (byte*) malloc(size);
	reset();
}
void InputParser::reset() {
	fill = next = 0;
	instring = hexmode = hasvalue = 0;
	top = limit;
}
void InputParser::poll() {
	if (!io.available()) {
		return;
	}

	char ch = io.read();
	if (ch < ' ' || fill >= top) {
		reset();
		return;
	}

	if (instring) {
		if (ch == '"') {
			buffer[fill++] = 0;
			do
			buffer[--top] = buffer[--fill];
			while (fill > value);
			ch = top;
			instring = 0;
		}
		buffer[fill++] = ch;
		return;
	}

	if (hexmode && (('0' <= ch && ch <= '9') || ('A' <= ch && ch <= 'F') || ('a' <= ch && ch <= 'f'))) {
		if (!hasvalue)
		value = 0;
		if (ch > '9')
		ch += 9;
		value <<= 4;
		value |= (byte) (ch & 0x0F);
		hasvalue = 1;
		return;
	}

	if ('0' <= ch && ch <= '9') {
		if (!hasvalue) value = 0;
		value = 10 * value + (ch - '0');
		hasvalue = 1;
		return;
	}

	hexmode = 0;

	/*if (ch == '$') {
		hexmode = 1;

	} else if (ch == '"') {
		instring = 1;
		value = fill;

	} else if (ch == ':') {
		(word&) buffer[fill] = value;
		fill += 2;
		value >>= 16;
		(word&) buffer[fill] = value;
		fill += 2;
		hasvalue = 0;

	} else if (ch == '.') {
		(word&) buffer[fill] = value;
		fill += 2;
		hasvalue = 0;

	} else if (ch == '-') {
		value = - value;
		hasvalue = 0;

	} else if (ch == ' ') {
		if (!hasvalue) return;
		buffer[fill++] = value;
		hasvalue = 0;

	} else if (ch == ',') {
		buffer[fill++] = value;
		hasvalue = 0;
	}*/

	switch (ch) {
		case '$':
			hexmode = 1;
			return;

		case '"':
			instring = 1;
			value = fill;
			return;
		case ':':
			(word&) buffer[fill] = value;
			fill += 2;
			value >>= 16;
			// fall through

		case '.':
			(word&) buffer[fill] = value;
			fill += 2;
			hasvalue = 0;
			return;

		case '-':
			value = - value;
			hasvalue = 0;
			return;

		case ' ':
			if (!hasvalue) return;
			// fall through

		case ',':
			buffer[fill++] = value;
			hasvalue = 0;
			return;
	}

	if (hasvalue) {
		io.print(F("Unrecognized character: "));
		io.print(ch);
		io.println();
		reset();
		return;
	}
	
	for (const Commands* p = cmds; ; ++p) {
		char code = pgm_read_byte(&p->code);
		if (code == 0)
		break;
		if (ch == code) {
			byte bytes = pgm_read_byte(&p->bytes);
			if (fill < bytes) {
				io.print(F("Not enough data, need "));
				io.print((int) bytes);
				io.println(F(" bytes"));
				} else {
				memset(buffer + fill, 0, top - fill);
				((void (*)()) pgm_read_word(&p->fun))();
			}
			reset();
			return;
		}
	}
	
	io.print(F("Known commands:"));
	for (const Commands* p = cmds; ; ++p) {
		char code = pgm_read_byte(&p->code);
		if (code == 0)
		break;
		io.print(' ');
		io.print(code);
	}
	io.println();
}
InputParser& InputParser::get(void *ptr, byte len) {
	memcpy(ptr, buffer + next, len);
	next += len;
	return *this;
}

InputParser& InputParser::operator >> (const char*& v) {
	byte offset = buffer[next++];
	v = top <= offset && offset < limit ? (char*) buffer + offset : "";
	return *this;
}

//- serial print functions
void pCharPGM(const uint8_t *buf) {
	char c;
	while (( c = pgm_read_byte(buf++)) != 0) {
		Serial << c;
	}
}

void pHexPGM(const uint8_t *buf, uint8_t len) {
	for (uint8_t i=0; i<len; i++) {
		
		pHexB(pgm_read_byte(&buf[i]));
		if(i+1 < len) {
			Serial << F(" ");
		}
	}
}

void pHexB(uint8_t val) {
	const char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	Serial << hexDigits[val >> 4] << hexDigits[val & 0xF];
}

void pHex(uint8_t *buf, uint8_t len, uint8_t mode) {
	for (uint8_t i=0; i<len; i++) {
		pHexB(buf[i]);
		if(i+1 < len) Serial << F(" ");
	}
	if (mode & SERIAL_DBG_PHEX_MODE_LEN) Serial << F(" (L:") << len << F(")");
	if (mode & SERIAL_DBG_PHEX_MODE_TIME) pTime();
	if (mode & SERIAL_DBG_PHEX_MODE_LF) Serial << F("\n");
}

void pTime(void) {
	Serial << F(" (M:") << millis() << F(")\n");
}
