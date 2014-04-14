/*
 * pawsconfigentitylabels.cpp - Author: Ondrej Hurt
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
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>

// COMMON INCLUDES
#include "util/log.h"
#include "globals.h"

// CLIENT INCLUDES
#include "entitylabels.h"

// PAWS INCLUDES
#include "pawsconfigentitylabels.h"
#include "paws/pawsmanager.h"
#include "paws/pawscombo.h"
#include "paws/pawscheckbox.h"
#include "paws/pawsradio.h"
bool pawsConfigEntityLabels::Initialize()
{
    SetName("ConfigEntityLabels");

    tree = NULL;
    colorPicker = NULL;

    defLabelColors[ENTITY_DEFAULT] = 0xff0000;
    defLabelColors[ENTITY_PLAYER] = 0x00ff00;
    defLabelColors[ENTITY_DEV] = 0xff8080;
    defLabelColors[ENTITY_TESTER] = 0x338ca7;
    defLabelColors[ENTITY_DEAD] = 0xff0000;
    defLabelColors[ENTITY_GM1] = 0x008000;
    defLabelColors[ENTITY_GM25] = 0xffff80;
    defLabelColors[ENTITY_NPC] = 0x00ffff;
    defLabelColors[ENTITY_GROUP] = 0x0080ff;
    defLabelColors[ENTITY_GUILD] = 0xf6dfa6;

    labelColors[ENTITY_DEFAULT] = defLabelColors[ENTITY_DEFAULT];
    labelColors[ENTITY_PLAYER] = defLabelColors[ENTITY_PLAYER];
    labelColors[ENTITY_DEV] = defLabelColors[ENTITY_DEV];
    labelColors[ENTITY_TESTER] = defLabelColors[ENTITY_TESTER];
    labelColors[ENTITY_DEAD] = defLabelColors[ENTITY_DEAD];
    labelColors[ENTITY_GM1] = defLabelColors[ENTITY_GM1];
    labelColors[ENTITY_GM25] = defLabelColors[ENTITY_GM25];
    labelColors[ENTITY_NPC] = defLabelColors[ENTITY_NPC];
    labelColors[ENTITY_GROUP] = defLabelColors[ENTITY_GROUP];
    labelColors[ENTITY_GUILD] = defLabelColors[ENTITY_GUILD];

    if ( !CreateTree() )
        return false;

    UseBorder("line");

    csRef<psCelClient> celclient = psengine->GetCelClient();
    assert(celclient);
    
    entityLabels = celclient->GetEntityLabels();
    assert(entityLabels);
    return true;
}
pawsConfigEntityLabels::~pawsConfigEntityLabels()
{
}
bool pawsConfigEntityLabels::CreateTree()
{
    CS_ASSERT(tree == NULL);
    tree = new pawsTree();

    if ( !tree->LoadFromFile("configentitylabels.xml") )
        return false;

    tree->MoveTo(screenFrame.xmin, screenFrame.ymin);
    tree->SetSize(screenFrame.Width(), screenFrame.Height());
    tree->SetScrollBars(false, true);
    tree->SetTreeLayout(new pawsStdTreeLayout(tree, 5, 20));
    //tree->SetTreeDecorator(new pawsStdTreeDecorator(tree, graphics2D, 0xffffffff, 0xffffff, 13));
    AddChild(tree);

    CreatureRBG = dynamic_cast<pawsRadioButtonGroup*>(FindWidget("CreaturesRG"));
    if (CreatureRBG == NULL)
        return false;
    ItemRBG = dynamic_cast<pawsRadioButtonGroup*>(FindWidget("ItemsRG"));
    if (ItemRBG == NULL)
        return false;
    visGuildCheck = dynamic_cast<pawsCheckBox*>(FindWidget("visGuild"));
    if (visGuildCheck == NULL)
        return false;

    return true;
}

bool pawsConfigEntityLabels::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget )
{
    int widID = widget->GetID(); //widget identificator
    
    // IDs above ENTITY_TYPES_AMOUNT indicate reset to default color.
    if ((widID >= ENTITY_TYPES_AMOUNT))
    {
        widID -= ENTITY_TYPES_AMOUNT;
        if((widID >= 0) && (widID < ENTITY_TYPES_AMOUNT))
        {
            labelColors[widID] = defLabelColors[widID];
            dirty = true;
        }
    }
    else
    {
        if ((widID >= 0) && (widID < ENTITY_TYPES_AMOUNT))
        {
            colorPicker = colorPicker->Create("Choose color",labelColors[widID],0,0xff,this,"colorPick",widID);
        }
    }
    
    return true;
}
void pawsConfigEntityLabels::OnColorEntered(const char* /*name*/, int param, int color)
{
    if (color != labelColors[param])    //param store button ID
    {
        labelColors[param] = color;
        dirty = true;
    }
}
bool pawsConfigEntityLabels::LoadConfig()
{
    psEntityLabelVisib visCreatures;
    psEntityLabelVisib visItems;
    bool showGuild;

    entityLabels->GetConfiguration(visCreatures, visItems, showGuild, labelColors);
    
    switch (visCreatures)
    {
        case LABEL_ALWAYS:
            CreatureRBG->SetActive("always");
            break;
        case LABEL_ONMOUSE:
            CreatureRBG->SetActive("mouse");
            break;
        case LABEL_ONTARGET:
            CreatureRBG->SetActive("target");
            break;
        default:
            CreatureRBG->SetActive("never");
    }
    switch (visItems)
    {
        case LABEL_ALWAYS:
            ItemRBG->SetActive("always");
            break;
        case LABEL_ONMOUSE:
            ItemRBG->SetActive("mouse");
            break;
        case LABEL_ONTARGET:
            ItemRBG->SetActive("target");
            break;
        default:
            ItemRBG->SetActive("never");
    }
    visGuildCheck->SetState(showGuild);

    dirty = false;
    return true;
}

