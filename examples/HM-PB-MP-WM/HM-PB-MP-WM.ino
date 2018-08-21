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
#define LED_PIN  4
#define LED_PIN2 5
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8
// Arduino pins for the buttons
// A0,A1,A2,A3 == PIN 14,15,16,17 on Pro Mini
#define BTN1_PIN 14
#define BTN2_PIN 15
//#define BTN3_PIN 16
//#define BTN4_PIN 17


#ifdef BTN3_PIN && BTN4_PIN
#define CHANNELS 8
#define DEV_MODEL {0x00, 0x35}
#else
#define CHANNELS 4
#define DEV_MODEL {0x00, 0x36}
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
      longPressTime(1);
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
      uint16_t _longpressTime = 300 + (this->getList1().longPressTime() * 100);
      //DHEX(number());DPRINTLN(" longPressTime ");DDECLN(_longpressTime);
      setLongPressTime(millis2ticks(_longpressTime));
    }

    virtual void multi(uint8_t count) {
      Button::multi(count);

      if ((BaseChannel::number() % 2 == 0)  && (count > 1)) {
        DPRINT(F(" multi press -> count: ")); DDECLN(count);
        sendMsg(count > 2);
      }

      if ((BaseChannel::number() % 2 == 1)  && (count == 1)) {
        DPRINTLN(F(" single press"));
        sendMsg(false);
      }

    }

    void sendMsg(bool lg) {
      DPRINTLN(""); DPRINTLN("***");
      DPRINT("SEND MSG CH "); DHEX(BaseChannel::number()); DPRINTLN(lg ? " long" : " short");
      DPRINTLN("***");
      RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();
      msg.init(this->device().nextcount(), BaseChannel::number(), repeatcnt, lg, this->device().battery().low());
      this->device().sendPeerEvent(msg, *this);
      repeatcnt++;
    }

    virtual void state(uint8_t s) {
      Button::state(s);
      if (BaseChannel::number() % 2 == 1) {
        if (s != released) {
          DHEX(BaseChannel::number());
          RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();
          msg.init(this->device().nextcount(), this->number(), repeatcnt, (s == longreleased || s == longpressed), this->device().battery().low());
          if ( s == longreleased) {
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
    }

    uint8_t state() const {
      return Button::state();
    }

    bool pressed () const {
      uint8_t s = state();
      return s == Button::pressed || s == Button::debounce || s == Button::longpressed;
    }
};

typedef MultiChannelDevice<Hal, RemoteChannel<Hal, PEERS_PER_CHANNEL, List0>, CHANNELS> RemoteType;

Hal hal;
RemoteType sdev(devinfo, 0x20);
ConfigButton<RemoteType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);

  if ( digitalPinToInterrupt(BTN1_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(BTN1_PIN, isrBTN1, CHANGE); else attachInterrupt(digitalPinToInterrupt(BTN1_PIN), isrBTN1, CHANGE);
  sdev.channel(1).button().init(BTN1_PIN);
  sdev.channel(2).button().init(BTN1_PIN);

  if ( digitalPinToInterrupt(BTN2_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(BTN2_PIN, isrBTN2, CHANGE); else attachInterrupt(digitalPinToInterrupt(BTN2_PIN), isrBTN2, CHANGE);
  sdev.channel(3).button().init(BTN2_PIN);
  sdev.channel(4).button().init(BTN2_PIN);

#ifdef BTN3_PIN
  if ( digitalPinToInterrupt(BTN3_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(BTN3_PIN, isrBTN3, CHANGE); else attachInterrupt(digitalPinToInterrupt(BTN3_PIN), isrBTN3, CHANGE);
  sdev.channel(5).button().init(BTN3_PIN);
  sdev.channel(6).button().init(BTN3_PIN);
#endif

#ifdef BTN4_PIN
  if ( digitalPinToInterrupt(BTN4_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(BTN4_PIN, isrBTN4, CHANGE); else attachInterrupt(digitalPinToInterrupt(BTN4_PIN), isrBTN4, CHANGE);
  sdev.channel(7).button().init(BTN4_PIN);
  sdev.channel(8).button().init(BTN4_PIN);
#endif

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();
}

static void isrBTN1 () {
  sdev.channel(1).irq();
  sdev.channel(2).irq();
}
static void isrBTN2 () {
  sdev.channel(3).irq();
  sdev.channel(4).irq();
}

#ifdef BTN3_PIN
static void isrBTN3 () {
  sdev.channel(5).irq();
  sdev.channel(6).irq();
}
#endif

#ifdef BTN4_PIN
static void isrBTN4 () {
  sdev.channel(7).irq();
  sdev.channel(8).irq();
}
#endif

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Sleep<>>(hal);
  }
}
