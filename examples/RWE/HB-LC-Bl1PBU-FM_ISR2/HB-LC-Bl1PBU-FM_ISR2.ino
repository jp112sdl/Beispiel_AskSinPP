//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-10-08 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=644p aes=no

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
#include <Blind.h>
#include <Remote.h>
#include <MultiChannelDevice.h>

#define DIR_RELAY_PIN       12 //PD4
#define ON_RELAY_PIN        13 //PD4
#define BTN_PIN_1           14 //PD6
#define BTN_PIN_2           8  //PD0
#define LED_PIN             0  //PB0
#define CONFIG_BUTTON_PIN   15 //PD7
#define CS_PIN              4  //PB4
#define GDO0_PIN            10 //PD2


// number of available peers per channel
#define PEERS_PER_BlindChannel   8
#define PEERS_PER_RemoteChannel  12

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x37, 0x03},// Device ID   lfd Nr 03 vergeben
  "HBBl1PBU03",           // Device Serial lfd Nr 03 vergeben
  {0xf3, 0x37},      // Device Model
  0x10,                   // Firmware Version
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

DEFREGISTER(BlindReg0,MASTERID_REGS,DREG_INTKEY,DREG_CONFBUTTONTIME,DREG_LOCALRESETDISABLE)
class BlindList0 : public RegList0<BlindReg0> {
public:
  BlindList0 (uint16_t addr) : RegList0<BlindReg0>(addr) {}
  void defaults () {
    clear();
    // intKeyVisible(false);
    confButtonTime(0xff);
    // localResetDisable(false);
  }
};

class BlChannel : public ActorChannel<Hal,BlindList1,BlindList3,PEERS_PER_BlindChannel,BlindList0,BlindStateMachine> {
public:
  typedef ActorChannel<Hal,BlindList1,BlindList3,PEERS_PER_BlindChannel,BlindList0,BlindStateMachine> BaseChannel;

  BlChannel () {}
  virtual ~BlChannel () {}

  virtual void switchState(uint8_t oldstate,uint8_t newstate, uint32_t stateDelay) {
    BaseChannel::switchState(oldstate, newstate, stateDelay);
    if( newstate == AS_CM_JT_RAMPON && stateDelay > 0 ) {
      motorUp();
    }
    else if( newstate == AS_CM_JT_RAMPOFF && stateDelay > 0 ) {
      motorDown();
    }
    else {
      motorStop();
    }
  }

  void motorUp () {
    digitalWrite(DIR_RELAY_PIN,HIGH);
    digitalWrite(ON_RELAY_PIN,HIGH);
  }

  void motorDown () {
    digitalWrite(DIR_RELAY_PIN,LOW);
    digitalWrite(ON_RELAY_PIN,HIGH);
  }

  void motorStop () {
    digitalWrite(DIR_RELAY_PIN,LOW);
    digitalWrite(ON_RELAY_PIN,LOW);
  }

  void init () {
    pinMode(ON_RELAY_PIN,OUTPUT);
    pinMode(DIR_RELAY_PIN,OUTPUT);
    motorStop();
    BaseChannel::init();
  }
};

// setup the device with channel type and number of channels
typedef MultiChannelDevice<Hal,BlChannel,1,BlindList0> BlindType;
typedef RemoteChannel<Hal, PEERS_PER_RemoteChannel, BlindList0> BtnChannel;

class MixDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, BlindList0>, 3, BlindList0> {
  public:
    VirtChannel<Hal, BlChannel, BlindList0>  blc1;
    VirtChannel<Hal, BtnChannel, BlindList0> btc1, btc2;

  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, BlindList0>, 3, BlindList0> DeviceType;
    MixDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(blc1, 1);
      DeviceType::registerChannel(btc1, 2);
      DeviceType::registerChannel(btc2, 3);
    }
    virtual ~MixDevice () {}

    BlChannel&  bl1Channel  () {
      return blc1;
    }

    BtnChannel& btn1Channel () {
      return btc1;
    }
    BtnChannel& btn2Channel () {
      return btc2;
    }
};
MixDevice sdev(devinfo, 0x20);
ConfigButton<MixDevice> cfgBtn(sdev);

void initPeerings (bool first) {
  if ( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    sdev.bl1Channel().peer(Peer(devid, 2), Peer(devid, 3));
    //sdev.btn1Channel().peer(Peer(devid, 1));
    //sdev.btn2Channel().peer(Peer(devid, 1));
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.bl1Channel().init();
  remoteChannelISR(sdev.btn1Channel(), BTN_PIN_1);
  remoteChannelISR(sdev.btn2Channel(), BTN_PIN_2);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);

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
