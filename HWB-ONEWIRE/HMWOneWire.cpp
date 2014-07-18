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
// #define DEBUG_UNO 1

// Do not remove the include below
#include "HMWOneWire.h"

#include <EEPROM.h>

#ifdef DEBUG_UNO
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>
#endif

// OneWire
#include <OneWire.h>

// HM Wired Protokoll
#include "HMWRS485.h"
// default module methods
#include "HMWModule.h"

// Ein paar defines...
#ifdef DEBUG_UNO
  #define RS485_RXD 5
  #define RS485_TXD 6
  #define LED 13        // Signal-LED
#else
  #define LED 4         // Signal-LED
#endif
#define BUTTON 8            // Das Bedienelement
#define RS485_TXEN 2        // Transmit-Enable

#define ONEWIRE_PIN A3

#define EEPROM_SIZE 1024   // TODO: Das muesste es auch zentral geben

#define MAX_SENSORS 10   // maximum number of 1-Wire Sensors
#define DEFAULT_TEMP -273.15   // for unused channels

// config of one sensor
struct sensor_config {
  byte :8;                          // dummy
  byte send_delta_temp;             // Temperaturdifferenz, ab der gesendet wird
  byte :8;
  unsigned int send_min_interval;   // Minimum-Sendeintervall
  unsigned int send_max_interval;   // Maximum-Sendeintervall
  byte address[8];                  // 1-Wire-Adresse
  byte :8;
};

// TODO: Check whether we have enough memory to do the following
sensor_config sensors[MAX_SENSORS];

// currently measured temperature
int currentTemp[MAX_SENSORS];         // Temperatures in °C * 100
// TODO: If we wwant to work with peering, then the config etc. probably needs to be set by peer
// temperature measured on last send
int lastSentTemp[MAX_SENSORS];
// time of last send
long lastSentTime[MAX_SENSORS];


// write config to EEPROM in a hopefully smart way
void writeConfig(){
    byte* ptr;
    byte data;
	// EEPROM lesen und schreiben
	ptr = (byte*)(sensors);
	for(int address = 0; address < sizeof(sensors[0]) * MAX_SENSORS; address++){
	  if(*ptr != EEPROM.read(address + 0x10))
		  EEPROM.write(address + 0x10, *ptr);
	  ptr++;
    };
};


// OneWire
OneWire myWire(ONEWIRE_PIN);


// Sensor Adressen lesen
// Before calling this function, config must have been read
// It only searches for new sensors if there is a free channel
void sensorAddressesGet() {

  byte addr[8];
  byte channel;

  // search for addresses on the bus
  myWire.reset_search();

  while(1) {
    // search free slot
	for(channel = 0; channel < MAX_SENSORS && sensors[channel].address[0] != 0xFF; channel++);
    if(channel == MAX_SENSORS) break;   // no free slot found
    // now channel points to a free slot
	if(!myWire.search(addr)) break;     // no further sensor found
	Serial.print(F("1-Wire Device found:"));
	for( byte i = 0; i < 8; i++) {
	  Serial.write(' ');
	  Serial.print(addr[i], HEX);
	};
	if(OneWire::crc8(addr, 7) != addr[7]) {
	  Serial.println(F("CRC is not valid - ignoring device!"));
	  continue;
    };
	// now we found a valid device, check if this is already known
	byte oldChan;
    for(oldChan = 0; oldChan < MAX_SENSORS; oldChan++) {
       if(memcmp(addr, sensors[oldChan].address, 8) == 0) break;   // found
    };
    if(oldChan < MAX_SENSORS){
  	  Serial.println(F("Device already known"));
  	  continue;
    };
    // we have a new device!
    memcpy(sensors[channel].address, addr, 8);
  };
  // now the devices are in sensors[], write them to the EEPROM
  writeConfig();
};


void setDefaults(){
  // defaults setzen
  for(byte channel = 0; channel < MAX_SENSORS; channel++){
    if(sensors[channel].send_delta_temp == 0xFF) sensors[channel].send_delta_temp = 5;
    if(sensors[channel].send_min_interval == 0xFFFF) sensors[channel].send_min_interval = 10;
    if(sensors[channel].send_max_interval == 0xFFFF) sensors[channel].send_max_interval = 150;
  };
};


// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:
	void setLevel(byte channel,unsigned int level) {
      return;  // there is nothing to set
	}

	unsigned int getLevel(byte channel) {
      // there is only one channel for now
	  int defTemp = DEFAULT_TEMP * 100;
	  if(channel > MAX_SENSORS) return (unsigned int)(defTemp);
	  return currentTemp[channel];
	};

	void readConfig(){
      byte* ptr;
	  // EEPROM lesen
	  ptr = (byte*)(sensors);
	  for(int address = 0; address < sizeof(sensors[0]) * MAX_SENSORS; address++){
	    *ptr = EEPROM.read(address + 0x10);
	    ptr++;
	  };
	  // defaults setzen
	  setDefaults();
	  // nach neuen Sensoren suchen
	  sensorAddressesGet();
	};
};

