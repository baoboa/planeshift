/*
 * updaterconfig.h - Author: Mike Gist
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

#ifndef __UPDATERCONFIG_H__
#define __UPDATERCONFIG_H__

#include <csutil/csstring.h>
#include <cstool/initapp.h>
#include <csutil/cfgmgr.h>
#include <csutil/cfgfile.h>
#include <iutil/document.h>

#include "util/singleton.h"

#define CONFIG_FILENAME "/this/pslaunch.cfg"
#define UPDATERINFO_FILENAME "/planeshift/userdata/updaterinfo.xml"
#define UPDATERINFO_CURRENT_FILENAME "/this/updaterinfo.xml"
#define INTEGRITY_ZIPNAME "/this/integrity.zip"
#define SERVERS_FILENAME "/planeshift/userdata/updateservers.xml"
#define SERVERS_CURRENT_FILENAME "/this/updateservers.xml"
#define UPDATE_CACHE_DIR "/planeshift/userdata/updatecache"
#define FALLBACK_SERVER "http://www.planeshift.it/"


class Mirror : public csRefCount
{
public:
    Mirror():
    id(0),  repair(false)
    {}
    
    ~Mirror() {}

    /* Return mirror ID */
    unsigned int GetID() const { return id; }

    /* Return mirror name */
    const char* GetName() const { return name; }

    /* Return mirror URL */
    const char* GetBaseURL() const { return baseURL; }

    /** Returns if the server is a server supporting repair. So that can be used for it.
     *  @return TRUE if the server supports repair
     */
    bool IsRepair() const { return repair; }

    /* Set mirror URL */
    void SetBaseURL(csString url) { baseURL = url; }

    void SetID(uint id) { this->id = id; }
    void SetName(const char* name) { this->name = name; }
    void SetBaseURL(const char* baseURL) { this->baseURL = baseURL; }

    /** Sets if the current server supports repair.
     *  @param repair Set this as true if this is a server supporting repair.
     */
    void SetIsRepair(const bool repair) { this->repair = repair; }

protected:
    /* Mirror ID */
    uint id;

    /* Mirror name */
    csString name;

    /* URL of the mirror (including update dir) */
    csString baseURL;

    ///Holds if this is a repair server
    bool repair;
};

class ClientVersion : public csRefCount
{
public:
    ClientVersion() {}
    ~ClientVersion() {}

    /* Get client update file name */
    const char* GetName() const { return name; }

    /* Get platform specific update file md5sum */
    const char* GetMD5Sum() const { return md5sum; }

    /* Get generic update file md5sum */
    const char* GetGenericMD5Sum() const { return genericmd5sum; }

    void SetName(const char* name) { this->name = name; }
    void SetMD5Sum(const char* md5sum) { this->md5sum = md5sum; }
    void SetGenericMD5Sum(const char* genericmd5sum) { this->genericmd5sum = genericmd5sum; }

private:
    /* Client update file name */
    csString name;

    /* md5sum of the platform specific update file */
    csString md5sum;

    /* md5sum of the generic update file */
    csString genericmd5sum;
};

struct Proxy
{
    /* Hostname of the proxy */
    csString host;
    /* Port */
    uint port;
};

/**
 * Holds an updater configuration file.
 */
class Config
{
public:
    Config();

    Mirror* GetMirror(uint mirrorNumber);
    csArray<Mirror> GetRepairMirrors();

    /**
     * Get mirrors.
     */
    csRefArray<Mirror>& GetMirrors() { return mirrors; }
    
    /**
     * Get clientVersions list.
     */
    csRefArray<ClientVersion>& GetClientVersions() { return clientVersions; }

    /**
     * Loads mirrors from an xml formatted entry.
     *
     * Eg:<pre>
     *  \<mirrors\>
     *      \<mirror id="1" name="Mirror1" url="http://www.mirror1.com/updaterdir" /\>
     *      \<mirror id="2" name="Mirror2" url="http://www.mirror2.com/updaterdir" /\>
     *  \</mirrors\></pre>
     *
     *  @param node The reference to the main node of an xml document containing the mirror list.
     *  @return FALSE if the mirrors entry was not found or no mirrors were added during the parsing.
     */
    bool LoadMirrors(iDocumentNode* node);

    /**
     * Init a xml config file.
     */
    bool Initialize(iDocumentNode* node);

    /**
     * Get our platform string.
     */
    const char* GetPlatform() const;

    const char* GetGeneric() const { return "generic"; }

    /**
     * Get latest updater version.
     */
    float GetUpdaterVersionLatest() const { return updaterVersionLatest; }

