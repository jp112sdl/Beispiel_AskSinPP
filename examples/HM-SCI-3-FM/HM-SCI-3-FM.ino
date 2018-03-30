//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <ThreeState.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED1_PIN 4
#define LED2_PIN 5
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

// Anzahl Kanäle (3 - 7)
#define CHANNEL_COUNT 3
// Pin für Kanal        1   2   3   4  5  6  7 
uint8_t sens_pins[] = {14, 15, 16, 17, 3, 6, 9 };

// number of available peers per channel
#define PEERS_PER_CHANNEL 10
#define CYCLETIME seconds2ticks(60UL*60*16)

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x07, 0x5F, 0x02},     // Device ID
  "JPSCI30002",           // Device Serial
  {0x00, 0x5F},           // Device Model
  0x01,                   // Firmware Version
  as::DeviceType::Swi,    // Device Type
  {0x00, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef DualStatusLed<LED2_PIN, LED1_PIN> LedType;
typedef AskSin<LedType, BatterySensorUni<A6,7>, RadioType> BaseHal;
class Hal : public BaseHal {
  public:
    void init (const HMID& id) {
      BaseHal::init(id);
      // measure battery every 1h
      battery.init(seconds2ticks(60UL * 60), sysclock);
    }
} hal;

DEFREGISTER(Reg0, DREG_INTKEY, DREG_CYCLICINFOMSG, MASTERID_REGS, DREG_TRANSMITTRYMAX)
class SCIList0 : public RegList0<Reg0> {
  public:
    SCIList0(uint16_t addr) : RegList0<Reg0>(addr) {}
    void defaults () {
      clear();
      cycleInfoMsg(true);
      transmitDevTryMax(6);
    }
};

DEFREGISTER(Reg1, CREG_AES_ACTIVE, CREG_MSGFORPOS, CREG_EVENTDELAYTIME, CREG_LEDONTIME, CREG_TRANSMITTRYMAX)
class SCIList1 : public RegList1<Reg1> {
  public:
    SCIList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
    void defaults () {
      clear();
      msgForPosA(1);
      msgForPosB(2);
      //aesActive(false);
      eventDelaytime(0);
      //ledOntime(100);
      transmitTryMax(6);
    }
};

typedef ThreeStateChannel<Hal, SCIList0, SCIList1, DefList4, PEERS_PER_CHANNEL> ChannelType;
typedef ThreeStateDevice<Hal, ChannelType, CHANNEL_COUNT, SCIList0> SCIType;

SCIType sdev(devinfo, 0x20);
ConfigButton<SCIType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  const uint8_t posmap[4] = {Position::State::PosA, Position::State::PosB, Position::State::PosA, Position::State::PosB};

  for (int i = 0; i < CHANNEL_COUNT; i++) {
      sdev.channel(i+1).init(sens_pins[i], sens_pins[i], posmap);
  }
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    // deep discharge protection
    // if we drop below critical battery level - switch off all and sleep forever
    if ( hal.battery.critical() ) {
      // this call will never return
      hal.activity.sleepForever(hal);
    }
    // if nothing to do - go sleep
    hal.activity.savePower<Sleep<> >(hal);
  }
}

