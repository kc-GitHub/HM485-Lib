/*
 * HMWDebug.h
 *
 *  Created on: 20.07.2014
 *      Author: thorsten
 */

#ifndef HMWDEBUG_H_
#define HMWDEBUG_H_

#include "Arduino.h"

extern Stream* hmwdebugstream;

template <typename T>
void hmwdebug(T msg) { if(hmwdebugstream) hmwdebugstream->print(msg); };

template <typename T>
void hmwdebug(T msg, int base) { if(hmwdebugstream) hmwdebugstream->print(msg, base); };


#endif /* HMWDEBUG_H_ */
