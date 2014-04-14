/*
 * updaterengine.cpp - Author: Mike Gist
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

#include <csutil/md5.h>
#include <csutil/hash.h>
#include <csutil/stringarray.h>
#include <csutil/xmltiny.h>
#include <csutil/randomgen.h>

#include "updaterconfig.h"
#include "updaterengine.h"
#include "binarypatch.h"

#ifndef CS_COMPILER_MSVC
#include <unistd.h>
#endif

#ifdef _WIN32
#include <shlobj.h>
#endif

UpdaterEngine::UpdaterEngine(csStringArray& args, iObjectRegistry* object_reg, const char* appName):
    downloader(NULL)
{
    InfoShare *is = new InfoShare();
    hasGUI = false;
    Init(args, object_reg, appName, is);
}

UpdaterEngine::UpdaterEngine(csStringArray& args, iObjectRegistry* object_reg, const char* appName,
        InfoShare *infoshare):
    downloader(NULL)
{
    hasGUI = true;
    Init(args, object_reg, appName, infoshare);
}

void UpdaterEngine::Init(csStringArray& args, iObjectRegistry* _object_reg, const char* _appName,
        InfoShare* _infoShare)
{
    object_reg = _object_reg;
    vfs = csQueryRegistry<iVFS> (object_reg);
    if(!vfs)
    {
        printf("No VFS!\n");
        exit(1);
    }
    vfs->ChDir("/this/");
    config = new UpdaterConfig(args, object_reg, vfs);
    fileUtil = new FileUtil(vfs);
    appName = _appName;
    infoShare = _infoShare;

    if(vfs->Exists("/this/updater.log"))
    {
        fileUtil->RemoveFile("/this/updater.log");
    }
    log = vfs->Open("/this/updater.log", VFS_FILE_WRITE);
}

UpdaterEngine::~UpdaterEngine()
{
    log.Invalidate();
    delete fileUtil;
    delete config;
    fileUtil = NULL;
    config = NULL;
    if(!hasGUI)
    {
        delete infoShare;
    }
}

void UpdaterEngine::PrintOutput(const char *string, ...)
{
    csString outputString;
    va_list args;
    va_start (args, string);
    outputString.FormatV (string, args);
    va_end (args);
    infoShare->ConsolePush(outputString);
    printf("%s", outputString.GetData());

    if(log.IsValid())
    {
        log->Write(outputString.GetData(), outputString.Length());
    }    
}

void UpdaterEngine::Run()
{
    CheckForUpdates();
    infoShare->SetUpdateChecked(true);

    while(!infoShare->GetExitGUI())
    {
        csSleep(100);
        if(infoShare->GetCheckIntegrity())
        {
            CheckIntegrity();
            infoShare->SetCheckIntegrity(false);
        }
    }
}

void UpdaterEngine::CheckForUpdates()
{
    // Check if the updater is disabled because of third-party support.
    if(!config->IsUpdateEnabled())
        return;

    // Make sure the old instance had time to terminate (self-update).
    if(config->IsSelfUpdating())
        csSleep(1000);

    // Load current config data.
    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
    if(!root)
    {
        PrintOutput("Unable to get root node\n");
        return;
    }

    csRef<iDocumentNode> confignode = root->GetNode("config");
    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return;
    }

    // Load updater config
    if (!config->GetCurrentConfig()->Initialize(confignode))
    {
        PrintOutput("Failed to Initialize mirror config current!\n");
        return;
    }

    //load current mirrors (precedence to updateservers.xml this way)
    //we don't fail for now here
    csRef<iDocumentNode> serversRoot = GetRootNode(SERVERS_CURRENT_FILENAME);
    if(!serversRoot.IsValid())
    {
        PrintOutput("Unable to get root node!\n");
    }
    else
    {
        if(!config->GetCurrentConfig()->LoadMirrors(serversRoot))
        {
            PrintOutput("Failed to Initialize mirror config current!\n");
        }
    }

    // Initialise downloader.
    downloader = new Downloader(GetVFS(), config);

    // Set proxy
    downloader->SetProxy(GetConfig()->GetProxy().host.GetData(),
            GetConfig()->GetProxy().port);

    PrintOutput("Checking for updates to the updater: ");

    if(CheckUpdater())
    {
        PrintOutput("Update Available!\n");

        // Check if we have write permissions.
#ifdef _WIN32
        // TODO: Improve this.
        if(!IsUserAnAdmin() && !log.IsValid())
#else
        if(!log.IsValid())
#endif
        {
            PrintOutput("Please run this program with write permissions on your PlaneShift directory to continue updating.\n");
            infoShare->SetUpdateAdminNeeded(true);
            return;
        }

        // If using a GUI, prompt user whether or not to update.
        if(hasGUI)
        {
            infoShare->SetUpdateNeeded(true);         
            while(!infoShare->GetPerformUpdate())
            {
                // Make sure we die if we exit the gui as well.
                if(!infoShare->GetUpdateNeeded() || infoShare->GetExitGUI() || CheckQuit())
                {
                    infoShare->Sync();
                    delete downloader;
                    downloader = NULL;
                    return;
                }

                csSleep(100);
            }

            infoShare->Sync();
        }

        // Begin the self update process.
        SelfUpdate(false);

        if(hasGUI)
        {
            // If we're going to self-update, close the GUI.
            infoShare->SetExitGUI(true);
            infoShare->Sync();
        }

        return;
    } else {
        PrintOutput("No updates needed!\n");
    }
    PrintOutput("Checking for updates to all files: ");

    // Check for normal updates.
    if(CheckGeneral())
    {
        PrintOutput("Updates Available!\n");

        // Check if we have write permissions.
#ifdef _WIN32
        // TODO: Improve this.
        if(!IsUserAnAdmin() && !log.IsValid())
#else
        if(!log.IsValid())
#endif
        {
            PrintOutput("Please run this program with write permissions on your PlaneShift directory to continue updating.\n");
            infoShare->SetUpdateAdminNeeded(true);
            return;
        }

        // If using a GUI, prompt user whether or not to update.
        if(hasGUI)
        {
            infoShare->SetUpdateNeeded(true);
            while(!infoShare->GetPerformUpdate())
            {
                if(!infoShare->GetUpdateNeeded() || infoShare->GetExitGUI() || CheckQuit())
                {
                    infoShare->Sync();
                    delete downloader;
                    downloader = NULL;
                    return;
                }

                csSleep(100);
            }
            infoShare->Sync();
        }

        // Begin general update.
        GeneralUpdate();

        // Maybe this fixes a bug.
        fflush(stdout);

        // Mark as finished and sync with gui.
        if(hasGUI)
        {
            PrintOutput("\nUpdate finished!\n");
            infoShare->SetUpdateNeeded(false);
            infoShare->Sync();
        }
    }
    else
    {
        // Mark no updates needed.
        PrintOutput("No updates needed!\n");
        if(hasGUI)
        {
            infoShare->SetUpdateNeeded(false);
        }
    }

    // Clean up.
    delete downloader;
    downloader = NULL;
}

bool UpdaterEngine::CheckUpdater()
{
    // Download the latest updaterinfo and updateservers.xml
    bool infosuccess = downloader->DownloadFile("updaterinfo.xml", UPDATERINFO_FILENAME, false, true, 1, true);
    bool serversuccess = downloader->DownloadFile("updateservers.xml", SERVERS_FILENAME, false, true, 1, true);
    if(!infosuccess && !serversuccess) //try getting new servers from the default server
    {
        bool success = true;
        delete downloader;
        downloader = new Downloader(vfs);
        downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);
        if(!downloader->DownloadFile(FALLBACK_SERVER "updateservers.xml", SERVERS_FILENAME, true, true, 1, true))
        {
            PrintOutput("\nFailed to download servers info!\n");
            success = false;
        }

        //retry
        csRef<iDocumentNode> serversRoot = GetRootNode(SERVERS_FILENAME);
        if(!serversRoot.IsValid())
        {
            PrintOutput("Unable to get servers root node!\n");
            success = false;
        }
        else
        {
            if(!config->GetCurrentConfig()->LoadMirrors(serversRoot))
            {
                PrintOutput("Failed to Initialize mirror config current!\n");
                success = false;
            }
        }

        delete downloader;
        downloader = new Downloader(vfs, config);

        //if it was successfull call this function again to try again
        if(success)
            return CheckUpdater();

        //in any case we are done here
        return false;        
    }

    // Load new config data.
    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_FILENAME);
    if(!root)
    {
        PrintOutput("Unable to get updaterinfo root node!\n");
        return false;
    }

    csRef<iDocumentNode> confignode = root->GetNode("config");
    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return false;
    }

    if(!confignode->GetAttributeValueAsBool("active", true))
    {
        PrintOutput("The updater mirrors are down, possibly for an update. Please try again later.\n");
        return false;
    }

    if(!config->GetNewConfig()->Initialize(confignode))
    {
        PrintOutput("Failed to Initialize mirror config new!\n");
        return false;
    }

    csRef<iDocumentNode> serversRoot = GetRootNode(SERVERS_FILENAME);
    if(!serversRoot.IsValid())
    {
        PrintOutput("Unable to get servers root node!\n");
    }
    else
    {
        if(!config->GetCurrentConfig()->LoadMirrors(serversRoot))
        {
            PrintOutput("Failed to Initialize mirror config current!\n");
        }
    }

    // Check if we need to update the mirrors.
    // TODO: Actually update the local list.
    if(config->GetNewConfig()->GetMirrors().GetSize() != 0)
    {
        config->GetCurrentConfig()->GetMirrors() = config->GetNewConfig()->GetMirrors();
    }

    // Compare Versions.
    if(!config->UpdatePlatform()) return false;
    if(config->GetNewConfig()->GetUpdaterVersionLatestMajor() > UPDATER_VERSION_MAJOR)
    {
        return true;
    }
    else if(config->GetNewConfig()->GetUpdaterVersionLatestMajor() == UPDATER_VERSION_MAJOR)
    {
        return (config->GetNewConfig()->GetUpdaterVersionLatestMinor() > UPDATER_VERSION_MINOR);
    }
    return false;
}

bool UpdaterEngine::CheckGeneral()
{
    // Check if general updating is enabled.
    if(!config->GetCurrentConfig()->IsActive())
    {
        return false;
    }

    /*
     * Compare length of both old and new client version lists.
     * If they're the same, then compare the last lines to be extra sure.
     * If they're not, then we know there's some updates.
     */

    // Start by fetching the configs.
    const csRefArray<ClientVersion>& oldCvs = config->GetCurrentConfig()->GetClientVersions();
    const csRefArray<ClientVersion>& newCvs = config->GetNewConfig()->GetClientVersions();

    size_t oldSize = oldCvs.GetSize();
    size_t newSize = newCvs.GetSize();

    // Old is bigger than the new (out of sync), or same size.
    if(oldSize >= newSize)
    {
        // If both are empty then skip the extra name check!
        if(newSize != 0)
        {
            bool outOfSync = oldSize > newSize;

            if(!outOfSync)
            {
                for(size_t i=0; i<newSize; i++)
                {
                    ClientVersion* oldCv = oldCvs.Get(i);
                    ClientVersion* newCv = newCvs.Get(i);

                    csString name(newCv->GetName());
                    if(!name.Compare(oldCv->GetName()))
                    {
                        outOfSync = true;
                        break;
                    }
                }
            }

            if(outOfSync)
            {
                // There's a problem and we can't continue. Throw a boo boo and clean up.
                PrintOutput("\nLocal config and server config are incompatible!\n");
                PrintOutput("This is caused when your local version becomes out of sync with the update mirrors.\n");
                PrintOutput("To resolve this, run a repair.\n");
                if(hasGUI)
                {
                    infoShare->SetOutOfSync(true);
                    infoShare->Sync();
                }
            }
        }

        return false;
    }

    // Check for updates concerning this platform.
    for(size_t i=oldSize; i<newSize; ++i)
    {
        ClientVersion* cv = newCvs[i];
        if(strcmp(cv->GetMD5Sum(), "") || strcmp(cv->GetGenericMD5Sum(), ""))
            return true;
    }

    // No updates related to this platform are available.
    return false;
}


