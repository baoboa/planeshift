/*
 * localization.h - Author: Ondrej Hurt
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



#ifndef LOCALIZATION_HEADER
#define LOCALIZATION_HEADER

#include <cstypes.h>
#include <csutil/hash.h>
#include <csutil/csstring.h>

struct iObjectRegistry;

/**
 * \addtogroup common_util
 * @{ */

struct psStringTableItem
{
    csString original;
    csString translated;
};

typedef csHash<psStringTableItem*,csString> psStringTableHash;

/** Localization class for languages.
 */
class psLocalization
{
public:
    psLocalization();
    ~psLocalization();
    
    void Initialize(iObjectRegistry* _object_reg);

    // Sets name of the language used by this program
    void SetLanguage(const csString & lang);

    /* Converts stardard path to a file to the VFS path to it.
     * It tries to search for the file in directory of the current language /this/lang/<language>/<shortPath> .
     * If not found, then the standard path is simply returned back with "/this/" prefix.
     *
     * Examples of shortPath:   "charpick.xml"
     *                          "art/buttons/group.gif"
     */
    csString FindLocalizedFile(const csString & shortPath);
    
    /// Translates string 'orig' using stringtable of the current language.
    const csString& Translate(const csString & orig);

    /// This is a utility function which queries for vfs and then checks for the file
    bool FileExists(const csString & fileName);

protected:
    void ClearStringTable();
    void WriteStringTable();
    
    csString language;
    csString filename;
    csString authors;
    bool dirty;
    psStringTableHash stringTbl;
    iObjectRegistry* object_reg;
};

/** @} */

#endif
