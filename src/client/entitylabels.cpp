/*
 * entitylabels.cpp - Author: Ondrej Hurt
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

// CS INCLUDES
#include <psconfig.h>
#include <csutil/scf.h>
#include <csutil/csstring.h>
#include <csgeom/vector2.h>
#include <iutil/objreg.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <iengine/movable.h>
#include <ivideo/graph2d.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <cstool/objmodel.h>
#include <csutil/eventnames.h>

// PS INCLUDES
#include "util/log.h"

#include "gui/psmainwidget.h"

#include "paws/pawsmanager.h"

#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"
#include "effects/pseffectobj.h"
#include "effects/pseffectobjtextable.h"

#include "globals.h"
#include "entitylabels.h"
#include "psclientchar.h"


#define LABEL_FONT             "/this/data/ttf/LiberationSans-Regular.ttf"
#define SPRITE_PLUGIN_NAME     "crystalspace.mesh.object.sprite.2d"
#define LABEL_MESH_NAME        "EntityLabel"
#define ENTITY_LABEL_SPACING   5       // horizontal space between label parts
#define LABEL_MARGIN           2
#define CONFIG_FILE_NAME       "/planeshift/userdata/options/entitylabels.xml"
#define CONFIG_FILE_NAME_DEF   "/this/data/options/entitylabels_def.xml"

#define SCALE    0.004
#define BORDER_SIZE 2

int ParseColor(const csString & str, iGraphics2D *g2d);

psEntityLabels::psEntityLabels()
{    
    visCreatures = LABEL_ONMOUSE;
    visItems = LABEL_ONMOUSE;
    showGuild = true;
    
    underMouse = NULL;
    underTarget = NULL;
    
    //set default values for entity label colors
    entityColors[ENTITY_DEFAULT] = 0xff0000;
    entityColors[ENTITY_PLAYER] = 0x00ff00;
    entityColors[ENTITY_DEV] = 0xff8080;
    entityColors[ENTITY_TESTER] = 0x338ca7;
    entityColors[ENTITY_DEAD] = 0xff0000;
    entityColors[ENTITY_GM1] = 0x008000;
    entityColors[ENTITY_GM25] = 0xffff80;
    entityColors[ENTITY_NPC] = 0x00ffff;
    entityColors[ENTITY_GROUP] = 0x0080ff;
    entityColors[ENTITY_GUILD] = 0xf6dfa6;
}

psEntityLabels::~psEntityLabels()
{
    if (eventhandler)
        eventQueue->RemoveListener(eventhandler);    
}

bool psEntityLabels::Initialize(iObjectRegistry * object_reg, psCelClient * _celClient)
{
    celClient  = _celClient;
    
    vfs = psengine->GetVFS();
    CS_ASSERT(vfs);
    
    eventhandler.AttachNew(new EventHandler(this));

    eventQueue =  csQueryRegistry<iEventQueue> (object_reg);
    if(!eventQueue)
    {
        Error1("No iEventQueue found!");
        CS_ASSERT(eventQueue);
        return false;
    }

    csEventID esub[] = {
      csevFrame (object_reg),
      //csevMouseEvent (object_reg),
      CS_EVENTLIST_END
    };
    eventQueue->RegisterListener(eventhandler, esub);

    LoadFromFile();
    return true;
}

void psEntityLabels::Configure(psEntityLabelVisib _visCreatures, psEntityLabelVisib _visItems, bool _showGuild, int* colors)
{
    // Hide all on changed visibility to refresh what we're showing
    if (visCreatures != _visCreatures || visItems != _visItems)
        HideAllLabels();

    bool refreshGuilds = (showGuild != _showGuild);

    visCreatures = _visCreatures;
    visItems = _visItems;
    showGuild = _showGuild;

    // Refresh guild labels if our display option has changed
    if (refreshGuilds)
        RefreshGuildLabels();

    ///WARNING: memory for colors MUST be alocated and have size >= ENTITY_TYPES_AMOUNT*sizeof(int)!
    if (colors == NULL)
    {
        Error1("can't configure colors");
        return;
    }
    entityColors[ENTITY_DEFAULT] = colors[ENTITY_DEFAULT];
    entityColors[ENTITY_PLAYER] = colors[ENTITY_PLAYER];
    entityColors[ENTITY_DEV] = colors[ENTITY_DEV];
    entityColors[ENTITY_TESTER] = colors[ENTITY_TESTER];
    entityColors[ENTITY_DEAD] = colors[ENTITY_DEAD];
    entityColors[ENTITY_GM1] = colors[ENTITY_GM1];
    entityColors[ENTITY_GM25] = colors[ENTITY_GM25];
    entityColors[ENTITY_NPC] = colors[ENTITY_NPC];
    entityColors[ENTITY_GROUP] = colors[ENTITY_GROUP];
    entityColors[ENTITY_GUILD] = colors[ENTITY_GUILD];
}

void psEntityLabels::GetConfiguration(psEntityLabelVisib & _visCreatures, psEntityLabelVisib & _visItems, bool & _showGuild, int* colors)
{
    _visCreatures = visCreatures;
    _visItems = visItems;
    _showGuild = showGuild;
    ///WARNING: memory for colors MUST be alocated and have size >= ENTITY_TYPES_AMOUNT*sizeof(int)!
    if (colors == NULL)
    {
        Error1("can't get colors");
        return;
    }
    colors[ENTITY_DEFAULT] = entityColors[ENTITY_DEFAULT];
    colors[ENTITY_PLAYER] = entityColors[ENTITY_PLAYER];
    colors[ENTITY_DEV] = entityColors[ENTITY_DEV];
    colors[ENTITY_TESTER] = entityColors[ENTITY_TESTER];
    colors[ENTITY_DEAD] = entityColors[ENTITY_DEAD];
    colors[ENTITY_GM1] = entityColors[ENTITY_GM1];
    colors[ENTITY_GM25] = entityColors[ENTITY_GM25];
    colors[ENTITY_NPC] = entityColors[ENTITY_NPC];
    colors[ENTITY_GROUP] = entityColors[ENTITY_GROUP];
    colors[ENTITY_GUILD] = entityColors[ENTITY_GUILD];
}

bool psEntityLabels::HandleEvent(iEvent& /*ev*/)
{
    static unsigned int count = 0;
    if (++count%10 != 0)  // Update once every 10th frame
        return false;

    if (celClient->GetMainPlayer() == NULL)
        return false;  // Not loaded yet

    if (visItems == LABEL_ALWAYS || visCreatures == LABEL_ALWAYS)
    {
        UpdateVisibility();
    }

    if (visItems == LABEL_ONMOUSE || visCreatures == LABEL_ONMOUSE)
    {
        UpdateMouseover();
    }

    if (visItems == LABEL_ONTARGET || visCreatures == LABEL_ONTARGET)
    {
        UpdateTarget();
    }   

    return false;
}

