#include <psconfig.h>

#include <stdarg.h>
#include "util/pserror.h"

#ifdef CS_COMPILER_MSVC
#include <conio.h>
#endif

void psprintf(const char *arg, ...)
{
    va_list args;
    va_start (args, arg);
#ifdef USE_READLINE
    rl_save_prompt();
#endif
    vprintf (arg, args);
    fflush(stdout);
#ifdef USE_READLINE
    rl_restore_prompt();
#endif
    va_end(args);
}

/// Error handling functions...
void errorhalt(const char* function, const char* file, int line,
    const char* msg)
{
//    ServerConsole::setColor(COL_RED);
    psprintf ("***Error: %s\n",msg);
    psprintf ("In file %s function %s line %d\n",file,function,line);
    psprintf ("program stopped!\n");
//    ServerConsole::setColor(COL_NORMAL);
    // are we on msvc?
#ifdef CS_COMPILER_MSVC
    getch();
#endif
    // yes, we're crude here, but with this we can trace back with the
    // debugger
    CS_ASSERT(false);
}

void errormsg(const char* function, const char* file, int line,
    const char* msg)
{
//    ServerConsole::setColor(COL_RED);
    psprintf ("***Warning: %s\n",msg);
    psprintf ("In file %s function %s line %d\n",file,function,line);
//    ServerConsole::setColor(COL_NORMAL);
}

