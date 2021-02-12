//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-12-26 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

//#define USE_AES
//#define HM_DEF_KEY ...
//#define HM_DEF_KEY_INDEX 0

#define SENSOR_ONLY
#define SIMPLE_CC1101_INIT
#define NOCRC
#define NDEBUG

//external EEPROM, unused in original eQ-3 Firmware 
//#define STORAGEDRIVER at24cX<0x50,128,32> 
//#include <Wire.h>

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <MultiChannelDevice.h>
#include <Remote.h>

#define LED_PIN           8
#define CC1101_GDO0_PIN   2
#define CC1101_PWR_PIN    5
#define CONFIG_BTN        0
#define BTN01_PIN         15
#define BTN02_PIN         14
#define ACTIVATE_PIN      9

#define WAKE_TIME_MS      2500   //Stay awake for 2.5 seconds after last action (original eQ-3 -> 10 secs)
#define MSG_COUNT_BYTE    8     //StorageConfig Byte to save last message counter value

#define BAT_LOW           22
#define BAT_CRIT          19

#define PEERS_PER_CHANNEL 16

using namespace as;
const struct DeviceInfo PROGMEM devinfo = {
    {0x00,0x36,0x21},       // Device ID
    "JPWSCRWE01",           // Device Serial
    {0x00,0x36},            // Device Model
    0x10,                   // Firmware Version
    as::DeviceType::Remote, // Device Type
    {0x00,0x00}             // Info Bytes
};

typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,CC1101_GDO0_PIN> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType,IrqInternalBatt,RadioType> Hal;

typedef RemoteChannel<Hal,PEERS_PER_CHANNEL,List0> ChannelType;
typedef MultiChannelDevice<Hal,ChannelType,2> RemoteType;

Hal hal;
RemoteType sdev(devinfo,0x20);
ConfigButton<RemoteType> cfgBtn(sdev);

template <uint8_t EN_PIN>
class PowerOffAlarm : public Alarm {
private:
  bool    timerActive;
public:
  PowerOffAlarm () : Alarm(0), timerActive(false) {}
  ~PowerOffAlarm () {}

  void init() {
    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, HIGH);
  }

  void activateTimer(bool en) {
    if (en == true && timerActive == false) {
      sysclock.cancel(*this);
      set(millis2ticks(WAKE_TIME_MS));
      sysclock.add(*this);
    } else if (en == false) {
      sysclock.cancel(*this);
    }
    timerActive = en;
 }

  virtual void trigger(__attribute__((unused)) AlarmClock& clock) {
    powerOff(true);
  }

  void powerOff(bool save) {
    if (save == true) {
      StorageConfig sc = sdev.getConfigArea();
      uint8_t currentCount = sdev.nextcount() - 1;
      sc.setByte(MSG_COUNT_BYTE, currentCount);
      sc.validate();
      DPRINT("setByte ");DDECLN(currentCount);
    }
    DPRINTLN(F("GOOD BYE JONNY"));_delay_ms(200);
    digitalWrite(ACTIVATE_PIN, LOW);
  }
  
};

PowerOffAlarm<ACTIVATE_PIN> pwrOffAlarm;

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);

  pinMode(ACTIVATE_PIN, OUTPUT);
  digitalWrite(ACTIVATE_PIN, HIGH);
  pinMode(CC1101_PWR_PIN, OUTPUT);
  digitalWrite(CC1101_PWR_PIN, LOW);

  pwrOffAlarm.init();

  buttonISR(cfgBtn,CONFIG_BTN);
  remoteISR(sdev,1,BTN01_PIN);
  remoteISR(sdev,2,BTN02_PIN);
  
  uint8_t TA = 0;
  if (digitalRead(CONFIG_BTN) == LOW) TA = 1;
  if (digitalRead(BTN01_PIN)  == LOW) TA = 2;
  if (digitalRead(BTN02_PIN)  == LOW) TA = 3;
  DPRINT("TA = ");DDECLN(TA);

  if (TA > 0) {
    sdev.init(hal);
    hal.battery.init();
    hal.battery.low(BAT_LOW);
    hal.battery.critical(BAT_CRIT);
    sdev.initDone();

    while (hal.battery.current() == 0);
    if (hal.battery.critical()) pwrOffAlarm.powerOff(false);

    StorageConfig sc = sdev.getConfigArea();
    uint8_t msgCount = sc.getByte(MSG_COUNT_BYTE);
    while (sdev.nextcount() < msgCount);
  
    hal.radio.setSendTimeout();
    if (TA == 1) cfgBtn.irq();
    else if (TA == 2) sdev.channel(1).button().irq();
    else if (TA == 3) sdev.channel(2).button().irq();
  } else {
    pwrOffAlarm.powerOff(false);
  }
}
void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  pwrOffAlarm.activateTimer( hal.activity.stayAwake() == false &&  worked == false && poll == false );
}
