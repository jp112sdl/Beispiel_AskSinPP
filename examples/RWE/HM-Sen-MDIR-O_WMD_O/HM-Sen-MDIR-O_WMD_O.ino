//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2023-04-18 modified by jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2023-04-12 modified by Peter.matic Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// ci-test=yes board=328p aes=no
//- -----------------------------------------------------------------------------------------------------------------------

//copy LMSS.h into your sketch directory from here:
//https://github.com/Peter-matic/HM-Sec-MDIR_WMD/blob/main/Library/LMSS.h

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

#define USE_WMDO  //if defined use WMDO, otherwise use WD

//AT24C512B (2FB 1) 512kBit (65,536 x 8) 64kByte EEPROM, 512 Pages á 128 Bytes
#define STORAGEDRIVER at24cX<0x50, 512, 128>
#include <Wire.h>


//#define NDEBUG
#define HIDE_IGNORE_MSG

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>

#include "LMSS.h"
// pin to measure LMSS-101
#define LMSS_SENSE_PIN A6
// pin to power LMSS-101 (or 0 if connected to vcc)
#define LMSS_ACTIVATOR_PIN 4
// curve steepness
#define LMSS_PARAM_M -129561
// axis intercept 
#define LMSS_PARAM_B 710343
// min value of measuring range
#define LMSS_PARAM_MIN 34
// max value of meauring range
#define LMSS_PARAM_MAX 600

#include <MultiChannelDevice.h>
#include <Motion.h>

#define LED_PIN 8
#define CONFIG_BUTTON_PIN 0
// Arduino pins for the PIRs (2 PIRs * 2 channels each)
#define PIR_PIN1 14
#define PIR_PIN2 15
#ifdef USW_WMDO
#define PIR_PIN3 255
#define PIR_PIN4 255
#else
#define PIR_PIN3 16 //WMD only
#define PIR_PIN4 17 //WMD only
#endif

// === Battery measurement ===
#define BAT_VOLT_LOW        33  // 3.3V low voltage threshold
#define BAT_VOLT_CRITICAL   30  // 3.0V critical voltage threshold, puts AVR into sleep-forever mode

// number of available peers per channel
#define PEERS_PER_CHANNEL 40

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x2a,0xbc,0x01},       // Device ID
#ifdef USE_WMDO    
    "RWEWMDO001",           // Device Serial
    {0x00,0x5d},            // Device Model
#else
    "RWEWMD0001",           // Device Serial
    {0x00,0x4a},            // Device Model
#endif
    0x16,                   // Firmware Version
    as::DeviceType::MotionDetector, // Device Type
    {0x01,0x00}             // Info Bytes
};

/**
 * Configure the used hardware
 */
typedef AvrSPI<10,11,12,13> SPIType;
typedef Radio<SPIType,2> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType,BattSensor<SyncMeter<ExternalVCC<A7,0xFF,LOW,3000,20>>>,RadioType> Hal;

typedef MotionChannel<Hal,PEERS_PER_CHANNEL,List0,Lmss<LMSS_SENSE_PIN,LMSS_ACTIVATOR_PIN,LMSS_PARAM_M,LMSS_PARAM_B,LMSS_PARAM_MIN,LMSS_PARAM_MAX>> MChannel;

typedef MultiChannelDevice<Hal,MChannel,1> MotionType;

Hal hal;
MotionType sdev(devinfo,0x20);
ConfigButton<MotionType> cfgBtn(sdev);

class PirHandler  {
  MotionType& device;
  class CheckAlarm : public Alarm {
  public:
    PirHandler& ph;
    CheckAlarm (PirHandler& _ph) : Alarm(0), ph(_ph) {}
    ~CheckAlarm () {}
    virtual void trigger(__attribute__((unused)) AlarmClock& clock) {
      ph.check();
    }
  };
protected:
  CheckAlarm ca;
  uint8_t tp;
  uint8_t pp;
  uint16_t timeout;
  volatile bool working;
public:
  PirHandler (MotionType& dev) : device(dev), ca(*this), tp(0xff), pp(0xff), timeout(1000), working(false) {}
  virtual ~PirHandler () {}

