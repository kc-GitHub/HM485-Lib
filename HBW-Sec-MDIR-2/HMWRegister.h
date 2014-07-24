/*
 * HMWRegister.h
 *
 *  Created on: 26.05.2014
 *      Author: thorsten
 */

#ifndef HMWREGISTER_H_
#define HMWREGISTER_H_

// Konfigurationsdaten als C++-Struktur

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

struct hmw_config {
	byte logging_time;     // 0x01
	long central_address;  // 0x02 - 0x05
	byte direct_link_deactivate:1;   // 0x06:0
	byte                       :7;   // 0x06:1-7
    hmw_config_key keys[HMW_CONFIG_NUM_KEYS];  // 0x07 - 0x0A
    hmw_config_switch switches[HMW_CONFIG_NUM_SWITCHES];  // 0x0B - 0x0E
};


#endif /* HMWREGISTER_H_ */
