//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2017-10-24 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-09-29 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER
// #define NDEBUG

// Works with HLW8012 and CSE7759 measurement chip

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <HLW8012.h>
#include <Switch.h>

#define CONFIG_BUTTON_PIN 8

#define HLW_MEASURE_INTERVAL            4
#define POWERMETER_CYCLIC_INTERVAL    120

#define RELAY_PIN                       5
#define SEL_PIN                         9
#define CF1_PIN                         7
#define CF_PIN                          6
#define LED_PIN                         4

// number of available peers per channel
#define PEERS_PER_SWCHANNEL     4
#define PEERS_PER_PMCHANNEL     1
#define PEERS_PER_SENSORCHANNEL 4

// all library classes are placed in the namespace 'as'
using namespace as;

#define CURRENT_MODE                    HIGH
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 470000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k
//#define defaultCurrentMultiplier      13670.9
//#define defaultVoltageMultiplier      441250.69
//#define defaultPowerMultiplier        12168954.98

//#define USE_OWNCALIBRATION
#define CurrentMultiplier             14476.52
#define VoltageMultiplier             427210.87
#define PowerMultiplier               11492903.00

HLW8012 hlw8012;

void hlw8012_cf1_interrupt() {
  hlw8012.cf1_interrupt();
}
void hlw8012_cf_interrupt() {
  hlw8012.cf_interrupt();
}

typedef struct {
  uint32_t E_Counter = 0;
  uint32_t Power     = 0;
  uint16_t Current   = 0;
  uint16_t Voltage   = 0;
  uint8_t  Frequency = 0;
} hlwValues;

hlwValues actualValues ;
hlwValues lastValues;
uint8_t averaging = 1;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0xac, 0x00},     // Device ID
  "HMESPMSw10",           // Device Serial
  {0x00, 0xac},           // Device Model
  0x26,                   // Firmware Version
  as::DeviceType::Switch, // Device Type
  {0x01, 0x00}            // Info Bytes
};

// Configure the used hardware
typedef AvrSPI<10, 11, 12, 13> RadioSPI;
typedef AskSin<StatusLed<LED_PIN>, NoBattery, Radio<RadioSPI, 2> > Hal;
Hal hal;

DEFREGISTER(Reg0, MASTERID_REGS, DREG_INTKEY, DREG_CONFBUTTONTIME, DREG_LOCALRESETDISABLE)
class PMSw1List0 : public RegList0<Reg0> {
  public:
    PMSw1List0(uint16_t addr) : RegList0<Reg0>(addr) {}
    void defaults () {
      clear();
    }
};

bool relayOn() {
  return (digitalRead(RELAY_PIN) == HIGH);
}
typedef SwitchChannel<Hal, PEERS_PER_SWCHANNEL, PMSw1List0> SwChannel;

DEFREGISTER(MReg1, CREG_AES_ACTIVE, CREG_AVERAGING, CREG_TX_MINDELAY, CREG_TX_THRESHOLD_POWER, CREG_TX_THRESHOLD_CURRENT, CREG_TX_THRESHOLD_VOLTAGE, CREG_TX_THRESHOLD_FREQUENCY)
class MeasureList1 : public RegList1<MReg1> {
  public:
    MeasureList1 (uint16_t addr) : RegList1<MReg1>(addr) {}
    void defaults () {
      clear();
      txMindelay(8);
      txThresholdPower(10000);
      txThresholdCurrent(100);
      txThresholdVoltage(100);
      txThresholdFrequency(100);
      averaging(1);
    }
};

DEFREGISTER(SensorReg1, CREG_AES_ACTIVE, CREG_LEDONTIME, CREG_TRANSMITTRYMAX, CREG_COND_TX_THRESHOLD_HI, CREG_COND_TX_THRESHOLD_LO, CREG_COND_TX, CREG_COND_TX_DECISION_ABOVE, CREG_COND_TX_DECISION_BELOW)
class SensorList1 : public RegList1<SensorReg1> {
  public:
    SensorList1 (uint16_t addr) : RegList1<SensorReg1>(addr) {}
    void defaults () {
      clear();
      transmitTryMax(6);
      condTxDecisionAbove(200);
      condTxDecisionBelow(0);
      condTxFalling(false);
      condTxRising(false);
      condTxCyclicBelow(false);
      condTxCyclicAbove(false);
      condTxThresholdHi(0);
      condTxThresholdLo(0);
    }
};

class PowerEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, bool boot, uint32_t e_counter, uint32_t power, uint16_t current, uint16_t voltage, uint8_t frequency ) {

      uint8_t ec1 = (e_counter >> 16) & 0x7f;
      if (boot == true) {
        ec1 |= 0x80;
      }

