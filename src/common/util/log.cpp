/*
* psplog.cpp -- Christophe Painchaud aka Atanor, DaSH <dash@ionblast.net>
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

#include <csutil/csstring.h>
#include <iutil/objreg.h>
#include <ivaria/reporter.h>
#include "util/consoleout.h"
#include "util/log.h"
#include <iutil/cfgmgr.h>

// Using tuples here
#include <utility>

namespace pslog
{

iObjectRegistry* logger;
bool disp_flag[MAX_FLAGS];
uint32 filters_id[MAX_FLAGS];

const char *flagnames[] = {
                        "LOG_ANY",
                        "LOG_WEATHER",
                        "LOG_SPAWN",
                        "LOG_CELPERSIST",
                        "LOG_PAWS",
                        "LOG_GROUP",
                        "LOG_CHEAT",
                        "LOG_LINMOVE",
                        "LOG_SPELLS",
                        "LOG_NEWCHAR",
                        "LOG_SUPERCLIENT",
                        "LOG_EXCHANGES",
                        "LOG_ADMIN",
                        "LOG_STARTUP",
                        "LOG_CHARACTER",
                        "LOG_CONNECTIONS",
                        "LOG_CHAT",
                        "LOG_NET",
                        "LOG_LOAD",
                        "LOG_NPC",
                        "LOG_TRADE",
                        "LOG_SOUND",
                        "LOG_COMBAT",
                        "LOG_SKILLXP",
                        "LOG_QUESTS",
                        "LOG_SCRIPT",
                        "LOG_RELATIONSHIPS",
                        "LOG_MESSAGES",
                        "LOG_CACHE",
                        "LOG_PETS",
                        "LOG_USER",
                        "LOG_LOOT",
                        "LOG_MINIGAMES",
                        "LOG_DRDATA",
                        "LOG_ACTIONLOCATION",
                        "LOG_ITEM",
                        "LOG_HIRE"
}; // End of flagnames

const char *flagsetting[] = {
                        "PlaneShift.Log.Any",
                        "PlaneShift.Log.Weather",
                        "PlaneShift.Log.Spawn",
                        "PlaneShift.Log.Cel",
                        "PlaneShift.Log.PAWS",
                        "PlaneShift.Log.Group",
                        "PlaneShift.Log.Cheat",
                        "PlaneShift.Log.Linmove",
                        "PlaneShift.Log.Spells",
                        "PlaneShift.Log.Newchar",
                        "PlaneShift.Log.Superclient",
                        "PlaneShift.Log.Exchanges",
                        "PlaneShift.Log.Admin",
                        "PlaneShift.Log.Startup",
                        "PlaneShift.Log.Character",
                        "PlaneShift.Log.Connections",
                        "PlaneShift.Log.Chat",
                        "PlaneShift.Log.Net",
                        "PlaneShift.Log.Load",
                        "PlaneShift.Log.NPC",
                        "PlaneShift.Log.Trade",
                        "PlaneShift.Log.Sound",
                        "PlaneShift.Log.Combat",
                        "PlaneShift.Log.SkillXP",
                        "PlaneShift.Log.Quests",
                        "PlaneShift.Log.Script",
                        "PlaneShift.Log.Relationships",
                        "PlaneShift.Log.Messages",
                        "PlaneShift.Log.Cache",
                        "PlaneShift.Log.Pets",
                        "PlaneShift.Log.User",
                        "PlaneShift.Log.Loot",
                        "PlaneShift.Log.Minigames",
                        "PlaneShift.Log.DRData",
                        "PlaneShift.Log.ActionLocation",
                        "PlaneShift.Log.Item",
                        "PlaneShift.Log.Hire"
}; // End of flagsettings

bool DoLog(int severity, LOG_TYPES type, uint32 filter_id)
{
    if (logger == 0)
        return false;
    if (!disp_flag[type] && severity > CS_REPORTER_SEVERITY_WARNING)
        return false;
    if (filters_id[type]!=0 && filter_id!=0 && filters_id[type]!=filter_id)
        return false;
    return true;
}


void LogMessage (const char* file, int line, const char* function,
             int severity, LOG_TYPES type, uint32 filter_id, const char* msg, ...)
{
    if (!DoLog(severity,type,filter_id)) return;

    va_list arg;

    ConsoleOutMsgClass con = CON_SPAM;
    switch (severity)
    {
        case CS_REPORTER_SEVERITY_WARNING: con = CON_WARNING; break;
        case CS_REPORTER_SEVERITY_NOTIFY: con = CON_NOTIFY; break;
        case CS_REPORTER_SEVERITY_ERROR: con = CON_ERROR; break;
        case CS_REPORTER_SEVERITY_BUG: con = CON_BUG; break;
        case CS_REPORTER_SEVERITY_DEBUG: con = CON_DEBUG; break;
    }

    if(con <= ConsoleOut::GetMaximumOutputClassStdout())
    {
        csString msgid;
        if(con < CON_WARNING && con > CON_CMDOUTPUT)
        {
            msgid.Format("<%s:%d %s SEVERE>\n",file, line, function);
        }
        else
            msgid = ""; // File, Line, Function is too much spam on the console for debug output
            //msgid.Format("<%s:%d %s>\n", file, line, function);

        csString description;
        va_start(arg, msg);
        description.FormatV(msg, arg);
        va_end(arg);
        description.Append("\n"); //add an ending new line

        // Disable use of csReport while testing using CPrintf
        //        va_start(arg, msg);
        //        csReportV (logger, severity, msgid, msg, arg);
        //        va_end(arg);
        CPrintf(con,msgid.GetDataSafe());
    // For safety, print to %s:
        CPrintf(con,"%s",description.GetDataSafe());

        /* ERR and BUG will be loged to errorLog by the CPrintf
        // Log errors to a file so they can be emailed to devs daily.
        if (severity == CS_REPORTER_SEVERITY_ERROR ||
            severity == CS_REPORTER_SEVERITY_BUG)
        {
            if(!errorLog)
                errorLog = fopen("errorlog.txt","w");
            fprintf(errorLog,msgid);
            fprintf(errorLog," ");
            va_start(arg, msg);
            vfprintf(errorLog,msg,arg);
            va_end(arg);
            fprintf(errorLog, "\n");
            fflush(errorLog);
        }
        */
    }
    else
    {
        // Log to file
        va_start(arg, msg);
        CVPrintfLog (con, msg, arg);
        va_end(arg);
    }
}


