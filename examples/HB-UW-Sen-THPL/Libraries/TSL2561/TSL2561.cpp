/*
	TSL2561 illumination sensor library for Arduino
	Mike Grusin, SparkFun Electronics
	
	This library provides functions to access the TAOS TSL2561
	Illumination Sensor.
	
	Our example code uses the "beerware" license. You can do anything
	you like with this code. No really, anything. If you find it useful,
	buy me a beer someday.

	version 1.0 2013/09/20 MDG initial version
*/

#include <TSL2561.h>
#include <Wire.h>
#include <util/delay.h>

/**
 * The constructor
 */
TSL2561::TSL2561(void) {
}

/**
 * Initialize TSL2561 library with default address (0x39).
 */
void TSL2561::begin(void) {
	begin(TSL2561_ADDR);
}

/**
 * Initialize TSL2561 library to arbitrary address or:
 * TSL2561_ADDR_0 (0x29 address pin connected to gnd)
 * TSL2561_ADDR   (0x39 address pin not connected)
 * TSL2561_ADDR_1 (0x49 address pin conneted to vcc)
 */
void TSL2561::begin(char i2c_address) {
	_i2c_address = i2c_address;
	Wire.begin();
}

/**
 * Turn on TSL2561, begin integrations
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::setPowerUp(void) {
	// Write 0x03 to command byte (power on)
	return(writeByte(TSL2561_REG_CONTROL, 0x03));
}

/**
 * Turn off TSL2561
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::setPowerDown(void) {
	// Clear command byte (power off)
	return(writeByte(TSL2561_REG_CONTROL, 0x00));
}

/**
 * If gain = false (0), device is set to low gain (1X)
 * If gain = high (1), device is set to high gain (16X)
 * If time = 0, integration will be 13.7ms
 * If time = 1, integration will be 101ms
 * If time = 2, integration will be 402ms
 * If time = 3, use manual start / stop
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::setTiming(boolean gain, unsigned char timeConst) {
	unsigned char timing;

	_gain = gain;

	if (timeConst == 0) {
		_integrationTime = 14;

	} else if (timeConst == 1) {
		_integrationTime = 101;

	} else if (timeConst == 2) {
		_integrationTime = 402;
	} else {
		_integrationTime = 0;
	}

	// Get timing byte
	if (readByte(TSL2561_REG_TIMING,timing)) {
		// Set gain (0 or 1)
		if (gain)
			timing |= 0x10;
		else
			timing &= ~0x10;

		// Set integration timeConst (0 to 3)
		timing &= ~0x03;
		timing |= (timeConst & 0x03);

		// Write modified timing byte back to device
		if (writeByte(TSL2561_REG_TIMING, timing)) {
			return(true);
		}
	}

	return(false);
}

boolean TSL2561::setTimingManual(unsigned int ms) {
//	if (writeByte(TSL2561_REG_TIMING, 0x81)) {

		Wire.beginTransmission(_i2c_address);
		Wire.write(0x81);
		Wire.write(0x03);
		Wire.endTransmission();

		Wire.beginTransmission(_i2c_address);
		Wire.write(0x81);
		Wire.write(0x0B);
		Wire.endTransmission();

		// Begin manual integration
//		writeByte(TSL2561_REG_TIMING, 0x0B);
		noInterrupts();															// disable all interrupts for accurate delay loop
		delayloop16( ((ms * 1930) - 830) );										// delay loop, wait ms AVR@8 mHz
		interrupts();															// enable interrupts again

		// Stop manual integration
//		writeByte(TSL2561_REG_TIMING, 0x03);
		Wire.beginTransmission(_i2c_address);
		Wire.write(0x81);
		Wire.write(0x03);
		Wire.endTransmission();

		setIntergrationTime(ms);
		return(true);
//	}

//	return(false);

}

/**
 * Starts a manual integration period
 * After running this command, you must manually stop integration with manualStop()
 * Internally sets integration time to 3 for manual integration (gain is unchanged)
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::manualStart(void) {
	unsigned char timing;
	
	// Get timing byte
	if (readByte(TSL2561_REG_TIMING,timing)) {
		// Set integration time to 3 (manual integration)
		timing |= INTEGATION_TIME_MANUAL;

		if (writeByte(TSL2561_REG_TIMING, timing)) {
			// Begin manual integration
			timing |= 0x08;

			// Write modified timing byte back to device
			if (writeByte(TSL2561_REG_TIMING, timing)) {
				return(true);
			}
		}
	}

	return(false);
}

/**
 * Stops a manual integration period
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::manualStop(void) {
	unsigned char timing;
	
	// Get timing byte
	if (readByte(TSL2561_REG_TIMING,timing)) {
		// Stop manual integration
		timing &= ~0x08;

		// Write modified timing byte back to device
		if (writeByte(TSL2561_REG_TIMING, timing)) {
			return(true);
		}
	}

	return(false);
}

/**
 * Retrieve raw integration results
 * data0 and data1 will be set to integration results
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::getData(unsigned int &data0, unsigned int &data1) {
	// Get data0 and data1 out of result registers
	if (readUInt(TSL2561_REG_DATA_0,data0) && readUInt(TSL2561_REG_DATA_1,data1)) {
		return(true);
	}

	return(false);
}

void TSL2561::setIntergrationTime(uint16_t integrationTime) {
	_integrationTime = integrationTime;
}

/**
 * Convert raw data to lux
 * CH0, CH1: results from getData()
 * lux will be set to resulting lux calculation
 * returns true (1) if calculation was successful
 * returns false (0) and lux = 0.0 if the sensor was satturated
 */