csString UpdaterEngine::GetCurrentClientVersion() {
    csString clientVersion = csString("?");

    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
    if(!root)
    {
        PrintOutput("Unable to get root node\n");
        return clientVersion.GetData();
    }

    csRef<iDocumentNode> confignode = root->GetNode("config");
    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return clientVersion.GetData();
    }

    // Get client versions.
    csRef<iDocumentNode> clientNode = confignode->GetNode("client");
    if(clientNode)
    {
        csRef<iDocumentNodeIterator> nodeItr = clientNode->GetNodes();
        while(nodeItr->HasNext())
        {
            csRef<iDocumentNode> cNode = nodeItr->Next();
            if(!strcmp(cNode->GetValue(),"version"))
            {
                clientVersion = csString(cNode->GetAttributeValue("name"));
            }
        }
    }
    else
    {
        PrintOutput("Unable to load client info!\n");
        return clientVersion.GetData();
    }

    return clientVersion.GetData();
}

csPtr<iDocumentNode> UpdaterEngine::GetRootNode(const char* nodeName, csRef<iDocument>* document)
{
    // Load xml.
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    if (!xml)
    {
        PrintOutput("Could not load the XML Document System\n");
        return NULL;
    }

    // Try to read file
    csRef<iDataBuffer> buf = vfs->ReadFile(nodeName);
    if (!buf || !buf->GetSize())
    {
        PrintOutput("Couldn't open xml file '%s'!\n", nodeName);
        return NULL;
    }

    csRef<iDocument> configdoc;

    // Try to parse file
    if(document)
    {
        *document = xml->CreateDocument();
        configdoc = *document;
    }
    else
    {
        configdoc = xml->CreateDocument();
    }

    const char* error = configdoc->Parse(buf);
    if (error)
    {
        PrintOutput("XML Parsing error in file '%s': %s.\n", nodeName, error);
        return NULL;
    }

    // Try to get root
    csRef<iDocumentNode> root = configdoc->GetRoot ();
    if (!root)
    {
        PrintOutput("Couldn't get config file rootnode!");
        return NULL;
    }

    return csPtr<iDocumentNode>(root);
}