void psEntityLabels::SetObjectText(GEMClientObject* object)
{
    if ( !object )
    {
        Warning1( LOG_ANY, "NULL object passed..." );
        return;
    }

    int colour = entityColors[ENTITY_DEFAULT];  // Default color, on inanimate objects
    if ( object->IsAlive() )
    {
        int type = object->GetMasqueradeType();
        if (type > 26)
            type = 26;

        switch ( type )    
        {
            default:
            case 0: // player or unknown group
                colour = entityColors[ENTITY_PLAYER];
                break;

            case -1: // NPC
                colour = entityColors[ENTITY_NPC];
                break;
            case -3: // DEAD
                colour = entityColors[ENTITY_DEAD];
                break;

            case 10: // Tester
                colour = entityColors[ENTITY_TESTER];
                break;
            case 21: // GM1
                colour = entityColors[ENTITY_GM1];
                break;            

            case 22:
            case 23:
            case 24:
            case 25: // GM2-5
                colour = entityColors[ENTITY_GM25];
                break;

            case 26: // dev char
                colour = entityColors[ENTITY_DEV];
                break;
        }                    
    }

    // Is our object an actor?
    GEMClientActor* actor = dynamic_cast<GEMClientActor*>(object);

    // Grouped with have other color
    bool grouped = false;
    if (actor && actor->IsGroupedWith(celClient->GetMainPlayer()))
    {
      colour = entityColors[ENTITY_GROUP];
      grouped = true;
    }

    // White colour labels for invisible objects should overide all (for living objects)
    int flags = object->Flags();
    bool invisible = object->IsAlive() && (flags & psPersistActor::INVISIBLE);
    
    if ( invisible )
    {
        colour = 0xffffff;                    
    }

    psEffectTextRow nameRow;
    psEffectTextRow guildRow;

    nameRow.text = object->GetName();
    nameRow.align = ETA_CENTER;
    nameRow.colour = colour;
    nameRow.hasShadow = false;
    nameRow.hasOutline = true;
    nameRow.outlineColour = 0;

    if (actor && showGuild)
    {
        csString guild( actor->GetGuildName() );
        csString guild2( psengine->GetGuildName() );

        if ( guild.Length() )
        {
            // If same guild, indicate with color, unless grouped or invisible
            if (guild == guild2 && !invisible && !grouped)
                colour = entityColors[ENTITY_GUILD];

            guildRow.text = "<" + guild + ">";
            guildRow.align = ETA_CENTER;
            guildRow.colour = colour;
            guildRow.hasShadow = false;
            guildRow.hasOutline = true;
            guildRow.outlineColour = 0;
        }
    }

    // Apply
    psEffect* entityLabel = object->GetEntityLabel();
    if(!entityLabel)
    {
        // Weird case
        Error2("Lost entity label of object %s!",object->GetName());
        return;
    }

    psEffectObjTextable* txt = entityLabel->GetMainTextObj();
    if(!txt)
    {
        // Ill-modded effect
        Error1("Effect 'entitylabel' has no text object");
        return;
    }

    size_t nameCharCount = nameRow.text.Length();
    size_t guildCharCount = guildRow.text.Length();
    size_t maxCharCount = nameCharCount > guildCharCount ? nameCharCount : guildCharCount;
    float scale = sqrt((float)maxCharCount) / 4.0f;

    // Finally set the text, with a black outline
    if (guildRow.text.Length())
    {
        txt->SetText(2, &nameRow, &guildRow);
        scale *= 1.5f;
    }
    else
    {
        txt->SetText(1, &nameRow);
    }        

    entityLabel->SetScaling(scale, 1.0f);
}

