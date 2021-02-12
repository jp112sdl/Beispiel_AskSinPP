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

#include <Register.h>
#include <ContactState.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED1_PIN 4
#define LED2_PIN 5
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

#define SENS_PIN      9
#define SENS_EN_PIN   6
#define SENS_EN_WAIT  50 //millis to wait after enable pin = high
#define SABOTAGE_PIN  7

// number of available peers per channel
#define PEERS_PER_CHANNEL 10
#define CYCLETIME seconds2ticks(60UL * 60 * 1)

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x03,0xc7,0x01},       // Device ID
    "JPSCO00001",           // Device Serial
    {0x00,0xC7},            // Device Model
    0x06,                   // Firmware Version
    as::DeviceType::ThreeStateSensor, // Device Type
    {0x01,0x00}             // Info Bytes
};

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,2> RadioType;
typedef DualStatusLed<LED2_PIN,LED1_PIN> LedType;
typedef AskSin<LedType,IrqInternalBatt,RadioType> BaseHal;
class Hal : public BaseHal {
public:
  void init (const HMID& id) {
    BaseHal::init(id);
    // measure battery every 1h
    battery.init(seconds2ticks(60UL*60),sysclock);
  }
} hal;

DEFREGISTER(Reg0,DREG_INTKEY,DREG_CYCLICINFOMSG,MASTERID_REGS,DREG_TRANSMITTRYMAX,DREG_SABOTAGEMSG)
class SCOList0 : public RegList0<Reg0> {
public:
  SCOList0(uint16_t addr) : RegList0<Reg0>(addr) {}
  void defaults () {
    clear();
    cycleInfoMsg(true);
    transmitDevTryMax(6);
    sabotageMsg(true);
  }
};

DEFREGISTER(Reg1,CREG_AES_ACTIVE,CREG_MSGFORPOS,CREG_EVENTDELAYTIME,CREG_LEDONTIME,CREG_TRANSMITTRYMAX)
class SCOList1 : public RegList1<Reg1> {
public:
  SCOList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
  void defaults () {
    clear();
    msgForPosA(1); // CLOSED
    msgForPosB(2); // OPEN
    aesActive(false);
    eventDelaytime(0);
    ledOntime(100);
    transmitTryMax(6);
  }
};

typedef TwoStateChannel<Hal,SCOList0,SCOList1,DefList4,PEERS_PER_CHANNEL, SENS_EN_WAIT> ChannelType;
typedef StateDevice<Hal,ChannelType,1,SCOList0, CYCLETIME> SCOType;

SCOType sdev(devinfo,0x20);
ConfigButton<SCOType> cfgBtn(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  sdev.channel(1).init(SENS_PIN, SENS_EN_PIN, SABOTAGE_PIN);
  while (hal.battery.current() == 0);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    // deep discharge protection
    // if we drop below critical battery level - switch off all and sleep forever
    if( hal.battery.critical() ) {
      // this call will never return
      hal.activity.sleepForever(hal);
    }
    // if nothing to do - go sleep
    hal.activity.savePower<Sleep<> >(hal);
  }
}
