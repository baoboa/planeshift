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

#ifndef _SOUND_DATA_CACHE_H_
#define _SOUND_DATA_CACHE_H_


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <iutil/objreg.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <iutil/vfs.h>
#include <isndsys/ss_data.h>
#include <isndsys/ss_loader.h>

#define DEFAULT_SOUNDFILE_CACHETIME 300000


/**
 * This struct keeps the information about a sound file.
 * @see SoundDataCache, the class that uses this struct.
 */
struct SoundFile
{
public:
    csString            name;           ///< Identifier of this file/resource. MUST be unique.
    csString            fileName;       ///< File's name in our vfs. It doesn't need to be unique.
    csRef<iSndSysData>  sndData;        ///< Data in suitable format.
    csTicks             lastTouch;      ///< Last time when this SoundFile was used/touched.

    /**
     * Constructs a SoundFile.
     * @param newName identifier of this SoundFile (must be unique).
     * @param newFileName file's name this SoundFile is using.
     */
    SoundFile(const char* newName, const char* newFileName);

    /**
     * Copy constructor. It copies the whole SoundFile.
     * @param copySoundFile SoundFile to copy.
     */
    SoundFile(SoundFile* const &copySoundFile);

    /**
     * Destructor.
     */
    ~SoundFile();
};


//--------------------------------------------------


/**
 * SoundDataCache is the data-keeper of SoundSystemManager. It loads and unloads all
 * sound files and provides a simple caching mechanism.
 *
 * Birth:
 * After the object creation, it should be called the method Initialize in order to
 * aquire references to the virtual file system and the CS sound loader. If a sound
 * library is present it should be loaded with LoadSoundLib.
 *
 * Usage:
 * Sounds can be retrieved with GetSoundData. The provided iSndSysData should always
 * be used with a csRef since the reference counting is checked to determine if the
 * sound is still in use or not. Referencing the sound data with a normal pointer
 * could cause the application to crash by ending up with pointing to an object that
 * the cache has destroyed.
 *
 * Cache:
 * The data is cached and unloaded when it is not referenced anymore and the time
 * given in the configuration option "PlaneShift.Sound.DataCacheTime" has elapsed.
 * One can force the cache to unload a sound with UnloadSoundFile if it is known
 * that the sound won't be used again.
 *
 * Death:
 * It is not necessary to call UnloadSoundLib. The destructor takes care of it too.
 */
class SoundDataCache
{
public:

    /**
     * Constructor. Initialization is done via Initialize because it's not guaranteed
     * that it's successful.
     */
    SoundDataCache();

    /**
     * Deconstructor. Unloads everything and destroys all SoundFile objects.
     */
    ~SoundDataCache();

    /**
     * Initializes CS iSndSysLoader and look for the VFS.
     * @param objectReg iObjectRegistry to get references to iVFS and iSndSysLoader.
     * @return true on success, false otherwise.
     */
    bool Initialize(iObjectRegistry* objectReg);

    /**
     * Reads information contained in the given sound library XML file.
     * @param fileName the path to the XML sound library file.
     * @param objectReg ps iObjectRegistry because we need iDocumentSystem.
     * @return true on success, false otherwise.
     */ 
    bool LoadSoundLib(const char* fileName, iObjectRegistry* objectReg);

    /**
     * Unloads everything LoadSoundLib created and clean the cache.
     */
    void UnloadSoundLib();

    /**
     * Fetches a sound data from the cache and loads it from the VFS if necessary. If
     * the sound is not in the sound library then it considers the parameter "name" as
     * a path in the VFS and it tries to load the data from there.
     * @attention always use the provided iSndSysData with a csRef since the reference
     * counting is used to determine if the sound is still in use or not. Referencing
     * the sound data with a normal pointer could cause the application to crash by
     * ending up with pointing to an object that the cache has destroyed.
     * @param name the name of the sound to fetch (or its path if it is not in the
     * library.
     * @param sndData object that will contain the sound data at the end.
     * @return true on success, false if the sound data couldn't be retrieved.
     */
    bool GetSoundData(const char* name, csRef<iSndSysData> &sndData);

    /**
     * Forces the cache to unload and delete the data of the given sound from the
     * memory, no matter if it's still in use or not.
     * @param namethe name of the sound to unload.
     */
    void UnloadSoundFile(const char* name);

    /**
     * Checks the reference counting to sounds to determine if they are still in use
     * or not. Unloads the sound data that has not been used for the time specified
     * in the configuration option "PlaneShift.Sound.DataCacheTime".
     */
    void Update();

private:
    uint                         cacheTime;         ///< Number of milliseconds a file remains cached.
    csRef<iVFS>                  vfs;               ///< VFS used to retrieve the sound library.
    csRef<iSndSysLoader>         sndLoader;         ///< Crystal Space sound loader.
    csHash<SoundFile*, csString> libSoundFiles;     ///< Maps the sounds' identifiers with their data.
    csHash<SoundFile*, csString> loadedSoundFiles;  ///< Hash of loaded SoundFiles.
    
    /**
     * Load the sound data into a SoundFile from the VFS (if not already loaded). If
     * the sound is not in libSoundFiles then it considers the parameter "name" as a
     * path in the VFS and it tries to load the data from there. This is useful for
     * dynamically generated sounds (downloaded from the server for example).
     * @param name the name of the sound we are searching (or its path if it is not
     * in the library.
     * @return a pointer to the loaded SoundFile or 0 if the data couldn't be loaded.
     */
    SoundFile* LoadSoundFile(const char* name);
};

#endif // _SOUND_DATA_CACHE_H_
