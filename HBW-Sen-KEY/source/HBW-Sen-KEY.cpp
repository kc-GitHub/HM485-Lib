//*******************************************************************
//
// HBW-Sen-EP.cpp
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HBW-Sen-EP zum Zählen von elektrischen Pulsen (z.B. S0-Schnittstelle)
//
//*******************************************************************


/********************************/
/* Pinbelegung: 				*/
/********************************/

#define BUTTON 8            // Das Bedienelement
#define RS485_TXEN 2        // Transmit-Enable

#define SS_PIN 10
#define RST_PIN 9




#define DEBUG_NONE 0   // Kein Debug-Ausgang, RS485 auf Hardware-Serial
#define DEBUG_UNO 1    // Hardware-Serial ist Debug-Ausgang, RS485 per Soft auf pins 5/6
#define DEBUG_UNIV 2   // Hardware-Serial ist RS485, Debug per Soft auf pins 5/6

#define DEBUG_VERSION DEBUG_UNIV

#if DEBUG_VERSION == DEBUG_UNO
  #define RS485_RXD 5
  #define RS485_TXD 6
  #define LED 13        // Signal-LED
#elif DEBUG_VERSION == DEBUG_UNIV
  #define DEBUG_RXD 5
  #define DEBUG_TXD 6
  #define LED 4
#else
  #define LED 4         // Signal-LED
#endif






// Do not remove the include below
#include "HBW-Sen-KEY.h"

#if DEBUG_VERSION == DEBUG_UNO || DEBUG_VERSION == DEBUG_UNIV
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>
#endif

// debug-Funktion
#include "HMWDebug.h"

#include <EEPROM.h>
#include <SPI.h>
#include <RFID.h>

// HM Wired Protokoll
#include "HMWRS485.h"
// default module methods
#include "HMWModule.h"


#include "HMWRegister.h"




#define NO_CARD 0
#define CARD_DETECTED 1

unsigned char rfidSerNum[5];
byte state;



#define I01 A0
#define I02 A1
#define I03 A2
#define I04 A3
#define CHANNEL_IO_COUNT 4
#define CHANNEL_IO_BYTES 1 //CHANNEL_IO_COUNT / 8
// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define CHANNEL_PORTS byte channelPorts[CHANNEL_IO_COUNT] = {I01, I02, I03, I04};
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

RFID rfid(SS_PIN, RST_PIN);



// write config to EEPROM in a hopefully smart way
void writeConfig(){
//    byte* ptr;
//    byte data;
	// EEPROM lesen und schreiben
//	ptr = (byte*)(sensors);
//	for(int address = 0; address < sizeof(sensors[0]) * CHANNEL_IO_COUNT; address++){
//	  hmwmodule->writeEEPROM(address + 0x10, *ptr);
//	  ptr++;
//    };
};




