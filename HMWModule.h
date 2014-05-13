/*
 * HMWModule.h
 *
 *  Created on: 09.05.2014
 *      Author: thorsten
 */

#ifndef HMWMODULE_H_
#define HMWMODULE_H_

#include "HMWRS485.h"

#include "Arduino.h"

// TODO: Firmware/Hardware Version aus EEPROM bzw. Flash?
#define MODULE_HARDWARE_VERSION 1
#define MODULE_FIRMWARE_VERSION 1


class HMWModule {
public:
	HMWModule(HMWRS485*);
	virtual ~HMWModule();

	void processEvents();

	byte deviceType;        // device type @ 0x7FF1 in FlashRom  TODO: Not really...
	char deviceSerial[10];    // string of device serial @ 0x7FF2 - 0x7FFB in FlashRom TODO: Not really...

private:
	HMWRS485* hmwrs485;

	void processEventKey();
    void processEventSetLevel();
	void processEventGetLevel();
	void processEventSetLock();
};

#endif /* HMWMODULE_H_ */
