/*
 * psdatabase.cpp - Author: Keith Fulton
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
#include <psconfig.h>

#include <iutil/objreg.h>
#include <ivaria/reporter.h>
#include <csutil/randomgen.h>


#include "psdatabase.h"
//#include "net/messages.h"
#include "log.h"
#include "strutil.h"
#include "psconst.h"

// This should be in header file somewhere...
#define PSAPP   "planeshift.application.server"
#define PS_QUERY_PLUGIN(goal,intf, str)                             \
    goal =  csQueryRegistry<intf> (object_reg);                     \
    if (!goal)                                                      \
    {                                                               \
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR, PSAPP,    \
            "No " str " plugin!");                                  \
        return false;                                               \
    }

iDataConnection *db;

psDatabase::psDatabase(iObjectRegistry *objectreg)
{
    object_reg = objectreg;
}

psDatabase::~psDatabase()
{
    // the close call deletes the plugin
    Close();
}

bool psDatabase::Initialize(const char* host, unsigned int port, const char* user,
                            const char* password, const char* database)
{
    if (db)
    {
        SetLastError("Database already initialized");
        return false;
    }
    
    PS_QUERY_PLUGIN(mysql, iDataConnection, "iDataConnection");
    db = mysql;

    bool ret = db->Initialize(host,port,database,user,password, LogCSV::GetSingletonPtr());

    if (!ret || !db->IsValid())
    {
        lasterror = db->GetLastError();
        db = NULL;
        return false;
    }
        
    return true;
}

const char *psDatabase::GetLastSQLError()
{
    if (!db)
        return "";
    
    return db->GetLastError();
}

const char* psDatabase::GetLastError()
{
    if (lasterror.IsEmpty())
        return GetLastSQLError();

    return (const char*) lasterror;
}

const char *psDatabase::GetLastQuery()
{
    if (!db)
        return "";
    
    return db->GetLastQuery();
}

void psDatabase::Close()
{
    if (db)
    {
        db->Close();
    }
    
    db = NULL;
}



