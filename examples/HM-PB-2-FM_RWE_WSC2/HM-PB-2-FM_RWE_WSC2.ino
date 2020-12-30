//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-12-26 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER
//#define USE_AES
//#define HM_DEF_KEY ...
//#define HM_DEF_KEY_INDEX 0

//#define NDEBUG

#define STORAGEDRIVER at24cX<0x50,128,32> 
#include <Wire.h>

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <MultiChannelDevice.h>
#include <Remote.h>

#define LED          8
#define CC1101_GDO0  2
#define CC1101_PWR   5
#define CONFIG_BTN   0
#define BTN01_PIN    15
#define BTN02_PIN    14
#define ACTIVATE_PIN 9

#define PEERS_PER_CHANNEL 10

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x00,0xc2,0x01},       // Device ID
    "JPWSCRWE01",           // Device Serial
    {0x00,0xc2},            // Device Model
    0x14,                   // Firmware Version
    as::DeviceType::Remote, // Device Type
    {0x00,0x00}             // Info Bytes
};

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,CC1101_GDO0> RadioType;
typedef StatusLed<LED> LedType;
typedef AskSin<LedType,IrqInternalBatt,RadioType> Hal;

typedef RemoteChannel<Hal,PEERS_PER_CHANNEL,List0> ChannelType;
typedef MultiChannelDevice<Hal,ChannelType,2> RemoteType;

Hal hal;
RemoteType sdev(devinfo,0x20);
ConfigButton<RemoteType> cfgBtn(sdev);

void setup () {
  pinMode(ACTIVATE_PIN, OUTPUT);
  digitalWrite(ACTIVATE_PIN, HIGH);
  pinMode(CC1101_PWR, OUTPUT);
  digitalWrite(CC1101_PWR, LOW);

  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  _delay_ms(200);
  sdev.init(hal);
  hal.battery.init(0,sysclock);
  hal.battery.low(22);
  hal.battery.critical(19);

  buttonISR(cfgBtn,CONFIG_BTN);
  remoteISR(sdev,1,BTN01_PIN);
  remoteISR(sdev,2,BTN02_PIN);

  while (hal.battery.current() == 0);

  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    hal.activity.savePower<Sleep<>>(hal);
  }
}
