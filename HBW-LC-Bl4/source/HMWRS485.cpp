/*
 * HMWRS485.cpp
 *
 *  Created on: 09.05.2014
 *      Author: Thorsten Pferdekaemper thorsten@pferdekaemper.com
 *      Nach einer Vorlage von Dirk Hoffmann (dirk@hfma.de) 2012
 *
 *  Homematic-Wired RS485-Protokoll (ohne Interpretation der Befehle)
 */

#include "HMWRS485.h"
#include "HMWDebug.h"

#include "Arduino.h"

HMWRS485::HMWRS485(Stream* _serial, byte _txEnablePin) {
	serial = _serial;
	txEnablePin = _txEnablePin;
	frameComplete = 0;
}

HMWRS485::~HMWRS485() {
	// TODO Auto-generated destructor stub
}

// TODO: Vielleicht kann man das ganze als Zustandsautomat implementieren,
//       um Speicherplatz zu sparen

// TODO: Eigene SoftSerial Library mit entsprechender Parity etc.?
//       (Im Prinzip nur fuer die Arduino-Version gebraucht.)
//   $baud = 19200
//   config Com1 = Dummy , Synchrone = 0 , Parity = even , Stopbits = 1 , Databits = 8 , Clockpol = 0

// Defines
// TODO: In Protokollbeschreibung nachsehen und entsprechend kommentieren
//       Das mit den short frames ist extrem unklar...
#define ENABLE_FRAME_START_SHORT 0

#define FRAME_START_SHORT 0xFE
#define FRAME_START_LONG 0xFD
#define CRC16_POLYNOM 0x1002
#define ESCAPE_CHAR 0xFC

// Empfangs-Status
#define FRAME_START 1    // Startzeichen gefunden
#define FRAME_ESCAPE 2   // Escape-Zeichen gefunden
#define FRAME_SENTACKWAIT 8  // Nachricht wurde gesendet, warte auf ACK
// #define FRAME_ACKOK 16   // ACK OK TODO: Wahrscheinlich nicht gebraucht
// #define FRAME_CRCOK 32   // CRC OK TODO: Wahrscheinlich nicht gebraucht

// Methods
// TODO: Make methods static?


void HMWRS485::loop() {
// main loop, muss immer wieder aufgerufen werden
// Daten empfangen (tut nichts, wenn keine Daten vorhanden)
  receive();
  // Check
  if(frameComplete) {
	frameComplete = 0;   // only once
    if(targetAddress == txSenderAddress || targetAddress == 0xFFFFFFFF){
      hmwdebug(F("parsing from loop..."));
	  if(parseFrame())
	    module->processEvent(frameData, frameDataLength, (targetAddress == 0xFFFFFFFF));
	}
  };
}


boolean HMWRS485::parseFrame () { // returns true, if event needs to be processed by the module
// Wird aufgerufen, wenn eine komplette Nachricht empfangen wurde

  byte seqNumReceived;
  byte seqNumSent;

//### START ACK verarbeiten ###
  if(frameStatus & FRAME_SENTACKWAIT) {   // wir warten auf ACK?
    // Empfangene Sendefolgenummer muss stimmen
    seqNumReceived = (frameControlByte >> 5) & 0x03;
    seqNumSent     = (txFrameControlByte >> 1) & 0x03;
    // Absenderadresse mus stimmen
    if(seqNumReceived == seqNumSent && senderAddress == txTargetAddress) {
      // "ackwait" zuruecksetzen (ansonsten wird auf den Timeout vertraut)
      hmwdebug(F("Received ACK"));
      frameStatus &= ~FRAME_SENTACKWAIT;
    }
  }
// ### END ACK verarbeiten ###
// Muss das Modul was dazu sagen?
// Nein, wenn es nur ein ACK ist oder die Laenge der Daten 0 ist
// TODO: Wiederholungen brauchen auch nur ein ACK

// Nur ein ACK
  if((frameControlByte & 0x03) == 1) return false;
// Leere Message und kein Broadcast? -> Ein ACK senden koennen wir selbst
  if(frameDataLength == 0 && targetAddress != 0xFFFFFFFF) {
    txTargetAddress = senderAddress;
    sendAck();
    return false;
  };
// ja, das Modul sollte seine Meinung dazu sagen
  return true;
} // parseFrame


