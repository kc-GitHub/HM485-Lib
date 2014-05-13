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

// TODO: Do the device relations really belong here?
// TODO: Wo werden die Device Relations gesetzt? Irgendwo im EEPROM?
#define MAX_DEVICE_RELATIONS 100

class HMWRS485 {
public:
	HMWRS485(Stream*, byte, Stream*);  // RS485 interface, TX-Enable Pin, debug-Serial
	virtual ~HMWRS485();

	void receive();  // muss zyklisch aufgerufen werden

	void parseFrame();
	void sendFrame();
	void sendAck(byte);  // rxSeqNum
	void debug(char*);

	// Empfangen
	byte frameComplete;
    unsigned long targetAddress;
	byte frameDataLength;                 // Laenge der Daten
	byte frameData[MAX_RX_FRAME_LENGTH];
	byte startByte;
	byte frameControlByte;
	unsigned long senderAddress;

	// Senden
	unsigned long txSenderAddress;       // eigene Adresse
	unsigned long txTargetAddress;        // Adresse des Moduls
	byte txFrameControlByte;
    byte txFrameDataLength;              // Laenge der Daten + Checksum
	byte txFrameData[MAX_RX_FRAME_LENGTH];

private:
// Das eigentliche RS485-Interface, kann SoftSerial oder (Hardware)Serial sein
	Stream* serial;
// Pin-Nummer fuer "TX-Enable"
	byte txEnablePin;
// Stream fuer Debug-Ausgaben
	Stream* debugSerial;

	// TODO: Das Folgende geht etwas verschwenderisch mit dem Speicher um
	// Addressen, auf deren ACK gewartet wird (?)
	unsigned long waitAckAddress[MAX_DEVICE_RELATIONS];
	// Nummer der gesendeten Nachricht, auf deren ACK wir warten (?)
	// TODO: oder sogar das ganze CTRL-Byte?
	byte waitAckSeqNum[MAX_DEVICE_RELATIONS];

	// Empfangs-Status
	byte frameStatus;

	byte findCtrlByAddress(unsigned long);
	void resetAckwaitByAddress(unsigned long);
	void sendFrameByte(byte);
	unsigned int crc16Shift(byte, unsigned int);
};

#endif /* HMWRS485_H_ */
