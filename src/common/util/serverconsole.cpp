/*
* serverconsole.cpp
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
* Author: Matthias Braun <MatzeBraun@gmx.de>
*/

#include <psconfig.h>

#include <csutil/snprintf.h>
#include <iutil/vfs.h>
#include <iutil/objreg.h>
#include <iutil/cmdline.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>
#include <ctype.h>

#include "util/eventmanager.h"

#ifdef CS_COMPILER_MSVC
#include <conio.h>
#endif

/* Include platform specific socket settings */
#ifdef USE_WINSOCK
#   include "net/sockwin.h"
#endif
#ifdef USE_UNISOCK
#   include "net/sockuni.h"
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#ifdef HAVE_READLINE_READLINE_H
#   include <readline/readline.h>
#   ifdef HAVE_READLINE_HISTORY_H
#    include <readline/history.h>
#    define USE_HISTORY
#   endif
#   define USE_READLINE
#endif

#ifdef HAVE_READLINE_H
/* g++ with glibc hack... */
#   ifdef __P
#    undef __P
#   endif
#   include <readline.h>
#   ifdef HAVE_HISTORY_H
#    include <history.h>
#    define USE_HISTORY
#   endif
#   define USE_READLINE
#endif

#ifndef whitespace
#   define whitespace(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\r') )
#endif

#include "serverconsole.h"
#include "command.h"
#include "util/pserror.h"

const char *ServerConsole::prompt = NULL;

#ifdef USE_READLINE
char **server_command_completion (const char *,int,int);
#endif

ServerConsole::ServerConsole(iObjectRegistry* oreg, const char *name, const char *prmpt)
{
    objreg     = oreg;
    appname    = name;
    prompt     = prmpt;
    cmdcatcher = NULL;
    stop       = false;
    thread.AttachNew(new CS::Threading::Thread(this));
    thread->Start();

#ifdef USE_READLINE
    rl_readline_name = name;
    rl_attempted_completion_function = server_command_completion;
    rl_initialize();
#endif
}

ServerConsole::~ServerConsole()
{
    stop = true;
    thread->Wait();
    thread = NULL;
}

// command execution time
const COMMAND *find_command(const char *name)
{
    for (int i=0; commands[i].name; i++)
        if (strcmp (name, commands[i].name) == 0)
            return (&commands[i]);

    return ((COMMAND *) NULL);
}

int execute_line(const char *cmd,csString *buffer)
{
    ConsoleOut::SetStringBuffer(buffer);
    char line[1000];
    strcpy (line, cmd);

    int i;
    const COMMAND *command;
    char *word;

    i = 0;
    while (line[i] && whitespace (line[i]))
        i++;
    word = line + i;

    while (line[i] && !whitespace (line[i]))
        i++;

    if (line[i])
        line[i++]='\0';

    command = find_command(word);

    if (!command)
    {
        CPrintf (CON_CMDOUTPUT, "No such command: %s\n", word);
        return -1;
    }

    while (whitespace (line[i]))
        i++;

    word = line + i;

    return ((*(command->func)) (word));
}

char *stripwhite(char *string)
{
    char *s,*t;

    for (s = string; whitespace (*s); s++)
        ;

    if (*s == 0)
        return s;

    t = s + strlen(s) - 1;
    while (t > s && whitespace (*t))
        t--;
    *++t = '\0';

    return s;
}

void ServerConsole::ExecuteScript(const char* script)
{
    const char* bufptr = script;

    while ( *bufptr != 0)
    {
        // skip empty lines
        while (*bufptr == '\n')
            bufptr++;

        char line[1000];
        size_t len = strcspn(bufptr, "\n");
        memcpy (line, bufptr, len);
        line[len] = '\0';
        bufptr += len;

        char* strippedline = stripwhite(line);
        if ((strippedline[0] == '#') || (strippedline[0] == '\0'))
            continue;

        char actualcommand[1000];
        strcpy(actualcommand, strippedline);
        size_t commentbegin = strcspn(actualcommand, "#");
        actualcommand[commentbegin] = '\0';

        if (actualcommand[0] == '\0')
            continue;

        CPrintf (CON_CMDOUTPUT, COL_BLUE "%s: " COL_NORMAL, prompt);
        CPrintf (CON_CMDOUTPUT, "%s\n", actualcommand);

        execute_line(actualcommand,NULL);
    }
}

void ServerConsole::Run()
{
    CPrintf(CON_CMDOUTPUT, COL_RED "-Server Console initialized (%s-V0.1)-\n" COL_NORMAL, appname);

    // Run any script files specified on the command line with -run=filename.
    csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser>(objreg);
    csRef<iVFS> vfs = csQueryRegistry<iVFS>(objreg);
    if (cmdline && vfs)
    {
        const char *autofile;
        for (int i = 0; (autofile = cmdline->GetOption("run", i)); i++)
        {
            csRef<iDataBuffer> script = vfs->ReadFile(autofile);
            if (script.IsValid())
                ExecuteScript(*(*script));
        }
    }

    MainLoop();
}

