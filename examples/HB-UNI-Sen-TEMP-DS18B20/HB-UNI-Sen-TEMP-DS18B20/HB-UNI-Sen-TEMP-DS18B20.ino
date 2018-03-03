//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <MultiChannelDevice.h>

#include "Ds18b20.h"

// Arduino Pro mini 8 Mhz
// Arduino pin for the config button
#define CONFIG_BUTTON_PIN 8

// #define TIME_MEASURE_AND_SEND_IN_S 15

// number of available peers per channel
#define PEERS_PER_CHANNEL 6

// all library classes are placed in the namespace 'as'
using namespace as;

#define INTERVAL 180

#define NUM_SENSORS 8

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x11, 0x12, 0x13},       	 // Device ID
  "UNITEMP001",           	  // Device Serial
  {0xF3, 0x01},            	 // Device Model
  0x10,                   	  // Firmware Version
  as::DeviceType::THSensor, 	// Device Type
  {0x01, 0x01}             	 // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef StatusLed<4> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> BaseHal;
class Hal : public BaseHal {
  public:
    void init (const HMID& id) {
      BaseHal::init(id);
      // init real time clock - 1 tick per second
      //rtc.init();
      // measure battery every 1h
      battery.init(seconds2ticks(60UL * 60), sysclock);
      battery.low(20); // Low voltage set to 2.0V
      battery.critical(18); // Critical voltage set to 1.8V
    }

    bool runready () {
      return sysclock.runready() || BaseHal::runready();
    }
} hal;

class WeatherEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int16_t temps[8], bool batlow) {

      uint8_t t1 = (temps[0] >> 8) & 0x7f;
      uint8_t t2 = temps[0] & 0xff;
      if ( batlow == true ) {
        t1 |= 0x80; // set bat low bit
      }
      Message::init(0x19, msgcnt, 0x70, BIDI, t1, t2); // first byte determines message length; pload[0] starts at byte 13

      for (int i = 0; i < 14; i++) {
        pload[i] = (temps[(i / 2) + 1] >> 8) & 0x7f;
        pload[i + 1] = temps[(i / 2) + 1] & 0xff;
        i++;
      }
    }
};

DEFREGISTER(WeatherRegsList0, DREG_BURSTRX)
typedef RegList0<WeatherRegsList0> WeatherList0;


class WeatherChannel : public Channel<Hal, List1, EmptyList, List4, PEERS_PER_CHANNEL, WeatherList0>, public Alarm {

    WeatherEventMsg msg;

    Ds18b20<4>    ds18b20;
    int16_t       temperatures[8];
    uint16_t          millis;

  public:
    WeatherChannel () : Channel(), Alarm(seconds2ticks(INTERVAL)), millis(0) {}
    virtual ~WeatherChannel () {}

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      // wait also for the millis
      if ( millis != 0 ) {
        tick =  seconds2ticks(INTERVAL);
        millis = 0; // reset millis
        clock.add(*this); // millis with sysclock
      }
      else {
        uint8_t msgcnt = device().nextcount();
        measure();
        msg.init(msgcnt, temperatures, device().battery().low());
        device().sendPeerEvent(msg, *this);

        // reactivate for next measure
        HMID id;
        device().getDeviceID(id);
        uint32_t nextsend = delay(id, msgcnt); // TIME_MEASURE_AND_SEND_IN_S*240000;
        tick = nextsend / 1000; // seconds to wait
        millis = nextsend % 1000; // millis to wait
        clock.add(*this);
      }
    }

    // here we do the measurement
    void measure () {
      memset(temperatures, 0, sizeof(temperatures));
      /*for (int i = 0; i < NUM_SENSORS; i++) {
        ds18b20.measure(i);
        temperatures[i] = ds18b20.temperature();
        }*/
      //Fake Werte zum Testen  
      temperatures[0] = 89;
      temperatures[1] = 190;
      temperatures[2] = 560;
      temperatures[3] = 1200;
      temperatures[4] = -18;
      temperatures[5] = -200;
      temperatures[6] = 95;
      temperatures[7] = 326;

      DPRINT("measure()");
    }

    // here we calc when to send next value
    uint32_t delay (const HMID& id, uint8_t msgcnt) {
      uint32_t value = ((uint32_t)id) << 8 | msgcnt;
      value = (value * 1103515245 + 12345) >> 16;
      value = (value & 0xFF) + 480;
      value *= 250;

      DDEC(value / 1000); DPRINT("."); DDECLN(value % 1000);

      return value;
    }

    void setup(Device<Hal, WeatherList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      seconds2ticks(INTERVAL);
      sysclock.add(*this);
      ds18b20.init();
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

typedef MultiChannelDevice<Hal, WeatherChannel, 1, WeatherList0> WeatherType;
WeatherType sdev(devinfo, 0x20);

ConfigButton<WeatherType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Sleep<>>(hal);
  }
}
