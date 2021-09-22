
#ifndef _SENS_BMP180_H_
#define _SENS_BMP180_H_

#include <Sensors.h>
#include <Wire.h>
#include <SFE_BMP180.h>

namespace as {

class Sens_Bmp180 : public Sensor {

  int16_t   _temperature;
  uint16_t  _pressure;
  uint16_t  _pressureNN;

  SFE_BMP180 bmp180;

public:

  Sens_Bmp180 () : _temperature(0), _pressure(0), _pressureNN(0) {}

  void init () {
    Wire.begin(); //ToDo sync with further I2C sensor classes
    _present = bmp180.begin();
    
    DPRINT("BMP180 init ");DDECLN(_present);
  }

  void measure (uint16_t height) {
    if (_present == true) {
      double T,P,p0;
      char status = bmp180.startTemperature();

      if (status != 0) {
        // Wait for the measurement to complete:
        delay(status);
        status = bmp180.getTemperature(T);
        if (status != 0) {
          status = bmp180.startPressure(3);
          if (status != 0) {
            delay(status);
            status = bmp180.getPressure(P,T);
            if (status != 0) {
              p0 = bmp180.sealevel(P,height);
            }
          }
        }
      }
      
      _temperature = (int16_t)(T * 10);
      _pressure    = (uint16_t)(P * 10);
      _pressureNN  = (uint16_t)(p0 * 10);
      
      DPRINTLN(F("BMP180:"));
      DPRINT(F("-T    : ")); DDECLN(_temperature);
      DPRINT(F("-P    : ")); DDECLN(_pressure);
      DPRINT(F("-P(NN): ")); DDECLN(_pressureNN);
    }
  }
  
  int16_t  temperature () { return _temperature; }
  uint16_t pressure ()    { return _pressure; }
  uint16_t pressureNN ()  { return _pressureNN; }

};

}

#endif
