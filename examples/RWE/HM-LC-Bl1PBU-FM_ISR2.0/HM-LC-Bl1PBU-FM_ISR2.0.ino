//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2017-12-14 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#define USE_HW_SERIAL

//#define HIDE_IGNORE_MSG
#define NDEBUG

#include <WireSoft.h>
TwoWireSoft Wire(SDA, SCL);

#include <SPI.h>
#include <AskSinPP.h>
#include <Blind.h>
#include <Remote.h>
#include <MultiChannelDevice.h>
#include <Blind.h>

#define DIR_RELAY_PIN         PB8
#define ON_RELAY_PIN          PB7
#define BTN_PIN_1             PB11
#define BTN_PIN_2             PA2

#define LED1_PIN              PA0
#define LED2_PIN              PA1
#define CONFIG_BUTTON_PIN     PC13

#define TRX_CS                PC14
#define TRX_GDO0              PC15

// number of available peers per channel
#define PEERS_PER_CHANNEL       12
#define PEERS_PER_BlindChannel   8

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0xaf,0x00,0x01},              // Device ID
    "FS20RSU201",                  // Device Serial
    {0x00,0x6A},                   // Device Model
    0x24,                          // Firmware Version
    as::DeviceType::BlindActuator, // Device Type
    {0x01,0x00}                    // Info Bytes
}; //

/**
 * Configure the used hardware
 */

typedef LibSPI<TRX_CS> SPIType;
typedef CC1101Radio<SPIType,TRX_GDO0> RadioType;
typedef DualStatusLed<LED1_PIN,LED2_PIN> LedType;
typedef AskSin<LedType, NoBattery, RadioType> Hal;

DEFREGISTER(BlindReg0,MASTERID_REGS,DREG_INTKEY,DREG_CONFBUTTONTIME,DREG_LOCALRESETDISABLE)

class BlindList0 : public RegList0<BlindReg0> {
public:
  BlindList0 (uint16_t addr) : RegList0<BlindReg0>(addr) {}
  void defaults () {
    clear();
    // intKeyVisible(false);
    confButtonTime(0xff);
    // localResetDisable(false);
  }
};

class BlChannel : public ActorChannel<Hal,BlindList1,BlindList3,PEERS_PER_CHANNEL,BlindList0,BlindStateMachine> {
public:
  typedef ActorChannel<Hal,BlindList1,BlindList3,PEERS_PER_CHANNEL,BlindList0,BlindStateMachine> BaseChannel;

  BlChannel () {}
  virtual ~BlChannel () {}

  virtual void switchState(uint8_t oldstate,uint8_t newstate, uint32_t stateDelay) {
    BaseChannel::switchState(oldstate, newstate, stateDelay);
    if( newstate == AS_CM_JT_RAMPON && stateDelay > 0 ) {
      motorUp();
    }
    else if( newstate == AS_CM_JT_RAMPOFF && stateDelay > 0 ) {
      motorDown();
    }
    else {
      motorStop();
    }
  }

  void motorUp () {
    digitalWrite(DIR_RELAY_PIN,HIGH);
    digitalWrite(ON_RELAY_PIN,HIGH);
  }

  void motorDown () {
    digitalWrite(DIR_RELAY_PIN,LOW);
    digitalWrite(ON_RELAY_PIN,HIGH);
  }

  void motorStop () {
    digitalWrite(DIR_RELAY_PIN,LOW);
    digitalWrite(ON_RELAY_PIN,LOW);
  }

  void init () {
    pinMode(ON_RELAY_PIN,OUTPUT);
    pinMode(DIR_RELAY_PIN,OUTPUT);
    motorStop();
    BaseChannel::init();
  }
};

// setup the device with channel type and number of channels
typedef MultiChannelDevice<Hal,BlChannel,1,BlindList0> BlindType;

Hal hal;
BlindType sdev(devinfo,0x20);
ConfigButton<BlindType> cfgBtn(sdev);
InternalButton<BlindType> btnup(sdev,1);
InternalButton<BlindType> btndown(sdev,2);

void initPeerings (bool first) {
  // create internal peerings - CCU2 needs this
  if( first == true ) {
    sdev.channel(1).peer(btnup.peer(),btndown.peer());
  }
}

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  //storage().setByte(0,0);
  bool first = sdev.init(hal);
  sdev.channel(1).init();

//  sdev.channel(1).getList1().refRunningTimeBottomTop(270);
//  sdev.channel(1).getList1().refRunningTimeTopBottom(270);

  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  buttonISR(btnup,BTN_PIN_1);
  buttonISR(btndown,BTN_PIN_2);

  initPeerings(first);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
  //  hal.activity.savePower<Idle<> >(hal);
  }
}