/**
 * This function updates the pslaunch application and updater software by
 * downloading a new version, checking the MD5 sum, unzip the application
 * to a temporary file, executing the temporary updater and switching in
 * the new updater and executes the new one in turn.
 */
bool UpdaterEngine::SelfUpdate(int selfUpdating)
{
#ifdef CS_PLATFORM_WIN32
    // Info for CreateProcess.
    STARTUPINFO siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;
    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    memset(&piProcessInfo, 0, sizeof(piProcessInfo));
    siStartupInfo.cb = sizeof(siStartupInfo);
#endif

    // Check what stage of the update we're in.
    switch(selfUpdating)
    {
    case 1: // We've downloaded the new file and executed it.
    {
        PrintOutput("Copying new files!\n");

        // Construct executable names.
        csString realName = appName;
        csString tempName = appName;
        realName.Append(SELFUPDATER_POSTFIX);
        tempName.Append(SELFUPDATER_TEMPFILE_POSTFIX);

#ifdef CS_PLATFORM_MACOSX
        realName = appName;
        tempName = appName;
        realName.AppendFmt(".app/Contents/MacOS/%s_static", appName.GetData());
        tempName.AppendFmt(".app/Contents/MacOS/%s_tmp_static", tempName.GetData());
#endif

        // Delete the old updater file and copy the new in place.
        while(!fileUtil->RemoveFile("/this/" + realName))
        {
            csSleep(50);
        }
        fileUtil->CopyFile("/this/" + tempName, "/this/" + realName, true, true);

        // Copy any art and data.
        csString zip = appName;
        zip.AppendFmt("%s.zip", config->GetCurrentConfig()->GetPlatform());

        // Mount zip
        csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/" + zip);
        vfs->Mount("/selfzip/", realZipPath->GetData());

        csString artPath = "/art/";
        artPath.AppendFmt("%s.zip", appName.GetData());
        fileUtil->CopyFile("/selfzip/" + artPath, "/this/" + artPath, true, false, true);

        csString dataPath = "/data/gui/";
        dataPath.AppendFmt("%s.xml", appName.GetData());
        fileUtil->CopyFile("/selfzip/" + dataPath, "/this/" + dataPath, true, false, true);

        vfs->Unmount("/selfzip/", realZipPath->GetData());

        // Create a process of the new updater.
#ifdef CS_PLATFORM_WIN32
        CreateProcess(realName.GetData(), "selfUpdateSecond", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo);
#else
        if(fork() == 0) {
#ifdef CS_PLATFORM_MACOSX
            execl(realName, realName, "selfUpdateSecond", NULL);
#else
            execl("./"+realName, "./"+realName, "selfUpdateSecond", NULL);
#endif
        }
#endif

        return true;
    }

    case 2: // We're now running the new updater in the correct location.
    {
        // Clean up left over files.
        PrintOutput("\nCleaning up!\n\n");

        csString tempName = appName;
        tempName.Append(SELFUPDATER_TEMPFILE_POSTFIX);

#ifdef CS_PLATFORM_MACOSX
        tempName = appName;
        tempName.AppendFmt(".app/Contents/MacOS/%s_tmp_static", tempName.GetData());
#endif

        // Construct zip name.
        csString zip = appName;
        zip.AppendFmt("%s.zip", config->GetCurrentConfig()->GetPlatform());

        // Remove updater zip.
        fileUtil->RemoveFile("/this/" + zip);

        // Remove temp updater file.
        while(!fileUtil->RemoveFile("/this/" + tempName))
        {
            csSleep(50);
        }

        GetConfig()->SetSelfUpdating(false);
        return false;
    }

    default: // We need to extract the new updater and execute it.
    {
        PrintOutput("Beginning self update!\n");

        // Construct executable names.
        csString realName = appName;
        csString tempName = appName;
        realName.Append(SELFUPDATER_POSTFIX);
        tempName.Append(SELFUPDATER_TEMPFILE_POSTFIX);

#ifdef CS_PLATFORM_MACOSX
        realName = appName;
        tempName = appName;
        realName.AppendFmt(".app/Contents/MacOS/%s_static", appName.GetData());
        tempName.AppendFmt(".app/Contents/MacOS/%s_tmp_static", tempName.GetData());
#endif

        // Construct zip name.
        csString zip = appName;
        zip.AppendFmt("%s.zip", config->GetCurrentConfig()->GetPlatform());

        // Download new updater file.
        downloader->DownloadFile(zip, zip, false, true);

        csString md5sum = GetMD5OfFile("/this/" + zip);

        if(!(config->GetNewConfig() &&
                config->GetNewConfig()->GetUpdaterVersionLatestMD5() &&
                md5sum.Compare(config->GetNewConfig()->GetUpdaterVersionLatestMD5())))
        {
            PrintOutput("md5sum of updater zip does not match correct md5sum!!\n");
            fileUtil->RemoveFile("/this/" + zip);
            return false;
        }

        // md5sum is correct, mount zip and copy file.
        csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/" + zip);
        vfs->Mount("/selfzip/", realZipPath->GetData());

        // Replace the starter script in the unix environment.
#if defined(CS_PLATFORM_UNIX) && !defined(CS_PLATFORM_MACOSX)
        fileUtil->CopyFile("/selfzip/" + appName, "/this/" + appName, true, true);
#endif

        // Update mac specific files.
#ifdef CS_PLATFORM_MACOSX
        csStringArray macUpdateFiles;
        macUpdateFiles.Push(".app/Contents/Resources/English.lproj/InfoPlist.strings");
        macUpdateFiles.Push(".app/Contents/Resources/setup.icns");
        macUpdateFiles.Push(".app/Contents/Info.plist");
        macUpdateFiles.Push(".app/Contents/PkgInfo");
        macUpdateFiles.Push(".app/Contents/version.plist");

        for(int i=0; i<macUpdateFiles.GetSize(); i++)
        {
            fileUtil->CopyFile("/selfzip/" + appName + macUpdateFiles.Get(i), "/this/" + appName + macUpdateFiles.Get(i), true, true);
        }
#endif


        fileUtil->CopyFile("/selfzip/" + realName, "/this/" + tempName, true, true);
        vfs->Unmount("/selfzip/", realZipPath->GetData());

        // Create a new process of the updater temporary instance so we could switch in
        // the new executable.
#ifdef CS_PLATFORM_WIN32
        CreateProcess(tempName.GetData(), "selfUpdateFirst", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo);
#else
        // Move file permissions to the temp file so it could be copied to the correct
        // file in next step of the selfUpdater.
        csRef<FileStat> fs = fileUtil->StatFile(realName);
        fileUtil->SetPermissions(tempName, fs);

        vfs->Sync();

        if(fork() == 0) {
#ifdef CS_PLATFORM_MACOSX
            execl(tempName, tempName, "selfUpdateFirst", NULL);
#else
            execl("./"+tempName, "./"+tempName, "selfUpdateFirst", NULL);
#endif
        }
#endif

        GetConfig()->SetSelfUpdating(true);
        return true;
    }
    }
}

