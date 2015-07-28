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
	virtual void setLevel(byte,unsigned int) {};  // channel, level (deprecated)
	virtual void setLevel(byte,byte,byte const * const) {};    // channel, data length, data
	virtual unsigned int getLevel(byte, byte) = 0;  // channel, command (0x78, 0x53), returns level
	virtual void readConfig() = 0;         // read config from EEPROM
};

class HMWModule : public HMWModuleBase {
public:
	HMWModule(HMWDeviceBase*, HMWRS485*, byte); // rs485, device type
	virtual ~HMWModule();

	virtual void processEvent(byte const * const frameData, byte frameDataLength, boolean isBroadcast = false);

	// the broadcast methods return...
	// 0 -> everything ok
	// 1 -> nothing sent because bus busy
	byte broadcastAnnounce(byte);  // channel
	byte broadcastKeyEvent(byte, byte, byte = 0);  // channel, keyPressNum, long/short (long = 1)

	// sendInfoMessage returns...
	//  0 -> ok
	//  1 -> bus not idle
	//  2 -> missing ACK (three times)
	byte sendInfoMessage(byte, unsigned int, unsigned long);   // channel, info, target address

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
    void processEventSetLevel(byte channel, byte dataLength, byte const * const data);  //TODO: rename?
	void processEventGetLevel(byte channel, byte command);
	void processEventSetLock();
	void processEmessage(byte const * const frameData);
};

#endif /* HMWMODULE_H_ */
