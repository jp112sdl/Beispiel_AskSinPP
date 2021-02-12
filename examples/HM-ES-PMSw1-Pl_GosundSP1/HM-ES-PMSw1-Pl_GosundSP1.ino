//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2017-10-24 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-09-29 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER
// #define NDEBUG
// #define USE_HW_SERIAL
//#define HJLDEBUG // Print measurement values
//#define USE_CC1101_ALT_FREQ_86835  //when using 'bad' cc1101 module

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <HLW8012.h>
#include <Switch.h>

#define HLW_MEASURE_INTERVAL            1
#define POWERMETER_CYCLIC_INTERVAL    120

// use MightyCore Standard pinout for ATmega644PA

// connection to Gosund
#define RELAY_PIN                       12    // high-active
#define BUTTON_PIN                      13
#define LED_BLUE_PIN                    14    // low-active
#define LED_RED_PIN                     15    // low-active

// connection to HLW8012
#define SEL_PIN                         17
#define CF1_PIN                         10
#define CF_PIN                          11

// connection to CC1101
#define CS_PIN                          4
#define MOSI_PIN                        5
#define MISO_PIN                        6
#define CLK_PIN                         7
#define GDO0_PIN                        2

// number of available peers per channel
#define PEERS_PER_SWCHANNEL     8
#define PEERS_PER_PMCHANNEL     8
#define PEERS_PER_SENSORCHANNEL 8

// all library classes are placed in the namespace 'as'
using namespace as;

#define CURRENT_MODE                    LOW
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 2 * 100000 ) // ( 2 * 100000 ) = Wert aus 'espurna' // ( 5 * 470000 ) = Wert aus Lib-Beispiel
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 )     // Real 1.009k

#define CurrentMultiplier             22361   //25740   // Wert aus 'espurna'
#define VoltageMultiplier             327831  //313400  // Wert aus 'espurna'
#define PowerMultiplier               3476844 //3414290 // Wert aus 'espurna'

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
bool    resetAverageCounting = false;
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
typedef AvrSPI<CS_PIN, MOSI_PIN, MISO_PIN, CLK_PIN> RadioSPI;
typedef AskSin<StatusLed<LED_BLUE_PIN>, NoBattery, Radio<RadioSPI, GDO0_PIN> > BaseHal;
typedef StatusLed<LED_RED_PIN> RedLedType;

class Hal: public BaseHal {
  public:
    void init(const HMID& id) {
      BaseHal::init(id);
#ifdef USE_CC1101_ALT_FREQ_86835
      // 2165E8 == 868.35 MHz
      radio.initReg(CC1101_FREQ2, 0x21);
      radio.initReg(CC1101_FREQ1, 0x65);
      radio.initReg(CC1101_FREQ0, 0xE8);
#endif
    }

    bool runready () {
      return sysclock.runready() || BaseHal::runready();
    }
} hal;

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
//typedef SwitchChannel<Hal, PEERS_PER_SWCHANNEL, PMSw1List0> SwChannel;
class SwChannel : public SwitchChannel<Hal, PEERS_PER_SWCHANNEL, PMSw1List0>  {

  protected:
    typedef SwitchChannel<Hal, PEERS_PER_SWCHANNEL, PMSw1List0> BaseChannel;
    RedLedType RedLed;

  public:
    SwChannel () : BaseChannel() {}
    virtual ~SwChannel() {}

    void init (uint8_t p) {
      RedLed.init();
      RedLed.invert(true);
      BaseChannel::init(p);
    }