boolean TSL2561::getLux(unsigned int CH0, unsigned int CH1, double &lux) {
	uint16_t satturationValue = 5047;

	if (_integrationTime > 178) {
		satturationValue = 0xFFFF; // 65535
	} else if (_integrationTime > 101) {
		satturationValue = 37177;
	}

	// Determine if either sensor saturated, calculation will not be accurate, break here
	if ((CH0 == satturationValue) || (CH1 == satturationValue)) {
		lux = 0.0;
		return(false);
	} else {
		lux = CalculateLux(CH0, CH1);
	}

	return true;
}

/**
 * Sets up interrupt operations
 * If control = 0, interrupt output disabled
 * If control = 1, use level interrupt, see setInterruptThreshold()
 * If persist = 0, every integration cycle generates an interrupt
 * If persist = 1, any value outside of threshold generates an interrupt
 * If persist = 2 to 15, value must be outside of threshold for 2 to 15 integration cycles
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::setInterruptControl(unsigned char control, unsigned char persist) {
	// Place control and persist bits into proper location in interrupt control register
	if (writeByte(TSL2561_REG_INTCTL, ((control & 0B00000011) << 4) | (persist & 0B00001111))) {
		return(true);
	}

	return(false);
}

/**
 * Set interrupt thresholds (channel 0 only)
 * low, high: 16-bit threshold values
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::setInterruptThreshold(uint16_t low, uint16_t high) {
	if (writeUInt(TSL2561_REG_THRESH_L, low) && writeUInt(TSL2561_REG_THRESH_H, high) ) {
		return(true);
	}

	return(false);
}

/*
 * Clears an active interrupt
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::clearInterrupt(void) {
	// Set up command byte for interrupt clear
	Wire.beginTransmission(_i2c_address);
	Wire.write(TSL2561_CMD_CLEAR);

	_error = Wire.endTransmission();
	if (_error == 0) {
		return(true);
	}

	return(false);
}

/*
 * Retrieves part and revision code from TSL2561
 * Sets ID to part ID (see datasheet)
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() below)
 */
boolean TSL2561::getID(unsigned char &ID) {
	// Get ID byte from ID register
	if (readByte(TSL2561_REG_ID,ID)) {
		return(true);
	}

	return(false);
}

/**
 * If any library command fails, you can retrieve an extended
 * error code using this command. Errors are from the wire library:
 * 0 = Success
 * 1 = Data too long to fit in transmit buffer
 * 2 = Received NACK on transmit of address
 * 3 = Received NACK on transmit of data
 * 4 = Other error
 */
byte TSL2561::getError(void) {
	return(_error);
}

// Private functions:

/**
 * Reads a byte from a TSL2561 address
 * Address: TSL2561 address (0 to 15)
 * Value will be set to stored byte
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() above)
 */
boolean TSL2561::readByte(unsigned char address, unsigned char &value) {
	// Set up command byte for read
	Wire.beginTransmission(_i2c_address);
	Wire.write((address & 0x0F) | TSL2561_CMD);
	_error = Wire.endTransmission();

	// Read requested byte
	if (_error == 0) {
		Wire.requestFrom(_i2c_address, 1);
		if (Wire.available() == 1) {
			value = Wire.read();
			return(true);
		}
	}

	return(false);
}

