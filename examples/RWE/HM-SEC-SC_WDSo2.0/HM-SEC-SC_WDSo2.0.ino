//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2022-05-26 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/4.0/de/
// ci-test=yes board=efm32 aes=no
//- -----------------------------------------------------------------------------------------------------------------------


#define USE_HM_SEC_SCO
//#define USE_HW_SERIAL
//#include "aes_secret.h"


#ifndef USE_HM_SEC_SCO
// HM-Sec-SCo has no external eeprom
#define STORAGEDRIVER m24mXX<0x0A,512,256>
#include <SoftWire.h>
TwoWireSoft Wire(SDA, SCL);
#endif

//#define HIDE_IGNORE_MSG
#define NDEBUG

#define ADC_CLOCK               400000        /* ADC conversion clock */
#define ADC_16BIT_MAX           65536           /* 2^16 */

#define LED1_PIN          PA1
#define LED2_PIN          PA0
#define CONFIG_BUTTON_PIN PC13

#define SABOTAGE_PIN          PB8
#define SABOTAGE_ACTIVE_STATE HIGH

#ifdef USE_HM_SEC_SCO
//HM-Sec-SCo uses StepUp converter with single AAA battery
#define BATT_MEASURE_CH   adcSingleInputCh4 //PD4
#endif

#define SENS_CH                 adcSingleInputCh5
#define SENS_EN_PIN1            PB13
#define SENS_EN_PIN2            PB14

#define OPT_TRG_LEVEL_LOW       3100
#define OPT_TRG_LEVEL_HIGH      3800

#define TRX_CS                PC14
#define TRX_GDO0              PC15

#define PEERS_PER_CHANNEL       20
#define CYCLETIME seconds2ticks(60UL * 60 * 1)

#ifdef BATT_MEASURE_CH
#define BATT_SENSOR BattSensor<SyncMeter<ExternalVCCEFM32<BATT_MEASURE_CH>>>
#else
#define BATT_SENSOR BatterySensor
#endif

#include <SPI.h>
#include <AskSinPP.h>
#include <Register.h>
#include <ContactState.h>

using namespace as;

const struct DeviceInfo PROGMEM devinfo = {
    {0x4e,0x51,0x27},       // Device ID
    "NEQ0942701",           // Device Serial
    {0x00,0xC7},            // Device Model
    0x10,                   // Firmware Version
    as::DeviceType::ThreeStateSensor, // Device Type
    {0x01,0x00}             // Info Bytes
};

typedef LibSPI<TRX_CS> SPIType;

typedef CC1101Radio<SPIType,TRX_GDO0> RadioType;

typedef DualStatusLed<LED1_PIN,LED2_PIN> LedType;
typedef AskSin<LedType,BATT_SENSOR,RadioType> BaseHal;
class Hal : public BaseHal {
public:
  void init (const HMID& id) {
    BaseHal::init(id);
    led.invert(true);
    // measure battery every 1h
    battery.init(seconds2ticks(60UL*60*1),sysclock);
    battery.low(12);
    battery.critical(11);
  }
} hal;

DEFREGISTER(Reg0,MASTERID_REGS,DREG_CYCLICINFOMSG,DREG_SABOTAGEMSG,DREG_LOCALRESETDISABLE,DREG_TRANSMITTRYMAX)
class SCOList0 : public RegList0<Reg0> {
public:
  SCOList0(uint16_t addr) : RegList0<Reg0>(addr) {}
  void defaults () {
    clear();
    cycleInfoMsg(true);
    transmitDevTryMax(6);
    sabotageMsg(true);
    localResetDisable(false);
  }
};

DEFREGISTER(Reg1,CREG_AES_ACTIVE,CREG_MSGFORPOS,CREG_EVENTDELAYTIME,CREG_LEDONTIME,CREG_TRANSMITTRYMAX)
class SCOList1 : public RegList1<Reg1> {
public:
  SCOList1 (uint16_t addr) : RegList1<Reg1>(addr) {}
  void defaults () {
    clear();
    msgForPosA(2); // OPEN
    msgForPosB(1); // CLOSED
    aesActive(true);
    eventDelaytime(0);
    ledOntime(0);
    transmitTryMax(6);
  }
};

class SCoOptPosition : public Position {
  uint8_t sens;
  uint8_t en1,en2;
public:
  SCoOptPosition () : sens(0), en1(0),en2(0) { _present = true; }

  void init (uint8_t pin, uint8_t enpin1, uint8_t enpin2) {
    sens=pin;
    en1 = enpin1;
    en2 = enpin2;
    pinMode(en1, OUTPUT);
    pinMode(en2, OUTPUT);

    CMU_ClockEnable(cmuClock_ADC0, true);
    ADC_Init_TypeDef init = ADC_INIT_DEFAULT;
    init.timebase = ADC_TimebaseCalc(0);
    init.prescale = ADC_PrescaleCalc(ADC_CLOCK, 0);
    ADC_Init(ADC0, &init);
  }

