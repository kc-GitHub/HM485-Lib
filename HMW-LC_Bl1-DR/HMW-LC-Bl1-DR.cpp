//*******************************************************************
//
// HMWHomebrew.cpp
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HMW-1WIRE-TMP10
// Thorsten Pferdekaemper (thorsten@pferdekaemper.com)
// nach einer Vorlage von
// Dirk Hoffmann (hoffmann@vmd-jena.de)
//
//-------------------------------------------------------------------
//Hardwarebeschreibung:
// =====================
//
// Pinsettings for Arduino Uno
//
// D0: RXD, normaler Serieller Port fuer Debug-Zwecke
// D1: TXD, normaler Serieller Port fuer Debug-Zwecke
// D2: RXD, RO des RS485-Treiber
// D3: TXD, DI des RS485-Treiber
// D4: Direction (DE/-RE) Driver/Receiver Enable vom RS485-Treiber
//
// D5: Taster 1
// D10: 1Wire-Data
// D13: Status-LED

// Die Firmware funktioniert mit dem Standard-Uno Bootloader, im
// Gegensatz zur Homematic-Firmware

//*******************************************************************

// Die "DEBUG_UNO"-Version ist fuer einen normalen Arduino Uno (oder so)
// gedacht. Dadurch wird RS485 ueber SoftwareSerial angesteuert
// und der Hardware-Serial-Port wird fuer Debug-Ausgaben benutzt
// Dadurch kann man die normale USB-Verbindung zum Programmieren und
// debuggen benutzen

#define DEBUG_NONE 0   // Kein Debug-Ausgang, RS485 auf Hardware-Serial
#define DEBUG_UNO 1    // Hardware-Serial ist Debug-Ausgang, RS485 per Soft auf pins 5/6
#define DEBUG_UNIV 2   // Hardware-Serial ist RS485, Debug per Soft auf pins 5/6

#define DEBUG_VERSION DEBUG_UNIV

// Do not remove the include below
#include "HMW-LC-Bl1-DR.h"

#if DEBUG_VERSION == DEBUG_UNO || DEBUG_VERSION == DEBUG_UNIV
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>
#endif

// debug-Funktion
#include "HMWDebug.h"

// OneWire
#include <OneWire.h>

// EEPROM
#include <EEPROM.h>

// HM Wired Protokoll
#include "HMWRS485.h"

// default module methods
#include "HMWModule.h"
#include "HMWRegister.h"


/********************************/
/* Pinbelegung: 				*/
/********************************/

#define BUTTON 8            // Das Bedienelement
#define RS485_TXEN 2        // Transmit-Enable
#define I01 9				// Taster 1
#define I02 10				// Taster 2
#define BLIND1_ACT 11		// "Ein-/Aus-Relais"
#define BLIND1_DIR 12		// "Richungs-Relais"


/********************************/
/* Config der Rollo-Steuerung:  */
/********************************/

#define ON LOW					// Möglichkeit für intertierte Logik
#define OFF HIGH
#define UP LOW					// "Richtungs-Relais"
#define DOWN HIGH
#define blindWaitTime 200		// Wartezeit [ms] zwischen Ansteuerung des "Richtungs-Relais" und des "Ein-/Aus-Relais"
#define blindOffsetTime 1000	// Zeit [ms], die beim Anfahren der Endlagen auf die berechnete Zeit addiert wird, um die Endlagen wirklich sicher zu erreichen




// Ein paar defines...
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




#define CHANNEL_IO_COUNT 2
#define CHANNEL_IO_BYTES 1 //CHANNEL_IO_COUNT / 8
// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define CHANNEL_PORTS byte channelPorts[CHANNEL_IO_COUNT] = {I01, I02};
// Port Status, d.h. Port ist auf 0 oder 1
byte portStatus[CHANNEL_IO_BYTES];
// TODO: wird wirklich int gebraucht oder tut's auch byte?
unsigned int keyLongPressTime[2];
byte loggingTime;



unsigned long blindTimeNextState;
unsigned long blindTimeStart;
unsigned long now;


byte blindNextState;
byte blindCurrentState;
byte blindPositionRequested;
byte blindPositionActual = 50;
byte blindAngleActual = 50;
byte blindDirection;
byte blindForceNextState;
int blindTimeTopBottom;
int blindTimeBottomTop;

#define STOP 0
#define WAIT 1
#define TURN_AROUND 2
#define MOVE 3
#define RELAIS_OFF 4
#define SWITCH_DIRECTION 5




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





void getCurrentPosition() {

  if (blindCurrentState == MOVE)
    if (blindDirection == UP) {
      blindPositionActual -= (now - blindTimeStart) / blindTimeBottomTop;
      if (blindPositionActual > 100) blindPositionActual = 0; // robustness
      blindAngleActual = 0;
    }
    else {
      blindPositionActual += (now - blindTimeStart) / blindTimeTopBottom;
      if (blindPositionActual > 100) blindPositionActual = 100; // robustness
      blindAngleActual = 100;
    }
  else // => current state == TURN_AROUND
    // blindPosition unchanged
    if (blindDirection == UP) {
      blindAngleActual -= (now - blindTimeStart) / config.blinds[0].blindTimeChangeOver;
      if (blindAngleActual > 100) blindAngleActual = 0; // robustness
    }
    else {
      blindAngleActual += (now - blindTimeStart) / config.blinds[0].blindTimeChangeOver;
      if (blindAngleActual > 100) blindAngleActual = 100; // robustness
    }
}









// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:

	void setLevel(byte channel,unsigned int level) {

		  // blind control
		  if(level == 0xFF) { // toggle

		    if (blindCurrentState == TURN_AROUND)
		      blindNextState = STOP;

		 // if (blindCurrentState == MOVE)  // muss nicht extra erwähnt werden, nextState ist ohnehin STOP
		 //   blindNextState = STOP;


		    blindForceNextState = true;

		    if ((blindCurrentState == STOP) || (blindCurrentState == RELAIS_OFF)) {
		      if (blindDirection == UP) blindPositionRequested = 100;
		      else blindPositionRequested = 0;
		      blindNextState = WAIT;
		    }
		  }


		  else if(level == 0xC9) { // stop
		    blindNextState = STOP;
		    blindForceNextState = true;
		  }


		  else { // level is set

		    blindPositionRequested = level / 2;

		    if (blindPositionRequested > 100)
		    	blindPositionRequested = 100;

		    if ((blindCurrentState == TURN_AROUND) || (blindCurrentState == MOVE)) { // aktuelle Position ist nicht bekannt

		      getCurrentPosition();

		      if (((blindDirection == UP) && (blindPositionRequested > blindPositionActual)) || ((blindDirection == DOWN) && (blindPositionRequested < blindPositionActual))) {
		        // Rollo muss die Richtung umkehren:
		        blindNextState = SWITCH_DIRECTION;
		        blindForceNextState = true;
		      }
		      else {
		        // Rollo fährt schon in die richtige Richtung
		        if (blindCurrentState == MOVE) { // Zeit muss neu berechnet werden // current state == TURN_AROUND, nicht zu tun, Zeit wird ohnehin beim ersten Aufruf von MOVE berechnet
		          blindNextState = MOVE; // Im Zustand MOVE wird die neue Zeit berechnet
		          blindForceNextState = true;
		        }
		      }

		    }


		    else { // nächster State != STOP, d.h. aktueller State != MOVE, d.h. Rollo steht gerade
		      // set next state
		      blindNextState = WAIT;
		      blindForceNextState = true;
		    }

		  }


	}



	unsigned int getLevel(byte channel) {

		return blindPositionRequested;

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
		if(config.logging_time == 0xFF) config.logging_time = 20;
		if(config.central_address == 0xFFFFFFFF) config.central_address = 0x00000001;
		for(byte channel = 0; channel < HMW_CONFIG_NUM_KEYS; channel++){
			if(config.keys[channel].long_press_time == 0xFF) config.keys[channel].long_press_time = 10;
		};

		blindTimeTopBottom = (config.blinds[0].blindTimeTopBottom_high << 8) + config.blinds[0].blindTimeTopBottom_low;
		blindTimeBottomTop = (config.blinds[0].blindTimeBottomTop_high << 8) + config.blinds[0].blindTimeBottomTop_low;

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
  for(byte channel = 0; channel < CHANNEL_IO_COUNT; channel++) {
	  hmwdebug("Channel     : "); hmwdebug(channel); hmwdebug("\r\n");
	  hmwdebug("\r\n");
  }
}
#endif





