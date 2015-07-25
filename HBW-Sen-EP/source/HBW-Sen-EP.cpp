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
#define Sen1 14				// 14 = A0 als digitaler Eingang
#define Sen2 15
#define Sen3 16
#define Sen4 17
#define Sen5 18
#define Sen6 19
#define Sen7 3
#define Sen8 7




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



#define POLLING_TIME 10	    // S0-Puls muss laut Definition mindestens 30ms lang sein - Abtastung mit 10ms soll ausreichen




// Do not remove the include below
#include "HBW-Sen-EP.h"

#if DEBUG_VERSION == DEBUG_UNO || DEBUG_VERSION == DEBUG_UNIV
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>
#endif

// debug-Funktion
#include "HMWDebug.h"

// EEPROM
#include <EEPROM.h>

// HM Wired Protokoll
#include "HMWRS485.h"
// default module methods
#include "HMWModule.h"


#include "HMWRegister.h"




// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define CHANNEL_PORTS byte channelPorts[HMW_CONFIG_NUM_COUNTERS] = {Sen1, Sen2, Sen3, Sen4, Sen5, Sen6, Sen7, Sen8};



byte currentPortState[HMW_CONFIG_NUM_COUNTERS];
byte oldPortState[HMW_CONFIG_NUM_COUNTERS];
int currentCount[HMW_CONFIG_NUM_COUNTERS];
int lastSentCount[HMW_CONFIG_NUM_COUNTERS];
long lastSentTime[HMW_CONFIG_NUM_COUNTERS];
long lastPortReadTime;




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
//    };
};




void setDefaults(){
  // defaults setzen
  if(config.logging_time == 0xFF) config.logging_time = 20;
  if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;

  for(byte channel = 0; channel < HMW_CONFIG_NUM_COUNTERS; channel++){
    if(config.counters[channel].send_delta_count == 0xFFFF) config.counters[channel].send_delta_count = 1;
    if(config.counters[channel].send_min_interval == 0xFFFF) config.counters[channel].send_min_interval = 10;
    if(config.counters[channel].send_max_interval == 0xFFFF) config.counters[channel].send_max_interval = 150;
  };
};



// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:

	void setLevel(byte channel,unsigned int level) {
	      return;  // there is nothing to set
	}



	unsigned int getLevel(byte channel) {
		// everything in the right limits?
		if(channel >= HMW_CONFIG_NUM_COUNTERS) return 0;
		return currentCount[channel];
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
  for(byte channel = 0; channel < HMW_CONFIG_NUM_COUNTERS; channel++) {
	  hmwdebug("Channel     : "); hmwdebug(channel); hmwdebug("\r\n");
	  hmwdebug("\r\n");
  }
}
#endif




void handleCounter() {

  long now = millis();


  //


  CHANNEL_PORTS
  if ((now - lastPortReadTime) > POLLING_TIME) {

	  for(byte channel = 0; channel < HMW_CONFIG_NUM_COUNTERS; channel++) {
		  currentPortState[channel] = digitalRead(channelPorts[channel]);
		  if (currentPortState[channel] > oldPortState[channel]) {
			currentCount[channel]++;
		  }
		  oldPortState[channel] = currentPortState[channel];


	  }
	  lastPortReadTime = now;
  }



  // Pruefen, ob wir irgendwas senden muessen

  for(byte channel = 0; channel < HMW_CONFIG_NUM_COUNTERS; channel++) {
	 // do not send before min interval
	 if(config.counters[channel].send_min_interval && now - lastSentTime[channel] < (long)(config.counters[channel].send_min_interval) * 1000)
		  continue;
    if(    (config.counters[channel].send_max_interval && now - lastSentTime[channel] >= (long)(config.counters[channel].send_max_interval) * 1000)
   	 || (config.counters[channel].send_delta_count
   	         && abs( currentCount[channel] - lastSentCount[channel] ) >= (config.counters[channel].send_delta_count))) {
	     hmwmodule->sendInfoMessage(channel,currentCount[channel],config.central_address);
        lastSentCount[channel] = currentCount[channel];
        lastSentTime[channel] = now;
    };
  };
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

   // set input/output
   // Input Pins arbeiten mit PULLUP, d.h. muessen per Taster
   // auf Masse gezogen werden
   long int now = millis();

   CHANNEL_PORTS
   for (byte i = 0; i < HMW_CONFIG_NUM_COUNTERS; i++) {
	   pinMode(channelPorts[i],INPUT);
	   lastSentTime[i] = now;
   }

   lastPortReadTime = now;


	// device type: 0x84
   // TODO: Modultyp irgendwo als define
 	hmwmodule = new HMWModule(&hmwdevice, &hmwrs485, 0x84);

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

	// Verarbeitung der S0-Schnittstelle
	handleCounter();

};






