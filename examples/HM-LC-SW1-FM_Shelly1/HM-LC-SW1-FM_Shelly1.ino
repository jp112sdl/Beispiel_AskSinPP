//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-10-21 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Switch.h>

#define LED_PIN           4
#define RELAY1_PIN        5
#define CONFIG_BUTTON_PIN 8
#define PEERS_PER_CHANNEL 12


// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0x04, 0x00},     // Device ID
  "JPSW1FM000",           // Device Serial
  {0x00, 0x04},           // Device Model
  0x16,                   // Firmware Version
  as::DeviceType::Switch, // Device Type
  {0x01, 0x00}            // Info Bytes
};


typedef AskSin<StatusLed<LED_PIN>, NoBattery, Radio<AvrSPI<10, 11, 12, 13> , 2> > Hal;
typedef MultiChannelDevice<Hal, SwitchChannel<Hal, PEERS_PER_CHANNEL, List0>, 1> SwitchType;

Hal hal;
SwitchType sdev(devinfo, 0x20);
ConfigToggleButton<SwitchType, LOW, HIGH, INPUT> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);

  sdev.channel(1).init(RELAY1_PIN, false);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);

  HMID devid;
  sdev.getDeviceID(devid);
  Peer ipeer(devid, 1);
  sdev.channel(1).peer(ipeer);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Idle<> >(hal);
  }
}
