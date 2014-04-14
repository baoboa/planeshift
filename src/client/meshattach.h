/*
 * meshattach.h - Author: Andrew Craig <andrew@hydlaa.com>
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
 #ifndef MESHATTACH_HEADER
 #define MESHATTACH_HEADER
 
//=============================================================================
// Crystal Space Includes
//=============================================================================

#include <csutil/csobject.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================

//=============================================================================
// Forward Declarations
//=============================================================================
class GEMClientObject;

/** Helper class to attach a PlaneShift GEM object to a particular mesh.
  * This is needed so we can get the GEM object when a player clicks on a mesh.
  */
class psGemMeshAttach : public scfImplementationExt1<psGemMeshAttach,
                                                     csObject,
                                                     scfFakeInterface<psGemMeshAttach> >
{
public:
    SCF_INTERFACE (psGemMeshAttach, 0, 0, 1);

    psGemMeshAttach (GEMClientObject* object);
    GEMClientObject* GetObject () const { return object; }    

protected:

private:

    GEMClientObject* object;      ///< The object we want to attach to the mesh.
};

#endif
