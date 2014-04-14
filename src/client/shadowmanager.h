/*
* shadowmanager.h - Author: Andrew Robberts
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

#ifndef SHADOW_MANAGER_H
#define SHADOW_MANAGER_H

// PS INCLUDES
#include "pscelclient.h"

struct iConfigManager;

class psShadowManager
{
private:
    csRef<iConfigManager>   cfgmgr;
    float                   shadowRange;
    bool                    shadowsEnabled;

    bool WithinRange(GEMClientObject * object, const csBox3 & bbox = csBox3()) const;
public:
    psShadowManager();
    ~psShadowManager();

    bool Load(const char * filename);

    void CreateShadow(GEMClientObject * object);
    void RemoveShadow(GEMClientObject * object);

    void RecreateAllShadows();
    void RemoveAllShadows();

    float GetShadowRange() const;
    void SetShadowRange(float shadowRange);

    void UpdateShadows();

    void DisableShadows() { shadowsEnabled = false; RemoveAllShadows(); }

    void EnableShadows() { shadowsEnabled = true; RecreateAllShadows(); }

    bool ShadowsEnabled() { return shadowsEnabled; }
};

#endif // SHADOW_MANAGER_H
