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
#include "Platform_Dependencies.h"
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
			if (millis() - changedTime >= 500)
			{
				if (on)
				{
					setLEDOutput(false);
				}else
				{
					setLEDOutput(true);
				}
			}
			break;
		case LED_slow:
			if (millis() - changedTime >= 1000)
			{
				if (on)
				{
					setLEDOutput(false);
				}else
				{
					setLEDOutput(true);
				}
			}
			break;
		default:
			break;
	}
}
void HBWSecLED::setLEDOutput(bool LEDon)
{
	if (LEDon)
	{
		digitalWrite(LED,HIGH);

	}else
	{
		digitalWrite(LED,LOW);

	}

	on = LEDon;
	changedTime = millis();

}
void HBWSecLED::setLEDPicture(LEDPicture pic)
{
	LEDpicture = pic;

	switch (pic)
	{
	case LED_on:
		setLEDOutput(true);
		break;
	case LED_fast:
	case LED_slow:
	case LED_off:
		setLEDOutput(false);
		break;

	}
}
