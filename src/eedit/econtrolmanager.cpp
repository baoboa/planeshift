/*
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

#include <csutil/objreg.h>

#include "econtrolmanager.h"
#include "eeditglobals.h"

#include "util/psxmlparser.h"

#include <csutil/ref.h>
#include <iutil/document.h>
#include <csutil/inputdef.h>
#include <csutil/event.h>
#include "util/psconst.h"

#define CMD_IMPLEMENTATION(func) void eControlManager::func(eControlManager *obj, bool down)

// Command function pointer.
typedef void keycmdfuncptr(eControlManager *, bool);

// Holds the map between a action's name and the function implementing it.
struct KeyCommand
{
    const char      *name;  // Action's name.
    keycmdfuncptr   *func;  // Function that implements the action.
};

KeyCommand commands[] = {
    {"forward1",        eControlManager::HandleForward},
    {"forward2",        eControlManager::HandleForward},
    {"backward1",       eControlManager::HandleBackward},
    {"backward2",       eControlManager::HandleBackward},
    {"rotateleft1",     eControlManager::HandleRotateLeft},
    {"rotateleft2",     eControlManager::HandleRotateLeft},
    {"rotateright1",    eControlManager::HandleRotateRight},
    {"rotateright2",    eControlManager::HandleRotateRight},
    {"lookup",          eControlManager::HandleLookUp},
    {"lookdown",        eControlManager::HandleLookDown},
    {0, 0}
};

eControlManager::eControlManager(iObjectRegistry* object_reg)
{
    this->object_reg = object_reg;
}

eControlManager::~eControlManager()
{
    ClearKeyMap();
}

bool eControlManager::HandleEvent(iEvent &event)
{
    if (CS_IS_KEYBOARD_EVENT(object_reg, event))
    {
        csKeyEventData data;
        csKeyEventHelper::GetEventData(&event, data);

        ExecuteKeyCommand(data);
        return true;
    }
    return false;
}

void eControlManager::ExecuteKeyCommand( csKeyEventData& key )
{
    // Figure out what action key this is for.
    size_t actionKey = 0;
    for ( actionKey = 0; actionKey < keyMap.GetSize(); actionKey++ )
    {      
        if (  keyMap[actionKey]->csKey.codeRaw == key.codeRaw )
            break;
            
        if (  keyMap[actionKey]->csKey.codeCooked == key.codeCooked )
            break;
            
    }
        
    if ( actionKey == keyMap.GetSize() )
        return;


    // Handle non-scripted keystrokes, like "run" or "jump"
    for (int i=0; commands[i].name; i++)
    {
        if (strcmp (keyMap[actionKey]->action, commands[i].name) == 0)
        {
            (*(commands[i].func))(this,key.eventType == csKeyEventTypeDown);
            return;
        }
    }
}

bool eControlManager::LoadKeyMap(const char*filename)
{
    csRef<iDocument> doc;    
    csRef<iDocumentNodeIterator> binds;
    csString key, value;

    doc = ParseFile(object_reg, filename);
    if (doc == NULL) return false;

    csRef<iDocumentNode> root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("No root in XML");
        return false;
    }
    
    csRef<iDocumentNode> keysNode = root->GetNode("keys");
    if (keysNode == NULL)
    {
        Error1("No <keys> tag in XML");
        return false;
    }
    
    csRef<iDocumentNode> keySetNode = keysNode->GetNode("KeySet");
    if (keySetNode == NULL)
    {
        Error1("No <KeySet> tag in XML");
        return false;
    }

    csRef<iEventNameRegistry> nr = csEventNameRegistry::GetRegistry (object_reg);
    binds = keySetNode->GetNodes("Keys");
    while (binds->HasNext())
    {        
        csRef<iDocumentNode> keyNode = binds->Next();

        csString key = keyNode->GetAttributeValue("key");
        csString action = keyNode->GetAttributeValue("value");
        
        csKeyEventData csKey;
        csInputDefinition::ParseKey (nr, key, &csKey.codeRaw, &csKey.codeCooked, &csKey.modifiers );        
        
        Map(action, csKey);
    }     
    return true;
}

void eControlManager::Map(const char *action, csKeyEventData &data)
{
    size_t actionInt = StringToAction( action );
    if ( actionInt != (size_t)-1)
    {        
        keyMap[actionInt]->csKey = data;
    }
    else
    {
        ActionKeyMap *newkey = new ActionKeyMap;
        newkey->action = action;
        newkey->csKey  = data;
        keyMap.Push(newkey);
    }
}

size_t eControlManager::StringToAction(const char *string)
{
    csString action( string );
    for (size_t id = 0;  id < keyMap.GetSize(); id++ )
    {
        if (keyMap[id]->action == action)
            return id;
    }
    return SIZET_NOT_FOUND;
}

void eControlManager::ClearKeyMap()
{
    for (size_t i = 0; i < keyMap.GetSize(); i++)
    {
        delete keyMap[i];
    }
    keyMap.Empty();
}


CMD_IMPLEMENTATION(HandleForward)
{
    editApp->SetCamFlag(CAM_FORWARD, down);
}

CMD_IMPLEMENTATION(HandleBackward)
{
    editApp->SetCamFlag(CAM_BACK, down);
}

CMD_IMPLEMENTATION(HandleRotateLeft)
{
    editApp->SetCamFlag(CAM_LEFT, down);
}

CMD_IMPLEMENTATION(HandleRotateRight)
{
    editApp->SetCamFlag(CAM_RIGHT, down);
}

CMD_IMPLEMENTATION(HandleLookUp)
{
    editApp->SetCamFlag(CAM_UP, down);
}

CMD_IMPLEMENTATION(HandleLookDown)
{
    editApp->SetCamFlag(CAM_DOWN, down);
}
