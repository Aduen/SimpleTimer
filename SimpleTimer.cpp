/*
 * SimpleTimer.h
 *
 * SimpleTimer - A timer library for Arduino.
 * Author: mromani@ottotecnica.com
 * Copyright (c) 2010 OTTOTECNICA Italy
 *
 * This library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser
 * General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser
 * General Public License along with this library; if not,
 * write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * updated by Aduen for use with member function pointers
 */

#include "SimpleTimer.h"

SimpleTimer::SimpleTimer() {
    long current_millis = micros();

    for (int i = 0; i < MAX_TIMERS; i++) {
        calls[i].enabled = false;
        calls[i].callback = 0;
        calls[i].prev_millis = current_millis;
        calls[i].params = -1;
    }

    num_timers = 0;
}


void SimpleTimer::run() {
    uint8_t i;
    long current_millis;

    // get current time
    current_millis = micros();

    for (i = 0; i < MAX_TIMERS; i++) {

        // only process active timers
        if (!calls[i].callback || !calls[i].enabled) continue;

		// is it time to process this timer ?
		if (current_millis - calls[i].prev_millis >= calls[i].delay) {

			// update time
			calls[i].prev_millis = current_millis;

			// "run forever" timers must always be executed
			if (calls[i].maxRuns == RUN_FOREVER) {
				(calls[i].callback)(calls[i].params);
                continue;
			}
			// other timers get executed the specified number of times

			//BUG take better care of your memory
			//if an earlier short timer gets deleted before an long later one, it gets overriden because
			//of the num_timers just stacks upwards and doesnt look down

			if (calls[i].numRuns < calls[i].maxRuns) {
				(calls[i].callback)(calls[i].params);
				calls[i].numRuns++;

				// after the last run, delete the timer
				// to save some cycles
				if (calls[i].numRuns >= calls[i].maxRuns) {
					deleteTimer(i);
				}
			}
        }
    }
}

uint8_t SimpleTimer::setUTimer(long d, timer_delegate f, uint8_t n) {
    if (num_timers >= MAX_TIMERS) {
        return -1;
    }
    
    uint8_t free_spot = getAvailableSpot();
    if(free_spot < 0)return -1;
    
    d = d<50 ? 50 : d-2-(free_spot*2); //takes Ntimers *2uS before call execution
    
    calls[free_spot].delay = d;
    calls[free_spot].callback = f;
    calls[free_spot].maxRuns = n;
    calls[free_spot].enabled = true;
    calls[free_spot].numRuns = 0;
    calls[free_spot].prev_millis = micros();
    calls[free_spot].params = free_spot;
    
    num_timers = getNumTimers();
    
    return free_spot;
}

uint8_t SimpleTimer::setTimer(long d, timer_delegate f, uint8_t n) {
	if (num_timers >= MAX_TIMERS) {
        return -1;
    }

    uint8_t free_spot = getAvailableSpot();
    if(free_spot < 0)return -1;

    calls[free_spot].delay = d*1000;
    calls[free_spot].callback = f;
    calls[free_spot].maxRuns = n;
    calls[free_spot].enabled = true;
    calls[free_spot].numRuns = 0;
    calls[free_spot].prev_millis = micros();
    calls[free_spot].params = free_spot;

    num_timers = getNumTimers();

    return free_spot;
}

uint8_t SimpleTimer::setTimer(long d, timer_delegate f, uint8_t n, uint8_t param) {
    if (num_timers >= MAX_TIMERS) {
        return -1;
    }

    uint8_t free_spot = getAvailableSpot();
    if(free_spot < 0)return -1;

    calls[free_spot].delay = d*1000;
    calls[free_spot].callback = f;
    calls[free_spot].maxRuns = n;
    calls[free_spot].enabled = true;
    calls[free_spot].numRuns = 0;
    calls[free_spot].prev_millis = micros();
    calls[free_spot].params = param;

    num_timers = getNumTimers();

    return free_spot;
}

uint8_t SimpleTimer::setInterval(long d, timer_delegate f) {
    return setTimer(d, f, RUN_FOREVER);
}


uint8_t SimpleTimer::setTimeout(long d, timer_delegate f) {
    return setTimer(d, f, RUN_ONCE);
}

void SimpleTimer::change_delay(uint8_t numTimer, unsigned long d){
	calls[numTimer].delay = d*1000;
	calls[numTimer].prev_millis = micros();
}

void SimpleTimer::deleteTimer(uint8_t numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }

    // nothing to disable if no timers are in use
    if (num_timers == 0) {
        return;
    }

    calls[numTimer].callback = 0;
    calls[numTimer].enabled = false;
    calls[numTimer].delay = 0;

    num_timers = getNumTimers();
}


boolean SimpleTimer::isEnabled(uint8_t numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return false;
    }

    return calls[numTimer].enabled;
}


void SimpleTimer::enable(uint8_t numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }

    calls[numTimer].enabled = true;
}


void SimpleTimer::disable(uint8_t numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }

    calls[numTimer].enabled = false;
}

uint8_t SimpleTimer::getResidualRuns(uint8_t numTimer)
{
    if (numTimer >= MAX_TIMERS) {
        return 0xFF;
    }
    
    return calls[numTimer].numRuns;
}


void SimpleTimer::toggle(uint8_t numTimer) {
    if (numTimer >= MAX_TIMERS) {
        return;
    }

    calls[numTimer].enabled = !calls[numTimer].enabled;
}

uint8_t SimpleTimer::getAvailableSpot() {
	for (int i = 0; i < MAX_TIMERS; i++) {
		if(calls[i].callback == 0)return i;
	}
	return -1;
}

uint8_t SimpleTimer::getNumTimers() {
	uint8_t numTimers = 0;
    for (int i = 0; i < MAX_TIMERS; i++) {
		if(calls[i].callback && calls[i].enabled)numTimers++;
	}
    return numTimers;
}
