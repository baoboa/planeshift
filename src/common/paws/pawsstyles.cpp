/*
 * pawsstyles.cpp - Author: Ondrej Hurt
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 * http://www.atomicblue.org)
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
#include <csutil/databuf.h>
#include "pawsstyles.h"
#include "util/log.h"
#include "util/psxmlparser.h"

pawsStyles::pawsStyles(iObjectRegistry* objectReg)
{
    this->objectReg = objectReg;
}

bool pawsStyles::LoadStyles(const csString &fileName)
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > (objectReg);
    assert(vfs);
    csRef<iDataBuffer> buff = vfs->ReadFile(fileName);
    if(buff == NULL)
    {
        Error2("Could not find file: %s", fileName.GetData());
        return false;
    }
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    assert(xml);
    csRef<iDocument> doc = xml->CreateDocument();
    assert(doc);
    const char* error = doc->Parse(buff);
    if(error)
    {
        Error3("Parse error in %s: %s", fileName.GetData(), error);
        return false;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(root == NULL) return false;
    csRef<iDocumentNode> topNode = root->GetNode("styles");
    if(topNode == NULL)
    {
        Error2("Missing <styles> tag in %s", fileName.GetData());
        return false;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes("style");
    while(iter->HasNext())
    {
        csRef<iDocumentNode> styleNode = iter->Next();
        csString name = styleNode->GetAttributeValue("name");
        if(styles.In(csString(name)))
        {
            // This is not an error anymore.  Custom skins can and should supercede standard styles.
            //Error2("Warning: PAWS style '%s' defined more than once", name.GetData());
        }
        else
            styles.Put(csString(name), styleNode);
    }

    InheritStyles();
    return true;
}

void pawsStyles::InheritStyles()
{
    STRING_HASH(bool) alreadyInh, beingInh;

    STRING_HASH(csRef<iDocumentNode>)::GlobalIterator iter = styles.GetIterator();
    while(iter.HasNext())
    {
        csRef<iDocumentNode> style = iter.Next();
        beingInh.DeleteAll(); // this needs to be emptied for each recursive run
        InheritFromParent(style, alreadyInh, beingInh);
    }
}

void pawsStyles::InheritFromParent(iDocumentNode* style, STRING_HASH(bool) & alreadyInh, STRING_HASH(bool) & beingInh)
{
    csString name = style->GetAttributeValue("name");
    csString inherit = style->GetAttributeValue("inherit");

    if(inherit.IsEmpty())   // nothing to inherit from
        return;

    if(beingInh.In(csString(name)))
    {
        Error1("Error: cyclic inheritance in PAWS styles !");
        return;
    }

    beingInh.Put(csString(name), true);
    csRef <iDocumentNode> parent = styles.Get(csString(inherit), NULL);
    if(parent != NULL)
    {
        if(!alreadyInh.In(csString(inherit)))
            InheritFromParent(parent, alreadyInh, beingInh);
        ApplyStyle(parent, style);
    }
    else
        Error2("Error: PAWS style %s not found !", inherit.GetData());
    alreadyInh.Put(csString(name), true);
}

void pawsStyles::ApplyStyle(const char* style, iDocumentNode* target)
{
    if(!style)
        return;

    iDocumentNode* styleXML = styles.Get(style, NULL);
    if(styleXML != NULL)
        ApplyStyle(styleXML, target);
    else
        Error2("Error: PAWS style %s not found", style);
}

void pawsStyles::ApplyStyle(iDocumentNode* style, iDocumentNode* target)
{
    CopyXMLNode(style, target, 3);
}

