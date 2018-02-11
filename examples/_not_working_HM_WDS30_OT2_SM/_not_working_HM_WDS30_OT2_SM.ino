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

#include <MultiChannelDevice.h>
#include <DallasTemperature.h>

// we use a Pro Mini
// Arduino pin for the LED
// D5 == PIN 5 on Pro Mini
#define LED_PIN 5
// D4 == PIN 4 on Pro Mini
#define DS18B20_PIN 4
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

// number of available peers per channel
#define PEERS_PER_CHANNEL 6

//seconds between sending messages
#define MSG_INTERVAL 400

// all library classes are placed in the namespace 'as'
using namespace as;

OneWire ourWire(DS18B20_PIN);
DallasTemperature sensors(&ourWire);

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x01, 0xa8, 0x81},     // Device ID
  "JPOT000001",           // Device Serial
  {0x00, 0xa8},           // Device Model
  0x16,                   // Firmware Version
  as::DeviceType::THSensor, // Device Type
  {0x01, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef StatusLed<5> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> BaseHal;
class Hal : public BaseHal {
  public:
    void init (const HMID& id) {
      BaseHal::init(id);
      // measure battery every 1h
      battery.init(seconds2ticks(60UL*60),sysclock);
      battery.low(22);
      battery.critical(19);
    }

    bool runready () {
      return sysclock.runready() || BaseHal::runready();
    }
} hal;
;

class WeatherEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int16_t temp,bool batlow) {
      uint8_t t1 = (temp >> 8) & 0x7f;
      uint8_t t2 = temp & 0xff;
      if ( batlow == true ) {
        t1 |= 0x80; // set bat low bit
      }
      Message::init(0x0c, msgcnt, 0x70, BIDI, t1, t2);
    }
};

class WeatherChannel : public Channel<Hal, List1, EmptyList, List4, PEERS_PER_CHANNEL, List0>, public Alarm {

    WeatherEventMsg msg;
    int16_t         temp;
    uint16_t        millis;

  public:

    WeatherChannel () : Channel(), Alarm(5), temp(0), millis(0) {}
    virtual ~WeatherChannel () {}
    
    // here we do the measurement
    void measure () {
      DPRINT("Measure...\n");
      temp = 255;//t * 10.0;
    }
    
    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      tick = delay();
      clock.add(*this);
      measure();

      msg.init(msgcnt, temp, device().battery().low());

      device().sendPeerEvent(msg, *this);
    }

    uint32_t delay () {
      return seconds2ticks(MSG_INTERVAL);
    }
    void setup(Device<Hal, List0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class MeasureEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int16_t temp,bool batlow) {
      uint8_t t1 = (temp >> 8) & 0x7f;
      uint8_t t2 = temp & 0xff;
      if ( batlow == true ) {
        t1 |= 0x80; // set bat low bit
      }
      Message::init(0x0c, msgcnt, 0x70, BIDI, t1, t2);
      pload[0] = "89";
      pload[1] = "89";
    }
};

class MeasureChannel : public Channel<Hal, List1, EmptyList, List4, PEERS_PER_CHANNEL, List0>, public Alarm {

    MeasureEventMsg msg;
    int16_t         temp;
    uint16_t        millis;

  public:

    MeasureChannel () : Channel(), Alarm(5), temp(0), millis(0) {}
    virtual ~MeasureChannel () {}
    
    // here we do the measurement
    void measure () {
      DPRINT("Measure...\n");
      temp = 255;//t * 10.0;
    }
    
    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      tick = delay();
      clock.add(*this);
      measure();

      msg.init(msgcnt, temp, device().battery().low());

      device().sendPeerEvent(msg, *this);
    }

    uint32_t delay () {
      return seconds2ticks(MSG_INTERVAL);
    }
    void setup(Device<Hal, List0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class OTDevice : public MultiChannelDevice<Hal,MeasureChannel,5, List0> {
  public:
    typedef MultiChannelDevice<Hal, MeasureChannel,5, List0> BaseDevice;
    OTDevice(const DeviceInfo& info, uint16_t addr) : BaseDevice(info, addr) {}
    virtual ~OTDevice () {}

    virtual void configChanged () {
      BaseDevice::configChanged();
    }
};
OTDevice sdev(devinfo, 0x20);
ConfigButton<OTDevice> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sensors.begin();
  sdev.init(hal);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Idle<>>(hal);
  }
}
