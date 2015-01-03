/*
 * HBWSecLED.h
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

#ifndef HBWSECLED_H_
#define HBWSECLED_H_

#define LED 13

enum LEDPicture
{
	LED_off,
	LED_on,
	LED_slow,
	LED_fast
};

class HBWSecLED {
public:
	HBWSecLED();
	virtual ~HBWSecLED();

	void triggerLED();
	void setLEDPicture(LEDPicture);

private:
	void setLEDOutput(bool LEDon);

	LEDPicture LEDpicture;
	unsigned long changedTime;
	bool on;
};

#endif /* HBWSECLED_H_ */