#ifdef WIN32
#include "windows.h"
#endif

#ifndef USE_READLINE

// simple version without readline...
void ServerConsole::MainLoop()
{
    char line[321];
    line[0] = '\0';

    CPrompt (COL_BLUE "%s: " COL_NORMAL, prompt);

    // This important flag allows clean shutdown outside the console
    // since stdin is polled asynchronously
    while (!stop)
    {

        // Make sure that if we don't type anything the loop is continued
        // so that the running flag will be checked.

#ifdef WIN32
        // Windows only allows select to be used on sockets so we have to use this
        // roundabout method. Note: the running flag won't be checked when the
        // user is in the middle of typing something.
        HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
        DWORD records_read = 0;
        INPUT_RECORD irec[1];

        // Wait until the console receives input
        if(WaitForSingleObject(hInput, 1000) == WAIT_TIMEOUT)
            continue;

        // Check if the record is for a keypress
        PeekConsoleInput(hInput, irec, 1, (LPDWORD)(&records_read));

        if(irec[0].EventType != KEY_EVENT)
        {
            // Read and discard this event
            ReadConsoleInput(hInput, irec, 1, (LPDWORD)(&records_read));
            continue;
        }
#else
        struct timeval tv;
        fd_set readfds;

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        // This allows the running flag to be checked every second but only works on Unix.
        int retval = select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);
        if(retval == 0)
        {
            continue;
        }
#endif

        if (fgets(line,319,stdin) == NULL)
        {
            // TODO: Yield timeslice here maybe?
			continue;
        }

        ConsoleOut::atStartOfLine = true;

        char* s;
        s = stripwhite(line);

        // Strip trailing newline
        if (s[strlen(s)-1]=='\n')
            s[strlen(s)-1]='\0';

        if (*s)
        {
#ifdef USE_HISTORY
            add_history(s);
#endif
            if (*s != '/')  // commands starting with slash are remote commands from npcclient to server
            {
                if(!strcmp("quit", s))
                    execute_line(s, NULL);
                else
                    EventManager::GetSingleton().Push(new psServerConsoleCommand(s));
            }
            else if (cmdcatcher)
            {
                // Give something else access to the command
                cmdcatcher->CatchCommand(s);
                CPrompt (COL_BLUE "%s: " COL_NORMAL, prompt);
            }
        }
        else
        {
            CPrompt (COL_BLUE "%s: " COL_NORMAL, prompt);
        }
    }
}
#else
void ServerConsole::MainLoop()
{
    while (!stop)
    {
        char *s;
        char cmdprompt[80];
        char *line;

        cs_snprintf (cmdprompt,80, COL_BLUE "%s: " COL_NORMAL, prompt);
        line = NULL;

        while (line == NULL)
        {
            line = readline(cmdprompt);
            /* TODO: Yield timeslice here maybe? */
        }

        s=stripwhite(line);
        if (*s)
        {
#ifdef USE_HISTORY
            add_history(s);
#endif
            execute_line(s,NULL);
        }

        free(line);
    }
}

/* more readline stuff */

char *dupstr (char *s)
{
    char *r;

    r = (char *) malloc (strlen (s) + 1);
    strcpy (r, s);
    return (r);
}

/* Generator function for command completion.  STATE lets us know whether
to start from scratch; without any state (i.e. STATE == 0), then we
start at the top of the list. */
char *command_generator (const char *text, int state)
{
    static int list_index, len;
    char *name;

    /* If this is a new word to complete, initialize now.  This includes
    saving the length of TEXT for efficiency, and initializing the index
    variable to 0. */
    if (!state)
    {
        list_index = 0;
        len = strlen (text);
    }

    /* Return the next name which partially matches from the command list. */
    while ((name = (char*)commands[list_index].name) != 0)
    {
        list_index++;

        if (strncmp (name, text, len) == 0)
            return (dupstr(name));
    }

    /* If no names matched, then return NULL. */
    return ((char *)NULL);
}

/* Attempt to complete on the contents of TEXT.  START and END bound the
region of rl_line_buffer that contains the word to complete.  TEXT is
the word to complete.  We can use the entire contents of rl_line_buffer
in case we want to do some simple parsing.  Return the array of matches,
or NULL if there aren't any. */
char **server_command_completion (const char *text,int start,int )
{
    char **matches;

    matches = (char **)NULL;

    /* If this word is at the start of the line, then it is a command
    to complete.  Otherwise it is the name of a file in the current
    directory. */
    if (start == 0)
        matches = rl_completion_matches (text, command_generator);

    return (matches);
}

#endif

