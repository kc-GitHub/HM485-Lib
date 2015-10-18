// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _HMWHomebrew_H_
#define _HMWHomebrew_H_
#include "Arduino.h"
//add your includes for the project HMWHomebrew here
#include "HBWSecMode.h"

//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

// default module methods
#include "HMWModule.h"

#define CHANNEL_IO_COUNT 2
#define MAX_LINK_ENTRIES 28
#define LINK_START_IN_EEPROM 0x357

struct hbw_sec_mdir_link
	{
		byte loc_channel;
		unsigned long address;
		byte peer_channel;
	};
enum lightstate
{
	off,
	on
};
//add your function definitions for the project HMWHomebrew here
class HMWDevice : public HMWDeviceBase {
  public:
	HMWDevice();
	virtual ~HMWDevice() {};
	void setLevel(byte channel,unsigned int level);
	unsigned int getLevel(byte channel, byte command);
	void readConfig();
	void setModuleConfig();
	void deviceLoop();
	void factoryReset();
	unsigned long getCentralAddress();
	bool getLinkForChannel(hbw_sec_mdir_link *link,uint16_t channel);
	bool hasMoreLinkForChannel(uint16_t channel);
	void timerLoop(void);
	bool isProgMode(void);
  private:
	void setLightState(lightstate state);
	void setMDState(bool on);
	void checkSensor(void);
	struct hbw_sec_mdir_ch_config
		{
			byte input_locked:1;			//0=LOCKED, 1=UNLOCKED
			byte :7;
		};
		struct hbw_sec_mdir_config {

			byte 						logging_time;     			// 0x01
			long 						central_address;  			// 0x02 - 0x05
			byte 						direct_link_deactivate:1;   // 0x06:0
			byte                       						  :7;   // 0x06:1-7
			hbw_sec_mdir_ch_config		ch[CHANNEL_IO_COUNT];
		}config;

		uint16_t linkReadOffset[CHANNEL_IO_COUNT] = {LINK_START_IN_EEPROM,LINK_START_IN_EEPROM};


	HBWSecMode mode;
};



//Do not add code below this line
#endif /* _HMWHomebrew_H_ */
