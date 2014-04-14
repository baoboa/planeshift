/*
 * globals.h
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

/* This file holds definitions for ALL global variables in the planeshift
 * server, normally you should move global variables into the psServer class
 */
#ifndef __GLOBALS_H__
#define __GLOBALS_H__

/**
 * \addtogroup npcclient
 * @{ */

// Forward declarations
class psNPCClient;
struct iDataConnection;

extern psNPCClient* npcclient; ///< Global connection to the NPC Client.

extern iDataConnection* db; ///< Global connection to the Database. Set from the psDatabase class.

/** @} */

#endif

