//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Switch.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED_PIN 4
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8
#define LED_PIN           4
#define OUT_LONG_PIN      5
#define OUT_SHORT_PIN     6


// number of available peers per channel
#define PEERS_PER_CHANNEL 8


// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x01, 0xd8, 0x09},     // Device ID
  "JPLCSw1002",           // Device Serial
  {0x00,0xd8},            // Device Model
  0x26,                   // Firmware Version
  as::DeviceType::Switch, // Device Type
  {0x01, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> RadioSPI;
typedef AskSin<StatusLed<LED_PIN>, NoBattery, Radio<RadioSPI, 2> > Hal;

class MySwitchChannel : public ActorChannel<Hal,SwitchList1,SwitchList3,PEERS_PER_CHANNEL,List0,SwitchStateMachine> {
  class PinControl : public Alarm {
  private:
    uint8_t pin_A;
    uint8_t pin_B;
    uint8_t loopcount;
  public:
    PinControl () : Alarm(0), pin_A(0), pin_B(0), loopcount(0) {}
    virtual ~PinControl () {}

    void initPins(uint8_t p_long,uint8_t p_short) {
      pin_A=p_long;
      pin_B=p_short;
      ArduinoPins::setOutput(pin_A);
      ArduinoPins::setOutput(pin_B);
    }

    void start() {
      sysclock.cancel(*this);
      loopcount = 0;
      set(0);
      sysclock.add(*this);
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      switch (loopcount) {
        case 0:
          ArduinoPins::setHigh(pin_A);
          ArduinoPins::setLow (pin_B);
          set(millis2ticks(1000));
          break;
        case 1:
          ArduinoPins::setLow (pin_A);
          ArduinoPins::setHigh(pin_B);
          set(millis2ticks(500));
        break;  
        case 2:
          ArduinoPins::setHigh(pin_A);
          ArduinoPins::setHigh(pin_B);
          set(millis2ticks(100));
        case 3:
          ArduinoPins::setLow (pin_A);
          ArduinoPins::setLow (pin_B);
        break;
      }

      if (loopcount < 3) clock.add(*this);
      
      loopcount++;   
    }
  };

  PinControl pincontrol;

protected:
  typedef ActorChannel<Hal,SwitchList1,SwitchList3,PEERS_PER_CHANNEL,List0,SwitchStateMachine> BaseChannel;
public:
  MySwitchChannel () : BaseChannel() {}
  virtual ~MySwitchChannel() {}

  void init (uint8_t p_long,uint8_t p_short) {
    pincontrol.initPins(p_long, p_short);
    typename BaseChannel::List1 l1 = BaseChannel::getList1();
    this->set(l1.powerUpAction() == true ? 200 : 0, 0, 0xffff );
    this->changed(true);
  }

  uint8_t flags () const {
    return BaseChannel::flags();
  }

  virtual void switchState(__attribute__((unused)) uint8_t oldstate,uint8_t newstate,__attribute__((unused)) uint32_t delay) {
    if (( newstate == AS_CM_JT_ON ) || ( newstate == AS_CM_JT_OFF )) {
      pincontrol.start();
    }
    this->changed(true);
  }
};

typedef MultiChannelDevice<Hal, MySwitchChannel, 1> SwitchType;

Hal hal;
SwitchType sdev(devinfo, 0x20);
ConfigToggleButton<SwitchType> cfgBtn(sdev);

void initPeerings (bool first) {
  // create internal peerings - CCU2 needs this
  if ( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    Peer ipeer(devid, 1);
    sdev.channel(1).peer(ipeer);
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.channel(1).init(OUT_LONG_PIN, OUT_SHORT_PIN);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);

  initPeerings(first);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Idle<> >(hal);
  }
}
