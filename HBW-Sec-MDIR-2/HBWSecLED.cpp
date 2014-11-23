/*
 * HBWSecLED.cpp
 *
 *  Copyright: 2014, Rene Fischer
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as 
 * published by the Free Software Foundation.
 *
 * There are special exceptions to the terms and conditions of the GPL 
 * as it is applied to this software. View the full text of the 
 * exception in file EXCEPTIONS-CONNECTOR-J in the directory of this 
 * software distribution.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "Arduino.h"
#include <HBWSecLED.h>

HBWSecLED::HBWSecLED() {
	// TODO Auto-generated constructor stub
	on = false;

	pinMode(LED, OUTPUT);

	setLEDPicture(LED_off);
}

HBWSecLED::~HBWSecLED() {
	// TODO Auto-generated destructor stub
}

void HBWSecLED::triggerLED()
{
	switch (LEDpicture)
	{
		case LED_fast:
			break;
		case LED_slow:
			break;
		default:
			break;
	}
}
void HBWSecLED::setLEDPicture(LEDPicture pic)
{
	LEDpicture = pic;

	switch (pic)
	{
	case LED_on:
		digitalWrite(LED,HIGH);
		break;
	case LED_off:
		digitalWrite(LED,LOW);
		break;

	}
}