/**
 * Write a byte to a TSL2561 address
 * Address: TSL2561 address (0 to 15)
 * Value: byte to write to address
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() above)
 */
boolean TSL2561::writeByte(unsigned char address, unsigned char value) {
	// Set up command byte for write

	Wire.beginTransmission(_i2c_address);
	Wire.write((address & 0x0F) | TSL2561_CMD);
	// Write byte
	Wire.write(value);

	_error = Wire.endTransmission();
	if (_error == 0) {
		return(true);
	}

	return(false);
}

/**
 * Reads an unsigned integer (16 bits) from a TSL2561 address (low byte first)
 * Address: TSL2561 address (0 to 15), low byte first
 * Value will be set to stored unsigned integer
 * Returns true (1) if successful, false (0) if there was an I2C error
 * Also see getError() above)
 */
boolean TSL2561::readUInt(unsigned char address, unsigned int &value) {
	char high, low;
	
	// Set up command byte for read
	Wire.beginTransmission(_i2c_address);
	Wire.write((address & 0x0F) | TSL2561_CMD);
	_error = Wire.endTransmission();

	// Read two bytes (low and high)
	if (_error == 0) {
		Wire.requestFrom(_i2c_address,2);
		if (Wire.available() == 2) {
			low = Wire.read();
			high = Wire.read();
			// Combine bytes into unsigned int
			value = word(high,low);
			return(true);
		}
	}

	return(false);
}

/**
 * Write an unsigned integer (16 bits) to a TSL2561 address (low byte first)
 * Address: TSL2561 address (0 to 15), low byte first
 * Value: unsigned int to write to address
 * Returns true (1) if successful, false (0) if there was an I2C error
 * (Also see getError() above)
 */
boolean TSL2561::writeUInt(unsigned char address, unsigned int value) {
	// Split int into lower and upper bytes, write each byte
	if (writeByte(address, lowByte(value)) && writeByte(address + 1, highByte(value))) {
		return(true);
	}

	return(false);
}

/**
 *
 */
void __attribute__((always_inline)) TSL2561::delayloop16 (unsigned int count) {
	/* Die Schleife dauert  4 * count + 3  Ticks */
	asm volatile (
		"1:"           "\n\t"
		"sbiw %0,1"    "\n\t"
		"brcc 1b"
		: "+w" (count)
	);
}






















//****************************************************************************
//
//  Copyright    2004−2005 TAOS, Inc.
//
//  THIS CODE AND INFORMATION IS PROVIDED ”AS IS” WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//    Module Name:
// lux.cpp
//
//****************************************************************************
#define LUX_SCALE   14    // scale by 2^14
#define RATIO_SCALE 9     // scale ratio by 2^9

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// Integration time scaling factors
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define CH_SCALE          10    // scale channel values by 2^10
#define CHSCALE_TINT0     0x7517 // 322/11 * 2^CH_SCALE
#define CHSCALE_TINT1     0x0fe7 // 322/81 * 2^CH_SCALE
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// T Package coefficients
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define K1T  0x0040       // 0.125 * 2^RATIO_SCALE
#define B1T  0x01f2       // 0.0304 * 2^LUX_SCALE
#define M1T  0x01be       // 0.0272 * 2^LUX_SCALE
#define K2T  0x0080       // 0.250 * 2^RATIO_SCALETSL2560, TSL2561
#define B2T  0x0214       // 0.0325 * 2^LUX_SCALE
#define M2T  0x02d1       // 0.0440 * 2^LUX_SCALE
#define K3T  0x00c0       // 0.375 * 2^RATIO_SCALE
#define B3T  0x023f       // 0.0351 * 2^LUX_SCALE
#define M3T  0x037b       // 0.0544 * 2^LUX_SCALE
#define K4T  0x0100       // 0.50 * 2^RATIO_SCALE
#define B4T  0x0270       // 0.0381 * 2^LUX_SCALE
#define M4T  0x03fe       // 0.0624 * 2^LUX_SCALE
#define K5T  0x0138       // 0.61 * 2^RATIO_SCALE
#define B5T  0x016f       // 0.0224 * 2^LUX_SCALE
#define M5T  0x01fc       // 0.0310 * 2^LUX_SCALE
#define K6T  0x019a       // 0.80 * 2^RATIO_SCALE
#define B6T  0x00d2       // 0.0128 * 2^LUX_SCALE
#define M6T  0x00fb       // 0.0153 * 2^LUX_SCALE
#define K7T  0x029a       // 1.3 * 2^RATIO_SCALE
#define B7T  0x0018       // 0.00146 * 2^LUX_SCALE
#define M7T  0x0012       // 0.00112 * 2^LUX_SCALE
#define K8T  0x029a       // 1.3 * 2^RATIO_SCALE
#define B8T  0x0000       // 0.000 * 2^LUX_SCALE
#define M8T  0x0000       // 0.000 * 2^LUX_SCALE


