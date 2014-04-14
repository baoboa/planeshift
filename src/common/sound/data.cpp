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


SoundFile::SoundFile (const char *newname, const char *newfilename)
{
    name      = csString(newname);
    filename  = csString(newfilename);
    loaded    = false;
    snddata   = NULL;
    lasttouch = csGetTicks();
}

SoundFile::SoundFile (SoundFile* const &copythat)
{
    name      = csString(copythat->name);
    filename  = csString(copythat->filename);
    loaded    = false;
    snddata   = NULL;
    lasttouch = csGetTicks();
}

SoundFile::~SoundFile ()
{
}

/*
 * SndData is a set of functions to help us load and unload sounds
 * simple interface
 */

SoundData::SoundData ()
{
}

SoundData::~SoundData ()
{
    csArray<SoundFile*> allsoundfiles;

    allsoundfiles = soundfiles.GetAll();

    for (size_t i = 0; i < allsoundfiles.GetSize(); i++)
    {
        delete allsoundfiles[i];
    }
    
    soundfiles.DeleteAll();
    
    UnloadSoundLib();
}

/*
 * Initializes a loader and vfs
 * if one of both fails it will return false
 */

bool SoundData::Initialize(iObjectRegistry* objectReg)
{
    if (!(sndloader = csQueryRegistry<iSndSysLoader> (objectReg)))
    {
        Error1("Failed to locate Sound loader!");
        return false;
    }


    if (!(vfs = csQueryRegistry<iVFS> (objectReg)))
    {
        Error1("psSndSourceMngr: Could not initialize. Cannot find iVFS");
        return false;
    }

    return true;
}

/*
 * Loads a soundfile into a given buffer
 * it checks if we have the resource and loads it if its not already loaded
 */

bool SoundData::LoadSoundFile (const char *name, csRef<iSndSysData> &snddata)
{
    SoundFile                   *snd;
    csRef<iDataBuffer>          soundbuf;

    if ((snd = GetSound(name)) != NULL)
    {
        /* Sound exists in "known or loaded state
         * get the sound with GetSound
         * check if its already loaded ..
         * return true if it is
         * if not load it and mark it loaded
         */
        if (snd->loaded == true)
        {
            snddata = snd->snddata;
            return true;
        }
    }
    else
    {
        // maybe this is a dynamic file create a handle and push it into our array
        snd = new SoundFile (name, name);
        PutSound(snd);
    }

    /* load the sounddata into a buffer */
    if (!(soundbuf = vfs->ReadFile (snd->filename)))
    {
        Error2("Can't load file '%s'!", name); /* FIXME */
        return false;
    }

    /* extract sound from data */
    if (!(snddata = sndloader->LoadSound (soundbuf)))
    {
        Error2("Can't load sound '%s'!", name);
        return false;
    }

    snd->loaded = true;
    snd->snddata = snddata;
    return true;
}

/*
 * This method unloads the given soundname
 * it DOES it. It doesnt care if its still used or not!
 */

void SoundData::UnloadSoundFile (const char *name)
{
    // do *not* use GetSound here as it potentially creates
    // a new sound, why would we want to create a new one
    // upon deletion?
    SoundFile* sound = soundfiles.Get(csHashCompute(name), NULL);
    if(sound)
    {
        DeleteSound(sound);
    }

    return;
}

/*
 * Returns the requested sound if it exists in our library or cache
 * NULL if it doesnt
 */

SoundFile *SoundData::GetSound (const char *name)
{
    SoundFile   *sound;

    sound = soundfiles.Get(csHashCompute(name), NULL);

    // sound is null when theres no cached SoundFile
    if (sound == NULL)
    {
        // we go search the library .. maybe it has it
        if (libsoundfiles.Contains(csHashCompute(name)))
        {
            // SoundFile is in our library, copy it
            sound = new SoundFile(libsoundfiles.Get(csHashCompute(name), NULL));
            PutSound(sound);
        }
        else
        {
            // no such SoundFile ;( return NULL
            return NULL;
        }
    }

    // update lasttouch to keep that SoundFile in memory
    sound->lasttouch = csGetTicks();
    return sound;
}

