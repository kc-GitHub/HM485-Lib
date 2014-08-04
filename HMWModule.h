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
#define MODULE_FIRMWARE_VERSION 0x0306


// Abstrakte Basisklasse mit Callbacks aus dem Modul
class HMWDeviceBase {
  public:
	virtual void setLevel(byte,unsigned int) = 0;  // channel, level
	virtual unsigned int getLevel(byte) = 0;       // channel, returns level
	virtual void readConfig() = 0;         // read config from EEPROM
};


class HMWModule {
public:
	HMWModule(HMWDeviceBase*, HMWRS485*, byte); // rs485, device type
	virtual ~HMWModule();

	void processEvents();

	void broadcastAnnounce(byte);  // channel
	void broadcastKeyEvent(byte, byte, byte = 0);  // channel, keyPressNum, long/short (long = 1)
	void broadcastInfoMessage(byte, unsigned int);   // channel, info

	byte deviceType;        // device type @ 0x7FF1 in FlashRom  TODO: Not really...

	// write to EEPROM, but only if not "value" anyway
	// the uppermost 4 bytes are reserved for the device address and can only be changed if privileged = true
	void writeEEPROM(int address, byte value, bool privileged = false );

private:
	HMWRS485* hmwrs485;
	HMWDeviceBase* device;

	void readAddressFromEEPROM();
	void determineSerial(byte*);

	void processEventKey();
    void processEventSetLevel();
	void processEventGetLevel();
	void processEventSetLock();
	void processEmessage();
};

#endif /* HMWMODULE_H_ */
