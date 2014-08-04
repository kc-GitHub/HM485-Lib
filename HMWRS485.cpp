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
//   $baud = 19200
//   config Com1 = Dummy , Synchrone = 0 , Parity = even , Stopbits = 1 , Databits = 8 , Clockpol = 0

//'----[ Config UART ]------------------------------------------------------------
//   On Urxc isrUrxc                                                              ' Interrupt-Routine setzen
//   Enable Urxc                                                                  ' Interrupt URXC einschalten

// Defines
// TODO: In Protokollbeschreibung nachsehen und entsprechend kommentieren
#define ENABLE_FRAME_START_SHORT 0

#define FRAME_START_SHORT 0xFE
#define FRAME_START_LONG 0xFD
#define CRC16_POLYNOM 0x1002
#define ESCAPE_CHAR 0xFC

// Empfangs-Status
#define FRAME_START 1    // Startzeichen gefunden
#define FRAME_ESCAPE 2   // Escape-Zeichen gefunden
#define FRAME_SENTACKWAIT 8  // Nachricht wurde gesendet, warte auf ACK
#define FRAME_ACKOK 16   // ACK OK
#define FRAME_CRCOK 32   // CRC OK


// Methods
// TODO: Make methods static?
// TODO: methods in .h file

void HMWRS485::parseFrame () {
// TODO: Dass die Nachricht an uns geht sollte ggf im Protokoll geprüft werden

  byte txSeqNum;
  byte foundCtrl;
  byte foundTxSeqNum;

      frameComplete = 0;

//### START ACK verarbeiten ###
      txSeqNum = (frameControlByte >> 1) & 0x03;

      if(startByte == FRAME_START_LONG){
         if(frameStatus & FRAME_SENTACKWAIT) {
            foundCtrl = findCtrlByAddress(senderAddress);
            // foundTxSeqNum
            foundTxSeqNum = (foundCtrl >> 1) & 0x03;
            if(foundCtrl > 0 && txSeqNum == foundTxSeqNum){
               // ACK Nachricht verarbeiten
               resetAckwaitByAddress(senderAddress);
            }
         }
      }else{
         targetAddress = 0;
         senderAddress = 0;
         if(frameStatus & FRAME_SENTACKWAIT){
        	 foundCtrl = findCtrlByAddress(senderAddress);  // TODO: ??? senderAddress ist 0
        	 // foundTxSeqNum
        	 foundTxSeqNum = (foundCtrl >> 1) & 0x03;
            if(!bitRead(foundCtrl,3) && txSeqNum == foundTxSeqNum){
            	// ACK Nachricht verarbeiten
               resetAckwaitByAddress(senderAddress);   // TODO: ist 0...
            }
         }
      };
// ### END ACK verarbeiten ###

      // TODO: Wird das gebraucht?
      // if(!bitRead(foundCtrl,0) && targetAddress < 0xFF) {
      //   sendAck(txSeqNum);    // IFRAME received with receiver address below 0xff -> ACK it
      // };
   } // parseFrame


// sucht das CTRL Byte der letzten Sendung in der Adresstabelle
// address: Sender-Adresse
// return hmwWaitAckSeqNum of pending ackwaits
byte HMWRS485::findCtrlByAddress(unsigned long address) {
  for(byte i = 0; i < MAX_DEVICE_RELATIONS; i++){
    if(address == waitAckAddress[i]) {
      return waitAckSeqNum[i];
    }
  }
  return 0;
}


