#include <AskSinMain.h>

#ifndef _REGISTER_h
	#define _REGISTER_h

	#define FRAME_TYPE           0x70										// Frame type 0x70 = WEATHER_EVENT
	#define DEVICE_INFO          0x01, 0x01, 0x00							// Device Info, 3 byte, describes device, not completely clear yet. includes amount of channels

	#if USE_ADRESS_SECTION == 1
		uint8_t devParam[] = {
			FIRMWARE_VERSION,
			0xFF, 0xFF,														// space for device type, assigned later
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,		// space for device serial, assigned later
			FRAME_TYPE,
			DEVICE_INFO,
			0xFF, 0xFF, 0xFF												// space for device address, assigned later
		};
	#else
		uint8_t devParam[] = {
			FIRMWARE_VERSION,
			DEVICE_TYPE,
			DEVICE_SERIAL,
			FRAME_TYPE,
			DEVICE_INFO,
			DEVICE_ADDRESS
		};
	#endif

	HM::s_devParm dParm = {
		3,																	// send retries, 1 byte, how often a string should be send out until we get an answer
		700,																// send timeout, 2 byte, time out for ACK handling
		devParam															// pointer to devParam, see above
	};

	//- channel device list table --------------------------------------------------------------------------------------------
	HM::s_modtable modTbl[] = {
		{ 0, 0, (s_mod_dlgt) NULL },										// ???
		{ 0, 0, (s_mod_dlgt) NULL },										// ???
	}; // 16 byte

	/**
	 * channel slice definition
	 * Each value represents a register index defined in s_regDevL0 below
	 */
	uint8_t sliceStr[] = {
		0x01, 0x05, 0x0A, 0x0B, 0x0C, 0x12, 0x14, 0x24, 0x25
	};

	/**
	 * Register definition for List 0
	 */
	struct s_regDevL0 {
		// 0x01, 0x05, 0x0A, 0x0B, 0x0C, 0x12, 0x14, 0x24, 0x25
		uint8_t burstRx;         // 0x01,             startBit:0, bits:8
		uint8_t             :6;  // 0x05              startBit:0, bits:6
		uint8_t ledMode     :2;  // 0x05,             startBit:6, bits:2
		uint8_t pairCentral[3];  // 0x0A, 0x0B, 0x0C, startBit:0, bits:8 (3 mal)
		uint8_t lowBatLimit;     // 0x12,             startBit:0, bits:8
		uint8_t transmDevTryMax; // 0x14,             startBit:0, bits:8
		uint8_t altitude[2];     // 0x24, 0x25        startBit:0, bits:8
	};

	// todo
	struct s_regChanL4 {
		// 0x01,
		uint8_t  peerNeedsBurst:1; // 0x01, s:0, e:1
		uint8_t                :7; //
	};

	struct s_regDev {
		s_regDevL0 l0;
	};

	struct s_regChan {
		s_regChanL4 l4;
	};

	struct s_regs {
		s_regDev ch0;
		s_regChan ch1;
	} regs; // 11 byte

	/**
	 * channel device list table, 22 bytes
	 */
	const s_cnlDefType cnlDefType[] PROGMEM = {
		// cnl, lst, peersMax, sIdx, sLen, pAddr,  pPeer,  *pRegs (pointer to regs structure)
		 { 0,   0,   0,        0x00, 9,    0x0000, 0x0000, (void*)&regs.ch0.l0},	// List 0
		 { 1,   4,   6,        0x05, 1,    0x0005, 0x0000, (void*)&regs.ch1.l4},	// List 4
	};

	// handover to AskSin lib, 6 bytes
	HM::s_devDef dDef = {
		1, 2, sliceStr, cnlDefType,
	};

	/**
	 * EEprom definition, 16 bytes
	 * Define start address and size in eeprom for magicNumber, peerDB, regsDB, userSpace
	 */
	HM::s_eeprom ee[] = {
		//magicNum, peerDB, regsDB, userSpace
		{   0x0000, 0x0002, 0x001a, 0x0025, },	// start address
		{   0x0002, 0x0030, 0x000b, 0x0000, },	// length
	};

	/**
	 * Definitions for EEprom defaults.
	 * Must enter in same order as e.g. defined in s_regDevL0
	 */
	const uint8_t regs00[] PROGMEM = {0x00, 0x64, 0x00, 0x00, 0x00, 0x10, 0x03, 0x00, 0x00};
	const uint8_t regs04[] PROGMEM = {0x1f, 0xa6, 0x5c, 0x05};

	s_defaultRegsTbl defaultRegsTbl[] = {
		// peer(0) or regs(1), channel, list, peer index, len, pointer to payload
		{ 1,                   0,       0,    0,          9,   regs00 },
	};

	HM::s_dtRegs dtRegs = {
		//amount of lines in defaultRegsTbl[], pointer to defaultRegsTbl[]
		1,                                     defaultRegsTbl
	};

#endif
