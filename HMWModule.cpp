/*
 * HMWModule.cpp
 *
 *  Created on: 09.05.2014
 *      Author: Thorsten Pferdekaemper (thorsten@pferdekaemper.com)
 *      Nach einer Vorlage von Dirk Hoffmann (dirk@hfma.de) 2012
 *
 *  Generelle Kommandos/Funktionen fuer Homematic Wired Module
 */

#include "HMWModule.h"

#include <EEPROM.h>

HMWModule::HMWModule(HMWDeviceBase* _device, HMWRS485* _hmwrs485, byte _deviceType, char* _deviceSerial, unsigned long _deviceAddress ) {
  device = _device;
  hmwrs485 = _hmwrs485;
  deviceType = _deviceType;
  // TODO: Keine Ahnung, ob es genau 10 Zeichen sein muessen
  memcpy(deviceSerial, _deviceSerial, 10);
  // TODO: Entscheiden, ob die device address zum Protokoll oder zum Modul gehoert
  hmwrs485->txSenderAddress = _deviceAddress;
}

HMWModule::~HMWModule() {
	// TODO Auto-generated destructor stub
}

// Processing of default events (related to all modules)
void HMWModule::processEvents() {
      unsigned int adrStart;
      byte sendAck;

      // ACKs werden nicht prozessiert
      // TODO: Das gehoert eigentlich ins Protokll eine Schicht tiefer
      if((hmwrs485->frameControlByte & 0x03) == 0x01) return;

      // Wenn irgendwas von Broadcast kommt, dann gibt es darauf keine
      // Reaktion, ausser z und Z (und es kommt von der Zentrale)
      // TODO: Muessen wir pruefen, ob's wirklich von der Zentrale kommt?
      if(hmwrs485->targetAddress == 0xFFFFFFFF){
    	switch(hmwrs485->frameData[0]){
    	  case 'Z':                                                               // End discovery mode
    	    // reset hmwModuleDiscovering
    	    // TODO: Discovery mode
    	    break;
    	  case 'z':                                                               // start discovery mode
    	    // set hmwModuleDiscovering
    	    // TODO: Discovery mode
    	    break;
    	  }
    	  return;
      };

      hmwrs485->txTargetAddress = hmwrs485->senderAddress;
      // gibt es was zu verarbeiten?
      // wenn nicht, dann einfach ein ACK schicken
      if(hmwrs485->frameDataLength <= 0){
    	  hmwrs485->sendAck();
    	  return;
      }

      hmwrs485->txFrameControlByte = 0x78;

      sendAck = 1;
      switch(hmwrs485->frameData[0]){
         case '!':                                                             // reset the Module
            // reset the Module jump after the bootloader
        	// Nur wenn das zweite Zeichen auch ein "!" ist
        	// TODO: Wirklich machen, aber wie geht das?
            if(hmwrs485->frameData[1] == '!') { };   //  then goto 0
            break;
         case 'A':                                                             // Announce
            sendAck = 2;
            break;
         case 'C':                                                              // re read Config
        	device->readConfig();           // callback to device
            break;
         case 'E':                                                              // ???
            // TODO: Der E-Befehl wird von der Zentrale beim Pairing geschickt
        	//       ganz klar ist aber nicht, was er macht. Bisher kam als
        	//       Daten immer 0x00001040 mit und das wurde von einem echten
        	//       Device mit einem e und 00 00 10 1F 00 00 00 00 00 60 00 beantwortet
        	//       Einfach mal nachmachen, aber 100% richtig ist das wahrscheinlich nicht
        	sendAck = 0;
        	hmwrs485->txFrameDataLength = 12;
        	hmwrs485->txFrameData[0] = 0x65;  //e
        	hmwrs485->txFrameData[1] = 0x00;
        	hmwrs485->txFrameData[2] = 0x00;
        	hmwrs485->txFrameData[3] = 0x10;
        	hmwrs485->txFrameData[4] = 0x1F;
        	hmwrs485->txFrameData[5] = 0x00;
        	hmwrs485->txFrameData[6] = 0x00;
        	hmwrs485->txFrameData[7] = 0x00;
        	hmwrs485->txFrameData[8] = 0x00;
        	hmwrs485->txFrameData[9] = 0x00;
        	hmwrs485->txFrameData[10] = 0x60;
        	hmwrs485->txFrameData[11] = 0x00;
            break;
         case 'K':                                                              // Key-Event
            processEventKey();
            sendAck = 2;
            break;
         case 'R':                                                              // Read EEPROM
            if(hmwrs485->frameDataLength == 4) {                                // Length of before incoming data must be 4
               sendAck = 0;
               hmwrs485->debug("read eeprom");
               adrStart = ((unsigned int)(hmwrs485->frameData[1]) << 8) | hmwrs485->frameData[2];  // start adress of eeprom
               for(byte i = 0; i < hmwrs485->frameData[3]; i++) {
            	   hmwrs485->txFrameData[i] = EEPROM.read(adrStart + i);
               };
               hmwrs485->txFrameDataLength = hmwrs485->frameData[3];
            };
            break;
         case 'S':                                                               // GET Level
            processEventGetLevel();
            sendAck = 2;
            break;
         case 'W':                                                               // Write EEPROM
            if(hmwrs485->frameDataLength == hmwrs485->frameData[3] + 4) {
               hmwrs485->debug("write eeprom");
               adrStart = ((unsigned int)(hmwrs485->frameData[1]) << 8) | hmwrs485->frameData[2];  // start adress of eeprom
               for(byte i = 4; i < hmwrs485->frameDataLength; i++){
            	 EEPROM.write(adrStart+i-4, hmwrs485->frameData[i]);
               }
            };
            break;

         case 'c':                                                               // Zieladresse löschen?
            // TODO: ???
            break;

         case 'h':                                                               // get Module type and hardware version
         	hmwrs485->debug("Hardware Version and Type");
            sendAck = 0;
            hmwrs485->txFrameData[0] = deviceType;
            hmwrs485->txFrameData[1] = MODULE_HARDWARE_VERSION;
            hmwrs485->txFrameDataLength = 2;
            break;
         case 'l':                                                               // set Lock
            processEventSetLock();
            break;
         case 'n':                                       // Seriennummer
        	memcpy(hmwrs485->txFrameData, deviceSerial,10);
        	hmwrs485->txFrameDataLength = 10;
        	sendAck = 0;
        	break;
         case 'q':                                                               // Zieladresse hinzufügen?
            // TODO: ???
        	break;
         case 's':                                                               // Aktor setzen
            processEventSetLevel();
            break;
         case 'u':                                                              // Update (Bootloader starten)
            // Bootloader neu starten
            // Goto $7c00                                                          ' Adresse des Bootloaders
        	// TODO: Bootloader?
        	break;
         case 'v':                                                               // get firmware version
        	hmwrs485->debug("Firmware Version");
            sendAck = 0;
            hmwrs485->txFrameData[0] = MODULE_FIRMWARE_VERSION / 0x100;
            hmwrs485->txFrameData[1] = MODULE_FIRMWARE_VERSION & 0xFF;
            hmwrs485->txFrameDataLength = 2;
            break;
         case 'x':                                                               // Level set
            processEventSetLevel();                                               // Install-Test TODO: ???
            sendAck = 2;
            break;

         case 'Ë':                                                               // Key-Sim-Event
            processEventKey();
            sendAck = 2;
            break;
      }

      if(sendAck == 2){
         // prepare info Frame
    	 hmwrs485->txFrameData[0] = 'i';
         sendAck = 0;
      };
      if(sendAck == 0){
    	  hmwrs485->sendFrame();
      }else{
    	  hmwrs485->sendAck();
      };
};

