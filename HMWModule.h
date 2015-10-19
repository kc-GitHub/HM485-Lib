/*
 * HMWModule.h
 *
 *  Created on: 09.05.2014
 *      Author: thorsten
 */

#ifndef HMWMODULE_H_
#define HMWMODULE_H_

#include "HMWRS485.h"

#include "Platform_Dependencies.h"

// TODO: Firmware/Hardware Version aus EEPROM bzw. Flash?
#define MODULE_HARDWARE_VERSION 1
#define MODULE_FIRMWARE_VERSION 0x0306
#

// Abstrakte Basisklasse mit Callbacks aus dem Modul
class HMWDeviceBase {
  public:
	virtual ~HMWDeviceBase() {;};
	virtual void setLevel(uint8_t,uint16_t) {};  // channel, level (deprecated)
	virtual void setLevel(uint8_t,uint8_t,uint8_t const * const) {};    // channel, data length, data
	virtual uint16_t getLevel(uint8_t, uint8_t) = 0;  // channel, command (0x78, 0x53), returns level
	virtual void readConfig() = 0;         // read config from EEPROM
};

class HMWModule : public HMWModuleBase {
public:
	#define HMW_TARGET_ADDRESS_BC ((uint32_t) 0xFFFFFFFF)
	HMWModule(HMWDeviceBase*, HMWRS485*, uint8_t); // rs485, device type
	virtual ~HMWModule();

	virtual void processEvent(uint8_t const * const frameData, uint8_t frameDataLength, boolean isBroadcast = false);

	// the broadcast methods return...
	//  0 -> ok
	//  1 -> bus not idle
	uint8_t broadcastAnnounce(uint8_t);  // channel
	inline uint8_t broadcastKeyEvent(uint8_t channel, uint8_t keyPressNum, uint8_t longPress = 0) {return sendKeyEvent(channel,keyPressNum,longPress);};  // channel, keyPressNum, long/short (long = 1)

	// the broadcast methods return...
	//  0 -> ok
	//  1 -> bus not idle
	//  2 -> missing ACK (three times)
	uint8_t sendKeyEvent(uint8_t channel, uint8_t keyPressNum, bool longPress = false ,uint32_t target_address = HMW_TARGET_ADDRESS_BC,uint8_t targetchannel = 0);

	// sendInfoMessage returns...
	//  0 -> ok
	//  1 -> bus not idle
	//  2 -> missing ACK (three times)
	uint8_t sendInfoMessage(uint8_t, uint16_t, uint32_t);   // channel, info, target address

	uint8_t deviceType;        // device type @ 0x7FF1 in FlashRom  TODO: Not really...

	// write to EEPROM, but only if not "value" anyway
	// the uppermost 4 bytes are reserved for the device address and can only be changed if privileged = true
	void writeEEPROM(int16_t address, uint8_t value, bool privileged = false );

private:
	HMWRS485* hmwrs485;
	HMWDeviceBase* device;

	void readAddressFromEEPROM();
	void determineSerial(uint8_t*);

	void processEventKey();
    void processEventSetLevel(uint8_t channel, uint8_t dataLength, uint8_t const * const data);  //TODO: rename?
	void processEventGetLevel(uint8_t channel, uint8_t command);
	void processEventSetLock();
	void processEmessage(uint8_t const * const frameData);
};

#endif /* HMWMODULE_H_ */