/*
 * This function updates our non-updater files to the latest versions,
 * writes new files and deletes old files.
 * This may take several iterations of patching.
 * After each iteration we need to update updaterinfo.xml.bak as well as the array.
 */
void UpdaterEngine::GeneralUpdate()
{
    // Start by fetching the configs.
    csRefArray<ClientVersion>& oldCvs = config->GetCurrentConfig()->GetClientVersions();
    const csRefArray<ClientVersion>& newCvs = config->GetNewConfig()->GetClientVersions();
    csRef<iDocument> updaterinfo;
    csRef<iDocumentNode> rootnode = GetRootNode(UPDATERINFO_CURRENT_FILENAME, &updaterinfo);
    csRef<iDocumentNode> confignode = rootnode->GetNode("config");

    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
        return;
    }

    // True if the platform is 'generic' (data only).
    bool genericPlatform = !strcmp(config->GetCurrentConfig()->GetPlatform(),
            config->GetCurrentConfig()->GetGeneric());

    // True if we're going to check the integrity of the game after update.
    bool repairAfterUpdate = false;

    // Main loop.
    while(CheckGeneral())
    {
        // Find starting point in newCvs from oldCvs.
        size_t index = oldCvs.GetSize();

        // Use index to find the first update version in newCvs.
        ClientVersion* newCv = newCvs.Get(index);

        /**
         * Two passes:
         * First is for the platform specific update.
         * Second is for the generic update.
         */
        for(int pass=1; pass<=2; ++pass)
        {
            // Check that this update is relevent to this platform.
            if(pass == 1 && !strcmp(newCv->GetMD5Sum(), ""))
                continue;

            // Only do second pass if the platform isn't 'generic'.
            if(pass == 2 && (genericPlatform || !strcmp(newCv->GetGenericMD5Sum(), "")))
                break;

            // Construct zip name.
            csString zip;
            if(pass == 1)
            {
                zip = config->GetCurrentConfig()->GetPlatform();
            }
            else
            {
                zip = config->GetCurrentConfig()->GetGeneric();
            }

            zip.AppendFmt("-%s.zip", newCv->GetName());

            // Download update zip.
            if(pass == 1 && !genericPlatform)
            {
                /*PrintOutput("\n");*/
                PrintOutput("Downloading platform specific update file..\n\n");
            }
            else
            {
                /*PrintOutput("\n");*/
                PrintOutput("Downloading generic update file..\n\n");
            }

            if(!downloader->DownloadFile(zip, zip, false, true))
            {
                PrintOutput("Failed to download the update file! Try again later.\n");
                return;
            }

            // Check md5sum is correct.
            csString md5sum = GetMD5OfFile("/this/" + zip);
            csString correctMD5Sum;
            if(pass == 1)
            {
                correctMD5Sum = newCv->GetMD5Sum();
            }
            else
            {
                correctMD5Sum = newCv->GetGenericMD5Sum();
            }

            if(!md5sum.Compare(correctMD5Sum))
            {
                PrintOutput("md5sum of client zip does not match correct md5sum!!\n");
                return;
            }

            // Mount zip
            csRef<iDataBuffer> realZipPath = vfs->GetRealPath("/this/" + zip);
            vfs->Mount("/zip/", realZipPath->GetData());

            // Parse deleted files xml, make a list.
            csRef<iDocumentNode> deletedrootnode = GetRootNode("/zip/deletedfiles.xml");
            if(deletedrootnode)
            {
                csRef<iDocumentNode> deletednode = deletedrootnode->GetNode("deletedfiles");
                csRef<iDocumentNodeIterator> nodeItr = deletednode->GetNodes();
                while(nodeItr->HasNext())
                {
                    csRef<iDocumentNode> node = nodeItr->Next();
                    csString file = node->GetAttributeValue("name");
                    fileUtil->RemoveFile("/this/" + file);
                }
            }

            // Parse new files xml, make a list.
            csRef<iDocumentNode> newrootnode = GetRootNode("/zip/newfiles.xml");
            if(newrootnode)
            {
                csRef<iDocumentNode> newnode = newrootnode->GetNode("newfiles");
                csRef<iDocumentNodeIterator> nodeItr = newnode->GetNodes();
                while(nodeItr->HasNext())
                {
                    csRef<iDocumentNode> node = nodeItr->Next();
                    csString name = node->GetAttributeValue("name");
                    bool exec = node->GetAttributeValueAsBool("exec");
                    fileUtil->CopyFile("/zip/" + name, "/this/" + name, true, exec);
                }
            }

            // Parse changed files xml, binary patch each file.
            csRef<iDocumentNode> changedrootnode = GetRootNode("/zip/changedfiles.xml");
            if(changedrootnode)
            {
                csRef<iDocumentNode> changednode = changedrootnode->GetNode("changedfiles");
                csRef<iDocumentNodeIterator> nodeItr = changednode->GetNodes();
                while(nodeItr->HasNext())
                {
                    csRef<iDocumentNode> next = nodeItr->Next();

                    csString newFilePath = next->GetAttributeValue("filepath");
                    csString diff = next->GetAttributeValue("diff");
                    csString oldFilePath = newFilePath;
                    oldFilePath.AppendFmt(".old");

                    // Start by checking whether the existing file is already up to date.
                    csString md5sum = GetMD5OfFile("/this/" + newFilePath);

                    csString fileMD5 = next->GetAttributeValue("md5sum");
                    csString oldMD5 = next->GetAttributeValue("oldmd5sum");

                    // If it's up to date then skip this file.
                    if(md5sum.Compare(fileMD5))
                    {
                        continue;
                    }
                    else if(!oldMD5.IsEmpty() && !md5sum.Compare(oldMD5))
                    {
                        PrintOutput("Skipping file %s because it has been modified - you may want to repair.\n", newFilePath.GetData());
                        continue;
                    }

                    // Move old file to a temp location ready for input.
                    fileUtil->MoveFile("/this/" + newFilePath, "/this/" + oldFilePath, true, false, true);

                    // Move diff to a real location ready for input.
                    fileUtil->CopyFile("/zip/" + diff, "/this/" + newFilePath + ".vcdiff", true, false, true);
                    diff = newFilePath + ".vcdiff";

                    // Get real paths.
                    csRef<iDataBuffer> oldFP = vfs->GetRealPath("/this/" + oldFilePath);
                    csRef<iDataBuffer> diffFP = vfs->GetRealPath("/this/" + diff);
                    csRef<iDataBuffer> newFP = vfs->GetRealPath("/this/" + newFilePath);

                    // Save permissions.
                    csRef<FileStat> fs = fileUtil->StatFile(oldFP->GetData());
#ifdef CS_PLATFORM_UNIX
                    if(next->GetAttributeValueAsBool("exec"))
                    {
                        fs->mode = fs->mode | S_IXUSR | S_IXGRP;
                    }
#endif

                    // Binary patch.
                    PrintOutput("Patching file %s: ", newFilePath.GetData());
                    if(!PatchFile(oldFP->GetData(), diffFP->GetData(), newFP->GetData()))
                    {
                        PrintOutput("Failed - reverting!\n");
                        fileUtil->RemoveFile("/this/" + newFilePath);
                        fileUtil->CopyFile("/this/" + oldFilePath, "/this/" + newFilePath, true, false);

                        if(!repairAfterUpdate && config->RepairFailed())
                        {
                            PrintOutput("Scheduling a repair at the end of the update!\n");
                            repairAfterUpdate = true;
                        }
                    }
                    else
                    {
                        PrintOutput("Done!\n");

                        // Check md5sum is correct.
                        PrintOutput("Checking for correct md5sum: ");
                        csString md5sum = GetMD5OfFile("/this/" + newFilePath);
                        if(!md5sum.IsEmpty())
                        {
                            csString fileMD5 = next->GetAttributeValue("md5sum");

                            if(!md5sum.Compare(fileMD5))
                            {
                                PrintOutput("\nMD5 of file %s does not match correct md5sum! Reverting file!\n", newFilePath.GetData());
                                fileUtil->RemoveFile("/this/" + newFilePath);
                                fileUtil->CopyFile("/this/" + oldFilePath, "/this/" + newFilePath, true, false);
                            }
                            else
                            {
                                PrintOutput("Success!\n");
                            }
                        }
                        else
                        {
                            PrintOutput("\nCould not get MD5 of patched file %s! Reverting file!\n", newFilePath.GetData());
                            fileUtil->RemoveFile("/this/" + newFilePath);
                            fileUtil->CopyFile("/this/" + oldFilePath, "/this/" + newFilePath, true, false);
                        }
                    }
                    // Clean up temp files.
                    fileUtil->RemoveFile("/this/" + oldFilePath);
                    fileUtil->RemoveFile("/this/" + diff, false);

                    // Set permissions.
                    if(fs.IsValid())
                    {
                        fileUtil->SetPermissions(newFP->GetData(), fs);
                    }
                }
            }

            // Unmount zip and delete.
            if(vfs->Unmount("/zip/", realZipPath->GetData()))
            {
                fileUtil->RemoveFile("/this/" + zip);
            }
            else
            {
                PrintOutput("Failed to unmount file %s\n", zip.GetData());
            }

            PrintOutput("\n");
        }

        // Add version info to updaterinfo.xml and oldCvs.
        csRef<iDocumentNode> newNode = confignode->GetNode("client")->CreateNodeBefore(CS_NODE_ELEMENT);
        csString md5 = "md5";
        newNode->SetValue("version");
        newNode->SetAttribute("name", newCv->GetName());
        newNode->SetAttribute(md5 + config->GetCurrentConfig()->GetPlatform(), newCv->GetMD5Sum());
        newNode->SetAttribute(md5 + config->GetCurrentConfig()->GetGeneric(), newCv->GetGenericMD5Sum());
        updaterinfo->Write(vfs, UPDATERINFO_CURRENT_FILENAME);
        oldCvs.PushSmart(newCv);
    }

    if(repairAfterUpdate)
    {
        CheckIntegrity(true);
    }
}

