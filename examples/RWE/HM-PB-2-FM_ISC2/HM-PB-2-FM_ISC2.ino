//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-02-12 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-02-12 re-vo-lution Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <MultiChannelDevice.h>
#include <Remote.h>

#define LED_PIN           8
#define CONFIG_BUTTON_PIN 0
#define BTN1_PIN          15
#define BTN2_PIN          14
#define CC1101_PWR_PIN    5
#define CC1101_GDO0_PIN   2


// number of available peers per channel
#define PEERS_PER_CHANNEL 16

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x02, 0xBF, 0x01},     // Device ID - 01 vergeben
  "RWEISC2001",           // Device Serial - 01 vergeben
  {0x00, 0xBF},           // Device Model
  0x14,                   // Firmware Version
  as::DeviceType::Remote, // Device Type
  {0x00, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> RadioSPI;
typedef Radio<RadioSPI, CC1101_PWR_PIN, CC1101_PWR_PIN> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, IrqInternalBatt, RadioType> Hal;

typedef RemoteChannel<Hal, PEERS_PER_CHANNEL, List0> ChannelType;
typedef MultiChannelDevice<Hal, ChannelType, 2> RemoteType;

Hal hal;
RemoteType sdev(devinfo, 0x20);
ConfigButton<RemoteType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  hal.battery.init();
  hal.battery.low(22);
  hal.battery.critical(19);
  
  remoteISR(sdev, 1, BTN1_PIN);
  remoteISR(sdev, 2, BTN2_PIN);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  
  while (hal.battery.current() == 0);

  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if (worked == false && poll == false ) {
    hal.activity.savePower<Sleep<>>(hal);
  }
}