//////////////////////////////////////////////////////////////////////////////
//    Routine:     unsigned int CalculateLux(unsigned int ch0, unsigned int ch0)
//
//    Description: Calculate the approximate illuminance (lux) given the raw
// channel values of the TSL2560. The equation if implemented
// as a piece−wise linear approximation.
//
//    Arguments:   unsigned int iGain − gain, where 0:1X, 1:16X
// unsigned int tInt − integration time, where 0:13.7mS, 1:100mS, 2:402mS,
//  3:Manual
// unsigned int ch0 − raw channel value from channel 0 of TSL2560
// unsigned int ch1 − raw channel value from channel 1 of TSL2560
//
//    Return:      unsigned int − the approximate illuminance (lux)
//
//////////////////////////////////////////////////////////////////////////////
double TSL2561::CalculateLux(unsigned int ch0, unsigned int ch1) {
	//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
	// first, scale the channel values depending on the gain and integration time
	// scale if integration time is NOT 402 msec
	unsigned long chScale, channel1, channel0;

	if (_integrationTime == 14) {	// 13.7 msec
		chScale = CHSCALE_TINT0;

	} else  if (_integrationTime == 101) {
		chScale = CHSCALE_TINT1;

	} else {						// 402, assume no scaling
		chScale = (1 << CH_SCALE);
	}

	// scale if gain is NOT 16X
	if (!_gain) {
		chScale = chScale << 4;  // scale 1X to 16X
	}

	// scale the channel values
	channel0 = (ch0 * chScale) >> CH_SCALE;
	channel1 = (ch1 * chScale) >> CH_SCALE;

	// find the ratio of the channel values (Channel1/Channel0) protect against divide by zero
	unsigned long ratio1 = 0;
	if (channel0 != 0) {
		ratio1 = (channel1 << (RATIO_SCALE+1)) / channel0;
	}

	// round the ratio value
	unsigned long ratio = (ratio1 + 1) >> 1;

	// is ratio <= eachBreak ?
	unsigned int b, m;
	// T package
	if ((ratio >= 0) && (ratio <= K1T)) {
		b=B1T; m=M1T;
	} else if (ratio <= K2T) {
		b=B2T; m=M2T;
	} else if (ratio <= K3T) {
		b=B3T; m=M3T;
	} else if (ratio <= K4T) {
		b=B4T; m=M4T;
	} else if (ratio <= K5T) {
		b=B5T; m=M5T;
	} else if (ratio <= K6T) {
		b=B6T; m=M6T;
	} else if (ratio <= K7T) {
		b=B7T; m=M7T;
	} else if (ratio > K8T) {
		b=B8T; m=M8T;
	}

	double temp = (channel0 * b) - (channel1 * m);
	temp = (temp < 0) ? 0 : temp;

	return(temp / 16384);
}

double  TSL2561::readBrightness(unsigned int &data0, unsigned int &data1) {
	double lux = 0;

	begin(TSL2561_ADDR_0);
	setPowerUp();

	// first measurement with low integration time and low resolution
	setTiming(false, INTEGATION_TIME_14);
	_delay_ms(50);
	getData(data0, data1);

	if ((data0 < 1000) && (data1 < 1000)) {
		boolean gain = ((data0 < 100) && (data1 < 100)) ? true : false;
		setTiming(gain, INTEGATION_TIME_402);
		_delay_ms(450);
		getData(data0, data1);

	} else if ((data0) < 2000 && (data1 < 2000)) {
		setTiming(false, INTEGATION_TIME_101);
		_delay_ms(150);
		getData(data0, data1);
	}

	boolean luxValid = getLux(data0, data1, lux);

	lux = (luxValid) ? lux : 65537;

	setPowerDown();

	return lux;
}

