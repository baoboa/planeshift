/*
 * command.h
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
 *  This file will only be included by serverconsole.cpp and defines some
 *  structures for commands, the console accepts...
 *
 * Author: Matthias Braun <MatzeBraun@gmx.de>
 */

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

/**
 * \addtogroup common_util
 * @{ */

typedef int cmdfuncptr(const char *);

/**
 *  This is a little class to store an array of commands and functions to
 *  call with each command.  The allowRemote flag determines whether
 *  each command is allowed from a remote console or just from the local
 *  console.
 */
struct COMMAND {
    const char *name;
    bool  allowRemote;
    cmdfuncptr *func;
    const char *doc;
};

extern const COMMAND commands[];

/** @} */

#endif