// Tasten
void handleKeys() {
	// TODO: Vielleicht besser eine Klasse HMWKey oder so anlegen
	// millis() zum Zeitpunkt eines Tastendrucks
	// verwendet zum Entprellen, lange Tastendruecke und wiederholtes Senden langer Tastendruecke
	static unsigned long keyPressedMillis[2] = {0,0}; // Wir haben zwei Inputs
	static byte keyPressNum[2] = {0,0};
	// wann wurde das letzte mal "short" gesendet?
	static unsigned long lastSentLong[2] = {0,0};
	long now = millis();
	readPins();

	for(byte i = 0; i < HMW_CONFIG_NUM_KEYS; i++){
		// INPUT_LOCKED?
		if(!config.keys[i].input_locked) continue; // inverted logic, locked = 0
		// Taste nicht gedrueckt (negative Logik wegen INPUT_PULLUP)
		if(bitRead(portStatus[i/8],i%8)){
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







void handleBlind() {

  now = millis();
  if ((blindForceNextState == true) || (now >= blindTimeNextState)) {


    switch(blindNextState) {

    case RELAIS_OFF:
      // Switch off the "direction" relay
      digitalWrite(BLIND1_DIR, OFF);

      // debug message
      //Serial.print(millis()); Serial.print(" "); Serial.print("RELAIS OFF "); Serial.print(" "); Serial.print(blindPositionActual); Serial.print(" "); Serial.print(blindPositionRequested); Serial.print("Angle: "); Serial.println(blindAngleActual);

      blindCurrentState = RELAIS_OFF;
      blindTimeNextState = now + 20000;  // time is increased to avoid cyclic call
      break;


    case WAIT:
      if (blindPositionRequested > blindPositionActual)  // blind needs to move down
        blindDirection = DOWN;
      else  // blind needs to move up
        blindDirection = UP;

      // switch on the "direction" relay
      digitalWrite(BLIND1_DIR, blindDirection);

      // debug message
      //Serial.print(now); Serial.print(" "); Serial.print("WAIT       "); Serial.print(" "); Serial.print(blindPositionActual); Serial.print(" "); Serial.print(blindPositionRequested); Serial.print("Angle: "); Serial.println(blindAngleActual);

      // Set next state & delay time
      blindCurrentState = WAIT;
      blindNextState = TURN_AROUND;
      blindForceNextState = false;
      blindTimeNextState = now + blindWaitTime;
      break;


    case MOVE:
      // switch on the "active" relay
      digitalWrite(BLIND1_ACT, ON);

      // debug message
      //Serial.print(millis()); Serial.print(" "); Serial.print("MOVE       "); Serial.print(" "); Serial.print(blindPositionActual); Serial.print(" "); Serial.print(blindPositionRequested); Serial.print("Angle: "); Serial.println(blindAngleActual);

      blindTimeStart = now;

      // Set next state & delay time
      blindCurrentState = MOVE;
      blindNextState = STOP;
      if (blindDirection == UP) {
        blindTimeNextState = now + (blindPositionActual - blindPositionRequested) * blindTimeBottomTop;
      }
      else {
        blindTimeNextState = now + (blindPositionRequested - blindPositionActual) * blindTimeTopBottom;
      }

      // add offset time if final positions are requested to ensure that final position is really reached
      if ((blindPositionRequested == 0) || (blindPositionRequested == 100))
        blindTimeNextState += blindOffsetTime;

      if (blindForceNextState == true)
        blindForceNextState = false;
      break;


    case STOP:
      // calculate current position if next state was forced by toggle/stop
      if (blindForceNextState == true) {
        blindForceNextState = false;
        getCurrentPosition();
        blindPositionRequested = blindPositionActual;
      }
      // or take over requested position if blind has moved for the desired time
      else {
        blindPositionActual = blindPositionRequested;
        if (blindDirection == UP) blindAngleActual = 0;
        else blindAngleActual = 100;
      }

      // send info message with current position
      hmwmodule->sendInfoMessage(2, blindPositionActual, 0xFFFFFFFF);

      // switch off the "active" relay
      digitalWrite(BLIND1_ACT, OFF);

      // debug message
      //Serial.print(now); Serial.print(" "); Serial.print("STOP       "); Serial.print(" "); Serial.print(blindPositionActual); Serial.print(" "); Serial.print(blindPositionRequested); Serial.print("Angle: "); Serial.println(blindAngleActual);

      // Set next state & delay time
      blindCurrentState = STOP;
      blindNextState = RELAIS_OFF;
      blindTimeNextState = now + blindWaitTime;
      break;


    case TURN_AROUND:
      // switch on the "active" relay
      digitalWrite(BLIND1_ACT, ON);
      blindTimeStart = now;

      // debug message
      //Serial.print(now); Serial.print(" "); Serial.print("TURN_AROUND"); Serial.print(" "); Serial.print(blindPositionActual); Serial.print(" "); Serial.print(blindPositionRequested); Serial.print("Angle: "); Serial.println(blindAngleActual);

      blindCurrentState = TURN_AROUND;
      blindNextState = MOVE;
      if (blindDirection == UP)
        blindTimeNextState = now + blindAngleActual * config.blinds[0].blindTimeChangeOver;
      else
        blindTimeNextState = now + (100 - blindAngleActual) * config.blinds[0].blindTimeChangeOver;
      break;


    case SWITCH_DIRECTION:

      // switch off the "active" relay
      digitalWrite(BLIND1_ACT, OFF);

      // debug message
      //Serial.print(now); Serial.print(" "); Serial.print("SWITCH_DIR "); Serial.print(" "); Serial.print(blindPositionActual); Serial.print(" "); Serial.println(blindPositionRequested);

      // Set current state, next state & delay time
      blindCurrentState = SWITCH_DIRECTION;
      blindNextState = WAIT;
      blindForceNextState = false;
      blindTimeNextState = now + blindWaitTime;
      break;
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

   // set input/output
   // Input Pins arbeiten mit PULLUP, d.h. muessen per Taster
   // auf Masse gezogen werden
   pinMode(I01,INPUT_PULLUP);
   pinMode(I02,INPUT_PULLUP);

   pinMode(BLIND1_ACT,OUTPUT);
   digitalWrite(BLIND1_ACT,OFF);
   pinMode(BLIND1_DIR,OUTPUT);
   digitalWrite(BLIND1_DIR,OFF);



	// device type: 0x81
   // TODO: Modultyp irgendwo als define
 	hmwmodule = new HMWModule(&hmwdevice, &hmwrs485, 0x15);

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

   handleKeys();

   handleBlind();

};






