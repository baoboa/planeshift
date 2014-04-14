/*
 * charapp.h
 *
 * Copyright (C) 2002-2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef CHAR_APP_HEADER
#define CHAR_APP_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/list.h>
#include <csgeom/vector3.h>
#include <iengine/mesh.h>
#include <imesh/animesh.h>
#include <imesh/spritecal3d.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "psengine.h"

struct Trait;
struct iEngine;
struct iLoader;
struct iVFS;
struct iTextureManager;
struct iGraphics3D;
struct iRegion;
struct iDocumentSystem;
struct iStringSet;

using namespace CS::Mesh;

/** Holds a set of skin tone.
    A skin tone can conisist of several different parts having different
    required materials on them.
*/
struct SkinToneSet
{
    csString part;                  ///< The part of the model to adjust.
    csString material;              ///< The material to place on that part.
};

/** A manager class that handles all the details of a characters appearance.
  */
class psCharAppearance : public DelayedLoader
{
public:
    /** Setup the class.
      * @param objectReg  The Crystal Space registry so we can aquire some needed
      *                   objects.
      */
    psCharAppearance(iObjectRegistry* objectReg);
    ~psCharAppearance();

    /** Set the mesh that is the base model character.
      * @param mesh The mesh that will be the core model that we want to manage
      *             the appearance for.
      */
    void SetMesh(iMeshWrapper* mesh);

    /** Set the material we want to use for the face.
      * @param faceMaterial The material to use.
      */
    void FaceTexture(csString& faceMaterial);

    /** Attach a hair sub mesh to the model.
      * @param submesh The name of the hair sub mesh to attach.
      */
    void HairMesh(csString& submesh);

    /** Attach a beard sub mesh to the model.
      * @param submesh The name of the beard sub mesh to attach.
      */
    void BeardMesh(csString& submesh);

    /** Set the colour of the hair.
      * @param shader the R,G,B value of the shader to use on the hair.
      */
    void HairColor(csVector3 &shader);

    /** Set the colour of the eyes.
     * @param shader the R,G,B value of the shader to use on the eyes.
     */
    void EyeColor(csVector3 &shader);

    /** Toggle the hair mesh on and off.
      * @param flag True if we want to show the hair. False if we want to hide it.
      */
    void ShowHair(bool flag=true);

    /** Toggle the meshes on and off.
      * @param slot The slot which which will apply/remove this restrain.
      * @param meshList The list of meshes which will be applied/removed this restrain.
      * @param show True if the slot is a restrain to showing the mesh. False if it's not anymore.
      */
    void ShowMeshes(csString& slot, csString& meshList, bool show=true);
    
    /** Checks if the mesh is hidden and in that case removes it.
      * Useful when we are adding a new mesh to check if it should really show.
      * @param meshName The name of the mesh to check for restrains.
      */
    void UpdateMeshShowStatus(csString& meshName);

    /** Set the skin tone.
      * @param part  The part of the model to change.
      * @param material The material to use on that part.
      */
    void SetSkinTone(csString& part, csString& material);

    /** Apply a set of traits based on a XML string.
      * @param traits The XML formated string that has a list of the traits
      *               to apply.
      */
    void ApplyTraits(const csString& traits);

    /**
     * Apply a set of equipment based on a XML string.
     *
     * @param equipment The XML formated string that has a list of the equipment
     *                  to apply.
     */
    void ApplyEquipment(const csString& equipment);

    /** Handle the visual aspect of mounting, by attaching the rider's mesh
      * on the back of the mount
      * @param mesh The mount's mesh
      */
    void ApplyRider(iMeshWrapper* mesh);

    /** Equip an item onto the model.
      * @param slotname    The socket we want to place the item.
      * @param mesh        The name of the 3D mesh to attach in above socket.
      * @param part        The part of the model we want to change ( ie Torso )
      * @param subMesh     The submesh we want to use ( ie $P_Plate )
      * @param texture     The texture to apply to that part.
      * @param removedMesh The mesh to remove when this item is equipped
      */
    void Equip(csString& slotname, csString& mesh, csString& part, csString& subMesh, csString& texture, csString& removedMesh);


    /** Remove an item from the model.
      * @param slotname    The socket we want to remove the item.
      * @param mesh        The name of the 3D mesh in above socket.
      * @param part        The part of the model we want to change ( ie Torso )
      * @param subMesh     The submesh we want to remove ( ie $P_Plate )
      * @param texture     The texture to apply to that part.
      * @param removedMesh The mesh to remove when this item is equipped
      */
    bool Dequip(csString& slotname, csString& mesh, csString& part, csString& subMesh, csString& texture, csString& removedMesh);

    /** Copy a current appearance class.
      * @param clone The class to copy.
      */
    void Clone(psCharAppearance* clone);


    /** Clears the equipment on a mesh. */
    void ClearEquipment(const char* slot = NULL);

    bool CheckLoadStatus();

    void SetSneak(bool sneaking);

