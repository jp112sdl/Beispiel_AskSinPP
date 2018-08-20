//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-08-20 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define MULTIPRESSTIME 500  //max. Zeit zwischen 2 Klicks

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <SPI.h>  // after including SPI Library - we can use LibSPI class
#include "ButtonMP.h"
#include <AskSinPP.h>
#include <LowPower.h>
#include <Register.h>
#include <MultiChannelDevice.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED_PIN 4
#define LED_PIN2 5
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8
// Arduino pins for the buttons
// A0,A1,A2,A3 == PIN 14,15,16,17 on Pro Mini
#define CHANNELS  8  //number of BTN-Pins * 2
#define BTN1_PIN 14
#define BTN2_PIN 15
#define BTN3_PIN 16
#define BTN4_PIN 17


#if CHANNELS == 4
#define DEV_MODEL {0x00, 0x36}
#else
#define DEV_MODEL {0x00, 0x35}
#endif

// number of available peers per channel
#define PEERS_PER_CHANNEL 10

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0x36, 0x01},     // Device ID
  "JPMULRC001",           // Device Serial
  DEV_MODEL,           // Device Model
  0x11,                   // Firmware Version
  as::DeviceType::Remote, // Device Type
  {0x00, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef LibSPI<10> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef DualStatusLed<LED_PIN2, LED_PIN> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> HalType;
class Hal : public HalType {
    // extra clock to count button press events
    AlarmClock btncounter;
  public:
    void init (const HMID& id) {
      HalType::init(id);
      // get new battery value after 50 key press
      battery.init(50, btncounter);
      battery.low(22);
      battery.critical(19);
    }

    void sendPeer () {
      --btncounter;
    }

    bool runready () {
      return HalType::runready() || btncounter.runready();
    }
};

DEFREGISTER(RemoteReg1, CREG_LONGPRESSTIME, CREG_AES_ACTIVE, CREG_DOUBLEPRESSTIME)
class RemoteList1 : public RegList1<RemoteReg1> {
  public:
    RemoteList1 (uint16_t addr) : RegList1<RemoteReg1>(addr) {}
    void defaults () {
      clear();
      longPressTime(400);
      // aesActive(false);
    }
};

template<class HALTYPE, int PEERCOUNT, class List0Type = List0>
class RemoteChannel : public Channel<HALTYPE, RemoteList1, EmptyList, DefList4, PEERCOUNT, List0Type>, public Button {

  private:
    uint8_t       repeatcnt;
    volatile bool isr;

  public:

    typedef Channel<HALTYPE, RemoteList1, EmptyList, DefList4, PEERCOUNT, List0Type> BaseChannel;

    RemoteChannel () : BaseChannel(), repeatcnt(0), isr(false) {}
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

    bool configChanged() {
      //we have to add 300ms to the value set in CCU!
      uint16_t _longpressTime = 300 + (this->getList1().longPressTime() * 100);
      //DPRINT("longpressTime = ");DDECLN(_longpressTime);
      setLongPressTime(millis2ticks(_longpressTime));
    }

    virtual void multi(uint8_t count) {
      Button::multi(count);
      DPRINT(" multi press -> count: "); DDECLN(count);

      uint8_t ch = (count > 1) ? BaseChannel::number() + 1 : BaseChannel::number();
      bool lp = (count > 2);

      DHEX(ch); DPRINTLN(lp ? " long" : " short");

      RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();

      msg.init(this->device().nextcount(), ch, repeatcnt, lp, this->device().battery().low());
      this->device().sendPeerEvent(msg, *this);
      repeatcnt++;
    }

    virtual void state(uint8_t s) {
      Button::state(s);
      if (s != released) {
        DHEX(BaseChannel::number());
        RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();
        msg.init(this->device().nextcount(), this->number(), repeatcnt, (s == longreleased || s == longpressed), this->device().battery().low());
        if (s == longreleased) {
          // send the message to every peer
          this->device().sendPeerEvent(msg, *this);
          repeatcnt++;
        }
        else if (s == longpressed) {
          // broadcast the message
          this->device().broadcastPeerEvent(msg, *this);
        }
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

#define remoteISR(device,chan,pin) class device##chan##ISRHandler { \
    public: \
      static void isr () { device.channel(chan).irq(); } \
  }; \
  device.channel(chan).button().init(pin); \
  if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
    enableInterrupt(pin,device##chan##ISRHandler::isr,CHANGE); \
  else \
    attachInterrupt(digitalPinToInterrupt(pin),device##chan##ISRHandler::isr,CHANGE);

#define remoteChannelISR(chan,pin) class __##pin##ISRHandler { \
    public: \
      static void isr () { chan.irq(); } \
  }; \
  chan.button().init(pin); \
  if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
    enableInterrupt(pin,__##pin##ISRHandler::isr,CHANGE); \
  else \
    attachInterrupt(digitalPinToInterrupt(pin),__##pin##ISRHandler::isr,CHANGE);

typedef MultiChannelDevice<Hal, RemoteChannel<Hal, PEERS_PER_CHANNEL, List0>, CHANNELS> RemoteType;

Hal hal;
RemoteType sdev(devinfo, 0x20);
ConfigButton<RemoteType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  remoteISR(sdev, 1, BTN1_PIN);
  remoteISR(sdev, 3, BTN2_PIN);
#ifdef BTN3_PIN
  remoteISR(sdev, 5, BTN3_PIN);
#endif
#ifdef BTN4_PIN
  remoteISR(sdev, 7, BTN4_PIN);
#endif
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
