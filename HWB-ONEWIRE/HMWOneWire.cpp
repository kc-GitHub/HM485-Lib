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

// Do not remove the include below
#include "HMWOneWire.h"

#include <EEPROM.h>
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>

// OneWire
#include <OneWire.h>

// HM Wired Protokoll
#include "HMWRS485.h"
// default module methods
#include "HMWModule.h"

// Ein paar defines...
#define RS485_RXD 2
#define RS485_TXD 3
#define RS485_TXEN 4
#define BUTTON 5      // Das Bedienelement
#define LED 13        // Signal-LED

#define EEPROM_SIZE 1024   // TODO: Das muesste es auch zentral geben

#define MAX_SENSORS 10   // maximum number of 1-Wire Sensors
#define DEFAULT_TEMP -273.15   // for unused channels

byte sensorAddr[MAX_SENSORS][8]; // Addresses of OneWire Sensor
float tempInCelsius[MAX_SENSORS];         // Temperatures in °C

// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:
	void setLevel(byte channel,unsigned int level) {
      return;  // there is nothing to set
	}

	unsigned int getLevel(byte channel) {
      // there is only one channel for now
	  if(channel > MAX_SENSORS) return (unsigned int)(-27315);
	  return (unsigned int)(tempInCelsius[channel] * 100);
	};

	void readConfig(){
      return;   // gibt's zurzeit nicht
	};

};

// OneWire auf Pin 10
OneWire myWire(10);


void setModuleConfig(HMWDevice* device) {
// read config from EEPROM
  device->readConfig();
}


SoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
HMWRS485 hmwrs485(&rs485, RS485_TXEN, &Serial);
HMWModule* hmwmodule;   // wird in setup initialisiert




// Sensor Adressen lesen
// TODO: Soemthing smarter with EEPROM etc.
void sensorAddressesGet() {
  // first kill all entries in sensorAddr
  memset(sensorAddr, 0, 8 * MAX_SENSORS);
  // search for addresses on the bus
  myWire.reset_search();
  for(int channel = 0; channel < MAX_SENSORS; channel++){
	if(!myWire.search(sensorAddr[channel])) {
	  memset(sensorAddr[channel],0,8);
	  return;
	};
    Serial.print("1-Wire Device found:");
	for( byte i = 0; i < 8; i++) {
	   Serial.write(' ');
	   Serial.print(sensorAddr[channel][i], HEX);
	};
    if(OneWire::crc8(sensorAddr[channel], 7) != sensorAddr[channel][7]) {
	  Serial.println("CRC is not valid - ignoring device!");
      memset(sensorAddr[channel],0,8);
      channel--;
    };
  };
};


// send "start conversion" to device
void oneWireStartConversion(byte channel) {
  // ignore channels without sensor
  if(!sensorAddr[channel][0])
	return;
  myWire.reset();
  myWire.select(sensorAddr[channel]);
  myWire.write(0x44, 1);        // start conversion, with parasite power on at the end
};


float oneWireReadTemp(byte channel) {
   // ignore channels without sensor
   if(!sensorAddr[channel][0])
  	 return DEFAULT_TEMP;

	byte data[12];

	  // present = ds.reset();   TODO: what exactly does the "present" do we need it?
	  myWire.reset();
	  myWire.select(sensorAddr[channel]);
	  myWire.write(0xBE);         // Read Scratchpad

	  for ( byte i = 0; i < 9; i++) {           // we need 9 bytes
	    data[i] = myWire.read();
	  }
	  // Serial.print(OneWire::crc8(data, 8), HEX);  TODO: Check CRC

	  // Convert the data to actual temperature
	  // because the result is a 16 bit signed integer, it should
	  // be stored to an "int16_t" type, which is always 16 bits
	  // even when compiled on a 32 bit processor.
	  int16_t raw = (data[1] << 8) | data[0];
	  if (sensorAddr[channel][0] == 0x10) {
	    raw = raw << 3; // 9 bit resolution default
	    if (data[7] == 0x10) {   // DS18S20 or old DS1820
	      // "count remain" gives full 12 bit resolution
	      raw = (raw & 0xFFF0) + 12 - data[6];
	    }
	  } else {
	    byte cfg = (data[4] & 0x60);
	    // at lower res, the low bits are undefined, so let's zero them
	    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
	    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
	    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
	    // default is 12 bit resolution, 750 ms conversion time
	  }
	  return (float)raw / 16.0;
};