      Message::init(0x16, msgcnt, 0x5f, BIDI | WKMEUP, ec1, (e_counter >> 8) & 0xff);
      pload[0] = (e_counter) & 0xff;
      pload[1] = (power >> 16) & 0xff;
      pload[2] = (power >> 8) & 0xff;
      pload[3] = (power) & 0xff;
      pload[4] = (current >> 8) & 0xff;
      pload[5] = (current) & 0xff;
      pload[6] = (voltage >> 8) & 0xff;
      pload[7] = (voltage) & 0xff;
      pload[8] = (frequency) & 0xff;
    }
};

class PowerMeterChannel : public Channel<Hal, MeasureList1, EmptyList, List4, PEERS_PER_PMCHANNEL, PMSw1List0>, public Alarm {
    PowerEventMsg msg;
    bool boot;
    uint32_t txThresholdPower;
    uint16_t txThresholdCurrent;
    uint16_t txThresholdVoltage;
    uint8_t txThresholdFrequency;
    uint8_t txMindelay;
  public:
    PowerMeterChannel () : Channel(), Alarm(0), boot(true), txThresholdPower(0), txThresholdCurrent(0), txThresholdVoltage(0), txThresholdFrequency(0), txMindelay(8)   {}
    virtual ~PowerMeterChannel () {}
    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      static uint8_t tickCount = 0;
      tick = seconds2ticks(delay());
      tickCount++;

      bool sendMessage = false;

      if ((txThresholdCurrent > 0)   && (abs((int)(actualValues.Current   - lastValues.Current))   >= txThresholdCurrent))   sendMessage = true;
      if ((txThresholdFrequency > 0) && (abs((int)(actualValues.Frequency - lastValues.Frequency)) >= txThresholdFrequency)) sendMessage = true;
      if ((txThresholdPower > 0)     && (abs((int)(actualValues.Power     - lastValues.Power))     >= txThresholdPower))     sendMessage = true;
      if ((txThresholdVoltage > 0)   && (abs((int)(actualValues.Voltage   - lastValues.Voltage))   >= txThresholdVoltage))   sendMessage = true;

      if ((!sendMessage) && (actualValues.Voltage > 0) && (lastValues.Voltage == 0)) sendMessage = true;

      if (tickCount > (POWERMETER_CYCLIC_INTERVAL / delay()))  {
        sendMessage = true;
        tickCount = 0;
      }

      if ((sendMessage || boot) && (actualValues.Voltage > 0)) {
        //DPRINTLN(F("PowerMeterChannel - SENDING MESSAGE"));
        msg.init(device().nextcount(), boot, actualValues.E_Counter, actualValues.Power, actualValues.Current, actualValues.Voltage, actualValues.Frequency);
        device().sendPeerEvent(msg, *this);
      } else {
        //DPRINTLN(F("PowerMeterChannel - no message to send"));
      }

      lastValues.Current = actualValues.Current;
      lastValues.Frequency = actualValues.Frequency;
      lastValues.Power = actualValues.Power;
      lastValues.Voltage = actualValues.Voltage;
      lastValues.E_Counter = actualValues.E_Counter;

