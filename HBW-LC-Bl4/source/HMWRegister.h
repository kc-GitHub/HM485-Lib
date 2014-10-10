/*
 * HMWRegister.h
 *
 *  Created on: 26.05.2014
 *      Author: thorsten
 */

#ifndef HMWREGISTER_H_
#define HMWREGISTER_H_

// Konfigurationsdaten als C++-Struktur




// Blind (Jalousie)
#define HMW_CONFIG_NUM_BLINDS 4

struct hmw_config_blind {
	byte logging:1;    			   // 0x07:0    0x0E	0x15	0x1C
	byte        :7;   			   // 0x07:1-7	0x0E
	byte blindTimeChangeOver;      // 0x08		0x0F	0x16	0x1D
	byte blindReferenceRunCounter; // 0x09		0x10	0x17	0x1E
	byte blindTimeBottomTop_low;   // 0x0A;		0x11	0x18	0x1F
	byte blindTimeBottomTop_high;  // 0x0B;
	byte blindTimeTopBottom_low;   // 0x0C;		0x13	0x1A	0x21
	byte blindTimeTopBottom_high;  // 0x0D;
};


struct hmw_config {
	byte logging_time;     // 0x01
	long central_address;  // 0x02 - 0x05
	byte direct_link_deactivate:1;   // 0x06:0
	byte                       :7;   // 0x06:1-7
    hmw_config_blind blinds[HMW_CONFIG_NUM_BLINDS];
};


#endif /* HMWREGISTER_H_ */
