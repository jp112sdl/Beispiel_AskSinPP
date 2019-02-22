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
#define MSG_INTERVAL 400

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0x9f, 0x01},     // Device ID
  "JPWAOD0001",           // Device Serial
  {0x00, 0x9f},           // Device Model
  0x10,                   // Firmware Version
  as::DeviceType::THSensor, // Device Type
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

DEFREGISTER(WaOdReg0, MASTERID_REGS, DREG_CYCLICINFOMSGDIS, DREG_LOCALRESETDISABLE, DREG_TRANSMITTRYMAX)
class WaOdList0 : public RegList0<WaOdReg0> {
  public:
    WaOdList0 (uint16_t addr) : RegList0<WaOdReg0>(addr) {}
    void defaults () {
      clear();
      // cyclicInfoMsgDis(0);
      transmitDevTryMax(6);
      // localResetDisable(false);
    }
};

DEFREGISTER(WaOdReg1, CREG_WATER_UPPER_THRESHOLD_CH, CREG_WATER_LOWER_THRESHOLD_CH, CREG_CASE_DESIGN, CREG_CASE_HIGH, CREG_FILL_LEVEL, CREG_CASE_WIDTH, CREG_CASE_LENGTH, CREG_MEASURE_LENGTH, CREG_USE_CUSTOM_CALIBRATION, CREG_LEDONTIME, CREG_TRANSMITTRYMAX)
class WaOdList1 : public RegList1<WaOdReg1> {
  public:
    WaOdList1 (uint16_t addr) : RegList1<WaOdReg1>(addr) {}
    void defaults () {
      clear();
      caseLength(100);
      caseWidth(100);
    }
};

class FillingEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, uint8_t fill) {
      Message::init(0xc, msgcnt, 0x41, BIDI, 0x01, msgcnt);
      pload[0] = fill;
    }
};

class FillingChannel : public Channel<Hal, WaOdList1, EmptyList, List4, PEERS_PER_CHANNEL, WaOdList0>, public Alarm {

    FillingEventMsg msg;
    int16_t         fill;
    uint16_t        millis;
    uint8_t last_flags = 0xff;

  public:
    FillingChannel () : Channel(), Alarm(5), fill(0), millis(0) {}
    virtual ~FillingChannel () {}

    // here we do the measurement
    void measure () {
      fill = 170;
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      tick = delay();
      clock.add(*this);

      if (last_flags != flags()) {
        this->changed(true);
        last_flags = flags();
      }

      measure();

      msg.init(msgcnt, fill);
      device().sendPeerEvent(msg, *this);
    }

    uint32_t delay () {
      return seconds2ticks(MSG_INTERVAL);
    }
    void setup(Device<Hal, WaOdList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }


    uint8_t status () const {
      return fill;
    }

    void configChanged() {
      DPRINTLN(F("*List1 changed"));
      DPRINT(F("*ledOntime ")); DDECLN(this->getList1().ledOntime());
      DPRINT(F("*transmitTryMax ")); DDECLN(this->getList1().transmitTryMax());
      DPRINT(F("*waterUpperThreshold ")); DDECLN(this->getList1().waterUpperThreshold());
      DPRINT(F("*waterLowerThreshold ")); DDECLN(this->getList1().waterLowerThreshold());
      DPRINT(F("*caseDesign ")); DDECLN(this->getList1().caseDesign());
      DPRINT(F("*caseHigh ")); DDECLN(this->getList1().caseHigh());
      DPRINT(F("*caseWidth ")); DDECLN(this->getList1().caseWidth());
      DPRINT(F("*caseLength ")); DDECLN(this->getList1().caseLength());
      DPRINT(F("*measureLength ")); DDECLN(this->getList1().measureLength());
      DPRINT(F("*fillLevel ")); DDECLN(this->getList1().fillLevel());
      DPRINT(F("*useCustomCalibration ")); DDECLN(this->getList1().useCustomCalibration());
    }

    uint8_t flags () const {
      uint8_t flags = this->device().battery().low() ? 0x80 : 0x00;
      return flags;
    }
};

typedef MultiChannelDevice<Hal, FillingChannel, 1, WaOdList0> WaOdType;
WaOdType sdev(devinfo, 0x20);

ConfigButton<WaOdType> cfgBtn(sdev);

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
