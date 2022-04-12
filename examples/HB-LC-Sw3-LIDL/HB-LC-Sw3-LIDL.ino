//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa     Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-09-26 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Switch.h>

#define LED_PIN           4  //LED
#define RELAY1_PIN        5  //K1  
#define RELAY2_PIN        6  //K2
#define RELAY3_PIN        7  //K3
#define CONFIG_BUTTON_PIN 8  //Switch

#define RELAY_ACTIVE_LOW false

#define PEERS_PER_CHANNEL 8

using namespace as;
const struct DeviceInfo PROGMEM devinfo = {
    {0xf3,0x26,0x01},       // Device ID
    "JHLCSW3L01",           // Device Serial
    {0xf3,0x26},            // Device Model
    0x16,                   // Firmware Version
    as::DeviceType::Switch, // Device Type
    {0x01,0x00}             // Info Bytes
};
typedef AvrSPI<10,11,12,13> RadioSPI;
typedef AskSin<StatusLed<LED_PIN>,NoBattery,Radio<RadioSPI,2> > Hal;
typedef MultiChannelDevice<Hal,SwitchChannel<Hal,PEERS_PER_CHANNEL,List0>,3> SwitchType;
Hal hal;
SwitchType sdev(devinfo,0x20);
ConfigButton<SwitchType> cfgBtn(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  sdev.channel(1).init(RELAY1_PIN,RELAY_ACTIVE_LOW);
  sdev.channel(2).init(RELAY2_PIN,RELAY_ACTIVE_LOW);
  sdev.channel(3).init(RELAY3_PIN,RELAY_ACTIVE_LOW);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  sdev.led().invert(true);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    hal.activity.savePower<Idle<> >(hal);
  }
}
