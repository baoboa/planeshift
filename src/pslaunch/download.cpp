/*
* download.cpp
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

#include <cssysdef.h>

#include <csutil/randomgen.h>

#include <curl/curl.h>

#include "download.h"
#include "updaterconfig.h"
#include "updaterengine.h"

const int progressWidth = 30;

struct progressData
{
	csTicks timeStart;
	csTicks timeLast;
	double dlLast;
	double speedLast;
	int lastSize;

	progressData() {
		timeLast = timeStart = csGetTicks();
		dlLast = 0.0;
		lastSize = 0;
		speedLast = -1.0;
	}
};

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

const char* normalize_bytes(double* bytes)
{
	if(*bytes < 1000.0)
		return "B";
	*bytes /= 1000.0;
	if(*bytes < 1000.0)
		return "kB";
	*bytes /= 1000.0;
	return "MB";
}

csString normalize_seconds(double seconds)
{
	unsigned int minutes = seconds / 60;
	unsigned int hours = minutes / 60;
	unsigned int days = hours / 24;
	seconds -= minutes * 60;
	minutes -= hours * 60;
	hours -= days * 24;
	csString time;
	if(days > 0)
		time.AppendFmt("%ud ", days);
	if(hours > 0)
		time.AppendFmt("%uh ", hours);
	if(days > 0)
		return time;
	if(minutes > 0)
		time.AppendFmt("%um ", minutes);
	if(hours > 0)
		return time;
	time.AppendFmt("%us", (unsigned int) seconds);
	return time;
}

int ProgressCallback(void *clientp, double finalSize, double dlnow, double /*ultotal*/, double /*ulnow*/)
{
    progressData* data = (progressData*) clientp;
    double progress = dlnow / finalSize;
    csTicks timeNow = csGetTicks();
    
    csTicks timeDelta = timeNow - data->timeLast;
    // Don't output anything if there's been no progress.
    if(progress == 0 || finalSize <= 102400)
        return 0;

    double dlDelta = dlnow - data->dlLast;
    csString progressLine;

    if(data->lastSize == 0)
        progressLine += '\n';
    else
        progressLine += '\r';
    progressLine += '[';
    for(int pos = 0; pos < progressWidth; pos++)
    {
        if(pos < progressWidth * progress)
            progressLine += '-';
        else
            progressLine += ' ';
    }


    // Recalculate download speed in seconds every 5 seconds.
    if(timeDelta > 5000 || data->speedLast == -1.0)
    {
    	data->speedLast = 1000.0 * dlDelta / timeDelta;
	data->timeLast	= timeNow;
	data->dlLast = dlnow;
    }

    double speed = data->speedLast;

    // Eta in seconds
    double eta = 0;
    csString etaStr;
    if (speed > 0.0)
    {
    	eta = (finalSize - dlnow) / speed;
    	etaStr = normalize_seconds(eta);
    }
    else
    {
        etaStr = "Never";
    }

    const char* speedUnits = normalize_bytes(&speed);

    double dlnormalized = dlnow;
    const char* dlUnits = normalize_bytes(&dlnormalized);
    progressLine.AppendFmt("]  %4.1f%s (%3d%%)  %4.1f%s/s eta %s    ", dlnormalized, dlUnits, (int) (progress * 100.0), speed, speedUnits, etaStr.GetData());
    data->lastSize = dlnow;
    UpdaterEngine::GetSingletonPtr()->PrintOutput(progressLine);

    fflush(stdout);
    
    return UpdaterEngine::GetSingletonPtr()->CheckQuit() ? 1 : 0;
}

Downloader::Downloader(iVFS* _vfs, UpdaterConfig* _config)
{
    this->Init(_vfs);

    config = _config;
    csRandomGen random = csRandomGen();
    startingMirrorID = random.Get((uint32)config->GetCurrentConfig()->GetMirrors().GetSize());
    activeMirrorID = startingMirrorID;    
}

Downloader::Downloader(iVFS* _vfs):
    startingMirrorID(0),activeMirrorID(0),config(NULL)
{
    this->Init(_vfs);
}

Downloader::~Downloader()
{
    delete[] curlerror;
    if(curl)
        curl_easy_cleanup(curl);

    if(fileUtil)
    {
        fileUtil->RemoveFile(UPDATE_CACHE_DIR, true);
        delete fileUtil;
    }
}

void Downloader::Init(iVFS* _vfs)
{
    curl = curl_easy_init();
    if(!curl)
    {
    	UpdaterEngine::GetSingletonPtr()->PrintOutput("CURL failed to initialize!\n");
        curlerror = NULL;
        fileUtil = NULL;
        return;
    }
    curlerror = new char[CURL_ERROR_SIZE];
    curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, curlerror);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    vfs = _vfs;
    // Rename completed download back to real name
    fileUtil = new FileUtil(vfs);
    fileUtil->MakeDirectory(UPDATE_CACHE_DIR);
}

void Downloader::SetProxy(const char* /*host*/, int /*port*/)
{
}

