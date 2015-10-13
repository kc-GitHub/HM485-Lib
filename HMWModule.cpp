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
#include "HMWDebug.h"

#include <EEPROM.h>

HMWModule::HMWModule(HMWDeviceBase* _device, HMWRS485* _hmwrs485, byte _deviceType) {
  device = _device;
  hmwrs485 = _hmwrs485;
  hmwrs485->module = this;   // fuer callbacks von Kommunikationsschicht
  deviceType = _deviceType;
  // TODO: Entscheiden, ob die device address zum Protokoll oder zum Modul gehoert
  readAddressFromEEPROM();
}

HMWModule::~HMWModule() {
	// TODO Auto-generated destructor stub
}

// Processing of default events (related to all modules)
void HMWModule::processEvent(byte const * const frameData, byte frameDataLength, boolean isBroadcast) {
      uint16_t adrStart;
      byte sendAck;

      // ACKs kommen hier nicht an, werden eine Schicht tiefer abgefangen

      // Wenn irgendwas von Broadcast kommt, dann gibt es darauf keine
      // Reaktion, ausser z und Z (und es kommt von der Zentrale)
      // TODO: Muessen wir pruefen, ob's wirklich von der Zentrale kommt?
      if(isBroadcast){
    	switch(frameData[0]){
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
      // gibt es was zu verarbeiten -> Ja, die Kommunikationsschicht laesst nur Messages durch,
      // die auch Daten enthalten

      // TODO: Das frame control byte sollte komplett unter Kontrolle von HMWRS485 sein
      hmwrs485->txFrameControlByte = 0x78;

      sendAck = 1;
      switch(frameData[0]){
         case '@':                                      // HBW-specifics
           if(frameData[1] == 'a' && frameDataLength == 6) {  // "a" -> address set
        	 // TODO: Avoid "central" addresses (like 0000...)
        	 for(byte i = 0; i < 4; i++)
        	   writeEEPROM(E2END - 3 + i, frameData[i + 2], true);
           };
           readAddressFromEEPROM();
           break;
         case '!':                                                             // reset the Module
            // reset the Module jump after the bootloader
        	// Nur wenn das zweite Zeichen auch ein "!" ist
        	// TODO: Wirklich machen, aber wie geht das?
            if(frameData[1] == '!') { };   //  then goto 0
            break;
         case 'A':                                                             // Announce
            sendAck = 2;
            break;
         case 'C':                                                              // re read Config
        	device->readConfig();           // callback to device
            break;
         case 'E':                           // see separate docs
        	sendAck = 0;
        	processEmessage(frameData);
            break;
         case 'K':                                                              // Key-Event
            processEventKey();
            sendAck = 2;
            break;
         case 'R':                                                              // Read EEPROM
        	// TODO: Check requested length...
            if(frameDataLength == 4) {                                // Length of before incoming data must be 4
               sendAck = 0;
               hmwdebug(F("read eeprom"));
               adrStart = ((uint16_t)(frameData[1]) << 8) | frameData[2];  // start adress of eeprom
               for(byte i = 0; i < frameData[3]; i++) {
            	   hmwrs485->txFrameData[i] = EEPROM.read(adrStart + i);
               };
               hmwrs485->txFrameDataLength = frameData[3];
            };
            break;
         case 'S':                                                               // GET Level
            processEventGetLevel(frameData[1], frameData[0]);
            sendAck = 2;
            break;
         case 'W':                                                               // Write EEPROM
            if(frameDataLength == frameData[3] + 4) {
            	hmwdebug(F("write eeprom"));
               adrStart = ((uint16_t)(frameData[1]) << 8) | frameData[2];  // start adress of eeprom
               for(byte i = 4; i < frameDataLength; i++){
            	 writeEEPROM(adrStart+i-4, frameData[i]);
               }
            };
            break;

         case 'c':                                                               // Zieladresse l�schen?
            // TODO: ???
            break;

         case 'h':                                                               // get Module type and hardware version
        	hmwdebug(F("Hardware Version and Type"));
            sendAck = 0;
            hmwrs485->txFrameData[0] = deviceType;
            hmwrs485->txFrameData[1] = MODULE_HARDWARE_VERSION;
            hmwrs485->txFrameDataLength = 2;
            break;
         case 'l':                                                               // set Lock
            processEventSetLock();
            break;
         case 'n':                                       // Seriennummer
        	determineSerial(hmwrs485->txFrameData);
        	hmwrs485->txFrameDataLength = 10;
        	sendAck = 0;
        	break;
         case 'q':                                                               // Zieladresse hinzuf�gen?
            // TODO: ???
        	break;
         // case 's':  // 's' (0x73) ist dasselbe wie 'x' (0x78), daher weiter unten implementiert
        	           // Das widerspricht zwar der Protokoll-Beschreibung, aber bisher haben alle
        	           // Original-HM-Geraete so reagiert
         case 'u':                                                              // Update (Bootloader starten)
            // Bootloader neu starten
            // Goto $7c00                                                          ' Adresse des Bootloaders
        	// TODO: Bootloader?
        	break;
         case 'v':                                                               // get firmware version
            hmwdebug(F("Firmware Version"));
            sendAck = 0;
            hmwrs485->txFrameData[0] = MODULE_FIRMWARE_VERSION / 0x100;
            hmwrs485->txFrameData[1] = MODULE_FIRMWARE_VERSION & 0xFF;
            hmwrs485->txFrameDataLength = 2;
            break;
         case 's':   // level set
         case 'x':   // Level set. In der Protokollbeschreibung steht hier was von "install test",
        	         // aber es sieht so aus, als ob 0x73 und 0x78 dasselbe tun
            processEventSetLevel(frameData[1], frameDataLength-2, &(frameData[2]));
            // get what the hardware did and send it back
            processEventGetLevel(frameData[1], frameData[0]);  // for feedback
            sendAck = 2;
            break;

         case '�':                                                               // Key-Sim-Event
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

   void HMWModule::processEventSetLevel(byte channel, byte dataLength, byte const * const data){
	 // tell the hardware
	 // deprecated, only for compatibility
     device->setLevel(channel, data[0]);
     // now the real stuff
     device->setLevel(channel, dataLength, data);
   };


   void HMWModule::processEventGetLevel(byte channel, byte command){
	 // get value from the hardware and send it back
	 hmwrs485->txFrameDataLength = 0x04;      // Length
	 hmwrs485->txFrameData[0] = 0x69;         // 'i'
	 hmwrs485->txFrameData[1] = channel;      // Sensornummer
	 uint16_t info = device->getLevel(channel, command);
	 hmwrs485->txFrameData[2] = info / 0x100;
	 hmwrs485->txFrameData[3] = info & 0xFF;
   };


   void HMWModule::processEventSetLock(){
		// TODO
   };


   void HMWModule::processEmessage(byte const * const frameData) {
	 // process E-Message

     byte blocksize = frameData[3];
     byte blocknum  = frameData[4];

     // length of response
     hmwrs485->txFrameDataLength = 4 + blocknum / 8;
     // care for odd block numbers
     if(blocknum % 8) hmwrs485->txFrameDataLength++;
     // we don't need to check the size as it can maximum
     // be 4 + 255 div 8 + 1 = 36
     // init to zero, mainly because we need it later
     memset(hmwrs485->txFrameData,0,hmwrs485->txFrameDataLength);
     // first byte "e" - answer on "E"
     hmwrs485->txFrameData[0]  = 0x65;  //e
     // next 3 bytes are just repeated from request
     hmwrs485->txFrameData[1]  = frameData[1];
     hmwrs485->txFrameData[2]  = frameData[2];
     hmwrs485->txFrameData[3]  = frameData[3];

     // determine whether blocks are used
     for(int block = 0; block <= blocknum; block++) {
       // check this memory block
       for(int byteIdx = 0; byteIdx < blocksize; byteIdx++) {
    	 if(EEPROM.read(block * blocksize + byteIdx) != 0xFF) {
    	   bitSet(hmwrs485->txFrameData[4 + block / 8], block % 8);
    	   break;
    	 }
       }
     };
   };


   // "Announce-Message" ueber broadcast senden
   byte HMWModule::broadcastAnnounce(byte channel) {
      hmwrs485->txTargetAddress = 0xFFFFFFFF;  // broadcast
      hmwrs485->txFrameControlByte = 0xF8;     // control byte
      hmwrs485->txFrameDataLength = 16;      // Length
      hmwrs485->txFrameData[0] = 0x41;         // 'i'
      hmwrs485->txFrameData[1] = channel;      // Sensornummer
      hmwrs485->txFrameData[2] = deviceType;
      hmwrs485->txFrameData[3] = 0; // MODULE_HARDWARE_VERSION;
      hmwrs485->txFrameData[4] = MODULE_FIRMWARE_VERSION / 0x100;
      hmwrs485->txFrameData[5] = MODULE_FIRMWARE_VERSION & 0xFF;
      determineSerial(hmwrs485->txFrameData + 6);
      return hmwrs485->sendFrame(true);  // only if bus is free
   };

   // "Key Pressed" ueber broadcast senden
   byte HMWModule::broadcastKeyEvent(byte channel, byte keyPressNum, byte longPress) {
	   hmwrs485->txTargetAddress = 0xFFFFFFFF;  // broadcast
	   hmwrs485->txFrameControlByte = 0xF8;     // control byte
	   hmwrs485->txFrameDataLength = 0x04;      // Length
	   hmwrs485->txFrameData[0] = 0x4B;         // 'K'
	   hmwrs485->txFrameData[1] = channel;      // Sensornummer
	   hmwrs485->txFrameData[2] = targetchannel;            // Zielaktor
	   // TODO: Counter
	   hmwrs485->txFrameData[3] = (longPress ? 3 : 2) + (keyPressNum << 2);
	   return hmwrs485->sendFrame(true);  // only if bus is free
   };

   // "i-Message" senden
   // this is only called from "outside" and not as a response
     byte HMWModule::sendInfoMessage(byte channel, uint16_t info, uint32_t target_address) {
  	   hmwrs485->txTargetAddress = target_address;  // normally central or broadcast
  	   hmwrs485->txFrameControlByte = 0xF8;     // control byte
  	   hmwrs485->txFrameDataLength = 0x04;      // Length
  	   hmwrs485->txFrameData[0] = 0x69;         // 'i'
  	   hmwrs485->txFrameData[1] = channel;      // Sensornummer
  	   hmwrs485->txFrameData[2] = info / 0x100;
  	   hmwrs485->txFrameData[3] = info & 0xFF;
  	   return hmwrs485->sendFrame(true);  // only if bus is free
     };


     void HMWModule::writeEEPROM(int16_t address, byte value, bool privileged ) {
       // save uppermost 4 bytes
       if(!privileged && (address > E2END - 4))
    	 return;
       // write if not anyway the same value
   	   if(value != EEPROM.read(address))
   		 EEPROM.write(address, value);
     };


     // read device address from EEPROM
     // TODO: Avoid "central" addresses (like 0000...)
     void HMWModule::readAddressFromEEPROM(){
       uint32_t address = 0;

       for(byte i = 0; i < 4; i++){
    	   address <<= 8;
    	   address |= EEPROM.read(E2END - 3 + i);
       }
       if(address == 0xFFFFFFFF)
    	   address = 0x42FFFFFF;
       hmwrs485->setOwnAddress(address);
     }


     void HMWModule::determineSerial(byte* buf) {
       char numAsStr[20];
       sprintf(numAsStr, "%07lu", hmwrs485->getOwnAddress() % 10000000L );
       buf[0] = 'H';
       buf[1] = 'B';
       buf[2] = 'W';
       memcpy(buf+3, numAsStr, 7);
     };
