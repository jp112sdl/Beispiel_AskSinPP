//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-12-26 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER
#define SENSOR_ONLY
#define SIMPLE_CC1101_INIT
#define NOCRC
#define NDEBUG

#define STORAGEDRIVER at24cX<0x50,128,32> 
#include <Wire.h>

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
//#include <ResetOnBoot.h>

#include <MultiChannelDevice.h>
#include <Remote.h>

#define LED         8
#define CC1101_GDO0 2
#define CC1101_PWR  5
#define BTN01_PIN   0
#define BTN02_PIN   14
#define BTN03_PIN   15
#define BTN04_PIN   16
#define BTN05_PIN   17
#define BTN06_PIN   4
#define BTN07_PIN   6
#define BTN08_PIN   7

#define SLEEP_DELAY_MS    3000 //wait milliseconds before sleep mode
#define PEERS_PER_CHANNEL 10

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x00,0xda,0x00},       // Device ID
    "HMRC00da00",           // Device Serial
    {0x00,0xda},            // Device Model
    0x10,                   // Firmware Version
    as::DeviceType::Remote, // Device Type
    {0x00,0x00}             // Info Bytes
};

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,CC1101_GDO0, CC1101_PWR> RadioType;
typedef StatusLed<LED> LedType;
typedef AskSin<LedType,IrqInternalBatt,RadioType> Hal;

typedef RemoteChannel<Hal,PEERS_PER_CHANNEL,List0> ChannelType;
typedef MultiChannelDevice<Hal,ChannelType,8> RemoteType;

Hal hal;
RemoteType sdev(devinfo,0x20);
//ResetOnBoot<RemoteType> resetOnBoot(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  hal.battery.init();
  hal.battery.low(22);
  hal.battery.critical(19);

  remoteISR(sdev,1,BTN01_PIN);
  remoteISR(sdev,2,BTN02_PIN);
  remoteISR(sdev,3,BTN03_PIN);
  remoteISR(sdev,4,BTN04_PIN);
  remoteISR(sdev,5,BTN05_PIN);
  remoteISR(sdev,6,BTN06_PIN);
  remoteISR(sdev,7,BTN07_PIN);
  remoteISR(sdev,8,BTN08_PIN);

  while (hal.battery.current() == 0);

 // resetOnBoot.init();
  sdev.initDone();
  sdev.startPairing();
}

class PowerOffAlarm : public Alarm {
  private:
    bool    timerActive;
  public:
    PowerOffAlarm () : Alarm(0), timerActive(false) {}
    virtual ~PowerOffAlarm () {}

    void activateTimer(bool en) {
      if (en == true && timerActive == false) {
        sysclock.cancel(*this);
        set(millis2ticks(SLEEP_DELAY_MS));
        sysclock.add(*this);
      } else if (en == false) {
        sysclock.cancel(*this);
      }
      timerActive = en;
    }

    virtual void trigger(__attribute__((unused)) AlarmClock& clock) {
      powerOff();
    }

    void powerOff() {
      hal.led.ledOff();
      hal.radio.setIdle();
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    }

} pwrOffAlarm;

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  pwrOffAlarm.activateTimer( hal.activity.stayAwake() == false &&  worked == false && poll == false );

}
