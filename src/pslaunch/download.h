/*
* download.h
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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


#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

#include <iutil/vfs.h>
#include <csutil/csstring.h>
#include <util/fileutil.h>

class UpdaterEngine;
class UpdaterConfig;

typedef void CURL;

class Downloader
{
public:
    Downloader(iVFS* _vfs, UpdaterConfig* _config);
    Downloader(iVFS* _vfs);
    ~Downloader();

    void Init(iVFS* _vfs);

    /*
     * If URL is false; download a file from 'file' and save to 'dest'.
     * If URL is true; download a file from 'file' where 'file' is the
     * full URL to the file, and save to 'dest'
     */
    bool DownloadFile (const char* file, const char* dest, bool URL, bool silent = false, uint retries = 1, bool vfsPath = false);

    /* Set the proxy server host and port */
    void SetProxy (const char* host, int port);
private:
    /* The ID of the mirror we randomly selected. */
    uint32 startingMirrorID;
    
    /* The current active mirror. */
    uint32 activeMirrorID;

    /* Cycle our currently active mirror to the next */
    uint CycleActiveMirror();

    /* Curl object! */
    CURL* curl;

    /* curl error string */
    char* curlerror;

    /* VFS */
    csRef<iVFS> vfs;

    /* FileUtil pointer */
    FileUtil* fileUtil;

    /* Config pointer */
    UpdaterConfig* config;
};

#endif // __DOWNLOAD_H__
