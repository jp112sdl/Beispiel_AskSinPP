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

//#include <sensors/Bh1750.h>
//#include <sensors/Tsl2561.h>
#include <sensors/Max44009.h>

#define SENSOR_CLASS MAX44009<>
//#define SENSOR_CLASS Bh1750<>
//#define SENSOR_CLASS Tsl2561<>                   // Brücke zwischen L und GND
//#define SENSOR_CLASS Tsl2561<TSL2561_ADDR_HIGH>  // Brücke zwischen H und GND
//#define SENSOR_CLASS Tsl2561<TSL2561_ADDR_FLOAT> // keine Brücke gesetzt

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

#define LUX_EVENT_CYCLIC_TIME 120 //seconds to send cyclic message
#define LUX_EVENT_CYCLIC      0x53
#define LUX_EVENT             0x54

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
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> BaseHal;
class Hal : public BaseHal {
  public:
    void init (const HMID& id) {
      BaseHal::init(id);
      // measure battery every 1h
      battery.init(seconds2ticks(60UL * 60), sysclock);
      battery.low(22);
      battery.critical(19);
    }

    bool runready () {
      return sysclock.runready() || BaseHal::runready();
    }
} hal;

DEFREGISTER(LiReg0, MASTERID_REGS, DREG_TRANSMITTRYMAX, DREG_CYCLICINFOMSGDIS, DREG_LOCALRESETDISABLE, DREG_INTKEY)
class LiList0 : public RegList0<LiReg0> {
  public:
    LiList0 (uint16_t addr) : RegList0<LiReg0>(addr) {}
    void defaults () {
      clear();
      transmitDevTryMax(1);
      //cyclicInfoMsgDis(0);
      // intKeyVisible(false);
      // localResetDisable(false);
    }
};

DEFREGISTER(LiReg1, CREG_AES_ACTIVE, CREG_TX_MINDELAY, CREG_TX_THRESHOLD_PERCENT)
class LiList1 : public RegList1<LiReg1> {
  public:
    LiList1 (uint16_t addr) : RegList1<LiReg1>(addr) {}
    void defaults () {
      clear();
      aesActive(false);
      txMindelay(8);
      //txThresholdPercent(0);
    }
};

class LuxEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, uint32_t lux, uint8_t type) {
      Message::init(0xf, msgcnt, type, RPTEN|WKMEUP, 0x00, 0xc1);
      pload[0] = (lux >> 24)  & 0xff;
      pload[1] = (lux >> 16) & 0xff;
      pload[2] = (lux >> 8) & 0xff;
      pload[3] = (lux) & 0xff;
    }
};

template <class SENSOR>
class LuxChannel : public Channel<Hal, LiList1, EmptyList, List4, PEERS_PER_CHANNEL, LiList0>, public Alarm {

    LuxEventMsg lmsg;
    uint32_t    lux;
    uint32_t    lux_prev;
    uint16_t    millis;

    SENSOR      sens;
    uint8_t     last_flags;
    uint8_t     cyclic_cnt;
    uint8_t     cyclic_dis_cnt;

  public:
    LuxChannel () : Channel(), Alarm(5), lux(0), lux_prev(0), millis(0), last_flags(0xff), cyclic_cnt(0), cyclic_dis_cnt(0) {}
    virtual ~LuxChannel () {}

    // here we do the measurement
    void measure () {
      DPRINT("Measure... ");
      sens.measure();
      lux = sens.brightness();
      DDEC(lux);
      DPRINTLN(" lux");
    }

    uint8_t flags () const {
      uint8_t flags = this->device().battery().low() ? 0x80 : 0x00;
      return flags;
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      cyclic_cnt++;

      uint8_t txMindelay = max(8,this->getList1().txMindelay());
      //DPRINT(F("TX_MINDELAY: ")); DDECLN(txMindelay);

      // reactivate for next measure
      set(seconds2ticks(txMindelay));
      clock.add(*this);

      // measure brightness
      measure();

      // if battery low is reached, send a message immediately
      if (last_flags != flags()) {
        this->changed(true);
        last_flags = flags();
      }

      bool sendMsg = false;

      uint8_t msgType = LUX_EVENT_CYCLIC;

      // send message as LUX_EVENT, but every 3 minutes as LUX_EVENT_CYCLIC
      uint8_t cyclicInfoMsgDis = device().getList0().cyclicInfoMsgDis();
      if (cyclic_cnt * txMindelay >= LUX_EVENT_CYCLIC_TIME) {

        if (cyclicInfoMsgDis == 0 || cyclic_dis_cnt >= cyclicInfoMsgDis) {
          sendMsg = true;
          cyclic_dis_cnt = 0;
        } else {
          cyclic_dis_cnt++;
        }

        cyclic_cnt = 0;
      }

      // check if lux is above/below threshold (if configured)
      uint8_t txThresholdPercent = this->getList1().txThresholdPercent();
      DPRINT(F("thresholdPcnt pcnt: "));DDECLN(txThresholdPercent);
      if (txThresholdPercent > 0) {  // a threshold is configured
        uint8_t pcnt = (lux > 0) ? min(abs(100.0 / (lux_prev) * lux - 100), 100) : 0;
        DPRINT(F("lux changed   pcnt: "));DDECLN(pcnt);
        if (pcnt >= txThresholdPercent) { // the calculated percentage between lux_prev and lux is greater or equal to the configured txThresholdPercent
          lux_prev = lux;                 // save the current lux in lux_prev
          sendMsg = true;
          msgType = LUX_EVENT;
        }
      }

      if (sendMsg == true) {
        lmsg.init(device().nextcount(), lux * 100, msgType);

        if (msgType == LUX_EVENT_CYCLIC) {
          device().broadcastEvent(lmsg, *this);
        } else {
          lmsg.setAck();
          device().sendPeerEvent(lmsg, *this);
        }

      }

    }

    void setup(Device<Hal, LiList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
      sens.init();
    }

    uint8_t status () const {
      return 0;
    }
};

typedef MultiChannelDevice<Hal,LuxChannel<SENSOR_CLASS>, 1, LiList0> LuxType;

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
    if ( hal.battery.critical() ) {
      hal.activity.sleepForever(hal);
    }
    hal.activity.savePower<Sleep<>>(hal);
  }
}

