/*
 * Platform_Dependencies.h
 *
 *  Copyright: 2015, Rene Fischer
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

#ifndef HM485_LIB_PLATFORM_DEPENDENCIES_H_
#define HM485_LIB_PLATFORM_DEPENDENCIES_H_

#define DEBUG_NONE 0   // Kein Debug-Ausgang, RS485 auf Hardware-Serial
#define DEBUG_UNO 1    // Hardware-Serial ist Debug-Ausgang, RS485 per Soft auf pins 5/6
#define DEBUG_UNIV 2   // Hardware-Serial ist RS485, Debug per Soft auf pins 5/6

#ifndef PLATFORM_GENERIC
#include "Arduino.h"
#include <EEPROM.h>
#ifdef DEBUG_ENABLE
#define DEBUG_VERSION DEBUG_UNO
#else
#define DEBUG_VERSION DEBUG_NONE
#endif
#else

#ifndef DEBUG_ENABLE
#define DEBUG_VERSION DEBUG_UNIV
#else
#define DEBUG_VERSION DEBUG_NONE

#endif

#endif

#endif /* HM485_LIB_PLATFORM_DEPENDENCIES_H_ */
