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

#ifndef TSL2561_h
#define TSL2561_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define INTEGATION_TIME_14                         0
#define INTEGATION_TIME_101                        1
#define INTEGATION_TIME_402                        2
#define INTEGATION_TIME_MANUAL                     3

#define TSL2561_ADDR_0                             0x29 // address with '0' shorted on board
#define TSL2561_ADDR                               0x39 // default address
#define TSL2561_ADDR_1                             0x49 // address with '1' shorted on board

// TSL2561 registers
#define TSL2561_CMD                                0x80
#define TSL2561_CMD_CLEAR                          0xC0
#define	TSL2561_REG_CONTROL                        0x00
#define	TSL2561_REG_TIMING                         0x01
#define	TSL2561_REG_THRESH_L                       0x02
#define	TSL2561_REG_THRESH_H                       0x04
#define	TSL2561_REG_INTCTL                         0x06
#define	TSL2561_REG_ID                             0x0A
#define	TSL2561_REG_DATA_0                         0x0C
#define	TSL2561_REG_DATA_1                         0x0E

#define TSL2561_INTERRUPT_CONTROL_DISABLED         0b00
#define TSL2561_INTERRUPT_CONTROL_LEVEL            0b01
#define TSL2561_INTERRUPT_CONTROL_SMBALERT         0b10

#define TSL2561_INTERRUPT_PSELECT_EVERY_ADC        0b0000
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_1   0b0001
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_2   0b0010
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_3   0b0011
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_4   0b0100
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_5   0b0101
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_6   0b0110
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_7   0b0110
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_8   0b0111
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_9   0b1001
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_10  0b1010
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_11  0b1011
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_12  0b1100
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_13  0b1101
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_14  0b1110
#define TSL2561_INTERRUPT_PSELECT_OUT_OF_RANGE_15  0b1111

#define TSL2561_INTERRUPT_CONTROL_DISABLED 0b00

class TSL2561 {
	public:
		/**
		 * The constructor
		 */
		TSL2561(void);
			
		/**
		 * Initialize TSL2561 library with default address (0x39).
		 */
		void begin(void);

		/**
		 * Initialize TSL2561 library to arbitrary address or:
		 * TSL2561_ADDR_0 (0x29 address pin connected to gnd)
		 * TSL2561_ADDR   (0x39 address pin not connected)
		 * TSL2561_ADDR_1 (0x49 address pin conneted to vcc)
		 */
		void begin(char i2c_address);
		
		/**
		 * Turn on TSL2561, begin integrations
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean setPowerUp(void);

		/**
		 * Turn off TSL2561
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean setPowerDown(void);

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
		boolean setTiming(boolean gain, unsigned char timeConst);

		/**
		 * Starts a manual integration period
		 * After running this command, you must manually stop integration with manualStop()
		 * Internally sets integration time to 3 for manual integration (gain is unchanged)
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean manualStart(void);

		/**
		 * Stops a manual integration period
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean manualStop(void);

		/**
		 * Retrieve raw integration results
		 * data0 and data1 will be set to integration results
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean getData(unsigned int &CH0, unsigned int &CH1);

		boolean readData(unsigned int &data0, unsigned int &data1);

		void setIntergrationTime(uint16_t integrationTime);

		/**
		 * Convert raw data to lux
		 * CH0, CH1: results from getData()
		 * lux will be set to resulting lux calculation
		 * returns true (1) if calculation was successful
		 * returns false (0) and lux = 0.0 if the sensor was satturated
		 */
		boolean getLux(unsigned int CH0, unsigned int CH1, double &lux);

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
		boolean setInterruptControl(unsigned char control, unsigned char persist);

		/**
		 * Set interrupt thresholds (channel 0 only)
		 * low, high: 16-bit threshold values
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean setInterruptThreshold(uint16_t low, uint16_t high);

		/*
		 * Clears an active interrupt
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean clearInterrupt(void);

		/*
		 * Retrieves part and revision code from TSL2561
		 * Sets ID to part ID (see datasheet)
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() below)
		 */
		boolean getID(unsigned char &ID);
			
		/**
		 * If any library command fails, you can retrieve an extended
		 * error code using this command. Errors are from the wire library:
		 * 0 = Success
		 * 1 = Data too long to fit in transmit buffer
		 * 2 = Received NACK on transmit of address
		 * 3 = Received NACK on transmit of data
		 * 4 = Other error
		 */
		byte getError(void);

		boolean setTimingManual(unsigned int ms);

		void __attribute__((always_inline)) delayloop16 (unsigned int count);

		double  readBrightness(unsigned int &data0, unsigned int &data1);


	private:
		char _i2c_address;
		uint16_t _integrationTime;
		uint8_t _gain;
		byte _error;

		/**
		 * Reads a byte from a TSL2561 address
		 * Address: TSL2561 address (0 to 15)
		 * Value will be set to stored byte
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() above)
		 */
		boolean readByte(unsigned char address, unsigned char &value);
	
		/**
		 * Write a byte to a TSL2561 address
		 * Address: TSL2561 address (0 to 15)
		 * Value: byte to write to address
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() above)
		 */
		boolean writeByte(unsigned char address, unsigned char value);

		/**
		 * Reads an unsigned integer (16 bits) from a TSL2561 address (low byte first)
		 * Address: TSL2561 address (0 to 15), low byte first
		 * Value will be set to stored unsigned integer
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * Also see getError() above)
		 */
		boolean readUInt(unsigned char address, unsigned int &value);

		/**
		 * Write an unsigned integer (16 bits) to a TSL2561 address (low byte first)
		 * Address: TSL2561 address (0 to 15), low byte first
		 * Value: unsigned int to write to address
		 * Returns true (1) if successful, false (0) if there was an I2C error
		 * (Also see getError() above)
		 */
		boolean writeUInt(unsigned char address, unsigned int value);

		double CalculateLux(unsigned int ch0, unsigned int ch1);

};

#endif
