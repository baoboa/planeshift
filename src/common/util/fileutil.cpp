/*
 * fileutil.cpp by Matthias Braun <matze@braunis.de>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

// All OS specific stuff should be in this file
#define CS_SYSDEF_PROVIDE_DIR

#ifdef WIN32
#pragma warning( disable : 4996 )
#endif

#ifdef CS_PLATFORM_WIN32
#include <direct.h>
#endif

#include <sys/stat.h>
#include <csutil/util.h>
#include <iutil/databuff.h> 
#include <csutil/csstring.h>
#include <iutil/stringarray.h>

#include "fileutil.h"

FileUtil::FileUtil(iVFS* _vfs)
{
    vfs = _vfs;
}

FileUtil::~FileUtil()
{
}

csPtr<FileStat> FileUtil::StatFile (const char* path)
{
    struct stat filestats;
    if (stat(path, &filestats) < 0)
        return NULL;

    csRef<FileStat> filestat;
    filestat.AttachNew(new FileStat);

#ifdef CS_PLATFORM_WIN32
    if (filestats.st_mode & _S_IFDIR)
        filestat->type = FileStat::TYPE_DIRECTORY;
    else
        filestat->type = FileStat::TYPE_FILE;

    filestat->link = false;
#else
    if (S_ISDIR(filestats.st_mode))
        filestat->type = FileStat::TYPE_DIRECTORY;
    else
        filestat->type = FileStat::TYPE_FILE;

    filestat->link = S_ISLNK(filestats.st_mode);
#endif

    filestat->size = (uint32_t)filestats.st_size;
    filestat->executable = (filestats.st_mode & S_IEXEC) != 0;
    filestat->readonly = !(filestats.st_mode & S_IWRITE);
    filestat->mode = filestats.st_mode;
    filestat->uid = filestats.st_uid;
    filestat->gid = filestats.st_gid;

    return csPtr<FileStat>(filestat);
}

bool FileUtil::RemoveFile (const char* filename, bool silent)
{
    if(!vfs->DeleteFile(filename))
    {
        // If vfs DeleteFile fails, fall back on old variant.
        int rc = remove(filename);
        if (rc < 0)
        {
            if(!silent)
                printf("Failed to remove file %s\n", filename);
            return false;
        }
    }
    return true;
}

void FileUtil::MakeDirectory (const char* directory)
{
    csString dirAppended(directory);
    dirAppended.Append("/");

    csRef<iDataBuffer> realPath = vfs->GetRealPath(dirAppended);
    while(!vfs->Exists(dirAppended))
    {
        
#ifdef CS_PLATFORM_WIN32
        int rc = mkdir(realPath->GetData());
#else
        int rc = mkdir(realPath->GetData(), S_IRUSR | S_IWUSR);
#endif

        csString dir = directory;
        csRef<iDataBuffer> real = realPath;
        while(rc < 0)
        {
            dir.Truncate(dir.FindLast('/'));

            if(vfs->Exists(directory))
            {
                printf("Couldn't create directory '%s'.", directory);
                return;
            }

            real = vfs->GetRealPath(dir + "/");
#ifdef CS_PLATFORM_WIN32
            rc = mkdir(real->GetData());
#else
            rc = mkdir(real->GetData(), S_IRUSR | S_IWUSR);
#endif
        }

#ifdef CS_PLATFORM_UNIX
        dir.Truncate(dir.FindLast('/'));
        csRef<iDataBuffer> parent = vfs->GetRealPath(dir + "/");
        csRef<FileStat> dirStat = StatFile(parent->GetData());
        SetPermissions(real->GetData(), dirStat);
#endif
    }
}

bool FileUtil::CopyFile(csString from, csString to, bool vfsPath, bool executable, bool silent, bool copyPermissions)
{
    csString n1;
    csString n2;

    csString file = to;
    csRef<iDataBuffer> buff = vfs->GetRealPath(to);
    if(vfsPath)
    {
        // Make parent dir if needed.
        csString parent = to;
        MakeDirectory(parent.Truncate(parent.FindLast('/')));

        if(!buff)
        {
            if(!silent)
                printf("Couldn't get the real filename for %s!\n",to.GetData());
            return false;
        }

        file = buff->GetData();
    }

    // Get current permissions for later.
    csRef<iDataBuffer> fromBuff = vfs->GetRealPath(from);
    csRef<FileStat> fromStat = StatFile(fromBuff->GetData());

    csRef<FileStat> stat = StatFile(file);
    if(stat && stat->readonly)
    {
        if(!silent)
            printf("Won't write to %s, because it's readonly\n",file.GetData());
        return true; // Return true to bypass DLL checks and stuff
    }

    if (!vfsPath)
    {
        n1 = "/this/" + from;
        n2 = "/this/" + to;
    }
    else
    {
        n1= from;
        n2= to;
    }

    csRef<iDataBuffer> buffer = vfs->ReadFile(n1.GetData(),true);

    if (!buffer)
    {
        if(!silent)
            printf("Couldn't read file %s!\n",n1.GetData());
        return false;
    }

    if (!vfs->WriteFile(n2.GetData(), buffer->GetData(), buffer->GetSize() ) )
    {
        if(!silent)
            printf("Couldn't write to %s!\n", n2.GetData());
        return false;
    }

#ifdef CS_PLATFORM_UNIX
    /**
     * On unix type systems we might need to set permissions after copy.
     * If the 'from' stat is null, the file is probably in a zip.
     * So we use the permissions of the parent folder of the 'to' location.
     */
    if(!fromStat.IsValid())
    {
        n2.Truncate(n2.FindLast('/')+1);
        csRef<iDataBuffer> db = vfs->GetRealPath(n2);
        fromStat = StatFile(db->GetData());
    }

    if(copyPermissions)
        SetPermissions(buff->GetData(), fromStat);

    if(executable)
    {
        if(chmod(buff->GetData(), fromStat->mode | S_IXUSR | S_IXGRP) == -1)
            printf("Failed to set permissions on file %s.\n", to.GetData());
    } 
#endif

    //keep the previous modification time. TODO? make it a flag?
    {
        csFileTime time;
        if(vfs->GetFileTime(n1, time))
        {
            vfs->SetFileTime(n2, time);
        }
    }

    return true;
}

bool FileUtil::isExecutable(const char *path)
{
    csRef<FileStat> stats = StatFile(path);
    if(stats.IsValid())
        return stats->executable;
    return true;
}

void FileUtil::SetPermissions(const char *path, FileStat *fs)
{
#ifdef CS_PLATFORM_UNIX
    if(chmod(path, fs->mode) == -1)
        printf("Failed to set permissions on file %s.\n", path);

    if(chown(path, fs->uid, fs->gid) == -1)
        printf("Failed to set permissions on file %s.\n", path);
#endif
}
