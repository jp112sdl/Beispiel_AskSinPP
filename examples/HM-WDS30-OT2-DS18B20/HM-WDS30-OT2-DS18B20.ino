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

#include <OneWire.h>
#include <sensors/Ds18b20.h>

// Arduino Pro mini 8 Mhz
// Arduino pin for the config button
#define CONFIG_BUTTON_PIN 8
#define LED_PIN           4

// number of available peers per channel
#define PEERS_PER_CHANNEL 6

//DS18B20 Sensors connected to pin
OneWire oneWire(3);

// send all xx seconds
#define MSG_INTERVAL 180

// all library classes are placed in the namespace 'as'
using namespace as;

enum tParams {INACTIVE, T1, T2, T1sT2, T2sT1};

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0xa8, 0x01},          // Device ID
  "JPOT200001",               // Device Serial
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
    void init(uint8_t msgcnt, uint16_t tempValues[4], bool batlow) {
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

      public:
        uint8_t       sensorcount;
        Ds18b20       sensors[4];

        // initialized values shown as -150°
        uint16_t tempValues[4] = {0xD8F1, 0xD8F1, 0xD8F1, 0xD8F1};
        SensorArray (UType& d) : Alarm(0), dev(d) {}

        virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
          tick = delay();
          sysclock.add(*this);

          if (sensorcount > 0) {
            Ds18b20::measure(sensors, sensorcount);
            // 1 sensors gives 1*temp, no difference
            tempValues[0] = sensors[0].temperature();

            // 2 sensors are classical 2*temp, +difference(1-2), - difference(2-1)
            if (sensorcount == 2) {
              tempValues[1] = sensors[1].temperature();
              tempValues[2] = tempValues[0] - tempValues[1];
              tempValues[3] = tempValues[1] - tempValues[0];
            }
            // 3 sensors are 2*temp, +difference(1-2), 3rd temp
            if (sensorcount == 3) {
              tempValues[1] = sensors[1].temperature();
              tempValues[2] = tempValues[0] - tempValues[1];
              tempValues[3] = sensors[2].temperature();
            }
            // 4 sensors are 4*temp no difference
            if (sensorcount == 4) {
              tempValues[1] = sensors[1].temperature();
              tempValues[2] = sensors[2].temperature();
              tempValues[3] = sensors[3].temperature();
            }

            MeasureEventMsg& msg = (MeasureEventMsg&)dev.message();

            msg.init(dev.nextcount(), tempValues, dev.battery().low());
            dev.send(msg, dev.getMasterID());
          }
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
      // up to 4 sensors
      sensarray.sensorcount = Ds18b20::init(oneWire, sensarray.sensors, 4);
      DPRINT("Found "); DDEC(sensarray.sensorcount); DPRINTLN(" DS18B20 Sensors");
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
