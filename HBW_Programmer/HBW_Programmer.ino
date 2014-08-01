

// Programmiergerät (normaler Arduino) über die folgenden Pins
// mit RS485-Transmitter verbinden:
#define rxPin 2
#define txPin 3
#define txEnablePin 4

#define TIMEOUT 2000
// Wichtig: Zwischen die Pins "RST" und "GND" einen 22µF Kondensator
// hängen, damit das Programmiergerät durch die Arduino IDE nicht
// geresettet wird und alle Befehle auf dem RS485-Bus übertragen werden



#include "SoftwareSerialX.h"
#include "stk500.h"

#define ENABLE_FRAME_START_SHORT 0

#define FRAME_START_SHORT 0xFE
#define FRAME_START_LONG 0xFD
#define CRC16_POLYNOM 0x1002
#define ESCAPE_CHAR 0xFC

// Empfangs-Status
#define FRAME_START 1    // Startzeichen gefunden
#define FRAME_ESCAPE 2   // Escape-Zeichen gefunden
#define FRAME_DISCOVERY 4    // Discovery erkannt
#define FRAME_SENTACKWAIT 8  // Nachricht wurde gesendet, warte auf ACK
#define FRAME_ACKOK 16   // ACK OK
#define FRAME_CRCOK 32   // CRC OK

#define MAX_RX_FRAME_LENGTH 200


byte frameComplete;
unsigned long targetAddress;
byte frameDataLength;                 // Laenge der Daten
byte frameData[MAX_RX_FRAME_LENGTH];
byte startByte;
byte frameControlByte;
unsigned long senderAddress;

// Senden
unsigned long clientID     = 0x00000000;    // ID des zu flashenden Devices
unsigned long programmerID = 0x00000002;    // ID des Programmers
unsigned long timeOutComparison;
byte txFrameControlByte;
byte txFrameDataLength;              // Laenge der Daten + Checksum
byte txFrameData[MAX_RX_FRAME_LENGTH];

byte frameStatus;
byte commandLength;
byte flashProcedureActive;
byte firstTry;
byte pingRequested;


SoftwareSerial mySerial(rxPin, txPin); // RX, TX






void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(19200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  pinMode(txEnablePin, OUTPUT);
  digitalWrite(txEnablePin, LOW);
  // set the data rate for the SoftwareSerial port
  mySerial.begin(19200);


}


void loop()
{

  Serial.println("Homematic Wired Programmer");
  Serial.println("--------------------------");


  clientID = getClientId();

  askForQuietMode();
  
  goToBootloader();

  flashProcedure();

}




void askForQuietMode()
{  
  txFrameData[0] = 0x7A;
  txFrameDataLength = 1;
  sendFrame();
  delay(100);
  sendFrame();

}



void goToBootloader()
{
  
  // Set Command to enter Bootloader-Mode
  Serial.print("Trying to enter Bootloader-Mode of Client ");
  Serial.println(clientID, HEX);
  txFrameData[0] = 0x75;
  txFrameDataLength = 1;
  sendFrame();
  
  delay(100);

  timeOutComparison = millis(); 
  pingRequested = true;
  firstTry = true;


  while(1) {
    
    if ((millis() - timeOutComparison) > TIMEOUT) {   
      pingRequested = true;
      timeOutComparison = millis();
      if (firstTry == true) {
        firstTry = false;
        Serial.println("No response from Bootloader. Please restart client to enter Bootloader...");        
      }

    }
    
    if (pingRequested) {
      // Send command to bootloader
      txFrameData[0] = STK_GET_SYNC;
      txFrameData[1] = CRC_EOP;
      txFrameDataLength = 2;
      sendFrame();
      timeOutComparison = millis(); 
      pingRequested = false;
    }
  
    // and wait for a reaction 
    receive();
    if (frameComplete)
    {
      frameComplete = 0;
      if (frameData[0] == STK_INSYNC) {
        Serial.println("Bootloader entered");
        Serial.println("Select >upload< to flash the device"); 
        return;
      }  
    }
    
  }
 
}




