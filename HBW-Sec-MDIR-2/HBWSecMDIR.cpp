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


// Ein paar defines...
#if DEBUG_VERSION == DEBUG_UNO
  #define RS485_RXD 2
  #define RS485_TXD 3
#elif DEBUG_VERSION == DEBUG_UNIV
  #define DEBUG_RXD 2
  #define DEBUG_TXD 3
#else

#endif

#define RS485_TXEN 4

#ifndef SENSOR_CHECK_INTERVALL_MS
#define SENSOR_CHECK_INTERVALL_MS 10
#endif

#ifndef MD_OFF_DELAY_MS
#define MD_OFF_DELAY_MS 6000
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


/*
 * IO DATA
 */
#define MOTION_STATUS_CHANNEL 2
#define LIGHT_STATUS_CHANNEL 1


#define DEVICE_TYPE 0x91


byte channelState = 0;
byte lastState = 0;

HMWDevice::HMWDevice()
{
	//mode.setIFactoryReset(this);
}
void HMWDevice::setLevel(byte channel,unsigned int level) {
	  // everything in the right limits?
      return;

	}

unsigned int HMWDevice::getLevel(byte channel) {
      // everything in the right limits?
	  if(channel >= 8) return 0;
	  // read

	  if( (channelState & (1 << channel) ) ) return 0xC800;
	  else return 0;
	};

void HMWDevice::setModuleConfig() {

		// read config from EEPROM
		readConfig();

		pinMode(SENS_CLOCK,OUTPUT);
		digitalWrite(SENS_CLOCK,LOW);

		pinMode(PROG_BUTTON, INPUT_PULLUP);

};

void HMWDevice::deviceLoop() {

	if ( mode.stateChanged() )
	{
		if (mode.isFactoryReset())
		{
			factoryReset();
		}else if( mode.isProgMode() )
		{

		}else if( mode.isNormalMode() )
		{

		}
	}


}

void HMWDevice::readConfig()
{
	  byte* ptr;
	  // EEPROM lesen
	  ptr = (byte*)(&config);
	  for(uint32_t address = 0; address < sizeof(config); address++){
		 *ptr = EEPROM.read(address + 0x01);
		 ptr++;
	  };

}

void HMWDevice::factoryReset()
{
	  // writes FF into config
	  memset((void*)&config, 0xFF, sizeof(config));

	  memset((void*)LINK_START_IN_EEPROM, 0xFF, sizeof(hbw_sec_mdir_link) * MAX_LINK_ENTRIES);
}

unsigned long HMWDevice::getCentralAddress()
{
	unsigned long result = 0;
	  for(int address = 2; address < 6; address++){
		result <<= 8;
		result |= EEPROM.read(address);
	  };
	  return result;
}
bool HMWDevice::hasMoreLinkForChannel(uint16_t channel)
{
	bool retVal = true;

	if ( linkReadOffset[channel] >= LINK_START_IN_EEPROM + (MAX_LINK_ENTRIES * sizeof(hbw_sec_mdir_link) ))
	{
		linkReadOffset[channel] = LINK_START_IN_EEPROM;
		retVal = false;
	}

	return retVal;
}
bool HMWDevice::getLinkForChannel(hbw_sec_mdir_link *link,uint16_t channel)
{
	bool retVal = false;

	if (channel < CHANNEL_IO_COUNT)
	{
		link->loc_channel = 0xff;
		link->peer_channel = 0xff;
		link->address = HMW_TARGET_ADDRESS_BC;

		if (hasMoreLinkForChannel(channel))
		{
			// Read next entries
			uint16_t pos = linkReadOffset[channel];
			uint8_t *dst = (uint8_t *) &link->address;

			hmwdebug("Reading from EEPROM\n");
			hmwdebug(" pos ");
			hmwdebug(pos, HEX);
			link->loc_channel = EEPROM.read(pos);
			hmwdebug(" channel ");
			hmwdebug(link->loc_channel, HEX);
			hmwdebug("\n");
			pos++;

			if (link->loc_channel == 0xff)
			{
				linkReadOffset[channel] = LINK_START_IN_EEPROM;
			}else
			{
				if (link->loc_channel == channel)
				{

					for (int idx = 3 ;idx >= 0 ; idx--)
					{
						*dst = EEPROM.read(pos + idx);
						dst++;

					}
					pos += 4;

					link->peer_channel = EEPROM.read(pos);
					pos++;

					retVal = true;
				}
				linkReadOffset[channel] += sizeof(hbw_sec_mdir_link);

			}
		}
	}
	return retVal;
}

