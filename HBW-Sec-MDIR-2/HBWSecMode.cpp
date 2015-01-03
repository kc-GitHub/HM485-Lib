/*
 * HBWSecMode.cpp
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

#include "HBWSecMode.h"

#include "HMWDebug.h"

HBWSecMode::HBWSecMode() {
	// TODO Auto-generated constructor stub
	prog_mode = PROG_MODE_OFF;
	progButtonPressed = false;
	led.setLEDPicture(LED_on);
}

HBWSecMode::~HBWSecMode() {
	// TODO Auto-generated destructor stub
}

void HBWSecMode::checkProgButton()
{
	static unsigned long lastChangeTime=0;
	if (progButtonPressed != !digitalRead(PROG_BUTTON) )
	{
		if (lastChangeTime == 0)
		{
			lastChangeTime = millis();
		}

		if (millis() - lastChangeTime >= 100)
		{
			lastChangeTime = 0;
			progButtonPressed = !digitalRead(PROG_BUTTON);
		}

	}

}

bool HBWSecMode::stateChanged()
{
	static unsigned long lasTime;

	unsigned long now = millis();
	uint8_t old_mode = prog_mode;

	checkProgButton();

	switch (prog_mode)
	{
	case PROG_MODE_FACTORY_RESET:
		// Called in next loop after ACK
		prog_mode = PROG_MODE_OFF;
		hmwdebug("prog mode off \n");
		break;
	case PROG_MODE_FACTORY_RESET_WAIT_ACK:
		if (!progButtonPressed)
		{
			if (now - lasTime > PROG_BUTTON_MODE_ON_DELAY_MS)
			{
				// Factory Reset enabled
				prog_mode = PROG_MODE_FACTORY_RESET;
				hmwdebug("Reset to Factory \n");

			}else
			{
				led.setLEDPicture(LED_on);
				prog_mode = PROG_MODE_OFF;
				hmwdebug("Reset to Factory canceled\n");
			}
		}

		break;
	case PROG_MODE_FACTORY_RESET_WAIT_PRESS_AGAIN:
		if (progButtonPressed)
		{
			prog_mode = PROG_MODE_FACTORY_RESET_WAIT_ACK;
			hmwdebug("Prog mode factory reset wait ack \n");
			lasTime = now;
		}else if (now -lasTime >= PROG_BUTTON_MODE_OFF_DELAY_MS)
		{
			led.setLEDPicture(LED_on);
			prog_mode = PROG_MODE_OFF;
			hmwdebug("Exit Progmode after timeout\n");
		}
		break;
	case PROG_MODE_FACTORY_RESET_WAIT_RELEASE:
		if (!progButtonPressed)
		{
			prog_mode = PROG_MODE_FACTORY_RESET_WAIT_PRESS_AGAIN;
			hmwdebug("Prog mode factory reset wait press again \n");
		}else if (now -lasTime >= PROG_BUTTON_MODE_OFF_DELAY_MS)
		{
			led.setLEDPicture(LED_on);
			prog_mode = PROG_MODE_OFF;
			hmwdebug("Exit Progmode after timeout\n");
		}
		break;
	case PROG_MODE_WAIT_RELEASE_BUTTON:
		if (progButtonPressed)
		{
			if (now - lasTime > PROG_BUTTON_MODE_ON_DELAY_MS)
			{
				// Prog enabled
				prog_mode = PROG_MODE_ON_WAIT_RELEAESE_BUTTON;
				hmwdebug("prog mode on wait release button\n");
			}

		}else
		{
			prog_mode = PROG_MODE_OFF;
			hmwdebug("Prog mode off\n");
		}
		break;

	case PROG_MODE_ON_WAIT_RELEAESE_BUTTON:
		if (progButtonPressed)
		{
			if (now - lasTime > PROG_BUTTON_FACTORY_RESET_DELAY_MS)
			{
				// Prog enabled
				prog_mode = PROG_MODE_FACTORY_RESET_WAIT_RELEASE;
				hmwdebug("prog mode factory reset wait release\n");
				led.setLEDPicture(LED_fast);
			}

		}else
		{
			prog_mode = PROG_MODE_ON;
			hmwdebug("Prog mode on\n");
		}
		break;
	case PROG_MODE_ON:
		if (now -lasTime >= PROG_BUTTON_MODE_OFF_DELAY_MS)
		{
			led.setLEDPicture(LED_on);
			prog_mode = PROG_MODE_OFF;
			hmwdebug("Exit Progmode after timeout\n");
		}
		break;

	case PROG_MODE_OFF:
	default:
		if (progButtonPressed)
		{
			led.setLEDPicture(LED_slow);
			lasTime = now;
			prog_mode = PROG_MODE_WAIT_RELEASE_BUTTON;
			hmwdebug("Prog Mode Wait Release Button\n");
		}

		break;
	}

	led.triggerLED();

	if (prog_mode != old_mode)
		return true;
	else
		return false;
}