// send Frame, wait for ACK and maybe repeat
void HMWRS485::sendFrame(){
// TODO: non-blocking
// TODO: Wenn als Antwort kein reines ACK kommt, dann geht die Antwort verloren
//       D.h. sie wird nicht interpretiert. Die Gegenstelle sollte es dann nochmal
//       senden, aber das ist haesslich.

// simple send for ACKs and Broadcasts
  if(txTargetAddress == 0xFFFFFFFF || ((txFrameControlByte & 0x03) == 1)) {
	hmwdebug(F("Sending ACK or Broadcast"));
	hmwdebug(txFrameControlByte,HEX);
	sendFrameSingle();
	// TODO: nicht besonders schoen, zuerst ackwait zu setzen und dann wieder zu loeschen
	frameStatus &= ~FRAME_SENTACKWAIT; // we do not really wait
	return;
  };

  unsigned long lastTry = 0;

  for(byte i = 0; i < 3; i++) {  // maximal 3 Versuche
    sendFrameSingle();
    lastTry = millis();
// wait for ACK
    // TODO: Die Wartezeit bis die Uebertragung wiederholt wird sollte einen Zufallsanteil haben
    while(millis() - lastTry < 200) {
// Daten empfangen (tut nichts, wenn keine Daten vorhanden)
      receive();
// Check
// TODO: Was tun, wenn inzwischen ein Broadcast kommt, auf den wir reagieren muessten?
//       (Pruefen, welche das sein koennten.)
      if(frameComplete) {
        if(targetAddress == txSenderAddress){
          frameComplete = 0;
          hmwdebug(F("parsing from ackwait "));
          parseFrame();
          if(!(frameStatus & FRAME_SENTACKWAIT))  // ACK empfangen
            return;
        };
      };
    };
  };
}


// Send a whole data frame.
// The following global vars must be set
//
// hmwTxTargetAdress(4)                   the target adress
// hmwTxFrameControllByte                 the controll byte
// hmwTxSenderAdress(4)                   the sender adress
// hmwTxFrameDataLength                   the length of data to send
// hmwTxFrameData(MAX_RX_FRAME_LENGTH)    the data array to send
void HMWRS485::sendFrameSingle() {
// TODO: Optimize, do not flush() after each single character
//       Do not toggle TX pin for each char

      byte tmpByte;

      unsigned int crc16checksum = 0xFFFF;

 // TODO: Das Folgende nimmt an, dass das ACK zur letzten empfangenen Sendung gehoert
 //       Wahrscheinlich stimmt das immer oder ist egal, da die Gegenseite nicht auf
 //       ein ACK wartet. (Da man nicht kein ACK senden kann, ist jeder Wert gleich
 //       gut, wenn keins gesendet werden muss.)
      if(txTargetAddress != 0xFFFFFFFF){
        byte txSeqNum = (frameControlByte >> 1) & 0x03;
        txFrameControlByte &= 0x9F;
        txFrameControlByte |= (txSeqNum << 5);
      };

      hmwdebug(F("\nSending\n"));
      digitalWrite(txEnablePin, HIGH);
      serial->write(FRAME_START_LONG);  // send startbyte
      crc16checksum = crc16Shift(FRAME_START_LONG , crc16checksum);

      byte i;
      unsigned long address = txTargetAddress;
      for( i = 0; i < 4; i++){      // send target address
    	 tmpByte = address >> 24;
         sendFrameByte( tmpByte );
         crc16checksum = crc16Shift(tmpByte, crc16checksum);
         address = address << 8;
      };

      sendFrameByte(txFrameControlByte);                                      // send control byte
      crc16checksum = crc16Shift(txFrameControlByte , crc16checksum);

      if(bitRead(txFrameControlByte,3)){                                      // check if message has sender
    	  address = txSenderAddress;
    	  for( i = 0; i < 4; i++){                                           // send sender address
    	    	 tmpByte = address >> 24;
    	         sendFrameByte( tmpByte );
    	         crc16checksum = crc16Shift(tmpByte, crc16checksum);
    	         address = address << 8;
    	  }
      };
      tmpByte = txFrameDataLength + 2;                              // send data length
      sendFrameByte(tmpByte);
      crc16checksum = crc16Shift(tmpByte , crc16checksum);

    	for(i = 0; i < txFrameDataLength; i++){            // send data, falls was zu senden
            sendFrameByte(txFrameData[i]);
            crc16checksum = crc16Shift(txFrameData[i], crc16checksum);
    	}
      crc16checksum = crc16Shift(0 , crc16checksum);
      crc16checksum = crc16Shift(0 , crc16checksum);

      // CRC schicken
      sendFrameByte(crc16checksum / 0x100);
      sendFrameByte(crc16checksum & 0xFF);

      serial->flush();                  // othwerwise, enable pin will go low too soon
      digitalWrite(txEnablePin, LOW);

      frameStatus |= FRAME_SENTACKWAIT;
      // frameStatus &= ~FRAME_ACKOK;    TODO: Remove, if not needed
} // sendFrameSingle