void UpdaterEngine::CheckIntegrity(bool automatic)
{
    if(automatic)
        PrintOutput("Fixing files which failed to update...\n");
    PrintOutput("Beginning integrity check!\n\n");


    //first try to load servers data.
    csRef<iDocumentNode> serversRoot = GetRootNode(SERVERS_CURRENT_FILENAME);
    bool success = false;
    if(serversRoot.IsValid() && config->GetCurrentConfig()->LoadMirrors(serversRoot))
        success = true;


    if(!success) //try getting the master server list first
    {
        PrintOutput("Failed to Initialize mirror config current!\n");

        // Check if we have write permissions.
        if(!log.IsValid())
        {
            PrintOutput("Please run this program with administrator permissions to run the integrity check.\n");
            return;
        }
        PrintOutput("Attempting to restore updateservers.xml!\n");

        downloader = new Downloader(vfs);
        downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);
        if(!downloader->DownloadFile(FALLBACK_SERVER "updateservers.xml", SERVERS_CURRENT_FILENAME, true, true, 1, true))
        {
            PrintOutput("\nFailed to download servers info!\n");
        }

        delete downloader;

        //retry
        serversRoot = GetRootNode(SERVERS_CURRENT_FILENAME);
        success = true;    
        if(!serversRoot.IsValid())
        {
            PrintOutput("Unable to get root node!\n");
            success = false;
        }
        else
        {
            if(!config->GetCurrentConfig()->LoadMirrors(serversRoot))
            {
                PrintOutput("Failed to Initialize mirror config current!\n");
                PrintOutput("Restoring updateservers.xml failed!\n");
                success = false;
            }
        }
    }

    //fallback on old system
    if(!success)
    {
        success = true;
        // try to Load current config data.
        csRef<iDocumentNode> confignode;
        csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
        if(!root.IsValid())
        {
            PrintOutput("Unable to get root node!\n");
            success = false;
        }
        else
        {
            confignode = root->GetNode("config");
            if (!confignode.IsValid())
            {
                PrintOutput("Couldn't find config node in configfile!\n");
                success = false;
            }
        }

        // Load updater config
        if (!config->GetCurrentConfig()->Initialize(confignode))
        {
            PrintOutput("Failed to Initialize mirror config current!\n");
            success = false;
        }
    }

    if(!success)
    {
        // Check if we have write permissions.
        if(!log.IsValid())
        {
            PrintOutput("Please run this program with administrator permissions to run the integrity check.\n");
            return;
        }

        PrintOutput("Attempting to restore updaterinfo.xml!\n");

        downloader = new Downloader(vfs);
        downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);
        if(!downloader->DownloadFile(FALLBACK_SERVER "updaterinfo.xml", UPDATERINFO_CURRENT_FILENAME, true, true, 1, true))
        {
            PrintOutput("\nFailed to download updater info!\n");
            return;
        }

        delete downloader;

        csRef<iDocumentNode> confignode;
        csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
        if(!root)
        {
            PrintOutput("Unable to get root node!\n");
            return;
        }

        confignode = root->GetNode("config");
        if (!confignode)
        {
            PrintOutput("Couldn't find config node in configfile!\n");
            return;
        }

        // Load updater config
        if (!config->GetCurrentConfig()->Initialize(confignode))
        {
            PrintOutput("Failed to Initialize mirror config current!\n");
            return;
        }
    }

    // Initialise downloader.
    downloader = new Downloader(vfs, config);

    // Set proxy
    downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);

    // Download new config.
    if(!downloader->DownloadFile("updaterinfo.xml", UPDATERINFO_CURRENT_FILENAME, false, true, 1, true))
    {
        PrintOutput("\nFailed to download updater info from a mirror!\n");

        //try redownloading
        // Check if we have write permissions.
        if(!log.IsValid())
        {
            PrintOutput("Please run this program with administrator permissions to run the integrity check.\n");
            return;
        }
        PrintOutput("Attempting to restore updateservers.xml!\n");

        downloader = new Downloader(vfs);
        downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);
        if(!downloader->DownloadFile(FALLBACK_SERVER "updateservers.xml", SERVERS_CURRENT_FILENAME, true, true, 1, true))
        {
            PrintOutput("\nFailed to download servers info!\n");
            return;
        }

        delete downloader;

        //retry
        serversRoot = GetRootNode(SERVERS_CURRENT_FILENAME);
        success = true;    
        if(!serversRoot.IsValid())
        {
            PrintOutput("Unable to get root node!\n");
            return;
        }
        else
        {
            if(!config->GetCurrentConfig()->LoadMirrors(serversRoot))
            {
                PrintOutput("Failed to Initialize mirror config current!\n");
                PrintOutput("Restoring updateservers.xml failed!\n");
                return;
            }
        }
        // retry Initialise downloader.
        downloader = new Downloader(vfs, config);

        // Set proxy
        downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);

        // Download new config.
        if(!downloader->DownloadFile("updaterinfo.xml", UPDATERINFO_CURRENT_FILENAME, false, true, 1, true))
        {
            PrintOutput("\nFailed to download updaterinfo!\n");
            return;
        }
    }
    else //keep ourselves up to date
    {
        downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);
        if(!downloader->DownloadFile("updateservers.xml", SERVERS_CURRENT_FILENAME, false, true, 1, true))
        {
            //not in the server let's try the fallback
            if(!downloader->DownloadFile(FALLBACK_SERVER "updateservers.xml", SERVERS_CURRENT_FILENAME, false, true, 1, true))
            {
                PrintOutput("\nFailed to download servers info!\n");
                return;
            }
        }

    }

    delete downloader;

    csRef<iDocumentNode> confignode;
    csRef<iDocumentNode> root = GetRootNode(UPDATERINFO_CURRENT_FILENAME);
    if(!root)
    {
        PrintOutput("Unable to get root node!\n");
    }

    confignode = root->GetNode("config");
    if (!confignode)
    {
        PrintOutput("Couldn't find config node in configfile!\n");
    }

    // Load updater config
    if (!config->GetCurrentConfig()->Initialize(confignode))
    {
        PrintOutput("\nFailed to Initialize mirror config current!\n");
        return;
    }

    // Initialise downloader.
    PrintOutput("Downloading integrity data:\n");
    downloader = new Downloader(vfs, config);

    // Set proxy
    downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);

    // Get the zip with md5sums.
    csString baseurl;
    csArray<Mirror> mirrors = config->GetCurrentConfig()->GetRepairMirrors();
    if(mirrors.GetSize() > 0)
    {
        csRandomGen random = csRandomGen();
        baseurl = mirrors[random.Get((uint32)mirrors.GetSize())].GetBaseURL();
    }
    else //fallback to old style
    {
        baseurl = config->GetCurrentConfig()->GetMirror(0)->GetBaseURL();
    }
    baseurl.Append("backup/");
    if(!downloader->DownloadFile(baseurl + "integrity.zip", INTEGRITY_ZIPNAME, true, true, 1, true))
    {
        PrintOutput("\nFailed to download integrity.zip!\n");
        return;
    }

    // Process the list of md5sums.
    csRef<iDataBuffer> realZipPath = vfs->GetRealPath(INTEGRITY_ZIPNAME);
    vfs->Mount("/zip/", realZipPath->GetData());

    bool failed = false;
    csRef<iDocumentNode> r = GetRootNode("/zip/integrity.xml");
    if(!r)
    {
        PrintOutput("Unable to get root node!\n");
        failed = true;
    }

    if(!failed)
    {
        csRef<iDocumentNode> md5sums = r->GetNode("md5sums");
        if (!md5sums)
        {
            PrintOutput("Couldn't find md5sums node!\n");
            failed = true;
        }

        if(!failed)
        {
            CheckAndUpdate(md5sums, baseurl, automatic);
        }
    }

    vfs->Unmount("/zip/", realZipPath->GetData());

    if(vfs->Exists(INTEGRITY_ZIPNAME))
        fileUtil->RemoveFile(INTEGRITY_ZIPNAME, true);

    return;
}

