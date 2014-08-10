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

// Die "DEBUG_UNO"-Version ist fuer einen normalen Arduino Uno (oder so)
// gedacht. Dadurch wird RS485 ueber SoftwareSerial angesteuert
// und der Hardware-Serial-Port wird fuer Debug-Ausgaben benutzt
// Dadurch kann man die normale USB-Verbindung zum Programmieren und
// debuggen benutzen

#define DEBUG_NONE 0   // Kein Debug-Ausgang, RS485 auf Hardware-Serial
#define DEBUG_UNO 1    // Hardware-Serial ist Debug-Ausgang, RS485 per Soft auf pins 5/6
#define DEBUG_UNIV 2   // Hardware-Serial ist RS485, Debug per Soft auf pins 5/6


// Do not remove the include below
#include "HBWSecMDIR.h"

#if DEBUG_VERSION == DEBUG_UNO || DEBUG_VERSION == DEBUG_UNIV
// TODO: Eigene SoftwareSerial
#include <SoftwareSerial.h>
#endif

// debug-Funktion
#include "HMWDebug.h"

#include <MsTimer2.h>

#include <EEPROM.h>

// HM Wired Protokoll
#include "HMWRS485.h"
// default module methods
#include "HMWModule.h"

// Ein paar defines...
#if DEBUG_VERSION == DEBUG_UNO
  #define RS485_RXD 2
  #define RS485_TXD 3
  #define LED 13
#elif DEBUG_VERSION == DEBUG_UNIV
  #define DEBUG_RXD 2
  #define DEBUG_TXD 3
  #define LED 13
#else
  #define LED 13        // Signal-LED
#endif

#define RS485_TXEN 4

#ifndef SENSOR_CHECK_INTERVALL_MS
#define SENSOR_CHECK_INTERVALL_MS 10
#endif

#ifndef MD_OFF_DELAY_MS
#define MD_OFF_DELAY_MS 6000
#endif

#ifndef EXT_BUTTON_LON_OFF_DELAY_MS
#define EXT_BUTTON_LON_DELAY_MS 3000
#endif

#ifndef EXT_BUTTON_LOFF_DELAY_MS
#define EXT_BUTTON_LOFF_DELAY_MS 60000
#endif

#ifndef SENS_HIGH_PEGEL
#define SENS_HIGH_PEGEL 500
#endif

/*
 * PORTS
 */
#define MD_LON A0
#define LOFF A1
#define SENS_CLOCK 12
#define EXT_BUTTON 11

/*
 * IO DATA
 */
#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
#define CHANNEL_IO_COUNT 4
#else
#define CHANNEL_IO_COUNT 2
#endif

#define MOTION_STATUS_CHANNEL 0
#define LIGHT_STATUS_CHANNEL 1
#define STATE_MASK 3

//#define CHANNEL_IO_BYTES ( CHANNEL_IO_COUNT / 8 )
#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
#define DEVICE_TYPE 0x11
#else
#define DEVICE_TYPE 0x91
#endif

#if 0
// Anzahl Tastereingaenge
#define HMW_CONFIG_NUM_KEYS 2

// Taster
struct hmw_config_key {
	byte input_type            :1;   // 0x07:0    0=SWITCH, 1=PUSHBUTTON
	byte input_locked          :1;   // 0x07:1    0=LOCKED, 1=UNLOCKED
	byte                       :6;   // 0x07:2-7
	byte long_press_time;            // 0x08
};

// Schalter (Aktoren)
#define HMW_CONFIG_NUM_SWITCHES 2

struct hmw_config_switch {
	byte logging:1;    // 0x0B:0     0=OFF, 1=ON
	byte        :7;    // 0x0B:1-7
	byte        :8;    // 0x0C      // dummy     //TODO: Optimize (?)
};
#endif