// Send a data byte.
// Before sending check byte for special chars. Special chars are escaped before sending
// TX-Pin needs to be HIGH before calling this
void HMWRS485::sendFrameByte(byte sendByte) {
	// Debug
	hmwdebug(sendByte >> 4, HEX);
	hmwdebug(sendByte & 15, HEX);
	hmwdebug(":");
    if(sendByte == FRAME_START_LONG || sendByte == FRAME_START_SHORT || sendByte == ESCAPE_CHAR) {
       serial->write(ESCAPE_CHAR);
       sendByte &= 0x7F;
    };
    serial->write(sendByte);
};

// Sendet eine ACK-Nachricht
// Folgende globale Variablen MUESSEN vorher gesetzt sein:
// txTargetAdress
// txSenderAdress
void HMWRS485::sendAck() {
      txFrameControlByte = 0x19;
      txFrameDataLength = 0;
      sendFrame();
};


// calculate crc16 checksum for each byte
// TODO: Maybe simplify a bit using bitRead()
unsigned int HMWRS485::crc16Shift(byte newByte , unsigned int oldCrc) {
  unsigned int crc = oldCrc;
  byte stat;

  for (byte i = 0; i < 8; i++) {
    if (crc & 0x8000) {stat = 1;}
    else              {stat = 0;}
    crc = (crc << 1);
    if (newByte & 0x80) {
      crc = (crc | 1);
    }
    if (stat) {
      crc = (crc ^ CRC16_POLYNOM);
    }
    newByte = newByte << 1;
  }
  return crc;
}  // crc16Shift


/* int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}; */