void flashProcedure() // run over and over
{

while(1) {
  
  // Daten von PC (Arduino IDE) einlesen
  while (Serial.available()) {

    flashProcedureActive = true;
    txFrameData[txFrameDataLength] = Serial.read();


    if (txFrameDataLength == 0) {
      /***********************************/
      /* Commands from STK500 protocoll  */
      /***********************************/
      if (txFrameData[0] == STK_GET_PARAMETER) {
         commandLength = 2;
      }
      else if (txFrameData[0] == STK_SET_DEVICE) {
        commandLength = 21;
      }
      else if (txFrameData[0] == STK_SET_DEVICE_EXT) {
        commandLength = 6;
      }
      else if (txFrameData[0] == STK_LOAD_ADDRESS) {
        commandLength = 3;
      }
      else if (txFrameData[0] == STK_UNIVERSAL) {
        commandLength = 5;
      }
      else if (txFrameData[0] == STK_PROG_PAGE) {
        commandLength = 4;
      }
      else if (txFrameData[0] == STK_READ_PAGE) {
        commandLength = 4;
      }
      else if (txFrameData[0] == STK_READ_SIGN) {
        commandLength = 1;
      }
      else if (txFrameData[0] == STK_GET_SYNC) {
        commandLength = 1;
      }

      /*******************************************/
      /* Commands for additional HM485 functions */
      /*******************************************/

      // ClientID einlesen
//      else if (txFrameData[0] == 'I') {
//        clientID = getClientId();
        // HM485-Botschaft schicken, um in Bootloader zu wechseln
        // falls keine Antwort kommt, Info, dass Client über HW-Reset in Bootloader gebracht werden muss
        // Zyklische Botschaft, um Client im Bootloader-Mode zu halten
        
//      }
/*      // Bus ausschalten
      else if (txFrameData[0] == 'I') {
        clientID = getClientId();
      }
      // ClientID ändern
      else if (txFrameData[0] == 'I') {
        clientID = getClientId();
      }
*/
      

      else {
        commandLength = 0;
      }
    }
    
    
    if ((txFrameData[0] == STK_PROG_PAGE) && (txFrameDataLength == 2)) {
      commandLength = txFrameData[2] + 4;
    }

   
    if (txFrameDataLength == commandLength) {
      txFrameDataLength++;
      sendFrame();
      txFrameDataLength = 0;
    }
    else {
      txFrameDataLength++;
    }
  }  


  
  // Send Sync Command to keep Client in Bootloader-Mode until Flash-Procedure starts  
  if ((millis() - timeOutComparison) > TIMEOUT) {
    timeOutComparison = millis();
    txFrameData[0] = STK_GET_SYNC;
    txFrameData[1] = CRC_EOP;
    txFrameDataLength = 2;
    sendFrame();
    txFrameDataLength = 0;
    flashProcedureActive = false;
  }

  

  receive();
  if(frameComplete) {
    frameComplete = 0;
    timeOutComparison = millis();

    if((targetAddress == programmerID) && (senderAddress == clientID)) {
      if (flashProcedureActive) {
        for(byte i = 0; i < frameDataLength; i++) {
          Serial.write(frameData[i]);
        }  
      }
      else {
        if (frameData[0] == STK_INSYNC) {
          Serial.write(".");
        }
      }
    }
  }
  
}  
  
}



unsigned long getClientId (void) {

  unsigned long clientId;

  Serial.print("Enter ClientId (32bit, HEX-Format, e.g. 3AE772B1): ");

  for (byte i=0; i<8; i++) {
    while (!Serial.available()) {}

    byte asc = Serial.read();

    clientId <<= 4;

    byte hex = asc - '0';
    
    if (hex > 9) {
      hex = asc - 55;
    }
    if (hex > 15) {
      hex = asc - 87;
    }

    clientId |= hex;
    
  }
  Serial.println(clientId, HEX);
  return clientId;
}