void psEntityLabels::CreateLabelOfObject(GEMClientObject *object)
{
    if (!object)
    {
        Debug1( LOG_ANY, 0, "NULL object passed to psEntityLabels::CreateLabelOfObject" );
        return;
    }

    // Don't make labels for the player or action locations
    if (object == celClient->GetMainPlayer() || object->GetObjectType() == GEM_ACTION_LOC)
        return;

    csRef<iMeshWrapper> mesh = object->GetMesh();

    // Has it got a mesh to attach to?
    if (!mesh || !mesh->GetMeshObject())
        return;

    DeleteLabelOfObject(object); // make sure the old label is gone

    // Get the height of the model
    const csBox3& boundBox = object->GetBBox();

    psEffectManager* effectMgr = psengine->GetEffectManager();

    // Create the effect
    unsigned int id = effectMgr->RenderEffect( "label", 
                                               csVector3(0.0f,boundBox.Max(1) + 0.25f,0.0f),
                                               mesh );

    psEffect* effect = effectMgr->FindEffect(id);

    object->SetEntityLabel(effect);

    // Update text
    SetObjectText(object);

    // Set to invisible by default.
    ShowLabelOfObject(object, false);
}


void psEntityLabels::DeleteLabelOfObject(GEMClientObject* object)
{
    if(!object)
        return;

    if(object->GetEntityLabel())
        psengine->GetEffectManager()->DeleteEffect(object->GetEntityLabel()->GetUniqueID());

    object->SetEntityLabel(NULL);
}


