//*******************************************************************
//
// HMWSecMDIR.cpp
//
// Homematic Wired Hombrew Hardware for Motion Detection Infrared Adapter
//
// The Motion Detection Infrared Adapter become an in wall device which
// adapts Motion Detection Infrared Sensors to homematic bus protocol.
// Supported Sensors are from Jung, Berker, Gira.
//
// Infrared Sensor pin layout is described in http://www.mikrocontroller.net/articles/24V_UP-Einsatz_f%C3%BCr_Bewegungsmelder_von_Jung,_Berker_und_Gira
//
// First Prototype is developed on Arduino Uno
// basic structure of this file is copied from HMW-LC-Sw2-DR
// Thanks to
// Thorsten Pferdekaemper (thorsten@pferdekaemper.com)
// and
// Dirk Hoffmann (hoffmann@vmd-jena.de)
//
// (C) Rene Fischer (renefischer@fischer-homenet.de)
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
// D5: motion detection/ light on (Pin 4) / Kanal 1
// D6: light off (Pin6)  / Kanal 2
#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
// D7: Schalter 1 / Kanal 3 only for compatibility
// D8: Schalter 2 / Kanal 4 only for compatibility
#endif
// D12: Sensor clock 50 Hz (Pin 1)
// D13: Status-LED

// Die Firmware funktioniert mit dem Standard-Uno Bootloader, im
// Gegensatz zur Homematic-Firmware

//*******************************************************************

// Do not remove the include below
#include "HBWSecMDIR.h"

#include <EEPROM.h>
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>
#include <MsTimer2.h>

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

#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
#define I01 A0
#define I02 A1
#define CHANNEL_IO_COUNT 4
#else
#define MD_LON A0
#define LOFF A1
#define CHANNEL_IO_COUNT 2
#endif
#define SENS_CLOCK 12
#define LED 13

#define MOTION_STATUS_CHANNEL 0
#define LIGHT_STATUS_CHANNEL 1
#define STATE_MASK 3
#define CHANNEL_IO_BYTES 1  //CHANNEL_IO_COUNT / 8

#define SENS_HIGH_PEGEL 500

// Das folgende Define kann benutzt werden, wenn ueber die
// Kanaele "geloopt" werden soll
// als Define, damit es zentral definiert werden kann, aber keinen (globalen) Speicherplatz braucht
//#define CHANNEL_PORTS byte channelPorts[CHANNEL_IO_COUNT] = { CHANNEL_INIT_LIST};

// Port Status, d.h. Port ist auf 0 oder 1
byte portState = 0;
byte lastState = 0;
// TODO: wird wirklich int gebraucht oder tut's auch byte?
//unsigned int keyLongPressTime[2];
//byte loggingTime;

// Config
hmw_config config;


#if 0
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
#endif

// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:
	void setLevel(byte channel,unsigned int level) {
	  // everything in the right limits?
#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
	  if(channel != 2 && channel != 3) return;
      if(level > 255) return;
      if (channel >= 8) return;
      // now set pin

      if(level == 0xFF) {   // toggle
    	level =  ( (portState >> channel) & 0x1 );
      }
      else if(level) // on
    	portState |= ( HIGH << channel );
      else
    	portState &= ~( HIGH << channel );
#else
      return;
#endif
	}

	unsigned int getLevel(byte channel) {
      // everything in the right limits?
	  if(channel >= 8) return 0;
	  // read

	  if( (portState & (1 << channel) ) ) return 0xC800;
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

#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
  pinMode(O03,OUTPUT);
  digitalWrite(O03,LOW);
  pinMode(O04,OUTPUT);
  digitalWrite(O04,LOW);

  pinMode(SENS_CLOCK,OUTPUT);
  digitalWrite(SENS_CLOCK,LOW);
#else

  pinMode(SENS_CLOCK,OUTPUT);
  digitalWrite(SENS_CLOCK,LOW);
#endif
}

SoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
HMWRS485 hmwrs485(&rs485, RS485_TXEN, &Serial);
HMWModule* hmwmodule;   // wird in setup initialisiert

void setLightState(bool on)
{
	if (on)
	{
		bitSet(portState,LIGHT_STATUS_CHANNEL);
	}else
	{
		bitClear(portState,LIGHT_STATUS_CHANNEL);
	}


}
void setMDState(bool on)
{
	if (on)
	{
		bitSet(portState,MOTION_STATUS_CHANNEL);
	}else
	{
		bitClear(portState,MOTION_STATUS_CHANNEL);
	}

}

