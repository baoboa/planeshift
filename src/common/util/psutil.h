/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef PSUTIL_H
#define PSUTIL_H

#include <cstypes.h>
#include <csutil/csstring.h>

/**
 * \addtogroup common_util
 * @{ */

// Colour support. Only supported by pawsMessageTextBox for now.
// 1 is a control character which should be filtered out of user text input.
// The code is a fixed-size of 8 bytes.
// byte 1 is 033 which is the control character
// 2-3 are the R value in hex
// 4-5 are the G value in hex
// 6-7 are the B value in hex
// 8 is the text size in decimal

// Shortcut color/size codes
// Safe for use with tolower, toupper, UTF-8
#define REDCODE "\033ff00000" 
#define GREENCODE "\03300ff000"
#define BLUECODE "\0330000ff0" 
#define WHITECODE "\033ffffff0"
#define DEFAULTCODE "\0330000000" 
#define ESCAPECODE '\033'
#define LENGTHCODE 8

class psColours
{
public:
	static bool ParseColour(const char* str, int& r, int& g, int& b, int& size)
	{
		if(strlen(str) < LENGTHCODE || str[0] != '\033')
			return false;
		str++;
		r = DecodeHex(str, 2);
		if(r == -1)
			return false;
		str += 2;
		g = DecodeHex(str, 2);
		if(g == -1)
			return false;
		str += 2;
		b = DecodeHex(str, 2);
		if(b == -1)
			return false;
		str += 2;
		size = DecodeHex(str, 1);
		if(size == -1)
			return false;
		str++;
		return true;
	}
	
	// Decode a hex string, can accept both uppercase and lowercase.
	// -1 indicates an error occurred.
	static int DecodeHex(const char* str, int len)
	{
		int count = 0;
		for(int i = 0; i < len; i++)
		{
			if(str[i] >= 'a' && str[i] <= 'f')
				count += (str[i] - 'a') + 10;
			else if(str[i] >= '0' && str[i] <= '9')
				count += (str[i] - '0');
			else if(str[i] >= 'A' && str[i] <= 'F')
				count += (str[i] - 'A') + 10;
			else return -1;
			count *= 16;
		}
		return count;
	}
};

/** Get the time of day in GMT.
  * @param string The string to place the time of day.
  */
void GetTimeOfDay(csString& string);

struct psPoint
{
    int x;
    int y;

    psPoint ():x(0), y(0) { }

    /// Constructor: initialize the object with given values
    psPoint (int iX, int iY):x(iX), y(iY) { }
};

class ScopedTimer; // Forward declaration

/** Callback function for ScopedTimers
 *
 *  Use this if special processing beside
 *  giving a warning message should be
 *  done when the scope use to long time.
 *
 */
class ScopedTimerCB
{
public:
    virtual void ScopedTimerCallback(const ScopedTimer* timer) = 0;
    virtual ~ScopedTimerCB() {}
};


/** Check how long time it take to process a scope
 *
 *  Used to time processes in the server. Will print
 *  or call a function if more then limit ticks is
 *  used for a scope.
 *
 */
class ScopedTimer
{
    csTicks         start;    ///< The time when started
    csTicks         timeUsed; ///< Time used if failed
    csTicks         limit;    ///< The limit of when to trigger
    ScopedTimerCB*  callback; ///< Callback to call if set
    csString        comment;  ///< The string to print
         
public:

    /** Start the timer that will print a warning message
     *
     * @param limit  The maximum number of ticks anticipated for this scope
     * @param format String format of the message to dump
     */
    ScopedTimer(csTicks limit, const char * format, ... );

    /** Start the timer that will call the callback
     *
     * @param limit    The maximum number of ticks anticipated for this scope
     * @param callback The callback class
     */
    ScopedTimer(csTicks limit, ScopedTimerCB* callback );
    
    /** Check the limit and trigger warning if more time used than allowed.
     *
     */
    ~ScopedTimer();

    /** To be used in Callbacks to get time used.
     * @return Time in ticks
     */
    csTicks TimeUsed() const;
};

/** Returns a random number.
 *
 * @return Returns a random number between 0.0 and 1.0.
 */
float psGetRandom();

/** Returns a random number with a limit.
 *
 * @return Returns a random number between 0 and limit.
 */
uint32 psGetRandom(uint32 limit);



/** @} */

#endif
