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
#include <sensors/Bh1750.h>

#include <MultiChannelDevice.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED_PIN 4
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8

// number of available peers per channel
#define PEERS_PER_CHANNEL 6

//seconds between sending messages
#define MSG_INTERVAL 20

// all library classes are placed in the namespace 'as'
using namespace as;


// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x34, 0xfd, 0x01},     // Device ID
  "JPLIO00001",           // Device Serial
  {0x00, 0xfd},           // Device Model
  0x01,                   // Firmware Version
  0x53, // Device Type
  {0x01, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef StatusLed<4> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> BaseHal;
class Hal : public BaseHal {
  public:
    void init (const HMID& id) {
      BaseHal::init(id);
      // measure battery every 1h
      battery.init(seconds2ticks(60UL *60), sysclock);
      battery.low(22);
      battery.critical(19);
    }

    bool runready () {
      return sysclock.runready() || BaseHal::runready();
    }
} hal;

DEFREGISTER(LiReg0,MASTERID_REGS,DREG_CYCLICINFOMSGDIS,DREG_LOCALRESETDISABLE,DREG_TRANSMITTRYMAX)
class LiList0 : public RegList0<LiReg0> {
public:
  LiList0 (uint16_t addr) : RegList0<LiReg0>(addr) {}
  void defaults () {
    clear();
    //cyclicInfoMsgDis(0);
    // intKeyVisible(false);
    // localResetDisable(false);
  }
};

DEFREGISTER(LiReg1,CREG_AES_ACTIVE,CREG_TX_MINDELAY,CREG_TX_THRESHOLD_PERCENT)
class LiList1 : public RegList1<LiReg1> {
public:
  LiList1 (uint16_t addr) : RegList1<LiReg1>(addr) {}
  void defaults () {
    clear();
    // aesActive(false);
    txMindelay(8);
    //txThresholdPercent(0);
  }
};

class LuxEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, uint32_t lux) {
      Message::init(0xf, msgcnt, 0x53, RPTEN | BCAST, 0x00, 0xc1);
      pload[0] = (lux >> 32)  & 0xff;
      pload[1] = (lux >> 16) & 0xff;
      pload[2] = (lux >> 8) & 0xff;
      pload[3] = (lux) & 0xff;
    }
};

class LuxChannel : public Channel<Hal, LiList1, EmptyList, List4, PEERS_PER_CHANNEL, LiList0>, public Alarm {

    LuxEventMsg   lmsg;
    uint32_t      lux;
    uint16_t      millis;

    Bh1750<>     bh1750;

  public:
    LuxChannel () : Channel(), Alarm(5), lux(0), millis(0) {}
    virtual ~LuxChannel () {}

    // here we do the measurement
    void measure () {
      DPRINT("Measure... ");
      bh1750.measure();
      lux = bh1750.brightness();
      DDEC(lux);
      DPRINTLN(" lux");
    }

    uint8_t flags () const {
      uint8_t flags = this->device().battery().low() ? 0x80 : 0x00;
      return flags;
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      tick = delay();
      clock.add(*this);
      
      measure();

      this->changed(true);
      lmsg.init(msgcnt, lux * 100);
      device().sendPeerEvent(lmsg, *this);
    }

    uint32_t delay () {
      return seconds2ticks(MSG_INTERVAL);
    }
    void setup(Device<Hal, LiList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
      bh1750.init();
    }

    uint8_t status () const {
      return 0;
    }
};


class LuxType : public MultiChannelDevice<Hal,LuxChannel,1,LiList0> {
public:
  typedef MultiChannelDevice<Hal,LuxChannel,1,LiList0> TSDevice;
  LuxType(const DeviceInfo& info,uint16_t addr) : TSDevice(info,addr) {}
  virtual ~LuxType () {}

  virtual void configChanged () {
    TSDevice::configChanged();
    DPRINTLN("config changed");
    DDECLN(getConfigByte(0x00)); // ???
  }
};


LuxType sdev(devinfo, 0x20);
ConfigButton<LuxType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Sleep<>>(hal);
  }
}

