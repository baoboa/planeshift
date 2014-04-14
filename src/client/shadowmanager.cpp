/*
* shadowmanager.cpp - Author: Andrew Robberts
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

// CS INCLUDES
#include <psconfig.h>

// CEL INCLUDES
#include <iengine/mesh.h>
#include <csgeom/box.h>
#include <imesh/objmodel.h>
#include <iutil/cfgmgr.h>

// PS INCLUDES
#include "globals.h"
#include "shadowmanager.h"
#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"
#include "effects/pseffectobj.h"
#include "effects/pseffectobjtext.h"

// Used to determine if shadows are enabled or not
#define DEFAULT_FILE  "/planeshift/data/options/shadows_def.xml"
#define USER_FILE     "/planeshift/userdata/options/shadows.xml"

bool psShadowManager::WithinRange(GEMClientObject * object, const csBox3 & bbox) const
{
    if (!object || !object->GetMesh())
        return false;

    if (shadowRange < 0)
        return true;

    if (!bbox.Empty())
        return bbox.Overlap(object->GetBBox());
    else
    {
        GEMClientObject * mainPlayer = psengine->GetCelClient()->GetMainPlayer();
        if (!mainPlayer)
            return true;
        csVector3 diff = object->GetMesh()->GetMovable()->GetPosition() -
                         mainPlayer->GetMesh()->GetMovable()->GetPosition();
        return (diff.SquaredNorm() <= shadowRange*shadowRange);
    }
}

psShadowManager::psShadowManager()
{
    cfgmgr = psengine->GetConfig();
    shadowRange = cfgmgr->GetFloat("PlaneShift.Visuals.ShadowRange", -1.0f);
    
    bool result = Load(USER_FILE);
    if (!result)
    {
        result = Load(DEFAULT_FILE);
        if(!result)
        {
            Error2("Could not find file: %s", DEFAULT_FILE);
        }
    }
}

psShadowManager::~psShadowManager()
{

}

bool psShadowManager::Load(const char * filename)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root;
    iVFS* vfs;
    const char* error;

    vfs = psengine->GetVFS();
    assert(vfs);
    csRef<iDataBuffer> buff = vfs->ReadFile(filename);
      
    if (buff == NULL)
    {
        return false;
    }
    
    doc = psengine->GetXMLParser()->CreateDocument();
    assert(doc);
    error = doc->Parse( buff );
    if ( error )
    {
        Error3("Parse error in %s: %s", filename, error);
        return false;
    }
    if (doc == NULL)
        return false;

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("No root in XML %s", filename);
        return false;
    }

    csRef<iDocumentNode> shadowsNode = root->GetNode("shadows");
    if (!shadowsNode)
    {
        Error2("No shadows node in %s", filename);
        return false;
    }

    if ((csString) shadowsNode->GetAttributeValue("enabled") == "false")
		DisableShadows();
	else
		EnableShadows();
    
    return true;
}

void psShadowManager::CreateShadow(GEMClientObject * object)
{
    if (!shadowsEnabled)
        return;

    if (!object)
        return;

    // don't create one if the object already has a shadow
    if (object->GetShadow())
        return;

    csRef<iMeshWrapper> mesh = object->GetMesh();
    if (!mesh || !mesh->GetMeshObject())
        return;

    if (!WithinRange(object))
        return;

    // calculate a suitable size for this shadow
    const csBox3& boundBox = object->GetBBox();
    float scale = (boundBox.Max(0) + boundBox.Max(2)) * 0.75f;
    if (scale < 0.35f)
        scale = 0.35f;

    psEffectManager* effectMgr = psengine->GetEffectManager();

    // Create the effect
    unsigned int id = effectMgr->RenderEffect("shadow", csVector3(0,0,0), mesh);
    psEffect* effect = effectMgr->FindEffect(id);
    if (!effect)
        return;

    effect->SetScaling(scale, 1.0f);
    object->SetShadow(effect);
}

void psShadowManager::RemoveShadow(GEMClientObject * object)
{
    if (!object)
        return;

    psEffect * shadow = object->GetShadow();
    if (!shadow)
        return;

    psengine->GetEffectManager()->DeleteEffect(shadow->GetUniqueID());
    object->SetShadow(0);
}

void psShadowManager::RecreateAllShadows()
{
    if (!shadowsEnabled)
        return;
        
    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()->GetEntities();
    size_t len = entities.GetSize();
    for (size_t a=0; a<len; ++a)
    {
        if(entities[a]->HasShadow())
        {
            CreateShadow(entities[a]);
        }
    }
}

void psShadowManager::RemoveAllShadows()
{
    if (shadowsEnabled)
        return;

    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()->GetEntities();
    size_t len = entities.GetSize();
    for (size_t a=0; a<len; ++a)
    {
        RemoveShadow(entities[a]);
    }
}

float psShadowManager::GetShadowRange() const
{
    return shadowRange;
}

void psShadowManager::SetShadowRange(float shadowRange)
{
    this->shadowRange = shadowRange;
    cfgmgr->SetFloat("PlaneShift.Visuals.ShadowRange", shadowRange);
}

void psShadowManager::UpdateShadows()
{
    if (!shadowsEnabled)
        return;

    GEMClientObject * mainPlayer = psengine->GetCelClient()->GetMainPlayer();
    if (!mainPlayer)
        return;

    csBox3 bbox;
    csVector3 pos(mainPlayer->GetMesh()->GetMovable()->GetPosition());
    bbox.AddBoundingVertex(pos.x+shadowRange, pos.y+shadowRange, pos.z+shadowRange);
    bbox.AddBoundingVertexSmart(pos.x-shadowRange, pos.y-shadowRange, pos.z-shadowRange);

    const csPDelArray<GEMClientObject>& entities = psengine->GetCelClient()->GetEntities();
    size_t len = entities.GetSize();
    for (size_t a=0; a<len; ++a)
    {
        if (!WithinRange(entities[a], bbox))
        {
            RemoveShadow(entities[a]);
        }
        else if(entities[a]->HasShadow())
        {
            CreateShadow(entities[a]);
        }
    }
}


