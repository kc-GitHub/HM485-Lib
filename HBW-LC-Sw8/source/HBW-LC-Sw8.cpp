//*******************************************************************
//
// HBW-LC-Sw8.cpp
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HBW-LC-Sw8
//
//*******************************************************************


/**********************
** History           **
***********************

  v2
  - Datentyp bei KeyEvent geändert - Tastendruck bei Peering kommt jetzt an
  v3
  - getlevel Funktion korrigiert

*/


#define DEVICE_ID 0x83
/********************************/
/* Pinbelegung: 				*/
/********************************/

#define BUTTON 8            // Das Bedienelement // Belegung wie "Universalsensor"
#define RS485_TXEN 2        // Transmit-Enable	 // Belegung wie "Universalsensor"
#define Out1 A0
#define Out2 A1
#define Out3 A2
#define Out4 A3
#define Out5 A4
#define Out6 A5
#define Out7 3
#define Out8 7




#define DEBUG_NONE 0   // Kein Debug-Ausgang, RS485 auf Hardware-Serial
#define DEBUG_UNO 1    // Hardware-Serial ist Debug-Ausgang, RS485 per Soft auf pins 5/6
#define DEBUG_UNIV 2   // Hardware-Serial ist RS485, Debug per Soft auf pins 5/6

#define DEBUG_VERSION DEBUG_NONE

#if DEBUG_VERSION == DEBUG_UNO
  #define RS485_RXD 5
  #define RS485_TXD 6
  #define LED 13        // Signal-LED
#elif DEBUG_VERSION == DEBUG_UNIV
  #define DEBUG_RXD 5	// Softwareserial // Belegung wie "Universalsensor"
  #define DEBUG_TXD 6	// Softwareserial // Belegung wie "Universalsensor"
  #define LED 4         // Signal-LED // Belegung wie "Universalsensor"
#else
  #define LED 4
#endif




// Do not remove the include below
#include "HBW-LC-Sw8.h"

#if DEBUG_VERSION == DEBUG_UNO || DEBUG_VERSION == DEBUG_UNIV
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>
#endif

// debug-Funktion
#include "HMWDebug.h"

// OneWire
//#include <OneWire.h>

// EEPROM
#include <EEPROM.h>

// HM Wired Protokoll
#include "HMWRS485.h"

// default module methods
#include "HMWModule.h"
#include "HMWRegister.h"




#define CHANNEL_IO_COUNT 8
#define CHANNEL_IO_BYTES 1 //CHANNEL_IO_COUNT / 8
// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define CHANNEL_PORTS byte channelPorts[CHANNEL_IO_COUNT] = {Out1, Out2, Out3, Out4, Out5, Out6, Out7, Out8};
// Port Status, d.h. Port ist auf 0 oder 1
byte portStatus[CHANNEL_IO_BYTES];





#if DEBUG_VERSION == DEBUG_UNO
	SoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
	HMWRS485 hmwrs485(&rs485, RS485_TXEN);
#elif DEBUG_VERSION == DEBUG_UNIV
	HMWRS485 hmwrs485(&Serial, RS485_TXEN);
#else
	HMWRS485 hmwrs485(&Serial, RS485_TXEN);  // keine Debug-Ausgaben
#endif
HMWModule* hmwmodule;   // wird in setup initialisiert


hmw_config config;


// write config to EEPROM in a hopefully smart way
void writeConfig(){
//    byte* ptr;
//    byte data;
	// EEPROM lesen und schreiben
//	ptr = (byte*)(sensors);
//	for(int address = 0; address < sizeof(sensors[0]) * CHANNEL_IO_COUNT; address++){
//	  hmwmodule->writeEEPROM(address + 0x10, *ptr);
//	  ptr++;
//  };
};




// Read all inputs/outputs
// setzt Bits in portStatus[]
void readPins() {
CHANNEL_PORTS
	for(byte i = 0; i < CHANNEL_IO_COUNT; i++){
		// Pin lesen und Bit in portStatus setzen
		// TODO: Check if this really works
		bitWrite(portStatus[i/8],i%8,digitalRead(channelPorts[i]));
	}
}




// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:

	void setLevel(byte channel,unsigned int level) {
		// everything in the right limits?
		if(channel >= CHANNEL_IO_COUNT) return;
		if(level > 255) return;
		// now set pin
		CHANNEL_PORTS   // create array 'channelPorts'
		if(level == 0xFF) { // toggle
			digitalWrite(channelPorts[channel], !digitalRead(channelPorts[channel]));
		}else if(level) // on
			digitalWrite(channelPorts[channel], HIGH ^ config.switches[channel].invert);
		else
			digitalWrite(channelPorts[channel], LOW  ^ config.switches[channel].invert);

	}


	byte getLevel(byte channel) {
		// everything in the right limits?
		if(channel >= CHANNEL_IO_COUNT) return 0;
		// read
		CHANNEL_PORTS   // create array 'channelPorts'
		if(digitalRead(channelPorts[channel]) ^ config.switches[channel].invert) return 100;
		else return 0;
	};




  void processKey(byte targetChannel, byte longPress) {

    if (longPress) {
      hmwdebug("Long key press, Channel ");
      hmwdebug(targetChannel); hmwdebug("\n");
    }
    else { // shortPress
      hmwdebug("Short key press, Channel ");
      hmwdebug(targetChannel); hmwdebug("\n");

      // Toggle
      setLevel(targetChannel, 0xFF);
    }
  }




	void readConfig(){
		byte* ptr;
		// EEPROM lesen
		ptr = (byte*)(&config);
		for(int address = 0; address < sizeof(config); address++){
			*ptr = EEPROM.read(address + 0x01);
			ptr++;
		};
		// defaults setzen, falls nicht sowieso klar
		if(config.logging_time == 0xFF) config.logging_time = 20;
		if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
		for (byte i=0; i < CHANNEL_IO_COUNT; i++) {
			if (config.switches[i].invert == 0xFF) config.switches[i].invert = 1;
		}

	};

};