bool Downloader::DownloadFile(const char *file, const char *dest, bool URL, bool silent, uint retries, bool vfsPath)
{
    // Get active url, append file to get full path.
    Mirror* mirror;
    if(URL)
    {
        mirror = new Mirror;
        mirror->SetBaseURL(file);
    }
    else
    {
        mirror = config->GetCurrentConfig()->GetMirrors().Get(activeMirrorID);
    }
    
    while(mirror)
    {
        csString url = mirror->GetBaseURL();
        
        if(!URL)
        {
            UpdaterEngine::GetSingletonPtr()->PrintOutput("Using mirror %s for %s\n", url.GetData(), file);
            url.Append(file);
        }

        csString destpath = dest;

        if (vfs)
        {
            if(!vfsPath)
            {
                destpath = "/this/";
                destpath.Append(dest);
            }
        }
        else
        {
            if(URL)
            {
                delete mirror;
                mirror = NULL;
            }

            printf("No VFS in object registry!?\n");
            return false;
        }

        long curlhttpcode = 200;
        csString error;

        /**
         * Create paths for both the "real" temp filename used during download
         * and the vfs filename used during copy and update of the actual
         * files.
         */
        csString fileName = UPDATE_CACHE_DIR;
        csString realFilePath;

        /**
         * Might seem wierd to use time and random to get a random file but it was
         * an easy solution to an small problem. Please change if you know a better
         * random function for filenames.
         */
        do
        {
            fileName = UPDATE_CACHE_DIR;
            fileName.Append("/");
            fileName.Append(time(NULL));
            fileName.Append(csRandomGen().Get());
            fileName.Append(".download");
        } while (vfs->Exists(fileName));  // Just make sure this file dosn't exist

        csRef<iDataBuffer> cachepath;
        cachepath = vfs->GetRealPath(fileName);
        realFilePath = cachepath->GetData();

        for(uint i=0; i<=retries; i++)
        {
            FILE* file;

            // Download to temp file
            file = fopen(realFilePath.GetData(), "wb");
            if (!file)
            {
                UpdaterEngine::GetSingletonPtr()->PrintOutput("Couldn't write to file! (%s)\n", realFilePath.GetData());

                if(URL)
                {
                    delete mirror;
                    mirror = NULL;
                }
                return false;
            }

            // lets escape the filename in the url
            // first we try to figure out what part of the URL is the filename
            csString filename = url.Slice(url.FindLast('/'));

            if (!filename.IsEmpty())
            {
                // lets remove the leading "/" from the filename
                filename.ReplaceAll("/", "");
                //now let's do the encoding
                char* encURL = curl_easy_escape(curl, filename.GetData(), strlen(filename.GetData()));
                if (encURL)
                {
                    //nice, the encoding worked. So lets replace the filename part in url with the encoded one.
                    url = url.Slice(0, url.FindLast('/')).Append("/").Append(encURL);
                }
                // and not to forget...free the string provided by curl again
                curl_free(encURL);
            }

            progressData data;
            curl_easy_setopt(curl, CURLOPT_URL, url.GetData());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &ProgressCallback);
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &data);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);

            CURLcode result = curl_easy_perform(curl);

            // Check if progress bar was shown.
            if(data.lastSize != 0)
            {
                UpdaterEngine::GetSingletonPtr()->PrintOutput("\n\n");
                data.lastSize = 0;
            }
            fclose (file);

            curl_easy_getinfo (curl, CURLINFO_HTTP_CODE, &curlhttpcode);

            if (result != CURLE_OK)
            {
                if(!silent)
                {
                    if (result == CURLE_COULDNT_CONNECT || result == CURLE_COULDNT_RESOLVE_HOST)
                        error.Format("Couldn't connect to mirror %s\n", url.GetData());
                    else
                        error.Format("Error %s while downloading file: %s\n", curlerror, url.GetData());
                }
            }
            else
            {
                break;
            }
        }
        // Tell the user that we failed
        if(curlhttpcode != 200 || !error.IsEmpty())
        {
            if(!silent)
            {
                if(error.IsEmpty())
                    UpdaterEngine::GetSingletonPtr()->PrintOutput("Server error %i (%s)\n", curlhttpcode, url.GetData());
                else
                    UpdaterEngine::GetSingletonPtr()->PrintOutput("Server error: %s (%i)\n", error.GetData(), curlhttpcode);
            }

            if(!URL)
            {
                // Try the next mirror.
                mirror = config->GetCurrentConfig()->GetMirror(CycleActiveMirror());
                continue;
            }
            break;
        }
        
        // Success!
        if(URL)
        {
            delete mirror;
            mirror = NULL;
        }

        if(vfs->Exists(destpath))
            fileUtil->RemoveFile(destpath);

        if(!fileUtil->CopyFile(fileName, destpath, true, false, true, false))
        {
            UpdaterEngine::GetSingletonPtr()->PrintOutput("Error renaming file %s to %s.\n", fileName.GetData(), destpath.GetData());
            fileUtil->RemoveFile(fileName, true);
            break;
        }
        else
        {
            fileUtil->RemoveFile(fileName, true);
        }

        if(URL)
        {
            delete mirror;
            mirror = NULL;
        }
        return true;
    }

    if(URL)
    {
        delete mirror;
        mirror = NULL;
    }
    else
    {
    	UpdaterEngine::GetSingletonPtr()->PrintOutput("\nThere are no active mirrors! Please check the forums for more info and help!\n");
    }

    return false;
}

uint Downloader::CycleActiveMirror()
{
    activeMirrorID++;
    // If we've reached the end, go back to the beginning of the list.
    if(activeMirrorID >= config->GetCurrentConfig()->GetMirrors().GetSize())
        activeMirrorID = 0;
    // If true, we've reached our start point. Break the loop.
    if(activeMirrorID == startingMirrorID)
        activeMirrorID = (uint32)config->GetCurrentConfig()->GetMirrors().GetSize();

    return activeMirrorID;
}