    virtual void switchState(__attribute__((unused)) uint8_t oldstate, uint8_t newstate,uint32_t delay) {
      BaseChannel::switchState(oldstate, newstate, delay);
      if ( newstate == AS_CM_JT_ON ) {
        resetAverageCounting = true;
        RedLed.ledOn();
      }
      else if ( newstate == AS_CM_JT_OFF ) {
        RedLed.ledOff();
      }
    }
};


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
    void init(uint8_t msgcnt, uint8_t typ, bool boot, uint32_t e_counter, uint32_t power, uint16_t current, uint16_t voltage, uint8_t frequency ) {

      uint8_t ec1 = (e_counter >> 16) & 0x7f;
      if (boot == true) {
        ec1 |= 0x80;
      }

      Message::init(0x16, msgcnt, typ, (typ == AS_MESSAGE_POWER_EVENT_CYCLIC) ? BCAST : BIDI | BCAST, ec1, (e_counter >> 8) & 0xff);
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
    PowerMeterChannel () : Channel(), Alarm(0), boot(false), txThresholdPower(0), txThresholdCurrent(0), txThresholdVoltage(0), txThresholdFrequency(0), txMindelay(8) {}
    virtual ~PowerMeterChannel () {}
    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      static uint8_t tickCount = 0;
      tick = seconds2ticks(delay());
      tickCount++;

      uint8_t msgType = 0;

      if (tickCount > (POWERMETER_CYCLIC_INTERVAL / delay()) || boot == false)  {
        msgType = AS_MESSAGE_POWER_EVENT_CYCLIC;
      }

      if ((txThresholdCurrent   > 0) && (abs((int)(actualValues.Current   - lastValues.Current)  ) >= (int)txThresholdCurrent))   msgType = AS_MESSAGE_POWER_EVENT;
      if ((txThresholdFrequency > 0) && (abs((int)(actualValues.Frequency - lastValues.Frequency)) >= (int)txThresholdFrequency)) msgType = AS_MESSAGE_POWER_EVENT;
      if ((txThresholdPower     > 0) && (abs((int)(actualValues.Power     - lastValues.Power)    ) >= (int)txThresholdPower))     msgType = AS_MESSAGE_POWER_EVENT;
      if ((txThresholdVoltage   > 0) && (abs((int)(actualValues.Voltage   - lastValues.Voltage)  ) >= (int)txThresholdVoltage))   msgType = AS_MESSAGE_POWER_EVENT;

      if ((msgType != AS_MESSAGE_POWER_EVENT) && (actualValues.Voltage > 0) && (lastValues.Voltage == 0)) msgType = AS_MESSAGE_POWER_EVENT;

      msg.init(device().nextcount(), msgType, boot, actualValues.E_Counter, actualValues.Power, actualValues.Current, actualValues.Voltage, actualValues.Frequency);
      switch (msgType) {
        case AS_MESSAGE_POWER_EVENT_CYCLIC:
          DPRINTLN(F("PowerMeterChannel - SENDING CYCLIC MESSAGE"));
          tickCount = 0;
          device().broadcastEvent(msg);
        break;
        case AS_MESSAGE_POWER_EVENT:
          if (device().getMasterID() > 0) {
            DPRINTLN(F("PowerMeterChannel - SENDING EVENT MESSAGE"));
            device().sendMasterEvent(msg);
          }
        break;
      }

      lastValues.Current = actualValues.Current;
      lastValues.Frequency = actualValues.Frequency;
      lastValues.Power = actualValues.Power;
      lastValues.Voltage = actualValues.Voltage;
      lastValues.E_Counter = actualValues.E_Counter;

      boot = true;
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
      DPRINT(F("txMindelay           = ")); DDECLN(txMindelay);
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
          static uint32_t Power      = 0;
          static uint32_t Current    = 0;
          static uint32_t Voltage    = 0;
          static uint8_t  avgCounter = 0;
          //static uint32_t Frequency  = 0; //unused

          if (resetAverageCounting == true) {
            //DPRINTLN(F("*** RESETTING AVERAGING ***"));
            resetAverageCounting = false;
            avgCounter = 0;
            Power = 0;
            Voltage = 0;
            Current = 0;
          }

          if (avgCounter < averaging) {
            Power   += hlw8012.getActivePower() * 100UL;
            Voltage += hlw8012.getVoltage()     * 10UL;
            Current += hlw8012.getCurrent()     * 1000UL;
            avgCounter++;
          } else {
            actualValues.Power   = relayOn() ? (Power / averaging) : 0;
            actualValues.Voltage = (Voltage / averaging);
            actualValues.Current = relayOn() ? (Current / averaging) : 0;
            resetAverageCounting = true;

#ifdef HJLDEBUG
            DPRINT(F("[HJL-01] Active Power (W)    : ")); DDECLN(actualValues.Power / 100);
            DPRINT(F("[HJL-01] Voltage (V)         : ")); DDECLN(actualValues.Voltage / 10);
            DPRINT(F("[HJL-01] Current (mA)        : ")); DDECLN(actualValues.Current);
            DPRINT(F("[HJL-01] Agg. energy (Wh)*10 : ")); DDECLN(actualValues.E_Counter);
#endif
          }

          actualValues.E_Counter = (hlw8012.getEnergy()  / 3600.0)   * 10;
          actualValues.Frequency = 0; // not implemented

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
  if( first == true ) {
    sdev.switchChannel().peer(cfgBtn.peer());
  }
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);
  sdev.switchChannel().init(RELAY_PIN);
  buttonISR(cfgBtn, BUTTON_PIN);
  initPeerings(first);
  sdev.led().invert(true);
  sdev.initDone();
  DDEVINFO(sdev);
  if ( digitalPinToInterrupt(CF1_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(CF1_PIN, hlw8012_cf1_interrupt, FALLING); else attachInterrupt(digitalPinToInterrupt(CF1_PIN), hlw8012_cf1_interrupt, FALLING);
  if ( digitalPinToInterrupt(CF_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(CF_PIN, hlw8012_cf_interrupt, FALLING); else attachInterrupt(digitalPinToInterrupt(CF_PIN), hlw8012_cf_interrupt, FALLING);
  hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true);
  hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);

  hlw8012.setCurrentMultiplier(CurrentMultiplier);
  hlw8012.setVoltageMultiplier(VoltageMultiplier);
  hlw8012.setPowerMultiplier(PowerMultiplier);
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    // hal.activity.savePower<Idle<true> >(hal);
  }
}
