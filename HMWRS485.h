/*
 * HMWRS485.h
 *
 *  Created on: 09.05.2014
 *      Author: thorsten
 */

#ifndef HMWRS485_H_
#define HMWRS485_H_

#include "Arduino.h"

#define MAX_RX_FRAME_LENGTH 64
// bus must be idle 210 + rand(0..100) ms
#define DIFS_CONSTANT 210
#define DIFS_RANDOM 100
// we wait max 200ms for an ACK
#define ACKWAITTIME 200
// while waiting for an ACK, bus might not be idle
// bus has to be idle at least this time for a retry
#define RETRYIDLETIME 150

// TODO: Do the device relations really belong here?
// TODO: Wo werden die Device Relations gesetzt? Irgendwo im EEPROM?
// #define MAX_DEVICE_RELATIONS 100  TODO: loeschen, wenn nicht gebraucht

// Abstrakte Basisklasse mit Callbacks fuer Verwender
class HMWModuleBase {
  public:
// processEvent wird nur aufgerufen, wenn es fuer das Modul was zu tun gibt
// Also nicht fuer ACKs oder duplicates
// TODO: Should return whether an ACK is needed or not
	virtual void processEvent(byte const * const frameData, byte frameDataLength, boolean isBroadcast = false) = 0;  // Data, broadcast-Flag
};


class HMWRS485 {
public:
	HMWRS485(Stream*, byte);  // RS485 interface, TX-Enable Pin
	virtual ~HMWRS485();

	void loop(); // main loop, die immer wieder aufgerufen werden muss

	// sendFrame macht ggf. Wiederholungen
	// onlyIfIdle: If this is set, then the bus must have been idle since 210+rand(0..100) ms
	// sendFrame returns...
	//   0 -> ok
	//   1 -> bus not idle (only if onlyIfIdle)
	//   2 -> three times no ACK (cannot occur for broadcasts or ACKs)
	byte sendFrame(boolean onlyIfIdle = false);
	void sendAck();  // ACK fuer gerade verarbeitete Message senden

	// eigene Adresse setzen und damit auch random seed
	void setOwnAddress(unsigned long address);
	unsigned long getOwnAddress();

	// Modul-Definition, wird vom Modul selbst gesetzt
	// TODO: Ist das gutes Design?
   HMWModuleBase* module;

    // Senderadresse beim Empfangen
    // TODO: Das sollte private sein, wird aber momentan noch vom Modul verwendet
	unsigned long senderAddress;

	// Senden
	unsigned long txTargetAddress;        // Adresse des Moduls, zu dem gesendet wird
	byte txFrameControlByte;
    byte txFrameDataLength;              // Laenge der Daten + Checksum
	byte txFrameData[MAX_RX_FRAME_LENGTH];

private:
// Das eigentliche RS485-Interface, kann SoftSerial oder (Hardware)Serial sein
	Stream* serial;
// Pin-Nummer fuer "TX-Enable"
	byte txEnablePin;
	// Empfangs-Status
	byte frameStatus;
// eigene Adresse
	unsigned long ownAddress;
// Empfangene Daten
	// Empfangen
	byte frameComplete;
    unsigned long targetAddress;
	byte frameDataLength;                 // Laenge der Daten
	byte frameData[MAX_RX_FRAME_LENGTH];
	byte startByte;
	byte frameControlByte;

// carrier sense
//  last time we have received anything
    unsigned long lastReceivedTime;
//  current minimum idle time
//  will be initialized in constructor
    unsigned int minIdleTime;

	// Sende-Versuch, wird ggf. wiederholt
	void receive();  // wird zyklisch aufgerufen
	boolean parseFrame();

	void sendFrameSingle();
	void sendFrameByte(byte);
	unsigned int crc16Shift(byte, unsigned int);
};

#endif /* HMWRS485_H_ */
