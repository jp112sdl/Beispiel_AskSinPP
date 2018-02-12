//- -----------------------------------------------------------------------------------------------------------------------
// AskSin driver implementation
// 2013-08-03 <horst@diebittners.de> Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
//- AskSin protocol functions ---------------------------------------------------------------------------------------------
//- with a lot of support from martin876 at FHEM forum
//- -----------------------------------------------------------------------------------------------------------------------
#include "AskSinMain.h"

//- some macros and definitions -------------------------------------------------------------------------------------------
#define _pgmB pgm_read_byte														// short hand for PROGMEM read
#define _pgmW pgm_read_word
#define maxPayload 16															// defines the length of send_conf_poll and send_peer_poll
//- -----------------------------------------------------------------------------------------------------------------------

//- -----------------------------------------------------------------------------------------------------------------------
//- Homematic communication functions -------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
s_intGDO0 intGDO0;																// structure to store interrupt number and pointer to active AskSin class instance
uint8_t bCast[] = {0,0,0,0};													// broad cast address

//  public://--------------------------------------------------------------------------------------------------------------
//- homematic public protocol functions
void     HM::init(void) {
	// register handling setup
	prepEEprom();																// check the eeprom for first time boot, prepares the eeprom and loads the defaults
	loadRegs();

	// communication setup
	cc.init();																	// init the TRX868 module
	intGDO0.nbr = cc.gdo0Int;													// make the gdo0 interrupt public
	attachInterrupt(intGDO0.nbr,isrGDO0,FALLING);								// attach the interrupt

	#if USE_ADRESS_SECTION == 1
		memcpy(hmId, &dParm.p[17], 3);											// initialize hmId
	#else
		memcpy(hmId, &dParm.p[17], 3);										// initialize hmId
	#endif

	hm.stayAwake(1000);
}

void     HM::poll(void) {														// task scheduler

	if (int0_flag > 0)    cc1101Recv_poll();
	if (recv.data[0] > 0) recv_poll();											// trace the received string and decide what to do further
	if (send.counter > 0) send_poll();											// something to send in the buffer?
	if (conf.act > 0)     send_conf_poll();										// some config to be send out
	if (pevt.act > 0)     send_peer_poll();										// send peer events

	power_poll();
	module_poll();																// poll the registered channel modules
	statusLed.poll();															// poll the status leds
	battery.poll();																// poll the battery check

	if(resetWdt_flag > 0 && (millis() - wdtResetTimer >= TIMEOUT_WDT_RESET)) {
		resetWdt_flag = 0;
		wdt_enable(WDTO_250MS);
		while(1);																// wait for Watchdog to generate reset
	}
}

void     HM::cc1101Recv_poll(void) {
	detachInterrupt(intGDO0.nbr);												// switch interrupt off

	if (hm.cc.receiveData(hm.recv.data)) {										// is something in the receive string
		hm.hm_dec(hm.recv.data);												// decode the content
	}

	int0_flag = 0;
	attachInterrupt(intGDO0.nbr,isrGDO0,FALLING);								// switch interrupt on again
}

void     HM::send_out(void) {
	if (bitRead(send.data[2],5) && memcmp(&send.data[7], bCast, 3) != 0) {
		send.retries = dParm.maxRetr;											// check for ACK request and not broadcast, then set max retries counter
	} else {
		send.retries = 1;														// otherwise send only one time
	}

	send.burst = bitRead(send.data[2],4);										// burst necessary?

	if (memcmp(&send.data[7], hmId, 3) == 0) {									// if the message is addressed to us,
		memcpy(recv.data,send.data,send.data[0]+1);								// then copy in receive buffer. could be the case while sending from serial console
		send.counter = 0;														// no need to fire

	} else {																	// it's not for us, so encode and put in send queue
		send.counter = 1;														// and fire
		send.timer = 0;
	}
}

/**
 * Perform a device reset.
 * At reset, all eeprom data was cleared.
 */
void     HM::reset(void) {
	statusLed.set(STATUSLED_2, STATUSLED_MODE_BLINKSFAST, 5);					// blink LED2 5 times short

	setEeWo(ee[0].magicNr,0);													// clear magic byte in eeprom and step in initRegisters
	prepEEprom();																// check the eeprom for first time boot, prepares the eeprom and loads the defaults
	loadRegs();
}

/**
 * Perform a device watchdog reset so we can jump to bootloader
 */
void     HM::resetWdt(void) {
	wdtResetTimer = millis();
	resetWdt_flag = 1;
}

/**
 * Set the power mode of the device
 *
 * There are 3 power modes for the TRX868 module
 * TX mode will switched on while something is in the send queue:
 *
 * 0 - RX mode enabled by default, take approx 17ma
 *
 * 1 - RX is in burst mode, RX will be switched on every 250ms to check if there is a carrier signal
 *     if yes - RX will stay enabled until timeout is reached, prolongation of timeout via receive function
 *     seems not necessary to be able to receive an ACK, RX mode should be switched on by send function
 *     if no  - RX will go in idle mode and wait for the next carrier sense check
 *
 * 2 - RX is off by default, TX mode is enabled while sending something
 *     configuration mode is required in this setup to be able to receive at least pairing and config request strings
 *     should be realized by a 30 sec timeout function for RX mode
 *
 * as output we need a status indication if TRX868 module is in receive, send or idle mode
 * idle mode is then the indication for power down mode of AVR
 */
void     HM::setPowerMode(uint8_t mode) {

	if ((mode == POWER_MODE_BURST) || (mode == POWER_MODE_SLEEP_WDT)) {
		//MCUSR &= ~(1 << WDRF);												// clear the reset flag
		WDTCSR |= (1 << WDCE) | (1 << WDE);										// set control register to change and enable the watch dog
		WDTCSR = 1 << WDP2;														// 250 ms

		//WDTCSR = 1 << WDP1 | 1 << WDP2;										// 1000 ms
		//WDTCSR = 1 << WDP0 | 1 << WDP1 | 1 << WDP2;							// 2000 ms
		//WDTCSR = 1 << WDP0 | 1 << WDP3;										// 8000 ms
		//powr.wdTme = 8190;													// store the watch dog time for adding in the poll function

		powr.wdTme = 250;														// store the watch dog time for adding in the poll function
	}

	if (mode == POWER_MODE_ON) {												// no power savings, RX is in receiving mode
		set_sleep_mode(SLEEP_MODE_IDLE);										// normal power saving

	} else if (mode == POWER_MODE_BURST) {										// some power savings, RX is in burst mode
		stayAwake(250);															// check in 250ms for a burst signal

	} else if ((mode == POWER_MODE_SLEEP_WDT) || (mode == POWER_MODE_SLEEP_DEEP)) {	// most power savings, RX is off beside a special function where RX stay in receive for 30 sec
		stayAwake(4000);															// after power on reset we stay 4 seconds awake to finish boot time
	}

	if (mode > POWER_MODE_ON) {
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);									// max power saving
	}

	powr.mode = mode;															// set power mode

	Serial << F("powerMode: ") << powr.mode << F("\n");

	powr.state = 1;																// after init of the TRX module it is in RX mode
	//Serial << F("pwr.mode:") << powr.mode << F("\n");
}

void     HM::stayAwake(uint32_t millisAwake) {
	if (powr.state == 0) cc.detectBurst(); 										// if TRX is in sleep, switch it on
	powr.state = 1;																// remember TRX state

	if (((millis() - powr.startMillis) < millisAwake) && (millisAwake < powr.nxtTO)) {
		return;																	// set the new timeout only if necessary
	}

	powr.nxtTO = millisAwake;													// stay awake for some time by setting next check time
	powr.startMillis = millis();
}

void     HM::setLedMode(uint8_t ledMode) {
	HM::ledMode = ledMode;
}

/**
 * Some functions for checking the config, preparing eeprom and load defaults
 * to eeprom or in regs structure
 */
