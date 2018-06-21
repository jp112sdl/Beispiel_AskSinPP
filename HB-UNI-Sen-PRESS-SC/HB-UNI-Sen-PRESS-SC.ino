//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-05-14 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <MultiChannelDevice.h>
#include <ThreeState.h>

// Arduino Pro mini 8 Mhz
// Arduino pin for the config button
#define CONFIG_BUTTON_PIN 8
// analoger Wert vom Drucksensor
#define SENSOR_PIN        14
// Status LED
#define LED_PIN           4
// Taster, um manuelle eine Messung auszulösen
#define ISR_PIN           9
// Ausgang Trigger SC-Kontakt ->
#define SC_TRIGGER_PIN    6
// -> Eingang Trigger SC-Kontakt
#define SC_PIN            5
// Offen/Verschlossen (H/L) Pegel vertauschen
#define SC_INVERT         false

// number of available peers per channel
#define PEERS_PER_CHANNEL      6
#define PEERS_PER_SCCHANNEL    8

//Korrekturfaktor der Clock-Ungenauigkeit, wenn keine RTC verwendet wird
#define SYSCLOCK_FACTOR    1.00 //0.88 im Sleep, nur bei Batteriebetrieb

#define ANALOG_SOCKET_VALUE 108

// all library classes are placed in the namespace 'as'
using namespace as;

volatile bool isrDetected = false;
#define sendISR(pin) class sendISRHandler { public: static void isr () { isrDetected = true; } }; pinMode(pin, INPUT_PULLUP); if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) enableInterrupt(pin,sendISRHandler::isr,RISING); else attachInterrupt(digitalPinToInterrupt(pin),sendISRHandler::isr,RISING);

enum sc_states {
  TRG_OFF,
  TRG_ON
};

enum PressureSensorTypes {
  MPA_1_2,
  MPA_0_5
};

const struct DeviceInfo PROGMEM devinfo = {
  {0xE9, 0x02, 0x01},          // Device ID
  "JPPRESSSC1",                // Device Serial
  {0xE9, 0x02},                // Device Model
  0x10,                        // Firmware Version
  0x53,    // Device Type
  {0x01, 0x01}                 // Info Bytes
};

typedef AskSin<StatusLed<LED_PIN>, NoBattery, Radio<AvrSPI<10, 11, 12, 13>, 2> > Hal;
Hal hal;

DEFREGISTER(UReg0, MASTERID_REGS, DREG_BURSTRX, 0x21, 0x22)
class UList0 : public RegList0<UReg0> {
  public:
    UList0 (uint16_t addr) : RegList0<UReg0>(addr) {}

    bool Sendeintervall (uint16_t value) const {
      return this->writeRegister(0x21, (value >> 8) & 0xff) && this->writeRegister(0x22, value & 0xff);
    }
    uint16_t Sendeintervall () const {
      return (this->readRegister(0x21, 0) << 8) + this->readRegister(0x22, 0);
    }

    void defaults () {
      clear();
      burstRx(false);
      Sendeintervall(60);
    }
};

DEFREGISTER(UReg1, 0x23, 0x24, 0x25, 0x26, 0x27)
class UList1 : public RegList1<UReg1> {
  public:
    UList1 (uint16_t addr) : RegList1<UReg1>(addr) {}

    bool PressureLimit (uint16_t value) const {
      return this->writeRegister(0x23, (value >> 8) & 0xff) && this->writeRegister(0x24, value & 0xff);
    }
    uint16_t PressureLimit () const {
      return (this->readRegister(0x23, 0) << 8) + this->readRegister(0x24, 0);
    }

    bool PressureHysteresis (uint16_t value) const {
      return this->writeRegister(0x25, (value >> 8) & 0xff) && this->writeRegister(0x26, value & 0xff);
    }
    uint16_t PressureHysteresis () const {
      return (this->readRegister(0x25, 0) << 8) + this->readRegister(0x26, 0);
    }

    bool PressureSensorType (uint8_t value)  const {
      this->writeRegister(0x27, value & 0xff);
    }
    uint8_t PressureSensorType () const {
      return this->readRegister(0x27, 0);
    }

    void defaults () {
      PressureLimit(0);
      PressureHysteresis(0);
      PressureSensorType(0);
      clear();
    }
};

DEFREGISTER(Reg1, CREG_AES_ACTIVE, CREG_MSGFORPOS, CREG_EVENTDELAYTIME, CREG_LEDONTIME, CREG_TRANSMITTRYMAX)
class SCList1 : public RegList1<Reg1> {
  public:
    SCList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
    void defaults () {
      clear();
      msgForPosA(1);
      msgForPosB(2);
      aesActive(false);
      eventDelaytime(0);
      ledOntime(100);
      transmitTryMax(6);
    }
};

typedef ThreeStateChannel<Hal, UList0, SCList1, DefList4, PEERS_PER_SCCHANNEL> SCChannel;

class PressureChannel : public Channel<Hal, UList1, EmptyList, List4, PEERS_PER_CHANNEL, UList0>, public Alarm {
    uint16_t         pressure;
    uint16_t         pressureLimit;
    uint16_t         pressureHysteresis;
    uint8_t          pressureSensorType;
    uint16_t         s_count;
    uint16_t         sc_state;
  public:

