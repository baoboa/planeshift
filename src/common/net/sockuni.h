/*
 * sockuni.h
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
 * Socket definitions for UNIX
 *
 * Author: Matthias Braun <MatzeBraun@gmx.de>
 */

#ifndef __SOCKUNI_H__
#define __SOCKUNI_H__

/* Include various files needed for sockets */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

/**
 * \addtogroup common_net
 * @{ */

//#define INCLUDE_IPV6_SUPPORT true

/* define some types */
#ifndef SOCKET
#define SOCKET    int
#endif

#ifndef IN_ADDR
#ifdef INCLUDE_IPV6_SUPPORT
#define IN_ADDR struct in6_addr
#else
#define IN_ADDR struct in_addr
#endif
#endif

#ifndef SOCKADDR_IN
#ifdef INCLUDE_IPV6_SUPPORT
#define SOCKADDR_IN struct sockaddr_in6
#else
#define SOCKADDR_IN struct sockaddr_in
#endif
#endif

#ifndef LPSOCKADDR
#define LPSOCKADDR  struct sockaddr *
#endif

#ifndef LPSOCKADDR_IN
#ifdef INCLUDE_IPV6_SUPPORT
#define LPSOCKADDR_IN    struct sockaddr_in6 *
#else
#define LPSOCKADDR_IN    struct sockaddr_in *
#endif
#endif

#define SOCK_SENDTO(a,b,c,d,e,f)                        sendto(a,(const void *) b,c,d,e,f)
#define SOCK_RECVFROM(a,b,c,d,e,f)                      recvfrom(a,(void *) b,c,d,e,f)
#define SOCK_IOCTL(a,b,c)                               ioctl(a,b,c)
#define SOCK_CLOSE(a)                                   close(a)
#define SOCK_SELECT(max,read,write,except,timeout)      select(max,read,write,except,timeout)

#define INVALID_SOCKET    -1

static inline int initSocket()
{
    /* we don't need to init sockets in unix... */
    return 0;
}

static inline void exitSocket()
{
    return;
}

/** @} */

#endif
