//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-01-29 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define NDEBUG
#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include <Register.h>
#include <ContactState.h>

#define LED_PIN      8
#define CONFIG_BTN   0
#define SENS1_PIN    A0
#define CC1101_PWR   5
#define CYCLETIME seconds2ticks(60UL * 60 * 16)
#define BAT_LOW      22
#define BAT_CRIT     20

#define PEERS_PER_CHANNEL 10

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x00,0x2f,0x01},       // Device ID
    "JPWDS00001",           // Device Serial
    {0x00,0x2F},            // Device Model
    0x18,                   // Firmware Version
    as::DeviceType::ThreeStateSensor, // Device Type
    {0x01,0x00}             // Info Bytes
};

typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,2, CC1101_PWR> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType,IrqInternalBatt,RadioType> Hal;
Hal hal;

DEFREGISTER(Reg0,DREG_INTKEY,DREG_CYCLICINFOMSG,MASTERID_REGS,DREG_TRANSMITTRYMAX,DREG_SABOTAGEMSG)
class SCList0 : public RegList0<Reg0> {
public:
  SCList0(uint16_t addr) : RegList0<Reg0>(addr) {}
  void defaults () {
    clear();
    cycleInfoMsg(true);
    transmitDevTryMax(6);
    sabotageMsg(false);
  }
};

DEFREGISTER(Reg1,CREG_AES_ACTIVE,CREG_MSGFORPOS,CREG_EVENTDELAYTIME,CREG_LEDONTIME,CREG_TRANSMITTRYMAX)
class SCList1 : public RegList1<Reg1> {
public:
  SCList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
  void defaults () {
    clear();
    msgForPosA(1); // CLOSED
    msgForPosB(2); // OPEN
    // aesActive(false);
    // eventDelaytime(0);
    ledOntime(100);
    transmitTryMax(6);
  }
};


class WDSPosition : public Position {
private:
  uint8_t pin;
public:
  WDSPosition () : pin(0) {}
  void init (uint8_t p) {
    pin = p;
    pinMode(p,INPUT);
  }

  void measure (__attribute__((unused)) bool async=false) {
    _position = digitalRead(pin) == LOW ? State::PosA : State::PosB;
  }
  uint32_t interval () { return 0; }
};

template <class HALTYPE,class List0Type,class List1Type,class List4Type,int PEERCOUNT>
class TwoPinChannel : public StateGenericChannel<WDSPosition,HALTYPE,List0Type,List1Type,List4Type,PEERCOUNT> {
public:
  typedef StateGenericChannel<WDSPosition,HALTYPE,List0Type,List1Type,List4Type,PEERCOUNT> BaseChannel;

  TwoPinChannel () : BaseChannel() {};
  ~TwoPinChannel () {}

  void init (uint8_t pin) {
    BaseChannel::possens.init(pin);
  }

  uint32_t interval () { return BaseChannel::possens.interval(); }
};

typedef TwoPinChannel<Hal,SCList0,SCList1,DefList4,PEERS_PER_CHANNEL> ChannelType;
typedef StateDevice<Hal,ChannelType,1,SCList0, CYCLETIME> SCType;

SCType sdev(devinfo,0x20);
ConfigButton<SCType> cfgBtn(sdev);

void funcISR () {
  // we simply activate the alarm
  Alarm& a = sdev.channel(1);
  sysclock.cancel(a);
  sysclock.add(a);
}

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  hal.battery.init();
  hal.battery.low(BAT_LOW);
  hal.battery.critical(BAT_CRIT);
  while (hal.battery.current() == 0);

  buttonISR(cfgBtn,CONFIG_BTN);
  sdev.channel(1).init(SENS1_PIN);
  sdev.initDone();

  contactISR(SENS1_PIN,funcISR);

  sdev.channel(1).changed(true);
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    if( hal.battery.critical() ) {
      hal.activity.sleepForever(hal);
    }
    hal.activity.savePower<Sleep<> >(hal);
  }
}
