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
#include "HMWHomebrew.h"

#include <EEPROM.h>
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>

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
#define I01 5
#define I02 6
#define O03 7
#define O04 8
#define LED 13

#define CHANNEL_IO_COUNT 4
#define CHANNEL_IO_BYTES 1  //CHANNEL_IO_COUNT / 8

// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
#define CHANNEL_PORTS byte channelPorts[CHANNEL_IO_COUNT] = {I01, I02, O03, O04};

// Port Status, d.h. Port ist auf 0 oder 1
byte portStatus[CHANNEL_IO_BYTES];

// TODO: wird wirklich int gebraucht oder tut's auch byte?
unsigned int keyLongPressTime[2];
byte loggingTime;

// Config
hmw_config config;



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
	  if(channel != 2 && channel != 3) return;
      if(level > 255) return;
      // now set pin
      CHANNEL_PORTS
      if(level == 0xFF) {   // toggle
    	digitalWrite(channelPorts[channel], !digitalRead(channelPorts[channel]));
      }else if(level) // on
    	digitalWrite(channelPorts[channel], HIGH);
      else
    	digitalWrite(channelPorts[channel], LOW);
	}

	unsigned int getLevel(byte channel) {
      // everything in the right limits?
	  if(channel >= CHANNEL_IO_COUNT) return 0;
	  // read
	  CHANNEL_PORTS
	  if(digitalRead(channelPorts[channel])) return 0xC800;
	  else return 0;
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
	};

};



void setModuleConfig(HMWDevice* device) {

// read config from EEPROM
  device->readConfig();

// set input/output
// Input Pins arbeiten mit PULLUP, d.h. muessen per Taster
// auf Masse gezogen werden
  pinMode(I01,INPUT_PULLUP);
  pinMode(I02,INPUT_PULLUP);
  pinMode(O03,OUTPUT);
  digitalWrite(O03,LOW);
  pinMode(O04,OUTPUT);
  digitalWrite(O04,LOW);
}




SoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
HMWRS485 hmwrs485(&rs485, RS485_TXEN, &Serial);
HMWModule* hmwmodule;   // wird in setup initialisiert

//The setup function is called once at startup of the sketch
void setup()
{

	pinMode(RS485_RXD, INPUT);
	pinMode(RS485_TXD, OUTPUT);
	pinMode(RS485_TXEN, OUTPUT);
	digitalWrite(RS485_TXEN, LOW);

	//   timer0 = 255
   Serial.begin(57600);
   rs485.begin(19200);

	// device type: 0x11 = HMW-LC-Sw2-DR
	// serial number
	// address
	// TODO: serial number und address sollte von woanders kommen
    HMWDevice* hmwdevice = new HMWDevice();
	hmwmodule = new HMWModule(hmwdevice, &hmwrs485, 0x11, "HHB2703111", 0x42380123);

// config aus EEPROM lesen
   setModuleConfig(hmwdevice);

  hmwrs485.debug("huhu\n");
}


// Tasten
void handleKeys() {
// TODO: Vielleicht besser eine Klasse HMWKey oder so anlegen
  // millis() zum Zeitpunkt eines Tastendrucks
  // verwendet zum Entprellen, lange Tastendruecke und wiederholtes Senden langer Tastendruecke
  static unsigned long keyPressedMillis[2] = {0,0};   // Wir haben zwei Inputs
  static byte keyPressNum[2] = {0,0};
  // wann wurde das letzte mal "short" gesendet?
  static unsigned long lastSentLong[2] = {0,0};

  long now = millis();

  for(byte i = 0; i < 2; i++){
// INPUT_LOCKED?
   if(!config.keys[i].input_locked) continue;   // inverted logic, locked = 0
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
	   };
	   keyPressedMillis[i] = 0;
	 };
   }else{
// Taste gedrueckt
	 // Taste war vorher schon gedrueckt
	 if(keyPressedMillis[i]){
       // muessen wir ein "long" senden?
	   if(lastSentLong[i]) {   // schon ein LONG gesendet
		  if(now - lastSentLong[i] >= 300){  // alle 300ms wiederholen
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
	   };
	 }else{
	   // Taste war vorher nicht gedrueckt
	   keyPressedMillis[i] = now ? now : 1; // der Teufel ist ein Eichhoernchen
	   lastSentLong[i] = 0;
	 }
   }
  }
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

// Check Keys
// Hier werden alle Ein-/Ausgaenge gelesen
// Pins lesen und nach portStatus[] schreiben
  readPins();

// Tasten abfragen, entprellen etc.
  handleKeys();

  // hmwTxTargetAdress(4)                   the target adress
  // hmwTxFrameControllByte                 the controll byte
  // hmwTxSenderAdress(4)                   the sender adress
  // hmwTxFrameDataLength                   the length of data to send
  // hmwTxFrameData(MAX_RX_FRAME_LENGTH)    the data array to send
}






