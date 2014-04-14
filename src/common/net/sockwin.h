
#ifndef __SOCKWIN_H__
#define __SOCKWIN_H__

//#include <windows.h>
#ifdef __CYGWIN__
// this avoid a warning in cygwin, we don't have to care about because we don't
// use the FD_SET type macors
#define USE_SYS_TYPES_FD_SET
#endif

// Do not link in windows 
#define _INC_WINDOWS
#include <winsock.h>
#undef _INC_WINDOWS


/**
 * \addtogroup common_net
 * @{ */

// INET_ADDRSTRLEN is defined in the ws2ipdef.h file for windows part of winsock 2.
// Until that is included we need to include it her.
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 22
#endif

#define SOCK_SENDTO(a,b,c,d,e,f)    sendto(a,(const char *)b,c,d,e,f)
#define SOCK_RECVFROM(a,b,c,d,e,f)    recvfrom(a,(char *)b,c,d,e,f)
#define SOCK_IOCTL(a,b,c)        ioctlsocket(a,b,(unsigned long *)c)
#define SOCK_CLOSE(a)            closesocket(a)
#define SOCK_SELECT(max,read,write,except,timeout)      select(max,read,write,except,timeout)

// hack, because on cygwin socklen_t is defined but not suitable for winsock
#ifdef socklen_t
#undef socklen_t
#endif
#define socklen_t            int

static inline int initSocket()
{
    WSADATA wsaData;
    WORD wVers = MAKEWORD(1, 1);
    return WSAStartup(wVers, &wsaData);
}

static inline void exitSocket()
{
    WSACleanup();
}

/** @} */

#endif

