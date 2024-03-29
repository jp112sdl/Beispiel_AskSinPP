//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-05-31 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Register.h>

#include <MultiChannelDevice.h>
#include "Sens_BMP180.h"
#include <Sensirion.h>

#define LED_PIN           4
#define CONFIG_BUTTON_PIN 8
#define BMP180_VCC_PIN    A2
//-----------------------------------------------------------------------------------------

//Korrektur von Temperatur und Luftfeuchte
//Einstellbarer OFFSET für Temperatur -> gemessene Temp +/- Offset = Angezeigte Temp.
#define OFFSETtemp 0 //z.B -50 ≙ -5°C / 50 ≙ +5°C

//Einstellbarer OFFSET für Luftfeuchte -> gemessene Luftf. +/- Offset = Angezeigte Luftf.
#define OFFSEThumi 0 //z.B -10 ≙ -10%RF / 10 ≙ +10%RF

//-----------------------------------------------------------------------------------------

#define PEERS_PER_CHANNEL 6
#define MSG_INTERVAL      180

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x34, 0x56, 0x83},     // Device ID
  "JPTH10I006",           // Device Serial
  {0xf3, 0x05},           // Device Model Indoor
  0x10,                   // Firmware Version
  as::DeviceType::THSensor, // Device Type
  {0x01, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, IrqInternalBatt, RadioType> Hal;
Hal hal;

DEFREGISTER(Reg0, MASTERID_REGS, 0x22, 0x23)
class customList0 : public RegList0<Reg0> {
  public:
  customList0(uint16_t addr) : RegList0<Reg0>(addr) {}

    bool height (uint16_t value) const {
      return this->writeRegister(0x22, (value >> 8) & 0xff) && this->writeRegister(0x23, value & 0xff);
    }
    uint16_t height () const {
      return (this->readRegister(0x22, 0) << 8) + this->readRegister(0x23, 0);
    }

    void defaults () {
      clear();
      height(0);
    }
};

class WeatherEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int16_t temp, uint8_t humidity, uint16_t airPressure, bool batlow) {
      uint8_t t1 = (temp >> 8) & 0x7f;
      uint8_t t2 = temp & 0xff;
      if ( batlow == true ) {
        t1 |= 0x80; // set bat low bit
      }
      Message::init(0xe, msgcnt, 0x70, BIDI | WKMEUP, t1, t2);
      pload[0] = humidity;
      pload[1] = (airPressure >> 8) & 0xff;
      pload[2] = airPressure & 0xff;
    }
};

class WeatherChannel : public Channel<Hal, List1, EmptyList, List4, PEERS_PER_CHANNEL, customList0>, public Alarm {

    WeatherEventMsg msg;
    int16_t         temp;
    uint8_t         humidity;
    uint16_t        pressure;

    Sensirion       sht10;
    Sens_Bmp180     bmp180;

  public:
    WeatherChannel () : Channel(), Alarm(5), temp(0), humidity(0), pressure(0), sht10(SDA,SCL) {}
    virtual ~WeatherChannel () {}

    // here we do the measurement
    void measure () {
      DPRINT("Measure...\n");

      
      pinMode(BMP180_VCC_PIN, OUTPUT);
      digitalWrite(BMP180_VCC_PIN, HIGH);
      _delay_ms(100);
      
      uint16_t rawData;
      if ( sht10.measTemp(&rawData) == 0) {
        float t = sht10.calcTemp(rawData);
        temp = t * 10;
        if ( sht10.measHumi(&rawData) == 0 ) {
          humidity = sht10.calcHumi(rawData, t);
        }
      }

      bmp180.init();
      bmp180.measure(device().getList0().height());
      pressure = bmp180.pressureNN();
      
      Wire.end();
      pinMode(BMP180_VCC_PIN, INPUT);


      DPRINTLN("T/H/P = " + String(temp) + "/" + String(humidity) + "/" + String(pressure));
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      tick = delay();
      clock.add(*this);
      measure();
      msg.init(msgcnt, temp, humidity, pressure, device().battery().low());
      if (msgcnt % 20 == 1) device().sendPeerEvent(msg, *this); else device().broadcastEvent(msg, *this);
    }

    uint32_t delay () {
      return seconds2ticks(MSG_INTERVAL);
    }
    void setup(Device<Hal, customList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
      pinMode(BMP180_VCC_PIN, INPUT);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

typedef MultiChannelDevice<Hal, WeatherChannel, 1, customList0> WeatherType;
WeatherType sdev(devinfo, 0x20);
ConfigButton<WeatherType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  hal.initBattery(60UL * 60, 22, 19);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  while (hal.battery.current() == 0);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Sleep<>>(hal);
  }
}
