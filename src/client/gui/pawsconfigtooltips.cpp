/*
* pawsconfigtooltips.cpp - Author: Neeno Pelero
*
* Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2 of the License)
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
*/

//==============================================================================
// CS INCLUDES
//==============================================================================
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>

//==============================================================================
// COMMON INCLUDES
//==============================================================================
#include "util/log.h"

//==============================================================================
// CLIENT INCLUDES
//==============================================================================
#include "../globals.h"

//==============================================================================
// PAWS INCLUDES
//==============================================================================
#include "pawsconfigtooltips.h"
#include "paws/pawscheckbox.h"


pawsConfigTooltips::pawsConfigTooltips(void)
{
}

bool pawsConfigTooltips::Initialize()
{
    colorPicker = NULL;

    if (!LoadFromFile("configtooltips.xml"))
        return false;

    if (!LoadDefaults())
        return false;

    return true;
}

bool pawsConfigTooltips::PostSetup()
{
    EnableTooltips = (pawsCheckBox*)FindWidget("EnableTooltips");
    if (!EnableTooltips) {
	Error1("Could not locate EnableTooltips widget!");
        return false;
    }

    EnableBgColor = (pawsCheckBox*)FindWidget("EnableBgColor");
    if (!EnableBgColor) {
	Error1("Could not locate EnableBgColor widget!");
        return false;
    }

    TooltipBgColor = (pawsTextBox*)FindWidget("BgColorFrame");
    if (!TooltipBgColor) {
	Error1("Could not locate BgColorFrame widget!");
        return false;
    }

    TooltipFontColor = (pawsTextBox*)FindWidget("FontColorFrame");
    if (!TooltipFontColor) {
	Error1("Could not locate FontColorFrame widget!");
        return false;
    }

    TooltipShadowColor = (pawsTextBox*)FindWidget("ShadowColorFrame");
    if (!TooltipShadowColor) {
	Error1("Could not locate ShadowColorFrame widget!");
        return false;
    }

    return true;
}

bool pawsConfigTooltips::LoadConfig()
{
    // load state from variables
    EnableTooltips->SetState(PawsManager::GetSingleton().getToolTipEnable());
    EnableBgColor->SetState(PawsManager::GetSingleton().getToolTipEnableBgColor());

    RefreshColorFrames();

    dirty = true;

    return true;
}

bool pawsConfigTooltips::SaveConfig()
{
    // save state to variables
    PawsManager::GetSingleton().setToolTipEnable(EnableTooltips->GetState());
    PawsManager::GetSingleton().setToolTipEnableBgColor(EnableBgColor->GetState());

    // save to file
    SaveSetting();

    return true;
}

void pawsConfigTooltips::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_TOOLTIPS_FILE_NAME);
    LoadDefaults();

    // transfer default values to standard variables
    PawsManager::GetSingleton().setToolTipEnable(defToolTipEnable);
    PawsManager::GetSingleton().setToolTipEnableBgColor(defToolTipEnableBgColor);
    PawsManager::GetSingleton().setTooltipsColors(0, defTooltipsColors[0]);
    PawsManager::GetSingleton().setTooltipsColors(1, defTooltipsColors[1]);
    PawsManager::GetSingleton().setTooltipsColors(2, defTooltipsColors[2]);

    LoadConfig();
}

void pawsConfigTooltips::SaveSetting()
{
    csRef<iFile> file;
    file = psengine->GetVFS()->Open(CONFIG_TOOLTIPS_FILE_NAME,VFS_FILE_WRITE);

    csRef<iDocumentSystem> docsys;
    docsys.AttachNew(new csTinyDocumentSystem ());

    csRef<iDocument> doc = docsys->CreateDocument();
    csRef<iDocumentNode> root, defaultRoot, TooltipsNode, showTooltipsNode, showBgColorNode, BgColorNode, FontColorNode, ShadowColorNode;

    root = doc->CreateRoot();

    TooltipsNode = root->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    TooltipsNode->SetValue("tooltips");

    showTooltipsNode = TooltipsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    showTooltipsNode->SetValue("enable_tooltips");
    showTooltipsNode->SetAttributeAsInt("value", EnableTooltips->GetState());

    showBgColorNode = TooltipsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    showBgColorNode->SetValue("enable_bgcolor");
    showBgColorNode->SetAttributeAsInt("value", EnableBgColor->GetState());

    BgColorNode = TooltipsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    BgColorNode->SetValue("bgcolor");
    BgColorNode->SetAttributeAsInt("value", PawsManager::GetSingleton().getTooltipsColors(0));

    FontColorNode = TooltipsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    FontColorNode->SetValue("fontcolor");
    FontColorNode->SetAttributeAsInt("value", PawsManager::GetSingleton().getTooltipsColors(1));

    ShadowColorNode = TooltipsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    ShadowColorNode->SetValue("shadowcolor");
    ShadowColorNode->SetAttributeAsInt("value", PawsManager::GetSingleton().getTooltipsColors(2));

    doc->Write(file);
}