void HMWModule::processEventKey(){
	// TODO
};

   void HMWModule::processEventSetLevel(){
	 // tell the hardware
     device->setLevel(hmwrs485->frameData[1], hmwrs485->frameData[2]);
     // get what the hardware did and send it back
     hmwrs485->txFrameDataLength = 0x04;      // Length
     hmwrs485->txFrameData[0] = 0x69;         // 'i'
     hmwrs485->txFrameData[1] = hmwrs485->frameData[1];      // Sensornummer
     unsigned int info = device->getLevel(hmwrs485->frameData[1]);
     hmwrs485->txFrameData[2] = info / 0x100;
     hmwrs485->txFrameData[3] = info & 0xFF;
   };

   void HMWModule::processEventGetLevel(){
		// TODO
   };

   void HMWModule::processEventSetLock(){
		// TODO
   };

   // "Announce-Message" ueber broadcast senden
   void HMWModule::broadcastAnnounce(byte channel) {
      hmwrs485->txTargetAddress = 0xFFFFFFFF;  // broadcast
      hmwrs485->txFrameControlByte = 0xF8;     // control byte
      hmwrs485->txFrameDataLength = 16;      // Length
      hmwrs485->txFrameData[0] = 0x41;         // 'i'
      hmwrs485->txFrameData[1] = channel;      // Sensornummer
      hmwrs485->txFrameData[2] = deviceType;
      hmwrs485->txFrameData[3] = 0; // MODULE_HARDWARE_VERSION;
      hmwrs485->txFrameData[4] = MODULE_FIRMWARE_VERSION / 0x100;
      hmwrs485->txFrameData[5] = MODULE_FIRMWARE_VERSION & 0xFF;
      memcpy(&hmwrs485->txFrameData[6], deviceSerial, 10);
      hmwrs485->sendFrame();
   };

   // "Key Pressed" ueber broadcast senden
   void HMWModule::broadcastKeyEvent(byte channel, byte keyPressNum, byte longPress) {
	   hmwrs485->txTargetAddress = 0xFFFFFFFF;  // broadcast
	   hmwrs485->txFrameControlByte = 0xF8;     // control byte
	   hmwrs485->txFrameDataLength = 0x04;      // Length
	   hmwrs485->txFrameData[0] = 0x4B;         // 'K'
	   hmwrs485->txFrameData[1] = channel;      // Sensornummer
	   hmwrs485->txFrameData[2] = 0;            // Zielaktor
	   // TODO: Counter
	   hmwrs485->txFrameData[3] = (longPress ? 3 : 2) + (keyPressNum << 2);
	   hmwrs485->sendFrame();
   };

   // "i-Message" ueber broadcast senden
     void HMWModule::broadcastInfoMessage(byte channel, unsigned int info) {
  	   hmwrs485->txTargetAddress = 0xFFFFFFFF;  // broadcast
  	   hmwrs485->txFrameControlByte = 0xF8;     // control byte
  	   hmwrs485->txFrameDataLength = 0x04;      // Length
  	   hmwrs485->txFrameData[0] = 0x69;         // 'i'
  	   hmwrs485->txFrameData[1] = channel;      // Sensornummer
  	   hmwrs485->txFrameData[2] = info / 0x100;
  	   hmwrs485->txFrameData[3] = info & 0xFF;
  	   hmwrs485->sendFrame();
     };
