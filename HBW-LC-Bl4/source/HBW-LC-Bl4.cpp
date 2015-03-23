//*******************************************************************
//
// HBW-LC-Bl4.cpp
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HBW-LC-Sw8
//
//*******************************************************************


/********************************/
/* Pinbelegung: 				*/
/********************************/

#define BUTTON 8            // Das Bedienelement
#define RS485_TXEN 2        // Transmit-Enable
#define BLIND1_ACT A0		// "Ein-/Aus-Relais"
#define BLIND1_DIR A1		// "Richungs-Relais"
#define BLIND2_ACT A2
#define BLIND2_DIR A3
#define BLIND3_ACT A4
#define BLIND3_DIR A5
#define BLIND4_ACT 3
#define BLIND4_DIR 7




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


/********************************/
/* Config der Rollo-Steuerung:  */
/********************************/

#define ON LOW					// Möglichkeit für intertierte Logik
#define OFF HIGH
#define UP LOW					// "Richtungs-Relais"
#define DOWN HIGH
#define blindWaitTime 200		// Wartezeit [ms] zwischen Ansteuerung des "Richtungs-Relais" und des "Ein-/Aus-Relais"
#define blindOffsetTime 1000	// Zeit [ms], die beim Anfahren der Endlagen auf die berechnete Zeit addiert wird, um die Endlagen wirklich sicher zu erreichen





// Do not remove the include below
#include "HBW-LC-Bl4.h"

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






unsigned long blindTimeNextState[4];
unsigned long blindTimeStart[4];
unsigned long now;


byte blindNextState[4];
byte blindCurrentState[4];
byte blindPositionRequested[4];
byte blindPositionRequestedSave[4];
byte blindPositionActual[4];
byte blindPositionLast[4];
byte blindAngleActual[4];
byte blindDirection[4];
byte blindForceNextState[4];
unsigned int blindTimeTopBottom[4];
unsigned int blindTimeBottomTop[4];
bool blindPositionKnown[4];
bool blindSearchingForRefPosition[4];
byte blindDir[4] = {BLIND1_DIR, BLIND2_DIR, BLIND3_DIR, BLIND4_DIR};
byte blindAct[4] = {BLIND1_ACT, BLIND2_ACT, BLIND3_ACT, BLIND4_ACT};

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






void getCurrentPosition(byte channel) {

	if (blindCurrentState[channel] == MOVE)
		if (blindDirection[channel] == UP) {
			blindPositionActual[channel] = blindPositionLast[channel] - (now - blindTimeStart[channel]) / blindTimeBottomTop[channel];
			if (blindPositionActual[channel] > 100)
				blindPositionActual[channel] = 0; // robustness
			blindAngleActual[channel] = 0;
		}
		else {
			blindPositionActual[channel] = blindPositionLast[channel] + (now - blindTimeStart[channel]) / blindTimeTopBottom[channel];
			if (blindPositionActual[channel] > 100)
				blindPositionActual[channel] = 100; // robustness
			blindAngleActual[channel] = 100;
		}
	else // => current state == TURN_AROUND
		// blindPosition unchanged
		if (blindDirection[channel] == UP) {
			blindAngleActual[channel] -= (now - blindTimeStart[channel]) / config.blinds[channel].blindTimeChangeOver;
			if (blindAngleActual[channel] > 100)
				blindAngleActual[channel] = 0; // robustness
		}
		else {
			blindAngleActual[channel] += (now - blindTimeStart[channel]) / config.blinds[channel].blindTimeChangeOver;
			if (blindAngleActual[channel] > 100)
				blindAngleActual[channel] = 100; // robustness
		}
}









// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:

	void setLevel(byte channel,unsigned int level) {

		// blind control
		if(level == 0xFF) { // toggle

			hmwdebug("Toggle\n");
			if (blindCurrentState[channel] == TURN_AROUND)
				blindNextState[channel] = STOP;

			// if (blindCurrentState[channel] == MOVE)  // muss nicht extra erwähnt werden, nextState ist ohnehin STOP
			//   blindNextState[channel] = STOP;


			blindForceNextState[channel] = true;

			if ((blindCurrentState[channel] == STOP) || (blindCurrentState[channel] == RELAIS_OFF)) {
				if (blindDirection[channel] == UP) blindPositionRequested[channel] = 100;
				else blindPositionRequested[channel] = 0;

				blindNextState[channel] = WAIT;

				// if current blind position is not known (e.g. due to a reset), actual position is set to the limit, to ensure that the moving time is long enough to reach the end position
				if (!blindPositionKnown[channel]) {
					hmwdebug("Position unknown\n");
					if (blindDirection[channel] == UP)
						blindPositionActual[channel] = 0;
					else
						blindPositionActual[channel] = 100;
				}

			}

		}


		else if(level == 0xC9) { // stop
			hmwdebug("Stop\n");
			blindNextState[channel] = STOP;
			blindForceNextState[channel] = true;
		}


		else { // level is set

			blindPositionRequested[channel] = level / 2;

			if (blindPositionRequested[channel] > 100)
				blindPositionRequested[channel] = 100;
			hmwdebug("Requested Position: "); hmwdebug(blindPositionRequested[channel]); hmwdebug("\n");


			if ((blindCurrentState[channel] == TURN_AROUND) || (blindCurrentState[channel] == MOVE)) { // aktuelle Position ist nicht bekannt

				getCurrentPosition(channel);

				if (((blindDirection[channel] == UP) && (blindPositionRequested[channel] > blindPositionActual[channel])) || ((blindDirection[channel] == DOWN) && (blindPositionRequested[channel] < blindPositionActual[channel]))) {
					// Rollo muss die Richtung umkehren:
					blindNextState[channel] = SWITCH_DIRECTION;
					blindForceNextState[channel] = true;
				}
				else {
					// Rollo fährt schon in die richtige Richtung
					if (blindCurrentState[channel] == MOVE) { // Zeit muss neu berechnet werden // current state == TURN_AROUND, nicht zu tun, Zeit wird ohnehin beim ersten Aufruf von MOVE berechnet
						blindNextState[channel] = MOVE; // Im Zustand MOVE wird die neue Zeit berechnet
						blindForceNextState[channel] = true;
					}
				}

			}


			else { // aktueller State != MOVE und != TURN_AROUND, d.h. Rollo steht gerade
				// set next state only if a new target value is requested
				if (blindPositionRequested[channel] != blindPositionActual[channel]) {
					blindNextState[channel] = WAIT;
					blindForceNextState[channel] = true;
				}
			}




			if (!blindPositionKnown[channel]) {
				hmwdebug("Position unknown. Moving to reference position.\n");

				if (blindPositionRequested[channel] == 0) {
					blindPositionActual[channel] = 100;
				}
				else {
					if (blindPositionRequested[channel] == 100) {
						blindPositionActual[channel] = 0;
					}
					else {	// Target position >0 and <100
						blindPositionActual[channel] = 100;
						blindPositionRequestedSave[channel] = blindPositionRequested[channel];
						blindPositionRequested[channel] = 0;
						blindSearchingForRefPosition[channel] = true;
					}
				}
			}



		}
	}



	unsigned int getLevel(byte channel) {

		//return blindPositionRequested[channel];
		getCurrentPosition(channel);
		return blindPositionActual[channel];

	};




	void processKey(unsigned char targetChannel, byte longPress) {

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

		for (byte channel = 0; channel < 4; channel++) {
			blindTimeTopBottom[channel] = (config.blinds[channel].blindTimeTopBottom_high << 8) + config.blinds[channel].blindTimeTopBottom_low;
			blindTimeBottomTop[channel] = (config.blinds[channel].blindTimeBottomTop_high << 8) + config.blinds[channel].blindTimeBottomTop_low;

			if (blindTimeTopBottom[channel] == 0xFFFF) blindTimeTopBottom[channel] = 200;
			if (blindTimeBottomTop[channel] == 0xFFFF) blindTimeBottomTop[channel] = 200;
			if (config.blinds[channel].blindTimeChangeOver == 0xFF) config.blinds[channel].blindTimeChangeOver = 20;
		}





	};

};

// The device will be created in setup()
HMWDevice hmwdevice;