    /**
     * Get latest updater major version.
     * @return An unsigned int with the major version
     */
    unsigned int GetUpdaterVersionLatestMajor() const { return updaterVersionLatestMajor; }

    /**
     * Get latest updater minor version.
     * @return An unsigned int with the minor version
     */
    unsigned int GetUpdaterVersionLatestMinor() const { return updaterVersionLatestMinor; }

    /** Get latest updater version md5sum */
    const char* GetUpdaterVersionLatestMD5() const { return updaterVersionLatestMD5; }

    /** Get whether the updater mirror is active */
    bool IsActive() const { return active; }

private:
    /// Latest updater version
    float updaterVersionLatest;

    /// Latest updater version, only major version
    unsigned int updaterVersionLatestMajor;

    /// Latest updater version, only major version
    unsigned int updaterVersionLatestMinor;

    /* Latest version md5sum */
    csString updaterVersionLatestMD5;

    /* List of mirrors */
    csRefArray<Mirror> mirrors;

    /* List of client versions */
    csRefArray<ClientVersion> clientVersions;

    /* Whether the updater mirror is active */
    bool active;
};

class UpdaterConfig : public Singleton<UpdaterConfig>
{
public:
    UpdaterConfig(csStringArray& args, iObjectRegistry* _object_reg, iVFS* _vfs);
    ~UpdaterConfig();

    /**
     * Returns true if the updater is self updating.
     */
    int IsSelfUpdating() const { return selfUpdating; }

    /**
     * Returns true if a integrity check (repair) needs to be done.
     */
    bool CheckForIntegrity() const { return checkIntegrity; }

    /**
     * Returns true if a mirror switch needs to be done.
     */
    bool SwitchMirror() const { return switchMirror; }

    /**
     * Returns the proxy struct.
     */
    Proxy GetProxy() { return proxy; }

    /**
     * Returns the current/old config from updaterinfo\.xml.
     */
    Config* GetCurrentConfig() const { return currentCon; }

    /**
     * Returns the new/downloaded config from updaterinfo\.xml.
     */
    Config* GetNewConfig() const { return newCon; }

    /**
     * Returns true if the last update was successful.
     */
    bool WasCleanUpdate() const { return cleanUpdate; }

    /**
     * Returns true if we want the updater to update platform specific files.
     */
    bool UpdatePlatform() const { return updatePlatform; }

    /**
     * Returns true if we want to do in-zip repairs.
     */
    bool RepairingInZip() const { return repairInZip; }

    /**
     * Returns true if we want a repair to keep the old file (*\.bak).
     */
    bool KeepingRepaired() const { return keepRepaired; }

    /**
     * Returns true if we want to perform a repair when files fail after an update.
     */
    bool RepairFailed() const { return repairFailed; }

    /**
     * True if we want to use the updater. This could be turned of when third-party
     * updater is used.
     */
    bool IsUpdateEnabled() const { return updateEnabled; }

    /**
     * Returns the configfile for the app.
     */
    iConfigFile* GetConfigFile() const { return configFile; }

    void SetSelfUpdating(bool t) { selfUpdating = t ? 1 : 0; }

    /**
     * Returns the new mirror address.
     */
    const char* GetNewMirrorAddress() const { return newMirror; }

private:

    /* Holds stage of self updating. */
    int selfUpdating;

    /* Holds whether or not the last update was successful. */
    bool cleanUpdate;

    /* True if we want the updater to update platform specific files. */
    bool updatePlatform;

    /* True if we're being asked to check the integrity of the install. */
    bool checkIntegrity;

    /* True if we're being asked to switch the updater mirror. */
    bool switchMirror;

    /* True if we want to do in-zip repairs. */
    bool repairInZip;

    /* True if we want a repair to keep the old file (*.bak). */
    bool keepRepaired;

    /* True if we want to perform a repair when files fail after an update. */
    bool repairFailed;

    /* True if we want to use the updater. This could be turned of when third-party
     * updater is used
     */
    bool updateEnabled;

    /* Address of new mirror. */
    csString newMirror;

    /* VFS, Object Registry */
    csRef<iVFS> vfs;
    static iObjectRegistry* object_reg;

    /* Config Manager */
    csRef<iConfigManager> configManager;

    /* Config File */
    csRef<iConfigFile> configFile;

    /* Proxy server */
    Proxy proxy;

    /* Current Config */
    Config* currentCon;

    /* New Config */
    Config* newCon;
};

#endif // __UPDATERCONFIG_H__
