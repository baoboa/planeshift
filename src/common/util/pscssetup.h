/*
 * cssetup.h - Authored by Elliot Paquette
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
// cssetup.h: registers classes from an xml file
//
////////////////////////////////////////////////////////////////////

#ifndef CSSETUP_HEADER
#define CSSETUP_HEADER

#include <cstool/initapp.h>

struct iConfigManager;
struct iConfigFile;
struct iVFS;
struct iObjectRegistry;

/**
 * \addtogroup common_util
 * @{ */

#ifndef PS_VERSION
#define PS_VERSION "0.6.1"
#endif

#ifndef APPNAME
#define APPNAME "PlaneShift Azure Spirit (" PS_VERSION ")"
#endif

#ifdef CS_COMPILER_GCC
#define PS_PAUSEEXIT(x) exit(x)
#endif


/** A helper class to setup Crystal Space and mount some dirs.
 */
class psCSSetup
{
public:
    psCSSetup(int, char**, const char*, const char*);
    ~psCSSetup();

    //Initialize CS
    iObjectRegistry* InitCS(iReporterListener* customReporter=0);

    iObjectRegistry* GetObjectRegistry()
    {
        return object_reg;
    }

    static iObjectRegistry* object_reg;

    /** Adds additional informations wraped in [] to the title bar.
     *
     *  @param Info The data to add inside the [] in the title bar.
     */
    bool AddWindowInformations(const char* Info);

    /** Sets an icon for the main window using a file name.
     *
     *  @param ImageFileLocation The VFS path to the icon file.
     *  @return true if the icon was found and was possible to set it up
     */
    bool SetIcon(const char* ImageFileLocation);

    /** Sets an icon for the main window using an iImage.
     *
     *  @param Image an iImage containing the imagine we want to set for the window.
     *  @return true if it was possible to ask cs to set the icon.
     */
    bool SetIcon(iImage* Image);

protected:
    //Unused part of initcs.inc, but included - better to than not to
    bool InitCSWindow(const char*);

    char* PS_GetFileName(char*);

    void MountUserData();

    int argc;
    char** argv;
    const char* engineConfigfile;
    const char* userConfigfile;

    csRef<iConfigFile>    cfg;
    csRef<iConfigManager> configManager;
    csRef<iVFS>           vfs;
};

/** @} */

#endif
