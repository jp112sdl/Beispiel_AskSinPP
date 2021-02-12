//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-02-17 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Switch.h>


#define CONFIG_BUTTON_PIN 8
#define RELAY1_PIN 3
#define RELAY2_PIN 4
#define RELAY3_PIN 5
#define RELAY4_PIN 6
#define RELAY5_PIN 7
#define RELAY6_PIN 9
#define RELAY7_PIN 14
#define RELAY8_PIN 15
#define RELAY9_PIN 16
#define RELAY10_PIN 17
#define RELAY11_PIN 18
#define RELAY12_PIN 19

// number of available peers per channel
#define PEERS_PER_CHANNEL 2

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0xf3,0x20,0x00},       // Device ID
    "JPSW120001",           // Device Serial
    {0xf3,0x20},            // Device Model
    0x10,                   // Firmware Version
    as::DeviceType::Switch, // Device Type
    {0x01,0x00}             // Info Bytes
};

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> RadioSPI;
typedef AskSin<NoLed,NoBattery,Radio<RadioSPI,2> > Hal;

typedef MultiChannelDevice<Hal,SwitchChannel<Hal,PEERS_PER_CHANNEL,List0>,12> SwitchType;
Hal hal;
SwitchType sdev(devinfo,0x20);
ConfigButton<SwitchType> cfgBtn(sdev);

void initPeerings (bool first) {
  // create internal peerings - CCU2 needs this
  if( first == true ) {
    HMID devid;
    sdev.getDeviceID(devid);
    for( uint8_t i=1; i<=sdev.channels(); ++i ) {
      Peer ipeer(devid,i);
      sdev.channel(i).peer(ipeer);
    }
  }
}

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.channel(1).init(RELAY1_PIN);
  sdev.channel(2).init(RELAY2_PIN);
  sdev.channel(3).init(RELAY3_PIN);
  sdev.channel(4).init(RELAY4_PIN);
  sdev.channel(5).init(RELAY5_PIN);
  sdev.channel(6).init(RELAY6_PIN);
  sdev.channel(7).init(RELAY7_PIN);
  sdev.channel(8).init(RELAY8_PIN);
  sdev.channel(9).init(RELAY9_PIN);
  sdev.channel(10).init(RELAY10_PIN);
  sdev.channel(11).init(RELAY11_PIN);
  sdev.channel(12).init(RELAY12_PIN);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  initPeerings(first);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    hal.activity.savePower<Idle<> >(hal);
  }
}