void psEntityLabels::OnObjectArrived( GEMClientObject* object )
{
    CS_ASSERT_MSG("Effects Manager must exist before loading entity labels!", psengine->GetEffectManager() );

    if(MatchVisibility(object->GetObjectType(), LABEL_NEVER))
    	return;

    // The controlled character never shows a label.
    if (object == celClient->GetMainPlayer())
        return;

    if(object->GetEntityLabel())
    {
        RepaintObjectLabel( object );
    } 
    else 
    {
        CreateLabelOfObject( object );

        //TODO: check this code it seems it doesn't check precisely the status
        //      of the label configuration
        if (visCreatures == LABEL_ONMOUSE || visItems == LABEL_ONMOUSE)
        {
            psPoint mouse = PawsManager::GetSingleton().GetMouse()->GetPosition();
            GEMClientObject *um = psengine->GetMainWidget()->FindMouseOverObject(mouse.x, mouse.y);
            if( um != object )
            {
                ShowLabelOfObject(object, false);
            }
        }
    }
}


bool psEntityLabels::LoadFromFile()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, entityLabelsNode, optionNode;
    csString option;
    
    csString fileName = CONFIG_FILE_NAME;
    if (!vfs->Exists(fileName))
    {
        fileName = CONFIG_FILE_NAME_DEF;
    }

    //csString fileName = CONFIG_FILE_NAME_DEF;
    
    doc = ParseFile(psengine->GetObjectRegistry(), fileName);
    if (doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("entitylabels.xml has no XML root");
        return false;
    }
    entityLabelsNode = root->GetNode("EntityLabels");
    if (entityLabelsNode == NULL)
    {
        Error1("entitylabels.xml has no <EntityLabels> tag");
        return false;
    }
    
    // Players, NPCs, monsters
    optionNode = entityLabelsNode->GetNode("Visibility");
    if (optionNode != NULL)
    {
        option = optionNode->GetAttributeValue("value");
        if (option == "always")
            visCreatures = LABEL_ALWAYS;
        else if (option == "mouse")
            visCreatures = LABEL_ONMOUSE;
        else if (option == "target")
            visCreatures = LABEL_ONTARGET;
        else if (option == "never")
            visCreatures = LABEL_NEVER;
    }
    
    optionNode = entityLabelsNode->GetNode("ShowGuild");
    if (optionNode != NULL)
    {
        option = optionNode->GetAttributeValue("value");
        showGuild = (option == "yes");
    }

    // Items
    optionNode = entityLabelsNode->GetNode("VisibilityItems");
    if (optionNode != NULL)
    {
        option = optionNode->GetAttributeValue("value");
        if (option == "always")
            visItems = LABEL_ALWAYS;
        else if (option == "mouse")
            visItems = LABEL_ONMOUSE;
        else if (option == "target")
            visItems = LABEL_ONTARGET;
        else if (option == "never")
            visItems = LABEL_NEVER;
    }    
    optionNode = entityLabelsNode->GetNode("playerColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_PLAYER] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("NPCColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_NPC] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("deadColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_DEAD] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("devColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_DEV] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("testerColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_TESTER] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("gm1Color");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_GM1] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("gm25Color");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_GM25] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("guildColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_GUILD] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("groupColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_GROUP] = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = entityLabelsNode->GetNode("defaultColor");
    if (optionNode != NULL)
    {
        entityColors[ENTITY_DEFAULT] = optionNode->GetAttributeValueAsInt("value");
    }

    return true;
}