bool UpdaterEngine::UpdateFile(csString baseurl, csString mountPath, csString filePath, bool failedEx, bool inZipFile)
{
    PrintOutput("\nDownloading file: %s\n", filePath.GetData());

    csString downloadpath(mountPath);
    downloadpath.Append(filePath);

    csRef<iDataBuffer> rp;
    csRef<FileStat> fs;

    if(!inZipFile)
    {
        // Save permissions.
        #ifdef CS_PLATFORM_UNIX
            rp = vfs->GetRealPath(downloadpath);
            fs = fileUtil->StatFile(rp->GetData());
        #endif

        fileUtil->CopyFile(downloadpath, downloadpath + ".bak", true, false, true);
    }

    // Make parent dir if needed.
    csString parent = mountPath;
    fileUtil->MakeDirectory(parent.Truncate(parent.FindLast('/')));

    // Download file.
    if(!downloader->DownloadFile(baseurl + filePath, downloadpath, true, true, 1, true))
    {
        // Maybe it's in a platform specific subdirectory. Try that next.
        csString url = baseurl + config->GetCurrentConfig()->GetPlatform() + "/";
        if(!downloader->DownloadFile(url + filePath, downloadpath, true, true, 1, true))
        {
            if(!inZipFile)
            {
                // Restore file.
                if(vfs->Exists(downloadpath))
                    fileUtil->RemoveFile(downloadpath, true);
                fileUtil->MoveFile(downloadpath + ".bak", downloadpath, true, false, true);
            }
            PrintOutput(" Failed!\n");
            return false;
        }
    }

    if(!inZipFile)
    {
        #ifdef CS_PLATFORM_UNIX
            // Restore permissions.
            if(fs.IsValid())
            {
                if(failedEx)
                {
                    fs->mode = fs->mode | S_IXUSR | S_IXGRP;
                }
                fileUtil->SetPermissions(rp->GetData(), fs);
            }
        #endif

        if(!config->KeepingRepaired())
        {
            if(vfs->Exists(downloadpath + ".bak"))
                fileUtil->RemoveFile(downloadpath + ".bak", true);
        }
    }
    PrintOutput(" Success!\n");
    return true;
}