void Initialize(iObjectRegistry* object_reg)
{
    logger = object_reg;

    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    for (int i = 0; i < MAX_FLAGS; i++)
    {
        disp_flag[i] = false;
    }

    disp_flag[LOG_ANY]         = true;
    disp_flag[LOG_CONNECTIONS] = true;
    disp_flag[LOG_CHAT]        = true;
    disp_flag[LOG_NET]         = true;
    disp_flag[LOG_CHARACTER]   = true;
    disp_flag[LOG_NEWCHAR]     = true;
}

void SetFlag(int index, bool flag, uint32 filter)
{
    csString str;

    disp_flag[index] = flag;
    
    str.AppendFmt("%s flag %s ",flagnames[index],flag?"activated":"deactivated");
    if (filter!=0 && filter!=(uint32)-1)
    {
        filters_id[index]=filter;
        str.AppendFmt("with filter %d.\n",filter);
    }
    else
    {
        filters_id[index]=0;
        str.AppendFmt("with no filter.\n");
    }
    CPrintf(CON_CMDOUTPUT,str.GetDataSafe());
}


void SetFlag(const char *name, bool flag, uint32 filter)
{
    bool unique = true;
    int index = -1;

    csString logStr(name);
    logStr.Upcase();
    name = logStr.GetDataSafe();
    bool all = !strcmp(name, "ALL");
    
    
    for (int i=0; i<MAX_FLAGS; i++)
    {
        if (!flagnames[i])
            continue;

        if (all)
        {
            SetFlag(i, flag, filter);
        }

        if (!all && strstr(flagnames[i], name))
        {
            if (unique && index != -1)
            {
                // We have found another  match already.
                CPrintf(CON_CMDOUTPUT, "'%s' isn't an unique flag name!\n",name);
                unique = false;
                CPrintf(CON_CMDOUTPUT, "%s\n", flagnames[index]);
            }
            index = i;
            if (!unique)
            {
                CPrintf(CON_CMDOUTPUT, "%s\n", flagnames[index]);
            }
        }

    }
    if (all)
    {
        return;
    }
    
    if (index == -1)
    {
        CPrintf(CON_CMDOUTPUT, "No flag found with the name '%s'!\n",name);
        return;
    }

    if (unique)
    {
        SetFlag(index, flag, filter);
    }
}

void DisplayFlags(const char *name)
{
    for (int i=0; i<MAX_FLAGS; i++)
    {
        if (!name || !strcmp(flagnames[i],name))
        {
            csString str;
            str.AppendFmt("%s = %s ",flagnames[i],disp_flag[i]?"true":"false");
            if (filters_id[i]!=0)
            {
                str.AppendFmt("with filter %d \n",filters_id[i]);
            }
            else
            {
                str.AppendFmt("\n");
            }
            CPrintf(CON_CMDOUTPUT,str.GetDataSafe());
        }
    }
}

