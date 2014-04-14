/*
 *  ccheck.h - Author: Mike Gist
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

class CCheck
{
public:
    CCheck(iObjectRegistry* object_reg);
    ~CCheck();

    void Run();

private:
    void PrintHelp();
    void ParseFile(const char* filePath, const char* fileName, bool processing);
    void PrintOutput(const char* string, ...);

    csRef<iObjectRegistry> object_reg;
    csRef<iVFS> vfs;
    csRef<iDocumentSystem> docsys;
    csRef<iStringSet> strings;

    csTinyDocumentSystem tinydoc;
    csString outpath;
    bool strip;

    csArray<csString> shaders;
    csArray<csString> textures;
    csArray<csString> materials;
    csArray<csString> meshfacts;

    csHash<csString, csString> ctextures;
    csHash<csString, csString> cmeshfacts;
    csRef<iFile> log;

    csRef<iDocumentNode> texmat;

    uint duplicateNum;
};
