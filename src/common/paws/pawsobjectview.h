/*
 * pawsobjectview.h - Author: Andrew Craig
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

#ifndef PAWS_OBJECT_VIEW_HEADER
#define PAWS_OBJECT_VIEW_HEADER

#include <iengine/rendermanager.h>
#include <iengine/camera.h>
#include <iengine/engine.h>
#include <iengine/light.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>

#include <imesh/object.h>

#include <cstool/csview.h>
#include <csutil/csstring.h>
#include <csutil/leakguard.h>
#include <ibgloader.h>
#include "pawswidget.h"

class psCharAppearance;
struct iBgLoader;

/**
 * \addtogroup common_paws
 * @{ */

/** This widget is used to view a mesh in it's own seperate world.
 */
class pawsObjectView : public pawsWidget
{
public:
    pawsObjectView();
    ~pawsObjectView();

    /** Make a copy of this mesh to view.
     * @param wrapper  Will use the factory of this mesh to create a copy of it.
     */
    void View(iMeshWrapper* wrapper);

    /** Use the specified mesh factory to create the mesh
     * @param wrapper  Will use this factory to create the mesh.
     */
    void View(iMeshFactoryWrapper* wrapper);

    /** View a mesh.
     * @param factName The name of the factory to use.
     */
    bool View(const char* factName);

    /** Creates the room ( world ) for the mesh to be placed.
     */
    bool Setup(iDocumentNode* node);

    /**
     * Loads a map to use as the backdrop.
     *
     * @param map The full path name of the map to load
     * @param sector The sector to use.
     *
     * @return True if the map was loaded correctly. False otherwise.
     */
    bool LoadMap(const char* map, const char* sector);

    /**
     * Continues an existing map load.
     */
    bool ContinueLoad(bool onlyMesh = false);

    /** Creates a default map. Creates a simple room to place object.
      *
      * @return True if the simple map is created.
      */
    bool CreateMap();

    void Clear();

    void Draw();
    void Draw3D(iGraphics3D*);

    void OnResize();

    iMeshWrapper* GetObject()
    {
        return mesh;
    }

    bool OnMouseDown(int button,int mod, int x, int y);
    bool OnMouseUp(int button,int mod, int x, int y);
    bool OnMouseExit();

    void Rotate(int speed,float radians); // Starts a rotate each {SPEED} ms, taking {RADIANS} radians steps
    void Rotate(float radians); // Rotate the object "staticly"

    void EnableMouseControl(bool v)
    {
        mouseControlled = v;
    }

    void SetCameraPosModifier(csVector3 &mod)
    {
        cameraMod = mod;
    }
    csVector3 &GetCameraPosModifier()
    {
        return cameraMod;
    }

    void LockCamera(csVector3 where, csVector3 at, bool mouseDownUnlock = false, bool mouseDownRotate = false);
    void UnlockCamera();

    /// Assign this view an ID
    void SetID(unsigned int id)
    {
        ID = id;
    }
    unsigned int GetID()
    {
        return ID;
    }

    void SetCharApp(psCharAppearance* cApp)
    {
        charApp = cApp;
    }
    psCharAppearance* GetCharApp()
    {
        return charApp;
    }

private:
    void DrawRotate();
    void DrawNoRotate();

    bool needsDraw;
    bool cameraLocked;
    bool doRotate;
    bool mouseDownUnlock;   ///< Checks to see if a right mouse down will break camera lock.
    bool mouseDownRotate;   ///< Checks to see if a left mouse will make the model rotate
    csVector3 cameraPosition;
    csVector3 lookingAt;

    csVector3 oldPosition;  ///< The old camera position before a lock.
    csVector3 oldLookAt;     ///< The old look at position before a lock.

    bool CreateArea();

    bool spinMouse;
    csVector2 downPos;
    float orgRadians;
    int orgTime;
    int downTime;
    bool mouseControlled;

    csRef<iBgLoader> loader;
    csRef<iSector> stage;
    csRef<iView>   view;
    csRef<iEngine> engine;
    csRef<iRenderManagerTargets> rmTargets;
    csRef<iMeshWrapper> mesh;
    csRef<iSector> meshSector;
    csRef<iView>   meshView;
    csRef<iTextureHandle> stageTarget;
    csRef<iTextureHandle> meshTarget;
    psCharAppearance* charApp;

    csVector3 objectPos;
    csVector3 cameraMod;

    void RotateDef(); // Used to reset to the values given by the controlling widget
    void RotateTemp(int speed,float radians); // Used to for example stop the rotate but not write the def values

    float distance;
    float origDistance;

    int rotateTime;
    float rotateRadians;
    float camRotate;

    unsigned int ID;

    csRef<iThreadReturn> meshFactory;
};
CREATE_PAWS_FACTORY(pawsObjectView);


/** @} */

#endif
