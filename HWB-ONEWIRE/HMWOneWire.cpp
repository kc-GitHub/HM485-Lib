//*******************************************************************
//
// HMWHomebrew.cpp
//
// Homematic Wired Hombrew Hardware
// Arduino Uno als Homematic-Device
// HMW-HB-ARDUINO-UNO
// Thorsten Pferdekaemper (thorsten@pferdekaemper.com)
// nach einer Vorlage von
// Dirk Hoffmann (hoffmann@vmd-jena.de)
//
//-------------------------------------------------------------------
//Hardwarebeschreibung:
// =====================
//
// Kompatibel mit HMW-LC-Sw2-DR
// Pinsettings for Arduino Uno
//
// D0: RXD, normaler Serieller Port fuer Debug-Zwecke
// D1: TXD, normaler Serieller Port fuer Debug-Zwecke
// D2: RXD, RO des RS485-Treiber
// D3: TXD, DI des RS485-Treiber
// D4: Direction (DE/-RE) Driver/Receiver Enable vom RS485-Treiber
//
// D5: Taster 1   / Kanal 1
// D6: Taster 2   / Kanal 2
// D7: Schalter 1 / Kanal 3
// D8: Schalter 2 / Kanal 4
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

// Datenstrukturen fuer Konfiguration
#include "HMWRegister.h"
// HM Wired Protokoll
#include "HMWRS485.h"
// default module methods
#include "HMWModule.h"

// Ein paar defines...
#define RS485_RXD 2
#define RS485_TXD 3
#define RS485_TXEN 4
#define LED 13

byte loggingTime;

// Config
hmw_config config;


// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:
	void setLevel(byte channel,unsigned int level) {
      return;  // there is nothing to set
	}

	unsigned int getLevel(byte channel) {
      // there is only one channel for now
	  if(channel > 0) return 0;
	  // read
	  // TODO: return remperature here
	  // TODO: unsigned int does not make sense
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


byte sensorAddr[8]; // Addresses of OneWire Sensor
float tempInCelsius = 0;  // Temperature in °C


// Sensor Adresse lesen
// Es wird davon ausgegangen, dass genau 1 Device am Bus haengt
void sensorAddressGet() {

    myWire.reset_search();
	if( !myWire.search(sensorAddr)) {
	  Serial.println("No 1-Wire device found.");
	  return;
	};
    Serial.print("ROM =");
	for( byte i = 0; i < 8; i++) {
	   Serial.write(' ');
	   Serial.print(sensorAddr[i], HEX);
	};
    if(OneWire::crc8(sensorAddr, 7) != sensorAddr[7]) {
	  Serial.println("CRC is not valid!");
	  return;
	};
};


// send "start conversion" to device
void oneWireStartConversion() {
  myWire.reset();
  myWire.select(sensorAddr);
  myWire.write(0x44, 1);        // start conversion, with parasite power on at the end
};


float oneWireReadTemp() {

	byte data[12];

	  // present = ds.reset();   TODO: what exactly does the "present" do we need it?
	  myWire.reset();
	  myWire.select(sensorAddr);
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
	  if (sensorAddr[0] == 0x10) {
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
void handleOneWire() {

  static boolean waitingForResult = false;
  static long lastTime = 0;
  long now = millis();

  // waiting for a result?
  if(waitingForResult) {
	if(now - lastTime > 1000){    // 1 second should be enough, even 750ms should be
	  tempInCelsius = oneWireReadTemp();
	  waitingForResult = false;
	  lastTime = now;
	};
  }else{                        // waiting for next measurement
	if(now - lastTime > 10000) {  // about every 10 secs or so
	  oneWireStartConversion();
	  waitingForResult = true;
	  lastTime = now;
	};
  };
  // TODO: myWire.depower() ? what does this mean?
};



void setup()
{
	pinMode(RS485_RXD, INPUT);
	pinMode(RS485_TXD, OUTPUT);
	pinMode(RS485_TXEN, OUTPUT);
	digitalWrite(RS485_TXEN, LOW);

	//   timer0 = 255
   Serial.begin(57600);
   rs485.begin(19200);

   // get 1-Wire Address
   sensorAddressGet();

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

 // zweimal pro Minute ein "A" und die Temperatur senden
   static long last = 0;
   static byte num = 0;
   long now = millis();
   if(now - last > 30000){
	   // TODO: negative temperatures... etc.
	   Serial.println(tempInCelsius);
	   hmwmodule->broadcastInfoMessage(20,(unsigned int)(tempInCelsius * 10));
	   hmwmodule->broadcastAnnounce(0);
	   last = now;
   }
}






