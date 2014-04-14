/*
 * data.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "data.h"
#include "util/log.h"
#include <csutil/xmltiny.h>
#include <iutil/cfgmgr.h>


SoundFile::SoundFile(const char* newName, const char* newFileName)
{
    name      = csString(newName);
    fileName  = csString(newFileName);
    sndData   = 0;
    lastTouch = csGetTicks();
}

SoundFile::SoundFile(SoundFile* const &copySoundFile)
{
    name      = csString(copySoundFile->name);
    fileName  = csString(copySoundFile->fileName);
    sndData   = 0;
    lastTouch = csGetTicks();
}

SoundFile::~SoundFile()
{
}


//--------------------------------------------------


SoundDataCache::SoundDataCache()
{
}

SoundDataCache::~SoundDataCache()
{
    UnloadSoundLib();
}

bool SoundDataCache::Initialize(iObjectRegistry* objectReg)
{
    if(!(sndLoader = csQueryRegistry<iSndSysLoader>(objectReg)))
    {
        Error1("Failed to locate Sound loader!");
        return false;
    }

    if(!(vfs = csQueryRegistry<iVFS>(objectReg)))
    {
        Error1("psSndSourceMngr: Could not initialize. Cannot find iVFS");
        return false;
    }

    // Configuration
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        cacheTime = configManager->GetInt("PlaneShift.Sound.DataCacheTime", DEFAULT_SOUNDFILE_CACHETIME);
    }
    else
    {
        cacheTime = DEFAULT_SOUNDFILE_CACHETIME;
    }

    return true;
}

bool SoundDataCache::GetSoundData(const char* name, csRef<iSndSysData> &sndData)
{
    SoundFile* soundFile;

    // check if it's cached and load it if it's not
    soundFile = loadedSoundFiles.Get(name, 0);
    if(soundFile == 0)
    {
        soundFile = LoadSoundFile(name);
    }

    // the file doesn't exist
    if(soundFile == 0)
    {
        return false;
    }

    // update lastTouch to keep the SoundFile in the cache
    soundFile->lastTouch = csGetTicks();

    sndData = soundFile->sndData;
    return true;
}

void SoundDataCache::UnloadSoundFile(const char* name)
{
    SoundFile* soundFile = loadedSoundFiles.Get(name, 0);
    if(soundFile != 0)
    {
        // this decrement the reference of csRef and delete if it's 0
        soundFile->sndData.Invalidate();
        loadedSoundFiles.Delete(name, soundFile);
    }
}

SoundFile* SoundDataCache::LoadSoundFile(const char* name)
{
    SoundFile*          soundFile;
    csRef<iDataBuffer>  soundBuf;
    bool                error = false;
    bool                isDynamic = false;

    // checking if this has been initialized correctly
    if(!sndLoader.IsValid() || !vfs.IsValid())
    {
        return 0;
    }

    // checking the sound library
    soundFile = libSoundFiles.Get(name, 0);
    if(soundFile == 0) // maybe this is a dynamic file
    {
        soundFile = new SoundFile(name, name);
        isDynamic = true;
    }

    // checking if the file is already loaded
    if(soundFile->sndData.IsValid())
    {
        return soundFile;
    }

    // loading the data into a buffer
    soundBuf = vfs->ReadFile(soundFile->fileName);
    if(soundBuf != 0)
    {
        // extracting sound data from the buffer
        soundFile->sndData = sndLoader->LoadSound(soundBuf);
        if(!soundFile->sndData.IsValid())
        {
            Error2("Can't load sound '%s'!", name);
            error = true;
        }
    }
    else
    {
        Error2("Can't load file '%s'!", name);
        error = true;
    }

    // handling errors
    if(error)
    {
        if(isDynamic) // file is not in the library
        {
            delete soundFile;
        }

        return 0;
    }

    // keeping track of the loaded files
    loadedSoundFiles.Put(name, soundFile);
    if(isDynamic) // new file in the library
    {
        libSoundFiles.Put(name, soundFile);
    }

    return soundFile;
}

void SoundDataCache::Update()
{
    csTicks             now;
    SoundFile*          soundFile;
    csArray<SoundFile*> allSoundFiles;

    // FIXME csticks
    now = csGetTicks();
    allSoundFiles = loadedSoundFiles.GetAll();

    for(size_t i = 0; i < allSoundFiles.GetSize(); i++)
    {
        soundFile = allSoundFiles[i];

        // checking if SoundDataCache is the only one that reference to the data
        if(soundFile->sndData->GetRefCount() == 1)
        {
            // checking if the cache time has elapsed
            if(soundFile->lastTouch + cacheTime <= now)
            {
                UnloadSoundFile(soundFile->name);
            }
        }
        else // data is still in use
        {
            soundFile->lastTouch = csGetTicks();
        }
    }
}

//----------- code below needs its own class --------------------//

/*
 * Reads the xml sound library, gets all the names and paths and store them in the
 * hash "libSoundFiles". Files will be loaded only if needed.
 */
bool SoundDataCache::LoadSoundLib(const char* fileName, iObjectRegistry* objectReg)
{
    csRef<iDocumentSystem>          xml;    /* try get existing Document System or create one*/
    csRef<iDataBuffer>              buff;   /* buffer for reading the xml */
    csRef<iDocument>                doc;    /* document created out of the xml */
    const char*                     error;  /* to store error msgs */
    csRef<iDocumentNode>            root;   /* document root */
    csRef<iDocumentNode>            topNode; /* topnode to work with */
    csRef<iDocumentNodeIterator>    iter; /* node iterator */
    csRef<iDocumentNode>            node;   /* yet another node .... */
    SoundFile*                      snd;        ///< soundfile

    // checking if this has been initialized correctly
    if(!vfs.IsValid())
    {
        return false;
    }

    if(!(xml = csQueryRegistry<iDocumentSystem>(objectReg)))
      xml.AttachNew(new csTinyDocumentSystem);

    buff = vfs->ReadFile(fileName);

    if(!buff || !buff->GetSize())
    {
        return false;
    }

    doc = xml->CreateDocument();

    error = doc->Parse(buff);
    if(error)
    {
        Error3("Parsing file %s gave error %s", fileName, error);
        return false;
    }


    if(!(root = doc->GetRoot()))
    {
        Error1("No XML root in soundlib.xml");
        return false;
    }


    if(!(topNode = root->GetNode("Sounds")))
    {
        Error1("No <sounds> tag in soundlib.xml");
        return false;
    }

    iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        node = iter->Next();

        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        if(strcmp(node->GetValue(), "Sound") == 0)
        {
            snd = new SoundFile(node->GetAttributeValue("name"),
                                node->GetAttributeValue("file"));

            libSoundFiles.Put(snd->name, snd);
        }
    }
    return true;
}

void SoundDataCache::UnloadSoundLib()
{
    // deleting sound files
    csHash<SoundFile*, csString>::GlobalIterator soundFileIter(libSoundFiles.GetIterator());
    SoundFile* soundFile;

    while(soundFileIter.HasNext())
    {
        soundFile = soundFileIter.Next();
        delete soundFile;
    }

    // unloading sound library
    libSoundFiles.DeleteAll();
    loadedSoundFiles.DeleteAll();
}

