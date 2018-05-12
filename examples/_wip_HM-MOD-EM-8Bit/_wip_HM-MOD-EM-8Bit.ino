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

#include <Switch.h>
#include <MultiChannelDevice.h>

#define LED_PIN 4
#define CONFIG_BUTTON_PIN 8

#define BTN1_PIN 3
#define BTN2_PIN 5
#define EM8_PIN0 6
#define EM8_PIN1 7
#define EM8_PIN2 9
#define EM8_PIN3 14
#define EM8_PIN4 15
#define EM8_PIN5 16
#define EM8_PIN6 17
#define EM8_PIN7 18

// number of available peers per channel
#define PEERS_PER_RemoteChannel  4
#define PEERS_PER_EM8CHANNEL  4

#define remISR(device,chan,pin) class device##chan##ISRHandler { \
    public: \
      static void isr () { device.remoteChannel(chan).irq(); } \
  }; \
  device.remoteChannel(chan).button().init(pin); \
  if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
    enableInterrupt(pin,device##chan##ISRHandler::isr,CHANGE); \
  else \
    attachInterrupt(digitalPinToInterrupt(pin),device##chan##ISRHandler::isr,CHANGE);

#define em8ISR(device,pin) class device##pin##ISR8Handler { \
    public: \
      static void isr () { device.em8bitChannel().irq(); } \
  }; \
  device.em8bitChannel().button().init(pin); \
  if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
    enableInterrupt(pin,device##pin##ISR8Handler::isr,CHANGE); \
  else \
    attachInterrupt(digitalPinToInterrupt(pin),device##pin##ISR8Handler::isr,CHANGE);
    
// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x32, 0x01},     // Device ID
  "JPEM8b0001",           // Device Serial
  {0x01, 0x06},           // Device Model
  0x10,                   // Firmware Version
  as::DeviceType::Switch, // Device Type
  {0x00, 0x00}            // Info Bytes
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

DEFREGISTER(Reg0, MASTERID_REGS, DREG_LEDMODE, DREG_TRANSMITTRYMAX, DREG_LOCALRESETDISABLE, DREG_LOWBATLIMIT)
class SwList0 : public RegList0<Reg0> {
  public:
    SwList0(uint16_t addr) : RegList0<Reg0>(addr) {}
    void defaults() {
      clear();
      intKeyVisible(true);
      cycleInfoMsg(true);
    }
};


DEFREGISTER(RemoteReg1, CREG_LONGPRESSTIME, CREG_AES_ACTIVE, CREG_DOUBLEPRESSTIME)
class RemoteList1 : public RegList1<RemoteReg1> {
  public:
    RemoteList1 (uint16_t addr) : RegList1<RemoteReg1>(addr) {}
    void defaults () {
      clear();
      longPressTime(1);
      // aesActive(false);
      // doublePressTime(0);
    }
};

class RemoteChannel : public Channel<Hal, RemoteList1, EmptyList, DefList4, PEERS_PER_RemoteChannel, SwList0>, public Button {
  private:
    uint8_t       repeatcnt;

  public:
    typedef Channel<Hal, RemoteList1, EmptyList, DefList4, PEERS_PER_RemoteChannel, SwList0> BaseChannel;

    RemoteChannel () : BaseChannel() {}
    virtual ~RemoteChannel () {}

    Button& button () {
      return *(Button*)this;
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }

    virtual void state(uint8_t s) {
      DHEX(BaseChannel::number());
      Button::state(s);
      RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();
      msg.init(this->device().nextcount(), this->number(), repeatcnt, (s == longreleased || s == longpressed), this->device().battery().low());
      if ( s == released || s == longreleased) {
        // send the message to every peer
        this->device().sendPeerEvent(msg, *this);
        repeatcnt++;
      }
      else if (s == longpressed) {
        // broadcast the message
        this->device().broadcastPeerEvent(msg, *this);
      }
    }

    uint8_t state() const {
      return Button::state();
    }

    bool pressed () const {
      uint8_t s = state();
      return s == Button::pressed || s == Button::debounce || s == Button::longpressed;
    }
};

DEFREGISTER(Em8Reg1, CREG_TRANSMITTRYMAX, CREG_DATA_TRANSMISSION_CONDITION, CREG_DATA_STABILITY_FILTER_TIME, CREG_DATA_INPUT_PROPERTIES)
class Em8List1 : public RegList1<Em8Reg1> {
  public:
    Em8List1 (uint16_t addr) : RegList1<Em8Reg1>(addr) {}
    void defaults () {
      clear();
    }
};

class Em8EventMsg : public Message {
  public:
    void init(uint8_t msgcnt, uint8_t val, uint8_t ch) {
      Message::init(0xc, msgcnt, 0x41, BIDI, ch, msgcnt);
      pload[0] = val;
    }
};

class EM8Channel : public Channel<Hal, Em8List1, EmptyList, List4, PEERS_PER_EM8CHANNEL, SwList0>, public Button {
    Em8EventMsg     msg;
    uint16_t        millis;

  public:
    EM8Channel () : Button() {}
    virtual ~EM8Channel () {}

    Button& button () {
      return *(Button*)this;
    }
    
    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      tick = delay();
      clock.add(*this);
      msg.init(device().nextcount(), 16, number());
      device().sendPeerEvent(msg, *this);
    }

    uint32_t delay () {
      return seconds2ticks(180);
    }
    
    void setup(Device<Hal, SwList0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sysclock.add(*this);
    }


    uint8_t status () const {
      return 0;
    }

    void irq() {
       
    }

    void configChanged() {
      DPRINTLN(F("*List1 changed"));
      DPRINT(F("*dataTransmissionCondtion ")); DDECLN(this->getList1().dataTransmissionCondtion());
      DPRINT(F("*dataStabilityFilterTime ")); DDECLN(this->getList1().dataStabilityFilterTime());
      DPRINT(F("*dataInputPropertieIn0 ")); DDECLN(this->getList1().dataInputPropertieIn0());
      DPRINT(F("*dataInputPropertieIn1 ")); DDECLN(this->getList1().dataInputPropertieIn1());
      DPRINT(F("*dataInputPropertieIn2 ")); DDECLN(this->getList1().dataInputPropertieIn2());
      DPRINT(F("*dataInputPropertieIn3 ")); DDECLN(this->getList1().dataInputPropertieIn3());
      DPRINT(F("*dataInputPropertieIn4 ")); DDECLN(this->getList1().dataInputPropertieIn4());
      DPRINT(F("*dataInputPropertieIn5 ")); DDECLN(this->getList1().dataInputPropertieIn5());
      DPRINT(F("*dataInputPropertieIn6 ")); DDECLN(this->getList1().dataInputPropertieIn6());
      DPRINT(F("*dataInputPropertieIn7 ")); DDECLN(this->getList1().dataInputPropertieIn7());
    }

    uint8_t flags () const {
      return 0;
    }
};

class MixDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, SwList0>, 3, SwList0> {
  public:
    VirtChannel<Hal, RemoteChannel, SwList0> remChannel[2];
    VirtChannel<Hal, EM8Channel, SwList0>    em8Channel;
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, SwList0>, 3, SwList0> DeviceType;
    MixDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(remChannel[0], 1);
      DeviceType::registerChannel(remChannel[1], 2);
      DeviceType::registerChannel(em8Channel, 3);

    }
    virtual ~MixDevice () {}


    RemoteChannel& remoteChannel (uint8_t num)  {
      return remChannel[num -  1];
    }

    EM8Channel& em8bitChannel () {
      return em8Channel;
    }

    virtual void configChanged () {
    }
};

MixDevice sdev(devinfo, 0x20);
ConfigButton<MixDevice> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);

  remISR(sdev, 1, BTN1_PIN);
  remISR(sdev, 2, BTN2_PIN);
  em8ISR(sdev, EM8_PIN0);
  em8ISR(sdev, EM8_PIN1);
  em8ISR(sdev,EM8_PIN2);
  em8ISR(sdev,EM8_PIN3);
  em8ISR(sdev,EM8_PIN4);
  em8ISR(sdev,EM8_PIN5);
  em8ISR(sdev,EM8_PIN6);
  em8ISR(sdev,EM8_PIN7);

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