// The device will be created in setup()
HMWDevice hmwdevice;


#ifdef DEBUG_UNO
SoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
HMWRS485 hmwrs485(&rs485, RS485_TXEN, &Serial);
#else
HMWRS485 hmwrs485(&Serial, RS485_TXEN);  // keine Debug-Ausgaben
#endif
HMWModule* hmwmodule;   // wird in setup initialisiert


// send "start conversion" to device
void oneWireStartConversion(byte channel) {
  // ignore channels without sensor
  if(sensors[channel].address[0] == 0xFF)
	return;
  myWire.reset();
  myWire.select(sensors[channel].address);
  myWire.write(0x44, 1);        // start conversion, with parasite power on at the end
};


float oneWireReadTemp(byte channel) {
   // ignore channels without sensor
   if(sensors[channel].address[0] == 0xFF)
  	 return DEFAULT_TEMP;

	byte data[12];

	  // present = ds.reset();   TODO: what exactly does the "present" do we need it?
	  myWire.reset();
	  myWire.select(sensors[channel].address);
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
	  if (sensors[channel].address[0] == 0x10) {
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
 	currentTemp[currentChannel] = oneWireReadTemp(currentChannel) * 100;
 	currentChannel = (currentChannel + 1) % MAX_SENSORS;
  };
  // start next measurement
  oneWireStartConversion(currentChannel);
  // TODO: myWire.depower() ? what does this mean?
};


void factoryReset() {
  // writes FF into config
  memset(sensors, 0xFF, sizeof(sensors[0]) * MAX_SENSORS);
  // set defaults
  setDefaults();
  // nach neuen Sensoren suchen
  // this will at the end write to EEPROM
  sensorAddressesGet();
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
    	if(now - lastTime > 100)   // send announce on short press
    		hmwmodule->broadcastAnnounce(0);
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

#ifdef DEBUG_UNO
void printChannelConf(){
  for(byte channel = 0; channel < MAX_SENSORS; channel++) {
	 Serial.print("Channel     :"); Serial.println(channel);
 	 Serial.print("Min Interval:"); Serial.println(sensors[channel].send_min_interval);
   	 Serial.print("Max Interval:"); Serial.println(sensors[channel].send_max_interval);
   	 Serial.print("Delta Temp  :"); Serial.println(sensors[channel].send_delta_temp);
   	 Serial.print("Current Temp:"); Serial.println(currentTemp[channel]);
   	 Serial.print("Last    Temp:"); Serial.println(lastSentTemp[channel]);
   	 Serial.print("Size        :"); Serial.println(sizeof(sensors[channel]));
   	 Serial.println();
  }
}
#endif


void setup()
{
#ifdef DEBUG_UNO
	pinMode(RS485_RXD, INPUT);
	pinMode(RS485_TXD, OUTPUT);
#endif
	pinMode(RS485_TXEN, OUTPUT);
	digitalWrite(RS485_TXEN, LOW);

	pinMode(BUTTON, INPUT_PULLUP);
	pinMode(LED, OUTPUT);

#ifdef DEBUG_UNO
   Serial.begin(57600);
   rs485.begin(19200);
#else
   Serial.begin(19200, SERIAL_8E1);
#endif

   // Default temperature is -273.15 (currently...)
   for(byte i = 0; i < MAX_SENSORS; i++) {
	  currentTemp[i] = DEFAULT_TEMP * 100;
      lastSentTemp[i] = DEFAULT_TEMP * 100;
      lastSentTime[i] = 0;
   };

   // config aus EEPROM lesen
   hmwdevice.readConfig();

	// device type: 0x81
	// serial number
	// address
	// TODO: serial number und address sollte von woanders kommen
   // TODO: Modultyp irgendwo als define
 	hmwmodule = new HMWModule(&hmwdevice, &hmwrs485, 0x81, "HHB2703110", 0x42380122);

    hmwrs485.debug("huhu\n");

    // send announce message
	hmwmodule->broadcastAnnounce(0);
#ifdef DEBUG_UNO
	printChannelConf();
#endif
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

 // Pruefen, ob wir irgendwas senden muessen
   long now = millis();
   for(byte channel = 0; channel < MAX_SENSORS; channel++) {
	 // channel has a sensor?
	 if(sensors[channel].address[0] == 0xFF) continue;
	 // do not send before min interval
	 if(sensors[channel].send_min_interval && now - lastSentTime[channel] < (long)(sensors[channel].send_min_interval) * 1000)
		  continue;
     if(    (sensors[channel].send_max_interval && now - lastSentTime[channel] >= (long)(sensors[channel].send_max_interval) * 1000)
    	 || (sensors[channel].send_delta_temp
    	         && abs( currentTemp[channel] - lastSentTemp[channel] ) >= (unsigned int)(sensors[channel].send_delta_temp) * 10)) {
	     hmwmodule->broadcastInfoMessage(channel,currentTemp[channel]);
         lastSentTemp[channel] = currentTemp[channel];
         lastSentTime[channel] = now;
     };
   };
};