bool pawsConfigEntityLabels::SaveConfig()
{
    psEntityLabelVisib visCreatures;
    psEntityLabelVisib visItems;
    bool showGuild;
    csString activeVisib;

    visCreatures = psEntityLabelVisib(CreatureRBG->GetActiveID()-100);
    visItems = psEntityLabelVisib(ItemRBG->GetActiveID()-103);

    showGuild = visGuildCheck->GetState();

    entityLabels->Configure(visCreatures, visItems, showGuild, labelColors);
    entityLabels->SaveToFile();
    entityLabels->RepaintAllLabels();
    dirty = false;
    return true;
}

void pawsConfigEntityLabels::SetDefault()
{
    if (!CreatureRBG->SetActive("mouse"))
    {
        Error1("No widget with such name");
        return;
    }
    if (!ItemRBG->SetActive("mouse"))
    {
        Error1("No widget with such name");
        return;
    }
    visGuildCheck->SetState(true);
    labelColors[ENTITY_DEFAULT] = defLabelColors[ENTITY_DEFAULT];
    labelColors[ENTITY_PLAYER] = defLabelColors[ENTITY_PLAYER];
    labelColors[ENTITY_DEV] = defLabelColors[ENTITY_DEV];
    labelColors[ENTITY_TESTER] = defLabelColors[ENTITY_TESTER];
    labelColors[ENTITY_DEAD] = defLabelColors[ENTITY_DEAD];
    labelColors[ENTITY_GM1] = defLabelColors[ENTITY_GM1];
    labelColors[ENTITY_GM25] = defLabelColors[ENTITY_GM25];
    labelColors[ENTITY_NPC] = defLabelColors[ENTITY_NPC];
    labelColors[ENTITY_GROUP] = defLabelColors[ENTITY_GROUP];
    labelColors[ENTITY_GUILD] = defLabelColors[ENTITY_GUILD];
    dirty = true;
}

bool pawsConfigEntityLabels::OnChange(pawsWidget * widget)
{
    if ((widget == CreatureRBG) || (widget == ItemRBG) || (widget == visGuildCheck))
        dirty = true;

    return true;
}
