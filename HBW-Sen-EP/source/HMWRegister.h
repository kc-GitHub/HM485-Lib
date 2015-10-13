/*
 * HMWRegister.h
 *
 *  Created on: 26.05.2014
 *      Author: thorsten
 */

#ifndef HMWREGISTER_H_
#define HMWREGISTER_H_

// Konfigurationsdaten als C++-Struktur



#define HMW_CONFIG_NUM_COUNTERS 8
// config of one sensor
struct hmw_counter_config {
  unsigned int send_delta_count;            // Zählerdifferenz, ab der gesendet wird
  unsigned int send_min_interval;   // Minimum-Sendeintervall
  unsigned int send_max_interval;   // Maximum-Sendeintervall
};




struct hmw_config {
	byte logging_time;     // 0x01
	long central_address;  // 0x02 - 0x05
	byte direct_link_deactivate:1;   // 0x06:0
	byte                       :7;   // 0x06:1-7
    hmw_counter_config counters[HMW_CONFIG_NUM_COUNTERS];

};


#endif /* HMWREGISTER_H_ */
