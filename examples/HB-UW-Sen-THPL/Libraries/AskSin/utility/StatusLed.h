//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- Status led driver -----------------------------------------------------------------------------------------------------
//- with a lot of contribution from Dirk at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef STATUSLED_H
#define STATUSLED_H

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Serial.h"

const uint16_t heartBeat[] = { 50, 200, 50, 1000};

/**
 * Class for status led control
 */
class StatusLed {
	#define STATUSLED_BLINKRATE_SLOW   300
	#define STATUSLED_BLINKRATE_FAST   150
	#define STATUSLED_BLINKRATE_SFAST  50

	#define STATUSLED_1                1
	#define STATUSLED_2                2
	#define STATUSLED_BOTH             3

	#define STATUSLED_MODE_OFF         0
	#define STATUSLED_MODE_ON          1
	#define STATUSLED_MODE_BLINKSLOW   2
	#define STATUSLED_MODE_BLINKFAST   3
	#define STATUSLED_MODE_BLINKSFAST  4
	#define STATUSLED_MODE_HEARTBEAT   5

  public://---------------------------------------------------------------
	
	/**
	 * Configure the led pins
	 * If you use one led only, you must define the first pin only
	 *
	 * @param	the pin of status led 1
	 * @param	the pin of status led 2
	 * @param	a callback function
	 */
	void config(uint8_t pin1, uint8_t pin2);

	/**
	 * Set mode for leds
	 * 0 -> led1, 1 -> led2, 2 -> led1 and led2
	 *
	 * @param	the leds
	 * @param	the mode
	 */
	void set(uint8_t leds, uint8_t tMode, uint8_t blinkCount = 0);

	/**
	 * Set leds off
	 * 0 -> led1, 1 -> led2, 2 -> led1 and led2
	 *
	 * @param	the leds
	 */
	void stop(uint8_t leds);

	void poll(void);
	void on(uint8_t ledNum);
	void off(uint8_t ledNum);

  private://-------------------------------------------------------------------------------------------------------------
	uint8_t  pin[2];

	/* 0 off, 1 -> on */
	uint8_t  state[2];
	uint8_t  mode[2];

	/* blink counter */
	uint8_t  bCnt[2];
	uint32_t nTime[2];
	uint32_t startTime[2];

	void onOff(uint8_t mode, uint8_t ledNum);
	void toggle(uint8_t ledNum);
};

#endif