void factoryReset() {
	// writes FF into config
	// memset(sensors, 0xFF, sizeof(sensors[0]) * CHANNEL_IO_COUNT);
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
			if(now - lastTime > 5000) {
				status = 2;
				hmwdebug(status);
			}
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




void debugStateChange(byte state, byte channel) {
	hmwdebug("State: ");
	switch(state) {
		case RELAIS_OFF: hmwdebug("RELAY_OFF"); break;
		case WAIT: hmwdebug("WAIT"); break;
		case MOVE: hmwdebug("MOVE"); break;
		case STOP: hmwdebug("STOP"); break;
		case TURN_AROUND: hmwdebug("TURN_AROUND"); break;
		case SWITCH_DIRECTION: hmwdebug("SWITCH_DIRECTION"); break;
	}
	hmwdebug(" - Time: ");
	hmwdebug(now);
	hmwdebug(" - Target: ");
	hmwdebug(blindPositionRequested[channel]);
	hmwdebug(" - Actual: ");
	hmwdebug(blindPositionActual[channel]);
	hmwdebug("\n");

}




void handleBlind() {

	now = millis();
	for (byte channel = 0; channel < 1; channel++) {
//  for (byte channel = 0; channel < 4; channel++) {

		if ((blindForceNextState[channel] == true) || (now >= blindTimeNextState[channel])) {

			switch(blindNextState[channel]) {

			case RELAIS_OFF:
				// Switch off the "direction" relay
				digitalWrite(blindDir[channel], OFF);

				// debug message
				debugStateChange(blindNextState[channel], channel);

				blindCurrentState[channel] = RELAIS_OFF;
				blindTimeNextState[channel] = now + 20000;  // time is increased to avoid cyclic call
				break;


			case WAIT:
				if (blindPositionRequested[channel] > blindPositionActual[channel])  // blind needs to move down
					blindDirection[channel] = DOWN;
				else  // blind needs to move up
					blindDirection[channel] = UP;

				// switch on the "direction" relay
				digitalWrite(blindDir[channel], blindDirection[channel]);

				// debug message
				debugStateChange(blindNextState[channel], channel);

				// Set next state & delay time
				blindCurrentState[channel] = WAIT;
				blindNextState[channel] = TURN_AROUND;
				blindForceNextState[channel] = false;
				blindTimeNextState[channel] = now + blindWaitTime;
				break;


			case MOVE:
				// switch on the "active" relay
				digitalWrite(blindAct[channel], ON);

				// debug message
				debugStateChange(blindNextState[channel], channel);

				blindTimeStart[channel] = now;
				blindPositionLast[channel] = blindPositionActual[channel];

				// Set next state & delay time
				blindCurrentState[channel] = MOVE;
				blindNextState[channel] = STOP;
				if (blindDirection[channel] == UP) {
					blindTimeNextState[channel] = now + (blindPositionActual[channel] - blindPositionRequested[channel]) * blindTimeBottomTop[channel];
				}
				else {
					blindTimeNextState[channel] = now + (blindPositionRequested[channel] - blindPositionActual[channel]) * blindTimeTopBottom[channel];
				}

				// add offset time if final positions are requested to ensure that final position is really reached
				if ((blindPositionRequested[channel] == 0) || (blindPositionRequested[channel] == 100))
					blindTimeNextState[channel] += blindOffsetTime;

				if (blindForceNextState[channel] == true)
					blindForceNextState[channel] = false;
				break;


			case STOP:
				// calculate current position if next state was forced by toggle/stop
				if (blindForceNextState[channel] == true) {
					blindForceNextState[channel] = false;
					getCurrentPosition(channel);
					blindPositionRequested[channel] = blindPositionActual[channel];
				}
				// or take over requested position if blind has moved for the desired time
				else {

					blindPositionActual[channel] = blindPositionRequested[channel];
					if (blindDirection[channel] == UP) blindAngleActual[channel] = 0;
					else blindAngleActual[channel] = 100;

					// if current position was not known (e.g. due to a reset) and end position are reached, then the current position is known again
					if ((!blindPositionKnown[channel]) && ((blindPositionRequested[channel] == 0) || (blindPositionRequested[channel] == 100))) {
						hmwdebug("Reference position reached. Actual position is known now\n");
						blindPositionKnown[channel] = true;
					}

				}

				// send info message with current position
				hmwmodule->sendInfoMessage(channel, blindPositionActual[channel], 0xFFFFFFFF);

				// switch off the "active" relay
				digitalWrite(blindAct[channel], OFF);

				// debug message
				debugStateChange(blindNextState[channel], channel);

				// Set next state & delay time
				blindCurrentState[channel] = STOP;
				blindNextState[channel] = RELAIS_OFF;
				blindTimeNextState[channel] = now + blindWaitTime;


				if (blindSearchingForRefPosition[channel] == true) {
					hmwdebug("Reference position reached. Moving to target position now.\n");
					hmwdevice.setLevel(channel, blindPositionRequestedSave[channel]*2);
					blindSearchingForRefPosition[channel] = false;
				}

				break;


			case TURN_AROUND:
				// switch on the "active" relay
				digitalWrite(blindAct[channel], ON);
				blindTimeStart[channel] = now;

				// debug message
				debugStateChange(blindNextState[channel], channel);

				blindCurrentState[channel] = TURN_AROUND;
				blindNextState[channel] = MOVE;
				if (blindDirection[channel] == UP)
					blindTimeNextState[channel] = now + blindAngleActual[channel] * config.blinds[channel].blindTimeChangeOver;
				else
					blindTimeNextState[channel] = now + (100 - blindAngleActual[channel]) * config.blinds[channel].blindTimeChangeOver;
				break;


			case SWITCH_DIRECTION:

				// switch off the "active" relay
				digitalWrite(blindAct[channel], OFF);

				// debug message
				debugStateChange(blindNextState[channel], channel);

				// Set current state, next state & delay time
				blindCurrentState[channel] = SWITCH_DIRECTION;
				blindNextState[channel] = WAIT;
				blindForceNextState[channel] = false;
				blindTimeNextState[channel] = now + blindWaitTime;
				break;
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


	// device type: 0x81
	// TODO: Modultyp irgendwo als define
	hmwmodule = new HMWModule(&hmwdevice, &hmwrs485, 0x82);

    delay(1000);
	hmwdebug("Huhu\n");

	for (byte channel = 0; channel < 4; channel++) {
		pinMode(blindAct[channel],OUTPUT);
		digitalWrite(blindAct[channel],OFF);
		pinMode(blindDir[channel],OUTPUT);
		digitalWrite(blindDir[channel],OFF);
		blindNextState[channel] = RELAIS_OFF;
		blindCurrentState[channel] = RELAIS_OFF;
		blindForceNextState[channel] = false;
		blindPositionKnown[channel] = false;
		blindPositionActual[channel] = 0;
		blindSearchingForRefPosition[channel] = false;
		hmwdebug("Channel "); hmwdebug(channel); hmwdebug("\n");
		hmwdebug("TimeTopBottom: "); hmwdebug(blindTimeTopBottom[channel], DEC); hmwdebug("\n");
		hmwdebug("TimeBottomTop: "); hmwdebug(blindTimeBottomTop[channel], DEC); hmwdebug("\n");
		hmwdebug("TimeTurnAround: "); hmwdebug(config.blinds[channel].blindTimeChangeOver, DEC); hmwdebug("\n");
	}



	// send announce message
	hmwmodule->broadcastAnnounce(0);
}


// The loop function is called in an endless loop
void loop()
{

	// Daten empfangen und alles, was zur Kommunikationsschicht gehört
	// processEvent vom Modul wird als Callback aufgerufen

	hmwrs485.loop();

	/* Only for debugging:
	if (Serial.available()) {
		byte read = Serial.read();
		if (read == 1) {
			Serial.println("getLevel");
			Serial.println(hmwdevice.getLevel(0));
		}
		else {
			hmwdevice.setLevel(0, read);
			Serial.print("TargetValue: ");
			Serial.println(read);
		}
	}
	*/


	// Bedienung ueber Button
	handleButton();

	handleBlind();

};