void checkSensor(void)
{
	static uint8_t md_lon_count = 0;
	static uint8_t loff_count = 0;
	static uint16_t md_count;

	/*Prüfe auf Impuls von ADC0 (Ein, Bewegung) */
	int tempVal = analogRead(MD_LON);

	if (tempVal > SENS_HIGH_PEGEL)
	{
	      md_lon_count ++;
	}
	else
	{
	      if (md_lon_count > 1 && md_lon_count < 4) /*kurzer Impuls -> Bewegung */
	      {
	    	  md_count = 600;                                   /*6 Sekunden */
	      }else if (md_lon_count > 4)               /*langer Impuls -> Licht ein */
	      {
	    	  setLightState(true);
	      }
	      md_lon_count = 0;
	}

	/*Prüfe auf Impuls von ADC1 (Aus) */
	tempVal = analogRead(LOFF);
	if(tempVal > SENS_HIGH_PEGEL)
	{
		loff_count++;
	}else
	{
		if (loff_count > 4)                                 /*langer Impuls -> Licht aus */
	         setLightState(false);

	    loff_count = 0;
	}

	/*Signal Bewegung */
	if( md_count > 0)
	{     setMDState(true);
	      md_count--;
	}else
	{     setMDState(false);
	}

	/*Takt für BWM */
	if ( digitalRead(SENS_CLOCK) == LOW)
	{
		digitalWrite(SENS_CLOCK,HIGH);
	}else
	{
		digitalWrite(SENS_CLOCK,LOW);
	}

}




//The setup function is called once at startup of the sketch
void setup()
{
	portState = 0;

	pinMode(RS485_RXD, INPUT);
	pinMode(RS485_TXD, OUTPUT);
	pinMode(RS485_TXEN, OUTPUT);
	digitalWrite(RS485_TXEN, LOW);

	//   timer0 = 255
	Serial.begin(57600);
	rs485.begin(19200);

	// TODO change device type to own type, use HMW-LC-SW2-DR to work with fhem
	// device type: 0x11 = HMW-LC-Sw2-DR
	// serial number
	// address
	// TODO: serial number und address sollte von woanders kommen
    HMWDevice* hmwdevice = new HMWDevice();
	hmwmodule = new HMWModule(hmwdevice, &hmwrs485, 0x11, "HHB2703111", 0x42380123);

	// config aus EEPROM lesen
	setModuleConfig(hmwdevice);

	hmwrs485.debug("huhu\n");

  	MsTimer2::set(500, checkSensor); // 500ms period
  	MsTimer2::start();
}

#if 0
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
	     // gleich ein Announce hinterher
	     // TODO: Vielleicht gehoert das in den allgemeinen Teil
	     hmwmodule->broadcastAnnounce(i);
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
	      // gleich ein Announce hinterher
		  // TODO: Vielleicht gehoert das in den allgemeinen Teil
		  hmwmodule->broadcastAnnounce(i);
	   };
	 }else{
	   // Taste war vorher nicht gedrueckt
	   keyPressedMillis[i] = now ? now : 1; // der Teufel ist ein Eichhoernchen
	   lastSentLong[i] = 0;
	 }
   }
  }
}
#endif

// The loop function is called in an endless loop
void loop()
{
   static byte keyPressNum[] = {0,0};
// Daten empfangen (tut nichts, wenn keine Daten vorhanden)
   hmwrs485.receive();
 // Check
   if(hmwrs485.frameComplete) {
      if(hmwrs485.targetAddress == hmwrs485.txSenderAddress || hmwrs485.targetAddress == 0xFFFFFFFF){
        hmwrs485.parseFrame();
        hmwmodule->processEvents();
      }
   };


   if (lastState != (portState & STATE_MASK) )
   {
	   if ( bitRead(lastState,LIGHT_STATUS_CHANNEL) != bitRead(portState,LIGHT_STATUS_CHANNEL))
       {
		   hmwrs485.debug("change LightState\n");
		   // TODO: muss das eigentlich an die Zentrale gehen?
		   hmwmodule->broadcastKeyEvent(LIGHT_STATUS_CHANNEL,keyPressNum[LIGHT_STATUS_CHANNEL]);
		   // gleich ein Announce hinterher
		   // TODO: Vielleicht gehoert das in den allgemeinen Teil
		   hmwmodule->broadcastAnnounce(LIGHT_STATUS_CHANNEL);

		   keyPressNum[LIGHT_STATUS_CHANNEL]++;
       }

	   if ( bitRead(lastState,MOTION_STATUS_CHANNEL) != bitRead(portState,MOTION_STATUS_CHANNEL))
	   {
		   hmwrs485.debug("change LightState\n");
	       // TODO: muss das eigentlich an die Zentrale gehen?
	       hmwmodule->broadcastKeyEvent(MOTION_STATUS_CHANNEL,keyPressNum[MOTION_STATUS_CHANNEL]);
	       // gleich ein Announce hinterher
	       // TODO: Vielleicht gehoert das in den allgemeinen Teil
	      hmwmodule->broadcastAnnounce(MOTION_STATUS_CHANNEL);

	      keyPressNum[MOTION_STATUS_CHANNEL]++;
	   }
	   lastState = portState & STATE_MASK;
   }
// Check Keys
// Hier werden alle Ein-/Ausgaenge gelesen
// Pins lesen und nach portStatus[] schreiben
  //readPins();

// Tasten abfragen, entprellen etc.
  //handleKeys();

  // hmwTxTargetAdress(4)                   the target adress
  // hmwTxFrameControllByte                 the controll byte
  // hmwTxSenderAdress(4)                   the sender adress
  // hmwTxFrameDataLength                   the length of data to send
  // hmwTxFrameData(MAX_RX_FRAME_LENGTH)    the data array to send
}





