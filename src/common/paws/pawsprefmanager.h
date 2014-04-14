/*
 * pawsprefmanager.h - Author: Andrew Craig
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
// pawsprefmanager.h: interface for the pawsPrefManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_PREF_MANAGER_HEADER
#define PAWS_PREF_MANAGER_HEADER

#include <iutil/document.h>
#include <csutil/parray.h>
#include <csutil/csstring.h>

struct iObjectRegistry;
struct iVFS;
struct iGraphics2D;
struct iFont;
struct BorderDefinition;
class PawsManager;

/**
 * \addtogroup common_paws
 * @{ */

#define BORDER_COLOURS 5


/** Holds/Loads the prefs from a pref file.
 */
class pawsPrefManager
{
public:
    pawsPrefManager();
    virtual ~pawsPrefManager();

    bool LoadPrefFile(const char* file);
    bool LoadBorderFile(const char* file);

    iFont* GetDefaultFont(bool scaled = true)
    {
        return scaled?defaultScaledFont:defaultFont;
    }
    const char* GetDefaultFontName()
    {
        return defaultFontName;
    }
    int    GetDefaultFontColour()
    {
        return defaultFontColour;
    }
    int    GetBorderColour(int index)
    {
        return borderColours[index];
    }

    BorderDefinition* GetBorderDefinition(const char* name);
private:

    iObjectRegistry*        objectReg;
    csRef<iVFS>             vfs;
    csRef<iDocumentSystem>  xml;
    csRef<iGraphics2D>      graphics2D;

    csRef<iFont> defaultFont, defaultScaledFont;
    int defaultFontColour;
    csString defaultFontName;

    int borderColours[BORDER_COLOURS];

    void LoadBorderColours(iDocumentNode* node);

    csPDelArray<BorderDefinition> borders;
};

/** @} */

#endif

