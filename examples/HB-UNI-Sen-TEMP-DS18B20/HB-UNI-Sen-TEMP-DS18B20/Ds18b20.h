#ifndef __SENSORS_DS18B20_h__
#define __SENSORS_DS18B20_h__

#include <Sensors.h>
#include <OneWire.h>
#include <DallasTemperature.h>

namespace as {

template <int DATAPIN>
class Ds18b20 : public Temperature {
  OneWire ourWire = OneWire(DATAPIN);
  DallasTemperature _ds18b20 = DallasTemperature(&ourWire);
  int _i;

public:
  Ds18b20 () : _ds18b20(DATAPIN) {}

  void init () {
    _ds18b20.begin();
    _present = true;
  }
  void measure (int num, __attribute__((unused)) bool async=false) {
    if( present() == true ) {
      _i = 0;
      do {
        if( _i != 0 ) {
          delay(200);
          DPRINT("DS18B20 Try # ");DDECLN(_i);
        }
        _ds18b20.requestTemperatures();
        _temperature = _ds18b20.getTempCByIndex(num) * 10;
        _i = _i + 1;
      } while ((_temperature == 0) && _i <= 10);
    }
  }
};

}

#endif
