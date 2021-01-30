//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-01-30 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

// Arduino IDE Settings
// use support for 644PA from MightyCore: https://github.com/MCUdude/MightyCore
// settings:
// Board:        ATMega644
// Pinout:       Standard
// Clock:        8MHz external
// Variant:      644P / 644PA
// BOD:          2.7V
// Compiler LTO: Enabled

#define NDEBUG

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <SPI.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Switch.h>

#define RELAY_PIN          12 //PD4
#define BTN_PIN_1          14 //PD6
#define BTN_PIN_2          8  //PD0
#define LED_PIN            0  //PB0
#define CONFIG_BUTTON_PIN  15 //PD7
#define CS_PIN             4  //PB4
#define GDO0_PIN           10 //PD2


// number of available peers per channel
#define PEERS_PER_CHANNEL  10

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0x69, 0x51},     // Device ID
  "JPRWEISS21",           // Device Serial
  {0x00, 0x69},           // Device Model
  0x24,                   // Firmware Version
  as::DeviceType::Switch, // Device Type
  {0x01, 0x00}            // Info Bytes
};
/**
   Configure the used hardware
*/
typedef LibSPI<CS_PIN> SPIType;
typedef Radio<SPIType, GDO0_PIN> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, NoBattery, RadioType> Hal;
Hal hal;

class ISS2SwitchChannel : public ActorChannel<Hal,SwitchList1,SwitchList3,PEERS_PER_CHANNEL,List0,SwitchStateMachine> {
protected:
  typedef ActorChannel<Hal,SwitchList1,SwitchList3,PEERS_PER_CHANNEL,List0,SwitchStateMachine> BaseChannel;
  uint8_t pin;

public:
  ISS2SwitchChannel () : BaseChannel(), pin(0) {}
  virtual ~ISS2SwitchChannel() {}

  void init (uint8_t p,bool value=false) {
    pin=p;
    ArduinoPins::setOutput(pin);
    typename BaseChannel::List1 l1 = BaseChannel::getList1();
    this->set(l1.powerUpAction() == true ? 200 : 0, 0, 0xffff );
    this->changed(true);
  }

  uint8_t flags () const {
    return BaseChannel::flags();
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

typedef ISS2SwitchChannel swc;
typedef MultiChannelDevice<Hal, swc, 1> SwitchType;

SwitchType sdev(devinfo,0x20);
ConfigButton<SwitchType> cfgBtn(sdev);
InternalButton<SwitchType> btn1(sdev,2);
InternalButton<SwitchType> btn2(sdev,3);

void initPeerings (bool first) {
  if ( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    sdev.channel(1).peer(Peer(devid, 2), Peer(devid, 3));
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.channel(1).init(RELAY_PIN, false);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  buttonISR(btn1,BTN_PIN_1);
  buttonISR(btn2,BTN_PIN_2);

  initPeerings(first);
  storage().store();

  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Idle<> >(hal);
  }
}
