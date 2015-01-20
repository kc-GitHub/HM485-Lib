/*
 * HMWRegister.h
 *
 *  Created on: 26.05.2014
 *      Author: thorsten
 */

#ifndef HMWREGISTER_H_
#define HMWREGISTER_H_

// Konfigurationsdaten als C++-Struktur




#define HMW_CONFIG_NUM_SWITCHES 8

struct hmw_config_switch {
	byte logging:1;    // 0x07:0     0=OFF, 1=ON
	byte        :7;    // 0x07:1-7
	byte invert :1;	   // 0x08:0	 0=normal, 1=inverted
	byte        :7;    // 0x08:1-7
};

struct hmw_config {
	byte logging_time;     // 0x01
	long central_address;  // 0x02 - 0x05
	byte direct_link_deactivate:1;   // 0x06:0
	byte                       :7;   // 0x06:1-7
    hmw_config_switch switches[HMW_CONFIG_NUM_SWITCHES];  // 0x0B - 0x0E
};


#endif /* HMWREGISTER_H_ */