// RS485 empfangen
// Muss zyklisch aufgerufen werden
// TODO: Als Interrupt-Routine? Dann geht's nicht mehr per SoftSerial
void HMWRS485::receive(){

  static byte rxStartByte;
  static unsigned long rxTargetAddress;
  static byte rxFrameControlByte;
  static unsigned long rxSenderAddress;
  static byte rxFrameDataLength;       // Länger der Daten + Checksum
  static byte rxFrameData[MAX_RX_FRAME_LENGTH];

  static byte addressLength;
  static byte addressLengthLong;
  static byte framePointer;
  static byte addressPointer;
  static unsigned int crc16checksum;

// TODO: Kann sich hier zu viel "anstauen", so dass das while vielleicht
//       nach ein paar Millisekunden unterbrochen werden sollte?

  while(serial->available()) {

    byte rxByte = serial->read();    // von Serial oder SoftSerial

    // Debug
    if( rxByte == 0xFD ) hmwdebug(F("\nReceiving \n"));
    hmwdebug(rxByte >> 4, HEX);
    hmwdebug(rxByte & 15, HEX);
    hmwdebug(":");

   if(rxByte == ESCAPE_CHAR && !(frameStatus & FRAME_ESCAPE)){
// TODO: Wenn frameEscape gesetzt ist, dann sind das zwei Escapes hintereinander
//       Das ist eigentlich ein Fehler -> Fehlerbehandlung
     frameStatus |= FRAME_ESCAPE;
   }else{
      if(rxByte == FRAME_START_LONG || rxByte == FRAME_START_SHORT){  // Startzeichen empfangen
         rxStartByte = rxByte;
         frameStatus |= FRAME_START;
         frameStatus &= ~FRAME_ESCAPE;
         // frameStatus &= ~FRAME_CRCOK; TODO: remove, if not needed
         framePointer = 0;
         addressPointer = 0;
         rxSenderAddress = 0;
         rxTargetAddress = 0;
         crc16checksum = crc16Shift(rxByte , 0xFFFF);
         if(rxByte == FRAME_START_LONG){    // Startzeichen 0xFD
            addressLength = 4;
            addressLengthLong = 9;
         }
#if ENABLE_FRAME_START_SHORT          // Startzeichen 0xFE
         else{
            addressLength = 1;
            addressLengthLong = 2;
         }
#endif

      }else if(frameStatus & FRAME_START) {  // Startbyte wurde zuvor gesetzt und Frame wird nicht ignoriert
         if(frameStatus & FRAME_ESCAPE) {
            rxByte |= 0x80;
            frameStatus &= ~FRAME_ESCAPE;
         };
         crc16checksum = crc16Shift(rxByte , crc16checksum);
         if(addressPointer < addressLength){  // Adressbyte Zieladresse empfangen
            rxTargetAddress <<= 8;
            rxTargetAddress |= rxByte;
            addressPointer++;
         }else if(addressPointer == addressLength){   // Controlbyte empfangen
            addressPointer++;
            rxFrameControlByte = rxByte;
         }else if( bitRead(rxFrameControlByte,3) && addressPointer < addressLengthLong) {
        	// Adressbyte Sender empfangen wenn CTRL_HAS_SENDER und FRAME_START_LONG
        	rxSenderAddress <<= 8;
        	rxSenderAddress |= rxByte;
            addressPointer++;
         }else if(addressPointer != 0xFF) { // Datenlänge empfangen
            addressPointer = 0xFF;
            rxFrameDataLength = rxByte;
         }else{                   // Daten empfangen
            rxFrameData[framePointer] = rxByte;   // Daten in Puffer speichern
            framePointer++;
            if(framePointer == rxFrameDataLength) {  // Daten komplett
               if(crc16checksum == 0) {    //
            	  frameStatus &= ~FRAME_START;
                  // frameStatus |= FRAME_CRCOK;  TODO: remove, if not needed
                  // Framedaten für die spätere Verarbeitung speichern
                  // TODO: Braucht man das wirklich?
                  //       Moeglicherweise braucht man das nur, wenn mit Interrupts gearbeitet wird
                  startByte = rxStartByte;
                  targetAddress = rxTargetAddress;
                  frameControlByte = rxFrameControlByte;
                  senderAddress = rxSenderAddress;

                  frameDataLength = rxFrameDataLength - 2;
                  memcpy(frameData, rxFrameData, frameDataLength);
                  framePointer = 0;
                  addressPointer = 0;
                  // es liegt eine neue Nachricht vor
                  frameComplete = 1;
                  // auch wenn noch Daten im Puffer sind, muessen wir erst einmal
                  // die gerade gefundene Nachricht verarbeiten
                  return;
               }else{
            	  hmwdebug(F("crc error"));
               }
            }
         }
      }
    }
  }
} // receive

