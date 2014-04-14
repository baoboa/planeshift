/*
 *  navgen.h - Author: Matthieu Kraus
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cssysdef.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iutil/cfgmgr.h>
#include <iengine/engine.h>
#include <ibgloader.h>
#include <tools/celhpf.h>

class NavGen
{
public:
    NavGen(iObjectRegistry* object_reg);
    ~NavGen();

    void Run();

private:
    void PrintHelp();
    csRef<iBgLoader> loader;
    csRef<iVFS> vfs;
    csRef<iEngine> engine;
    csRef<iCelHNavStructBuilder> builder;
    csRef<iConfigManager> config;
    csRef<iObjectRegistry> object_reg;
};