void sendFrame() {

      byte tmpByte;

      unsigned int crc16checksum = 0xFFFF;
      txFrameControlByte = 0x78;

      byte txSeqNum = (frameControlByte >> 1) & 0x03;
      txFrameControlByte &= 0x9F;
      txFrameControlByte |= (txSeqNum << 5);

      digitalWrite(txEnablePin, HIGH);
      mySerial.write(FRAME_START_LONG);  // send startbyte
      digitalWrite(txEnablePin, LOW);
      crc16checksum = crc16Shift(FRAME_START_LONG , crc16checksum);

      byte i;
      unsigned long address = clientID;
      for( i = 0; i < 4; i++){      // send target address
    	 tmpByte = address >> 24;
         sendFrameByte( tmpByte );
         crc16checksum = crc16Shift(tmpByte, crc16checksum);
         address = address << 8;
      };

      sendFrameByte(txFrameControlByte);                                      // send controll byte
      crc16checksum = crc16Shift(txFrameControlByte , crc16checksum);

      if(bitRead(txFrameControlByte,3)){                                      // check if message has sender
    	  address = programmerID;
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
void sendFrameByte(byte sendByte) {


  // "Senden" einschalten
      digitalWrite(txEnablePin, HIGH);
      if(sendByte == FRAME_START_LONG || sendByte == FRAME_START_SHORT || sendByte == ESCAPE_CHAR) {
         mySerial.write(ESCAPE_CHAR);
         sendByte &= 0x7F;
      };
      mySerial.write(sendByte);
  // "Senden" ausschalten
      digitalWrite(txEnablePin, LOW);
};




void receive(){

  static byte rxStartByte;
  static unsigned long rxTargetAddress;
  static byte rxFrameControlByte;
  static unsigned long rxSenderAddress;
  static byte rxFrameDataLength;       // L�nger der Daten + Checksum
  static byte rxFrameData[MAX_RX_FRAME_LENGTH];

  static byte addressLength;
  static byte addressLengthLong;
  static byte framePointer;
  static byte addressPointer;
  static unsigned int crc16checksum;

// TODO: Kann sich hier zu viel "anstauen", so dass das while vielleicht
//       nach ein paar Millisekunden unterbrochen werden sollte?

  while(mySerial.available()) {

    byte rxByte = mySerial.read();    // von Serial oder SoftSerial

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
         frameStatus &= ~FRAME_DISCOVERY;
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
            if ((rxFrameControlByte & 0b00000011) == 0b00000011){
              frameStatus |= FRAME_DISCOVERY;
            }
         }else if( bitRead(rxFrameControlByte,3) && (addressPointer < addressLengthLong) && !(frameStatus & FRAME_DISCOVERY)) {
        	// Adressbyte Sender empfangen wenn CTRL_HAS_SENDER und FRAME_START_LONG
                // und kein Discovery angefordert ist (ControlByte = 0bXXXXXX11
        	rxSenderAddress <<= 8;
        	rxSenderAddress |= rxByte;
            addressPointer++;
         }else if(addressPointer != 0xFF) { // Datenl�nge empfangen
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
                  // Framedaten f�r die sp�tere Verarbeitung speichern
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

                  if(frameStatus & FRAME_DISCOVERY ) {
                    // Discovery Mode?
                    // Abfrage könnte auch in HMWHomebrew erfolgen (innerhalb der "hmwrs485.frameComplete-Abfrage")
                    // Aufruf müsste dann nicht aus dieser Funktion heraus erfolgen / public Variable notwendig
                    // processDiscovery();   // kein Discovery im Bootloader       
                  }

               }else{
                 Serial.print("CRC-Error - ");
                 Serial.write(crc16checksum >> 8);
                 Serial.write(crc16checksum);

               }
            }
         }
      }
    }
  }

  
} // receive








// calculate crc16 checksum for each byte
// TODO: Maybe simplify a bit using bitRead()
unsigned int crc16Shift(byte newByte , unsigned int oldCrc) {
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

