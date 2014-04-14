/*
* worldeditor.h - Author: Mike Gist
*
* Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/ref.h>
#include "util/genericevent.h"

class pawsMainWidget;
class PawsManager;
struct iEngine;
struct iGraphics3D;
struct iKeyboardDriver;
struct iObjectRegistry;
struct iSceneManipulate;
struct iVirtualClock;
struct iView;

class WorldEditor
{
public:
    WorldEditor(int argc, char* argv[]);
    ~WorldEditor();

    void Run();

private:
    /* Handles an event from the event handler */
    bool HandleEvent(iEvent &ev);

    /* Init plugins, paws, world etc. */
    bool Init();

    /* Handles camera movement */
    void HandleMovement();

    /* Handles mesh manipulation */
    void HandleMeshManipulation(iEvent &ev);

    // Editing modes
    enum EditMode
    {
        Select = 0,
        Create,
        TranslateXZ,
        TranslateY,
        Rotate,
        Remove
    };

    EditMode editMode;

    // Whether to move the camera or the mesh.
    bool moveCamera;

    // CS
    iObjectRegistry* objectReg;
    csRef<iView> view;
    csRef<iGraphics3D> g3d;
    csRef<iEngine> engine;
    csRef<iKeyboardDriver> kbd;
    csRef<iVirtualClock> vc;
    DeclareGenericEventHandler(EventHandler, WorldEditor, "worldeditor");

    // PS
    PawsManager* paws;
    pawsMainWidget* mainWidget;
    csRef<iSceneManipulate> sceneManipulator;
    /// Current orientation of the camera.
    float rotX, rotY;

    // Event ids.
    csEventID MouseMove;
    csEventID MouseDown;
    csEventID KeyDown;
    csEventID FrameEvent;
};