void SoundData::PutSound (SoundFile* &sound)
{
    // i know theres PutUnique but i have a bad feeling about overwriting
    soundfiles.Put(csHashCompute((const char*) sound->name), sound);
}

void SoundData::DeleteSound (SoundFile* &sound)
{
    // do *not* delete all, but only the *specific* one as we don't check
    // for an existing key upon putting by maybe deleting a duplicate
    // deleting all here may result in a memory leak
    soundfiles.Delete(csHashCompute((const char*)sound->name), sound);
    delete sound;
}

void SoundData::Update ()
{
    csTicks             now;
    csArray<SoundFile*> allsoundfiles;
    SoundFile          *sound;

    // FIXME csticks
    now = csGetTicks();
    allsoundfiles = soundfiles.GetAll();

    for (size_t i = 0; i < allsoundfiles.GetSize(); i++)
    {
        sound = allsoundfiles[i];

        if ((sound->lasttouch + SOUNDFILE_CACHETIME) <= now
            && sound->loaded == true
            && sound->snddata->GetRefCount() == 1)
        {
            // UnloadSoundFile takes "names" as arguments and works on our hash
            UnloadSoundFile(sound->name);
        }
        else
        {
            sound->lasttouch = csGetTicks();
        }
    }
}

//----------- code below needs its own class --------------------//

/*
 * loads soundlib.xml get all the names and filenames
 * store them in the hash "soundfiles"
 *
 * i think we could load all the sounds right now but i fear that this
 * could eat a lot of memory
 * reload should be possible but im too lazy right now
 *
 */

bool SoundData::LoadSoundLib (const char* filename, iObjectRegistry* objectReg)
{
    csRef<iDocumentSystem>          xml;    /* try get existing Document System or create one*/
    csRef<iDataBuffer>              buff;   /* buffer for reading the xml */
    csRef<iDocument>                doc;    /* document created out of the xml */
    const char*                     error;  /* to store error msgs */
    csRef<iDocumentNode>            root;   /* document root */
    csRef<iDocumentNode>            topNode; /* topnode to work with */
    csRef<iDocumentNodeIterator>    iter; /* node iterator */
    csRef<iDocumentNode>            node;   /* yet another node .... */
    SoundFile                      *snd;        ///< soundfile

    if (!(xml = csQueryRegistry<iDocumentSystem> (objectReg)))
      xml.AttachNew(new csTinyDocumentSystem);

    buff = vfs->ReadFile(filename);

    if ( !buff || !buff->GetSize())
    {
        return false;
    }

    doc = xml->CreateDocument();

    error = doc->Parse(buff);
    if (error)
    {
        Error3("Parsing file %s gave error %s", filename, error);
        return false;
    }


    if( !(root = doc->GetRoot()))
    {
        Error1("No XML root in soundlib.xml");
        return false;
    }


    if( !(topNode = root->GetNode("Sounds")))
    {
        Error1("No <sounds> tag in soundlib.xml");
        return false;
    }

    iter = topNode->GetNodes();

    while (iter->HasNext())
    {
        node = iter->Next();

        if (node->GetType() != CS_NODE_ELEMENT)
            continue;

        if (strcmp(node->GetValue(), "Sound") == 0)
        {
            snd = new SoundFile(node->GetAttributeValue("name"),
                                node->GetAttributeValue("file"));

            libsoundfiles.Put(csHashCompute((const char*) snd->name), snd);
       }
    }
    return true;
}

void SoundData::UnloadSoundLib ()
{
    csArray<SoundFile*> allsoundfiles;

    allsoundfiles = libsoundfiles.GetAll();

    for (size_t i = 0; i < allsoundfiles.GetSize(); i++)
    {
        delete allsoundfiles[i];
    }
    
    libsoundfiles.DeleteAll();
}

