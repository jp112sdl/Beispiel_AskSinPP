//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2018-04-16 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER


#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <SPI.h>  // after including SPI Library - we can use LibSPI class

#include <AskSinPP.h>
#include <LowPower.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include "FreeMono9pt7bMod.h"

#include <Register.h>
#include <MultiChannelDevice.h>
#include "Icons.h"

#define CONFIG_BUTTON_PIN 13
#define LED_PIN            4
#define BTN1_PIN          14
#define BTN2_PIN          15

#define CENTER_TEXT
#define TEXT_LENGTH       12

#define DISPLAY_LINES      6
#define DISPLAY_ROTATE     1 // 0 = 0° , 1 = 90°, 2 = 180°, 3 = 270°
#define ICON_WIDTH        20
#define ICON_HEIGHT       20

#define TFT_LED     6
#define TFT_CS      8
#define TFT_MOSI    9
#define TFT_RST    10
#define TFT_SCK    11
#define TFT_DC     12
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);
enum HMColors { clWHITE, clRED, clORANGE, clYELLOW, clGREEN, clBLUE };

// number of available peers per channel
#define PEERS_PER_CHANNEL 8

#define MSG_START_KEY 0x02
#define MSG_COLOR_KEY 0x11
#define MSG_TEXT_KEY  0x12
#define MSG_ICON_KEY  0x13

// all library classes are placed in the namespace 'as'
using namespace as;

bool bTurnDisplayOn = false;
bool bDisplayStandbyTimerRunning = false;
void enableDisplayLED(bool state) {
  (state == true) ? digitalWrite(TFT_LED, HIGH) :   digitalWrite(TFT_LED, LOW);
}
// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0xd3, 0x00},          // Device ID
  "JPDISP0000",                // Device Serial
  {0x00, 0xD3},                // Device Model
  0x10,                        // Firmware Version
  as::DeviceType::Remote,      // Device Type
  {0x00, 0x00}                 // Info Bytes
};

typedef struct {
  uint8_t Color = 0x00;
  uint8_t Icon = 0xff;
  String Text = "";
} DisplayInfo;

DisplayInfo DisplayLine[DISPLAY_LINES];

String List1Texts[20];

/**
   Configure the used hardware
*/
typedef LibSPI<53> SPIType;
typedef Radio<SPIType, 2> RadioType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType, NoBattery, RadioType> Hal;
Hal hal;

DEFREGISTER(Reg0, MASTERID_REGS, 0x07, 0x0e)
class DispList0 : public RegList0<Reg0> {
  public:
    DispList0(uint16_t addr) : RegList0<Reg0>(addr) {}

    bool STANDBY_TIME (uint8_t value) const {
      return this->writeRegister(0x0e, value & 0xff);
    }
    uint8_t STANDBY_TIME () const {
      return this->readRegister(0x0e, 0);
    }

    bool LANGUAGE (uint8_t value) const {
      return this->writeRegister(0x07, value & 0xff);
    }
    uint8_t LANGUAGE () const {
      return this->readRegister(0x07, 0);
    }
    void defaults () {
      clear();
      STANDBY_TIME(5);
      LANGUAGE(1);
    }
};

DEFREGISTER(RemoteReg1, CREG_AES_ACTIVE, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51)
class RemoteList1 : public RegList1<RemoteReg1> {
  public:
    RemoteList1 (uint16_t addr) : RegList1<RemoteReg1>(addr) {}

    bool TEXT1 (uint8_t value[TEXT_LENGTH]) const {
      for (int i = 0; i < TEXT_LENGTH; i++) {
        this->writeRegister(0x36 + i, value[i] & 0xff);
      }
      return true;
    }
    String TEXT1 () const {
      String a = "";
      for (int i = 0; i < TEXT_LENGTH; i++) {
        byte b = this->readRegister(0x36 + i, 0x20);
        if (b == 0) b = 32;
        a += char(b);
      }
      return a;
    }


