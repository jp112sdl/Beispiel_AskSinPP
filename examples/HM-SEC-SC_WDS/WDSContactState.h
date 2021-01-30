//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2017-10-19 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2020-01-30 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------

#ifndef __WDSCONTACTSTATE_H__
#define __WDSCONTACTSTATE_H__


#include "MultiChannelDevice.h"

namespace as {

template <class HALTYPE,class List0Type,class List1Type,class List4Type,int PEERCOUNT>
class WDSStateChannel : public Channel<HALTYPE,List1Type,EmptyList,List4Type,PEERCOUNT,List0Type> {

  class EventSender : public Alarm {
  public:
    WDSStateChannel& channel;
    uint8_t count, state;

    EventSender (WDSStateChannel& c) : Alarm(0), channel(c), count(0), state(255) {}
    virtual ~EventSender () {}
    virtual void trigger (__attribute__ ((unused)) AlarmClock& clock) {
      SensorEventMsg& msg = (SensorEventMsg&)channel.device().message();
      msg.init(channel.device().nextcount(),channel.number(),count++,state,channel.device().battery().low());
      channel.device().sendPeerEvent(msg,channel);
    }
  };

  EventSender sender;

  class CheckAlarm : public Alarm {
  public:
    WDSStateChannel& wc;
    CheckAlarm (WDSStateChannel& _wc) : Alarm(0), wc(_wc) {}
    virtual ~CheckAlarm () {}
    virtual void trigger(__attribute__((unused)) AlarmClock& clock) {
      wc.check();
    }
  };

private:
    uint8_t pin;
protected:
    CheckAlarm ca;
public:
  typedef Channel<HALTYPE,List1Type,EmptyList,List4Type,PEERCOUNT,List0Type> BaseChannel;

  WDSStateChannel () : BaseChannel(), sender(*this), pin(0), ca(*this) {}
  virtual ~WDSStateChannel () {}

  void setup(Device<HALTYPE,List0Type>* dev,uint8_t number,uint16_t addr) {
    BaseChannel::setup(dev,number,addr);
  }

  void irq() {
    sysclock.cancel(ca);
    ca.set(millis2ticks(10));
    sysclock.add(ca);
  }

  void init (uint8_t pin) {
    this->pin = pin;
    pinMode(pin, INPUT);

  }

  uint8_t status () const {
    return sender.state;
  }

  uint8_t flags () const {
    uint8_t flags = 0x00;
    flags |= this->device().battery().low() ? 0x80 : 0x00;
    return flags;
  }

  void check() {
    uint8_t newstate = sender.state;

    uint8_t msg = digitalRead(this->pin) == LOW ? this->getList1().msgForPosA() : this->getList1().msgForPosB();

    if( msg == 1) newstate = 0;
    else if( msg == 2) newstate = 200;

    if( sender.state == 255 ) {
      // we are in the init stage - store state only
      sender.state = newstate;
    }
    else if( newstate != sender.state ) {
      uint8_t delay = this->getList1().eventDelaytime();
      sender.state = newstate;
      sysclock.cancel(sender);
      if( delay == 0 ) {
        sender.trigger(sysclock);
      }
      else {
        sender.set(AskSinBase::byteTimeCvtSeconds(delay));
        sysclock.add(sender);
      }
      uint16_t ledtime = (uint16_t)this->getList1().ledOntime() * 5;
      if( ledtime > 0 ) {
        this->device().led().ledOn(millis2ticks(ledtime),0);
      }
    }
  }
};

#define DEFCYCLETIME seconds2ticks(60UL*60*20)
template<class HalType,class ChannelType,int ChannelCount,class List0Type,uint32_t CycleTime=DEFCYCLETIME> // at least one message per day
class StateDevice : public MultiChannelDevice<HalType,ChannelType,ChannelCount,List0Type> {
  class CycleInfoAlarm : public Alarm {
    StateDevice& dev;
  public:
    CycleInfoAlarm (StateDevice& d) : Alarm (CycleTime), dev(d) {}
    virtual ~CycleInfoAlarm () {}

    void trigger (AlarmClock& clock)  {
      set(CycleTime);
      clock.add(*this);
      dev.channel(1).changed(true);
    }
  } cycle;
public:
  typedef MultiChannelDevice<HalType,ChannelType,ChannelCount,List0Type> DevType;
  StateDevice(const DeviceInfo& info,uint16_t addr) : DevType(info,addr), cycle(*this) {}
  virtual ~StateDevice () {}

  virtual void configChanged () {
    if( this->getList0().cycleInfoMsg() == true ) {
      DPRINTLN(F("Activate Cycle Msg"));
      sysclock.cancel(cycle);
      cycle.set(CycleTime);
      sysclock.add(cycle);
    }
    else {
      DPRINTLN(F("Deactivate Cycle Msg"));
      sysclock.cancel(cycle);
    }
  }
};


#define contactISR(chan,pin) class __##pin##ISRHandler { \
  public: \
  static void isr () { chan.irq(); } \
}; \
chan.init(pin); \
if( digitalPinToInterrupt(pin) == NOT_AN_INTERRUPT ) \
  enableInterrupt(pin,__##pin##ISRHandler::isr,CHANGE); \
else \
  attachInterrupt(digitalPinToInterrupt(pin),__##pin##ISRHandler::isr,CHANGE);

}

#endif
