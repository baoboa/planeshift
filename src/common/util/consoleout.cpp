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
#include <csutil/sysfunc.h>
#include <csutil/threading/thread.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>

#ifdef CS_COMPILER_MSVC
#include <conio.h>
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

#include "consoleout.h"
#include "command.h"
#include "util/pserror.h"

FILE* errorLog = NULL;
static FILE* outputfile = NULL;
static ConsoleOutMsgClass maxoutput_stdout = CON_SPAM;
static ConsoleOutMsgClass maxoutput_file = CON_SPAM;

int ConsoleOut::shift = 0;
csString *ConsoleOut::strBuffer = NULL;
bool ConsoleOut::atStartOfLine = true;
bool ConsoleOut::promptDisplayed = false;

void ConsoleOut::SetMaximumOutputClassStdout (ConsoleOutMsgClass con)
{
    // always output command output on stdout
    if(con < CON_CMDOUTPUT)
    {
        maxoutput_stdout = CON_CMDOUTPUT;
    }
    else
    {
        maxoutput_stdout = con;
    }
}

void ConsoleOut::SetMaximumOutputClassFile (ConsoleOutMsgClass con)
{
    maxoutput_file = con;
}

ConsoleOutMsgClass ConsoleOut::GetMaximumOutputClassStdout()
{
    return maxoutput_stdout;
}

ConsoleOutMsgClass ConsoleOut::GetMaximumOutputClassFile()
{
    return maxoutput_file;
}

void ConsoleOut::SetOutputFile (const char* filename, bool append)
{
    if (outputfile)
    {
        fclose (outputfile);
        outputfile = NULL;
    }
    if (filename)
    {
        outputfile = fopen (filename, append ? "a" : "w");
    }
}

void ConsoleOut::Intern_Printf (ConsoleOutMsgClass con, const char* string, ...)
{
    va_list args;
    va_start(args, string);
    Intern_VPrintf(con,string,args);
    va_end(args);
}

void ConsoleOut::Intern_VPrintf (ConsoleOutMsgClass con, const char* string, va_list args)
{
    csString output;
    csString time_buffer; //holds the time string to be appended to each line
    bool ending_newline = false;
    output.FormatV(string, args); //formats the output
	
	//get time stamp
    time_t curtime = time(NULL);
    struct tm *loctime;
    loctime = localtime (&curtime);
    time_buffer.Format("%s", asctime(loctime)); //formats the time string to be appended
    time_buffer.Truncate(time_buffer.Length()-1);
    time_buffer.Append(", ");
    // Append any shift
    for (int i=0; i < shift; i++)
    {
        time_buffer.Append("  ");
    }
    output.Insert(0, time_buffer); //add it to the starting of the string

    // Format output with timestamp at each line and apply shifts
    
    if(output.GetAt(output.Length()-1) == '\n')  //check if there is an ending new line to avoid substitution there
    {
        output.Truncate(output.Length()-1);
        ending_newline = true;
    }

    time_buffer.Insert(0, "\n"); //add the leading new line in the time string
    output.FindReplace("\n", time_buffer); //adds the string to be appended to the output string
    
    if(ending_newline) //restore the ending newline if it was removed
        output.Append("\n");

    // Now that we have output correctly formated check where to send the output

    // Check for stdout
    if (con <= maxoutput_stdout)
    {
        if (strBuffer)
        {
            strBuffer->Append(output);
        }
        else
        {
            if (promptDisplayed)
            {
                printf("\n");
                promptDisplayed = false;
            }
            printf("%s", output.GetDataSafe());
            fflush(stdout);
        }
    }
    

    // Check for output file
    if (outputfile && con <= maxoutput_file)
    {
        fprintf(outputfile, "%s", output.GetDataSafe());
    }
    // Check for error log
    if (con == CON_ERROR ||
        con == CON_BUG)
    {
        if(!errorLog)
        {
            errorLog = fopen("errorlog.txt","w");
        }

        if(errorLog)
        {
            fprintf(errorLog, "%s", output.GetDataSafe());
            fflush(errorLog);
        }
    }


#ifdef USE_READLINE
    rl_redisplay();
#endif
}

void ConsoleOut::Intern_Printf_LogOnly(ConsoleOutMsgClass con,
                                       const char *string, ...)
{
    va_list args;
    va_start(args, string);
    Intern_VPrintf_LogOnly(con,string,args);
    va_end(args);
}

void ConsoleOut::Intern_VPrintf_LogOnly(ConsoleOutMsgClass con,
                                       const char *string, va_list args)
{
    if (outputfile && con <= maxoutput_file)
    {
        for (int i=0; i < shift; i++)
        {
            vfprintf (outputfile, "  ", args);
        }
        vfprintf (outputfile, string, args);
        fflush (outputfile);
    }
}

void ConsoleOut::Shift()
{
    shift ++;
}

void ConsoleOut::Unshift()
{
    shift --;
}

void ConsoleOut::SetPrompt(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    csString buffer;
    buffer.FormatV(format, args);
    va_end(args);

    if (!atStartOfLine)
    {
        printf("\n");
        atStartOfLine = true;
    }

    printf("%8u) %s",csGetTicks(),buffer.GetDataSafe());
    promptDisplayed = true;
}

