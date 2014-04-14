/*
 * consoleout.h - author: Matze Braun <matze@braunis.de>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __CONSOLEOUT_H__
#define __CONSOLEOUT_H__

#include <stdarg.h>

#include <csutil/csstring.h>

/**
 * \addtogroup common_util
 * @{ */

extern FILE * errorLog;

/**
 * Different message classes.
 */
enum ConsoleOutMsgClass
{
  CON_NONE = 0,        // No output.
  CON_CMDOUTPUT,       // Used for server command output. Is always displayed.
  CON_BUG,             // A bug. Very important to show.
  CON_ERROR,           // Error
  CON_WARNING,         // Warning
  CON_NOTIFY,          // Notification
  CON_DEBUG,           // Debugging message
  CON_SPAM             // High volume spam used for detailed logging
};

/**
 * Simple static class for controlled user output.
 */
class ConsoleOut
{
public:
    /**
     * Used to print things to the console.
     */
    static void Intern_Printf(ConsoleOutMsgClass con, const char *arg, ...);

    /**
     * Used to print things to the console.
     */
    static void Intern_VPrintf(ConsoleOutMsgClass con, const char *arg, va_list ap);

    /**
     * Used to print things to the console.
     * This version only does the log to the output file.
     */
    static void Intern_Printf_LogOnly(ConsoleOutMsgClass con,
        const char *arg, ...);

    /**
     * Used to print things to the console.
     * This version only does the log to the output file.
     */
    static void Intern_VPrintf_LogOnly(ConsoleOutMsgClass con,
        const char *arg, va_list args);

    /**
     * Setup the console to additionally write to some file
     * instead of only stdout. If append is true then append
     * to the file.
     */
    static void SetOutputFile (const char* filename, bool append);

    /**
     * Set the maximum message class that we want to show on standard
     * output. By default this is CON_SPAM. Set to CON_NONE to disable
     * all output.
     */
    static void SetMaximumOutputClassStdout (ConsoleOutMsgClass con);

    /**
     * Set the maximum message class that we want to show on the output
     * file. By default this is CON_SPAM. Set to CON_NONE to disable
     * all output.
     */
    static void SetMaximumOutputClassFile (ConsoleOutMsgClass con);

    /**
     * Set or clear the string buffer.  If a string buffer is specified
     * then the console only prints to the string instead of to the
     * console, so the output can be captured and re-used.
     */
    static void SetStringBuffer(csString *buffer)
    {
        strBuffer = buffer;
        if (buffer)
            buffer->Clear();
    }

    /**
     * Set the prompt to be used for stdout.
     */
    static void SetPrompt(const char*format, ...);

    static ConsoleOutMsgClass GetMaximumOutputClassStdout();
    static ConsoleOutMsgClass GetMaximumOutputClassFile();
    
    static void Shift();
    static void Unshift();
    
    static int shift;
    static csString *strBuffer;
    static bool atStartOfLine;
    static bool promptDisplayed;
};

/**
 * Allows other classes to print to the server console easily.
 */
#define CPrintf         ConsoleOut::Intern_Printf
#define CVPrintf        ConsoleOut::Intern_VPrintf
#define CPrintfLog      ConsoleOut::Intern_Printf_LogOnly
#define CVPrintfLog     ConsoleOut::Intern_VPrintf_LogOnly
#define CShift          ConsoleOut::Shift
#define CUnshift        ConsoleOut::Unshift
#define CPrompt         ConsoleOut::SetPrompt

/** @} */

#endif

