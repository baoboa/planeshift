/*
* fileutil.h by Matthias Braun <matze@braunis.de>
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

/* This file contains
* Utility functions to access the filesystem in a platform
* independant way. It would be nicer to use vfs here, but vfs is too limited
* for our needs (testing for file/directory, file permissions) */

#ifndef __FILEUTIL_H__
#define __FILEUTIL_H__

#include <psstdint.h>
#include <iutil/vfs.h>
#include <csutil/refcount.h>

struct iVFS;

/**
 * \addtogroup common_util
 * @{ */

class FileStat : public csRefCount
{
public:
    enum Type
    {
        TYPE_FILE,
        TYPE_DIRECTORY
    };

    Type type;
    bool link;
    bool executable;
    uint32_t size;
    char* target;
    bool readonly;
    unsigned short mode;
    short uid;
    short gid;

    FileStat ()
    { target = NULL; }
    ~FileStat ()
    { delete[] target; }
};


class FileUtil
{
private:
    csRef<iVFS> vfs;
public:
    FileUtil(iVFS* vfs);
    ~FileUtil();
    /* Tests if the file exists and returns data about the file. */
    csPtr<FileStat> StatFile(const char* path);

    bool RemoveFile(const char* filename, bool silent = false);

    /* Creates a directory, given a vfs path (/this/). */
    void MakeDirectory(const char* directory);

    /* Copies a file. */
    bool CopyFile(csString from, csString to, bool vfsPath, bool executable, bool silent = false, bool copyPermissions = true);

    /* Moves a file */
    inline void MoveFile(csString from, csString to, bool vfsPath, bool executable, bool silent = false)
    {
        CopyFile(from, to, vfsPath, executable, silent);
        RemoveFile(from, silent);
    }

    /* Returns true is the file at the specified path is executable. */
    bool isExecutable(const char* path);

    /* Sets all permissions on a file. */
    void SetPermissions(const char* path, FileStat* fs);
};

/** @} */

#endif // __FILEUTIL_H__
