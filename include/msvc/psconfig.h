/* config.h files are created by autoconf/automake in the gnu world...
 * So You have to set up your own for visualc, which is done here
 * This version is currently set up for windows (VC6/7/8)
 */
 
#ifdef __CONFIG_H__
#   error PLEASE include this file only once in each .cpp file! (not in .h)
#endif
#define __CONFIG_H__

/* because we also have to include cssysdef.h everywhere include it here */
#include <cssysdef.h>
#include <stdio.h>

/* print out debugging messages */
#define DEBUG

/* We want to use winsock */
#define USE_WINSOCK

/* We use win threading */
#define USE_WINTASK

/* We need to define uni32_t and others ourself */
#define DEFINE_STDINT_TYPES

/* the type which defines 64 bit integer */
//#define TYPE64	  __int64

/* the type some socketfunctions like recvfrom require for sockaddr size */
//#define	socklen_t   int

/* define this for error output, because visualc doesn't have __PRETTY_FUNCTION
 * like gcc does.
 */
//#define __PRETTY_FUNCTION__ __FUNCTION__  //doesn't seems to work in VC
#define __PRETTY_FUNCTION__ ""

/* we don't have readline */
//#define HAVE_READLINE_H
//#define HAVE_HISTORY_H

/* use ANSI ESCAPE SEQUENCES for console output (color!) */
//#define USE_ANSI_ESCAPE

#ifdef WIN32
// pragma turns off portability warning about [0] below.
#pragma warning( disable : 4200 )

// pragma turns off warning about only having a default: switch label
#pragma warning( disable : 4355 )

// pragma turns off warning about not able to generate default constructor
#pragma warning( disable : 4511 )

#include <conio.h> // needed for getch();

#define PS_PAUSEEXIT(x) \
  { printf("Exiting PlaneShift.  Press Enter..."); getch(); exit(x); }

// function has different name in Windows
#define snprintf _snprintf

#define vsnprintf _vsnprintf


// MSVC6 does not define this function, but MSVC7 does
#ifndef __FUNCTION__
#define __FUNCTION__ "function name not available"
#endif

#endif
