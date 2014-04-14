/*
* Author: Andrew Robberts
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

#include <psconfig.h>

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include  <iengine/scenenode.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <imesh/spritecal3d.h>
#include <imesh/nullmesh.h>
#include <imap/loader.h>

#include "effects/pseffectanchorsocket.h"

#include "util/pscssetup.h"
#include "util/log.h"

psEffectAnchorSocket::psEffectAnchorSocket()
    :psEffectAnchor()
{
    socket = 0;
    meshID = 0;
}

psEffectAnchorSocket::~psEffectAnchorSocket()
{
    if(socket)
        socket->DetachSecondary(meshID);
}

bool psEffectAnchorSocket::Load(iDocumentNode* node)
{

    // get the attributes
    name.Clear();
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if(attrName == "name")
            name = attr->GetValue();
        if(attrName == "socket")
            socketName = attr->GetValue();
    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect socket anchor with no name.\n");
        return false;
    }
    if(socketName.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect socket anchor without specifying a socket.\n");
        return false;
    }

    if(!psEffectAnchor::Load(node))
        return false;

    return true;
}

bool psEffectAnchorSocket::Create(const csVector3 &offset, iMeshWrapper* posAttach, bool rotateWithMesh)
{
    static unsigned long nextUniqueID = 0;
    csString anchorID = "effect_anchor_socket_";
    anchorID += nextUniqueID++;

    objBasePos = offset;
    objOffset = csVector3(0,0,0);
    this->rotateWithMesh = rotateWithMesh;

    if(!posAttach)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_WARNING, "planeshift_effects",
                 "Trying to attach an effect socket anchor to nothing.\n");
        return false;
    }

    cal3d =  scfQueryInterface<iSpriteCal3DState> (posAttach->GetMeshObject());
    if(!cal3d)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_WARNING, "planeshift_effects",
                 "Trying to attach an effect socket anchor to an invalid mesh type.\n");
        return false;
    }
    socket = cal3d->FindSocket(socketName);
    if(!socket)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_WARNING, "planeshift_effects",
                 "Trying to attach an effect socket anchor to a socket that doesn't exist.\n");
        return false;
    }

    mesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", anchorID);
    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (mesh->GetMeshObject());
    if(!state)
    {
        Error1("No NullMeshState.");
        return false;
    }
    state->SetRadius(1.0);
    mesh->QuerySceneNode()->SetParent(posAttach->QuerySceneNode());
    meshID = socket->AttachSecondary(mesh, csReversibleTransform());

    posAttach->GetMovable()->UpdateMove();
    isReady = false;

    initPos = mesh->GetMovable()->GetFullPosition();

    return true;
}

bool psEffectAnchorSocket::Update(csTicks elapsed)
{
    if(!isReady)
    {
        if(!mesh || !mesh->GetMovable())  //not valid anchor. so tell pseffect to remove us.
            return false;

        if(initPos == mesh->GetMovable()->GetFullPosition())
            return true;
        isReady = true;
    }

    life += (float)elapsed;
    if(life > animLength)
        life = fmod(life,animLength);
    if(!life)
        life += animLength;

    if(keyFrames->GetSize() == 0)
        return true;

    currKeyFrame = FindKeyFrameByTime(life);
    nextKeyFrame = currKeyFrame + 1;
    if(nextKeyFrame >= keyFrames->GetSize())
        nextKeyFrame = 0;

    // POSITION
    objOffset = lerpVec(csVector3(keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_X],
                                  keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Y],
                                  keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Z]),
                        csVector3(keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_X],
                                  keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Y],
                                  keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Z]),
                        keyFrames->Get(currKeyFrame)->time, keyFrames->Get(nextKeyFrame)->time, life);
    csMatrix3 mat;
    mat.Identity();
    socket->SetSecondaryTransform(meshID, csReversibleTransform(mat, objOffset));

    return true;
}

psEffectAnchor* psEffectAnchorSocket::Clone() const
{
    psEffectAnchorSocket* newObj = new psEffectAnchorSocket();
    CloneBase(newObj);

    // socket anchor specific
    newObj->socketName = socketName;

    return newObj;
}

void psEffectAnchorSocket::SetSocket(const char* name)
{
    if(!cal3d)
        socketName = name;
    else
    {
        iSpriteCal3DSocket* newSocket = cal3d->FindSocket(name);
        if(newSocket)
        {
            if(socket)
                socket->DetachSecondary(meshID);

            socketName = name;
            socket = newSocket;

            meshID = socket->AttachSecondary(mesh, csReversibleTransform());
            initPos = mesh->GetMovable()->GetFullPosition();
        }
    }
}

