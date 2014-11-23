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

#include <HBWSecMode.h>

HBWSecMode::HBWSecMode() {
	// TODO Auto-generated constructor stub
	mode = Mode_Normal;
}

HBWSecMode::~HBWSecMode() {
	// TODO Auto-generated destructor stub
}
#define PROG_MODE_OFF 					0
#define PROG_MODE_FACTORY_RESET	    	3
#define PROG_MODE_ON 					2
#define PROG_MODE_WAIT_RELEASE_BUTTON 	1

#define isProgModeActive() prog_mode >= PROG_MODE_ON
uint8_t prog_mode = PROG_MODE_OFF;

bool HBWSecMode::stateChanged()
{
//#ifdef ENABLE_PROG_MODE
	static long lasTime;

	long now = millis();
	bool buttonPressed = !digitalRead(PROG_BUTTON);

	switch (prog_mode)
	{

	case PROG_MODE_WAIT_RELEASE_BUTTON:
		if (!buttonPressed)
		{
			if (now - lasTime > PROG_BUTTON_FACTORY_RESET_DELAY_MS)
			{
				// Factory Reset enabled
				prog_mode = PROG_MODE_FACTORY_RESET;
				hmwdebug("prog mode factory reset\n");

			}
#ifdef ENABLE_PROG_MODE
			else if (now - lasTime > PROG_BUTTON_MODE_ON_DELAY_MS)
			{
				// Prog enabled
				prog_mode = PROG_MODE_ON;
				hmwdebug("prog mode on\n");
			}
#endif
			else if (now - lasTime > 100)
			{
				// Prog Mode Canceled
				prog_mode = PROG_MODE_OFF;
				hmwdebug("prog mode off \n");
			}

		}
		break;
	case PROG_MODE_ON:
	case PROG_MODE_FACTORY_RESET:
		if (now -lasTime >= PROG_BUTTON_MODE_OFF_DELAY_MS)
		{
			prog_mode = PROG_MODE_OFF;
			hmwdebug("prog Mode off\n");
		}else if (buttonPressed)
		{
			prog_mode = PROG_MODE_OFF;
			hmwdebug("prog Mode off\n");
			mode = Mode_Fact;
		}

		break;
	case PROG_MODE_OFF:
	default:
		if (buttonPressed)
		{
			lasTime = now;
			prog_mode = PROG_MODE_WAIT_RELEASE_BUTTON;
			hmwdebug("Prog Mode Wait Release Button\n");
		}

		break;
	}
//#endif
}