HMWDevice* hmwdevice = NULL;

void factoryReset() {

	hmwdevice->factoryReset();

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

#define isLightOn() (bitRead(channelState,LIGHT_STATUS_CHANNEL) == 1)


void HMWDevice::setLightState(lightstate state)
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
void HMWDevice::setMDState(bool on)
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

bool HMWDevice::isProgMode(void)
{
	return mode.isProgMode();
}

void HMWDevice::checkSensor(void)
{
	static uint8_t md_lon_count = 0;
#ifdef USE_LIGHT_OFF
	static uint8_t loff_count = 0;
#endif
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
#ifdef USE_LIGHT_OFF
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
#endif
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
void HMWDevice::timerLoop(void)
{
	checkSensor();
	//checkButton();
}

void timerLoop(void)
{
	hmwdevice->timerLoop();
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

	hmwdevice = new HMWDevice();
	hmwmodule = new HMWModule(hmwdevice, &hmwrs485, DEVICE_TYPE);

	// config aus EEPROM lesen
	hmwdevice->setModuleConfig();

	hmwdebug("huhu\n");

  	MsTimer2::set(SENSOR_CHECK_INTERVALL_MS, timerLoop); // 500ms period
  	MsTimer2::start();
}


// The loop function is called in an endless loop
void loop()
{

	static byte keyPressNum[] = {0,0};
	// Check
	hmwrs485.loop();

	hmwdevice->deviceLoop();
	if (!hmwdevice->isProgMode())
	{
		if (bitRead(channelState,LIGHT_STATUS_CHANNEL) == 1)
		{
			if (hmwdevice->hasMoreLinkForChannel(LIGHT_STATUS_CHANNEL))
			{
				hbw_sec_mdir_link link;
				if( hmwdevice->getLinkForChannel(&link,LIGHT_STATUS_CHANNEL) )
				{
					hmwdebug("Sending Light On to ");
					hmwdebug(link.loc_channel, HEX);
					hmwdebug(link.address, HEX);
					hmwdebug(link.peer_channel, HEX);
					hmwdebug("\n");
					hmwmodule->sendKeyEvent( (byte) LIGHT_STATUS_CHANNEL,keyPressNum[LIGHT_STATUS_CHANNEL],byte(0),link.address,link.peer_channel);
				}else
				{
					//hmwmodule->sendKeyEvent( (byte) LIGHT_STATUS_CHANNEL,keyPressNum[LIGHT_STATUS_CHANNEL],byte(0),(unsigned long)HMW_TARGET_ADDRESS_BC,byte(0));
					bitClear(channelState,LIGHT_STATUS_CHANNEL);
					// gleich ein Announce hinterher
					// TODO: Vielleicht gehoert das in den allgemeinen Teil
					hmwmodule->broadcastAnnounce(LIGHT_STATUS_CHANNEL);
				}
				keyPressNum[LIGHT_STATUS_CHANNEL]++;
			}else
			{
				//hmwmodule->sendKeyEvent( (byte) LIGHT_STATUS_CHANNEL,keyPressNum[LIGHT_STATUS_CHANNEL],byte(0),(unsigned long)HMW_TARGET_ADDRESS_BC,byte(0));
				bitClear(channelState,LIGHT_STATUS_CHANNEL);
				// gleich ein Announce hinterher
				// TODO: Vielleicht gehoert das in den allgemeinen Teil
				hmwmodule->broadcastAnnounce(LIGHT_STATUS_CHANNEL);
			}
		}

		if ( bitRead(lastState,MOTION_STATUS_CHANNEL) != bitRead(channelState,MOTION_STATUS_CHANNEL))
		{
			hmwdebug("change MotionState\n");
			hmwdebug(bitRead(channelState,MOTION_STATUS_CHANNEL));
			hmwdebug("\n");
			// TODO: InfoMessage oder KeyEvent?
			hmwmodule->sendInfoMessage(MOTION_STATUS_CHANNEL,bitRead(channelState,MOTION_STATUS_CHANNEL),HMW_TARGET_ADDRESS_BC);
			// gleich ein Announce hinterher
			// TODO: Vielleicht gehoert das in den allgemeinen Teil
			hmwmodule->broadcastAnnounce(MOTION_STATUS_CHANNEL);

			keyPressNum[MOTION_STATUS_CHANNEL]++;
		}

		lastState = channelState;
	}else
	{
		hmwdebug("ProgMode On\n");
	}

}