    bool TEXT2 (uint8_t value[TEXT_LENGTH]) const {
      for (int i = 0; i < TEXT_LENGTH; i++) {
        this->writeRegister(0x46 + i, value[i] & 0xff);
      }
      return true;
    }
    String TEXT2 () const {
      String a = "";
      for (int i = 0; i < TEXT_LENGTH; i++) {
        byte b = this->readRegister(0x46 + i, 0x20);
        if (b == 0) b = 32;
        a += char(b);
      }
      return a;
    }

    void defaults () {
      clear();
      uint8_t initValues[TEXT_LENGTH] = {32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32};
      //TEXT1(initValues);
      //TEXT2(initValues);
    }
};

void initDisplayLines() {
  for (int i = 0; i < DISPLAY_LINES; i++) {
    DisplayLine[i].Color = 0x00;
    DisplayLine[i].Icon = 0xff;
    DisplayLine[i].Text = "";
  }
}

class DispChannel : public Channel<Hal, RemoteList1, EmptyList, DefList4, PEERS_PER_CHANNEL, DispList0>,
  public Button {

  private:
    uint8_t       repeatcnt;
    volatile bool isr;
    uint8_t       commandIdx;
    uint8_t       command[112];

  public:

    DispChannel () : Channel(), repeatcnt(0), isr(false), commandIdx(0) {}
    virtual ~DispChannel () {}

    Button& button () {
      return *(Button*)this;
    }

    void configChanged() {
      List1Texts[(number() - 1)  * 2] = this->getList1().TEXT1();
      List1Texts[((number() - 1) * 2) + 1] = this->getList1().TEXT2();
      //DDEC(number()); DPRINT(F(" - TEXT1 = ")); DPRINTLN(this->getList1().TEXT1());
      //DDEC(number()); DPRINT(F(" - TEXT2 = ")); DPRINTLN(this->getList1().TEXT2());
    }

    uint8_t status () const {
      return 0;
    }

    uint8_t flags () const {
      return 0;
    }

    bool process (const Message& msg) {
      DPRINTLN("process Message");
      return true;
    }


    bool process (const ActionCommandMsg& msg) {
      static bool getText = false;
      String Text = "";
      for (int i = 0; i < msg.len(); i++) {
        command[commandIdx] = msg.value(i);
        commandIdx++;
      }

      if (msg.eot()) {
        static uint8_t currentLine = 0;
        if (commandIdx > 0 && command[0] == MSG_START_KEY) {
          //DPRINT("RECV: ");
          for (int i = 0; i < commandIdx; i++) {
            //DHEX(command[i]); DPRINT(" ");

            if (getText) {
              if (command[i] >= 0x20 && command[i] < 0x80) {
                char c = command[i];
                Text += c;
              } else {
                getText = false;
                DisplayLine[currentLine].Text = Text;
              }
            }

            if (command[i] == MSG_TEXT_KEY) {
              if (command[i + 1] < 0x80) {
                getText = true;
              } else {
                uint8_t textNum = command[i + 1] - 0x80;
                //DPRINT("USE PRECONF TEXT NUMBER "); DDECLN(textNum);
                DisplayLine[currentLine].Text =  List1Texts[textNum];
              }
            }

            if (command[i] == MSG_COLOR_KEY) {
              DisplayLine[currentLine].Color = command[i + 1] - 0x80;
            }

            if (command[i] == MSG_ICON_KEY) {
              DisplayLine[currentLine].Icon = command[i + 1] - 0x80;
            }

            if (command[i] == AS_ACTION_COMMAND_EOL) {
              currentLine++;
              Text = "";
              getText = false;
            }
          }
        }
        //DPRINTLN("");
        currentLine = 0;
        commandIdx = 0;
        memset(command, 0, sizeof(command));

        /*for (int i = 0; i < DISPLAY_LINES; i++) {
          DPRINT("ROW "); DDEC(i + 1); DPRINT(" COLOR = "); DDEC(DisplayLine[i].Color); DPRINT(" ICON = "); DDEC(DisplayLine[i].Icon); DPRINT(" TEXT = "); DPRINT(DisplayLine[i].Text); DPRINTLN("");
          }*/

        bDisplayStandbyTimerRunning = false;
        bTurnDisplayOn = true;
      }

      return true;
    }

    bool process (const RemoteEventMsg& msg) {
      return true;
    }

    virtual void state(uint8_t s) {
      DHEX(Channel::number());
      Button::state(s);
      RemoteEventMsg& msg = (RemoteEventMsg&)this->device().message();
      msg.init(this->device().nextcount(), this->number(), repeatcnt, (s == longreleased || s == longpressed), this->device().battery().low());
      if ( s == released || s == longreleased) {
        this->device().sendPeerEvent(msg, *this);
        repeatcnt++;
      }
      else if (s == longpressed) {
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


class DisplayDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, DispList0>, 10, DispList0> {
    class displayStandbyTimerAlarm : public Alarm {
        DisplayDevice& dev;
      public:
        displayStandbyTimerAlarm (DisplayDevice& d) : Alarm (0), dev(d) {}
        virtual ~displayStandbyTimerAlarm () {}
        void trigger (AlarmClock& clock)  {
          dev.stopDisplayStandbyTimer();
        }
    } displayStandbyTimer;

  public:
    VirtChannel<Hal, DispChannel, DispList0> c1, c2, c3, c4, c5, c6, c7, c8, c9, c10;
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, DispList0>, 10, DispList0> DeviceType;
    DisplayDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr), displayStandbyTimer(*this) {
      DeviceType::registerChannel(c1, 1);
      DeviceType::registerChannel(c2, 2);
      DeviceType::registerChannel(c3, 3);
      DeviceType::registerChannel(c4, 4);
      DeviceType::registerChannel(c5, 5);
      DeviceType::registerChannel(c6, 6);
      DeviceType::registerChannel(c7, 7);
      DeviceType::registerChannel(c8, 8);
      DeviceType::registerChannel(c9, 9);
      DeviceType::registerChannel(c10, 10);
    }
    virtual ~DisplayDevice () {}

    DispChannel& disp1Channel ()  {
      return c1;
    }
    DispChannel& disp2Channel ()  {
      return c2;
    }

    void stopDisplayStandbyTimer() {
      //DPRINTLN("DISPLAY AUS");
      bDisplayStandbyTimerRunning = false;
      enableDisplayLED(false);
      bTurnDisplayOn = false;
      sysclock.cancel(displayStandbyTimer);
    }

    void startDisplayStandbyTimer() {
      //DPRINTLN("DISPLAY EIN");
      bDisplayStandbyTimerRunning = true;
      enableDisplayLED(true);
      sysclock.cancel(displayStandbyTimer);
      displayStandbyTimer.set(seconds2ticks(this->getList0().STANDBY_TIME()));
      sysclock.add(displayStandbyTimer);
    }

    virtual void configChanged () {
      sysclock.cancel(displayStandbyTimer);
      displayStandbyTimer.set(seconds2ticks(this->getList0().STANDBY_TIME()));
      sysclock.add(displayStandbyTimer);
    }
};
DisplayDevice sdev(devinfo, 0x20);
ConfigButton<DisplayDevice> cfgBtn(sdev);

static void isr1 () {
  sdev.startDisplayStandbyTimer();
  sdev.disp1Channel().irq();
}

static void isr2 () {
  sdev.startDisplayStandbyTimer();
  sdev.disp2Channel().irq();
}

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  sdev.disp1Channel().button().init(BTN1_PIN);
  sdev.disp2Channel().button().init(BTN2_PIN);
  if ( digitalPinToInterrupt(BTN1_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(BTN1_PIN, isr1, CHANGE); else attachInterrupt(digitalPinToInterrupt(BTN1_PIN), isr1, CHANGE);
  if ( digitalPinToInterrupt(BTN2_PIN) == NOT_AN_INTERRUPT ) enableInterrupt(BTN2_PIN, isr2, CHANGE); else attachInterrupt(digitalPinToInterrupt(BTN2_PIN), isr2, CHANGE);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();
  sdev.disp1Channel().changed(true);

  initDisplay();
}


void loop() {
  if (bTurnDisplayOn && !bDisplayStandbyTimerRunning) {
    for (int i = 0; i < DISPLAY_LINES; i++) {
      drawLine(i + 1, DisplayLine[i].Color, DisplayLine[i].Icon, DisplayLine[i].Text);
    }
    initDisplayLines();
    sdev.startDisplayStandbyTimer();
  }

  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Idle<>>(hal);
  }
}

void initDisplay() {
  tft.initR(INITR_GREENTAB);
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(DISPLAY_ROTATE);
  tft.setFont(&FreeMono9pt7bMod);
  tft.setTextWrap(false);
  drawLine(2, clWHITE, 255, F("------------"));
  drawLine(3, clRED, 5, F("AskSin++"));
  drawLine(4, clYELLOW, 6, F("V " ASKSIN_PLUS_PLUS_VERSION));
  drawLine(5, clWHITE, 255, F("------------"));
}

void drawLine(uint8_t rowNum, uint8_t colorNum, uint8_t iconNum, String text) {
  clearLine(rowNum);
  drawText(rowNum, colorNum, text);
  drawIcon(rowNum, iconNum);
}

void clearLine(uint8_t rowNum) {
  int8_t rowOffset = ((rowNum - 1) * (ICON_HEIGHT + 1)) + 1;
  tft.fillRect(0, rowOffset, tft.width(), ICON_HEIGHT, ST7735_BLACK);
}

static const uint16_t * icons[12] = {icon0, icon1, icon2, icon3, icon4, icon5, icon6, icon7, icon8, icon9, icon10, icon11};
void drawIcon(uint8_t rowNum, uint8_t iconNum) {
  uint8_t colOffset = tft.width() - ICON_WIDTH - 2;
  uint8_t rowOffset = ((rowNum - 1) * (ICON_HEIGHT + 1)) + 1;
  uint8_t row, col;
  uint16_t buffidx = 0;

  if (iconNum != 0xff) {
    for (row = rowOffset; row < ICON_WIDTH + rowOffset; row++) {
      for (col = colOffset; col < ICON_WIDTH + colOffset; col++) {
        tft.drawPixel(col, row, pgm_read_word(icons[iconNum] + buffidx));
        buffidx++;
      }
    }
  } else {
    tft.fillRect(tft.width() - ICON_WIDTH - 2, ((rowNum - 1) * (ICON_HEIGHT + 1)) + 1, ICON_WIDTH, ICON_HEIGHT, ST7735_BLACK);
  }
}

void drawText(uint8_t rowNum, uint8_t colorNum, String text) {
  switch (colorNum) {
    case clWHITE:
      tft.setTextColor(0xFFFF);
      break;
    case clRED:
      tft.setTextColor(0xF800);
      break;
    case clORANGE:
      tft.setTextColor(0xFBE0);
      break;
    case clYELLOW:
      tft.setTextColor(0xFFE0);
      break;
    case clGREEN:
      tft.setTextColor(0x07E0);
      break;
    case clBLUE:
      tft.setTextColor(0x001F);
      break;
    default:
      tft.setTextColor(0xFFFF);
  }

  text.trim();

  uint8_t leftOffset = 1;
#ifdef CENTER_TEXT
  leftOffset = (((TEXT_LENGTH - text.length())) + 1) * 4.5;
#endif

  tft.setCursor(leftOffset, 12 + ((rowNum - 1) * 22));

  tft.print(text);
}
