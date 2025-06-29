//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-10-07 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2019-01-10 scuba82 Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// link: https://github.com/pa-pa/AskSinPP/tree/V4/examples/custom/HB-LC-Dim1TPBU-FM
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=644p aes=no

// define this to read the device id, serial and device type from bootloader section
//#define USE_OTA_BOOTLOADER

// Arduino IDE Settings
// use support for 644PA from MightyCore: https://github.com/MCUdude/MightyCore
// settings:
// Board:        ATMega644
// EEPROM:       EEPROM retained
// Pinout:       Standard
// Clock:        8MHz external
// Variant:      644P / 644PA
// BOD:          2.7V
// Compiler LTO: Enabled

#define NDEBUG

// use external EEProm 24c32 via I2C 
//#define STORAGEDRIVER at24cX<0x50,512,256>

#define EI_NOTEXTERNAL
#include <Wire.h>
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <Dimmer.h>
#include <Remote.h>
#include <sensors/Ntc.h>



// PIN definitions
#define DIMMERPIN          22  //PC6
//#define BTN_PIN_1          12  //PD4 
//#define BTN_PIN_2           8  //PD0(RxD) 
#define LED_PIN             0  //PB0
#define CONFIG_BUTTON_PIN   8  //PD7
#define GDO0_PIN           10  //PD2
#define LOADPIN            27  //PA4
#define ZEROPIN             2  //PB2
#define NTC_SENSE_PIN      24  //PA0  pin to measure ntc
#define NTC_ACTIVATOR_PIN   0  // pin to power ntc (or 0 if connected to vcc)

// NTC definitions
#define NTC_T0 25 // temperature where ntc has resistor value of R0
#define NTC_R0 10000 // resistance both of ntc and known resistor
#define NTC_B 3435 // b value of ntc (see datasheet)
#define NTC_OVERSAMPLING 2 // number of additional bits by oversampling (should be between 0 and 6, highly increases number of measurements)

// Phase Cut mode 
#define PHASECUT_MODE 0 // 0 = trailing-edge phase cut; 1 = leading-edge phase cut

// number of available peers per channel
#define PEERS_PER_DimChannel     8  

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x00,0xb4,0x02},       // Device ID - lfd Nr 01 vergeben
    "RWEPSD0002",           // Device Serial - lfd Nr 01 vergeben
    {0x00,0xb4},            // Device Model
    0x29,                   // Firmware Version
    as::DeviceType::Dimmer, // Device Type
    {0x01,0x00}             // Info Bytes
};
/**
   Configure the used hardware
*/
typedef AvrSPI<4,5,6,7> SPIType;
typedef Radio<SPIType, GDO0_PIN> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, NoBattery, RadioType> Hal;
Hal hal;
typedef DimmerChannel<Hal, PEERS_PER_DimChannel> DimChannel;
typedef DimmerDevice<Hal, DimChannel, 3, 3> DimDevice;

DimDevice sdev(devinfo, 0x20);
DimmerControl<Hal,DimDevice,ZC_Control<>> control(sdev);
//ConfigButton<DimDevice> cfgBtn(sdev);
//InternalButton<DimDevice> btn1(sdev, 1);
//InternalButton<DimDevice> btn2(sdev, 2);
ConfigToggleButton<DimDevice> cfgBtn(sdev);

class OverloadSens : public Alarm {
  uint8_t overloadcount = 0;
  uint8_t counter;
  public:
  OverloadSens () : Alarm(0) {}
  virtual ~OverloadSens () {}
  void init () {
    pinMode(LOADPIN,INPUT);
    set(seconds2ticks(15));
    sysclock.add(*this);
  }
  virtual void trigger (AlarmClock& clock) {
     bool senspin = digitalRead(LOADPIN);
     control.setOverload(senspin);
     set(seconds2ticks(1));
     clock.add(*this);
  }
};
OverloadSens overload;

class TempSens : public Alarm {
  Ntc<NTC_SENSE_PIN,NTC_R0,NTC_B,NTC_ACTIVATOR_PIN,NTC_T0,NTC_OVERSAMPLING> ntc;
  
  public:
  TempSens () : Alarm(0) {}
  virtual ~TempSens () {}

  void init () {
    ntc.init();
    set(seconds2ticks(15));
    sysclock.add(*this);
  }

  virtual void trigger (AlarmClock& clock) {
    ntc.measure();
    DPRINT("Temp: ");DDECLN(ntc.temperature());
    control.setTemperature(ntc.temperature());
    set(seconds2ticks(2));
    clock.add(*this);
  }
};
TempSens tempsensor;



void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  if( control.init(hal,DIMMERPIN) ){
    sdev.channel(1).peer(cfgBtn.peer(), cfgBtn.peer());
    sdev.channel(2).peer(cfgBtn.peer(), cfgBtn.peer());
    sdev.channel(3).peer(cfgBtn.peer(), cfgBtn.peer());
  }

   
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
//  buttonISR(btn1, BTN_PIN_1);
//  buttonISR(btn2, BTN_PIN_2);
  tempsensor.init();
  overload.init();
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
}