// The device will be created in setup()
HMWDevice hmwdevice;



void factoryReset() {
  // writes FF into config
//  memset(sensors, 0xFF, sizeof(sensors[0]) * CHANNEL_IO_COUNT);
  // set defaults
//  setDefaults();
}


void handleButton() {
  // langer Tastendruck (5s) -> LED blinkt hektisch
  // dann innerhalb 10s langer Tastendruck (3s) -> LED geht aus, EEPROM-Reset

  static long lastTime = 0;
  static byte status = 0;  // 0: normal, 1: Taste erstes mal gedrückt, 2: erster langer Druck erkannt
                           // 3: Warte auf zweiten Tastendruck, 4: Taste zweites Mal gedrückt
                           // 5: zweiter langer Druck erkannt

  long now = millis();
  boolean buttonState = !digitalRead(BUTTON);


  switch(status) {
    case 0:
      if(buttonState) {status = 1;  hmwdebug(status);}
      lastTime = now;
      break;
    case 1:
      if(buttonState) {   // immer noch gedrueckt
        if(now - lastTime > 5000) {status = 2;   hmwdebug(status);}
      }else{              // nicht mehr gedrückt
    	if(now - lastTime > 100)   // determine sensors and send announce on short press
    		hmwmodule->broadcastAnnounce(0);
        status = 0;
        hmwdebug(status);
      };
      break;
    case 2:
      if(!buttonState) {  // losgelassen
    	status = 3;
    	hmwdebug(status);
    	lastTime = now;
      };
      break;
    case 3:
      // wait at least 100ms
      if(now - lastTime < 100)
    	break;
      if(buttonState) {   // zweiter Tastendruck
    	status = 4;
    	hmwdebug(status);
    	lastTime = now;
      }else{              // noch nicht gedrueckt
    	if(now - lastTime > 10000) {status = 0;   hmwdebug(status);}   // give up
      };
      break;
    case 4:
      if(now - lastTime < 100) // entprellen
          	break;
      if(buttonState) {   // immer noch gedrueckt
        if(now - lastTime > 3000) {status = 5;  hmwdebug(status);}
      }else{              // nicht mehr gedrückt
        status = 0;
        hmwdebug(status);
      };
      break;
    case 5:   // zweiter Druck erkannt
      if(!buttonState) {    //erst wenn losgelassen
    	// Factory-Reset          !!!!!!  TODO: Gehoert das ins Modul?
    	factoryReset();
    	hmwmodule->setNewId();
    	status = 0;
    	hmwdebug(status);
      }
      break;
  }

  // control LED
  static long lastLEDtime = 0;
  switch(status) {
    case 0:
      digitalWrite(LED, LOW);
      break;
    case 1:
      digitalWrite(LED, HIGH);
      break;
    case 2:
    case 3:
    case 4:
      if(now - lastLEDtime > 100) {  // schnelles Blinken
    	digitalWrite(LED,!digitalRead(LED));
    	lastLEDtime = now;
      };
      break;
    case 5:
      if(now - lastLEDtime > 750) {  // langsames Blinken
       	digitalWrite(LED,!digitalRead(LED));
       	lastLEDtime = now;
      };
  }
};






void setup()
{
#if DEBUG_VERSION == DEBUG_UNO
	pinMode(RS485_RXD, INPUT);
	pinMode(RS485_TXD, OUTPUT);
#elif DEBUG_VERSION == DEBUG_UNIV
	pinMode(DEBUG_RXD, INPUT);
	pinMode(DEBUG_TXD, OUTPUT);
#endif
	pinMode(RS485_TXEN, OUTPUT);
	digitalWrite(RS485_TXEN, LOW);

	pinMode(BUTTON, INPUT_PULLUP);
	pinMode(LED, OUTPUT);

#if DEBUG_VERSION == DEBUG_UNO
   hmwdebugstream = &Serial;
   Serial.begin(19200);
   rs485.begin(19200);    // RS485 via SoftwareSerial
#elif DEBUG_VERSION == DEBUG_UNIV
   SoftwareSerial* debugSerial = new SoftwareSerial(DEBUG_RXD, DEBUG_TXD);
   debugSerial->begin(19200);
   hmwdebugstream = debugSerial;
   Serial.begin(19200, SERIAL_8E1);
#else
   Serial.begin(19200, SERIAL_8E1);
#endif


   // config aus EEPROM lesen
   hmwdevice.readConfig();

   CHANNEL_PORTS   // create array 'channelPorts'
   for (byte channel = 0; channel < CHANNEL_IO_COUNT; channel++) {
	   pinMode(channelPorts[channel],OUTPUT);
	   digitalWrite(channelPorts[channel], LOW ^ config.switches[channel].invert);
   }



 	hmwmodule = new HMWModule(&hmwdevice, &hmwrs485, DEVICE_ID);

    hmwdebug("Huhu\n");

    // send announce message
	hmwmodule->broadcastAnnounce(0);
}


// The loop function is called in an endless loop
void loop()
{

 // Daten empfangen und alles, was zur Kommunikationsschicht gehört
 // processEvent vom Modul wird als Callback aufgerufen
   hmwrs485.loop();


 // Bedienung ueber Button
   handleButton();

};






