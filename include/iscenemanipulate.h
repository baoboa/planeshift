/*
 * iscenemanipulate.h - Author: Mike Gist
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

#ifndef __ISCENEMANIPULATE_H__
#define __ISCENEMANIPULATE_H__

class csVector2;
struct iCamera;

enum PS_MANIPULATE // rotation type
{
    PS_MANIPULATE_NONE  = 0,
    PS_MANIPULATE_PITCH = 1,
    PS_MANIPULATE_YAW   = 2,
    PS_MANIPULATE_ROLL  = 4
};

struct iSceneManipulate : public virtual iBase
{
    SCF_INTERFACE(iSceneManipulate, 2, 1, 0);

   /**
    * Creates a new instance of the given factory at the given screen space coordinates.
    * @param factName The name of the factory to be used to create the mesh.
    * @param matName The optional name of the material to set on the mesh. Pass NULL to set none.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    virtual iMeshWrapper* CreateAndSelectMesh(const char* factName, const char* matName,
        iCamera* camera, const csVector2& pos) = 0;

   /**
    * Selects the closest mesh at the given screen space coordinates.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    virtual iMeshWrapper* SelectMesh(iCamera* camera, const csVector2& pos) = 0;

   /**
    * Translates the mesh selected by CreateAndSelectMesh() or SelectMesh().
    * @param vertical True if you want to translate vertically (along y-axis).
    * False to translate snapped to the mesh at the screen space coordinates.
    * @param camera The camera related to the screen space coordinates.
    * @param pos The screen space coordinates.
    */
    virtual bool TranslateSelected(bool vertical, iCamera* camera, const csVector2& pos) = 0;

   /**
    * Rotates the mesh selected by CreateAndSelectMesh() or SelectMesh().
    * @param pos The screen space coordinates to use to base the rotation, relative to the last saved coordinates.
    */
    virtual void RotateSelected(const csVector2& pos) = 0;

   /**
    * Set the axes to rotate around with RotateSelected.
    * @param horizontal_flags bitflag with axes rotated via horizontal movement.
    * @param vertical_flags bitflag with axes rotated via horizontal movement.
    * @see PS_MANIPULATE
    */
    virtual void SetRotation(int horizontal_flags, int vertical_flags) = 0;

   /**
    * Removes the currently selected mesh from the scene.
    */
    virtual void RemoveSelected() = 0;

   /**
    * retrieve the final position and rotation.
    */
    virtual void GetPosition(csVector3& pos, csVector3& rot, const csVector2& screenPos) = 0;

   /**
    * Sets the previous position (e.g. in case you warped the mouse)
    */
    virtual void SetPosition(const csVector2& pos) = 0;
};

#endif // __ISCENEMANIPULATE_H__