struct hmw_config {
#if 0
	byte logging_time;     // 0x01
	long central_address;  // 0x02 - 0x05
	byte direct_link_deactivate:1;   // 0x06:0
	byte                       :7;   // 0x06:1-7
    hmw_config_key keys[HMW_CONFIG_NUM_KEYS];  // 0x07 - 0x0A
    hmw_config_switch switches[HMW_CONFIG_NUM_SWITCHES];  // 0x0B - 0x0E
#endif
    byte :8;                          // dummy
    byte send_delta_temp;             // Temperaturdifferenz, ab der gesendet wird
    byte :8;
    unsigned int send_min_interval;   // Minimum-Sendeintervall
    unsigned int send_max_interval;   // Maximum-Sendeintervall
    byte address[8];                  // 1-Wire-Adresse
    byte :8;
}config;

// Port Status, d.h. Port ist auf 0 oder 1
byte channelState = 0;
byte lastState = 0;


// Klasse fuer Callbacks vom Protokoll
class HMWDevice : public HMWDeviceBase {
  public:
	void setLevel(byte channel,unsigned int level) {
	  // everything in the right limits?
#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
	  if(channel != 2 && channel != 3) return;
      if(level > 255) return;
      if (channel >= CHANNEL_IO_COUNT) return;
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

	pinMode(EXT_BUTTON, INPUT_PULLUP);
}

#if DEBUG_VERSION == DEBUG_UNO
SoftwareSerial rs485(RS485_RXD, RS485_TXD); // RX, TX
HMWRS485 hmwrs485(&rs485, RS485_TXEN);
#elif DEBUG_VERSION == DEBUG_UNIV
HMWRS485 hmwrs485(&Serial, RS485_TXEN);
#else
HMWRS485 hmwrs485(&Serial, RS485_TXEN);  // keine Debug-Ausgaben
#endif
HMWModule* hmwmodule;   // wird in setup initialisiert
enum lightstate
{
	off,
	on
};

#define isLightOn() (bitRead(channelState,LIGHT_STATUS_CHANNEL) == 1)

void setLightState(lightstate state)
{
	if (state == on)
	{
		bitSet(channelState,LIGHT_STATUS_CHANNEL);
		hmwdebug("bitSet Light on\n");
	}else if (state == off)
	{
		bitClear(channelState,LIGHT_STATUS_CHANNEL);
		hmwdebug("bitSet Light off\n");
	}


}
void setMDState(bool on)
{
	if (on)
	{
		bitSet(channelState,MOTION_STATUS_CHANNEL);
		hmwdebug("bitSet Motion on\n");
	}else
	{
		bitClear(channelState,MOTION_STATUS_CHANNEL);
		//hmwdebug("bitSet Motion off\n");
	}

}

#define EXT_BUTTON_LIGHT_OFF 		0
#define EXT_BUTTON_WAIT_LIGHT_ON 	1
#define EXT_BUTTON_LIGHT_ON 		2
#define EXT_BUTTON_WAIT_LIGHT_OFF 	3

uint8_t button_state = EXT_BUTTON_LIGHT_OFF;

void checkButton(void)
{
	static uint16_t delay;

	switch (button_state)
	{

	case EXT_BUTTON_WAIT_LIGHT_ON:
		if (digitalRead(EXT_BUTTON) == LOW)
		{
			delay--;
			if (delay == 0)
			{
				setLightState(on);
				button_state = EXT_BUTTON_LIGHT_ON;
				hmwdebug("ext button Light on\n");
			}
		}
		break;
	case EXT_BUTTON_LIGHT_ON:
		if (digitalRead(EXT_BUTTON) == HIGH)
		{
			delay = EXT_BUTTON_LOFF_DELAY_MS / SENSOR_CHECK_INTERVALL_MS;
			button_state = EXT_BUTTON_WAIT_LIGHT_OFF;
			hmwdebug("ext button wait Light off\n");
		}
		break;
	case EXT_BUTTON_WAIT_LIGHT_OFF:
		delay--;
		if (delay == 0)
		{
			setLightState(off);
			button_state = EXT_BUTTON_LIGHT_OFF;
			hmwdebug("ext button Light off\n");
		}
		break;
	case EXT_BUTTON_LIGHT_OFF:
	default:
		if (digitalRead(EXT_BUTTON) == LOW)
		{
			delay = EXT_BUTTON_LON_DELAY_MS / SENSOR_CHECK_INTERVALL_MS;
			button_state = EXT_BUTTON_WAIT_LIGHT_ON;
			hmwdebug("ext button wait Light on\n");
		}
		break;
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
	    	  md_count = MD_OFF_DELAY_MS / SENSOR_CHECK_INTERVALL_MS;                                   /*6 Sekunden */
	      }else if (md_lon_count > 4)               /*langer Impuls -> Licht ein */
	      {
	    	  setLightState(on);
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
	         setLightState(off);

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
void timerLoop(void)
{
	checkSensor();
	checkButton();
}
//The setup function is called once at startup of the sketch
void setup()
{
	channelState = 0;

#if DEBUG_VERSION == DEBUG_UNO
	pinMode(RS485_RXD, INPUT);
	pinMode(RS485_TXD, OUTPUT);
#elif DEBUG_VERSION == DEBUG_UNIV
	pinMode(DEBUG_RXD, INPUT);
	pinMode(DEBUG_TXD, OUTPUT);
#endif
	pinMode(RS485_TXEN, OUTPUT);
	digitalWrite(RS485_TXEN, LOW);

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

	// TODO change device type to own type, use HMW-LC-SW2-DR to work with fhem
	// device type: 0x11 = HMW-LC-Sw2-DR
	// serial number
	// address
	// TODO: serial number und address sollte von woanders kommen
    HMWDevice* hmwdevice = new HMWDevice();
	hmwmodule = new HMWModule(hmwdevice, &hmwrs485, DEVICE_TYPE);

	// config aus EEPROM lesen
	setModuleConfig(hmwdevice);

	hmwdebug("huhu\n");

  	MsTimer2::set(SENSOR_CHECK_INTERVALL_MS, timerLoop); // 500ms period
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
	   hmwdebug("LastState");
	   hmwdebug(channelState);
	   hmwdebug("\n");
	   if ( bitRead(lastState,LIGHT_STATUS_CHANNEL) != bitRead(channelState,LIGHT_STATUS_CHANNEL))
       {
		   hmwdebug("change LightState");
		   hmwdebug(bitRead(channelState,LIGHT_STATUS_CHANNEL));
		   hmwdebug("\n");
		   // TODO: muss das eigentlich an die Zentrale gehen?
#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
		   hmwmodule->broadcastKeyEvent(LIGHT_STATUS_CHANNEL,keyPressNum[LIGHT_STATUS_CHANNEL]);
#else
		   hmwmodule->broadcastInfoMessage(LIGHT_STATUS_CHANNEL,bitRead(channelState,LIGHT_STATUS_CHANNEL));
#endif
		   // gleich ein Announce hinterher
		   // TODO: Vielleicht gehoert das in den allgemeinen Teil
		   hmwmodule->broadcastAnnounce(LIGHT_STATUS_CHANNEL);

		   keyPressNum[LIGHT_STATUS_CHANNEL]++;
       }

	   if ( bitRead(lastState,MOTION_STATUS_CHANNEL) != bitRead(channelState,MOTION_STATUS_CHANNEL))
	   {
		   hmwdebug("change MotionState\n");
		   hmwdebug(bitRead(channelState,MOTION_STATUS_CHANNEL));
		   hmwdebug("\n");
	       // TODO: muss das eigentlich an die Zentrale gehen?
#ifdef HBW_SEC_MDIR_IS_HMW_LC_SW2
		   hmwmodule->broadcastKeyEvent(MOTION_STATUS_CHANNEL,keyPressNum[MOTION_STATUS_CHANNEL]);
#else
		   hmwmodule->broadcastInfoMessage(MOTION_STATUS_CHANNEL,bitRead(channelState,MOTION_STATUS_CHANNEL));
#endif
		   // gleich ein Announce hinterher
	       // TODO: Vielleicht gehoert das in den allgemeinen Teil
	      hmwmodule->broadcastAnnounce(MOTION_STATUS_CHANNEL);

	      keyPressNum[MOTION_STATUS_CHANNEL]++;
	   }
	   lastState = channelState & STATE_MASK;
   }

}