void UpdaterEngine::CheckAndUpdate(iDocumentNode* md5sums, csString baseurl, bool accepted)
{

    PrintOutput("Checking file integrity:\n");
    PrintOutput("Using mirror %s\n\n", baseurl.GetData());

    csRefArray<iDocumentNode> failed;

    CheckMD5s(md5sums, "/this/", accepted, &failed);

    size_t failedSize = failed.GetSize();
    if(failedSize == 0)
    {
        if(!accepted)
        {
            PrintOutput("All files passed the check!\n");
        }
    }
    else
    {
        char c = ' ';
        if(!accepted)
        {
            PrintOutput("The following files failed the check:\n");
            for(size_t i=0; i<failedSize; i++)
            {
                PrintOutput("%s\n", failed.Get(i)->GetAttributeValue("path"));
            }

            PrintOutput("\nDo you wish to download the correct copies of these files? (y/n)\n");

            if(!hasGUI)
            {
                while(c != 'y' && c != 'n')
                {
                    c = (char)getchar();
                }
            }
            else
            {
                infoShare->SetUpdateNeeded(true);
                while(infoShare->GetUpdateNeeded())
                {
                    if(CheckQuit())
                    {
                        infoShare->SetCancelUpdater(false);
                        return;
                    }

                    csSleep(100);
                }
                c = infoShare->GetPerformUpdate() ? 'y' : 'n';
                infoShare->SetPerformUpdate(false);
                infoShare->Sync();
            }
        }

        if(c == 'n')
        {
            if(vfs->Exists("/this/updaterinfo.xml.bak"))
            {
                fileUtil->RemoveFile("/this/updaterinfo.xml", true);
                fileUtil->MoveFile("/this/updaterinfo.xml.bak", "/this/updaterinfo.xml", true, false);
            }
        }
        else if(accepted || c == 'y')
        {
            for(size_t i=0; i<failedSize; i++)
            {
                if(CheckQuit())
                {
                    infoShare->SetCancelUpdater(false);
                    return;
                }

                if(config->RepairingInZip() && failed.Get(i)->GetAttributeValueAsBool("checkonly"))
                {
                    csRef<iDocumentNode> zipmd5sums;
                    csRef<iDocumentNodeIterator> zips = md5sums->GetNodes("zip");
                    while(zips->HasNext())
                    {
                        zipmd5sums = zips->Next();
                        if(!strcmp(failed.Get(i)->GetAttributeValue("path"), zipmd5sums->GetAttributeValue("path")))
                        {
                            csString realPath = zipmd5sums->GetAttributeValue("path");
                            csRef<iDataBuffer> rpdb = vfs->GetRealPath("/this/" + realPath);
                            realPath = rpdb->GetData();
                            csString path = zipmd5sums->GetAttributeValue("path");
                            path = path.Truncate(path.Length()-4);
                            vfs->Mount("/updatezip/", realPath);

                            csRefArray<iDocumentNode> failedInZip;
                            CheckMD5s(zipmd5sums, "/updatezip/", true, &failedInZip);

                            csString zipDownloadUrl = baseurl;
                            csString zipFilePath = failed.Get(i)->GetAttributeValue("path");
                            zipFilePath.Truncate(zipFilePath.Length()-4);
                            zipDownloadUrl.Append(zipFilePath);
                            zipDownloadUrl.Append("/");

                            for(size_t j=0; j<failedInZip.GetSize(); j++)
                            {
                                UpdateFile(
                                        zipDownloadUrl,
                                        "/updatezip/",
                                        failedInZip.Get(j)->GetAttributeValue("path"),
                                        failedInZip.Get(j)->GetAttributeValueAsBool("exec"),
                                        true
                                );
                            }

                            vfs->Unmount("/updatezip/", realPath);
                            break;
                        }
                    }

                    continue;
                }

                UpdateFile(
                        baseurl,
                        "/this/",
                        failed.Get(i)->GetAttributeValue("path"),
                        failed.Get(i)->GetAttributeValueAsBool("exec")
                );

            }

            if(vfs->Exists("/this/updaterinfo.xml.bak"))
                fileUtil->RemoveFile("/this/updaterinfo.xml.bak", true);
            PrintOutput("\nDone!\n");

            if(CheckUpdater())
            {
                SelfUpdate(false);
                if(hasGUI)
                {
                    // If we're going to self-update, close the GUI.
                    infoShare->SetExitGUI(true);
                    infoShare->Sync();
                }
            }

            infoShare->SetCurrentClientVersion(GetCurrentClientVersion());
            infoShare->Sync();
        }
    }
}

