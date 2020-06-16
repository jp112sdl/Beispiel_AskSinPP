//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa     Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2019-09-17 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <Register.h>
#include <MultiChannelDevice.h>

#define CONFIG_BUTTON_PIN 8
#define DREG_COMPATIBILITY_MODE 0x17

using namespace as;

const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0x76, 0x01},     // Device ID
  "REPEATER01",           // Device Serial
  {0x00, 0x76},           // Device Model
  0x10,                   // Firmware Version
  0x40,                   // Device Type
  {0x01, 0x00}            // Info Bytes
};
typedef AskSin<StatusLed<4>, NoBattery, Radio<AvrSPI<10, 11, 12, 13>, 2, 100>> Hal;
Hal hal;

DEFREGISTER(UReg0, MASTERID_REGS, DREG_COMPATIBILITY_MODE)
class UList0 : public RegList0<UReg0> {
  public:
    UList0 (uint16_t addr) : RegList0<UReg0>(addr) {}

    bool compatbilityMode (uint8_t value) const {
      return this->writeRegister(DREG_COMPATIBILITY_MODE, value & 0xff);
    }
    bool compatbilityMode () const {
      return this->readRegister(DREG_COMPATIBILITY_MODE, 0);
    }

    void defaults () {
      clear();
      compatbilityMode(0);
    }
};

DEFREGISTER(Reg2, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252)
class List2 : public RegList2<Reg2> {
  public:
  List2 (uint16_t addr) : RegList2<Reg2>(addr) {}
  void defaults () {
    clear();
  }
};

class RepeaterChannel : public Channel<Hal,List1,EmptyList,EmptyList,1,UList0,List2> {
private:
  struct RepeaterPartner {
    HMID SENDER;
    HMID RECEIVER;
    bool BCAST;
  };
public:
  RepeaterPartner RepeaterPartnerDevices[36];

  typedef Channel<Hal,List1,EmptyList,EmptyList,1,UList0,List2> BaseChannel;

  RepeaterChannel () : BaseChannel() {}
  virtual ~RepeaterChannel () {}

  uint8_t status () const {
    return 0;
  }

  uint8_t flags () const {
    return 0;
  }

  void dumpRepeatedDevices() {
    for (uint8_t i = 0; i < 36; i++) {
      if (i < 9) DPRINT("0");
      DDEC(i+1);
      DPRINT(": ");
      RepeaterPartnerDevices[i].SENDER.dump();
      DPRINT(" ");
      RepeaterPartnerDevices[i].RECEIVER.dump();
      DPRINT(" ");

      if (i % 2 == 1)
        DDECLN(RepeaterPartnerDevices[i].BCAST);
      else
        { DDEC(RepeaterPartnerDevices[i].BCAST); DPRINT(" | "); }
    }
  }

  bool configChanged() {
    for (uint8_t i = 0; i < 36; i++) {
      RepeaterPartnerDevices[i]  = {
          { getList2().ADDRESS_SENDER_HIGH_BYTE(i)  , getList2().ADDRESS_SENDER_MID_BYTE(i)  , getList2().ADDRESS_SENDER_LOW_BYTE(i)   },
          { getList2().ADDRESS_RECEIVER_HIGH_BYTE(i), getList2().ADDRESS_RECEIVER_MID_BYTE(i), getList2().ADDRESS_RECEIVER_LOW_BYTE(i) },
          getList2().BROADCAST_BEHAVIOR(i)};
    }
    dumpRepeatedDevices();

    return true;
  }
};

class RepeaterDevice : public ChannelDevice<Hal, VirtBaseChannel<Hal, UList0>, 1, UList0> {
  public:
    VirtChannel<Hal, RepeaterChannel , List0> c1;
  public:
    typedef ChannelDevice<Hal, VirtBaseChannel<Hal, List0>, 1, UList0> DeviceType;
    RepeaterDevice (const DeviceInfo& info, uint16_t addr) : DeviceType(info, addr) {
      DeviceType::registerChannel(c1, 1);
    }
    virtual ~RepeaterDevice () {}
    RepeaterChannel& RepChannel () { return c1; }

    void unsetRptEn(Message& msg) {
      uint8_t f = msg.flags();
      f -= Message::RPTEN;
      msg.flags(f);
    }

    void broadcastRptEvent (Message& msg, const HMID& sender) {
      msg.clearAck();
      msg.burstRequired(false);
      msg.setBroadcast();
      msg.setRepeated();
      unsetRptEn(msg);
      msg.from(sender);
      msg.to(HMID::broadcast);
      send(msg);
    }

    void resendBidiMsg(Message& msg, const HMID& sender, const HMID& receiver) {
      msg.setRepeated();
      unsetRptEn(msg);
      msg.from(sender);
      msg.to(receiver);
      send(msg);
    }

    virtual bool process(Message& msg) {
      HMID me;
      getDeviceID(me);

      //Message is not from/for myself , was not repeated (! RPTED) and has repeating enabled (RPTEN)
      if (msg.from() != me &&
          msg.to()   != me &&
         (msg.flags() & Message::RPTEN) &&
        !(msg.flags() & Message::RPTED)) {

        uint8_t partnerIdx = 255;

        //look in RepeaterPartnerDevices, if Message is from/for any of them
        for (uint8_t i = 0; i < 36; i++) {
          if (
               (
                ( RepChannel().RepeaterPartnerDevices[i].SENDER   == msg.from() && RepChannel().RepeaterPartnerDevices[i].RECEIVER == msg.to() ) ||
                ( RepChannel().RepeaterPartnerDevices[i].RECEIVER == msg.from() && RepChannel().RepeaterPartnerDevices[i].SENDER   == msg.to() ) ||
                ( RepChannel().RepeaterPartnerDevices[i].SENDER   == msg.from() && (msg.flags() & Message::BCAST) )
               )
              ) {
            partnerIdx = i;
            break;
          }
        }

        //a device was found in RepeaterPartnerDevices
        if (partnerIdx < 255) {
          //_delay_ms(10);
          if (msg.flags() & Message::BCAST) {
            if (RepChannel().RepeaterPartnerDevices[partnerIdx].BCAST) {
              DPRINT(F("Repeating BCAST Message: "));
              broadcastRptEvent(msg, msg.from());
            } else return ChannelDevice::process(msg);
          } else {
            DPRINT(F("Repeating  BIDI Message: "));
            resendBidiMsg(msg, msg.from(), msg.to());
          }
          //msg.dump();
          return true;
        }

      }
      //nothing for us to do - run main msg processing
      return ChannelDevice::process(msg);
    }
};

RepeaterDevice sdev(devinfo, 0x20);
ConfigButton<RepeaterDevice> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);
  sdev.initDone();
  //sdev.RepChannel().changed(true);
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if ( worked == false && poll == false ) {
    hal.activity.savePower<Idle<true>>(hal);
  }
}
