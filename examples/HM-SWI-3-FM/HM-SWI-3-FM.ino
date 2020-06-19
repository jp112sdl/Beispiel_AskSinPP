//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-06-19 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <MultiChannelDevice.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED1_PIN 4
#define LED2_PIN 5
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

// Anzahl Kanäle (3 - 7)
#define CHANNEL_COUNT 3
#define IN1_PIN  9
#define IN2_PIN  7
#define IN3_PIN  14

// number of available peers per channel
#define PEERS_PER_CHANNEL 10


#define switchISR(device,chan,pin) class device##chan##ISRHandler { \
  public: \
  static void isr () { device.channel(chan).irq(); } \
}; \
device.channel(chan).initPin(pin); \
if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
  enableInterrupt(pin,device##chan##ISRHandler::isr,CHANGE); \
else \
  attachInterrupt(digitalPinToInterrupt(pin),device##chan##ISRHandler::isr,CHANGE);

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0x46, 0x09},     // Device ID
  "JPSWI30009",           // Device Serial
  {0x00, 0x46},           // Device Model
  0x01,                   // Firmware Version
  as::DeviceType::Swi,    // Device Type
  {0x00, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef DualStatusLed<LED2_PIN, LED1_PIN> LedType;
typedef AskSin<LedType, NoBattery, RadioType> Hal;
Hal hal;


class SwiChannel : public Channel<Hal,List1,EmptyList,DefList4,PEERS_PER_CHANNEL,List0> {
  class pinChangedAlarm : public Alarm {
  public:
    SwiChannel& swic;
    pinChangedAlarm (SwiChannel& _swic) : Alarm(0), swic(_swic) {}
    ~pinChangedAlarm () {}
    virtual void trigger(__attribute__((unused)) AlarmClock& clock) {
      swic.send();
    }
  };
protected:
  pinChangedAlarm pca;
private:
   bool last;
   uint8_t p;
   uint8_t cnt;
public:

  typedef Channel<Hal,List1,EmptyList,DefList4,PEERS_PER_CHANNEL,List0> BaseChannel;

  SwiChannel () : BaseChannel(), pca(*this), last(false), p(0), cnt(0) {}
  virtual ~SwiChannel () {}

  uint8_t status () const {
    return 0;
  }

  uint8_t flags () const {
    return 0;
  }

  void irq () {
    sysclock.cancel(pca);
    sysclock.add(pca);
  }

  void initPin (uint8_t pin) {
    p = pin;
    pinMode(p, INPUT_PULLUP);
    last = digitalRead(p);
  }

  void send() {
    bool s = digitalRead(p);
    if (s != last) {
      last = s;
      DHEX(BaseChannel::number());
      RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();
      msg.init(this->device().nextcount(),this->number(),cnt++,false,false);
      this->device().broadcastPeerEvent(msg,*this);
    }
  }
};

typedef MultiChannelDevice<Hal, SwiChannel, CHANNEL_COUNT, List0> SWIType;

SWIType sdev(devinfo, 0x20);
ConfigButton<SWIType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  switchISR(sdev, 1, IN1_PIN);
  switchISR(sdev, 2, IN2_PIN);
  switchISR(sdev, 3, IN3_PIN);
  sdev.initDone();
}

void loop() {
  hal.runready();
  sdev.pollRadio();

}