  void irq(uint8_t triggeredPin, uint8_t partnerPin) {
      tp = triggeredPin-14;
      pp = partnerPin-14;
      if (working == false) {
        working = true;
        PCICR &= ~(1 << PCIE1);
        PCIFR &= ~(1 << PCIF1);
        sysclock.cancel(ca);
        sysclock.add(ca);
      }
  }

  void disableWorking(uint8_t step) {
    //DDECLN(step);
    working = false; 
    PCICR |= (1 << PCIE1);
    PCIFR |= (1 << PCIF1);
  }
  
  void check() {  
      //Pegel des auslösenden Komparatorpins (tp) muss zwischen 10ms und 750ms H sein
      timeout=750;    
      while (PINC & (1 << tp)) {
        if (timeout==0 || PINC & (1 << pp)) { disableWorking(1); return; }
        _delay_ms(1); timeout--;
      }
      if (timeout > 740) { disableWorking(2); return; }

      //fällt der Pegel von tp während des Timeout-Fensters auf L
      //wird gewartet, dass der Pegel des komplementären Komparatorpins (pp)
      //innerhalb von 500ms auf H wechselt
      timeout=500;
      while (!(PINC & (1 << pp))) { 
         if (timeout == 0) { disableWorking(3); return; }
         _delay_ms(1); timeout--; 
      }
 
      //ist der Wechsel des pp auf H innerhalb des Timeout-Fensters erfolgt
      //muss nun noch dieser nun für mind. 30ms H bleiben
      timeout=30;
      while (PINC & (1 << pp) && timeout > 0) {
        if (PINC & (1 << tp)) { disableWorking(4); return; }
       _delay_ms(1); timeout--;
      }

      //stand der H-Pegel des pp mind. 50ms an, so ist eine korrekte
      //Bewegung erkannt worden
      if (timeout == 0) {
        device.channel(1).motionDetected();
        disableWorking(0);
        return;
      } else { disableWorking(5); return; }
  }
};

#define pirISR(device) static PirHandler pirHandler(device);\
  class device##pir##ISRHandler { \
  public: \
  static void isr1 () { pirHandler.irq(PIR_PIN1, PIR_PIN2);} \
  static void isr2 () { pirHandler.irq(PIR_PIN2, PIR_PIN1);} \
  static void isr3 () { pirHandler.irq(PIR_PIN3, PIR_PIN4);} \
  static void isr4 () { pirHandler.irq(PIR_PIN4, PIR_PIN3);} \
}; \
                        pinMode(PIR_PIN1,INPUT); enableInterrupt(PIR_PIN1,device##pir##ISRHandler::isr1,RISING);   \
                        pinMode(PIR_PIN2,INPUT); enableInterrupt(PIR_PIN2,device##pir##ISRHandler::isr2,RISING);   \
if (PIR_PIN3 != 0xff) { pinMode(PIR_PIN3,INPUT); enableInterrupt(PIR_PIN3,device##pir##ISRHandler::isr3,RISING); } \
if (PIR_PIN4 != 0xff) { pinMode(PIR_PIN4,INPUT); enableInterrupt(PIR_PIN4,device##pir##ISRHandler::isr4,RISING); }

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  sdev.init(hal);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);  
  hal.initBattery(60UL*60,BAT_VOLT_LOW,BAT_VOLT_CRITICAL); // Measure Battery every 1h
  
  sdev.initDone();
  
  _delay_ms(5000);  //PIR Module benötigt nach Stromzufuhr ca. 5 Sekunden zum "Initialisieren"
  pirISR(sdev);
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    if( hal.battery.critical() ) {
      hal.activity.sleepForever(hal);
    }
    hal.activity.savePower<Sleep<>>(hal);
  }
}