  uint16_t readADC(const ADC_SingleInput_TypeDef input) {
    ADC_InitSingle_TypeDef sInit = ADC_INITSINGLE_DEFAULT;
    sInit.input = input;
    sInit.reference = adcRef1V25;
    sInit.acqTime = adcAcqTime32;
    ADC_InitSingle(ADC0, &sInit);
    ADC_Start(ADC0, adcStartSingle);

    while ( ADC0->STATUS & ADC_STATUS_SINGLEACT);

    return ADC_DataSingleGet(ADC0);
  }

  void measure (__attribute__((unused)) bool async=false)  __attribute__((optimize("-O0"))) {
    digitalWrite(en1,HIGH);
    _delay_us(20);
    digitalWrite(en2,HIGH);
    _delay_us(50);

    uint16_t value = readADC(SENS_CH);

    digitalWrite(en1,LOW);
    digitalWrite(en2,LOW);
    //DPRINT("value=");DDECLN(value);
    static uint8_t state = State::NoPos;

    if (state != State::PosB && value < OPT_TRG_LEVEL_LOW) {
      state = State::PosB;
    }

    if (state != State::PosA && value > OPT_TRG_LEVEL_HIGH) {
      state = State::PosA;
    }

    _position = state;
  }
  uint32_t interval () { return millis2ticks(500); }
};

class SCoChannel : public StateGenericChannel<SCoOptPosition,Hal,SCOList0,SCOList1,List4,PEERS_PER_CHANNEL> {
public:
  typedef StateGenericChannel<SCoOptPosition,Hal,SCOList0,SCOList1,List4,PEERS_PER_CHANNEL> BaseChannel;

  SCoChannel () : BaseChannel() {};
  ~SCoChannel () {}

  void init (uint8_t pin, uint8_t en1, uint8_t en2, uint8_t sab) {
    BaseChannel::possens.init(pin, en1, en2);
    BaseChannel::init(sab);
  }
};

typedef StateDevice<Hal,SCoChannel,1,SCOList0, CYCLETIME> SCOType;

SCOType sdev(devinfo,0x20);
ConfigButton<SCOType, HIGH, LOW, INPUT_PULLUP> cfgBtn(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
#ifdef _WIRESOFT_H_
  Wire.begin();
#endif
  sdev.init(hal);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  sdev.initDone();
  DDEVINFO(sdev);
  DPRINT(F("HW Revision: ")); DDECLN(SYSTEM_GetProdRev());
  DPRINT(F("SRAMSize:    ")); DDEC(SYSTEM_GetSRAMSize()); DPRINTLN(F("kB"));
  DPRINT(F("FlashSize:   ")); DDEC(SYSTEM_GetFlashSize()); DPRINTLN(F("kB"));
  DPRINT(F("PageSize:    ")); DDEC(SYSTEM_GetFlashPageSize()); DPRINTLN(F("byte"));

  GPIO_DriveModeSet(gpioPortA, gpioDriveModeLowest);
  GPIO_DriveModeSet(gpioPortB, gpioDriveModeLowest);
  GPIO_DriveModeSet(gpioPortC, gpioDriveModeLowest);
  GPIO_DriveModeSet(gpioPortD, gpioDriveModeLowest);
  GPIO_DriveModeSet(gpioPortE, gpioDriveModeLowest);
  GPIO_DriveModeSet(gpioPortF, gpioDriveModeLowest);

  // https://www.silabs.com/documents/public/application-notes/an0027.pdf
  // 2.1 GPIO Leakage, unused pins should be disabled
  pinMode(PA2,  gpioModeDisabled);
  pinMode(PB7,  gpioModeDisabled);
  pinMode(PB11, gpioModeDisabled);
  pinMode(PE13, gpioModeDisabled);
  pinMode(PF0,  gpioModeDisabled);
  pinMode(PF1,  gpioModeDisabled);
  pinMode(PF2,  gpioModeDisabled);

  sdev.channel(1).init(SENS_CH, SENS_EN_PIN1, SENS_EN_PIN2, SABOTAGE_PIN);

  hal.activity.stayAwake(seconds2ticks(5));

}

void loop() {
  bool worked = hal.runready();

  if (sdev.channel(1).msgSent() == true) {
    sdev.channel(1).msgSent(false);
    //measure battery voltage after a message was sent
    hal.battery.meter().start();
    //restart the cycle alarm if an open/close message was sent
    sdev.restartCycleTimer();
  }

  //measure battery voltage on any info message (cycle, sabotage)
  if (sdev.channel(1).changed() == true) {
    hal.battery.meter().start();
  }

  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    if( hal.battery.critical() ) {
      hal.activity.sleepForever(hal);
    }
    hal.activity.savePower<Sleep >(hal);
  }
}
