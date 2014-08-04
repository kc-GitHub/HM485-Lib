HM485-Lib
=========

Framework to develop HM-Wired compatible devices on base of Arduino Hardware

The actual library consists of the following files:
	HMWModule.cpp
	HMWModule.h
	HMWRS485.cpp
	HMWRS485.h
	HMWDebug.cpp
	HMWDebug.h
	
If you want to use SoftwareSerial for the RS485 connection, you need to replace the original SoftwareSerial.cpp by the one given here.
	
The directory contains actual implementations of devices (only prototypes so far).

HMW-IO-12-FM does not work yet
HMW-LC-Sw2-DR is like the original HMW-LC-Sw2-DR, but quite some stuff missing
HBW-1W-T10 reads 1-Wire temperature sensors and sends the result to a CCU	

