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
#include <sensors/Sht31.h> //https://github.com/adafruit/Adafruit_SHT31

#include <MultiChannelDevice.h>

// we use a Pro Mini
// Arduino pin for the LED
// D4 == PIN 4 on Pro Mini
#define LED_PIN 4
// Arduino pin for the config button
// B0 == PIN 8 on Pro Mini
#define CONFIG_BUTTON_PIN 8


//-----------------------------------------------------------------------------------------

//Korrektur von Temperatur und Luftfeuchte
//Einstellbarer OFFSET für Temperatur -> gemessene Temp +/- Offset = Angezeigte Temp.
#define OFFSETtemp 0 //z.B -50 ≙ -5°C / 50 ≙ +5°C

//Einstellbarer OFFSET für Luftfeuchte -> gemessene Luftf. +/- Offset = Angezeigte Luftf.
#define OFFSEThumi 0 //z.B -10 ≙ -10%RF / 10 ≙ +10%RF

//-----------------------------------------------------------------------------------------

// number of available peers per channel
#define PEERS_PER_CHANNEL 6

//seconds between sending messages
#define MSG_INTERVAL 180

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x34, 0x3f, 0x00},     // Device ID
  "JPTH10I000",           // Device Serial
  //{0x00, 0x3d},           // Device Model Outdoor
  {0x00, 0x3f},           // Device Model Indoor
  0x10,                   // Firmware Version
  as::DeviceType::THSensor, // Device Type
  {0x01, 0x00}            // Info Bytes
};

/**
   Configure the used hardware
*/
typedef AvrSPI<10, 11, 12, 13> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, BatterySensor, RadioType> Hal;
Hal hal;

class WeatherEventMsg : public Message {
  public:
    void init(uint8_t msgcnt, int16_t temp, uint8_t humidity, bool batlow) {
      uint8_t t1 = (temp >> 8) & 0x7f;
      uint8_t t2 = temp & 0xff;
      if ( batlow == true ) {
        t1 |= 0x80; // set bat low bit
      }
      Message::init(0xc, msgcnt, 0x70, BIDI | WKMEUP, t1, t2);
      pload[0] = humidity;
    }
};

class WeatherChannel : public Channel<Hal, List1, EmptyList, List4, PEERS_PER_CHANNEL, List0>, public Alarm {

    WeatherEventMsg msg;
    int16_t         temp;
    uint8_t         humidity;
    Sht31<>         sht31; 
    uint16_t        millis;

  public:
    WeatherChannel () : Channel(), Alarm(5), temp(0), humidity(0), millis(0) {}
    virtual ~WeatherChannel () {}


    // here we do the measurement
    void measure () {
      DPRINT("Measure...\n");
      sht31.measure();
      temp = sht31.temperature();
      humidity = sht31.humidity();
      DPRINT("T/H = " + String(temp+OFFSETtemp) + "/" + String(humidity+OFFSEThumi) + "\n");
    }

    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      uint8_t msgcnt = device().nextcount();
      // reactivate for next measure
      tick = delay();
      clock.add(*this);
      measure();

      msg.init(msgcnt, temp+OFFSETtemp, humidity+OFFSEThumi, device().battery().low());
      if (msgcnt % 20 == 1) device().sendPeerEvent(msg, *this); else device().broadcastEvent(msg, *this);
    }

    uint32_t delay () {
      return seconds2ticks(MSG_INTERVAL);
    }
    void setup(Device<Hal, List0>* dev, uint8_t number, uint16_t addr) {
      Channel::setup(dev, number, addr);
      sht31.init();
      sysclock.add(*this);
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }
};

typedef MultiChannelDevice<Hal, WeatherChannel, 1> WeatherType;
WeatherType sdev(devinfo, 0x20);
ConfigButton<WeatherType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  hal.initBattery(60UL * 60, 22, 19);
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
