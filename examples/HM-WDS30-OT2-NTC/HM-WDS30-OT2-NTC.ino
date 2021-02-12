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
#include <MultiChannelDevice.h>
#include <sensors/Ntc.h>

// Arduino Pro mini 8 Mhz
// Arduino pin for the config button
#define CONFIG_BUTTON_PIN 8
// Arduino pin for the StatusLed
#define LED_PIN           4
// number of available peers per channel
#define PEERS_PER_CHANNEL 6

// send all xx seconds
#define MSG_INTERVAL 120

// temperature where ntc has resistor value of R0
#define NTC_T0 25
// resistance both of ntc and known resistor
#define NTC_R0 10000
// b value of ntc (see datasheet)
#define NTC_B 3435

// number of additional bits by oversampling (should be between 0 and 6, highly increases number of measurements)
#define NTC_OVERSAMPLING 2

// pin to measure first ntc
#define SENSOR0_SENSE_PIN 14
// pin to power first ntc (or 0 if connected to vcc)
#define SENSOR0_ACTIVATOR_PIN 6

// pin to measure second ntc
#define SENSOR1_SENSE_PIN 15
// pin to power second ntc (or 0 if connected to vcc)
#define SENSOR1_ACTIVATOR_PIN 5

// all library classes are placed in the namespace 'as'
using namespace as;

enum tParams {INACTIVE, T1, T2, T1sT2, T2sT1};

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0xa8, 0x02},          // Device ID
  "JPOT200002",               // Device Serial
  {0x00, 0xa8},              // Device Model
  0x10,                       // Firmware Version
  as::DeviceType::THSensor,   // Device Type
  {0x01, 0x01}               // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> Hal;
Hal hal;

DEFREGISTER(Reg0, MASTERID_REGS, DREG_BURSTRX, DREG_TPARAMS, DREG_CYCLICINFOMSGDIS, DREG_LOCALRESETDISABLE)
class UList0 : public RegList0<Reg0> {
  public:
    UList0(uint16_t addr) : RegList0<Reg0>(addr) {}
    void defaults () {
      clear();
      localResetDisable(false);
      burstRx(false);
      tParamSelect(3);
      cyclicInfoMsgDis(0);
    }
};

class MeasureEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int tempValues[4], bool batlow) {
      Message::init(0x1a, msgcnt, 0x53, (msgcnt % 20 == 1) ? (BIDI | WKMEUP) : BCAST, batlow ? 0x80 : 0x00, 0x41);
      for (int i = 0; i < 4; i++) {
        pload[i * 3] = (tempValues[i] >> 8) & 0xff;
        pload[(i * 3) + 1] = (tempValues[i]) & 0xff;
        pload[(i * 3) + 2] = 0x42 + i;
      }
    }
};

class WeatherChannel : public Channel<Hal, List1, EmptyList, List4, PEERS_PER_CHANNEL, UList0> {
  public:
    WeatherChannel () : Channel() {}
    virtual ~WeatherChannel () {}

    void configChanged() {
      //DPRINTLN("Config changed List1");
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class UType : public MultiChannelDevice<Hal, WeatherChannel, 5, UList0> {

    class SensorArray : public Alarm {
        UType& dev;
        Ntc<SENSOR0_SENSE_PIN,NTC_R0,NTC_B,SENSOR0_ACTIVATOR_PIN,NTC_T0,NTC_OVERSAMPLING> sensor0;
        Ntc<SENSOR1_SENSE_PIN,NTC_R0,NTC_B,SENSOR1_ACTIVATOR_PIN,NTC_T0,NTC_OVERSAMPLING> sensor1;

      public:
        int tempValues[4] = {0, 0, 0, 0};
        SensorArray (UType& d) : Alarm(0), dev(d) { }

        virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
          tick = delay();
          sysclock.add(*this);

          sensor0.measure();
          sensor1.measure();

          tempValues[0] = sensor0.temperature();
          tempValues[1] = sensor1.temperature();

          tempValues[2] = tempValues[0] - tempValues[1];
          tempValues[3] = tempValues[1] - tempValues[0];

          DPRINT("T:");DDEC(tempValues[0]);DPRINT(";");DDEC(tempValues[1]);DPRINT(";");DDEC(tempValues[2]);DPRINT(";");DDECLN(tempValues[3]);

          MeasureEventMsg& msg = (MeasureEventMsg&)dev.message();

          msg.init(dev.nextcount(), tempValues, dev.battery().low());
          dev.send(msg, dev.getMasterID());
        }

        uint32_t delay () {
          return seconds2ticks(MSG_INTERVAL * (dev.getList0().cyclicInfoMsgDis() + 1));
        }

    } sensarray;

  public:
    typedef MultiChannelDevice<Hal, WeatherChannel, 5, UList0> TSDevice;
    UType(const DeviceInfo& info, uint16_t addr) : TSDevice(info, addr), sensarray(*this) {}
    virtual ~UType () {}

    virtual void configChanged () {
      TSDevice::configChanged();
      DPRINTLN("Config Changed List0");
      DPRINT("Wake-On-Radio: "); DDECLN(this->getList0().burstRx());
      DPRINT("tParamSelect: "); DDECLN(this->getList0().tParamSelect());
      DPRINT("localResetDisable: "); DDECLN(this->getList0().localResetDisable());
      DPRINT("cyclicInfoMsgDis: "); DDECLN(this->getList0().cyclicInfoMsgDis());
    }

    bool init (Hal& hal) {
      TSDevice::init(hal);
      sensarray.set(seconds2ticks(5));
      sysclock.add(sensarray);
    }
};

UType sdev(devinfo, 0x20);
ConfigButton<UType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  hal.initBattery(60UL * 60, 22, 19);
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