// handle 1-Wire temperature measurement
// measurement every 10 seconds, one measurement needs about 1 second
// we have up to 10 sensors, so give every one 1 second
void handleOneWire() {

  static byte currentChannel = 255;  // we should never have 255 channels
  static long lastTime = 0;
  long now = millis();

  // once every 1 seconds
  if(now -lastTime < 1000) return;
  lastTime = now;

  if(currentChannel == 255){  // special for the first call
	currentChannel = 0;
  }else{
	// read temperature
 	tempInCelsius[currentChannel] = oneWireReadTemp(currentChannel);
 	currentChannel = (currentChannel + 1) % MAX_SENSORS;
  };
  // start next measurement
  oneWireStartConversion(currentChannel);
  // TODO: myWire.depower() ? what does this mean?
};


void factoryReset() {
// writes FF into the EEPROM
  for(int addr = 0; addr < EEPROM_SIZE; addr++) {
	if(EEPROM.read(addr) != 0xFF)
		EEPROM.write(addr, 0xFF);
  }
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
      if(buttonState) status = 1;
      lastTime = now;
      break;
    case 1:
      if(buttonState) {   // immer noch gedrueckt
        if(now - lastTime > 5000) status = 2;
      }else{              // nicht mehr gedrückt
        status = 0;
      };
      break;
    case 2:
      if(!buttonState) {  // losgelassen
    	status = 3;
    	lastTime = now;
      };
      break;
    case 3:
      // wait at least 100ms
      if(now - lastTime < 100)
    	break;
      if(buttonState) {   // zweiter Tastendruck
    	status = 4;
    	lastTime = now;
      }else{              // noch nicht gedrueckt
    	if(now - lastTime > 10000) status = 0;    // give up
      };
      break;
    case 4:
      if(now - lastTime < 100) // entprellen
          	break;
      if(buttonState) {   // immer noch gedrueckt
        if(now - lastTime > 3000) status = 5;
      }else{              // nicht mehr gedrückt
        status = 0;
      };
      break;
    case 5:   // zweiter Druck erkannt
      if(!buttonState) {    //erst wenn losgelassen
    	// Factory-Reset          !!!!!!  TODO: Gehoert das ins Modul?
    	factoryReset();
    	status = 0;
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
	pinMode(RS485_RXD, INPUT);
	pinMode(RS485_TXD, OUTPUT);
	pinMode(RS485_TXEN, OUTPUT);
	digitalWrite(RS485_TXEN, LOW);

	pinMode(BUTTON, INPUT_PULLUP);
	pinMode(LED, OUTPUT);

	//   timer0 = 255
   Serial.begin(57600);
   rs485.begin(19200);

   // Default temperature is -273.15 (currently...)
   for(byte i = 0; i < MAX_SENSORS; i++)
	  tempInCelsius[i] = DEFAULT_TEMP;

   // get 1-Wire Address
   sensorAddressesGet();

	// device type: 0x11 = HMW-LC-Sw2-DR
	// serial number
	// address
	// TODO: serial number und address sollte von woanders kommen
    // TODO: Das 1-Wire Teil braucht hier was Spezielles
   // TODO: Modultyp irgendwo als define
    HMWDevice* hmwdevice = new HMWDevice();
	hmwmodule = new HMWModule(hmwdevice, &hmwrs485, 0x81, "HHB2703110", 0x42380122);

// config aus EEPROM lesen
    setModuleConfig(hmwdevice);

    hmwrs485.debug("huhu\n");
}


// The loop function is called in an endless loop
void loop()
{
// Daten empfangen (tut nichts, wenn keine Daten vorhanden)
   hmwrs485.receive();
 // Check
   if(hmwrs485.frameComplete) {
      if(hmwrs485.targetAddress == hmwrs485.txSenderAddress || hmwrs485.targetAddress == 0xFFFFFFFF){
        hmwrs485.parseFrame();
        hmwmodule->processEvents();
      }
   };

 // Temperatur lesen
   handleOneWire();

 // Bedienung ueber Button
   handleButton();

 // zweimal pro Minute ein "A" und die Temperatur senden
   static long last = 0;
   static byte num = 0;
   long now = millis();
   if(now - last > 30000){
	   for(byte channel = 0; channel < MAX_SENSORS; channel++) {
	     Serial.println(tempInCelsius[channel]);
	     hmwmodule->broadcastInfoMessage(channel,(unsigned int)(tempInCelsius[channel] * 100));
	   };
	   hmwmodule->broadcastAnnounce(0);   // only once
	   last = now;
   }
}






