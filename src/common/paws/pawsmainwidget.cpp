/*
 * pawsmainwidget.cpp - Author: Andrew Craig
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
// pawsmainwidget.cpp: implementation of the pawsMainWidget class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>

#include <iutil/vfs.h>
#include <iutil/document.h>
#include <iutil/evdefs.h>
#include <iutil/databuff.h>
#include <csutil/xmltiny.h>
#include <csutil/inputdef.h>
#include <csutil/event.h>
#include <iengine/mesh.h>
#include <iengine/sector.h>

#include "pawsmainwidget.h"

#include "pawsmanager.h"
#include "util/log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsMainWidget::pawsMainWidget()
{
    screenFrame = csRect(0,0,graphics2D->GetWidth(),graphics2D->GetHeight());
    if(!LoadGUIKeys("/this/data/guikeys.xml"))
        Error1("Failed to load GUI keys");

    clipRect = csRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
    alpha = -1;

    needsRender = true;
}

pawsMainWidget::~pawsMainWidget()
{
}


bool pawsMainWidget::OnKeyDown(utf32_char keyCode, utf32_char /*key*/, int modifiers)
{
    pawsScriptKey* found = 0;

    for(size_t x = 0; x < keys.GetSize(); x++)
    {
        if(keys[x]->key.codeRaw == (utf32_char)keyCode &&
                csKeyEventHelper::GetModifiersBits(keys[x]->key.modifiers) == (uint32)modifiers)
        {
            found = keys[x];
            break;
        }
    }


    if(found)
    {
        pawsWidget* widget = FindWidget(found->widgetName);
        if(widget)
        {
            widget->PerformAction(found->action);
            return true;
        }
    }

    return false;
}

bool pawsMainWidget::OnMouseDown(int /*button*/, int /*keyModifier*/, int /*x*/, int /*y*/)
{
    return false;
}

bool pawsMainWidget::OnMouseUp(int /*button*/, int /*keyModifier*/, int /*x*/, int /*y*/)
{
    return false;
}

bool pawsMainWidget::OnDoubleClick(int /*button*/, int /*keyModifier*/, int /*x*/, int /*y*/)
{
    return false;
}


bool pawsMainWidget::LoadGUIKeys(const char* guiKeyFile)
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > (PawsManager::GetSingleton().GetObjectRegistry());

    csRef<iDataBuffer> buf = vfs->ReadFile(guiKeyFile);
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);

    if(!buf || !buf->GetSize())
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    if(!doc)
        return false;

    const char* error = doc->Parse(buf);

    if(error)
    {
        Error2("Error in XML: %s\n", error);
        return false;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root) return false;

    csRef<iDocumentNode> topNode = root->GetNode("keys");
    if(!topNode)
    {
        Error2("Missing <keys> tag in %s",guiKeyFile);
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();


    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        if(strcmp(node->GetValue(), "key") == 0)
        {

            pawsScriptKey* key = new pawsScriptKey;

            const char* keyvalue = node->GetAttributeValue("key");
            csInputDefinition::ParseKey(PawsManager::GetSingleton().GetEventNameRegistry(),
                                        keyvalue, &(key->key.codeRaw), &(key->key.codeCooked), &(key->key.modifiers));

            key->widgetName = node->GetAttributeValue("widgetname");
            key->action     = node->GetAttributeValue("action");

            keys.Push(key);
        }
    }

    return true;
}

void pawsMainWidget::ApplyWindowSettingsOnChildren(pawsWidget* caller, int alphaMin, int alphaMax, float fadeSpeed, bool fade, bool scaleFont)
{
    for(size_t z = 0; z < children.GetSize(); z++)
    {
        pawsWidget* wdg = children[z];
        if(wdg->IsConfigurable())
        {
            if(wdg != caller)
                wdg->DestroyWidgetConfigWindow();

            wdg->SetBackgroundAlpha(alphaMax);
            wdg->SetMinAlpha(alphaMin);
            wdg->SetFadeSpeed(fadeSpeed);
            wdg->SetFade(fade);
            wdg->SetFontScaling(scaleFont);
        }
    }
}
