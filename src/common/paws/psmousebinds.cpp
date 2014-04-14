/*
 * psmousebinds.cpp - Author: Andrew Robberts
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
#include <iutil/vfs.h>
#include <iutil/document.h>
#include <iutil/evdefs.h>
#include <csutil/xmltiny.h>
#include <csutil/inputdef.h>
#include <iengine/mesh.h>
#include <csutil/databuf.h>

// COMMON INCLUDES
#include "util/log.h"

// PS INCLUDES
#include "psmousebinds.h"

#define  CTRL_STR   "Ctrl+"
#define  ALT_STR    "Alt+"
#define  SHIFT_STR  "Shift+"
#define  LEFTCLICK_STR    "LeftClick"
#define  RIGHTCLICK_STR   "RightClick"
#define  MIDDLECLICK_STR  "MiddleClick"
#define  SCROLLUP_STR     "ScrollUp"
#define  SCROLLDOWN_STR   "ScrollDown"
#define  SCROLLLEFT_STR     "ScrollLeft"
#define  SCROLLRIGHT_STR   "ScrollRight"
#define  EXTRA1_STR       "Extra1"
#define  EXTRA2_STR       "Extra2"
#define  NONE_STR         "None"


csString psMouseBinds::MouseButtonToString(uint button, uint32 modifiers)
{
    csString str;

    if(modifiers & CSMASK_CTRL)
        str += CTRL_STR;

    if(modifiers & CSMASK_ALT)
        str += ALT_STR;

    if(modifiers & CSMASK_SHIFT)
        str += SHIFT_STR;

    switch(button)
    {
        case csmbLeft:
            str += LEFTCLICK_STR;
            break;
        case csmbRight:
            str += RIGHTCLICK_STR;
            break;
        case csmbMiddle:
            str += MIDDLECLICK_STR;
            break;
        case csmbWheelUp:
            str += SCROLLUP_STR;
            break;
        case csmbWheelDown:
            str += SCROLLDOWN_STR;
            break;
        case csmbHWheelLeft:
            str += SCROLLLEFT_STR;
            break;
        case csmbHWheelRight:
            str += SCROLLRIGHT_STR;
            break;
        case csmbExtra1:
            str += EXTRA1_STR;
            break;
        case csmbExtra2:
            str += EXTRA2_STR;
            break;
        case csmbNone:
        default:
            str = NONE_STR;
            break;
    }

    return str;
}

bool psMouseBinds::StringToMouseButton(const csString &str, uint &button, uint32 &modifiers)
{
    size_t pos = 0;
    modifiers = 0;

    if(str.StartsWith(CTRL_STR))
    {
        modifiers |= CSMASK_CTRL;
        pos += strlen(CTRL_STR);
    }

    if(str.Slice(pos).StartsWith(ALT_STR))
    {
        modifiers |= CSMASK_ALT;
        pos += strlen(ALT_STR);
    }

    if(str.Slice(pos).StartsWith(SHIFT_STR))
    {
        modifiers |= CSMASK_SHIFT;
        pos += strlen(SHIFT_STR);
    }

    csString btn = str.Slice(pos);
    if(btn == LEFTCLICK_STR)
        button = csmbLeft;
    else if(btn == RIGHTCLICK_STR)
        button = csmbRight;
    else if(btn == MIDDLECLICK_STR)
        button = csmbMiddle;
    else if(btn == SCROLLUP_STR)
        button = csmbWheelUp;
    else if(btn == SCROLLDOWN_STR)
        button = csmbWheelDown;
    else if(btn == SCROLLLEFT_STR)
        button = csmbHWheelLeft;
    else if(btn == SCROLLRIGHT_STR)
        button = csmbHWheelRight;
    else if(btn == EXTRA1_STR)
        button = csmbExtra1;
    else if(btn == EXTRA2_STR)
        button = csmbExtra2;
    else
    {
        button = (uint)csmbNone;
        return false;
    }

    return true;
}


csString OnOffToString(const bool onOff)
{
    return (onOff ? csString("on") : csString("off"));
}


bool psMouseBinds::LoadFromFile(iObjectRegistry* object_reg, const csString &filename)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, mouseNode, actionNode;
    csRef<iDocumentNodeIterator> xmlbinds;
    csString action, button, option, value;
    csString modifier("0");

    UnbindAll();

    csRef<iVFS> vfs;
    csRef<iDocumentSystem>  xml;
    const char* error;

    vfs =  csQueryRegistry<iVFS > (object_reg);
    assert(vfs);
    csRef<iDataBuffer> buff = vfs->ReadFile(filename);
    if(buff == NULL)
    {
        Error2("Could not find file: %s", filename.GetData());
        return false;
    }
    xml.AttachNew(new csTinyDocumentSystem);
    assert(xml);
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse(buff);
    if(error)
    {
        Error3("Parse error in %s: %s", filename.GetData(), error);
        return false;
    }

    if(doc == NULL) return false;

    root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("No root in XML");
        return false;
    }
    mouseNode = root->GetNode("mouse");
    if(mouseNode == NULL)
    {
        Error1("No <mouse> tag in XML");
        return false;
    }

    xmlbinds = mouseNode->GetNodes("Actions");
    while(xmlbinds->HasNext())
    {
        actionNode = xmlbinds->Next();

        action = actionNode->GetAttributeValue("action");
        button = actionNode->GetAttributeValue("button");
        modifier   = actionNode->GetAttributeValue("modifier");
        Bind(action, button, modifier);
    }

    xmlbinds = mouseNode->GetNodes("OnOff");
    while(xmlbinds->HasNext())
    {
        actionNode = xmlbinds->Next();

        option = actionNode->GetAttributeValue("option");
        value = actionNode->GetAttributeValue("value");
        SetOnOff(option, value);
    }

    xmlbinds = mouseNode->GetNodes("Int");
    while(xmlbinds->HasNext())
    {
        actionNode = xmlbinds->Next();

        option = actionNode->GetAttributeValue("option");
        value = actionNode->GetAttributeValue("value");
        SetInt(option, value);
    }


    int x, y;
    if(!GetInt("VertSensitivity", y))
        y = 10;

    if(!GetInt("HorzSensitivity", x))
        x = 10;

    return true;
}


bool psMouseBinds::SaveToFile(iObjectRegistry* object_reg, const csString &filename)
{
    csString xml;

    xml += "<mouse>\n";
    size_t x;
    for(x = 0; x < binds.GetSize(); x++)
    {
        xml += "    <Actions action=\"";
        xml += binds[x]->action;
        xml += "\" button=\"";
        xml += binds[x]->event.Button;
        xml += "\" modifier=\"";
        xml += binds[x]->event.Modifiers;
        xml += "\"/>\n";
    }
    for(x = 0; x < boolOptions.GetSize(); x++)
    {
        xml += "    <OnOff   option=\"";
        xml += boolOptions[x]->option;
        xml += "\" value=\"";
        xml += boolOptions[x]->value;
        xml += "\"/>\n";
    }
    for(x = 0; x < intOptions.GetSize(); x++)
    {
        xml += "    <Int     option=\"";
        xml += intOptions[x]->option;
        xml += "\" value=\"";
        xml += intOptions[x]->value;
        xml += "\"/>\n";
    }
    xml += "</mouse>\n";


    csRef<iVFS> vfs =  csQueryRegistry<iVFS > (object_reg);
    assert(vfs);
    return vfs->WriteFile(filename, xml.GetData(), xml.Length());
}


void psMouseBinds::Bind(const csString &action, csMouseEventData &event)
{
    psMouseBind* iter;

    iter = FindAction(action);
    if(iter)
    {
        iter->event = event;
    }
    else
    {
        psMouseBind* bind = new psMouseBind;
        bind->action = action;
        bind->event = event;
        binds.Push(bind);
    }
}


void psMouseBinds::Bind(const csString &action, int button, int modifier)
{
    csMouseEventData mouse;
    mouse.Button = button;
    mouse.Modifiers = modifier;
    mouse.x = 0;
    mouse.y = 0;
    mouse.numAxes = 0;

    Bind(action, mouse);
}


void psMouseBinds::Bind(const csString &action, csString &button, csString &modifier)
{
    csMouseEventData mouse;

    mouse.Button = atoi(button.GetData());
    mouse.Modifiers = atoi(modifier.GetData());
    mouse.x = 0;
    mouse.y = 0;
    mouse.numAxes = 0;

    Bind(action, mouse);
}


void psMouseBinds::SetOnOff(const csString &option, csString &value)
{
    SetOnOff(option, (atoi(value.GetData()) != 0));
}


void psMouseBinds::SetOnOff(const csString &option, bool value)
{
    psMouseOnOff* iter;

    iter = FindOnOff(option);

    if(iter)
    {
        iter->value = value;
    }
    else
    {
        psMouseOnOff* onOff = new psMouseOnOff;
        onOff->option = option;
        onOff->value = value;
        boolOptions.Push(onOff);
    }
}


void psMouseBinds::SetInt(const csString &option, csString &value)
{
    SetInt(option, atoi(value.GetData()));
}


void psMouseBinds::SetInt(const csString &option, int value)
{
    psMouseInt* iter;
    iter = FindInt(option);
    if(iter)
    {
        iter->value = value;
    }
    else
    {
        psMouseInt* intOp = new psMouseInt;
        intOp->option = option;
        intOp->value = value;
        intOptions.Push(intOp);
    }
}


bool psMouseBinds::GetBind(const csString &action, csMouseEventData &event)
{
    psMouseBind* iter;

    iter = FindAction(action);
    if(iter)
    {
        event = iter->event;
        return true;
    }
    else
        return false;
}


bool psMouseBinds::GetBind(const csString &action, csString &event)
{
    psMouseBind* iter;

    iter = FindAction(action);
    if(iter)
    {
        event = MouseButtonToString(iter->event.Button, iter->event.Modifiers);
        return true;
    }
    else
        return false;
}

bool psMouseBinds::CheckBind(const csString &action, int button, int modifiers)
{
    csMouseEventData event;
    if(GetBind(action, event))
    {
#ifdef CS_PLATFORM_MACOSX
        if(event.Button == csmbRight && event.Modifiers == 0 && (button == csmbLeft && modifiers == CSMASK_CTRL))
            return true;
#endif
        return ((uint)button == event.Button && (uint)modifiers == event.Modifiers);
    }

    return false;
}

bool psMouseBinds::GetOnOff(const csString &option, csString &value)
{
    psMouseOnOff* iter;

    iter = FindOnOff(option);
    if(iter)
    {
        value = OnOffToString(iter->value);
        return true;
    }
    else
        return false;
}


bool psMouseBinds::GetOnOff(const csString &option, bool &value)
{
    psMouseOnOff* iter;

    iter = FindOnOff(option);
    if(iter)
    {
        value = iter->value;
        return true;
    }
    else
        return false;
}


bool psMouseBinds::GetInt(const csString &option, csString &value)
{
    psMouseInt* iter;

    iter = FindInt(option);
    if(iter)
    {
        value.Clear();
        value += iter->value;
        return true;
    }
    else
        return false;
}


bool psMouseBinds::GetInt(const csString &option, int &value)
{
    psMouseInt* iter;

    iter = FindInt(option);
    if(iter)
    {
        value = iter->value;
        return true;
    }
    else
        return false;
}


void psMouseBinds::Unbind(const csString &action)
{
    psMouseBind* iter;

    iter = FindAction(action);
    if(iter)
        binds.Delete(iter);
}


void psMouseBinds::RemoveOnOff(const csString &option)
{
    psMouseOnOff* iter;
    iter = FindOnOff(option);
    if(iter)
        boolOptions.Delete(iter);
}


void psMouseBinds::RemoveInt(const csString &option)
{
    psMouseInt* iter;
    iter = FindInt(option);
    if(iter)
        intOptions.Delete(iter);
}


void psMouseBinds::UnbindAll()
{
    binds.DeleteAll();
    boolOptions.DeleteAll();
    intOptions.DeleteAll();
}


psMouseBind* psMouseBinds::FindAction(const csString &action)
{
    for(size_t x = 0; x < binds.GetSize(); x++)
    {
        if(binds[x]->action == action)
            return binds[x];
    }

    return 0;
}


psMouseOnOff* psMouseBinds::FindOnOff(const csString &option)
{
    for(size_t x = 0; x < boolOptions.GetSize(); x++)
    {
        if(boolOptions[x]->option == option)
            return boolOptions[x];
    }

    return 0;
}


psMouseInt* psMouseBinds::FindInt(const csString &option)
{
    for(size_t x = 0; x < intOptions.GetSize(); x++)
    {
        if(intOptions[x]->option == option)
            return intOptions[x];
    }

    return 0;
}