void setDefaults(){
  // defaults setzen
  if(config.logging_time == 0xFF) config.logging_time = 20;
  if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
  for(byte channel = 0; channel < HMW_CONFIG_NUM_KEYS; channel++){
	if(config.keys[channel].long_press_time == 0xFF) config.keys[channel].long_press_time = 10;
	if(config.keys[channel].inverted == 0xFF) config.keys[channel].inverted = 1;
	if(config.keys[channel].pullup == 0xFF) config.keys[channel].pullup = 1;
  }

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
	      return;  // there is nothing to set
	}



	unsigned int getLevel(byte channel) {
		// everything in the right limits?

		// Rückgabewert!!!
		return 0;
	};



	void readConfig(){
		byte* ptr;
		// EEPROM lesen
		ptr = (byte*)(&config);
		for(int address = 0; address < sizeof(config); address++){
			*ptr = EEPROM.read(address + 0x01);
			ptr++;
		};

		// defaults setzen, falls nicht sowieso klar
		setDefaults();


	   CHANNEL_PORTS
	   for (byte channel = 0; channel < HMW_CONFIG_NUM_KEYS; channel++) {
		   if (config.keys[channel].pullup)
			   pinMode(channelPorts[channel],INPUT_PULLUP);
		   else
			   pinMode(channelPorts[channel],INPUT);
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


#if DEBUG_VERSION != DEBUG_NONE
void printChannelConf(){
  for(byte channel = 0; channel < HMW_CONFIG_NUM_KEYS; channel++) {
	  hmwdebug("Channel     : "); hmwdebug(channel); hmwdebug("\r\n");
	  hmwdebug("\r\n");
  }
}
#endif




void handleRfid() {
    if (rfid.isCard()) {
        if (rfid.readCardSerial()) {
            if (state == NO_CARD ||
            	rfid.serNum[0] != rfidSerNum[0] ||
                rfid.serNum[1] != rfidSerNum[1] ||
                rfid.serNum[2] != rfidSerNum[2] ||
                rfid.serNum[3] != rfidSerNum[3] ||
                rfid.serNum[4] != rfidSerNum[4]
            ) {

                state = CARD_DETECTED;

            	rfidSerNum[0] = rfid.serNum[0];
                rfidSerNum[1] = rfid.serNum[1];
                rfidSerNum[2] = rfid.serNum[2];
                rfidSerNum[3] = rfid.serNum[3];
                rfidSerNum[4] = rfid.serNum[4];

                hmwmodule->sendInfoMessage(0, &rfid.serNum[0], 4, config.central_address);

             } else {
               /* If we have the same ID, just write a dot. */
               // Serial.print(".");
             }
          }
    }
    else {
    	// keine Karte gefunden
    	if (state == CARD_DETECTED) {

    		state = NO_CARD;
    		long cardId = 0;
            byte *ptr;
    		ptr = (byte*)(&cardId);
            hmwmodule->sendInfoMessage(0, ptr, 4, config.central_address);
            // Serial.println("Card removed");
    	}
    	else {
            // Serial.println("No card reader detected");
    	}

    }

    rfid.halt();


}





// Tasten
void handleKeys() {
	// TODO: Vielleicht besser eine Klasse HMWKey oder so anlegen
	// millis() zum Zeitpunkt eines Tastendrucks
	// verwendet zum Entprellen, lange Tastendruecke und wiederholtes Senden langer Tastendruecke
	static unsigned long keyPressedMillis[4] = {0,0,0,0}; // Wir haben zwei Inputs
	static byte keyPressNum[4] = {0,0,0,0};
	// wann wurde das letzte mal "short" gesendet?
	static unsigned long lastSentLong[4] = {0,0,0,0};
	long now = millis();
	readPins();

	for(byte i = 0; i < HMW_CONFIG_NUM_KEYS; i++){
		// INPUT_LOCKED?
		if(!config.keys[i].input_locked) continue; // inverted logic, locked = 0
		// Taste nicht gedrueckt (negative Logik wegen INPUT_PULLUP)
		if((bitRead(portStatus[i/8],i%8)) ^ config.keys[i].inverted ^ 1) {
			// Taste war auch vorher nicht gedrueckt kann ignoriert werden
			// Taste war vorher gedrueckt?
			if(keyPressedMillis[i]){
				// entprellen, nur senden, wenn laenger als 50ms gedrueckt
				// aber noch kein "long" gesendet
				if(now - keyPressedMillis[i] >= 50 && !lastSentLong[i]){
					keyPressNum[i]++;
					// TODO: muss das eigentlich an die Zentrale gehen?
					hmwmodule->broadcastKeyEvent(i,keyPressNum[i]);
					// gleich ein Announce hinterher
					// TODO: Vielleicht gehoert das in den allgemeinen Teil
					// hmwmodule->broadcastAnnounce(i);
				};
				keyPressedMillis[i] = 0;
			};
		}else{
			// Taste gedrueckt
			// Taste war vorher schon gedrueckt
			if(keyPressedMillis[i]){
				// muessen wir ein "long" senden?
				if(lastSentLong[i]) { // schon ein LONG gesendet
					if(now - lastSentLong[i] >= 300){ // alle 300ms wiederholen
						// keyPressNum nicht erhoehen
						lastSentLong[i] = now ? now : 1; // der Teufel ist ein Eichhoernchen
						// TODO: muss das eigentlich an die Zentrale gehen?
						hmwmodule->broadcastKeyEvent(i,keyPressNum[i], true);
					};
				}else if(millis() - keyPressedMillis[i] >= long(config.keys[i].long_press_time) * 100) {
					// erstes LONG
					keyPressNum[i]++;
					lastSentLong[i] = millis();
					// TODO: muss das eigentlich an die Zentrale gehen?
					hmwmodule->broadcastKeyEvent(i,keyPressNum[i], true);
					// gleich ein Announce hinterher
					// TODO: Vielleicht gehoert das in den allgemeinen Teil
					// hmwmodule->broadcastAnnounce(i);
				};
			}else{
				// Taste war vorher nicht gedrueckt
				keyPressedMillis[i] = now ? now : 1; // der Teufel ist ein Eichhoernchen
				lastSentLong[i] = 0;
			}
		}
	}
}







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


   SPI.begin();
   rfid.init();


	// device type: 0x84
   // TODO: Modultyp irgendwo als define
 	hmwmodule = new HMWModule(&hmwdevice, &hmwrs485, 0x85);

    hmwdebug("Huhu\n");

    // send announce message
	hmwmodule->broadcastAnnounce(0);
#if DEBUG_VERSION != DEBUG_NONE
	printChannelConf();
#endif
}


// The loop function is called in an endless loop
void loop()
{
	// Daten empfangen und alles, was zur Kommunikationsschicht gehört
	// processEvent vom Modul wird als Callback aufgerufen
	hmwrs485.loop();


	// Bedienung ueber Button
	handleButton();

	// Verarbeitung des Kartenlesers
	handleRfid();

	// Tasten
	handleKeys();

};






