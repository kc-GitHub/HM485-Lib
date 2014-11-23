/*
 * HBWSecMode.h
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

#ifndef HBWSecMode_H_
#define HBWSecMode_H_

#ifndef PROG_BUTTON_FACTORY_RESET_DELAY_MS
#define PROG_BUTTON_FACTORY_RESET_DELAY_MS	6000
#endif

#ifndef PROG_BUTTON_MODE_ON_DELAY_MS
#define PROG_BUTTON_MODE_ON_DELAY_MS 		3000
#endif

#ifndef PROG_BUTTON_MODE_OFF_DELAY_MS
#define PROG_BUTTON_MODE_OFF_DELAY_MS 		20000
#endif

#define PROG_BUTTON 11

class HBWSecMode {
public:
	HBWSecMode();
	virtual ~HBWSecMode();

	bool stateChanged();
	bool isProgMode() {return mode = Mode_Prog;};
	bool isFactoryReset() {return mode = Mode_Fact;};
	bool isNormalMode() {return mode = Mode_Normal;};
private:
	enum mode
	{
		Mode_Normal,
		Mode_Prog,
		Mode_Fact
	}mode;

};

#endif /* HBWSecMode_H_ */