    PressureChannel () : Channel(), Alarm(0), s_count(0), sc_state(TRG_OFF) {}
    virtual ~PressureChannel () {}

    void measure() {

      uint16_t analogValue = 0;
      for (uint8_t i = 0; i < 10; i++) {
        analogValue += analogRead(SENSOR_PIN);
        _delay_ms(5);
      }
      analogValue = analogValue / 10;

      float sensor_factor;
      switch (pressureSensorType) {
        case MPA_1_2:
          sensor_factor = 0.75;
          break;
        case MPA_0_5:
          sensor_factor = 1.6;
          break;
        default:
          break;
      }

      float _p = (((analogValue / 1024.0) - (ANALOG_SOCKET_VALUE / 1000.0)) / sensor_factor) * 1000;
      pressure = _p > 0 ? _p : 0;
      DPRINT(F("+Pressure  (#")); DDEC(number()); DPRINT(F(") Analogwert: ")); DDECLN(analogValue);
      DPRINT(F("+Pressure  (#")); DDEC(number()); DPRINT(F(")       mBar: ")); DDECLN(pressure * 10);

      if (pressure > pressureLimit + pressureHysteresis) {
        if (sc_state == TRG_ON) {
          scChanged(TRG_OFF);
        }
      }

      if (pressure < pressureLimit - pressureHysteresis) {
        if (sc_state == TRG_OFF) {
          scChanged(TRG_ON);
        }
      }
    }

    void irq() {
      sysclock.cancel(*this);
      measure();
      processMessage();
      sysclock.add(*this);
    }

    void scChanged(uint8_t state = TRG_OFF) {
      sc_state = state;
      digitalWrite(SC_TRIGGER_PIN, (SC_INVERT) ? !sc_state : sc_state);
      processMessage();
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      set(seconds2ticks(SYSCLOCK_FACTOR));

      measure();

      s_count++;
      if (s_count % device().getList0().Sendeintervall() == 0) {
        s_count = 0;
        processMessage();
      }

      sysclock.add(*this);
    }

    void processMessage() {
      DPRINT(F("processMessage():: pressure=")); DDECLN(pressure);
      tick = delay();
      Message& msg = (Message&)this->device().message();
      msg.init(0x0b, this->device().nextcount(), 0x53, Message::BCAST, (pressure >> 8) & 0xff, pressure & 0xff);
      device().sendPeerEvent(msg, *this);
    }

    uint32_t delay() {
      return seconds2ticks(SYSCLOCK_FACTOR);
    }

    void configChanged() {
      DPRINTLN(F("Config changed List1"));
      pressureLimit = this->getList1().PressureLimit();
      DPRINT(F("*PressureLimit:      ")); DDECLN(pressureLimit);
      pressureHysteresis = this->getList1().PressureHysteresis();
      DPRINT(F("*PressureHysteresis: ")); DDECLN(pressureHysteresis);
      pressureSensorType = this->getList1().PressureSensorType();
      DPRINT(F("*pressureSensorType: ")); DDECLN(pressureSensorType);
    }

    void setup(Device<Hal, UList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      pinMode(SENSOR_PIN, INPUT);
      pinMode(SC_TRIGGER_PIN, OUTPUT);
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class UType : public ChannelDevice<Hal, VirtBaseChannel<Hal, UList0>, 2, UList0> {
  public:
    VirtChannel<Hal, PressureChannel, UList0>   channel1;
    VirtChannel<Hal, SCChannel, UList0>         channel2;
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, UList0>, 2, UList0> DeviceType;

    UType (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(channel1, 1);
      DeviceType::registerChannel(channel2, 2);
    }
    virtual ~UType () {}

    PressureChannel& pressureChannel ()  {
      return channel1;
    }

    SCChannel& scChannel ()  {
      return channel2;
    }

    virtual void configChanged () {
      DeviceType::configChanged();
      DPRINT(F("*Wake-On-Radio: "));
      DDECLN(this->getList0().burstRx());
      DPRINT(F("*Sendeintervall: ")); DDECLN(this->getList0().Sendeintervall());
    }
};

UType sdev(devinfo, 0x20);
ConfigButton<UType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  printDeviceInfo();
  sdev.init(hal);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sendISR(ISR_PIN);
  const uint8_t posmap[4] = {Position::State::PosA, Position::State::PosB, Position::State::PosA, Position::State::PosB};
  sdev.scChannel().init(SC_PIN, SC_PIN, posmap);
  sdev.initDone();
  sdev.pressureChannel().irq();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();

  if ( worked == false && poll == false ) {
    if (isrDetected) {
      DPRINTLN(F("manual button pressed"));
      sdev.pressureChannel().irq();
      isrDetected = false;
    }
    hal.activity.savePower<Idle<>>(hal);
  }
}

void printDeviceInfo() {
  HMID ids;
  sdev.getDeviceID(ids);

  uint8_t ser[10];
  sdev.getDeviceSerial(ser);

  DPRINT(F("Device Info: "));
  for (int i = 0; i < 10; i++) {
    DPRINT(char(ser[i]));
  }
  DPRINT(" ("); DHEX(ids); DPRINTLN(")");
}