bool pawsConfigTooltips::LoadDefaults()
{
    csString fileName;
    if( psengine->GetVFS()->Exists(PawsManager::GetSingleton().getToolTipSkinPath()) )
       fileName = PawsManager::GetSingleton().getToolTipSkinPath();   // skin.zip
    else fileName = CONFIG_TOOLTIPS_FILE_NAME_DEF;  // data/options
    csRef<iDocument> ToolTipdoc;
    csRef<iDocumentNode> ToolTiproot,TooltipsNode;
    csString TooltipOption;

    ToolTipdoc = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(), fileName);
    if (ToolTipdoc == NULL)
    {
        Error1("Failed to parse file");
        return false;
    }

    ToolTiproot = ToolTipdoc->GetRoot();
    if (ToolTiproot == NULL)
    {
        Error1("File has no XML root");
        return false;
    }

    TooltipsNode = ToolTiproot->GetNode("tooltips");
    if (TooltipsNode == NULL)
    {
        Error1("File has no <tooltips> tag");
        return false;
    }
    else
    {
        csRef<iDocumentNodeIterator> ToolTipoNodes = TooltipsNode->GetNodes();
        while(ToolTipoNodes->HasNext())
        {
            csRef<iDocumentNode> TooltipOption = ToolTipoNodes->Next();
            csString ToolTipnodeName (TooltipOption->GetValue());

            if (ToolTipnodeName == "enable_tooltips")
                defToolTipEnable = TooltipOption->GetAttributeValueAsBool("value");

            if (ToolTipnodeName == "enable_bgcolor")
                defToolTipEnableBgColor = TooltipOption->GetAttributeValueAsBool("value");

            if (ToolTipnodeName == "bgcolor")
                defTooltipsColors[0] = TooltipOption->GetAttributeValueAsInt("value");

            if (ToolTipnodeName == "fontcolor")
                defTooltipsColors[1] = TooltipOption->GetAttributeValueAsInt("value");

            if (ToolTipnodeName == "shadowcolor")
                defTooltipsColors[2] = TooltipOption->GetAttributeValueAsInt("value");
        }
    }
    return true;
}

bool pawsConfigTooltips::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget )
{
    int widID = widget->GetID(); //widget identificator

    if ((widID >= 3))
    {
        widID -= 3;
        if ((widID >= 0) && (widID < 3))
        {
            PawsManager::GetSingleton().setTooltipsColors(widID, defTooltipsColors[widID]);
            RefreshColorFrames();
            dirty = true;
        }
    }
    else
    {
        if ((widID >= 0) && (widID < 3))
        {
            colorPicker = colorPicker->Create("Choose color",PawsManager::GetSingleton().getTooltipsColors(widID),0,255,this,"colorPick",widID);
        }
    }

    return true;
}

void pawsConfigTooltips::OnColorEntered(const char* /*name*/, int param, int color)
{
    if (color != PawsManager::GetSingleton().getTooltipsColors(param))    //param store button ID
    {
        PawsManager::GetSingleton().setTooltipsColors(param, color);
        RefreshColorFrames();
        dirty = true;
    }
}

void pawsConfigTooltips::RefreshColorFrames()
{
    csRef<iGraphics2D> g2d = PawsManager::GetSingleton().GetGraphics2D();
    int red, green, blue;

    g2d->GetRGB(PawsManager::GetSingleton().getTooltipsColors(0), red, green, blue);
    TooltipBgColor->SetBackgroundColor(red, green, blue);

    g2d->GetRGB(PawsManager::GetSingleton().getTooltipsColors(1), red, green, blue);
    TooltipFontColor->SetBackgroundColor(red, green, blue);

    g2d->GetRGB(PawsManager::GetSingleton().getTooltipsColors(2), red, green, blue);
    TooltipShadowColor->SetBackgroundColor(red, green, blue);
}