inline void psEntityLabels::UpdateVisibility()
{
    csVector3 here = celClient->GetMainPlayer()->Pos();
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=0; i < entities.GetSize(); i++)
    {
        GEMClientObject* object = entities.Get(i);
        if (!object->HasLabel())
            continue;

        csRef<iMeshWrapper> mesh = object->GetMesh();
        if (!mesh)
            continue;

        // Don't show other player names unless introduced.
        if (object->GetObjectType() == GEM_ACTOR && !(object->Flags() & psPersistActor::NAMEKNOWN))
            continue;
        
        // Don't show labels of stuff whose label isn't always visible
        if (!MatchVisibility(object->GetObjectType(), LABEL_ALWAYS))
        	continue;
        
        // Only show labels within range
        csVector3 there = mesh->GetMovable()->GetPosition();
        int range = (object->GetObjectType() == GEM_ITEM) ? RANGE_TO_SEE_ITEM_LABELS : RANGE_TO_SEE_ACTOR_LABELS ;
        bool show = ( (here-there).Norm() < range );

        ShowLabelOfObject(object,show);
    }
}

inline void psEntityLabels::UpdateMouseover()
{
    GEMClientObject* lastUnderMouse = underMouse;

    // Get mouse position
    psPoint mouse = PawsManager::GetSingleton().GetMouse()->GetPosition();

    // Find out the object
    underMouse = psengine->GetMainWidget()->FindMouseOverObject(mouse.x, mouse.y);
    	
    // Is this a new object?
    if (underMouse != lastUnderMouse)
    {
        // Hide old
        if (lastUnderMouse != NULL && MatchVisibility(lastUnderMouse->GetObjectType(), LABEL_ONMOUSE))
            ShowLabelOfObject(lastUnderMouse,false);
        
        if (underMouse != NULL && !MatchVisibility(underMouse->GetObjectType(), LABEL_ONMOUSE))
        		return;

        // Show new
        if (underMouse != NULL)
        {
            // Don't show other player names unless introduced.
            if (underMouse->GetObjectType() == GEM_ACTOR && !(underMouse->Flags() & psPersistActor::NAMEKNOWN))
                return;

            csRef<iMeshWrapper> mesh = underMouse->GetMesh();
            if (mesh)
            {
                // Only show labels within range
                csVector3 here = celClient->GetMainPlayer()->Pos();
                csVector3 there = mesh->GetMovable()->GetPosition();
                int range = (underMouse->GetObjectType() == GEM_ITEM) ? RANGE_TO_SEE_ITEM_LABELS : RANGE_TO_SEE_ACTOR_LABELS ;
                bool show = ( (here-there).Norm() < range );

                ShowLabelOfObject(underMouse,show);
            }
        }
    }
}

inline void psEntityLabels::UpdateTarget()
{
    GEMClientObject* lastUnderTarget = underTarget;

    // Find out the object
    underTarget = psengine->GetCharManager()->GetTarget();

    // Is this a new object?
    if (underTarget != lastUnderTarget)
    {
        // Hide old
        if (lastUnderTarget != NULL && MatchVisibility(lastUnderTarget->GetObjectType(), LABEL_ONTARGET))
            ShowLabelOfObject(lastUnderTarget,false);
        
        if (underTarget != NULL && !MatchVisibility(underTarget->GetObjectType(), LABEL_ONTARGET))
            return;

        // Show new
        if (underTarget != NULL)
        {
            // Don't show other player names unless introduced.
            if (underTarget->GetObjectType() == GEM_ACTOR && !(underTarget->Flags() & psPersistActor::NAMEKNOWN))
                return;

            csRef<iMeshWrapper> mesh = underTarget->GetMesh();
            if (mesh)
            {
                // Only show labels within range
                csVector3 here = celClient->GetMainPlayer()->Pos();
                csVector3 there = mesh->GetMovable()->GetPosition();
                int range = (underTarget->GetObjectType() == GEM_ITEM) ? RANGE_TO_SEE_ITEM_LABELS : RANGE_TO_SEE_ACTOR_LABELS ;
                bool show = ((here-there).Norm() < range);

                ShowLabelOfObject(underTarget, show);
            }
        }
    }
}

