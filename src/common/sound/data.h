/*
 * data.h
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

#ifndef _SOUND_DATA_H_
#define _SOUND_DATA_H_

/*
 * i still have to make this doxygen compatible .. please dont mock me
 * but i need to get this story out of my head and into this file
 *
 * How SoundData works: (Birth till Death)
 *
 * Birth:
 *
 * On Initialization it aquires references to vfs and a loader.
 * It returns false if it fails and no further checking is done.
 *
 * On succes the very first thing that may happen is that LoadSoundLib is called.
 * It fills libsoundfiles with the data it found in soundlib.xml
 * Even if not called we can still process requests for filenames within our vfs.
 *
 * We assume LoadSoundLib has been called and the library is filled.
 *
 * Now Someone calls LoadSoundFile to load a named resource or file
 * the one that calls expects a pointer to the SoundFile he requested
 * depending on how successfull this is LoadSoundFile will fill it in and return
 * true or false.
 *
 * GetSound is called and GetSound will search if theres a cached
 * SoundFile or if theres a unloaded resource in our soundlib.
 * if its cached then it will return that one. Thats best case
 * because LoadSoundFile can return a pointer to the cached data.
 * If its in our soundlib, then GetSound will make a copy and put that into our cache
 * (using PutSound) on succes it will return a Pointer to a valid SoundFile
 * on failure it will return NULL.
 *
 * GetSound may return NULL. In that case LoadSoundfile creates a new SoundFile
 * (object) with the given name as filename to check if we got a (dynamic)
 * file as name.
 * That happens if we are playing voicefiles. Those are downloaded from the server
 * and thus do not exist in our libraray. PutSound is used to add it to the cache.
 *
 * However theres always a valid SoundFile (object) for our loader
 * Now LoadSoundFile tries to load that file (finally).
 * There are two cases:
 * 1) file exists and is loadable (succes .. return true)
 * 2) file doesnt exist or ISNT loadable (failure .. return false)
 *
 * case 1) means that loaded (a bool) is set to true and snddata valid
 * case 2) loaded remains false and snddata is invalid / NULL
 *
 * LoadSoundFile has returned and the caller
 * might now do whatever he wanted todo with that snddata.
 *
 * Death:
 *
 * SoundData has a Update method that checks if there are expired SoundFiles
 * theres a hardcoded caching time of 300 seconds. After 300 seconds it will
 * check if there are still references on the snddata our SoundFile provides.
 * If theres only one then its the SoundFile object itself.
 * That means we go ahead and delete that object using DeleteSound.
 *
 * Now sometimes it isnt that easy ;) Maybe thats a looping background sound
 * in that case our RefCount is at last higher then one.
 * We set lasttouch to current ticks .. and check again .. in 300 seconds ;)
 *
 * Anyway UnloadSound is a public method and thus someone may call it for any
 * reason. Be aware!
 * It will crash your program if you unload data which is still in use ;)
 */

// FIXME i should be an option ;)
#define SOUNDFILE_CACHETIME     300000  ///<- number of milliseconds a file remains cached

#include <cssysdef.h>
#include <iutil/objreg.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <iutil/vfs.h>
#include <isndsys/ss_data.h>
#include <isndsys/ss_loader.h>

/**
 * Class that contains the most important informations about a soundfile
 * It contains the name, filename and if loaded the data.
 * its used by @see SoundData to manage our SoundFile(s). 
 */

class SoundFile
{
    public:
    csString            name;           ///< name of this file/resource MUST be unique
    csString            filename;       ///< filename in our vfs (maybe not unique)
    csRef<iSndSysData>  snddata;        ///< data in suitable format
    bool                loaded;         ///< true if snddata is loaded, false if not
    csTicks             lasttouch;      ///< last time when this SoundFile was used/touched

    /**
     * Constructs a SoundFile.
     * @param newname name of this SoundFile (must be unique)
     * @param newfilename filename this SoundFile is using
     */
    SoundFile(const char *newname, const char *newfilename);
    /**
     * Copy Constructor.
     * Copys the whole SoundFile.
     */
    SoundFile (SoundFile* const &copythat);
    /**
     * Destructor.
     */
    ~SoundFile();
};

/**
 * SoundData is the datakeeper of @see SoundSystemManager.
 * It loads and unloads all Soundfiles and provides a simple caching
 * mechanism.
 */

class SoundData
{
    public:

    /**
     * Constructor .. empty.
     * Initialization is done via Initialize because its not guaranteed
     * that its successful.
     */
    SoundData ();
    /**
     * Deconstructor.
     * Unloads everything and destroys all SoundFile object.
     */
    ~SoundData ();

    /**
     * Initializes Loader and VFS.
     * Will return true on success and false if not.
     * @param objectReg objectReg to get references to iVFS and iSndSysLoader 
     */
    bool Initialize(iObjectRegistry* objectReg);
    /**
     * Reads soundlib.xml and creates reference SoundFile objects.
     * It fills the private hash with unloaded SoundFile objects.
     * Those will be copied and loaded if needed
     * @param filename filename to load
     * @param objectReg ps objectreg because we need iDocumentSystem
     */ 
    bool LoadSoundLib (const char* filename, iObjectRegistry* objectReg);
    /**
     * Unloads everything LoadSoundLib created.
     * Will purge the hash and delete all reference SoundFile objects.
     */
    void UnloadSoundLib ();

    /**
     * Loads a soundfile out of the vfs.
     * The file given by name will be loaded into a iSndSysData object.
     * @param name filename to load
     * @param snddata iSndSysData object to write the data into
     */
    bool LoadSoundFile (const char *name, csRef<iSndSysData> &snddata);
    /**
     * Unloads a soundfile and deletes its snddata object.
     * The Soundfile given by name will be unloaded. Be careful with this one!
     * It doesnt check if its still in use.
     * @param name SoundFile by name
     */
    void UnloadSoundFile (const char *name);
    /**
     * Checks usage of all SoundFile objects and unloads them if appropriate.
     * Each SoundFile has a Timestamp and a Refcount that help to determine
     * if it can be freed. Unload happens if its Cachetime is expired if its
     * RefCount is 1. RefCount 1 means were the one that holds it. 
     */
    void Update ();

    private:
    csRef<iSndSysLoader> sndloader;         ///< Crystalspace soundloader
    csHash<SoundFile *> soundfiles;         ///< Hash of loaded SoundFiles
    csHash<SoundFile *> libsoundfiles;      ///< Hash of Resources soundlib.xml provides
    csRef<iVFS> vfs;                        ///< vfs where were reading from
    
    /**
     * Fetches a SoundFile object out of our cache.
     * @param name Name of the SoundFile we are searching.
     */
    SoundFile *GetSound (const char *name);
    /**
     * Adds a SoundFile object to our cache.
     */
    void PutSound (SoundFile* &sound);
    /**
     * Delete a SoundFile and deletes it from our hash.
     */
    void DeleteSound (SoundFile* &sound);
};

#endif /*_SOUND_DATA_H_*/