    /**
     * Change the material on a part of the model.
     *
     * @param part The part of the model we want to change the materail on ( ex Torso )
     * @param materialName The name of the material to use.
     *
     * @return true if the material was changed.
     */
    bool ChangeMaterial(const char* part, const char* materialName);

private:
    /**
     * Parse a string from it's parts into a proper string.
     *
     * For example part Torso and str $P_Plate will give Torso_Plate.
     * @param part The part of them model to parse in.
     * @param str  The string to parse.
     *
     * @return The parsed string.
     */
    csString ParseStrings(const char *part, const char* str) const;

    /** Set a particular trait into place.
      * This breaks out into a switch to apply the trait to the correct location.
      * @param trait The trait to apply
      *
      * @return true if applied false if it could not be applied.
      */
    bool SetTrait(Trait * trait);

    /** Change a mesh on the model.
      * @param partPattern The part to change (ex Torso )
      * @param newPart The new mesh to place here ( exaple Plate_Torso ).
      *
      * @return true if mesh was changed.
      */
    bool ChangeMesh(const char* partPattern, const char* newPart);

    /** Attach an object into a socket on the model.
      * @param socketName The name of the cal3d socket on the model to place the item in.
      * @param meshFactName The name of the mesh factory of the item to place here.
      * @param materialName The optional name of a material to apply to the attached object.
      *
      * @return true if item was successfully attached to the model.
      */
    bool Attach(const char* socketName, const char* meshFactName, const char* materialName = NULL);

    /** Remove an item from a socket on the model.
      * @param socketName The name of the socket to remove the item from.
      *
      * @return True if the item was successfully removed from the socket.
      */
    bool Detach(const char* socketName);

    /** Set the default material back onto a particular part of the model.
      * @param part The part we want to set the default material back on.
      */
    void DefaultMaterial(csString& part);

    /** Set the default mesh back onto a particular part.
      * For example, plate armour is removed so need to place back the default torso
      * mesh.
      * @param part The part to replace the default mesh for.
      * @param subMesh The mesh to remove even if not starting his name with the partname
      */
    void DefaultMesh(const char* part, const char *subMesh = NULL);

    /** Free ressources allocated via the loader.
     *  Note that this frees the base mesh as well.
     */
    void Free();

    void ProcessAttach(iThreadReturn* factory, iThreadReturn* material, const char* meshFactName, const char* socket);
    void ProcessAttach(iMeshWrapper* meshWrap, const char* socket);
    void ProcessAttach(iThreadReturn* material, const char* materialName, const char* partName);
    bool ProcessMaterial(iThreadReturn* material, const char* materialName, const char* partName);

    csRef<iMeshWrapper> baseMesh;                       ///< The mesh that is our base model.

    csRef<iSpriteCal3DFactoryState>  stateFactory;      ///< The Cal3D factory object
    csRef<iSpriteCal3DState>    state;                  ///< The Cal3D sprite state

    csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory; ///< The animesh object factory.
    csRef<CS::Mesh::iAnimatedMesh> animeshObject; ///< The animesh object.

    csRef<iShaderVarStringSet>  stringSet;              ///< Used by shader variables.
    csRef<iStringSet> strings;                          ///< Used by shader types.

    /** @name Crystal space objects.
     */
    //@{
    csRef<iEngine>              engine;
    csRef<iVFS>                 vfs;
    csRef<iGraphics3D>          g3d;
    csRef<iTextureManager>      txtmgr;
    csRef<iDocumentSystem>      xmlparser;
    csRef<iShaderManager>       shman;
    //@}

    csString eyeMesh;                                   ///< Default eye mesh.
    csString hairMesh;                                  ///< Default hair mesh.
    csString beardMesh;                                 ///< Beard mesh.

    csVector3 eyeShader;                                ///< Default eye colour.
    csVector3 hairShader;                               ///< Default hair colour.

    bool beardAttached;                                 ///< Flag if beard is on/off.

    csHash<csString> removedMeshes;                     ///< Contains the mesh which have been removed from the model.
    csHash<csRef<iThreadReturn>, csString> materials;   ///< Contains the custom materials for the slots
    csHash<csRef<iThreadReturn>, csString> factories;   ///< Ctonains the custom factories for the slots

    bool eyeColorSet;                                   ///< Flag if eye colour set.
    bool hairColorSet;                                  ///< Flag if hair colour set.

    bool sneak;                                         ///< Flag if we're in sneak mode.

    csString faceMaterial;                              ///< Default face material.
    csArray<SkinToneSet> skinToneSet;                   ///< Default skin colours.

    csArray<csString> usedSlots;                        ///< Slots that have been used.
    csHash<int, csString> effectids;                    ///< Array of effects that are in use.
    csHash<int, csString> lightids;                     ///< Array of lights that are in use.

    /// Delayed loading.
    struct Attachment
    {
        bool factory;

        csString socket;
        csString partName;

        csString factName;
        csRef<iThreadReturn> factoryPtr;
        csString materialName;
        csRef<iThreadReturn> materialPtr;

        Attachment(bool factory) : factory(factory)
        {
        }
    };

    csList<Attachment> delayedAttach;
};

#endif