bool GetValue(const char *name)
{
    for (int i=0; i< MAX_FLAGS; i++)
    {
        if (flagnames[i] && !strcmp(flagnames[i],name))
        {
            return disp_flag[i];            
        }
    }
    return false;
}

const char* GetName(int id)
{
    return flagnames[id];
}

const char* GetSettingName(int id)
{
    return flagsetting[id];
}


} // end of namespace pslog

LogCSV::LogCSV(iConfigManager* configmanager, iVFS* vfs)
{
    std::pair<csString, const char*> logs[MAX_CSV];
    size_t maxSize = configmanager->GetInt("PlaneShift.LogCSV.MaxSize", 10*1024*1024);
    
    logs[CSV_PALADIN] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.Paladin"),
                                                         "Date/Time, Client, Type, Sector, Start pos (xyz), Maximum displacement, Real displacement, Start velocity, Angular velocity, Paladin version\n");
    logs[CSV_EXCHANGES] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.Exchanges"),
                                                                                 "Date/Time, Source Client, Target Client, Type, Item, Quantity, Cost\n");
    logs[CSV_AUTHENT] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.Authent"),
                                       "Date/Time, Client, Client ID, Details\n");
    logs[CSV_STATUS] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.Status"),
                                                        "Date/Time, Status\n");
    logs[CSV_ADVICE] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.Advice"),
                                                        "Date/Time, Source Client, Target Client, Message\n");
    logs[CSV_ECONOMY] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.Economy"),
                                                         "Action, Count, Item, Quality, From, To, Price, Time\n");
    logs[CSV_STUCK] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.Stuck"),
                                                                             "Date/Time, Client, Race, Gender, Sector, PosX, PosY,"
                                                                             " PosZ, Direction\n");
    logs[CSV_SQL] = std::make_pair(configmanager->GetStr("PlaneShift.LogCSV.File.SQL"),
                                     "Date/Time, Query, Time taken");

    for(int i = 0;i < MAX_CSV;i++)
    {
        StartLog(logs[i].first, vfs, logs[i].second, maxSize, csvFile[i]);
    }
}
             
void LogCSV::StartLog(const char* logfile, iVFS* vfs, const char* header, size_t maxSize, csRef<iFile>& csvFile)
{
        bool writeHeader = false;
        if (!vfs->Exists(logfile))
        {
            csvFile = vfs->Open(logfile,VFS_FILE_WRITE);
            writeHeader = true;            
        }
        else
        {            
            csvFile = vfs->Open(logfile,VFS_FILE_APPEND);
            
            // Need to rotate log
            if (csvFile && csvFile->GetSize() > maxSize)
            {
                CPrintf(CON_ERROR, "Log File %s is too big! Current size is: %u. Rotating log.", logfile, csvFile->GetSize());
                
                csvFile = NULL;
                
                // Rolling history
                for (int index = 10; index > 0; index--)
                {
                    csString src(logfile), dst(logfile);
                    src.Append(index);
                    dst.Append(index + 1);
                    // Rotate the files (move file[index] to file[index+1])
                    if (vfs->Exists(src))
                    {
                        csRef<iDataBuffer> existingData = vfs->ReadFile(src, false);
                        vfs->WriteFile(dst, existingData->GetData(), existingData->GetSize());
                    }
                }
                
                csRef<iDataBuffer> existingData = vfs->ReadFile(logfile, false);
                
                csString temp(logfile);
                vfs->WriteFile(temp + "1", existingData->GetData(), existingData->GetSize());
                csvFile = vfs->Open(logfile,VFS_FILE_WRITE);
                
                writeHeader = true;
            }
        }
        if(csvFile.IsValid() && writeHeader)
        {
            csvFile->Write(header, strlen(header));
            csvFile->Flush();
        }
}

void LogCSV::Write(int type, csString& text)
{
    if (!csvFile[type])
        return;


    time_t curtime = time(NULL);
    struct tm *loctime;
    loctime = localtime (&curtime);
    csString buf(asctime(loctime));
    buf.Truncate(buf.Length()-1);

    buf.Append(", ");
    buf.Append(text);
    buf.Append("\n");

    csvFile[type]->Write(buf, buf.Length());

    static unsigned count = 0;

    if (type == CSV_STATUS)
        csvFile[type]->Flush();
    else
    {
        // Unfair flushing mechanism for the time being.
        count++;
        if(count % 16 == 0)
        {
            count = 0;
            csvFile[type]->Flush();
        }
    }
}
