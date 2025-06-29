//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2023-02-21 Peter.matic Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef __SENSORS_LMSS_h__
#define __SENSORS_LMSS_h__

#include <Sensors.h>

namespace as {

/*
 * SENSEPIN is the analog pin
 * ACTIVATEPIN is a output pin to power up the LMSS only while measuring. Use 0 if the LMSS is connected to VCC and not to a pin.
 */

template <uint8_t SENSEPIN,uint8_t ACTIVATEPIN=0,long PARAM_M=0,long PARAM_B=0,uint16_t PARAM_MIN=0,uint16_t PARAM_MAX=0>
class Lmss : public Brightness {
     
public:

Lmss () {}
  
  void init () {
    pinMode(SENSEPIN, INPUT);
  pinMode(ACTIVATEPIN, OUTPUT);
    }

  void measure (__attribute__((unused)) bool async=false) {
  
    
  static bool start_up;
  
  // equation of logarythmic charakteristic of LMSS-101
  // y = m*log(x)+b  -->  x = 10power((y-b)/m)
  // x: brightness in [lux], y: analog value in [digits]
  
    
  float exponent = 0.0;
  float lux = 0.0;
  uint16_t analog_value = 0;
  float b = (float)PARAM_B / 1000.0;
  float m = (float)PARAM_M / 1000.0;
  
    if (ACTIVATEPIN) {
      pinMode(ACTIVATEPIN, OUTPUT);
      digitalWrite(ACTIVATEPIN, HIGH);
  // waiting time for warmup of sensor  
  delay(30);
    }
  //read analog value
    analog_value = analogRead(SENSEPIN);
    
    if (ACTIVATEPIN) {
      digitalWrite(ACTIVATEPIN, LOW);
    }

  // calculate brightness from analog value
    exponent = ((float)analog_value - b) / m;
  lux = pow(10.0,exponent);
  
  // cutoff values above max range
  if (lux > PARAM_MAX) {
    lux = PARAM_MAX;
    }
  
  // adjust brightness to fit in 8 bit value with offset of min value
  lux = (lux  / ((float)PARAM_MAX) * (254.0 - (float)PARAM_MIN) + (float)PARAM_MIN);
  
  
    // Preset maxbrigt in motion.h to 255 at startup
  if (!start_up) {
    start_up = !start_up;
    lux = 255.0;
    }
  
  _brightness = lux;
    }
  
}; // end class
} //end namespace


#endif
