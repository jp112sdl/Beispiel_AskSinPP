//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//            dirk@forum.fhem.de
//- -----------------------------------------------------------------------------------------------------------------------
//- Low battery alarm -----------------------------------------------------------------------------------------------------
//- with a lot of contribution from Dirk at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------

#include "Battery.h"

/**
 * Configure the battery measurement
 *
 * mode:      Measurement mode BATTERY_MODE_BANDGAP_MESSUREMENT or BATTERY_MODE_EXTERNAL_MESSUREMENT
 * enablePin: The pin which enables the external measurement helper hardware. Set to 0 if not needed.
 * adcPin:    The ADC pin for external measurement. Set to 0 if not needed.
 * time:      interval to check the battery voltage
 */
void Battery::config(uint8_t mode, uint8_t enablePin, uint8_t adcPin, uint8_t fact, uint16_t time) {
	tMode = mode;
	tEnablePin = enablePin;
	tAdcPin = adcPin;
	tFact = fact;
	tTime = time;
	nTime = time;

	if (tEnablePin > 0) {
		pinMode(tEnablePin, INPUT);
	}
}

/**
 * Set the minimal battery voltage in tenth volt
 *
 * tenthVolts: minimum voltage before battery warning occurs
 */
void Battery::setMinVoltage(uint8_t tenthVolts) {
	tTenthVolts = tenthVolts;
}

/**
 * Cyclic battery measurement function.
 */
void Battery::poll(void) {
	uint8_t batteryVoltage;

	if ((tMode == 0) || (millis() - nTime < tTime)) {
		return;																	// nothing to do, step out

	} else  if (tMode == BATTERY_MODE_BANDGAP_MESSUREMENT) {
		voltage = getBatteryVoltageInternal();

	} else  if (tMode == BATTERY_MODE_EXTERNAL_MESSUREMENT) {
		voltage = getBatteryVoltageExternal();
	}

	if ((voltage > oldVoltage && (voltage - oldVoltage) > BATTERY_STATE_HYSTERESIS) || voltage < oldVoltage) {
		oldVoltage = voltage;
		state = (voltage < tTenthVolts) ? 1 : 0;									// set the battery status
	}

	nTime = millis();

}

/**
 * get battery voltage
 * Measure AVCC again the the internal band gap reference
 *
 *	REFS1 REFS0          --> internal bandgap reference
 *	MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG) (for instance Atmega 328p)
 */
uint8_t Battery::getBatteryVoltageInternal() {
	uint16_t adcValue = getAdcValue(
		(0 << REFS1) | (1 << REFS0),											// Voltage Reference = AVCC with external capacitor at AREF pin
		(1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (0 << MUX0)					// Input Channel = 1.1V (V BG)
	);
	adcValue = AVR_BANDGAP_VOLTAGE * 1023 / adcValue / 100;						// calculate battery voltage in 1/10 V

	return adcValue;
}

uint8_t Battery::getBatteryVoltageExternal() {
	pinMode(tEnablePin, OUTPUT);
	digitalWrite(tEnablePin, LOW);

	uint16_t adcValue = getAdcValue(
		(1 << REFS1) | (1 << REFS0), tAdcPin									// Voltage Reference = Internal 1.1V Voltage Reference
	);

	adcValue = adcValue * AVR_BANDGAP_VOLTAGE / 1023 / tFact;					// calculate battery voltage in 1/10 V

	pinMode(tEnablePin, INPUT);

	return adcValue;
}

uint16_t Battery::getAdcValue(uint8_t voltageReference, uint8_t inputChannel) {
	uint16_t adcValue = 0;

	ADMUX = (voltageReference | inputChannel);

	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);							// Enable ADC and set ADC prescaler
	for(int i = 0; i < BATTERY_NUM_MESS_ADC + BATTERY_DUMMY_NUM_MESS_ADC; i++) {
		ADCSRA |= (1 << ADSC);													// start conversion
		while (ADCSRA & (1 << ADSC)) {}											// wait for conversion complete

		if (i >= BATTERY_DUMMY_NUM_MESS_ADC) {									// we discard the first dummy measurements
			adcValue += ADCW;
		}
	}

	ADCSRA &= ~(1 << ADEN);														// ADC disable

	adcValue = adcValue / BATTERY_NUM_MESS_ADC;

	return adcValue;
}
