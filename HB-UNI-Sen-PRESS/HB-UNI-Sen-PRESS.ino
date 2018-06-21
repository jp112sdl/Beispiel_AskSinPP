//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-04-16 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <MultiChannelDevice.h>

// Arduino Pro mini 8 Mhz
// Arduino pin for the config button
#define CONFIG_BUTTON_PIN 8
#define ISR_PIN           9
#define LED_PIN           4

// number of available peers per channel
#define PEERS_PER_CHANNEL 6

#define ANALOG_SOCKET_VALUE 110

// all library classes are placed in the namespace 'as'
using namespace as;

//Korrekturfaktor der Clock-Ungenauigkeit, wenn keine RTC verwendet wird
#define SYSCLOCK_FACTOR    1.00 //0.88 im Sleep, nur bei Batteriebetrieb

volatile bool isrDetected = false;

//SENSOR_EN_PIN und SENSOR_PIN sind immer paarweise und kommagetrennt hinzuzufügen:
//Beispiel für 3 Sensoren
//byte SENSOR_PINS[]         {14, 15, 16}; //AOut Pin des Sensors

byte SENSOR_EN_PINS[]      {5}; //VCC Pin des Sensors
byte SENSOR_PINS[]         {14}; //AOut Pin des Sensors

enum PressureSensorTypes {
  MPA_1_2,
  MPA_0_5
};

#define sendISR(pin) class sendISRHandler { \
    public: \
      static void isr () { isrDetected = true; } \
  }; \
  pinMode(pin, INPUT_PULLUP); \
  if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
    enableInterrupt(pin,sendISRHandler::isr,RISING); \
  else \
    attachInterrupt(digitalPinToInterrupt(pin),sendISRHandler::isr,RISING);

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0xE9, 0x01, 0x01},          // Device ID
  "JPPRESS001",                // Device Serial
  {0xE9, 0x01},                // Device Model
  0x10,                        // Firmware Version
  0x53,    // Device Type
  {0x01, 0x01}                 // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> RadioSPI;
typedef AskSin<StatusLed<LED_PIN>, NoBattery, Radio<RadioSPI, 2> > Hal;
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

    bool PressureSensorType (uint8_t value)  const {
      this->writeRegister(0x27, value & 0xff);
    }
    uint8_t PressureSensorType () const {
      return this->readRegister(0x27, 0);
    }

    void defaults () {
      clear();
    }
};

class WeatherEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, uint8_t channel, uint16_t pressure) {

      Message::init(0x0e, msgcnt, 0x53, BCAST , 0x00, 0xc1);
      pload[0] = channel  & 0xff;
      pload[1] = (pressure >> 8) & 0xff;
      pload[2] = pressure & 0xff;
    }
};

class WeatherChannel : public Channel<Hal, UList1, EmptyList, List4, PEERS_PER_CHANNEL, UList0>, public Alarm {
    WeatherEventMsg msg;
    uint16_t        millis;
    uint16_t        pressure;

  public:
    WeatherChannel () : Channel(), Alarm(0), millis(0) {}
    virtual ~WeatherChannel () {}

    void measure() {
      digitalWrite(SENSOR_EN_PINS[(number() - 1)], HIGH);
      _delay_ms(500);
      uint16_t sens_val = 0;
      for (uint8_t i = 0; i < 10; i++) {
        sens_val += analogRead(SENSOR_PINS[(number() - 1)]);
        _delay_ms(5);
      }
      sens_val = sens_val / 10;
      digitalWrite(SENSOR_EN_PINS[(number() - 1)], LOW);

      float sensor_factor = 0.75;
      switch (this->getList1().PressureSensorType()) {
        case MPA_1_2:
          sensor_factor = 0.75;
          break;
        case MPA_0_5:
          sensor_factor = 1.6;
          break;
        default:
          break;
      }

      float _p = (((sens_val / 1024.0) - (ANALOG_SOCKET_VALUE / 1000.0)) / sensor_factor) * 1000;
      pressure = _p > 0 ? _p : 0;
      DPRINT(F("+Pressure  (#")); DDEC(number()); DPRINT(F(") Analogwert: ")); DDECLN(sens_val);
      DPRINT(F("+Pressure  (#")); DDEC(number()); DPRINT(F(")       mBar: ")); DDECLN(pressure * 10);
    }

    void irq () {
      sysclock.cancel(*this);
      processMessage();
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      processMessage();
    }

    void processMessage() {
      measure();
      tick = delay();
      msg.init(device().nextcount(), number(), pressure);
      device().sendPeerEvent(msg, *this);
      sysclock.add(*this);
    }

    uint32_t delay () {
      uint16_t _txMindelay = 20;
      _txMindelay = device().getList0().Sendeintervall();
      if (_txMindelay == 0) _txMindelay = 20;
      return seconds2ticks(_txMindelay  * SYSCLOCK_FACTOR);
    }

    void configChanged() {
      DPRINTLN(F("Config changed List1"));
      //pressureSensorType = this->getList1().PressureSensorType();
      //DPRINT(F("*pressureSensorType: ")); DDECLN(pressureSensorType);
    }

    void setup(Device<Hal, UList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      pinMode(SENSOR_PINS[ number - 1 ], INPUT);
      pinMode(SENSOR_EN_PINS[ number - 1 ], OUTPUT);
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

class UType : public MultiChannelDevice<Hal, WeatherChannel, sizeof(SENSOR_PINS), UList0> {
  public:
    typedef MultiChannelDevice<Hal, WeatherChannel, sizeof(SENSOR_PINS), UList0> TSDevice;
    UType(const DeviceInfo& info, uint16_t addr) : TSDevice(info, addr) {}
    virtual ~UType () {}

    virtual void configChanged () {
      TSDevice::configChanged();
      DPRINT(F("*Wake-On-Radio: "));
      DDECLN(this->getList0().burstRx());
      DPRINT(F("*Sendeintervall: ")); DDECLN(this->getList0().Sendeintervall());
    }
};

UType sdev(devinfo, 0x20);
ConfigButton<UType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  if (sizeof(SENSOR_PINS) != sizeof(SENSOR_EN_PINS)) {
    DPRINTLN(F("!!! ERROR: Anzahl SENSOR_PINS entspricht nicht der Anzahl SENSOR_EN_PINS"));
  } else {

    printDeviceInfo();
    sdev.init(hal);
    buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
#ifdef ISR_PIN
    sendISR(ISR_PIN);
#endif
    sdev.initDone();
  }
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();

  if ( worked == false && poll == false ) {
    if (isrDetected) {
      DPRINTLN(F("manual button pressed"));
      for (int i = 1; i <= sizeof(SENSOR_PINS); i++) {
        sdev.channel(i).irq();
      }
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