void UpdaterEngine::CheckMD5s(iDocumentNode* md5sums, csString mountPath, bool accepted, csRefArray<iDocumentNode> *failed)
{
    csRef<iDocumentNodeIterator> md5nodes = md5sums->GetNodes("md5sum");
    while(md5nodes->HasNext())
    {
        if(CheckQuit())
        {
            infoShare->SetCancelUpdater(false);
            return;
        }
        csRef<iDocumentNode> node = md5nodes->Next();

        csString platform = node->GetAttributeValue("platform");

        if(!config->UpdatePlatform() && !platform.Compare("all"))
            continue;

        if(!(platform.Compare(config->GetCurrentConfig()->GetPlatform())
                || platform.Compare("cfg") || platform.Compare("all")))
            continue;

        csString path = node->GetAttributeValue("path");
        csString md5sum = node->GetAttributeValue("md5sum");

        csString md5s = GetMD5OfFile(mountPath + path);
        if(md5s.IsEmpty())
        {
            // File is genuinely missing.
            failed->Push(node);
            continue;
        }

        if(!md5s.Compare(md5sum))
        {
            if(path.StartsWith(appName, true))
            {
                // schedule a SelfUpdate at the end
                continue;
            }
            failed->Push(node);
        }
    }
}

bool UpdaterEngine::SwitchMirror()
{
    // Check if we have write permissions.
#ifdef _WIN32
    // TODO: Improve this.
    if(!IsUserAnAdmin() && !log.IsValid())
#else
    if(!log.IsValid())
#endif
    {
        PrintOutput("Please run this program with write permissions on your PlaneShift directory to continue updating.\n");
        return false;
    }

    csString xmlPath = UPDATERINFO_CURRENT_FILENAME;
    csString xmlBakPath;
    xmlBakPath.Format("%s_bak", xmlPath.GetData());

    fileUtil->CopyFile(xmlPath, xmlBakPath, true, false, true);

    // Initialise downloader.
    downloader = new Downloader(vfs);

    // Set proxy
    downloader->SetProxy(config->GetProxy().host.GetData(), config->GetProxy().port);

    // Download new xml file.
    csString xmlAddress;
    xmlAddress.Format("%s/updaterinfo.xml", config->GetNewMirrorAddress());
    if(!downloader->DownloadFile(xmlAddress, xmlPath, true, true, 1, true))
    {
        PrintOutput("Failed to download updaterinfo from new mirror.\n");
        fileUtil->MoveFile(xmlBakPath, xmlPath, true, false, true);
        return false;
    }

    if(vfs->Exists(xmlBakPath))
        fileUtil->RemoveFile(xmlBakPath, true);
    return true;
}

csString UpdaterEngine::GetMD5OfFile(csString filePath)
{
    // Check md5sum is correct.
    csRef<iDataBuffer> buffer = vfs->ReadFile(filePath, true);
    if (!buffer)
    {
        PrintOutput("Could not get MD5 of %s!!\n", filePath.GetData());
        return "";
    }

    return CS::Utility::Checksum::MD5::Encode(buffer->GetData(), buffer->GetSize()).HexString();
}