      boot = false;
      sysclock.add(*this);
    }

    uint32_t delay() {
      return max(1, txMindelay);
    }

    void configChanged() {
      DPRINTLN(F("PowerMeterChannel Config changed List1"));
      txThresholdPower     = this->getList1().txThresholdPower();        // 1.00 W = 100
      txThresholdCurrent   = this->getList1().txThresholdCurrent();      // 1 mA   = 1
      txThresholdVoltage   = this->getList1().txThresholdVoltage();      // 10.0V  = 100
      txThresholdFrequency = this->getList1().txThresholdFrequency();    // 1 Hz   = 100
      txMindelay           = this->getList1().txMindelay();
      averaging            = this->getList1().averaging();
      //DPRINT(F("txMindelay           = ")); DDECLN(txMindelay);
      DPRINT(F("txThresholdPower     = ")); DDECLN(txThresholdPower);
      DPRINT(F("txThresholdCurrent   = ")); DDECLN(txThresholdCurrent);
      DPRINT(F("txThresholdVoltage   = ")); DDECLN(txThresholdVoltage);
      //DPRINT(F("txThresholdFrequency = ")); DDECLN(txThresholdFrequency);
    }

    void setup(Device<Hal, PMSw1List0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class SensorChannel : public Channel<Hal, SensorList1, EmptyList, List4, PEERS_PER_SENSORCHANNEL, PMSw1List0>, public Alarm {
  public:

    SensorChannel () : Channel(), Alarm(0) {}
    virtual ~SensorChannel () {}

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      static uint8_t evcnt = 0;
      bool sendMsg = false;
      tick = seconds2ticks(10);

      /* needs too much i.e. use a smaller bootloader on the pro mini
        if (relayOn() == true) {
        SensorEventMsg& rmsg = (SensorEventMsg&)device().message();
        static uint8_t aboveMsgSent = false;
        static uint8_t belowMsgSent = false;
        switch (number()) {
          case 3:
            // Leistungssensor
            if (this->getList1().condTxRising() == true)  {
              if (actualValues.Power > this->getList1().condTxThresholdHi()) {
                if ((aboveMsgSent == false && this->getList1().condTxCyclicAbove() == false) || this->getList1().condTxCyclicAbove() == true) {
                  rmsg.init(device().nextcount(), number(), evcnt++, this->getList1().condTxDecisionAbove(), false , false);
                  sendMsg = true;
                  aboveMsgSent = true;
                }
              } else {
                aboveMsgSent = false;
              }
            }
            if (this->getList1().condTxFalling() == true) {
              if (actualValues.Power < this->getList1().condTxThresholdLo()) {
                if ((belowMsgSent == false && this->getList1().condTxCyclicBelow() == false) || this->getList1().condTxCyclicBelow() == true) {
                  rmsg.init(device().nextcount(), number(), evcnt++, this->getList1().condTxDecisionBelow(), false , false);
                  sendMsg = true;
                  belowMsgSent = true;
                }
              } else {
                belowMsgSent = false;
              }
            }
            break;
          case 4:
            // Stromsensor
            if (this->getList1().condTxRising() == true)  {
              if (actualValues.Current > this->getList1().condTxThresholdHi()) {
                if ((aboveMsgSent == false && this->getList1().condTxCyclicAbove() == false) || this->getList1().condTxCyclicAbove() == true) {
                  rmsg.init(device().nextcount(), number(), evcnt++, this->getList1().condTxDecisionAbove(), false , false);
                  sendMsg = true;
                  aboveMsgSent = true;
                }
              } else {
                aboveMsgSent = false;
              }
            }
            if (this->getList1().condTxFalling() == true) {
              if (actualValues.Current < this->getList1().condTxThresholdLo()) {
                if ((belowMsgSent == false && this->getList1().condTxCyclicBelow() == false) || this->getList1().condTxCyclicBelow() == true) {
                  rmsg.init(device().nextcount(), number(), evcnt++, this->getList1().condTxDecisionBelow(), false , false);
                  sendMsg = true;
                  belowMsgSent = true;
                }
              } else {
                belowMsgSent = false;
              }
            }
            break;
          case 5:
            // Spannungssensor
            if (this->getList1().condTxRising() == true)  {
              if (actualValues.Voltage > this->getList1().condTxThresholdHi()) {
                if ((aboveMsgSent == false && this->getList1().condTxCyclicAbove() == false) || this->getList1().condTxCyclicAbove() == true) {
                  rmsg.init(device().nextcount(), number(), evcnt++, this->getList1().condTxDecisionAbove(), false , false);
                  sendMsg = true;
                  aboveMsgSent = true;
                }
              } else {
                aboveMsgSent = false;
              }
            }
            if (this->getList1().condTxFalling() == true) {
              if (actualValues.Voltage < this->getList1().condTxThresholdLo()) {
                if ((belowMsgSent == false && this->getList1().condTxCyclicBelow() == false) || this->getList1().condTxCyclicBelow() == true) {
                  rmsg.init(device().nextcount(), number(), evcnt++, this->getList1().condTxDecisionBelow(), false , false);
                  sendMsg = true;
                  belowMsgSent = true;
                }
              } else {
                belowMsgSent = false;
              }
            }
            break;
          case 6:
            // Frequenzsensor
            break;
        }

        if (sendMsg) {
          device().sendPeerEvent(rmsg, *this);
          sendMsg = false;
        }
        }
      */
      sysclock.add(*this);
    }

    void configChanged() {
      DPRINT(F("SensorChannel ")); DDEC(number()); DPRINTLN(F(" Config changed List1"));

      //DPRINT(F("transmitTryMax      = ")); DDECLN(this->getList1().transmitTryMax());
      //DPRINT(F("condTxDecisionAbove = ")); DDECLN(this->getList1().condTxDecisionAbove());
      //DPRINT(F("condTxDecisionBelow = ")); DDECLN(this->getList1().condTxDecisionBelow());
      //DPRINT(F("condTxFalling       = ")); DDECLN(this->getList1().condTxFalling());
      //DPRINT(F("condTxRising        = ")); DDECLN(this->getList1().condTxRising());
      //DPRINT(F("condTxCyclicAbove   = ")); DDECLN(this->getList1().condTxCyclicAbove());
      //DPRINT(F("condTxCyclicBelow   = ")); DDECLN(this->getList1().condTxCyclicBelow());
      //DPRINT(F("condTxThresholdHi   = ")); DDECLN(this->getList1().condTxThresholdHi());
      //DPRINT(F("condTxThresholdLo   = ")); DDECLN(this->getList1().condTxThresholdLo());
    }

    void setup(Device<Hal, PMSw1List0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class MixDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, PMSw1List0>, 6, PMSw1List0> {
    class MeasureAlarm : public Alarm {
        MixDevice& dev;
      public:
        MeasureAlarm (MixDevice& d) : Alarm (0), dev(d) {}
        virtual ~MeasureAlarm () {}
        void trigger (AlarmClock& clock)  {

          set(seconds2ticks(HLW_MEASURE_INTERVAL));
          static uint8_t  avgCounter = 0;
          static uint32_t Power      = 0;
          static uint32_t Current    = 0;
          static uint32_t Voltage    = 0;
          static uint32_t Frequency  = 0;

          if (avgCounter < averaging) {
            Power   += hlw8012.getActivePower() * 100;
            Voltage += hlw8012.getVoltage()     * 10;
            Current += hlw8012.getCurrent()     * 1000UL;
            avgCounter++;
          } else {
            actualValues.Power   = relayOn() ? (Power / averaging) : 0;
            actualValues.Voltage = (Voltage / averaging);
            actualValues.Current = relayOn() ? (Current / averaging) : 0;
            avgCounter = 0;
            Power = 0;
            Voltage = 0;
            Current = 0;
          }

          actualValues.E_Counter = (hlw8012.getEnergy()  / 3600.0)   * 10;
          actualValues.Frequency = 0; // not implemented
          DPRINT(F("[HLW] Active Power (W)    : ")); DDECLN(actualValues.Power / 100);
          DPRINT(F("[HLW] Voltage (V)         : ")); DDECLN(actualValues.Voltage / 10);
          DPRINT(F("[HLW] Current (mA)        : ")); DDECLN(actualValues.Current);
          DPRINT(F("[HLW] Agg. energy (Wh)*10 : ")); DDECLN(actualValues.E_Counter);

          clock.add(*this);
        }
    } hlwMeasure;
  public:
    VirtChannel<Hal, SwChannel,         PMSw1List0> c1;
    VirtChannel<Hal, PowerMeterChannel, PMSw1List0> c2;
    VirtChannel<Hal, SensorChannel,     PMSw1List0> c3, c4, c5, c6;
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, PMSw1List0>, 6, PMSw1List0> DeviceType;
    MixDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr), hlwMeasure(*this) {
      DeviceType::registerChannel(c1, 1);
      DeviceType::registerChannel(c2, 2);
      DeviceType::registerChannel(c3, 3);
      DeviceType::registerChannel(c4, 4);
      DeviceType::registerChannel(c5, 5);
      DeviceType::registerChannel(c6, 6);
      sysclock.cancel(hlwMeasure);
      hlwMeasure.set(seconds2ticks(HLW_MEASURE_INTERVAL));
      sysclock.add(hlwMeasure);
    }
    virtual ~MixDevice () {}

    SwChannel& switchChannel ()  {
      return c1;
    }
    PowerMeterChannel& powermeterChannel ()  {
      return c2;
    }
    SensorChannel& sensorChannel3Power ()  {
      return c3;
    }
    SensorChannel& sensorChannel4Current ()  {
      return c4;
    }
    SensorChannel& sensorChannel5Voltage ()  {
      return c5;
    }
    SensorChannel& sensorChannel6Frequency ()  {
      return c6;
    }
    virtual void configChanged () {
      DPRINTLN(F("MixDevice Config changed List0"));
    }
};
MixDevice sdev(devinfo, 0x20);
ConfigToggleButton<MixDevice> cfgBtn(sdev);

void initPeerings (bool first) {
  HMID devid;
  sdev.getDeviceID(devid);
  Peer ipeer(devid, 1);
  sdev.channel(1).peer(ipeer);
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.switchChannel().init(RELAY_PIN, false);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  initPeerings(first);
  sdev.initDone();
  if ( digitalPinToInterrupt(CF1_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(CF1_PIN, hlw8012_cf1_interrupt, CHANGE); else attachInterrupt(digitalPinToInterrupt(CF1_PIN), hlw8012_cf1_interrupt, CHANGE);
  if ( digitalPinToInterrupt(CF_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(CF_PIN, hlw8012_cf_interrupt, CHANGE); else attachInterrupt(digitalPinToInterrupt(CF_PIN), hlw8012_cf_interrupt, CHANGE);
  hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true);
  hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);

#ifdef USE_OWNCALIBRATION
  hlw8012.setCurrentMultiplier(CurrentMultiplier);
  hlw8012.setVoltageMultiplier(VoltageMultiplier);
  hlw8012.setPowerMultiplier(PowerMultiplier);
#endif
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    // hal.activity.savePower<Idle<true> >(hal);
  }
}
