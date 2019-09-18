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
#include "MultiChannelDeviceMod.h"

#define CONFIG_BUTTON_PIN 8
#define DREG_COMPATIBILITY_MODE 0x17

using namespace as;

const struct DeviceInfo PROGMEM devinfo = {
  {0x00, 0x76, 0x01},     // Device ID
  "REP0000001",           // Device Serial
  {0x00, 0x76},           // Device Model
  0x10,                   // Firmware Version
  0x40,                   // Device Type
  {0x01, 0x00}            // Info Bytes
};
typedef AskSin<StatusLed<4>, NoBattery, Radio<AvrSPI<10, 11, 12, 13>, 2>> Hal;
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
class RepList2 : public RegisterList<Reg2> {
public:
  RepList2 (uint16_t addr) : RegisterList<Reg2>(addr) {}
  bool    ADDRESS_SENDER1_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(1  , value & 0xff); }
  uint8_t ADDRESS_SENDER1_HIGH_BYTE    ()              const { return this->readRegister (1  , 0);            }
  bool    ADDRESS_SENDER1_MID_BYTE     (uint8_t value) const { return this->writeRegister(2  , value & 0xff); }
  uint8_t ADDRESS_SENDER1_MID_BYTE     ()              const { return this->readRegister (2  , 0);            }
  bool    ADDRESS_SENDER1_LOW_BYTE     (uint8_t value) const { return this->writeRegister(3  , value & 0xff); }
  uint8_t ADDRESS_SENDER1_LOW_BYTE     ()              const { return this->readRegister (3  , 0);            }

  bool    ADDRESS_RECEIVER1_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(4  , value & 0xff); }
  uint8_t ADDRESS_RECEIVER1_HIGH_BYTE  ()              const { return this->readRegister (4  , 0);            }
  bool    ADDRESS_RECEIVER1_MID_BYTE   (uint8_t value) const { return this->writeRegister(5  , value & 0xff); }
  uint8_t ADDRESS_RECEIVER1_MID_BYTE   ()              const { return this->readRegister (5  , 0);            }
  bool    ADDRESS_RECEIVER1_LOW_BYTE   (uint8_t value) const { return this->writeRegister(6  , value & 0xff); }
  uint8_t ADDRESS_RECEIVER1_LOW_BYTE   ()              const { return this->readRegister (6  , 0);            }

  bool    BROADCAST_BEHAVIOR1          (uint8_t value) const { return this->writeRegister(7  , value & 0xff); }
  bool    BROADCAST_BEHAVIOR1          ()              const { return this->readRegister (7  , 0);            }

  bool    ADDRESS_SENDER2_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(8  , value & 0xff); }
  uint8_t ADDRESS_SENDER2_HIGH_BYTE    ()              const { return this->readRegister (8  , 0);            }
  bool    ADDRESS_SENDER2_MID_BYTE     (uint8_t value) const { return this->writeRegister(9  , value & 0xff); }
  uint8_t ADDRESS_SENDER2_MID_BYTE     ()              const { return this->readRegister (9  , 0);            }
  bool    ADDRESS_SENDER2_LOW_BYTE     (uint8_t value) const { return this->writeRegister(10 , value & 0xff); }
  uint8_t ADDRESS_SENDER2_LOW_BYTE     ()              const { return this->readRegister (10 , 0);            }

  bool    ADDRESS_RECEIVER2_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(11 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER2_HIGH_BYTE  ()              const { return this->readRegister (11 , 0);            }
  bool    ADDRESS_RECEIVER2_MID_BYTE   (uint8_t value) const { return this->writeRegister(12 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER2_MID_BYTE   ()              const { return this->readRegister (12 , 0);            }
  bool    ADDRESS_RECEIVER2_LOW_BYTE   (uint8_t value) const { return this->writeRegister(13 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER2_LOW_BYTE   ()              const { return this->readRegister (13 , 0);            }

  bool    BROADCAST_BEHAVIOR2          (uint8_t value) const { return this->writeRegister(14 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR2          ()              const { return this->readRegister (14 , 0);            }

  bool    ADDRESS_SENDER3_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(15 , value & 0xff); }
  uint8_t ADDRESS_SENDER3_HIGH_BYTE    ()              const { return this->readRegister (15 , 0);            }
  bool    ADDRESS_SENDER3_MID_BYTE     (uint8_t value) const { return this->writeRegister(16 , value & 0xff); }
  uint8_t ADDRESS_SENDER3_MID_BYTE     ()              const { return this->readRegister (16 , 0);            }
  bool    ADDRESS_SENDER3_LOW_BYTE     (uint8_t value) const { return this->writeRegister(17 , value & 0xff); }
  uint8_t ADDRESS_SENDER3_LOW_BYTE     ()              const { return this->readRegister (17 , 0);            }

  bool    ADDRESS_RECEIVER3_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(18 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER3_HIGH_BYTE  ()              const { return this->readRegister (18 , 0);            }
  bool    ADDRESS_RECEIVER3_MID_BYTE   (uint8_t value) const { return this->writeRegister(19 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER3_MID_BYTE   ()              const { return this->readRegister (19 , 0);            }
  bool    ADDRESS_RECEIVER3_LOW_BYTE   (uint8_t value) const { return this->writeRegister(20 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER3_LOW_BYTE   ()              const { return this->readRegister (20 , 0);            }

  bool    BROADCAST_BEHAVIOR3          (uint8_t value) const { return this->writeRegister(21 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR3          ()              const { return this->readRegister (21 , 0);            }

  bool    ADDRESS_SENDER4_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(22 , value & 0xff); }
  uint8_t ADDRESS_SENDER4_HIGH_BYTE    ()              const { return this->readRegister (22 , 0);            }
  bool    ADDRESS_SENDER4_MID_BYTE     (uint8_t value) const { return this->writeRegister(23 , value & 0xff); }
  uint8_t ADDRESS_SENDER4_MID_BYTE     ()              const { return this->readRegister (23 , 0);            }
  bool    ADDRESS_SENDER4_LOW_BYTE     (uint8_t value) const { return this->writeRegister(24 , value & 0xff); }
  uint8_t ADDRESS_SENDER4_LOW_BYTE     ()              const { return this->readRegister (24 , 0);            }

  bool    ADDRESS_RECEIVER4_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(25 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER4_HIGH_BYTE  ()              const { return this->readRegister (25 , 0);            }
  bool    ADDRESS_RECEIVER4_MID_BYTE   (uint8_t value) const { return this->writeRegister(26 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER4_MID_BYTE   ()              const { return this->readRegister (26 , 0);            }
  bool    ADDRESS_RECEIVER4_LOW_BYTE   (uint8_t value) const { return this->writeRegister(27 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER4_LOW_BYTE   ()              const { return this->readRegister (27 , 0);            }

  bool    BROADCAST_BEHAVIOR4          (uint8_t value) const { return this->writeRegister(28 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR4          ()              const { return this->readRegister (28 , 0);            }

  bool    ADDRESS_SENDER5_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(29 , value & 0xff); }
  uint8_t ADDRESS_SENDER5_HIGH_BYTE    ()              const { return this->readRegister (29 , 0);            }
  bool    ADDRESS_SENDER5_MID_BYTE     (uint8_t value) const { return this->writeRegister(30 , value & 0xff); }
  uint8_t ADDRESS_SENDER5_MID_BYTE     ()              const { return this->readRegister (30 , 0);            }
  bool    ADDRESS_SENDER5_LOW_BYTE     (uint8_t value) const { return this->writeRegister(31 , value & 0xff); }
  uint8_t ADDRESS_SENDER5_LOW_BYTE     ()              const { return this->readRegister (31 , 0);            }

  bool    ADDRESS_RECEIVER5_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(32 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER5_HIGH_BYTE  ()              const { return this->readRegister (32 , 0);            }
  bool    ADDRESS_RECEIVER5_MID_BYTE   (uint8_t value) const { return this->writeRegister(33 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER5_MID_BYTE   ()              const { return this->readRegister (33 , 0);            }
  bool    ADDRESS_RECEIVER5_LOW_BYTE   (uint8_t value) const { return this->writeRegister(34 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER5_LOW_BYTE   ()              const { return this->readRegister (34 , 0);            }

  bool    BROADCAST_BEHAVIOR5          (uint8_t value) const { return this->writeRegister(35 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR5          ()              const { return this->readRegister (35 , 0);            }

  bool    ADDRESS_SENDER6_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(36 , value & 0xff); }
  uint8_t ADDRESS_SENDER6_HIGH_BYTE    ()              const { return this->readRegister (36 , 0);            }
  bool    ADDRESS_SENDER6_MID_BYTE     (uint8_t value) const { return this->writeRegister(37 , value & 0xff); }
  uint8_t ADDRESS_SENDER6_MID_BYTE     ()              const { return this->readRegister (37 , 0);            }
  bool    ADDRESS_SENDER6_LOW_BYTE     (uint8_t value) const { return this->writeRegister(38 , value & 0xff); }
  uint8_t ADDRESS_SENDER6_LOW_BYTE     ()              const { return this->readRegister (38 , 0);            }

  bool    ADDRESS_RECEIVER6_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(39 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER6_HIGH_BYTE  ()              const { return this->readRegister (39 , 0);            }
  bool    ADDRESS_RECEIVER6_MID_BYTE   (uint8_t value) const { return this->writeRegister(40 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER6_MID_BYTE   ()              const { return this->readRegister (40 , 0);            }
  bool    ADDRESS_RECEIVER6_LOW_BYTE   (uint8_t value) const { return this->writeRegister(41 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER6_LOW_BYTE   ()              const { return this->readRegister (41 , 0);            }

  bool    BROADCAST_BEHAVIOR6          (uint8_t value) const { return this->writeRegister(42 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR6          ()              const { return this->readRegister (42 , 0);            }

  bool    ADDRESS_SENDER7_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(43 , value & 0xff); }
  uint8_t ADDRESS_SENDER7_HIGH_BYTE    ()              const { return this->readRegister (43 , 0);            }
  bool    ADDRESS_SENDER7_MID_BYTE     (uint8_t value) const { return this->writeRegister(44 , value & 0xff); }
  uint8_t ADDRESS_SENDER7_MID_BYTE     ()              const { return this->readRegister (44 , 0);            }
  bool    ADDRESS_SENDER7_LOW_BYTE     (uint8_t value) const { return this->writeRegister(45 , value & 0xff); }
  uint8_t ADDRESS_SENDER7_LOW_BYTE     ()              const { return this->readRegister (45 , 0);            }

  bool    ADDRESS_RECEIVER7_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(46 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER7_HIGH_BYTE  ()              const { return this->readRegister (46 , 0);            }
  bool    ADDRESS_RECEIVER7_MID_BYTE   (uint8_t value) const { return this->writeRegister(47 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER7_MID_BYTE   ()              const { return this->readRegister (47 , 0);            }
  bool    ADDRESS_RECEIVER7_LOW_BYTE   (uint8_t value) const { return this->writeRegister(48 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER7_LOW_BYTE   ()              const { return this->readRegister (48 , 0);            }

  bool    BROADCAST_BEHAVIOR7          (uint8_t value) const { return this->writeRegister(49 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR7          ()              const { return this->readRegister (49 , 0);            }

  bool    ADDRESS_SENDER8_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(50 , value & 0xff); }
  uint8_t ADDRESS_SENDER8_HIGH_BYTE    ()              const { return this->readRegister (50 , 0);            }
  bool    ADDRESS_SENDER8_MID_BYTE     (uint8_t value) const { return this->writeRegister(51 , value & 0xff); }
  uint8_t ADDRESS_SENDER8_MID_BYTE     ()              const { return this->readRegister (51 , 0);            }
  bool    ADDRESS_SENDER8_LOW_BYTE     (uint8_t value) const { return this->writeRegister(52 , value & 0xff); }
  uint8_t ADDRESS_SENDER8_LOW_BYTE     ()              const { return this->readRegister (52 , 0);            }

  bool    ADDRESS_RECEIVER8_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(53 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER8_HIGH_BYTE  ()              const { return this->readRegister (53 , 0);            }
  bool    ADDRESS_RECEIVER8_MID_BYTE   (uint8_t value) const { return this->writeRegister(54 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER8_MID_BYTE   ()              const { return this->readRegister (54 , 0);            }
  bool    ADDRESS_RECEIVER8_LOW_BYTE   (uint8_t value) const { return this->writeRegister(55 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER8_LOW_BYTE   ()              const { return this->readRegister (55 , 0);            }

  bool    BROADCAST_BEHAVIOR8          (uint8_t value) const { return this->writeRegister(56 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR8          ()              const { return this->readRegister (56 , 0);            }

  bool    ADDRESS_SENDER9_HIGH_BYTE    (uint8_t value) const { return this->writeRegister(57 , value & 0xff); }
  uint8_t ADDRESS_SENDER9_HIGH_BYTE    ()              const { return this->readRegister (57 , 0);            }
  bool    ADDRESS_SENDER9_MID_BYTE     (uint8_t value) const { return this->writeRegister(58 , value & 0xff); }
  uint8_t ADDRESS_SENDER9_MID_BYTE     ()              const { return this->readRegister (58 , 0);            }
  bool    ADDRESS_SENDER9_LOW_BYTE     (uint8_t value) const { return this->writeRegister(59 , value & 0xff); }
  uint8_t ADDRESS_SENDER9_LOW_BYTE     ()              const { return this->readRegister (59 , 0);            }

  bool    ADDRESS_RECEIVER9_HIGH_BYTE  (uint8_t value) const { return this->writeRegister(60 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER9_HIGH_BYTE  ()              const { return this->readRegister (60 , 0);            }
  bool    ADDRESS_RECEIVER9_MID_BYTE   (uint8_t value) const { return this->writeRegister(61 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER9_MID_BYTE   ()              const { return this->readRegister (61 , 0);            }
  bool    ADDRESS_RECEIVER9_LOW_BYTE   (uint8_t value) const { return this->writeRegister(62 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER9_LOW_BYTE   ()              const { return this->readRegister (62 , 0);            }

  bool    BROADCAST_BEHAVIOR9          (uint8_t value) const { return this->writeRegister(63 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR9          ()              const { return this->readRegister (63 , 0);            }

  bool    ADDRESS_SENDER10_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(64 , value & 0xff); }
  uint8_t ADDRESS_SENDER10_HIGH_BYTE   ()              const { return this->readRegister (64 , 0);            }
  bool    ADDRESS_SENDER10_MID_BYTE    (uint8_t value) const { return this->writeRegister(65 , value & 0xff); }
  uint8_t ADDRESS_SENDER10_MID_BYTE    ()              const { return this->readRegister (65 , 0);            }
  bool    ADDRESS_SENDER10_LOW_BYTE    (uint8_t value) const { return this->writeRegister(66 , value & 0xff); }
  uint8_t ADDRESS_SENDER10_LOW_BYTE    ()              const { return this->readRegister (66 , 0);            }

  bool    ADDRESS_RECEIVER10_HIGH_BYTE (uint8_t value) const { return this->writeRegister(67 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER10_HIGH_BYTE ()              const { return this->readRegister (67 , 0);            }
  bool    ADDRESS_RECEIVER10_MID_BYTE  (uint8_t value) const { return this->writeRegister(68 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER10_MID_BYTE  ()              const { return this->readRegister (68 , 0);            }
  bool    ADDRESS_RECEIVER10_LOW_BYTE  (uint8_t value) const { return this->writeRegister(69 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER10_LOW_BYTE  ()              const { return this->readRegister (69 , 0);            }

  bool    BROADCAST_BEHAVIOR10         (uint8_t value) const { return this->writeRegister(70 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR10         ()              const { return this->readRegister (70 , 0);            }

  bool    ADDRESS_SENDER11_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(71 , value & 0xff); }
  uint8_t ADDRESS_SENDER11_HIGH_BYTE   ()              const { return this->readRegister (71 , 0);            }
  bool    ADDRESS_SENDER11_MID_BYTE    (uint8_t value) const { return this->writeRegister(72 , value & 0xff); }
  uint8_t ADDRESS_SENDER11_MID_BYTE    ()              const { return this->readRegister (72 , 0);            }
  bool    ADDRESS_SENDER11_LOW_BYTE    (uint8_t value) const { return this->writeRegister(73 , value & 0xff); }
  uint8_t ADDRESS_SENDER11_LOW_BYTE    ()              const { return this->readRegister (73 , 0);            }

  bool    ADDRESS_RECEIVER11_HIGH_BYTE (uint8_t value) const { return this->writeRegister(74 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER11_HIGH_BYTE ()              const { return this->readRegister (74 , 0);            }
  bool    ADDRESS_RECEIVER11_MID_BYTE  (uint8_t value) const { return this->writeRegister(75 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER11_MID_BYTE  ()              const { return this->readRegister (75 , 0);            }
  bool    ADDRESS_RECEIVER11_LOW_BYTE  (uint8_t value) const { return this->writeRegister(76 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER11_LOW_BYTE  ()              const { return this->readRegister (76 , 0);            }

  bool    BROADCAST_BEHAVIOR11         (uint8_t value) const { return this->writeRegister(77 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR11         ()              const { return this->readRegister (77 , 0);            }

  bool    ADDRESS_SENDER12_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(78 , value & 0xff); }
  uint8_t ADDRESS_SENDER12_HIGH_BYTE   ()              const { return this->readRegister (78 , 0);            }
  bool    ADDRESS_SENDER12_MID_BYTE    (uint8_t value) const { return this->writeRegister(79 , value & 0xff); }
  uint8_t ADDRESS_SENDER12_MID_BYTE    ()              const { return this->readRegister (79 , 0);            }
  bool    ADDRESS_SENDER12_LOW_BYTE    (uint8_t value) const { return this->writeRegister(80 , value & 0xff); }
  uint8_t ADDRESS_SENDER12_LOW_BYTE    ()              const { return this->readRegister (80 , 0);            }

  bool    ADDRESS_RECEIVER12_HIGH_BYTE (uint8_t value) const { return this->writeRegister(81 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER12_HIGH_BYTE ()              const { return this->readRegister (81 , 0);            }
  bool    ADDRESS_RECEIVER12_MID_BYTE  (uint8_t value) const { return this->writeRegister(82 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER12_MID_BYTE  ()              const { return this->readRegister (82, 0);            }
  bool    ADDRESS_RECEIVER12_LOW_BYTE  (uint8_t value) const { return this->writeRegister(83 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER12_LOW_BYTE  ()              const { return this->readRegister (83 , 0);            }

  bool    BROADCAST_BEHAVIOR12         (uint8_t value) const { return this->writeRegister(84 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR12         ()              const { return this->readRegister (84 , 0);            }

  bool    ADDRESS_SENDER13_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(85 , value & 0xff); }
  uint8_t ADDRESS_SENDER13_HIGH_BYTE   ()              const { return this->readRegister (85 , 0);            }
  bool    ADDRESS_SENDER13_MID_BYTE    (uint8_t value) const { return this->writeRegister(86 , value & 0xff); }
  uint8_t ADDRESS_SENDER13_MID_BYTE    ()              const { return this->readRegister (86 , 0);            }
  bool    ADDRESS_SENDER13_LOW_BYTE    (uint8_t value) const { return this->writeRegister(87 , value & 0xff); }
  uint8_t ADDRESS_SENDER13_LOW_BYTE    ()              const { return this->readRegister (87 , 0);            }

  bool    ADDRESS_RECEIVER13_HIGH_BYTE (uint8_t value) const { return this->writeRegister(88 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER13_HIGH_BYTE ()              const { return this->readRegister (88 , 0);            }
  bool    ADDRESS_RECEIVER13_MID_BYTE  (uint8_t value) const { return this->writeRegister(89 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER13_MID_BYTE  ()              const { return this->readRegister (89 , 0);            }
  bool    ADDRESS_RECEIVER13_LOW_BYTE  (uint8_t value) const { return this->writeRegister(90 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER13_LOW_BYTE  ()              const { return this->readRegister (90 , 0);            }

  bool    BROADCAST_BEHAVIOR13         (uint8_t value) const { return this->writeRegister(91 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR13         ()              const { return this->readRegister (91 , 0);            }

  bool    ADDRESS_SENDER14_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(92 , value & 0xff); }
  uint8_t ADDRESS_SENDER14_HIGH_BYTE   ()              const { return this->readRegister (92 , 0);            }
  bool    ADDRESS_SENDER14_MID_BYTE    (uint8_t value) const { return this->writeRegister(93 , value & 0xff); }
  uint8_t ADDRESS_SENDER14_MID_BYTE    ()              const { return this->readRegister (93 , 0);            }
  bool    ADDRESS_SENDER14_LOW_BYTE    (uint8_t value) const { return this->writeRegister(94 , value & 0xff); }
  uint8_t ADDRESS_SENDER14_LOW_BYTE    ()              const { return this->readRegister (94 , 0);            }

  bool    ADDRESS_RECEIVER14_HIGH_BYTE (uint8_t value) const { return this->writeRegister(95 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER14_HIGH_BYTE ()              const { return this->readRegister (95 , 0);            }
  bool    ADDRESS_RECEIVER14_MID_BYTE  (uint8_t value) const { return this->writeRegister(96 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER14_MID_BYTE  ()              const { return this->readRegister (96 , 0);            }
  bool    ADDRESS_RECEIVER14_LOW_BYTE  (uint8_t value) const { return this->writeRegister(97 , value & 0xff); }
  uint8_t ADDRESS_RECEIVER14_LOW_BYTE  ()              const { return this->readRegister (97 , 0);            }

  bool    BROADCAST_BEHAVIOR14         (uint8_t value) const { return this->writeRegister(98 , value & 0xff); }
  bool    BROADCAST_BEHAVIOR14         ()              const { return this->readRegister (98 , 0);            }

  bool    ADDRESS_SENDER15_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(99 , value & 0xff); }
  uint8_t ADDRESS_SENDER15_HIGH_BYTE   ()              const { return this->readRegister (99 , 0);            }
  bool    ADDRESS_SENDER15_MID_BYTE    (uint8_t value) const { return this->writeRegister(100 , value & 0xff); }
  uint8_t ADDRESS_SENDER15_MID_BYTE    ()              const { return this->readRegister (100, 0);            }
  bool    ADDRESS_SENDER15_LOW_BYTE    (uint8_t value) const { return this->writeRegister(101, value & 0xff); }
  uint8_t ADDRESS_SENDER15_LOW_BYTE    ()              const { return this->readRegister (101, 0);            }

  bool    ADDRESS_RECEIVER15_HIGH_BYTE (uint8_t value) const { return this->writeRegister(102, value & 0xff); }
  uint8_t ADDRESS_RECEIVER15_HIGH_BYTE ()              const { return this->readRegister (102, 0);            }
  bool    ADDRESS_RECEIVER15_MID_BYTE  (uint8_t value) const { return this->writeRegister(103, value & 0xff); }
  uint8_t ADDRESS_RECEIVER15_MID_BYTE  ()              const { return this->readRegister (103, 0);            }
  bool    ADDRESS_RECEIVER15_LOW_BYTE  (uint8_t value) const { return this->writeRegister(104, value & 0xff); }
  uint8_t ADDRESS_RECEIVER15_LOW_BYTE  ()              const { return this->readRegister (104, 0);            }

  bool    BROADCAST_BEHAVIOR15         (uint8_t value) const { return this->writeRegister(105, value & 0xff); }
  bool    BROADCAST_BEHAVIOR15         ()              const { return this->readRegister (105, 0);            }

  bool    ADDRESS_SENDER16_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(106, value & 0xff); }
  uint8_t ADDRESS_SENDER16_HIGH_BYTE   ()              const { return this->readRegister (106, 0);            }
  bool    ADDRESS_SENDER16_MID_BYTE    (uint8_t value) const { return this->writeRegister(107, value & 0xff); }
  uint8_t ADDRESS_SENDER16_MID_BYTE    ()              const { return this->readRegister (107, 0);            }
  bool    ADDRESS_SENDER16_LOW_BYTE    (uint8_t value) const { return this->writeRegister(108, value & 0xff); }
  uint8_t ADDRESS_SENDER16_LOW_BYTE    ()              const { return this->readRegister (108, 0);            }

  bool    ADDRESS_RECEIVER16_HIGH_BYTE (uint8_t value) const { return this->writeRegister(109, value & 0xff); }
  uint8_t ADDRESS_RECEIVER16_HIGH_BYTE ()              const { return this->readRegister (109, 0);            }
  bool    ADDRESS_RECEIVER16_MID_BYTE  (uint8_t value) const { return this->writeRegister(110, value & 0xff); }
  uint8_t ADDRESS_RECEIVER16_MID_BYTE  ()              const { return this->readRegister (110, 0);            }
  bool    ADDRESS_RECEIVER16_LOW_BYTE  (uint8_t value) const { return this->writeRegister(111, value & 0xff); }
  uint8_t ADDRESS_RECEIVER16_LOW_BYTE  ()              const { return this->readRegister (111, 0);            }

  bool    BROADCAST_BEHAVIOR16         (uint8_t value) const { return this->writeRegister(112, value & 0xff); }
  bool    BROADCAST_BEHAVIOR16         ()              const { return this->readRegister (112, 0);            }

  bool    ADDRESS_SENDER17_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(113, value & 0xff); }
  uint8_t ADDRESS_SENDER17_HIGH_BYTE   ()              const { return this->readRegister (113, 0);            }
  bool    ADDRESS_SENDER17_MID_BYTE    (uint8_t value) const { return this->writeRegister(114, value & 0xff); }
  uint8_t ADDRESS_SENDER17_MID_BYTE    ()              const { return this->readRegister (114, 0);            }
  bool    ADDRESS_SENDER17_LOW_BYTE    (uint8_t value) const { return this->writeRegister(115, value & 0xff); }
  uint8_t ADDRESS_SENDER17_LOW_BYTE    ()              const { return this->readRegister (115, 0);            }

  bool    ADDRESS_RECEIVER17_HIGH_BYTE (uint8_t value) const { return this->writeRegister(116, value & 0xff); }
  uint8_t ADDRESS_RECEIVER17_HIGH_BYTE ()              const { return this->readRegister (116, 0);            }
  bool    ADDRESS_RECEIVER17_MID_BYTE  (uint8_t value) const { return this->writeRegister(117, value & 0xff); }
  uint8_t ADDRESS_RECEIVER17_MID_BYTE  ()              const { return this->readRegister (117, 0);            }
  bool    ADDRESS_RECEIVER17_LOW_BYTE  (uint8_t value) const { return this->writeRegister(118, value & 0xff); }
  uint8_t ADDRESS_RECEIVER17_LOW_BYTE  ()              const { return this->readRegister (118, 0);            }

  bool    BROADCAST_BEHAVIOR17         (uint8_t value) const { return this->writeRegister(119, value & 0xff); }
  bool    BROADCAST_BEHAVIOR17         ()              const { return this->readRegister (119, 0);            }

  bool    ADDRESS_SENDER18_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(120, value & 0xff); }
  uint8_t ADDRESS_SENDER18_HIGH_BYTE   ()              const { return this->readRegister (120, 0);            }
  bool    ADDRESS_SENDER18_MID_BYTE    (uint8_t value) const { return this->writeRegister(121, value & 0xff); }
  uint8_t ADDRESS_SENDER18_MID_BYTE    ()              const { return this->readRegister (121, 0);            }
  bool    ADDRESS_SENDER18_LOW_BYTE    (uint8_t value) const { return this->writeRegister(122, value & 0xff); }
  uint8_t ADDRESS_SENDER18_LOW_BYTE    ()              const { return this->readRegister (122, 0);            }

  bool    ADDRESS_RECEIVER18_HIGH_BYTE (uint8_t value) const { return this->writeRegister(123, value & 0xff); }
  uint8_t ADDRESS_RECEIVER18_HIGH_BYTE ()              const { return this->readRegister (123, 0);            }
  bool    ADDRESS_RECEIVER18_MID_BYTE  (uint8_t value) const { return this->writeRegister(124, value & 0xff); }
  uint8_t ADDRESS_RECEIVER18_MID_BYTE  ()              const { return this->readRegister (124, 0);            }
  bool    ADDRESS_RECEIVER18_LOW_BYTE  (uint8_t value) const { return this->writeRegister(125, value & 0xff); }
  uint8_t ADDRESS_RECEIVER18_LOW_BYTE  ()              const { return this->readRegister (125, 0);            }

  bool    BROADCAST_BEHAVIOR18         (uint8_t value) const { return this->writeRegister(126, value & 0xff); }
  bool    BROADCAST_BEHAVIOR18         ()              const { return this->readRegister (126, 0);            }

  bool    ADDRESS_SENDER19_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(127, value & 0xff); }
  uint8_t ADDRESS_SENDER19_HIGH_BYTE   ()              const { return this->readRegister (127, 0);            }
  bool    ADDRESS_SENDER19_MID_BYTE    (uint8_t value) const { return this->writeRegister(128, value & 0xff); }
  uint8_t ADDRESS_SENDER19_MID_BYTE    ()              const { return this->readRegister (128, 0);            }
  bool    ADDRESS_SENDER19_LOW_BYTE    (uint8_t value) const { return this->writeRegister(129, value & 0xff); }
  uint8_t ADDRESS_SENDER19_LOW_BYTE    ()              const { return this->readRegister (129, 0);            }

  bool    ADDRESS_RECEIVER19_HIGH_BYTE (uint8_t value) const { return this->writeRegister(130, value & 0xff); }
  uint8_t ADDRESS_RECEIVER19_HIGH_BYTE ()              const { return this->readRegister (130, 0);            }
  bool    ADDRESS_RECEIVER19_MID_BYTE  (uint8_t value) const { return this->writeRegister(131, value & 0xff); }
  uint8_t ADDRESS_RECEIVER19_MID_BYTE  ()              const { return this->readRegister (131, 0);            }
  bool    ADDRESS_RECEIVER19_LOW_BYTE  (uint8_t value) const { return this->writeRegister(132, value & 0xff); }
  uint8_t ADDRESS_RECEIVER19_LOW_BYTE  ()              const { return this->readRegister (132, 0);            }

  bool    BROADCAST_BEHAVIOR19         (uint8_t value) const { return this->writeRegister(133, value & 0xff); }
  bool    BROADCAST_BEHAVIOR19         ()              const { return this->readRegister (133, 0);            }

  bool    ADDRESS_SENDER20_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(134, value & 0xff); }
  uint8_t ADDRESS_SENDER20_HIGH_BYTE   ()              const { return this->readRegister (134, 0);            }
  bool    ADDRESS_SENDER20_MID_BYTE    (uint8_t value) const { return this->writeRegister(135, value & 0xff); }
  uint8_t ADDRESS_SENDER20_MID_BYTE    ()              const { return this->readRegister (135, 0);            }
  bool    ADDRESS_SENDER20_LOW_BYTE    (uint8_t value) const { return this->writeRegister(136, value & 0xff); }
  uint8_t ADDRESS_SENDER20_LOW_BYTE    ()              const { return this->readRegister (136, 0);            }

  bool    ADDRESS_RECEIVER20_HIGH_BYTE (uint8_t value) const { return this->writeRegister(137, value & 0xff); }
  uint8_t ADDRESS_RECEIVER20_HIGH_BYTE ()              const { return this->readRegister (137, 0);            }
  bool    ADDRESS_RECEIVER20_MID_BYTE  (uint8_t value) const { return this->writeRegister(138, value & 0xff); }
  uint8_t ADDRESS_RECEIVER20_MID_BYTE  ()              const { return this->readRegister (138, 0);            }
  bool    ADDRESS_RECEIVER20_LOW_BYTE  (uint8_t value) const { return this->writeRegister(139, value & 0xff); }
  uint8_t ADDRESS_RECEIVER20_LOW_BYTE  ()              const { return this->readRegister (139, 0);            }

  bool    BROADCAST_BEHAVIOR20         (uint8_t value) const { return this->writeRegister(140, value & 0xff); }
  bool    BROADCAST_BEHAVIOR20         ()              const { return this->readRegister (140, 0);            }

  bool    ADDRESS_SENDER21_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(141, value & 0xff); }
  uint8_t ADDRESS_SENDER21_HIGH_BYTE   ()              const { return this->readRegister (141, 0);            }
  bool    ADDRESS_SENDER21_MID_BYTE    (uint8_t value) const { return this->writeRegister(142, value & 0xff); }
  uint8_t ADDRESS_SENDER21_MID_BYTE    ()              const { return this->readRegister (142, 0);            }
  bool    ADDRESS_SENDER21_LOW_BYTE    (uint8_t value) const { return this->writeRegister(143, value & 0xff); }
  uint8_t ADDRESS_SENDER21_LOW_BYTE    ()              const { return this->readRegister (143, 0);            }

  bool    ADDRESS_RECEIVER21_HIGH_BYTE (uint8_t value) const { return this->writeRegister(144, value & 0xff); }
  uint8_t ADDRESS_RECEIVER21_HIGH_BYTE ()              const { return this->readRegister (144, 0);            }
  bool    ADDRESS_RECEIVER21_MID_BYTE  (uint8_t value) const { return this->writeRegister(145, value & 0xff); }
  uint8_t ADDRESS_RECEIVER21_MID_BYTE  ()              const { return this->readRegister (145, 0);            }
  bool    ADDRESS_RECEIVER21_LOW_BYTE  (uint8_t value) const { return this->writeRegister(146, value & 0xff); }
  uint8_t ADDRESS_RECEIVER21_LOW_BYTE  ()              const { return this->readRegister (146, 0);            }

  bool    BROADCAST_BEHAVIOR21         (uint8_t value) const { return this->writeRegister(147, value & 0xff); }
  bool    BROADCAST_BEHAVIOR21         ()              const { return this->readRegister (147, 0);            }

  bool    ADDRESS_SENDER22_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(148, value & 0xff); }
  uint8_t ADDRESS_SENDER22_HIGH_BYTE   ()              const { return this->readRegister (148, 0);            }
  bool    ADDRESS_SENDER22_MID_BYTE    (uint8_t value) const { return this->writeRegister(149, value & 0xff); }
  uint8_t ADDRESS_SENDER22_MID_BYTE    ()              const { return this->readRegister (149, 0);            }
  bool    ADDRESS_SENDER22_LOW_BYTE    (uint8_t value) const { return this->writeRegister(150, value & 0xff); }
  uint8_t ADDRESS_SENDER22_LOW_BYTE    ()              const { return this->readRegister (150, 0);            }

  bool    ADDRESS_RECEIVER22_HIGH_BYTE (uint8_t value) const { return this->writeRegister(151, value & 0xff); }
  uint8_t ADDRESS_RECEIVER22_HIGH_BYTE ()              const { return this->readRegister (151, 0);            }
  bool    ADDRESS_RECEIVER22_MID_BYTE  (uint8_t value) const { return this->writeRegister(152, value & 0xff); }
  uint8_t ADDRESS_RECEIVER22_MID_BYTE  ()              const { return this->readRegister (152, 0);            }
  bool    ADDRESS_RECEIVER22_LOW_BYTE  (uint8_t value) const { return this->writeRegister(153, value & 0xff); }
  uint8_t ADDRESS_RECEIVER22_LOW_BYTE  ()              const { return this->readRegister (153, 0);            }

  bool    BROADCAST_BEHAVIOR22         (uint8_t value) const { return this->writeRegister(154, value & 0xff); }
  bool    BROADCAST_BEHAVIOR22         ()              const { return this->readRegister (154, 0);            }

  bool    ADDRESS_SENDER23_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(155, value & 0xff); }
  uint8_t ADDRESS_SENDER23_HIGH_BYTE   ()              const { return this->readRegister (155, 0);            }
  bool    ADDRESS_SENDER23_MID_BYTE    (uint8_t value) const { return this->writeRegister(156, value & 0xff); }
  uint8_t ADDRESS_SENDER23_MID_BYTE    ()              const { return this->readRegister (156, 0);            }
  bool    ADDRESS_SENDER23_LOW_BYTE    (uint8_t value) const { return this->writeRegister(157, value & 0xff); }
  uint8_t ADDRESS_SENDER23_LOW_BYTE    ()              const { return this->readRegister (157, 0);            }

  bool    ADDRESS_RECEIVER23_HIGH_BYTE (uint8_t value) const { return this->writeRegister(158, value & 0xff); }
  uint8_t ADDRESS_RECEIVER23_HIGH_BYTE ()              const { return this->readRegister (158, 0);            }
  bool    ADDRESS_RECEIVER23_MID_BYTE  (uint8_t value) const { return this->writeRegister(159, value & 0xff); }
  uint8_t ADDRESS_RECEIVER23_MID_BYTE  ()              const { return this->readRegister (159, 0);            }
  bool    ADDRESS_RECEIVER23_LOW_BYTE  (uint8_t value) const { return this->writeRegister(160, value & 0xff); }
  uint8_t ADDRESS_RECEIVER23_LOW_BYTE  ()              const { return this->readRegister (160, 0);            }

  bool    BROADCAST_BEHAVIOR23         (uint8_t value) const { return this->writeRegister(161, value & 0xff); }
  bool    BROADCAST_BEHAVIOR23         ()              const { return this->readRegister (161, 0);            }

  bool    ADDRESS_SENDER24_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(162, value & 0xff); }
  uint8_t ADDRESS_SENDER24_HIGH_BYTE   ()              const { return this->readRegister (162, 0);            }
  bool    ADDRESS_SENDER24_MID_BYTE    (uint8_t value) const { return this->writeRegister(163, value & 0xff); }
  uint8_t ADDRESS_SENDER24_MID_BYTE    ()              const { return this->readRegister (163, 0);            }
  bool    ADDRESS_SENDER24_LOW_BYTE    (uint8_t value) const { return this->writeRegister(164, value & 0xff); }
  uint8_t ADDRESS_SENDER24_LOW_BYTE    ()              const { return this->readRegister (164, 0);            }

  bool    ADDRESS_RECEIVER24_HIGH_BYTE (uint8_t value) const { return this->writeRegister(165, value & 0xff); }
  uint8_t ADDRESS_RECEIVER24_HIGH_BYTE ()              const { return this->readRegister (165, 0);            }
  bool    ADDRESS_RECEIVER24_MID_BYTE  (uint8_t value) const { return this->writeRegister(166, value & 0xff); }
  uint8_t ADDRESS_RECEIVER24_MID_BYTE  ()              const { return this->readRegister (166, 0);            }
  bool    ADDRESS_RECEIVER24_LOW_BYTE  (uint8_t value) const { return this->writeRegister(167, value & 0xff); }
  uint8_t ADDRESS_RECEIVER24_LOW_BYTE  ()              const { return this->readRegister (167, 0);            }

  bool    BROADCAST_BEHAVIOR24         (uint8_t value) const { return this->writeRegister(168, value & 0xff); }
  bool    BROADCAST_BEHAVIOR24         ()              const { return this->readRegister (168, 0);            }

  bool    ADDRESS_SENDER25_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(169, value & 0xff); }
  uint8_t ADDRESS_SENDER25_HIGH_BYTE   ()              const { return this->readRegister (169, 0);            }
  bool    ADDRESS_SENDER25_MID_BYTE    (uint8_t value) const { return this->writeRegister(170, value & 0xff); }
  uint8_t ADDRESS_SENDER25_MID_BYTE    ()              const { return this->readRegister (170, 0);            }
  bool    ADDRESS_SENDER25_LOW_BYTE    (uint8_t value) const { return this->writeRegister(171, value & 0xff); }
  uint8_t ADDRESS_SENDER25_LOW_BYTE    ()              const { return this->readRegister (171, 0);            }

  bool    ADDRESS_RECEIVER25_HIGH_BYTE (uint8_t value) const { return this->writeRegister(172, value & 0xff); }
  uint8_t ADDRESS_RECEIVER25_HIGH_BYTE ()              const { return this->readRegister (172, 0);            }
  bool    ADDRESS_RECEIVER25_MID_BYTE  (uint8_t value) const { return this->writeRegister(173, value & 0xff); }
  uint8_t ADDRESS_RECEIVER25_MID_BYTE  ()              const { return this->readRegister (173, 0);            }
  bool    ADDRESS_RECEIVER25_LOW_BYTE  (uint8_t value) const { return this->writeRegister(174, value & 0xff); }
  uint8_t ADDRESS_RECEIVER25_LOW_BYTE  ()              const { return this->readRegister (174, 0);            }

  bool    BROADCAST_BEHAVIOR25         (uint8_t value) const { return this->writeRegister(175, value & 0xff); }
  bool    BROADCAST_BEHAVIOR25         ()              const { return this->readRegister (175, 0);            }

  bool    ADDRESS_SENDER26_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(176, value & 0xff); }
  uint8_t ADDRESS_SENDER26_HIGH_BYTE   ()              const { return this->readRegister (176, 0);            }
  bool    ADDRESS_SENDER26_MID_BYTE    (uint8_t value) const { return this->writeRegister(177, value & 0xff); }
  uint8_t ADDRESS_SENDER26_MID_BYTE    ()              const { return this->readRegister (177, 0);            }
  bool    ADDRESS_SENDER26_LOW_BYTE    (uint8_t value) const { return this->writeRegister(178, value & 0xff); }
  uint8_t ADDRESS_SENDER26_LOW_BYTE    ()              const { return this->readRegister (178, 0);            }

  bool    ADDRESS_RECEIVER26_HIGH_BYTE (uint8_t value) const { return this->writeRegister(179, value & 0xff); }
  uint8_t ADDRESS_RECEIVER26_HIGH_BYTE ()              const { return this->readRegister (179, 0);            }
  bool    ADDRESS_RECEIVER26_MID_BYTE  (uint8_t value) const { return this->writeRegister(180, value & 0xff); }
  uint8_t ADDRESS_RECEIVER26_MID_BYTE  ()              const { return this->readRegister (180, 0);            }
  bool    ADDRESS_RECEIVER26_LOW_BYTE  (uint8_t value) const { return this->writeRegister(181, value & 0xff); }
  uint8_t ADDRESS_RECEIVER26_LOW_BYTE  ()              const { return this->readRegister (181, 0);            }

  bool    BROADCAST_BEHAVIOR26         (uint8_t value) const { return this->writeRegister(182, value & 0xff); }
  bool    BROADCAST_BEHAVIOR26         ()              const { return this->readRegister (182, 0);            }

  bool    ADDRESS_SENDER27_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(183, value & 0xff); }
  uint8_t ADDRESS_SENDER27_HIGH_BYTE   ()              const { return this->readRegister (183, 0);            }
  bool    ADDRESS_SENDER27_MID_BYTE    (uint8_t value) const { return this->writeRegister(184, value & 0xff); }
  uint8_t ADDRESS_SENDER27_MID_BYTE    ()              const { return this->readRegister (184, 0);            }
  bool    ADDRESS_SENDER27_LOW_BYTE    (uint8_t value) const { return this->writeRegister(185, value & 0xff); }
  uint8_t ADDRESS_SENDER27_LOW_BYTE    ()              const { return this->readRegister (185, 0);            }

  bool    ADDRESS_RECEIVER27_HIGH_BYTE (uint8_t value) const { return this->writeRegister(186, value & 0xff); }
  uint8_t ADDRESS_RECEIVER27_HIGH_BYTE ()              const { return this->readRegister (186, 0);            }
  bool    ADDRESS_RECEIVER27_MID_BYTE  (uint8_t value) const { return this->writeRegister(187, value & 0xff); }
  uint8_t ADDRESS_RECEIVER27_MID_BYTE  ()              const { return this->readRegister (187, 0);            }
  bool    ADDRESS_RECEIVER27_LOW_BYTE  (uint8_t value) const { return this->writeRegister(188, value & 0xff); }
  uint8_t ADDRESS_RECEIVER27_LOW_BYTE  ()              const { return this->readRegister (188, 0);            }

  bool    BROADCAST_BEHAVIOR27         (uint8_t value) const { return this->writeRegister(189, value & 0xff); }
  bool    BROADCAST_BEHAVIOR27         ()              const { return this->readRegister (189, 0);            }

  bool    ADDRESS_SENDER28_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(190, value & 0xff); }
  uint8_t ADDRESS_SENDER28_HIGH_BYTE   ()              const { return this->readRegister (190, 0);            }
  bool    ADDRESS_SENDER28_MID_BYTE    (uint8_t value) const { return this->writeRegister(191, value & 0xff); }
  uint8_t ADDRESS_SENDER28_MID_BYTE    ()              const { return this->readRegister (191, 0);            }
  bool    ADDRESS_SENDER28_LOW_BYTE    (uint8_t value) const { return this->writeRegister(192, value & 0xff); }
  uint8_t ADDRESS_SENDER28_LOW_BYTE    ()              const { return this->readRegister (192, 0);            }

  bool    ADDRESS_RECEIVER28_HIGH_BYTE (uint8_t value) const { return this->writeRegister(193, value & 0xff); }
  uint8_t ADDRESS_RECEIVER28_HIGH_BYTE ()              const { return this->readRegister (193, 0);            }
  bool    ADDRESS_RECEIVER28_MID_BYTE  (uint8_t value) const { return this->writeRegister(194, value & 0xff); }
  uint8_t ADDRESS_RECEIVER28_MID_BYTE  ()              const { return this->readRegister (194, 0);            }
  bool    ADDRESS_RECEIVER28_LOW_BYTE  (uint8_t value) const { return this->writeRegister(195, value & 0xff); }
  uint8_t ADDRESS_RECEIVER28_LOW_BYTE  ()              const { return this->readRegister (195, 0);            }

  bool    BROADCAST_BEHAVIOR28         (uint8_t value) const { return this->writeRegister(196, value & 0xff); }
  bool    BROADCAST_BEHAVIOR28         ()              const { return this->readRegister (196, 0);            }

  bool    ADDRESS_SENDER29_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(197, value & 0xff); }
  uint8_t ADDRESS_SENDER29_HIGH_BYTE   ()              const { return this->readRegister (197, 0);            }
  bool    ADDRESS_SENDER29_MID_BYTE    (uint8_t value) const { return this->writeRegister(198, value & 0xff); }
  uint8_t ADDRESS_SENDER29_MID_BYTE    ()              const { return this->readRegister (198, 0);            }
  bool    ADDRESS_SENDER29_LOW_BYTE    (uint8_t value) const { return this->writeRegister(199, value & 0xff); }
  uint8_t ADDRESS_SENDER29_LOW_BYTE    ()              const { return this->readRegister (199, 0);            }

  bool    ADDRESS_RECEIVER29_HIGH_BYTE (uint8_t value) const { return this->writeRegister(200 ,value & 0xff); }
  uint8_t ADDRESS_RECEIVER29_HIGH_BYTE ()              const { return this->readRegister (200 ,0);            }
  bool    ADDRESS_RECEIVER29_MID_BYTE  (uint8_t value) const { return this->writeRegister(201 ,value & 0xff); }
  uint8_t ADDRESS_RECEIVER29_MID_BYTE  ()              const { return this->readRegister (201 ,0);            }
  
  bool    ADDRESS_RECEIVER29_LOW_BYTE  (uint8_t value) const { return this->writeRegister(202 ,value & 0xff); }
  uint8_t ADDRESS_RECEIVER29_LOW_BYTE  ()              const { return this->readRegister (202 ,0);            }
  bool    BROADCAST_BEHAVIOR29         (uint8_t value) const { return this->writeRegister(203 ,value & 0xff); }
  bool    BROADCAST_BEHAVIOR29         ()              const { return this->readRegister (203 ,0);            }

  bool    ADDRESS_SENDER30_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(204 ,value & 0xff); }
  uint8_t ADDRESS_SENDER30_HIGH_BYTE   ()              const { return this->readRegister (204 ,0);            }
  bool    ADDRESS_SENDER30_MID_BYTE    (uint8_t value) const { return this->writeRegister(205 ,value & 0xff); }
  uint8_t ADDRESS_SENDER30_MID_BYTE    ()              const { return this->readRegister (205 ,0);            }
  bool    ADDRESS_SENDER30_LOW_BYTE    (uint8_t value) const { return this->writeRegister(206 ,value & 0xff); }
  uint8_t ADDRESS_SENDER30_LOW_BYTE    ()              const { return this->readRegister (206 ,0);            }

  bool    ADDRESS_RECEIVER30_HIGH_BYTE (uint8_t value) const { return this->writeRegister(207, value & 0xff); }
  uint8_t ADDRESS_RECEIVER30_HIGH_BYTE ()              const { return this->readRegister (207, 0);            }
  bool    ADDRESS_RECEIVER30_MID_BYTE  (uint8_t value) const { return this->writeRegister(208, value & 0xff); }
  uint8_t ADDRESS_RECEIVER30_MID_BYTE  ()              const { return this->readRegister (208, 0);            }
  bool    ADDRESS_RECEIVER30_LOW_BYTE  (uint8_t value) const { return this->writeRegister(209, value & 0xff); }
  uint8_t ADDRESS_RECEIVER30_LOW_BYTE  ()              const { return this->readRegister (209, 0);            }

  bool    BROADCAST_BEHAVIOR30         (uint8_t value) const { return this->writeRegister(210, value & 0xff); }
  bool    BROADCAST_BEHAVIOR30         ()              const { return this->readRegister (210, 0);            }

  bool    ADDRESS_SENDER31_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(211, value & 0xff); }
  uint8_t ADDRESS_SENDER31_HIGH_BYTE   ()              const { return this->readRegister (211, 0);            }
  bool    ADDRESS_SENDER31_MID_BYTE    (uint8_t value) const { return this->writeRegister(212, value & 0xff); }
  uint8_t ADDRESS_SENDER31_MID_BYTE    ()              const { return this->readRegister (212, 0);            }
  bool    ADDRESS_SENDER31_LOW_BYTE    (uint8_t value) const { return this->writeRegister(213, value & 0xff); }
  uint8_t ADDRESS_SENDER31_LOW_BYTE    ()              const { return this->readRegister (213, 0);            }

  bool    ADDRESS_RECEIVER31_HIGH_BYTE (uint8_t value) const { return this->writeRegister(214, value & 0xff); }
  uint8_t ADDRESS_RECEIVER31_HIGH_BYTE ()              const { return this->readRegister (214, 0);            }
  bool    ADDRESS_RECEIVER31_MID_BYTE  (uint8_t value) const { return this->writeRegister(215, value & 0xff); }
  uint8_t ADDRESS_RECEIVER31_MID_BYTE  ()              const { return this->readRegister (215, 0);            }
  bool    ADDRESS_RECEIVER31_LOW_BYTE  (uint8_t value) const { return this->writeRegister(216, value & 0xff); }
  uint8_t ADDRESS_RECEIVER31_LOW_BYTE  ()              const { return this->readRegister (216, 0);            }

  bool    BROADCAST_BEHAVIOR31         (uint8_t value) const { return this->writeRegister(217, value & 0xff); }
  bool    BROADCAST_BEHAVIOR31         ()              const { return this->readRegister (217, 0);            }

  bool    ADDRESS_SENDER32_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(218, value & 0xff); }
  uint8_t ADDRESS_SENDER32_HIGH_BYTE   ()              const { return this->readRegister (218, 0);            }
  bool    ADDRESS_SENDER32_MID_BYTE    (uint8_t value) const { return this->writeRegister(219, value & 0xff); }
  uint8_t ADDRESS_SENDER32_MID_BYTE    ()              const { return this->readRegister (219, 0);            }
  bool    ADDRESS_SENDER32_LOW_BYTE    (uint8_t value) const { return this->writeRegister(220, value & 0xff); }
  uint8_t ADDRESS_SENDER32_LOW_BYTE    ()              const { return this->readRegister (220, 0);            }

  bool    ADDRESS_RECEIVER32_HIGH_BYTE (uint8_t value) const { return this->writeRegister(221, value & 0xff); }
  uint8_t ADDRESS_RECEIVER32_HIGH_BYTE ()              const { return this->readRegister (221, 0);            }
  bool    ADDRESS_RECEIVER32_MID_BYTE  (uint8_t value) const { return this->writeRegister(222, value & 0xff); }
  uint8_t ADDRESS_RECEIVER32_MID_BYTE  ()              const { return this->readRegister (222, 0);            }
  bool    ADDRESS_RECEIVER32_LOW_BYTE  (uint8_t value) const { return this->writeRegister(223, value & 0xff); }
  uint8_t ADDRESS_RECEIVER32_LOW_BYTE  ()              const { return this->readRegister (223, 0);            }

  bool    BROADCAST_BEHAVIOR32         (uint8_t value) const { return this->writeRegister(224, value & 0xff); }
  bool    BROADCAST_BEHAVIOR32         ()              const { return this->readRegister (224, 0);            }

  bool    ADDRESS_SENDER33_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(225, value & 0xff); }
  uint8_t ADDRESS_SENDER33_HIGH_BYTE   ()              const { return this->readRegister (225, 0);            }
  bool    ADDRESS_SENDER33_MID_BYTE    (uint8_t value) const { return this->writeRegister(226, value & 0xff); }
  uint8_t ADDRESS_SENDER33_MID_BYTE    ()              const { return this->readRegister (226, 0);            }
  bool    ADDRESS_SENDER33_LOW_BYTE    (uint8_t value) const { return this->writeRegister(227, value & 0xff); }
  uint8_t ADDRESS_SENDER33_LOW_BYTE    ()              const { return this->readRegister (227, 0);            }

  bool    ADDRESS_RECEIVER33_HIGH_BYTE (uint8_t value) const { return this->writeRegister(228, value & 0xff); }
  uint8_t ADDRESS_RECEIVER33_HIGH_BYTE ()              const { return this->readRegister (228, 0);            }
  bool    ADDRESS_RECEIVER33_MID_BYTE  (uint8_t value) const { return this->writeRegister(229, value & 0xff); }
  uint8_t ADDRESS_RECEIVER33_MID_BYTE  ()              const { return this->readRegister (229, 0);            }
  bool    ADDRESS_RECEIVER33_LOW_BYTE  (uint8_t value) const { return this->writeRegister(230, value & 0xff); }
  uint8_t ADDRESS_RECEIVER33_LOW_BYTE  ()              const { return this->readRegister (230, 0);            }

  bool    BROADCAST_BEHAVIOR33         (uint8_t value) const { return this->writeRegister(231, value & 0xff); }
  bool    BROADCAST_BEHAVIOR33         ()              const { return this->readRegister (231, 0);            }

  bool    ADDRESS_SENDER34_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(232, value & 0xff); }
  uint8_t ADDRESS_SENDER34_HIGH_BYTE   ()              const { return this->readRegister (232, 0);            }
  bool    ADDRESS_SENDER34_MID_BYTE    (uint8_t value) const { return this->writeRegister(233, value & 0xff); }
  uint8_t ADDRESS_SENDER34_MID_BYTE    ()              const { return this->readRegister (233, 0);            }
  bool    ADDRESS_SENDER34_LOW_BYTE    (uint8_t value) const { return this->writeRegister(234, value & 0xff); }
  uint8_t ADDRESS_SENDER34_LOW_BYTE    ()              const { return this->readRegister (234, 0);            }

  bool    ADDRESS_RECEIVER34_HIGH_BYTE (uint8_t value) const { return this->writeRegister(235, value & 0xff); }
  uint8_t ADDRESS_RECEIVER34_HIGH_BYTE ()              const { return this->readRegister (235, 0);            }
  bool    ADDRESS_RECEIVER34_MID_BYTE  (uint8_t value) const { return this->writeRegister(236, value & 0xff); }
  uint8_t ADDRESS_RECEIVER34_MID_BYTE  ()              const { return this->readRegister (236, 0);            }
  bool    ADDRESS_RECEIVER34_LOW_BYTE  (uint8_t value) const { return this->writeRegister(237, value & 0xff); }
  uint8_t ADDRESS_RECEIVER34_LOW_BYTE  ()              const { return this->readRegister (237, 0);            }

  bool    BROADCAST_BEHAVIOR34         (uint8_t value) const { return this->writeRegister(238, value & 0xff); }
  bool    BROADCAST_BEHAVIOR34         ()              const { return this->readRegister (238, 0);            }
  
  bool    ADDRESS_SENDER35_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(239, value & 0xff); }
  uint8_t ADDRESS_SENDER35_HIGH_BYTE   ()              const { return this->readRegister (239, 0);            }
  bool    ADDRESS_SENDER35_MID_BYTE    (uint8_t value) const { return this->writeRegister(240, value & 0xff); }
  uint8_t ADDRESS_SENDER35_MID_BYTE    ()              const { return this->readRegister (240, 0);            }
  bool    ADDRESS_SENDER35_LOW_BYTE    (uint8_t value) const { return this->writeRegister(241, value & 0xff); }
  uint8_t ADDRESS_SENDER35_LOW_BYTE    ()              const { return this->readRegister (241, 0);            }

  bool    ADDRESS_RECEIVER35_HIGH_BYTE (uint8_t value) const { return this->writeRegister(242, value & 0xff); }
  uint8_t ADDRESS_RECEIVER35_HIGH_BYTE ()              const { return this->readRegister (242, 0);            }
  bool    ADDRESS_RECEIVER35_MID_BYTE  (uint8_t value) const { return this->writeRegister(243, value & 0xff); }
  uint8_t ADDRESS_RECEIVER35_MID_BYTE  ()              const { return this->readRegister (243, 0);            }
  bool    ADDRESS_RECEIVER35_LOW_BYTE  (uint8_t value) const { return this->writeRegister(244, value & 0xff); }
  uint8_t ADDRESS_RECEIVER35_LOW_BYTE  ()              const { return this->readRegister (244, 0);            }

  bool    BROADCAST_BEHAVIOR35         (uint8_t value) const { return this->writeRegister(245, value & 0xff); }
  bool    BROADCAST_BEHAVIOR35         ()              const { return this->readRegister (245, 0);            }

  bool    ADDRESS_SENDER36_HIGH_BYTE   (uint8_t value) const { return this->writeRegister(246, value & 0xff); }
  uint8_t ADDRESS_SENDER36_HIGH_BYTE   ()              const { return this->readRegister (246, 0);            }
  bool    ADDRESS_SENDER36_MID_BYTE    (uint8_t value) const { return this->writeRegister(247, value & 0xff); }
  uint8_t ADDRESS_SENDER36_MID_BYTE    ()              const { return this->readRegister (247, 0);            }
  bool    ADDRESS_SENDER36_LOW_BYTE    (uint8_t value) const { return this->writeRegister(248, value & 0xff); }
  uint8_t ADDRESS_SENDER36_LOW_BYTE    ()              const { return this->readRegister (248, 0);            }

  bool    ADDRESS_RECEIVER36_HIGH_BYTE (uint8_t value) const { return this->writeRegister(249, value & 0xff); }
  uint8_t ADDRESS_RECEIVER36_HIGH_BYTE ()              const { return this->readRegister (249, 0);            }
  bool    ADDRESS_RECEIVER36_MID_BYTE  (uint8_t value) const { return this->writeRegister(250, value & 0xff); }
  uint8_t ADDRESS_RECEIVER36_MID_BYTE  ()              const { return this->readRegister (250, 0);            }
  bool    ADDRESS_RECEIVER36_LOW_BYTE  (uint8_t value) const { return this->writeRegister(251, value & 0xff); }
  uint8_t ADDRESS_RECEIVER36_LOW_BYTE  ()              const { return this->readRegister (251, 0);            }

  bool    BROADCAST_BEHAVIOR36         (uint8_t value) const { return this->writeRegister(252, value & 0xff); }
  bool    BROADCAST_BEHAVIOR36         ()              const { return this->readRegister (252, 0);            }

  void defaults () {
    DPRINTLN("EEPROM CLEAR");
    clear();
  }
};


class RepeaterChannel : public Channel<Hal,List1,RepList2,EmptyList,DefList4,1,UList0> {
private:
  struct RepeaterPartner {
    HMID P1;
    HMID P2;
    bool BCAST;
  };
public:
  RepeaterPartner RepeaterPartnerDevices[36];

  typedef Channel<Hal,List1,RepList2,EmptyList,DefList4,1,UList0> BaseChannel;

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
      RepeaterPartnerDevices[i].P1.dump();
      DPRINT(" ");
      RepeaterPartnerDevices[i].P2.dump();
      DPRINT(" ");

      if (i % 2 == 1)
        DDECLN(RepeaterPartnerDevices[i].BCAST);
      else
        { DDEC(RepeaterPartnerDevices[i].BCAST); DPRINT(" | "); }
    }
  }

  bool configChanged() {
    RepeaterPartnerDevices[0]  = { { getList2().ADDRESS_SENDER1_HIGH_BYTE(), getList2().ADDRESS_SENDER1_MID_BYTE(), getList2().ADDRESS_SENDER1_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER1_HIGH_BYTE(), getList2().ADDRESS_RECEIVER1_MID_BYTE(), getList2().ADDRESS_RECEIVER1_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR1()};
    RepeaterPartnerDevices[1]  = { { getList2().ADDRESS_SENDER2_HIGH_BYTE(), getList2().ADDRESS_SENDER2_MID_BYTE(), getList2().ADDRESS_SENDER2_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER2_HIGH_BYTE(), getList2().ADDRESS_RECEIVER2_MID_BYTE(), getList2().ADDRESS_RECEIVER2_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR2()};
    RepeaterPartnerDevices[2]  = { { getList2().ADDRESS_SENDER3_HIGH_BYTE(), getList2().ADDRESS_SENDER3_MID_BYTE(), getList2().ADDRESS_SENDER3_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER3_HIGH_BYTE(), getList2().ADDRESS_RECEIVER3_MID_BYTE(), getList2().ADDRESS_RECEIVER3_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR3()};
    RepeaterPartnerDevices[3]  = { { getList2().ADDRESS_SENDER4_HIGH_BYTE(), getList2().ADDRESS_SENDER4_MID_BYTE(), getList2().ADDRESS_SENDER4_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER4_HIGH_BYTE(), getList2().ADDRESS_RECEIVER4_MID_BYTE(), getList2().ADDRESS_RECEIVER4_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR4()};
    RepeaterPartnerDevices[4]  = { { getList2().ADDRESS_SENDER5_HIGH_BYTE(), getList2().ADDRESS_SENDER5_MID_BYTE(), getList2().ADDRESS_SENDER5_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER5_HIGH_BYTE(), getList2().ADDRESS_RECEIVER5_MID_BYTE(), getList2().ADDRESS_RECEIVER5_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR5()};
    RepeaterPartnerDevices[5]  = { { getList2().ADDRESS_SENDER6_HIGH_BYTE(), getList2().ADDRESS_SENDER6_MID_BYTE(), getList2().ADDRESS_SENDER6_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER6_HIGH_BYTE(), getList2().ADDRESS_RECEIVER6_MID_BYTE(), getList2().ADDRESS_RECEIVER6_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR6()};
    RepeaterPartnerDevices[6]  = { { getList2().ADDRESS_SENDER7_HIGH_BYTE(), getList2().ADDRESS_SENDER7_MID_BYTE(), getList2().ADDRESS_SENDER7_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER7_HIGH_BYTE(), getList2().ADDRESS_RECEIVER7_MID_BYTE(), getList2().ADDRESS_RECEIVER7_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR7()};
    RepeaterPartnerDevices[7]  = { { getList2().ADDRESS_SENDER8_HIGH_BYTE(), getList2().ADDRESS_SENDER8_MID_BYTE(), getList2().ADDRESS_SENDER8_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER8_HIGH_BYTE(), getList2().ADDRESS_RECEIVER8_MID_BYTE(), getList2().ADDRESS_RECEIVER8_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR8()};
    RepeaterPartnerDevices[8]  = { { getList2().ADDRESS_SENDER9_HIGH_BYTE(), getList2().ADDRESS_SENDER9_MID_BYTE(), getList2().ADDRESS_SENDER9_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER9_HIGH_BYTE(), getList2().ADDRESS_RECEIVER9_MID_BYTE(), getList2().ADDRESS_RECEIVER9_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR9()};

    RepeaterPartnerDevices[9]  = { { getList2().ADDRESS_SENDER10_HIGH_BYTE(), getList2().ADDRESS_SENDER10_MID_BYTE(), getList2().ADDRESS_SENDER10_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER10_HIGH_BYTE(), getList2().ADDRESS_RECEIVER10_MID_BYTE(), getList2().ADDRESS_RECEIVER10_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR10()};
    RepeaterPartnerDevices[10] = { { getList2().ADDRESS_SENDER11_HIGH_BYTE(), getList2().ADDRESS_SENDER11_MID_BYTE(), getList2().ADDRESS_SENDER11_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER11_HIGH_BYTE(), getList2().ADDRESS_RECEIVER11_MID_BYTE(), getList2().ADDRESS_RECEIVER11_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR11()};
    RepeaterPartnerDevices[11] = { { getList2().ADDRESS_SENDER12_HIGH_BYTE(), getList2().ADDRESS_SENDER12_MID_BYTE(), getList2().ADDRESS_SENDER12_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER12_HIGH_BYTE(), getList2().ADDRESS_RECEIVER12_MID_BYTE(), getList2().ADDRESS_RECEIVER12_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR12()};
    RepeaterPartnerDevices[12] = { { getList2().ADDRESS_SENDER13_HIGH_BYTE(), getList2().ADDRESS_SENDER13_MID_BYTE(), getList2().ADDRESS_SENDER13_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER13_HIGH_BYTE(), getList2().ADDRESS_RECEIVER13_MID_BYTE(), getList2().ADDRESS_RECEIVER13_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR13()};
    RepeaterPartnerDevices[13] = { { getList2().ADDRESS_SENDER14_HIGH_BYTE(), getList2().ADDRESS_SENDER14_MID_BYTE(), getList2().ADDRESS_SENDER14_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER14_HIGH_BYTE(), getList2().ADDRESS_RECEIVER14_MID_BYTE(), getList2().ADDRESS_RECEIVER14_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR14()};
    RepeaterPartnerDevices[14] = { { getList2().ADDRESS_SENDER15_HIGH_BYTE(), getList2().ADDRESS_SENDER15_MID_BYTE(), getList2().ADDRESS_SENDER15_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER15_HIGH_BYTE(), getList2().ADDRESS_RECEIVER15_MID_BYTE(), getList2().ADDRESS_RECEIVER15_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR15()};
    RepeaterPartnerDevices[15] = { { getList2().ADDRESS_SENDER16_HIGH_BYTE(), getList2().ADDRESS_SENDER16_MID_BYTE(), getList2().ADDRESS_SENDER16_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER16_HIGH_BYTE(), getList2().ADDRESS_RECEIVER16_MID_BYTE(), getList2().ADDRESS_RECEIVER16_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR16()};
    RepeaterPartnerDevices[16] = { { getList2().ADDRESS_SENDER17_HIGH_BYTE(), getList2().ADDRESS_SENDER17_MID_BYTE(), getList2().ADDRESS_SENDER17_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER17_HIGH_BYTE(), getList2().ADDRESS_RECEIVER17_MID_BYTE(), getList2().ADDRESS_RECEIVER17_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR17()};
    RepeaterPartnerDevices[17] = { { getList2().ADDRESS_SENDER18_HIGH_BYTE(), getList2().ADDRESS_SENDER18_MID_BYTE(), getList2().ADDRESS_SENDER18_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER18_HIGH_BYTE(), getList2().ADDRESS_RECEIVER18_MID_BYTE(), getList2().ADDRESS_RECEIVER18_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR18()};
    RepeaterPartnerDevices[18] = { { getList2().ADDRESS_SENDER19_HIGH_BYTE(), getList2().ADDRESS_SENDER19_MID_BYTE(), getList2().ADDRESS_SENDER19_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER19_HIGH_BYTE(), getList2().ADDRESS_RECEIVER19_MID_BYTE(), getList2().ADDRESS_RECEIVER19_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR19()};

    RepeaterPartnerDevices[19] = { { getList2().ADDRESS_SENDER20_HIGH_BYTE(), getList2().ADDRESS_SENDER20_MID_BYTE(), getList2().ADDRESS_SENDER20_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER20_HIGH_BYTE(), getList2().ADDRESS_RECEIVER20_MID_BYTE(), getList2().ADDRESS_RECEIVER20_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR20()};
    RepeaterPartnerDevices[20] = { { getList2().ADDRESS_SENDER21_HIGH_BYTE(), getList2().ADDRESS_SENDER21_MID_BYTE(), getList2().ADDRESS_SENDER21_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER21_HIGH_BYTE(), getList2().ADDRESS_RECEIVER21_MID_BYTE(), getList2().ADDRESS_RECEIVER21_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR21()};
    RepeaterPartnerDevices[21] = { { getList2().ADDRESS_SENDER22_HIGH_BYTE(), getList2().ADDRESS_SENDER22_MID_BYTE(), getList2().ADDRESS_SENDER22_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER22_HIGH_BYTE(), getList2().ADDRESS_RECEIVER22_MID_BYTE(), getList2().ADDRESS_RECEIVER22_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR22()};
    RepeaterPartnerDevices[22] = { { getList2().ADDRESS_SENDER23_HIGH_BYTE(), getList2().ADDRESS_SENDER23_MID_BYTE(), getList2().ADDRESS_SENDER23_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER23_HIGH_BYTE(), getList2().ADDRESS_RECEIVER23_MID_BYTE(), getList2().ADDRESS_RECEIVER23_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR23()};
    RepeaterPartnerDevices[23] = { { getList2().ADDRESS_SENDER24_HIGH_BYTE(), getList2().ADDRESS_SENDER24_MID_BYTE(), getList2().ADDRESS_SENDER24_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER24_HIGH_BYTE(), getList2().ADDRESS_RECEIVER24_MID_BYTE(), getList2().ADDRESS_RECEIVER24_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR24()};
    RepeaterPartnerDevices[24] = { { getList2().ADDRESS_SENDER25_HIGH_BYTE(), getList2().ADDRESS_SENDER25_MID_BYTE(), getList2().ADDRESS_SENDER25_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER25_HIGH_BYTE(), getList2().ADDRESS_RECEIVER25_MID_BYTE(), getList2().ADDRESS_RECEIVER25_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR25()};
    RepeaterPartnerDevices[25] = { { getList2().ADDRESS_SENDER26_HIGH_BYTE(), getList2().ADDRESS_SENDER26_MID_BYTE(), getList2().ADDRESS_SENDER26_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER26_HIGH_BYTE(), getList2().ADDRESS_RECEIVER26_MID_BYTE(), getList2().ADDRESS_RECEIVER26_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR26()};
    RepeaterPartnerDevices[26] = { { getList2().ADDRESS_SENDER27_HIGH_BYTE(), getList2().ADDRESS_SENDER27_MID_BYTE(), getList2().ADDRESS_SENDER27_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER27_HIGH_BYTE(), getList2().ADDRESS_RECEIVER27_MID_BYTE(), getList2().ADDRESS_RECEIVER27_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR27()};
    RepeaterPartnerDevices[27] = { { getList2().ADDRESS_SENDER28_HIGH_BYTE(), getList2().ADDRESS_SENDER28_MID_BYTE(), getList2().ADDRESS_SENDER28_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER28_HIGH_BYTE(), getList2().ADDRESS_RECEIVER28_MID_BYTE(), getList2().ADDRESS_RECEIVER28_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR28()};
    RepeaterPartnerDevices[28] = { { getList2().ADDRESS_SENDER29_HIGH_BYTE(), getList2().ADDRESS_SENDER29_MID_BYTE(), getList2().ADDRESS_SENDER29_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER29_HIGH_BYTE(), getList2().ADDRESS_RECEIVER29_MID_BYTE(), getList2().ADDRESS_RECEIVER29_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR29()};

    RepeaterPartnerDevices[29] = { { getList2().ADDRESS_SENDER30_HIGH_BYTE(), getList2().ADDRESS_SENDER30_MID_BYTE(), getList2().ADDRESS_SENDER30_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER30_HIGH_BYTE(), getList2().ADDRESS_RECEIVER30_MID_BYTE(), getList2().ADDRESS_RECEIVER30_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR30()};
    RepeaterPartnerDevices[30] = { { getList2().ADDRESS_SENDER31_HIGH_BYTE(), getList2().ADDRESS_SENDER31_MID_BYTE(), getList2().ADDRESS_SENDER31_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER31_HIGH_BYTE(), getList2().ADDRESS_RECEIVER31_MID_BYTE(), getList2().ADDRESS_RECEIVER31_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR31()};
    RepeaterPartnerDevices[31] = { { getList2().ADDRESS_SENDER32_HIGH_BYTE(), getList2().ADDRESS_SENDER32_MID_BYTE(), getList2().ADDRESS_SENDER32_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER32_HIGH_BYTE(), getList2().ADDRESS_RECEIVER32_MID_BYTE(), getList2().ADDRESS_RECEIVER32_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR32()};
    RepeaterPartnerDevices[32] = { { getList2().ADDRESS_SENDER33_HIGH_BYTE(), getList2().ADDRESS_SENDER33_MID_BYTE(), getList2().ADDRESS_SENDER33_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER33_HIGH_BYTE(), getList2().ADDRESS_RECEIVER33_MID_BYTE(), getList2().ADDRESS_RECEIVER33_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR33()};
    RepeaterPartnerDevices[33] = { { getList2().ADDRESS_SENDER34_HIGH_BYTE(), getList2().ADDRESS_SENDER34_MID_BYTE(), getList2().ADDRESS_SENDER34_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER34_HIGH_BYTE(), getList2().ADDRESS_RECEIVER34_MID_BYTE(), getList2().ADDRESS_RECEIVER34_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR34()};
    RepeaterPartnerDevices[34] = { { getList2().ADDRESS_SENDER35_HIGH_BYTE(), getList2().ADDRESS_SENDER35_MID_BYTE(), getList2().ADDRESS_SENDER35_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER35_HIGH_BYTE(), getList2().ADDRESS_RECEIVER35_MID_BYTE(), getList2().ADDRESS_RECEIVER35_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR35()};
    RepeaterPartnerDevices[35] = { { getList2().ADDRESS_SENDER36_HIGH_BYTE(), getList2().ADDRESS_SENDER36_MID_BYTE(), getList2().ADDRESS_SENDER36_LOW_BYTE() }, { getList2().ADDRESS_RECEIVER36_HIGH_BYTE(), getList2().ADDRESS_RECEIVER36_MID_BYTE(), getList2().ADDRESS_RECEIVER36_LOW_BYTE() }, this->getList2().BROADCAST_BEHAVIOR36()};

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

    void broadcastRptEvent (Message& msg, const HMID& sender) {
      msg.clearAck();
      msg.burstRequired(false);
      msg.setBroadcast();
      msg.setRepeated();
      msg.unsetRpten();
      msg.from(sender);
      msg.to(HMID::broadcast);
      send(msg);
    }

    void resendBidiMsg(Message& msg, const HMID& sender, const HMID& receiver, bool ack) {
      if (ack) {
        msg.ack().init(0);
        kstore.addAuth(msg);
      }
      msg.setRepeated();
      msg.unsetRpten();
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
        bool found = false;
        bool bcast = false;

        //look in RepeaterPartnerDevices, if Message is from/for any of them
        for (uint8_t i = 0; i < 36; i++) {
          if (RepChannel().RepeaterPartnerDevices[i].P1 == msg.from() || RepChannel().RepeaterPartnerDevices[i].P1 == msg.to()) {
            found = true;
            bcast = RepChannel().RepeaterPartnerDevices[i].BCAST;
            break;
          }
        }

        //a device was found in RepeaterPartnerDevices
        if (found) {
          _delay_ms(5);
          if (msg.flags() & Message::BCAST) {
            if (bcast) {
              DPRINT(F("Repeating BCAST Message: "));
              broadcastRptEvent(msg, msg.from());
            } else return ChannelDevice::process(msg);
          } else {
            if (msg.ackRequired()) {
              DPRINT(F("Repeating  ACK Message: "));
              resendBidiMsg(msg, msg.from(), msg.to(), true);
            } else {
              DPRINT(F("Repeating BIDI Message: "));
              resendBidiMsg(msg, msg.from(), msg.to(), false);
            }
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