bool psEntityLabels::SaveToFile()
{
    csString xml;
    csString visCreaturesStr, visItemsStr, showGuildStr, colorStr;

    switch (visCreatures)
    {
        case LABEL_ALWAYS:   visCreaturesStr = "always"; break;
        case LABEL_ONMOUSE:  visCreaturesStr = "mouse"; break;
        case LABEL_ONTARGET: visCreaturesStr = "target"; break;
        case LABEL_NEVER:    visCreaturesStr = "never"; break;
    }

    switch (visItems)
    {
        case LABEL_ALWAYS:   visItemsStr = "always"; break;
        case LABEL_ONMOUSE:  visItemsStr = "mouse"; break;
        case LABEL_ONTARGET: visItemsStr = "target"; break;
        case LABEL_NEVER:    visItemsStr = "never"; break;
    }
    
    showGuildStr = showGuild ? "yes" : "no";
    
    xml = "<EntityLabels>\n";
    xml += "    <Visibility value=\"" + visCreaturesStr + "\"/>\n";
    xml += "    <VisibilityItems value=\"" + visItemsStr + "\"/>\n";
    xml += "    <ShowGuild value=\"" + showGuildStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_PLAYER]);
    xml += "    <playerColor value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_NPC]);
    xml += "    <NPCColor value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_DEAD]);
    xml += "    <deadColor value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_DEV]);
    xml += "    <devColor value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_TESTER]);
    xml += "    <testerColor value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_GM1]);
    xml += "    <gm1Color value=\"" +colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_GM25]);
    xml += "    <gm25Color value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_GUILD]);
    xml += "    <guildColor value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_GROUP]);
    xml += "    <groupColor value=\"" + colorStr + "\"/>\n";
    colorStr = (entityColors[ENTITY_DEFAULT]);
    xml += "    <defaultColor value=\"" + colorStr + "\"/>\n";
    xml += "</EntityLabels>\n";
    
    return vfs->WriteFile(CONFIG_FILE_NAME, xml.GetData(), xml.Length());
}

void psEntityLabels::RemoveObject( GEMClientObject* object )
{
    DeleteLabelOfObject(object);

    if(underMouse == object)
        underMouse = NULL;
    if(underTarget == object)
        underTarget = NULL;
}

inline void psEntityLabels::ShowLabelOfObject(GEMClientObject* object, bool show)
{
    // The controlled character never shows a label.
    if (object == celClient->GetMainPlayer())
        return;

    if (object->GetEntityLabel() == NULL && show)
    {
        CreateLabelOfObject(object);
    }

    psEffect* entityLabel = object->GetEntityLabel();
    if (entityLabel != NULL)
    {
        psengine->GetEffectManager()->ShowEffect(entityLabel->GetUniqueID(),show);
    }
}

void psEntityLabels::RepaintAllLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=0; i < entities.GetSize(); i++)
        RepaintObjectLabel( entities.Get(i) );
}

void psEntityLabels::HideAllLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=0; i < entities.GetSize(); i++)
        ShowLabelOfObject( entities.Get(i), false );
}

void psEntityLabels::RefreshGuildLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=0; i < entities.GetSize(); i++)
    {
        GEMClientActor* actor = dynamic_cast<GEMClientActor*>(entities.Get(i));
        if ( actor && csString(actor->GetGuildName()).Length() )
            OnObjectArrived(actor);
    }
}

void psEntityLabels::RepaintObjectLabel(GEMClientObject* object)
{
    if (object && object->GetObjectType() != GEM_ACTION_LOC)
        SetObjectText(object);
}

void psEntityLabels::LoadAllEntityLabels()
{
    const csPDelArray<GEMClientObject>& entities = celClient->GetEntities();
    for (size_t i=0; i<entities.GetSize(); i++)
        OnObjectArrived( entities.Get(i) );
}

bool psEntityLabels::MatchVisibility(GEMOBJECT_TYPE type, psEntityLabelVisib vis)
{
	if(type == GEM_ACTOR)
	{
		return visCreatures == vis;
	}
	
	if(type == GEM_ITEM)
	{
		return visItems == vis;
	}
	
	return false;
}
