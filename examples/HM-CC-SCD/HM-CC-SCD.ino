//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define CFG_STEPUP_BYTE 0x00
#define CFG_STEPUP_OFF  0x00
#define CFG_STEPUP_ON   0x01

#define CFG_BAT_LOW_BYTE 0x01
#define CFG_BAT_CRITICAL_BYTE 0x02

// define device configuration bytes
#define DEVICE_CONFIG CFG_STEPUP_OFF,22,19

// 24 0030 4D455130323134373633 80 910101

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
// SENS1 SENS2
//   0    0 = normal
//   0    1 = erhöht
//   1    0 = stark erhöht
#define SENS1_PIN 14
#define SENS2_PIN 15

// number of available peers per channel
#define PEERS_PER_CHANNEL 10
#define CYCLETIME seconds2ticks(60UL * 60 * 16)

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x13,0x56,0x34},       // Device ID
    "JPCCSCD001",           // Device Serial
    {0x00,0x56},            // Device Model
    0x10,                   // Firmware Version
    as::DeviceType::ThreeStateSensor, // Device Type
    {0x01,0x00}             // Info Bytes
};

class SwitchSensor {
  InternalVCC internal;
  ExternalVCC<17,7,LOW,3000> external;
  uint8_t mod;
public:
  typedef uint16_t ValueType;
  static const int DefaultDelay = 250;

  SwitchSensor() : mod(0) {}

  void mode (uint8_t m) {
    mod = m;
    init();
  }

  void init () {
    if( mod == 0 ) {
      internal.init();
      DPRINTLN("InternalVCC");
    }
    else {
      external.init();
      DPRINTLN("Stepup - ExternalVCC");
    }
  }

  void start () {
    if( mod == 0 ) internal.start();
    else external.start();
  }

  uint16_t finish () {
    return mod == 0 ? internal.finish() : external.finish();
  }
};

typedef BattSensor<AsyncMeter<SwitchSensor> > BatSensor;

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,2> RadioType;
typedef DualStatusLed<LED2_PIN,LED1_PIN> LedType;
typedef AskSin<LedType,BatSensor,RadioType> BaseHal;
class Hal : public BaseHal {
public:
  void init (const HMID& id) {
    BaseHal::init(id);
    // measure battery every 1h
    battery.init(seconds2ticks(60UL*60),sysclock);
  }
} hal;

DEFREGISTER(Reg0,DREG_INTKEY,DREG_CYCLICINFOMSG,MASTERID_REGS,DREG_TRANSMITTRYMAX)
class SCDList0 : public RegList0<Reg0> {
public:
  SCDList0(uint16_t addr) : RegList0<Reg0>(addr) {}
  void defaults () {
    clear();
    cycleInfoMsg(true);
    transmitDevTryMax(6);
  }
};

DEFREGISTER(Reg1,CREG_AES_ACTIVE,CREG_MSGFORPOS,CREG_EVENTDELAYTIME,CREG_LEDONTIME,CREG_TRANSMITTRYMAX)
class SCDList1 : public RegList1<Reg1> {
public:
  SCDList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
  void defaults () {
    clear();
    msgForPosA(1);
    msgForPosB(2); 
    msgForPosC(3); 
    // aesActive(false);
    // eventDelaytime(0);
    ledOntime(100);
    transmitTryMax(6);
  }
};


typedef ThreeStateChannel<Hal,SCDList0,SCDList1,DefList4,PEERS_PER_CHANNEL> ChannelType;
class SCDType : public ThreeStateDevice<Hal,ChannelType,1,SCDList0,CYCLETIME> {
public:
  typedef ThreeStateDevice<Hal,ChannelType,1,SCDList0,CYCLETIME> TSDevice;
  SCDType(const DeviceInfo& info,uint16_t addr) : TSDevice(info,addr) {}
  virtual ~SCDType () {}

  virtual void configChanged () {
    TSDevice::configChanged();
    // set battery low/critical values
    battery().low(getConfigByte(CFG_BAT_LOW_BYTE));
    battery().critical(getConfigByte(CFG_BAT_CRITICAL_BYTE));
    // set the battery mode
    battery().meter().sensor().mode(getConfigByte(CFG_STEPUP_BYTE));
  }
};

SCDType sdev(devinfo,0x20);
ConfigButton<SCDType> cfgBtn(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  const uint8_t posmap[4] = {Position::State::PosC,Position::State::PosC,Position::State::PosB,Position::State::PosA};
  sdev.channel(1).init(SENS1_PIN,SENS2_PIN,posmap);
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
