/*
 * entitylabels.h - Author: Ondrej Hurt
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



#ifndef ENTITYLABEL_HEADER
#define ENTITYLABEL_HEADER

// CS INCLUDES
#include <cstypes.h>
#include <csutil/ref.h>
#include <imesh/sprite2d.h>
#include <ivideo/fontserv.h>
#include <csutil/leakguard.h>

// PS INCLUDES
#include "pscelclient.h"
#include "util/genericevent.h"

struct iPluginManager;
struct iFont;
struct iEventQueue;
struct iEvent;

/** psEntityLabelVisib defines possible visibility modes of labels */
enum psEntityLabelVisib 
     { 
        LABEL_ALWAYS = 0,         ///< labels of all entities are visible
        LABEL_ONMOUSE,            ///< only label of the entity under mouse is visible
        LABEL_ONTARGET,           ///< only label of the targeted entity is visible
        LABEL_NEVER               ///< no labels are visible
     };
///psRntityLabelType defines types of entities that have own color
enum psEntityLabelType
    {
        ENTITY_DEFAULT=0,       ///< default or unmovable entities
        ENTITY_PLAYER,          ///< players
        ENTITY_NPC,             ///< NPCs
        ENTITY_DEAD,            ///< dead bodies
        ENTITY_GM1,             ///< GM 1
        ENTITY_GM25,            ///< GM 2-5
        ENTITY_TESTER,          ///< testers
        ENTITY_DEV,             ///< developers
        ENTITY_GROUP,           ///< grouped-with-you chars
        ENTITY_GUILD,           ///< grouped-with-you-and-the-same-guild chars
        ENTITY_TYPES_AMOUNT     ///< amount of entity types
    };

/**
 * class psEntityLabels serves for creation and management of 2D sprites hanging above cel-entities 
 * that display entity names and possibly guilds
 */
class psEntityLabels
{
public:
    psEntityLabels();
    virtual ~psEntityLabels();
    
    /** 
     * This is called before some entity is deleted 
     */
    virtual void RemoveObject( GEMClientObject* object );
    
    // from iEventHandle:
    
    /** 
     * This is called before the scene is painted on screen:
     */
    bool HandleEvent (iEvent &Event);
    

    bool Initialize(iObjectRegistry * object_reg, psCelClient * celClient);
    
    /**
     * Sets options for label behaviour:
     */
    void Configure(psEntityLabelVisib visCreatures, psEntityLabelVisib visItems, bool showGuild, int* colors);
    
    /**
     * Gets options for label behaviour:
     */
    void GetConfiguration(psEntityLabelVisib & visCreatures, psEntityLabelVisib & visItems, bool & showGuild, int* colors);
    
    /**
     * Reads options for label behaviour from file:
     */
    bool LoadFromFile();
    
    /**
     * Saves options for label behaviour to file:
     */
    bool SaveToFile();
    
    /**
     * This must be called when client receives cel entity from server:
     */
    void OnObjectArrived( GEMClientObject* object );
    
    /**
     * Used to repaint labels
     */
    void RepaintAllLabels();
    void RepaintObjectLabel(GEMClientObject* object);
    
    /// (re)loads all entity labels
    void LoadAllEntityLabels();

protected:
    
    /// Updates label visibility based on range
    void UpdateVisibility();
    
    /// Updates label visibility for entities under the cursor
    void UpdateMouseover();

    /// Updates label visibility for targeted entities
    void UpdateTarget();
    
    /**
     * This describes one row of text displayed on entity label
     */
    typedef struct
    {
        csString text;
        int x, y;
        int width, height;
        int r,g,b; //addes by jacob to complete task
    } labelRow;
    
    /**
     * Determines what will be written in label
     */
    void SetObjectText( GEMClientObject* object);

    /**
     * Creates label and its texture
     */
    void CreateLabelOfObject( GEMClientObject* object );

    /**
     * Shows or hides label of given entity (only sets visibility, doesn't create/destruct it)
     * This is used when visibility mode is OnMouseOver - labels are created for all entities,
     * but only one is set visible (or none at all).
     */
    void ShowLabelOfObject( GEMClientObject* object, bool show );
    
    /// Hides all labels
    void HideAllLabels();
    
    /// Refreshes just the actors with guild names (used on showGuild change)
    void RefreshGuildLabels();
    
    /**
     * Deletes label from entity
     */    
    void DeleteLabelOfObject( GEMClientObject* object );
     
    
    bool MatchVisibility(GEMOBJECT_TYPE type, psEntityLabelVisib vis);
    /**
     * Configuration options
     */
    psEntityLabelVisib visCreatures;
    psEntityLabelVisib visItems;
    bool showGuild;                    // should the label contain name of character's guild ?

    ///Entity labels colors
    int entityColors[ENTITY_TYPES_AMOUNT];

    /**
     * Entity that is under mouse cursor (or NULL)
     */    
    GEMClientObject * underMouse;

    /**
     * Entity which was targeted (or NULL)
     */
    GEMClientObject * underTarget;

    /**
     * References to some system-wide objects that we use
     */
    csRef<iEventQueue> eventQueue;
    iVFS* vfs;
    psCelClient* celClient;

    /// Declare our event handler
    DeclareGenericEventHandler(EventHandler,psEntityLabels,"planeshift.entityevent");
    csRef<EventHandler> eventhandler;
};

#endif
