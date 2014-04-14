/*
 * serverconsole.h - author: Matze Braun <matze@braunis.de>
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
#ifndef __SERVERCONSOLE_H__
#define __SERVERCONSOLE_H__

#include <csutil/threading/thread.h>
#include "util/consoleout.h"
#include "util/gameevent.h"

#ifdef    USE_ANSI_ESCAPE
#define COL_NORMAL  "\033[m\017"
#define COL_RED        "\033[31m" 
#define COL_BLUE    "\033[34m"
#define COL_YELLOW  "\033[33m"
#define    COL_CYAN    "\033[36m"
#define COL_GREEN   "\033[32m"
#else
#define COL_NORMAL  ""
#define COL_RED        ""
#define COL_BLUE    ""
#define COL_CYAN    ""
#define COL_GREEN   ""
#define COL_YELLOW  ""
#endif

struct COMMAND;

const COMMAND *find_command(const char *name);
int execute_line(const char *line,csString *buffer);

struct iObjectRegistry;

/**
 * \addtogroup common_util
 * @{ */

/**
 * This defines an interface for intercepting commands instead of handling them
 * locally in the server console.
 */
class iCommandCatcher
{
public:
    virtual void CatchCommand(const char *cmd) = 0;
    virtual ~iCommandCatcher() {}
};

/**
 * This class is implements the user input and output console for the server.
 */
class ServerConsole : public ConsoleOut, public CS::Threading::Runnable
{
public:
    ServerConsole(iObjectRegistry *oreg, const char *appname, const char *prompt);
    ~ServerConsole();

    /**
     * Executes a script of commands.
     *
     * The script is passed in as a char array buffer (NOT a filename). This function
     * goes through the char array script line at a time and executes
     * the given server command. It also allows for commenting the
     * script using the \#comment...
     */
    static void ExecuteScript(const char* script);

    /**
     * Starts the server console, first looking if there's a script passed
     * via -run=file on the command line, then entering the main loop.
     */
    void Run();

    /**
     * The main server console loop.
     *
     * This waits for a user to enter a line of data and executes the command
     * that the user entered.
     */
    void MainLoop();

    void SetCommandCatcher(iCommandCatcher *cmdcatch)
    {
        cmdcatcher = cmdcatch;
    }

    /// Holds the prompt.
    static const char *prompt;

protected:
    /// The name of this application.  Only used for printing & readline.
    const char *appname;

    /// The server console runs in its own thread.
    csRef<CS::Threading::Thread> thread;

    /// If true, the server is shutting down, and the main loop should stop.
    bool stop;

    /// CommandCatcher intercepts typed commands without processing them here
    iCommandCatcher *cmdcatcher;

    iObjectRegistry *objreg;
};

// Allows the console commands to be thread-safe by inserting them into the main event queue
class psServerConsoleCommand : public psGameEvent
{
    csString command;

public:
    // 0 offset for highest priority in the game event queue
    psServerConsoleCommand(const char* command) : psGameEvent(0, 0, "psServerStatusRunEvent"), command(command) {};
    void Trigger()
    {
        execute_line(command,NULL);
        CPrintf (CON_CMDOUTPUT, COL_BLUE "%s: " COL_NORMAL, ServerConsole::prompt);
    };
    virtual csString ToString() const
    {
        return command;
    }
};

/** @} */

#endif

