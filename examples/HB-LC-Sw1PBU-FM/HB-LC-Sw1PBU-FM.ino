//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-10-07 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
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
#include <Remote.h>
#include <MultiChannelDevice.h>

#define RELAY_PIN          12 //PD4
#define BTN_PIN_1          14 //PD6
#define BTN_PIN_2          8  //PD0
#define LED_PIN            0  //PB0
#define CONFIG_BUTTON_PIN  15 //PD7
#define CS_PIN             4  //PB4
#define GDO0_PIN           10 //PD2


// number of available peers per channel
#define PEERS_PER_SwitchChannel  8
#define PEERS_PER_RemoteChannel  12

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x35, 0x01},// Device ID
  "HBSw1PBU01",           // Device Serial
  {0xf3, 0x35},      // Device Model
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

DEFREGISTER(Reg0, MASTERID_REGS, DREG_INTKEY, DREG_CYCLICINFOMSG)
class SwList0 : public RegList0<Reg0> {
  public:
    SwList0(uint16_t addr) : RegList0<Reg0>(addr) {}
    void defaults() {
      clear();
      intKeyVisible(true);
    }
};

typedef SwitchChannel<Hal, PEERS_PER_SwitchChannel, SwList0> SwChannel;
typedef RemoteChannel<Hal, PEERS_PER_RemoteChannel, SwList0> BtnChannel;

class MixDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, SwList0>, 3, SwList0> {
  public:
    VirtChannel<Hal, SwChannel, SwList0>  swc1;
    VirtChannel<Hal, BtnChannel, SwList0> btc1, btc2;

  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, SwList0>, 3, SwList0> DeviceType;
    MixDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(swc1, 1);
      DeviceType::registerChannel(btc1, 2);
      DeviceType::registerChannel(btc2, 3);
    }
    virtual ~MixDevice () {}

    SwChannel&  sw1Channel  () {
      return swc1;
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
    sdev.sw1Channel().peer(Peer(devid, 2), Peer(devid, 3));
    sdev.btn1Channel().peer(Peer(devid, 1));
    sdev.btn2Channel().peer(Peer(devid, 1));
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.sw1Channel().init(RELAY_PIN, false);
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