// setzt das Ackwait-Flag für eine Adresse zurück
// address: Addresse des "Partners"
void HMWRS485::resetAckwaitByAddress(unsigned long address) {
      byte remainAckwait = 0;

      for(byte i = 0; i < MAX_DEVICE_RELATIONS; i++){
         if(address == waitAckAddress[i]){
             waitAckAddress[i] = 0;
             waitAckSeqNum[i] = 0;
         }else{
// TODO: Ist das wirklich korrekt? Eigentlich warten wir ja nur auf die, bei
//       denen auch eine Adresse eingetragen ist
           remainAckwait++;
         }
      };

      // TODO: was genau soll das?
      if(remainAckwait > 0){
         frameStatus |= FRAME_SENTACKWAIT;
      }else{
         frameStatus &= ~FRAME_SENTACKWAIT;
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
void HMWRS485::sendFrame() {

      byte tmpByte;

// TODO
//      if hmwTxFrameControllByte.0 = 1 AND hmwTxFrameControllByte.1 = 1 AND hmwTxFrameControllByte.2 = 0 then
//         ' we do not send discovery messages
//         exit sub
//      end if

      unsigned int crc16checksum = 0xFFFF;

 // TODO: Das Folgende nimmt an, dass das ACK zur letzten empfangenen Sendung gehoert
      if(txTargetAddress != 0xFFFFFFFF){
        byte txSeqNum = (frameControlByte >> 1) & 0x03;
        txFrameControlByte &= 0x9F;
        txFrameControlByte |= (txSeqNum << 5);
      };

      hmwdebug(F("\nSending\n"));
      digitalWrite(txEnablePin, HIGH);
      serial->write(FRAME_START_LONG);  // send startbyte
      serial->flush();                  // othwerwise, enable pin will go low too soon
      digitalWrite(txEnablePin, LOW);
      crc16checksum = crc16Shift(FRAME_START_LONG , crc16checksum);

      byte i;
      unsigned long address = txTargetAddress;
      for( i = 0; i < 4; i++){      // send target address
    	 tmpByte = address >> 24;
         sendFrameByte( tmpByte );
         crc16checksum = crc16Shift(tmpByte, crc16checksum);
         address = address << 8;
      };

      sendFrameByte(txFrameControlByte);                                      // send controll byte
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
      crc16checksum = crc16Shift(0 , crc16checksum);  // TODO: ???
      crc16checksum = crc16Shift(0 , crc16checksum);

      // CRC schicken
      sendFrameByte(crc16checksum / 0x100);   // TODO: High byte first? Ist das auch bei Adressen so?
      sendFrameByte(crc16checksum & 0xFF);

      frameStatus |= FRAME_SENTACKWAIT;
      frameStatus &= ~FRAME_ACKOK;
} // sendFrame


// Send a data byte.
// Before sending check byte for special chars. Special chars are escaped before sending
void HMWRS485::sendFrameByte(byte sendByte) {
	   // Debug
	hmwdebug(sendByte, HEX);

  // "Senden" einschalten
      digitalWrite(txEnablePin, HIGH);
      if(sendByte == FRAME_START_LONG || sendByte == FRAME_START_SHORT || sendByte == ESCAPE_CHAR) {
         serial->write(ESCAPE_CHAR);
         sendByte &= 0x7F;
      };
      serial->write(sendByte);
  // "Senden" ausschalten
      serial->flush();                  // othwerwise, enable pin will go low too soon
      digitalWrite(txEnablePin, LOW);
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
    hmwdebug(rxByte, HEX);

   if(rxByte == ESCAPE_CHAR && !(frameStatus & FRAME_ESCAPE)){
// TODO: Wenn frameEscape gesetzt ist, dann sind das zwei Escapes hintereinander
//       Das ist eigentlich ein Fehler -> Fehlerbehandlung
     frameStatus |= FRAME_ESCAPE;
   }else{
      if(rxByte == FRAME_START_LONG || rxByte == FRAME_START_SHORT){  // Startzeichen empfangen
         rxStartByte = rxByte;
         frameStatus |= FRAME_START;
         frameStatus &= ~FRAME_ESCAPE;
         frameStatus &= ~FRAME_CRCOK;
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
// TODO: Fehlt da die Checksumme, oder ist rxFrameDataLength sowieso inklusive CRC?
//               crc16Register = crc16Shift(0 , crc16Register)
//               crc16Register = crc16Shift(0 , crc16Register)
               if(crc16checksum == 0) {    //
            	  frameStatus &= ~FRAME_START;
                  frameStatus |= FRAME_CRCOK;
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
               }else{
            	  hmwdebug(F("crc error"));
               }
            }
         }
      }
    }
  }
} // receive