void     HM::printConfig(void) {
	uint8_t peer[4];
	s_cnlDefType t;
	
	// print content of devDef incl slice string table
	Serial << F("\ncontent of dDef for ") << dDef.lstNbr << F(" elements\n");
	for (uint8_t i = 0; i < dDef.lstNbr; i++) {

		memcpy_P((void*)&t, &dDef.chPtr[i], sizeof(s_cnlDefType));
		if ((t.lst == 3) || (t.lst == 4)) getCnlListByPeerIdx(t.cnl, 0);		// load list3/4
		
		Serial << F("cnl:") << t.cnl  << F(", lst:") << t.lst <<  F(", pMax:") << t.pMax <<  F("\n");
		Serial << F("idx:") << t.sIdx << F(", len:") << t.sLen << F(", addr:") << t.pAddr << F("\n");
		
		Serial << F("regs: "); pHex(dDef.slPtr + t.sIdx, t.sLen, SERIAL_DBG_PHEX_MODE_LF);
		Serial << F("data: "); pHex((uint8_t*)t.pRegs, t.sLen, SERIAL_DBG_PHEX_MODE_NONE);
		Serial << F(" ");

		if (t.pMax == 0) {
			Serial << F("\n\n");
			continue;
		}
		
		for (uint8_t j = 1; j < t.pMax; j++) {
			getCnlListByPeerIdx(t.cnl, j);										// load list3/4
			Serial; pHex((uint8_t*)t.pRegs, t.sLen, SERIAL_DBG_PHEX_MODE_NONE);
			Serial<< F(" ");
		}

		Serial << F("\n");
		for (uint8_t j = 0; j < t.pMax; j++) {
			getPeerByIdx(t.cnl, j, peer);
			pHex(peer, 4, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F("   ");
		}
		Serial << F("\n\n");
	}

	Serial << F("\n");
}

void     HM::prepEEprom(void) {
	uint16_t crc = 0;															// define variable for storing crc
	uint8_t  *p = (uint8_t*)dDef.chPtr;											// cast devDef to char
	
	for (uint8_t i = 0; i < dDef.lstNbr; i++) {									// step through all lines of chDefType table
		for (uint8_t j = 0; j < 9; j++) {										// step through the first 9 bytes
			crc = crc16(crc, _pgmB(&p[(i*11)+j]));								// calculate the 16bit checksum for the table
		}
	}
	
	#ifdef RPDB_DBG																// some debug message
		Serial << F("prepEEprom t_crc: ") << crc << F(", e_crc: ") << getEeWo(ee[0].magicNr) << F("\n");
	#endif

	if (crc != getEeWo(ee[0].magicNr)) {										// compare against eeprom's magic number
		#ifdef RPDB_DBG
			Serial << F("prep eeprom\n");
			Serial << F("format eeprom for:\n");
			Serial << F("peerDB, addr:") << ee[0].peerDB << F(", len:") << ee[1].peerDB << F("\n");
			Serial << F("regsDB, addr:") << ee[0].regsDB << F(", len:") << ee[1].regsDB << F("\n");
		#endif
		
		clrEeBl(ee[0].peerDB, ee[1].peerDB);									// format the eeprom area (peerDB)
		clrEeBl(ee[0].regsDB, ee[1].regsDB);									// format the eeprom area (regsDB)
		loadDefaults();															// do we have some default settings
	}

	setEeWo(ee[0].magicNr,crc);													// write magic number to it's position
}

void     HM::loadDefaults(void) {
	if (dtRegs.nbr) {
		#ifdef RPDB_DBG
			Serial << F("set defaults:\n");
		#endif
		
		for (uint8_t i = 0; i < dtRegs.nbr; i++) {								// step through the table
			s_defaultRegsTbl *t = &dtRegs.ptr[i];								// pointer for better handling
			
			// check if we search for regs or peer
			uint16_t eeAddr = 0;
			if (t->prIn == 0) {													// we are going for peer
				eeAddr = cdPeerAddrByCnlIdx(t->cnl ,t->pIdx);					// get the eeprom address

			} else if (t->prIn == 1) {											// we are going for regs
				eeAddr = cdListAddrByCnlLst(t->cnl, t->lst ,t->pIdx);			// get the eeprom address
			}
			
			// find the respective address and write to eeprom
			for (uint8_t j = 0; j < t->len; j++) {								// step through the bytes
				setEeBy(eeAddr++,_pgmB(&t->b[j]));								// read from PROGMEM and write to eeprom
			}
			
			#ifdef RPDB_DBG														// some debug message
				Serial << F("cnl:") << t->cnl << F(", lst:") << t->lst << F(", idx:") << t->pIdx << ", addr:" << eeAddr << ", data: "; pHexPGM(t->b, t->len, SERIAL_DBG_PHEX_MODE_LF);
			#endif
		}

		_delay_ms(50);															// give eeprom some time
	}
}

void     HM::loadRegs(void) {
	// default regs filled regarding the cnlDefTable
	for (uint8_t i = 0; i < dDef.lstNbr; i++) {									// step through the list
		const s_cnlDefType *t = &dDef.chPtr[i];									// pointer for easier handling
		
		getEeBl(ee[0].regsDB + _pgmW(&t->pAddr), _pgmB(&t->sLen), (void*)_pgmW(&t->pRegs)); // load content from eeprom to user structure
	}

	// fill the master id
	uint8_t ret = getRegAddr(0,0,0,0x0a,3,dParm.MAID);							// get regs for 0x0a
	//Serial << F("MAID:"); pHex(dParm.MAID,3, SERIAL_DBG_PHEX_MODE_LF)
}

void     HM::regCnlModule(uint8_t cnl, s_mod_dlgt Delegate, uint16_t *mainList, uint16_t *peerList) {
	modTbl[cnl].mDlgt = Delegate;
	modTbl[cnl].use = 1;
	
	// register the call back address for the list pointers
	for (uint8_t i = 0; i < dDef.lstNbr; i++) {									// step through the list
		const s_cnlDefType *t = &dDef.chPtr[i];									// pointer for easier handling
		if (_pgmB(&t->cnl) != cnl) continue;

		if (_pgmB(&t->lst) >= 3) *peerList = _pgmW(&t->pRegs);
		if (_pgmB(&t->lst) <= 1) *mainList = _pgmW(&t->pRegs);
	}

}

uint32_t HM::getHMID(void) {
	uint8_t a[3];
	a[0] = hmId[2];
	a[1] = hmId[1];
	a[2] = hmId[0];
	a[3] = 0;

	return *(uint32_t*)&a;
}

uint8_t  HM::getMsgCnt(void) {
	return send.mCnt - 1;
}

//- -----------------------------------------------------------------------------------------------------------------------
//- external functions for pairing and communicating with the module
//- -----------------------------------------------------------------------------------------------------------------------

/**
 * Send a pairing request to master
 *
 *                                01 02    03                            04 05 06 07
 *  1A 00 A2 00 3F A6 5C 00 00 00 10 80 02 50 53 30 30 30 30 30 30 30 31 9F 04 01 01
 */
void     HM::startPairing(void) {
	statusLed.set(STATUSLED_2, STATUSLED_MODE_BLINKSLOW);						// led blink in config mode

	#if USE_ADRESS_SECTION == 1
		memcpy(send_payLoad, dParm.p, 17);										// copy details out of register.h
	#else
		memcpy(send_payLoad, dParm.p, 17);									// copy details out of register.h
	#endif

	send_prep(send.mCnt++,0xA2,0x00,dParm.MAID ,send_payLoad,17);
}

void     HM::sendInfoActuatorStatus(uint8_t cnl, uint8_t status, uint8_t flag) {
	if (memcmp(dParm.MAID,bCast,3) == 0) {
		return;																	// not paired, nothing to send
	}

	//	"10;p01=06"   => { txt => "INFO_ACTUATOR_STATUS", params => {
	//		CHANNEL => "2,2",
	//		STATUS  => '4,2',
	//		UNKNOWN => "6,2",
	//		RSSI    => '08,02,$val=(-1)*(hex($val))' } },
	send_payLoad[0] = 0x06;														// INFO_ACTUATOR_STATUS
	send_payLoad[1] = cnl;														// channel
	send_payLoad[2] = status;													// status
	send_payLoad[3] = flag;														// unknown
	send_payLoad[4] = cc.rssi;													// RSSI
	
	// if it is an answer to a CONFIG_STATUS_REQUEST we have to use the same message id as the request
	uint8_t tCnt;
	if ((recv.data[3] == 0x01) && (recv.data[11] == 0x0E)) {
		tCnt = recv_rCnt;
	} else {
		tCnt = send.mCnt++;
	}

	send_prep(tCnt,0xA4,0x10,dParm.MAID,send_payLoad,5);						// prepare the message
}

void     HM::sendACKStatus(uint8_t cnl, uint8_t status, uint8_t douolo) {
	//if (memcmp(regDev.pairCentral,broadCast,3) == 0) return;					// not paired, nothing to send

	//	"02;p01=01"   => { txt => "ACK_STATUS",  params => {
	//		CHANNEL        => "02,2",
	//		STATUS         => "04,2",
	//		DOWN           => '06,02,$val=(hex($val)&0x20)?1:0',
	//		UP             => '06,02,$val=(hex($val)&0x10)?1:0',
	//		LOWBAT         => '06,02,$val=(hex($val)&0x80)?1:0',
	//		RSSI           => '08,02,$val=(-1)*(hex($val))', }},
	send_payLoad[0] = 0x01;														// ACK Status
	send_payLoad[1] = cnl;														// channel
	send_payLoad[2] = status;													// status
	send_payLoad[3] = douolo | (battery.state << 7);							// down, up, low battery
	send_payLoad[4] = cc.rssi;													// RSSI
	
	// l> 0E EA 80 02 1F B7 4A 63 19 63 01 01 C8 00 4B
	//send_prep(recv_rCnt,0x80,0x02,regDev.pairCentral,send_payLoad,5);	// prepare the message
	send_prep(recv_rCnt,0x80,0x02,recv_reID,send_payLoad,5);					// prepare the message
}

void     HM::sendPeerREMOTE(uint8_t cnl, uint8_t longPress, uint8_t lowBat) {
	// no data needed, because it is a (40)REMOTE EVENT
	// "40"          => { txt => "REMOTE"      , params => {
	//	   BUTTON   => '00,2,$val=(hex($val)&0x3F)',
	//	   LONG     => '00,2,$val=(hex($val)&0x40)?1:0',
	//	   LOWBAT   => '00,2,$val=(hex($val)&0x80)?1:0',
	//	   COUNTER  => "02,2", } },

	//              type  source     target     msg   cnt
	// l> 0B 0A B4  40    1F A6 5C   1F B7 4A   01    01 (l:12)(160188)
	// l> 0B 0B B4 40 1F A6 5C 1F B7 4A 41 02 (l:12)(169121)

	pevt.t = cnlDefbyPeer(cnl);													// get the index number in cnlDefType
	if (pevt.t == NULL) {
		return;
	}

	//pevt.msgCnt = dParm.cnlCnt[cnl];
	pevt.msgCnt = modTbl[cnl].msgCnt;

	if (longPress == 1) {
		pevt.reqACK = 0;														// repeated messages do not need an ACK
	} else {
		pevt.reqACK = 0x20;
	}

	pevt.type = 0x40;															// we want to send an remote event

	if (battery.state) {
		lowBat = 1;
	}

	pevt.data[0] = cnl | ((longPress) ? 1 : 0) << 6 | lowBat << 7;				// construct message

	pevt.data[1] = modTbl[cnl].msgCnt++;										// increase event counter, important for switch event
//	pevt.data[1] = dParm.cnlCnt[cnl]++;											// increase event counter, important for switch event
	pevt.len = 2;																// 2 bytes payload

	pevt.act = 1;																		// active, 1 = yes, 0 = no
	//Serial << F("remote; cdpIdx:") << pevt.cdpIdx << F(", type:"); pHexB(pevt.type)
	//Serial << F(", rACK:"); pHexB(pevt.reqACK)
	//Serial << F(", msgCnt:") << pevt.msgCnt << F(", data:"); pHex(pevt.data,pevt.len, SERIAL_DBG_PHEX_MODE_LF);
}

void     HM::sendPeerWEATHER(uint8_t cnl, int16_t temp, uint8_t hum, uint16_t pres, uint32_t lux) {
	//	TEMP     => '00,4,$val=((hex($val)&0x3FFF)/10)*((hex($val)&0x4000)?-1:1)',
	//	HUM      => '04,2,$val=(hex($val))', } },

	//              type  source     target     temp    hum
	// l> 0C 0A B4  70    1F A6 5C   1F B7 4A   01 01   01 (l:13)(160188)
	// l> 0B 0B B4  40    1F A6 5C   1F B7 4A 41 02 (l:12)(169121)

	pevt.t = cnlDefbyPeer(cnl);													// get the index number in cnlDefType
	if (pevt.t == NULL) return;
	
	pevt.type = 0x70;															// 0x70 -> frame-id for weather event
	pevt.reqACK = 0x20;															// we like to get an ACK

	pevt.data[0] = (temp & 0x7fff) >> 8 | battery.state << 7;					// temperature data
	pevt.data[1] = (temp & 0xFF);

	pevt.data[2] = hum;															// humidity

	pevt.data[3] = (pres >> 8) & 0xFF;											// air pressure
	pevt.data[4] = pres & 0xFF;

	pevt.data[5] = (lux >> 24) & 0xFFFFFF;										// luminosity
	lux = lux & 0xFFFFFF;
	pevt.data[6] = (lux >> 16) & 0xFFFF;
	lux = lux & 0xFFFF;
	pevt.data[7] = (lux >> 8) & 0xFF;
	pevt.data[8] = lux & 0xFF;

	// todo: change to 1byte
	pevt.data[9] = ((battery.voltage*100) >> 8) & 0xFF;								// battery voltage
	pevt.data[10] = (battery.voltage*100) & 0xFF;

	pevt.len = 11;

	pevt.msgCnt++;
	
	pevt.act = 1;																// active, 1 = yes, 0 = no
}

void     HM::send_ACK(void) {
	uint8_t payLoad[] = {0x00};	 														// ACK
	send_prep(recv_rCnt,0x80,0x02,recv_reID,payLoad,1);
}

void     HM::send_NACK(void) {
	uint8_t payLoad[] = {0x80};															// NACK
	send_prep(recv_rCnt,0x80,0x02,recv_reID,payLoad,1);
}

//  protected://-----------------------------------------------------------------------------------------------------------
//- hm communication functions
void     HM::recv_poll(void) {															// handles the receive objects
	// do some checkups
	if (memcmp(&recv.data[7], hmId, 3) == 0) {
		recv.forUs = 1;																	// for us
	} else {
		recv.forUs = 0;
	}

	if (memcmp(&recv.data[7], bCast, 3) == 0) {
		recv.bCast = 1;																	// or a broadcast
	} else {
		recv.bCast = 0;																	// otherwise only a log message
	}
	
	// show debug message
	#ifdef AS_DBG																		// some debug message
		if(recv.forUs) {
			Serial << F("-> ");
		} else if(recv.bCast) {
			Serial << F("b> ");
		} else {
			Serial << F("l> ");
		}

		pHex(recv.data, recv.data[0]+1, SERIAL_DBG_PHEX_MODE_LEN | SERIAL_DBG_PHEX_MODE_TIME);
		#ifdef AS_DBG_Explain
			exMsg(recv.data);																// explain message
		#endif
	#endif

	// if it's not for us or a broadcast message we could skip
	if ((recv.forUs == 0) && (recv.bCast == 0)) {
		recv.data[0] = 0;																// clear receive string
		return;
	}

	// ToDo: Workaround for pairing problems with CCU. We should fix this
	// x:1A CA E5, y: 16 30 83
/*
	// is the message from a valid sender (pair or peer), if not then exit - takes ~2ms
	if (isPairKnown(recv_reID) == 0) {													// check against peers
		#if defined(AS_DBG)																// some debug message
			Serial << F("pair/peer did not fit, exit\n");
		#endif

		recv.data[0] = 0;																// clear receive string
		return;
	}
*/

	// check if it was a repeated message, delete while already received
	if (recv_isRpt) {																	// check repeated flag
		#if defined(AS_DBG)																// some debug message
			Serial << F("   repeated message; ");
			if (recv.mCnt == recv_rCnt) {
				Serial << F("already received - skip\n");
			} else {
				Serial << F("not received before...\n");
			}
		#endif
		
		if (recv.mCnt == recv_rCnt) {													// already received
			recv.data[0] = 0;															// therefore ignore
			return;																		// and skip
		}
	}

	// if the message comes from pair, we should remember the message id
	if (isPairKnown(recv_reID)) {
		recv.mCnt = recv_rCnt;
	}

	if (((recv.forUs) || (recv.bCast)) && (recv_isMsg)) {								// message is a config message
		if (recv_msgTp == 0x01) {														// configuration message handling
			recv_PairConfig();
		
		} else if (recv_msgTp == 0x02) {												// message seems to be an ACK
			send.counter = 0;

			if (pevt.act == 1) {
				// we got an ACK after key press?
				statusLed.stop(STATUSLED_BOTH);
				if (ledMode == LED_MODE_EVERYTIME) {
					statusLed.set(STATUSLED_1, STATUSLED_MODE_BLINKSFAST, 1);			// blink led 1 once
				}
			}
	
		} else if (recv_msgTp == 0x11) {												// pair event handling
			recv_PairEvent();
			
		} else if (recv_msgTp >= 0x12) {												// peer event handling
			recv_PeerEvent();
			
		}

		#if defined(AS_DBG)																// some debug message
			else Serial << F("\nUNKNOWN MESSAGE, PLEASE REPORT!\n\n");
		#endif
	}
	
//	if((recv.forUs) && (recv.data[2] == 0x30) && (recv_msgTp == 0x11)) {				// message for update event
//			recv_UpdateEvent();
//	}

	if (recv.forUs) {
		main_Jump();																	// does main sketch want's to be informed?
	}

	//to do: if it is a broadcast message, do something with
	recv.data[0] = 0;																	// otherwise ignore
}

void     HM::send_poll(void) {															// handles the send queue
	unsigned long mills = millis();

	if(send.counter <= send.retries && (mills - send.startMillis >= send.timer)) {		// not all sends done and timing is OK
		// here we encode and send the string
		hm_enc(send.data);																// encode the string
		detachInterrupt(intGDO0.nbr);													// disable interrupt otherwise we could get some new content while we copy the buffer
		cc.sendData(send.data,send.burst);												// and send
		attachInterrupt(intGDO0.nbr,isrGDO0,FALLING);									// enable the interrupt again
		hm_dec(send.data);																// decode the string
		
		// setting some variables
		send.counter++;																	// increase send counter
		send.timer = dParm.timeOut;														// set the timer for next action
		send.startMillis = mills;
		powr.state = 1;																	// remember TRX module status, after sending it is always in RX mode

		if ((powr.mode > POWER_MODE_ON)) {
			stayAwake(TIMEOUT_AFTER_SENDING);											// stay awake for some time after sending in power mode > POWER_MODE_ON
		}

		#if defined(AS_DBG)																// some debug messages
			Serial << F("<- "); pHex(send.data, send.data[0]+1, SERIAL_DBG_PHEX_MODE_LEN | SERIAL_DBG_PHEX_MODE_TIME);
		#endif

		if (pevt.act == 1) {
			if (ledMode == LED_MODE_EVERYTIME) {
				statusLed.set(STATUSLED_BOTH, STATUSLED_MODE_BLINKSFAST, 1);			// blink led 1 and led 2 once after key press
			}
		}
	}
	
	if(send.counter > send.retries && send.counter < dParm.maxRetr) {					// all send but don't wait for an ACK
		send.counter = 0;
		send.timer = 0;																	// clear send flag
	}
	
	if(send.counter > send.retries && (mills - send.startMillis >= send.timer)) {		// max retries arrived, but seems to have no answer
		send.counter = 0;
		send.timer = 0;																	// cleanup of send buffer
		// todo: error handling, here we could jump some were to blink a led or whatever
		
		if (pevt.act == 1) {
			statusLed.stop(STATUSLED_BOTH);
			if (ledMode == LED_MODE_EVERYTIME) {
				statusLed.set(STATUSLED_2, STATUSLED_MODE_BLINKSLOW, 1);				// blink the led 2 once if key press before
			}
		}

		#if defined(AS_DBG)
			Serial << F("-> NA "); pTime(); _delay_ms(10);
		#endif
	}

}																						// ready, should work

void     HM::send_conf_poll(void) {
	if (send.counter > 0) return;														// send queue is busy, let's wait
	uint8_t len;
	
	statusLed.set(STATUSLED_1, STATUSLED_MODE_BLINKSFAST, 1);							// blink the led 1 once

	//Serial << F("send conf poll\n");
	if (conf.type == 0x01) {
		// answer                            Cnl  Peer         Peer         Peer         Peer
		// l> 1A 05 A0 10 1E 7A AD 63 19 63  01   1F A6 5C 02  1F A6 5C 01  11 22 33 02  11 22 33 01
		//                                   Cnl  Termination
		// l> 0E 06 A0 10 1E 7A AD 63 19 63  01   00 00 00 00
		len = getPeerForMsg(conf.channel, send_payLoad+1);								// get peer list
		if (len == 0x00) {																// check if all done
			memset(&conf, 0, sizeof(conf));												// clear the channel struct
			return;																		// exit
		} else if (len == 0xff) {														// failure, out of range
			memset(&conf, 0, sizeof(conf));												// clear the channel struct
			send_NACK();
		} else {																		// seems to be ok, answer
			send_payLoad[0] = 0x01;														// INFO_PEER_LIST
			send_prep(conf.mCnt++,0xA0,0x10,conf.reID,send_payLoad,len+1);				// prepare the message
		//send_prep(send.mCnt++,0xA0,0x10,conf.reID,send_payLoad,len+1);				// prepare the message
		}

	} else if (conf.type == 0x02) {
		// INFO_PARAM_RESPONSE_PAIRS message
		//                               RegL_01:  30:06 32:50 34:4B 35:50 56:00 57:24 58:01 59:01 00:00
		// l> 1A 04 A0 10  1E 7A AD  63 19 63  02  30 06 32 50 34 4B 35 50 56 00 57 24 58 01 59 01 (l:27)(131405)
		
		len = getListForMsg2(conf.channel, conf.list, conf.peer, send_payLoad+1);		// get the message
		if (len == 0) {																	// check if all done
			memset(&conf, 0, sizeof(conf));												// clear the channel struct
			return;																		// and exit
		} else if (len == 0xff) {														// failure, out of range
			memset(&conf, 0, sizeof(conf));												// clear the channel struct
			send_NACK();
		} else {																		// seems to be ok, answer
			send_payLoad[0] = 0x02;			 											// INFO_PARAM_RESPONSE_PAIRS
			send_prep(conf.mCnt++,0xA0,0x10,conf.reID,send_payLoad,len+1);				// prepare the message
			//send_prep(send.mCnt++,0xA0,0x10,conf.reID,send_payLoad,len+1);			// prepare the message
		}

	} else if (conf.type == 0x03) {
		// INFO_PARAM_RESPONSE_SEQ message
		//                               RegL_01:  30:06 32:50 34:4B 35:50 56:00 57:24 58:01 59:01 00:00
		// l> 1A 04 A0 10  1E 7A AD  63 19 63  02  30 06 32 50 34 4B 35 50 56 00 57 24 58 01 59 01 (l:27)(131405)
		
	}
	
}

void     HM::send_peer_poll(void) {
	// step through the peers and send the message
	// if we didn't found a peer to send to, then send status to master

	static uint8_t pPtr = 0, msgCnt_L, statSend;
	static s_cnlDefType *t, *tL;
	
	if (send.counter > 0) return;														// something is in the send queue, lets wait for the next free slot

	// first time run detection
	if ((tL == pevt.t) && (msgCnt_L == pevt.msgCnt)) {									// nothing changed
		if ((statSend == 1) && (pPtr >= _pgmB(&t->pMax))) {								// check if we are through
			pevt.act = 0;																// no need to jump in again
			return;
		}
	} else {																			// start again
		tL = pevt.t;																	// remember for next time
		msgCnt_L = pevt.msgCnt;
		pPtr = 0;																		// set peer pointer to start
		statSend = 0;																	// if loop is on end and statSend is 0 then we have to send to master
		t  = pevt.t;																	// some shorthand
	}
	
	// check if peer pointer is through and status to master must be send - return if done
	if ((statSend == 0) && ( pPtr >= _pgmB(&t->pMax))) {
		send_prep(send.mCnt++,(0x82|pevt.reqACK),pevt.type,dParm.MAID,pevt.data,pevt.len); // prepare the message
		statSend = 1;
		return;
	}
	
	// step through the peers, get the respective list4 and send the message
	uint32_t tPeer;
	uint8_t ret = getPeerByIdx(_pgmB(&t->cnl),pPtr,(uint8_t*)&tPeer);									
	//Serial << F("pP:") << pPtr << F(", tp:"); pHex((uint8_t*)&tPeer,4, SERIAL_DBG_PHEX_MODE_LF);
	if (tPeer == 0) {
		pPtr++;
		return;
	}
	
	// if we are here we have a valid peer address to send to
	statSend = 1;																		// first peer address found, no need to send status to the master
	
	// now we need the respective list4 to know if we have to send a burst message
	// in regLstByte there are two information. peer needs AES and burst needed
	// AES will be ignored at the moment, but burst needed will be translated into the message flag - bit 0 in regLstByte, translated to bit 4 = burst transmission in msgFlag
	uint8_t lB[1];
	getRegAddr(_pgmB(&t->cnl),_pgmB(&t->lst),pPtr++,0x01,1,lB);							// get regs for 0x01
	//Serial << F("rB:"); pHexB(lB[0]);
	//Serial << F("\n")
	send_prep(send.mCnt++,(0x82|pevt.reqACK|bitRead(lB[0],0)<<4),pevt.type,(uint8_t*)&tPeer,pevt.data,pevt.len); // prepare the message
}

void     HM::power_poll(void) {
	// there are 3 power modes for the TRX868 module
	// TX mode will switched on while something is in the send queue
	// 1 - RX mode enabled by default, take approx 17ma
	// 2 - RX is in burst mode, RX will be switched on every 250ms to check if there is a carrier signal
	//     if yes - RX will stay enabled until timeout is reached, prolongation of timeout via receive function seems not necessary
	//				to be able to receive an ACK, RX mode should be switched on by send function
	//     if no  - RX will go in idle mode and wait for the next carrier sense check
	// 3 - RX is off by default, TX mode is enabled while sending something
	//     configuration mode is required in this setup to be able to receive at least pairing and config request strings
	//     should be realized by a 15 sec timeout function for RX mode
	//     system time in millis will be hold by a regular wakeup from the watchdog timer
	// 4 - Same as power mode 3 but without watchdog
	

	if (powr.mode == POWER_MODE_ON)    return;									// in mode 1 there is nothing to do

	unsigned long mills = millis();

	if (mills - powr.startMillis < powr.nxtTO) return;							// no need to do anything
	if (send.counter > 0)   return;												// send queue not empty
	
	if ((powr.mode == POWER_MODE_BURST) && (powr.state == 0)) {
		// power mode 2, module is in sleep and next check is reached
		if (cc.detectBurst()) {													// check for a burst signal, if we have one, we should stay awake
			stayAwake(TIMEOUT_AFTER_SENDING);
			Serial << F("BURST !!! \n");
		} else {																// no burst was detected, go to sleep in next cycle
			//todo: ceck if needed anymore
			stayAwake(1);
		}

		powr.state = 1;

		return;

	} else if ((powr.mode == POWER_MODE_BURST) && (powr.state == 1)) {
		// power mode 2, module is active and next check is reached

		cc.setPowerDownState();													// go to sleep
		powr.state = 0;
		stayAwake(250);

	} else if ((powr.mode >= POWER_MODE_SLEEP_WDT) && (powr.state == 1)) {
		// 	power mode 3, check RX mode against timer. typically RX is off beside a special command to switch RX on for at least 30 seconds
		cc.setPowerDownState();													// go to sleep
		powr.state = 0;

	} else if ((powr.mode > POWER_MODE_ON) && (powr.state == 0)) {				// TRX module is off, so lets sleep for a while
		// sleep for mode 2, 3 and 4

		statusLed.stop(STATUSLED_BOTH);											// stop blinking, because we are going to sleep
		if ((powr.mode == POWER_MODE_BURST) || (powr.mode == POWER_MODE_SLEEP_WDT)) {
			WDTCSR |= (1 << WDIE);												// enable watch dog if power mode 2 or 3
		}

//		Serial << F(":"); _delay_ms(100);

		ADCSRA = 0;																// disable ADC
		uint8_t xPrr = PRR;														// turn off various modules
		PRR = 0xFF;

		sleep_enable();															// enable the sleep mode

		MCUCR = (1 << BODS) | ( 1 << BODSE);									// turn off brown-out enable in software
		MCUCR = (1 << BODS);													// must be done right before sleep

//		digitalWrite(5, 1); // debug-LED
		sleep_cpu();															// goto sleep
//		digitalWrite(5, 0); // debug-LED
		/* wake up here */
		
		sleep_disable();														// disable sleep
		if ((powr.mode == POWER_MODE_BURST) || (powr.mode == POWER_MODE_SLEEP_WDT)) {
			WDTCSR &= ~(1 << WDIE);												// disable watch dog
		}

		PRR = xPrr;																// restore modules
		
		if (wd_flag == 1) {														// add the watchdog time to millis()
			wd_flag = 0;														// to detect the next watch dog timeout
			timer0_millis += powr.wdTme;										// add watchdog time to millis() function
		} else {
			stayAwake(TIMEOUT_AFTER_SENDING);									// stay awake for some time, if the wakeup was not raised from watchdog
		}

//		Serial << F(".\n");
	}
}

void     HM::module_poll(void) {
	for (uint8_t i = 0; i <= dDef.cnlNbr; i++) {
		if (modTbl[i].use) modTbl[i].mDlgt(0,0,0,NULL,0);
	}
}

void     HM::hm_enc(uint8_t *buf) {

	buf[1] = (~buf[1]) ^ 0x89;
	uint8_t buf2 = buf[2];
	uint8_t prev = buf[1];

	uint8_t i;
	for (i=2; i<buf[0]; i++) {
		prev = (prev + 0xdc) ^ buf[i];
		buf[i] = prev;
	}

	buf[i] ^= buf2;
}

void     HM::hm_dec(uint8_t *buf) {
	uint8_t prev = buf[1];
	buf[1] = (~buf[1]) ^ 0x89;

	uint8_t i, t;
	for (i=2; i<buf[0]; i++) {
		t = buf[i];
		buf[i] = (prev + 0xdc) ^ buf[i];
		prev = t;
	}

	buf[i] ^= buf[2];
}
void     HM::exMsg(uint8_t *buf) {
	#ifdef AS_DBG_Explain
		#define b_len   buf[0]
		#define b_msgTp buf[3]
		#define b_by10  buf[10]
		#define b_by11  buf[11]
	
		Serial << F("   ");																	// save some byte and send 3 blanks once, instead of having it in every if
		
		if        ((b_msgTp == 0x00)) {
			Serial << F("DEVICE_INFO; fw: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", type: "); pHex(&buf[11],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", serial: "); pHex(&buf[13],10, SERIAL_DBG_PHEX_MODE_LF);
			Serial << F("              , class: "); pHex(&buf[23],1, SERIAL_DBG_PHEX_MODE_NONE)
			Serial << F(", pCnlA: "); pHex(&buf[24],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pCnlB: "); pHex(&buf[25],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", na: "); pHex(&buf[26],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x01)) {
			Serial << F("CONFIG_PEER_ADD; cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", peer: "); pHex(&buf[12],3, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pCnlA: "); pHex(&buf[15],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pCnlB: "); pHex(&buf[16],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x02)) {
			Serial << F("CONFIG_PEER_REMOVE; cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", peer: "); pHex(&buf[12],3, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pCnlA: "); pHex(&buf[15],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pCnlB: "); pHex(&buf[16],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x03)) {
			Serial << F("CONFIG_PEER_LIST_REQ; cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x04)) {
			Serial << F("CONFIG_PARAM_REQ; cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE)
			Serial << F(", peer: "); pHex(&buf[12],3, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pCnl: "); pHex(&buf[15],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", lst: "); pHex(&buf[16],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x05)) {
			Serial << F("CONFIG_START; cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", peer: "); pHex(&buf[12],3, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pCnl: "); pHex(&buf[15],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", lst: "); pHex(&buf[16],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x06)) {
			Serial << F("CONFIG_END; cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x08)) {
			Serial << F("CONFIG_WRITE_INDEX; cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE)
			Serial << F(", data: "); pHex(&buf[12],(buf[0]-11), SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x09)) {
			Serial << F("CONFIG_SERIAL_REQ");

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x0A)) {
			Serial << F("PAIR_SERIAL, serial: "); pHex(&buf[12],10, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x01) && (b_by11 == 0x0E)) {
			Serial << F("CONFIG_STATUS_REQUEST, cnl: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x00)) {
			if (b_len == 0x0A) Serial << F("ACK");
			else Serial << F("ACK; data: "); pHex(&buf[11],b_len-10, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x01)) {
			Serial << F("ACK_STATUS; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", status: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", down/up/loBat: "); pHex(&buf[13],1, SERIAL_DBG_PHEX_MODE_NONE);
			if (b_len > 13) Serial << F(", rssi: "); pHex(&buf[14],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x02)) {
			Serial << F("ACK2");

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x04)) {
			Serial << F("ACK_PROC; para1: "); pHex(&buf[11],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", para2: "); pHex(&buf[13],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", para3: "); pHex(&buf[15],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", para4: "); pHex(&buf[17],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x80)) {
			Serial << F("NACK");

		} else if ((b_msgTp == 0x02) && (b_by10 == 0x84)) {
			Serial << F("NACK_TARGET_INVALID");

		} else if ((b_msgTp == 0x03)) {
			Serial << F("AES_REPLY; data: "); pHex(&buf[10],b_len-9, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x04) && (b_by10 == 0x01)) {
			Serial << F("TOpHMLAN:SEND_AES_CODE; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x04)) {
			Serial << F("TO_ACTOR:SEND_AES_CODE; code: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x00)) {
			Serial << F("INFO_SERIAL; serial: "); pHex(&buf[11],10, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x01)) {
			Serial << F("INFO_PEER_LIST; peer1: "); pHex(&buf[11],4, SERIAL_DBG_PHEX_MODE_NONE);
			if (b_len >= 19) Serial << F(", peer2: "); pHex(&buf[15],4, SERIAL_DBG_PHEX_MODE_NONE);
			if (b_len >= 23) Serial << F(", peer3: "); pHex(&buf[19],4, SERIAL_DBG_PHEX_MODE_NONE);
			if (b_len >= 27) Serial << F(", peer4: "); pHex(&buf[23],4, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x02)) {
			Serial << F("INFO_PARAM_RESPONSE_PAIRS; data: "); pHex(&buf[11],b_len-10, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x03)) {
			Serial << F("INFO_PARAM_RESPONSE_SEQ; offset: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", data: "); pHex(&buf[12],b_len-11, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x04)) {
			Serial << F("INFO_PARAMETER_CHANGE; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", peer: "); pHex(&buf[12],4, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", pLst: "); pHex(&buf[16],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", data: "); pHex(&buf[17],b_len-16, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x10) && (b_by10 == 0x06)) {
			Serial << F("INFO_ACTUATOR_STATUS; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", status: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", na: "); pHex(&buf[13],1, SERIAL_DBG_PHEX_MODE_NONE);
			if (b_len > 13) Serial << F(", rssi: "); pHex(&buf[14],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x02)) {
			Serial << F("SET; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", value: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", rampTime: "); pHex(&buf[13],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", duration: "); pHex(&buf[15],2, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x03)) {
			Serial << F("STOP_CHANGE; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x04) && (b_by11 == 0x00)) {
			Serial << F("RESET");

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x80)) {
			Serial << F("LED; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE)
			Serial << F(", color: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x81) && (b_by11 == 0x00)) {
			Serial << F("LED_ALL; Led1To16: "); pHex(&buf[12],4, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x81)) {
			Serial << F("LED; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", time: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", speed: "); pHex(&buf[13],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x11) && (b_by10 == 0x82)) {
			Serial << F("SLEEPMODE; cnl: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", mode: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x12)) {
			Serial << F("HAVE_DATA");

		} else if ((b_msgTp == 0x3E)) {
			Serial << F("SWITCH; dst: "); pHex(&buf[10],3, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", na: "); pHex(&buf[13],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", cnl: "); pHex(&buf[14],1, SERIAL_DBG_PHEX_MODE_NONE)
			Serial << F(", counter: "); pHex(&buf[15],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x3F)) {
			Serial << F("TIMESTAMP; na: "); pHex(&buf[10],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", time: "); pHex(&buf[12],2, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x40)) {
			Serial << F("REMOTE; button: "); pHexB(buf[10] & 0x3F)
			Serial << F(", long: "); (buf[10] & 0x40 ? 1:0)
			Serial << F(", lowBatt: "); (buf[10] & 0x80 ? 1:0)
			Serial << F(", counter: "); pHexB(buf[11]);

		} else if ((b_msgTp == 0x41)) {
			Serial << F("SENSOR_EVENT; button: "); pHexB(buf[10] & 0x3F)
			Serial << F(", long: "); (buf[10] & 0x40 ? 1:0)
			Serial << F(", lowBatt: "); (buf[10] & 0x80 ? 1:0)
			Serial << F(", value: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", next: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x53)) {
			Serial << F("SENSOR_DATA; cmd: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE)
			Serial << F(", fld1: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", val1: "); pHex(&buf[12],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", fld2: "); pHex(&buf[14],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", val2: "); pHex(&buf[15],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", fld3: "); pHex(&buf[17],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", val3: "); pHex(&buf[18],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", fld4: "); pHex(&buf[20],1, SERIAL_DBG_PHEX_MODE_NONE)
			Serial << F(", val4: "); pHex(&buf[21],2, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x58)) {
			Serial << F("CLIMATE_EVENT; cmd: "); pHex(&buf[10],1, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", valvePos: "); pHex(&buf[11],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else if ((b_msgTp == 0x70)) {
			Serial << F("WEATHER_EVENT; temp: "); pHex(&buf[10],2, SERIAL_DBG_PHEX_MODE_NONE);
			Serial << F(", hum: "); pHex(&buf[12],1, SERIAL_DBG_PHEX_MODE_NONE);

		} else {
			Serial << F("Unknown Message, please report!");
		}
		Serial << F("\n\n");
	#endif
}

// receive message handling
void     HM::recv_PairConfig(void) {
	if (recv_by11 == 0x01) {			// 01, 01, ConfigPeerAdd
		// description --------------------------------------------------------
		//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
		// l> 10 55 A0 01 63 19 63 1E 7A AD 03   01  1F A6 5C  06         05

		// do something with the information ----------------------------------
		addPeerFromMsg(recv_payLoad[0], recv_payLoad+2);

		// send appropriate answer ---------------------------------------------
		// l> 0A 55 80 02 1E 7A AD 63 19 63 00
		if (recv_ackRq) send_ACK();													// send ACK if requested
			
	} else if (recv_by11 == 0x02) {		// 01, 02, ConfigPeerRemove
		// description --------------------------------------------------------
		//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
		// l> 10 55 A0 01 63 19 63 1E 7A AD 03   02  1F A6 5C  06         05

		// do something with the information ----------------------------------
		uint8_t ret = remPeerFromMsg(recv_payLoad[0], recv_payLoad+2);

		// send appropriate answer ---------------------------------------------
		// l> 0A 55 80 02 1E 7A AD 63 19 63 00
		if (recv_ackRq) send_ACK();

	} else if (recv_by11 == 0x03) {		// 01, 03, ConfigPeerListReq
		// description --------------------------------------------------------
		//                                  Cnl
		// l> 0B 05 A0 01 63 19 63 1E 7A AD 01  03

		// do something with the information ----------------------------------
		conf.mCnt = recv_rCnt;
		conf.channel = recv_payLoad[0];
		memcpy(conf.reID, recv_reID, 3);
		conf.type = 0x01;

		// send appropriate answer ---------------------------------------------
		// answer will be generated in config_poll function
		conf.act = 1;

	} else if (recv_by11 == 0x04) {		// 01, 04, ConfigParamReq
		// description --------------------------------------------------------
		//                                  Cnl    PeerID    PeerCnl  ParmLst
		// l> 10 04 A0 01 63 19 63 1E 7A AD 01  04 00 00 00  00       01
		// do something with the information ----------------------------------
		conf.mCnt = recv_rCnt;
		conf.channel = recv_payLoad[0];
		conf.list = recv_payLoad[6];
		memcpy(conf.peer, &recv_payLoad[2], 4);
		memcpy(conf.reID, recv_reID, 3);
		// todo: check when message type 2 or 3 is selected
		conf.type = 0x02;

		// send appropriate answer ---------------------------------------------
		// answer will be generated in config_poll function
		conf.act = 1;

	} else if (recv_by11 == 0x05) {		//01, 05, ConfigStart
		// description --------------------------------------------------------
		//                                  Cnl    PeerID    PeerCnl  ParmLst
		// l> 10 01 A0 01 63 19 63 1E 7A AD 00  05 00 00 00  00       00

		// do something with the information ----------------------------------
		// todo: check against known master id, if master id is empty, set from everyone is allowed
		conf.channel = recv_payLoad[0];												// set parameter
		memcpy(conf.peer,&recv_payLoad[2],4);
		conf.list = recv_payLoad[6];
		conf.wrEn = 1;																// and enable write to config
			
		// send appropriate answer ---------------------------------------------
		if (recv_ackRq) send_ACK();													// send ACK if requested

	} else if (recv_by11 == 0x06) {		// 01,06, ConfigEnd
		// description --------------------------------------------------------
		//                                  Cnl
		// l> 0B 01 A0 01 63 19 63 1E 7A AD 00  06
		
		// do something with the information ----------------------------------
		conf.wrEn = 0;																// disable write to config
		loadRegs();																	// probably something changed, reload config
		module_Jump(&recv_msgTp, &recv_by10, &recv_by11, &recv_by10, NULL, 0);		// search and jump in the respective channel module
			
		// send appropriate answer ---------------------------------------------
		if (recv_ackRq) send_ACK();													// send ACK if requested

	} else if (recv_by11 == 0x08) {		// 01, 08, ConfigWriteIndex
		// description --------------------------------------------------------
		//                                  Cnl    Data
		// l> 13 02 A0 01 63 19 63 1E 7A AD 00  08 02 01 0A 63 0B 19 0C 63

		// do something with the information ----------------------------------
		if ((!conf.wrEn) || (!(conf.channel == recv_payLoad[0]))) {					// but only if we are in config mode
			#if defined(AS_DBG)
				Serial << F("   write data, but not in config mode\n");
			#endif
			return;
		}
		uint8_t payLen = recv_len - 11;												// calculate len of payload and provide the data
		uint8_t ret = setListFromMsg(conf.channel, conf.list, conf.peer, payLen, &recv_payLoad[2]);
		//Serial << F("we: ") << conf.wrEn << F(", cnl: ") << conf.channel << F(", lst: ") << conf.list << F(", peer: "); pHex(conf.peer,4, SERIAL_DBG_PHEX_MODE_LF);
		//Serial << F("pl: "); pHex(&recv_payLoad[2],payLen, SERIAL_DBG_PHEX_MODE_NONE);
		//Serial << F(", ret: ", SERIAL_DBG_PHEX_MODE_LF);

		// send appropriate answer ---------------------------------------------
		if (recv_ackRq)  send_ACK();												// send ACK if requested

	} else if (recv_by11 == 0x09) {		// 01, 09, ConfigSerialReq
		// description --------------------------------------------------------
		// l> 0B 48 A0 01 63 19 63 1E 7A AD 00 09

		// do something with the information ----------------------------------
		// nothing to do, we have only to answer

		// send appropriate answer ---------------------------------------------
		//                                     SerNr
		// l> 14 48 80 10 1E 7A AD 63 19 63 00 4A 45 51 30 37 33 31 39 30 35
		send_payLoad[0] = 0x00; 													// INFO_SERIAL

		#if USE_ADRESS_SECTION == 1
			memcpy(&send_payLoad[1], &dParm.p[3], 11);								// copy details out of register.h
		#else
			memcpy(&send_payLoad[1], &dParm.p[3], 11);							// copy details out of register.h
		#endif

		send_prep(recv_rCnt,0x80,0x10,recv_reID,send_payLoad,11);					// prepare the message

	} else if (recv_by11 == 0x0A) {		// 01, 0A, Pair_Serial
		// description --------------------------------------------------------
		//                                         Serial
		// l> 15 48 A0 01 63 19 63 1E 7A AD 00 0A  4A 45 51 30 37 33 31 39 30 35
			
		// do something with the information ----------------------------------
		// compare serial number with own serial number and send pairing string back
		if (memcmp(&recv_payLoad[2], &dParm.p[3], 10) != 0) return;

		// send appropriate answer ---------------------------------------------
		// l> 1A 01 A2 00 3F A6 5C 00 00 00 10 80 02 50 53 30 30 30 30 30 30 30 31 9F 04 01 01
		#if USE_ADRESS_SECTION == 1
			memcpy(send_payLoad, dParm.p, 17);										// copy details out of register.h
		#else
			memcpy_P(send_payLoad, dParm.p, 17);									// copy details out of register.h
		#endif
		send_prep(send.mCnt++,0xA2,0x00,recv_reID,send_payLoad,17);

	} else if (recv_by11 == 0x0E) {		// 01, 0E, ConfigStatusReq
		// description --------------------------------------------------------
		//                                   Cnl
		// l> 0B 30 A0 01 63 19 63 2F B7 4A  01  0E

		// do something with the information ----------------------------------
		module_Jump(&recv_msgTp, &recv_by10, &recv_by11, &recv_by10, NULL, 0);		// jump in the respective channel module

		// send appropriate answer ---------------------------------------------
		// answer will be send from client function
	}
}
void     HM::recv_PairEvent(void) {
	// description --------------------------------------------------------
	//                 pair                    cnl  payload
	// -> 0E E6 A0 11  63 19 63  2F B7 4A  02  01   C8 00 00
	// answer is an enhanced ACK:
	// <- 0E E7 80 02 1F B7 4A 63 19 63 01 01 C8 00 54
		
	// do something with the information ----------------------------------
	module_Jump(&recv_msgTp, &recv_by10, &recv_by11, &recv_by11, recv_payLoad+2, recv_len-11);
				
	// send appropriate answer ---------------------------------------------
	// answer should be initiated by client function in user area

}

/**
 * We will reboot the controller and start the update via the bootloader
 */
//void HM::recv_UpdateEvent(void) {
//	wdt_enable(WDTO_30MS);														// activate the watchdog
//	while(1) {}																	// never ending loop, so the watchdog must reset the controller
//}

void     HM::recv_PeerEvent(void) {
	// description --------------------------------------------------------
	//                 peer                cnl  payload
	// -> 0B 56 A4 40  AA BB CC  3F A6 5C  03   CA

	// do something with the information ----------------------------------
	uint8_t peer[4];																	// bring it in a search able format
	memcpy(peer,recv_reID,4);
	peer[3] = recv_payLoad[0] & 0xF;													// only the low byte is the channel indicator
	
	uint8_t cnl = getCnlByPeer(peer);													// check on peer database
	if (!cnl) return;																	// if peer was not found, the function returns a 0 and we can leave

	uint8_t pIdx = getIdxByPeer(cnl, peer);												// get the index of the respective peer
	if (pIdx == 0xff) return;															// peer does not exist

	getCnlListByPeerIdx(cnl, pIdx);														// load list3/4
	
	module_Jump(&recv_msgTp, &recv_by10, &recv_by11, &cnl, recv_payLoad+1, recv_len-10);
	
	// send appropriate answer ---------------------------------------------
	// answer should be initiated by client function in user area
}

uint8_t  HM::main_Jump(void) {
	uint8_t ret = 0;
	for (uint8_t i = 0; ; i++) {												// find the call back function
		s_jumptable *p = &jTbl[i];												// some shorthand
		if (p->by3 == 0) {
			break;																// break if on end of list
		}
		
		if ((p->by3 != recv_msgTp) && (p->by3  != 0xff)) continue;				// message type didn't fit, therefore next
		if ((p->by10 != recv_by10) && (p->by10 != 0xff)) continue;				// byte 10 didn't fit, therefore next
		if ((p->by11 != recv_by11) && (p->by11 != 0xff)) continue;				// byte 11 didn't fit, therefore next
		
		// if we are here, all conditions are checked and ok
		p->fun(recv_payLoad, recv_len - 9);										// and jump into
		ret = 1;																// remember that we found a valid function to jump into
	}

	return ret;
}
uint8_t  HM::module_Jump(uint8_t *by3, uint8_t *by10, uint8_t *by11, uint8_t *cnl, uint8_t *data, uint8_t len) {
	if (modTbl[*cnl].use == 0) return 0;													// nothing found in the module table, exit

	modTbl[*cnl].mDlgt(*by3,*by10,*by11,data,len);											// find the right entry in the table
	return 1;
}

//- internal send functions
void     HM::send_prep(uint8_t msgCnt, uint8_t comBits, uint8_t msgType, uint8_t *targetID, uint8_t *payLoad, uint8_t payLen) {
	send.data[0]  = 9 + payLen;															// message length
	send.data[1]  = msgCnt;																// message counter

	// set the message flags
	//    #RPTEN    0x80: set in every message. Meaning?
	//    #RPTED    0x40: repeated (repeater operation)
	//    #BIDI     0x20: response is expected						- should be in comBits
	//    #Burst    0x10: set if burst is required by device        - will be set in peer send string if necessary
	//    #Bit3     0x08:
	//    #CFG      0x04: Device in Config mode						- peer seems to be always in config mode, message to master only if an write mode enable string was received
	//    #WAKEMEUP 0x02: awake - hurry up to send messages			- only for a master, peer don't need
	//    #WAKEUP   0x01: send initially to keep the device awake	- should be only necessary while receiving

	send.data[2]  = comBits;															// take the communication bits
	send.data[3]  = msgType;															// message type
	memcpy(&send.data[4], hmId, 3);														// source id
	memcpy(&send.data[7], targetID, 3);													// target id

	if ((uint16_t)payLoad != (uint16_t)send_payLoad)
	memcpy(send_payLoad, payLoad, payLen);												// payload
	
	#if defined(AS_DBG)																	// some debug messages
	exMsg(send.data);																	// explain message
	#endif

	send_out();
}

//- to check incoming messages if sender is known
uint8_t  HM::isPeerKnown(uint8_t *peer) {
	uint8_t tPeer[3];

	for (uint16_t i = 0; i < ee[1].peerDB; i+=4) {
		getEeBl(ee[0].peerDB+i,3,tPeer);												// copy the respective eeprom block to the pointer
		if (memcmp(peer,tPeer,3) == 0) return 1;
	}
	
	return 0;																			// nothing found
}

uint8_t  HM::isPairKnown(uint8_t *pair) {
	//Serial << F("x:"); pHex(dParm.MAID,3, SERIAL_DBG_PHEX_MODE_NONE);
	//Serial << F(" y:"); pHex(pair,3, SERIAL_DBG_PHEX_MODE_LF, SERIAL_DBG_PHEX_MODE_NONE);
	if (memcmp(dParm.MAID, bCast, 3) == 0) return 1;									// return 1 while not paired
	
	if (memcmp(dParm.MAID, pair, 3) == 0) return 1;										// check against regDev
	else return 0;																		// found nothing, return 0
}

//- peer specific public functions
uint8_t  HM::addPeerFromMsg(uint8_t cnl, uint8_t *peer) {
	// given peer incl the peer address (3 bytes) and probably of 2 peer channels (2 bytes)
	// therefore we have to shift bytes in our own variable
	uint8_t tPeer[4],ret1,ret2;															// size variables

	if (peer[3]) {
		memcpy(tPeer,peer,4);															// copy the peer in a variable
		ret1 = addPeer(cnl, tPeer);														// add the first peer
	}

	if (peer[4]) {
		tPeer[3] = peer[4];																// change the peer channel
		ret2 = addPeer(cnl, tPeer);														// add the second peer
	}

	#ifdef RPDB_DBG																		// some debug message
	Serial << F("addPeerFromMsg, cnl: ") << cnl << F(", ret1: ") << ret1 << F(", ret2: ") << ret2 << F("\n");
	#endif
	
	//                                  Cnl      PeerID    PeerCnl_A  PeerCnl_B
	// l> 10 55 A0 01 63 19 63 1E 7A AD 03   01  1F A6 5C  06         05
	recv_payLoad[7] = ret1;
	recv_payLoad[8] = ret2;
	module_Jump(&recv_msgTp, &recv_by10, &recv_by11, &recv_by10, recv_payLoad+5, 4);		// jump in the respective channel module

	return 1;																			// every thing went ok
}
uint8_t  HM::remPeerFromMsg(uint8_t cnl, uint8_t *peer) {
	// given peer incl the peer address (3 bytes) and probably of 2 peer channels (2 bytes)
	// therefore we have to shift bytes in our own variable
	uint8_t tPeer[4];																	// size variables

	memcpy(tPeer,peer,4);																// copy the peer in a variable
	uint8_t ret1 = remPeer(cnl, tPeer);													// remove the first peer

	tPeer[3] = peer[4];																	// change the peer channel
	uint8_t ret2 = remPeer(cnl, tPeer);													// remove the second peer

	#ifdef RPDB_DBG																		// some debug message
	Serial << F("remPeerFromMsg, cnl: ") << cnl << F(", ret1: ") << ret1 << F(", ret2: ") << ret2 << F("\n");
	#endif
	
	return 1;																			// every thing went ok
}
uint8_t  HM::valPeerFromMsg(uint8_t *peer) {
	uint8_t ret = getCnlByPeer(peer);
	return (ret == 0xff)?0:1;
}
uint8_t  HM::getPeerForMsg(uint8_t cnl, uint8_t *buf) {
	static uint8_t idx_L = 0xff, ptr;
	static s_cnlDefType *tL;

	s_cnlDefType *t = cnlDefbyPeer(cnl);												// get the index number in cnlDefType

 	if (tL != t) {																		// check for first time
		ptr = 0;																		// set the pointer to 0
		tL = t;																			// remember for next call check
		
	} else {																			// next time detected
		if (ptr == 0xff) {																// we are completely through
			tL = NULL;
			return 0;	
		}
	}
	
	uint8_t cnt = 0;																	// size a counter and a long peer store
	uint32_t peerL;

	while ((ptr < _pgmB(&t->pMax)) && (cnt < maxPayload)) {								// loop until buffer is full or end of table is reached

		peerL = getEeLo(cdPeerAddrByCnlIdx(cnl,ptr));									// get the peer
		
		//Serial << F("start: ") << start << F(", len: ") << len << F(", pos: ") << (cnt-start) << F("\n");
		if (peerL != 0) {																// if we found an filled block

			//Serial << F("i: ") << cnt << F("pL: "); pHex((uint8_t*)&peerL,4, SERIAL_DBG_PHEX_MODE_LF);
			memcpy(&buf[cnt], (uint8_t*)&peerL, 4);										// copy the content
			cnt+=4;																		// increase the pointer to buffer for next time
		}
		ptr++;
	}
	
	if (cnt < maxPayload) {																// termination of message needs two zeros
		memcpy(&buf[cnt], bCast, 4);													// copy the content
		cnt+=4;																			// increase the pointer to buffer for next time
		ptr = 0xff;																		// indicate that we are through
	}

	return cnt;																			// return delivered bytes
}

//- regs specific public functions
uint8_t  HM::getListForMsg2(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf) {
	static uint8_t pIdx_L = 0xff, ptr = 0;

	uint8_t pIdx = getIdxByPeer(cnl, peer);												// get the index of the respective peer
	if (pIdx == 0xff) return 0xff;														// peer does not exist

	if (pIdx_L != pIdx) {																// check for first time
		ptr = 0;																		// set the pointer to 0
		pIdx_L = pIdx;																	// remember for next call check
		
	} else {																			// next time detected
		if (ptr == 0xff) {
			pIdx_L = 0xff;																// set the pointer to 0
			return 0;																	// we are completely through
		}
	}
	
	s_cnlDefType *t = cnlDefbyList(cnl, lst);
	if (t == NULL) return 0;

	uint8_t  slcAddr = _pgmB(&t->sIdx);													// get the respective addresses
	uint16_t regAddr = cdListAddrByPtr(t, pIdx);

	uint8_t cnt = 0;																	// step through the bytes and generate message
	while ((ptr < _pgmB(&t->sLen)) && (cnt < maxPayload)) {								// loop until buffer is full or end of table is reached
		memcpy(&buf[cnt++], &dDef.slPtr[slcAddr + ptr], 1);								// copy one byte from sliceStr
		getEeBl(regAddr + ptr, 1, &buf[cnt++]);
		ptr++;
	}

	if (cnt < maxPayload) {																// termination of message needs two zeros
		buf[cnt++] = 0;																	// write 0 to buffer
		buf[cnt++] = 0;
		ptr = 0xff;																		// indicate that we are through
	}

	return cnt;																			// return delivered bytes
}
uint8_t  HM::setListFromMsg(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t len, uint8_t *buf) {
	// get the address of the respective slcStr
	uint8_t pIdx = getIdxByPeer(cnl, peer);												// get the index of the respective peer
	if (pIdx == 0xff) return 0xff;														// peer does not exist
	
	s_cnlDefType *t = cnlDefbyList(cnl, lst);
	if (t == NULL) return 0;

	
	uint8_t  slcAddr = _pgmB(&t->sIdx);													// get the respective addresses
	uint16_t regAddr = cdListAddrByPtr(t, pIdx);

	// search through the sliceStr for even bytes in buffer
	for (uint8_t i = 0; i < len; i+=2) {
		void *x = memchr(&dDef.slPtr[slcAddr], buf[i], _pgmB(&t->sLen));				// find the byte in slcStr
		if ((uint16_t)x == 0) continue;													// got no result, try next
		pIdx = (uint8_t)((uint16_t)x - (uint16_t)&dDef.slPtr[slcAddr]);					// calculate the relative position
		//Serial << F("x: "); pHexB(buf[i]);
		//Serial << F(", ad: ") << pIdx << F("\n");
		setEeBy(regAddr+pIdx,buf[i+1]);													// write the bytes to the respective address
	}
	return 1;
}
uint8_t  HM::getRegAddr(uint8_t cnl, uint8_t lst, uint8_t pIdx, uint8_t addr, uint8_t len, uint8_t *buf) {

	s_cnlDefType *t = cnlDefbyList(cnl, lst);
	if (t == NULL) return 0;

	void *x = memchr(&dDef.slPtr[_pgmB(&t->sIdx)], addr, _pgmB(&t->sLen));				// search for the reg byte in slice string
	if ((uint16_t)x == 0) return 0;														// if not found return 0
	uint8_t xIdx = (uint8_t)((uint16_t)x - (uint16_t)&dDef.slPtr[_pgmB(&t->sIdx)]);		// calculate the base address
	
	uint16_t regAddr = cdListAddrByPtr(t, pIdx);										// get the base address in eeprom
	getEeBl(regAddr+xIdx,len,buf);														// get the respective block from eeprom
	return 1;																			// we are successfully
}
uint8_t  HM::doesListExist(uint8_t cnl, uint8_t lst) {
	if (cnlDefbyList(cnl,lst) == NULL) return 0;
	else return 1;
}
void     HM::getCnlListByPeerIdx(uint8_t cnl, uint8_t peerIdx) {

	s_cnlDefType *t = cnlDefbyPeer(cnl);
	if (t == NULL) return;


	uint16_t addr = cdListAddrByPtr(t, peerIdx);										// get the respective eeprom address
	getEeBl(addr, _pgmB(&t->sLen), (void*)_pgmW(&t->pRegs));							// load content from eeprom
	
	#if defined(SM_DBG)																	// some debug message
	Serial << F("Loading list3/4 for cnl: ") << cnl << F(", peer: "); pHex(peer,4, SERIAL_DBG_PHEX_MODE_LF);
	#endif
}
void     HM::setListFromModule(uint8_t cnl, uint8_t peerIdx, uint8_t *data, uint8_t len) {
	s_cnlDefType *t = cnlDefbyPeer(cnl);
	
	uint16_t addr = cdListAddrByPtr(t,peerIdx);
	if (addr == 0x00) return;
	setEeBl(addr, len, data);
}

//- peer specific private functions
uint8_t  HM::getCnlByPeer(uint8_t *peer) {

	for (uint8_t i = 1; i <= dDef.cnlNbr; i++) {										// step through all channels

		uint8_t ret = getIdxByPeer(i,peer);												// searching inside the channel
		//Serial << F("cnl: ") << i << F(", ret: ") << ret << F("\n");							// some debug message
		if (ret != 0xff) return i;														// if something found then return
	}
	return 0xff;																		// nothing found
}
uint8_t  HM::getIdxByPeer(uint8_t cnl, uint8_t *peer) {
	if (cnl == 0) return 0;																// channel 0 does not provide any peers

	s_cnlDefType *t = cnlDefbyPeer(cnl);
	if (t == NULL) return 0xff;
	
	for (uint8_t i = 0; i < _pgmB(&t->pMax); i++) {										// step through the peers
		if (getEeLo(cdPeerAddrByCnlIdx(cnl,i)) == *(uint32_t*)peer) return i;			// load peer from eeprom and compare
	}
	return 0xff;																		// out of range
}
uint8_t  HM::getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer) {
	uint16_t eeAddr = cdPeerAddrByCnlIdx(cnl ,idx);										// get the address in eeprom
	if (eeAddr == 0) return 0;															// no address found, exit
	getEeBl(eeAddr,4,peer);																// copy the respective eeprom block to the pointer
	return 1;																			// done
}
uint8_t  HM::getFreePeerSlot(uint8_t cnl) {
	s_cnlDefType *t = cnlDefbyPeer(cnl);
	if (t == NULL) return 0xff;
	
	for (uint8_t i = 0; i < _pgmB(&t->pMax); i++) {										// step through all peers, given by pMax
		if (getEeLo(cdPeerAddrByCnlIdx(cnl,i)) == 0) return i;							// if we found an empty block then return the number
	}
	return 0xff;																		// no block found
}
uint8_t  HM::cntFreePeerSlot(uint8_t cnl) {
	//Serial << F("idx:") << ret << F(", pMax:") << _pgmB(&dDef.chPtr[ret].pMax) << F("\n");
	s_cnlDefType *t = cnlDefbyPeer(cnl);
	if (t == NULL) return 0xff;
	
	uint8_t cnt = 0;
	for (uint8_t i = 0; i < _pgmB(&t->pMax); i++) {										// step through all peers, given by pMax
		if (getEeLo(cdPeerAddrByCnlIdx(cnl,i)) == 0) cnt++;								// increase counter if slot is empty
	}
	return cnt;																			// return the counter
}
uint8_t  HM::addPeer(uint8_t cnl, uint8_t *peer) {
	if (getIdxByPeer(cnl, peer) != 0xff) return 0xfe;									// check if peer already exists
	
	uint8_t idx = getFreePeerSlot(cnl);													// find a free slot
	if (idx == 0xff) return 0xfd;														// no free slot

	uint16_t eeAddr = cdPeerAddrByCnlIdx(cnl, idx);										// get the address in eeprom
	if (eeAddr == 0) return 0xff;														// address not found, exit
	
	setEeBl(eeAddr,4,peer);																// write to the slot
	return idx;
}
uint8_t  HM::remPeer(uint8_t cnl, uint8_t *peer) {
	uint8_t idx = getIdxByPeer(cnl, peer);												// get the idx of the peer
	if (idx == 0xff) return 0;															// peer not found, exit
	
	uint16_t eeAddr = cdPeerAddrByCnlIdx(cnl, idx);										// get the address in eeprom
	if (eeAddr == 0) return 0;															// address not found, exit
	
	setEeLo(eeAddr,0);																	// write a 0 to the given slot
	return 1;																			// everything is fine
}

//- cnlDefType specific functions
uint16_t HM::cdListAddrByCnlLst(uint8_t cnl, uint8_t lst, uint8_t peerIdx) {
	s_cnlDefType *t = cnlDefbyList(cnl, lst);
	return cdListAddrByPtr(t, peerIdx);
}
uint16_t HM::cdListAddrByPtr(s_cnlDefType *ptr, uint8_t peerIdx) {
	return ee[0].regsDB + _pgmW(&ptr->pAddr) + (peerIdx * _pgmB(&ptr->sLen));			// calculate the starting address
}
s_cnlDefType* HM::cnlDefbyList(uint8_t cnl, uint8_t lst) {
	
	for (uint8_t i = 0; i < dDef.lstNbr; i++) {											// step through the list

		if ((cnl == _pgmB(&dDef.chPtr[i].cnl)) && (lst == _pgmB(&dDef.chPtr[i].lst)))
		return (s_cnlDefType*)&dDef.chPtr[i];											// find the respective channel and list
	}
	return NULL;																		// nothing found
}

uint16_t HM::cdPeerAddrByCnlIdx(uint8_t cnl, uint8_t peerIdx) {
	s_cnlDefType *t = cnlDefbyPeer(cnl);
	if (t == NULL) return 0;
	
	if ((peerIdx) && (peerIdx >= _pgmB(&t->pMax))) return 0;							// return if peer index is out of range
	return ee[0].peerDB + _pgmW(&t->pPeer) + (peerIdx * 4);								// calculate the starting address
}
s_cnlDefType* HM::cnlDefbyPeer(uint8_t cnl) {

	for (uint8_t i = 0; i < dDef.lstNbr; i++) {											// step through the list

		if ((cnl == _pgmB(&dDef.chPtr[i].cnl)) && (_pgmB(&dDef.chPtr[i].pMax)))
		return (s_cnlDefType*)&dDef.chPtr[i];											// find the respective channel which includes a peer
	}
	return NULL;																		// nothing found
}

//- pure eeprom handling, i2c to be implemented
uint8_t  HM::getEeBy(uint16_t addr) {
	// todo: lock against writing
	// todo: extend range for i2c eeprom
	return eeprom_read_byte((uint8_t*)addr);
	//uint8_t b;
	//getEeBl(addr,1,&b);
	//return b;
}
void     HM::setEeBy(uint16_t addr, uint8_t payload) {
	// todo: lock against reading
	// todo: extend range for i2c eeprom
	eeprom_write_byte((uint8_t*)addr,payload);
	//setEeBl(addr,1,&payload);

}
uint16_t HM::getEeWo(uint16_t addr) {
	// todo: lock against writing
	// todo: extend range for i2c eeprom
	return eeprom_read_word((uint16_t*)addr);
	//uint16_t w;
	//getEeBl(addr,2,&w);
	//return w;

}
void     HM::setEeWo(uint16_t addr, uint16_t payload) {
	// todo: lock against reading
	// todo: extend range for i2c eeprom
	eeprom_write_word((uint16_t*)addr,payload);
	//setEeBl(addr,2,&payload);
}
uint32_t HM::getEeLo(uint16_t addr) {
	// todo: lock against writing
	// todo: extend range for i2c eeprom
	return eeprom_read_dword((uint32_t*)addr);
	//uint32_t l;
	//getEeBl(addr,4,&l);
	//return l;

}
void     HM::setEeLo(uint16_t addr, uint32_t payload) {
	// todo: lock against reading
	// todo: extend range for i2c eeprom
	eeprom_write_dword((uint32_t*)addr,payload);
	//setEeBl(addr,4,&payload);

}
void     HM::getEeBl(uint16_t addr,uint8_t len,void *ptr) {
	// todo: lock against reading
	// todo: extend range for i2c eeprom
	eeprom_read_block((void*)ptr,(const void*)addr,len);
}
void     HM::setEeBl(uint16_t addr,uint8_t len,void *ptr) {
	// todo: lock against reading
	// todo: extend range for i2c eeprom
	eeprom_write_block((const void*)ptr,(void*)addr,len);
}
void     HM::clrEeBl(uint16_t addr, uint16_t len) {
	for (uint16_t l = 0; l < len; l++) {										// step through the bytes of eeprom
		setEeBy(addr+l, 0);														// and write a 0
	}

	_delay_ms(50);																// give eeprom some time
}

HM hm;
//- -----------------------------------------------------------------------------------------------------------------------

//- interrupt handling for interface communication module to AskSin library
void isrGDO0() {
	int0_flag = 1;
}

ISR( WDT_vect ) {
	wd_flag = 1;
}
