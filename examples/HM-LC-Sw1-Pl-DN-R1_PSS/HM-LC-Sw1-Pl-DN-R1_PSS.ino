//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-01-29 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Switch.h>

#define LED_PIN           8
#define CONFIG_BUTTON_PIN 0
#define RELAY1_PIN        A0


// number of available peers per channel
#define PEERS_PER_CHANNEL 8


// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x01, 0xd8, 0xa9},     // Device ID
  "JPPSS00001",           // Device Serial
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

class PSSSwitchChannel : public ActorChannel<Hal,SwitchList1,SwitchList3,PEERS_PER_CHANNEL,List0,SwitchStateMachine> {
protected:
  typedef ActorChannel<Hal,SwitchList1,SwitchList3,PEERS_PER_CHANNEL,List0,SwitchStateMachine> BaseChannel;
  uint8_t pin;

public:
  PSSSwitchChannel () : BaseChannel(), pin(0) {}
  virtual ~PSSSwitchChannel() {}

  void init (uint8_t p,bool value=false) {
    pin=p;
    ArduinoPins::setOutput(pin);
    typename BaseChannel::List1 l1 = BaseChannel::getList1();
    this->set(l1.powerUpAction() == true ? 200 : 0, 0, 0xffff );
    this->changed(true);
  }

  uint8_t flags () const {
    uint8_t flags = BaseChannel::flags();
    if( this->device().battery().low() == true ) {
      flags |= 0x80;
    }
    return flags;
  }


  virtual void switchState(__attribute__((unused)) uint8_t oldstate,uint8_t newstate,__attribute__((unused)) uint32_t delay) {
    if( newstate == AS_CM_JT_ON ) {
      ArduinoPins::setHigh(pin);
      device().led().invert(true);
    }
    else if ( newstate == AS_CM_JT_OFF ) {
      ArduinoPins::setLow(pin);
      device().led().invert(false);
    }
    this->changed(true);
  }
};

typedef PSSSwitchChannel swc;

typedef MultiChannelDevice<Hal, swc, 1> SwitchType;

Hal hal;
SwitchType sdev(devinfo, 0x20);
ConfigToggleButton<SwitchType> cfgBtn(sdev);

void initPeerings (bool first) {
  // create internal peerings - CCU2 needs this
  if ( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    for ( uint8_t i = 1; i <= sdev.channels(); ++i ) {
      Peer ipeer(devid, i);
      sdev.channel(i).peer(ipeer);
    }
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.channels(1);
  sdev.channel(1).init(RELAY1_PIN, false);

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
