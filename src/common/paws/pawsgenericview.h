/*
 * Author: Andrew Craig
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
#ifndef PAWS_GENERIC_VIEW_HEADER
#define PAWS_GENERIC_VIEW_HEADER

#include <iengine/camera.h>
#include <csutil/deprecated_warn_off.h>
#include <iengine/engine.h>
#include <csutil/deprecated_warn_on.h>
#include <iengine/light.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>

#include <imesh/object.h>
#include <imesh/sprite3d.h>

#include <cstool/csview.h>
#include <csutil/leakguard.h>

#include "pawswidget.h"

struct iCollection;

/**
 * \addtogroup common_paws
 * @{ */

/**
 * This widget is used to view a mesh in it's own seperate world.
 */
class pawsGenericView : public pawsWidget
{
public:
    pawsGenericView();
    ~pawsGenericView();
    pawsGenericView(const pawsGenericView &origin);
    /** Creates the room ( world ) for the mesh to be placed.
     */
    bool Setup(iDocumentNode* node);

    /**
     * Loads a map to use as the backdrop.
     *
     * @param map The full path name of the map to load
     * @param sector The sector to load the map into.
     * @return True if the map was loaded correctly. False otherwise.
     */
    bool LoadMap(const char* map, const char* sector = 0);

    void Draw();

    iView* GetView()
    {
        return view;
    }

    const char* GetMapName() const
    {
        return mapName;
    }

private:
    bool CreateArea();

    csRef<iSector> stage;
    csRef<iView>   view;
    csRef<iEngine> engine;
    csRef<iCollection> col;

    csVector3 objectPos;
    static int idName;

    csString mapName;
    bool loadedMap;
};
CREATE_PAWS_FACTORY(pawsGenericView);


/** @} */

#endif
