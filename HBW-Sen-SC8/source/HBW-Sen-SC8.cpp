//*******************************************************************
//
// HBW-Sen-SC8.cpp
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HBW-Sen-SC8 zum Einlesen von 8 Tastern
// - Active HIGH oder LOW kann über das FHEM-Webfrontend konfiguriert werden
// - Pullup kann über das FHEM-Webfrontend aktiviert werden
// - Erkennung von Doppelklicks
// - Zusätzliches Event beim Loslassen einer lang gedrückten Taste
//
//*******************************************************************


/********************************/
/* Pinbelegung: 				*/
/********************************/

#define BUTTON 8            // Das Bedienelement
#define RS485_TXEN 2        // Transmit-Enable


#define In1 A0
#define In2 A1
#define In3 A2
#define In4 A3
#define In5 A4
#define In6 A5
#define In7 3
#define In8 7



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
  #define LED 13
#else
  #define LED 4         // Signal-LED
#endif






// Do not remove the include below
#include "HBW-Sen-SC8.h"

#if DEBUG_VERSION == DEBUG_UNO || DEBUG_VERSION == DEBUG_UNIV
	// TODO: Eigene SoftwareSerial
	#include <SoftwareSerial.h>
#endif

// debug-Funktion
#include "HMWDebug.h"

#include <EEPROM.h>

// HM Wired Protokoll
#include "HMWRS485.h"
// default module methods
#include "HMWModule.h"

#include "HMWRegister.h"

#include <ClickButton.h>



#define CHANNEL_IO_COUNT 8
#define CHANNEL_IO_BYTES 1 //CHANNEL_IO_COUNT / 8
// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define CHANNEL_PORTS byte channelPorts[CHANNEL_IO_COUNT] = {In1, In2, In3, In4, In5, In6, In7, In8};
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

// Instantiate ClickButton objects in an array
ClickButton button[HMW_CONFIG_NUM_KEYS] = {
  ClickButton (In1, LOW, CLICKBTN_PULLUP),
  ClickButton (In2, LOW, CLICKBTN_PULLUP),
  ClickButton (In3, LOW, CLICKBTN_PULLUP),
  ClickButton (In4, LOW, CLICKBTN_PULLUP),
  ClickButton (In5, LOW, CLICKBTN_PULLUP),
  ClickButton (In6, LOW, CLICKBTN_PULLUP),
  ClickButton (In7, LOW, CLICKBTN_PULLUP),
  ClickButton (In8, LOW, CLICKBTN_PULLUP)
};




// write config to EEPROM in a hopefully smart way
void writeConfig(){
    byte* ptr;
    byte data;
	// EEPROM lesen und schreiben

    // Testen, ob Config richtig geschrieben wird!
	ptr = (byte*)(&config.keys[0]);
	for(int address = 0x07; address < sizeof(config); address++){
	  hmwmodule->writeEEPROM(address, *ptr);
	  ptr++;
    };
};




void setDefaults(){
  // defaults setzen
  if(config.logging_time == 0xFF) config.logging_time = 20;
  if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
  for(byte channel = 0; channel < HMW_CONFIG_NUM_KEYS; channel++){
	if(config.keys[channel].long_press_time == 0xFF) config.keys[channel].long_press_time = 40;
	if(config.keys[channel].input_locked == 0xFF) config.keys[channel].input_locked = 0;
	if(config.keys[channel].inverted == 0xFF) config.keys[channel].inverted = 0;
	if(config.keys[channel].pullup == 0xFF) config.keys[channel].pullup = 1;
  }

};





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


	   for (int i=0; i < HMW_CONFIG_NUM_KEYS; i++) {
	     // Setup button timers (all in milliseconds / ms)
	     // (These are default if not set, but changeable for convenience)
	     button[i] = ClickButton (channelPorts[i], config.keys[i].inverted, config.keys[i].pullup);

		 button[i].debounceTime   = 20;   // Debounce timer in ms
	     button[i].multiclickTime = 250;  // Time limit for multi clicks
	     button[i].longClickTime  = config.keys[i].long_press_time * 10; // Time until long clicks register
	   }


	};


};

// The device will be created in setup()
HMWDevice hmwdevice;



void factoryReset() {
  // writes FF into config
  for(unsigned int i=0; i < sizeof(config); i++) {
	  hmwmodule->writeEEPROM(i, 0xFF, false);
  }
  // memset(sensors, 0xFF, sizeof(config));
  // set defaults
  setDefaults();
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






// Tasten
void handleKeys() {

	static unsigned long lastSentLong[8] = {0,0,0,0,0,0,0,0};
	long now = millis();
	static int keyState[HMW_CONFIG_NUM_KEYS];
    static byte keyPressNum[8] = {0,0,0,0,0,0,0,0};



	for (int i =  0; i < HMW_CONFIG_NUM_KEYS; i++) {

		button[i].Update();


		if (button[i].clicks != 0) {
			keyState[i] = button[i].clicks;

			if (button[i].clicks == 1) { // Einfachklick
				keyPressNum[i]++;
				hmwmodule->broadcastKeyEvent(i,keyPressNum[i], 3); // Taste gedrückt
    		}
    		if (button[i].clicks >= 2) {	// Mehrfachklick
    				keyPressNum[i]++;
    				hmwmodule->broadcastKeyEvent(i,keyPressNum[i], 1); // Taste gedrückt
	   		}
			if (button[i].clicks < 0) {  // erstes LONG
				keyPressNum[i]++;
				lastSentLong[i] = millis();
				hmwmodule->broadcastKeyEvent(i,keyPressNum[i], 3);
			}

	    }



		else if (keyState[i] < 0) {		// Taste ist oder war lang gedrückt
	    	if (button[i].depressed == true) {	// ist noch immer gedrückt --> alle 300ms senden
//				if(lastSentLong[i]) {
					if(now - lastSentLong[i] >= 300){ // alle 300ms erneut senden
						lastSentLong[i] = now ? now : 1;
						hmwmodule->broadcastKeyEvent(i,keyPressNum[i], 3);
					}
//				}

	    	}
	    	else {		// "Losgelassen" senden
		   		if (keyState[i] < 0)	{	// Taste wurde vorher lange gehalten (-1, -2 oder -3)
		   			hmwmodule->broadcastKeyEvent(i,keyPressNum[i], 0);
		   			lastSentLong[i] = 0;
		    		keyState[i] = 0;
		   		}

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


   Serial.println("Booting...");

   // config aus EEPROM lesen
   hmwdevice.readConfig();


	// device type: 0x84
    // TODO: Modultyp irgendwo als define
 	hmwmodule = new HMWModule(&hmwdevice, &hmwrs485, 0x86);

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


	// Tasten
	handleKeys();

};






