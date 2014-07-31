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
// A0: motion detection/ light on (Pin 4) / Kanal 1
// A1: light off (Pin6)  / Kanal 2
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
#define SENSOR_CHECK_INTERVALL 500

#define MD_LON A0
#define LOFF A1

#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
#define CHANNEL_IO_COUNT 4
#else
#define CHANNEL_IO_COUNT 2
#endif

#define SENS_CLOCK 12
#define LED 13

#define MOTION_STATUS_CHANNEL 0
#define LIGHT_STATUS_CHANNEL 1
#define STATE_MASK 3
#define CHANNEL_IO_BYTES 1  //CHANNEL_IO_COUNT / 8

#define SENS_HIGH_PEGEL 500

// Port Status, d.h. Port ist auf 0 oder 1
byte channelState = 0;
byte lastState = 0;

// Config
hmw_config config;

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
    	level =  ( (channelState >> channel) & 0x1 );
      }
      else if(level) // on
    	channelState |= ( HIGH << channel );
      else
    	channelState &= ~( HIGH << channel );
#else
      return;
#endif
	}

	unsigned int getLevel(byte channel) {
      // everything in the right limits?
	  if(channel >= 8) return 0;
	  // read

	  if( (channelState & (1 << channel) ) ) return 0xC800;
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

	};

};



void setModuleConfig(HMWDevice* device) {

	// read config from EEPROM
	device->readConfig();

	pinMode(SENS_CLOCK,OUTPUT);
	digitalWrite(SENS_CLOCK,LOW);

#ifdef HBW_SECMDIR_USE_STATE_LED
	pinMode(LED,OUTPUT);
	digitalWrite(LED,HIGH);
#endif
}

SoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
HMWRS485 hmwrs485(&rs485, RS485_TXEN, &Serial);
HMWModule* hmwmodule;   // wird in setup initialisiert

void setLightState(bool on)
{
	if (on)
	{
		bitSet(channelState,LIGHT_STATUS_CHANNEL);
	}else
	{
		bitClear(channelState,LIGHT_STATUS_CHANNEL);
	}


}
void setMDState(bool on)
{
	if (on)
	{
		bitSet(channelState,MOTION_STATUS_CHANNEL);
	}else
	{
		bitClear(channelState,MOTION_STATUS_CHANNEL);
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
	channelState = 0;

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

  	MsTimer2::set(SENSOR_CHECK_INTERVALL, checkSensor); // 500ms period
  	MsTimer2::start();
}


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


   if (lastState != (channelState & STATE_MASK) )
   {
	   if ( bitRead(lastState,LIGHT_STATUS_CHANNEL) != bitRead(channelState,LIGHT_STATUS_CHANNEL))
       {
		   hmwrs485.debug("change LightState\n");
		   // TODO: muss das eigentlich an die Zentrale gehen?
		   hmwmodule->broadcastKeyEvent(LIGHT_STATUS_CHANNEL,keyPressNum[LIGHT_STATUS_CHANNEL]);
		   // gleich ein Announce hinterher
		   // TODO: Vielleicht gehoert das in den allgemeinen Teil
		   hmwmodule->broadcastAnnounce(LIGHT_STATUS_CHANNEL);

		   keyPressNum[LIGHT_STATUS_CHANNEL]++;
       }

	   if ( bitRead(lastState,MOTION_STATUS_CHANNEL) != bitRead(channelState,MOTION_STATUS_CHANNEL))
	   {
		   hmwrs485.debug("change LightState\n");
	       // TODO: muss das eigentlich an die Zentrale gehen?
	       hmwmodule->broadcastKeyEvent(MOTION_STATUS_CHANNEL,keyPressNum[MOTION_STATUS_CHANNEL]);
	       // gleich ein Announce hinterher
	       // TODO: Vielleicht gehoert das in den allgemeinen Teil
	      hmwmodule->broadcastAnnounce(MOTION_STATUS_CHANNEL);

	      keyPressNum[MOTION_STATUS_CHANNEL]++;
	   }
	   lastState = channelState & STATE_MASK;
   }

}





