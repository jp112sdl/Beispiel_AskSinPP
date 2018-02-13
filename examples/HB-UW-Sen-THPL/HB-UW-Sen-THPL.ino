#include "HB-UW-Sen-THPL.h"
#include "Register.h"															// configuration sheet

//- load library's --------------------------------------------------------------------------------------------------------
#include <Wire.h>																// i2c library, needed for bmp085 or bmp180
#include <BMP085.h>
#include <BH1750.h>
#include <Buttons.h>															// remote buttons library
#include "Sensor_BMP180_BH1750.h"


// homematic communication
HM::s_jumptable jTbl[] = {														// jump table for HM communication
	// byte3, byte10, byte11, function to call									// 0xff means - any byte
	 { 0x11,  0x04,  0x00,    cmdReset },										// Reset message
	 { 0x01,  0xFF,  0x06,    cmdConfigChanged },								// Config end message
	 { 0x01,  0xFF,  0x0E,    cmdStatusRequest },
	 { 0x00 }
};

Buttons button[1];																// declare remote button object

BMP085 bmp085;
BH1750 bh1750;

Sensors_BMP180_BH1750 sensTHPL;

// main functions
void setup() {

	//millis overflow test
	//timer0_millis = 4294967295 - 666666;										// overflow reached in about 11 minutes and 7 seconds

	// We disable the Watchdog first
	wdt_disable();

	#ifdef SER_DBG
		Serial.begin(57600);													// serial setup
		Serial << F("Starting sketch...\n");									// ...and some information
		Serial << F("freeMem: ") << freeMem() << F(" byte") << F("\n");
	#endif

	#if USE_ADRESS_SECTION == 1
		getDataFromAddressSection(devParam, 1,  ADDRESS_SECTION_START + 0, 12);	// get device type (model-ID) and serial number from bootloader section at 0x7FF0 and 0x7FF2
		getDataFromAddressSection(devParam, 17, ADDRESS_SECTION_START + 12, 3);	// get device address stored in bootloader section at 0x7FFC

		Serial << F("Device type from Bootloader: "); pHex(&devParam[1],  2, SERIAL_DBG_PHEX_MODE_LF);
		Serial << F("Serial from Bootloader: ")     ; pHex(&devParam[3], 10, SERIAL_DBG_PHEX_MODE_LF);
		Serial << F("Addresse from Bootloader: ")   ; pHex(&devParam[17], 3, SERIAL_DBG_PHEX_MODE_LF);
	#else
		Serial << F("Device type from PROGMEM: "); pHex(&devParam[1],  2, SERIAL_DBG_PHEX_MODE_LF);
		Serial << F("Serial from PROGMEM: ")     ; pHex(&devParam[3], 10, SERIAL_DBG_PHEX_MODE_LF);
		Serial << F("Addresse from PROGMEM: ")   ; pHex(&devParam[17], 3, SERIAL_DBG_PHEX_MODE_LF);
	#endif

	hm.cc.config(10,11,12,13,2,0);												// CS, MOSI, MISO, SCK, GDO0, Interrupt

	// setup battery measurement
	hm.battery.config(
		BATTERY_MODE_EXTERNAL_MESSUREMENT, 7, 1, BATTERY_MEASSUREMENT_FACTOR, 900000	// battery measurement every 15 minutes
	);

	hm.init();																	// initialize the hm module

	button[0].regInHM(0, &hm);													// register buttons in HM per channel, handover HM class pointer
	button[0].config(8, NULL);													// configure button on specific pin and handover a function pointer to the main sketch

	sensTHPL.regInHM(1, &hm);													// register sensor class in hm
	sensTHPL.config(A4, A5, 0, &bmp085, &bh1750);						// data pin, clock pin and timing - 0 means HM calculated timing, every number above will taken in milliseconds

	byte rr = MCUSR;
	MCUSR =0;

	cmdConfigChanged(0, 0);

	// initialization done, we blink 3 times
	hm.statusLed.config(4, 4);													// configure the status led pin
	hm.statusLed.set(STATUSLED_BOTH, STATUSLED_MODE_BLINKFAST, 3);

}

void getDataFromAddressSection(uint8_t *buffer, uint8_t bufferStartAddress, uint16_t sectionAddress, uint8_t dataLen) {
	for (unsigned char i = 0; i < dataLen; i++) {
		buffer[(i + bufferStartAddress)] = pgm_read_byte(sectionAddress + i);
	}
}

void getPgmSpaceData(uint8_t *buffer, uint16_t address, uint8_t len) {
	for (unsigned char i = 0; i < len; i++) {
		buffer[i] = pgm_read_byte(address+i);
	}
}

void loop() {
	hm.poll();																	// poll the HM communication
}

void cmdReset(uint8_t *data, uint8_t len) {
	#ifdef SER_DBG
		Serial << F("reset, data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LF);
	#endif

	hm.send_ACK();																// send an ACK
	if (data[1] == 0) {
		hm.reset();
//		hm.resetWdt();
	}
}

void cmdConfigChanged(uint8_t *data, uint8_t len) {
	// low battery level
	uint8_t lowBatVoltage = regs.ch0.l0.lowBatLimit;
	lowBatVoltage = (lowBatVoltage < BATTERY_MIN_VOLTAGE) ? BATTERY_MIN_VOLTAGE : lowBatVoltage;
	lowBatVoltage = (lowBatVoltage > BATTERY_MAX_VOLTAGE) ? BATTERY_MAX_VOLTAGE : lowBatVoltage;
	hm.battery.setMinVoltage(lowBatVoltage);

	// led mode
	hm.setLedMode((regs.ch0.l0.ledMode) ? LED_MODE_EVERYTIME : LED_MODE_CONFIG);

	// power mode for HM device
//	hm.setPowerMode(POWER_MODE_SLEEP_WDT);
	hm.setPowerMode((regs.ch0.l0.burstRx) ? POWER_MODE_BURST : POWER_MODE_SLEEP_WDT);
//	hm.setPowerMode(POWER_MODE_ON);

	// set max transmit retry
	uint8_t transmDevTryMax = regs.ch0.l0.transmDevTryMax;
	transmDevTryMax = (transmDevTryMax < 1) ? 1 : transmDevTryMax;
	transmDevTryMax = (transmDevTryMax > 10) ? 10 : transmDevTryMax;
	dParm.maxRetr = transmDevTryMax;

	// set altitude
	int16_t altitude = regs.ch0.l0.altitude[1] | (regs.ch0.l0.altitude[0] << 8);
	altitude = (altitude < -500) ? -500 : altitude;
	altitude = (altitude > 10000) ? 10000 : altitude;
	sensTHPL.setAltitude(altitude);

	#ifdef SER_DBG
		Serial << F("Config changed. Data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LEN | SERIAL_DBG_PHEX_MODE_LF);
		Serial << F("lowBatLimit: ") << regs.ch0.l0.lowBatLimit << F("\n");
		Serial << F("ledMode: ") << regs.ch0.l0.ledMode << F("\n");
		Serial << F("burstRx: ") << regs.ch0.l0.burstRx << F("\n");
		Serial << F("transmDevTryMax: ") << transmDevTryMax << F("\n");
		Serial << F("altitude: ") << altitude << F("\n");
	#endif
}

void cmdStatusRequest(uint8_t *data, uint8_t len) {
	Serial << F("status request, data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LF);
	_delay_ms(50);
}

void HM_Remote_Event(uint8_t *data, uint8_t len) {
	Serial << F("remote event, data: "); pHex(data,len, SERIAL_DBG_PHEX_MODE_LF);
}
