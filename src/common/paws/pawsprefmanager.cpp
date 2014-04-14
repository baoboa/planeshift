/*
 * pawsprefmanager.cpp - Author: Andrew Craig
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
// pawsprefmanager.cpp: implementation of the pawsPrefManager class.
//
//////////////////////////////////////////////////////////////////////

#include <psconfig.h>
#include "pawsprefmanager.h"
#include "pawsimagedrawable.h"
#include "pawsmanager.h"
#include "pawsborder.h"

#include <csutil/util.h>

#include <csutil/xmltiny.h>
#include <csutil/scfstr.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <iutil/document.h>
#include <ivideo/fontserv.h>
#include <ivideo/graph2d.h>
#include <iutil/databuff.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsPrefManager::pawsPrefManager()
{
    objectReg = PawsManager::GetSingleton().GetObjectRegistry();

    vfs =  csQueryRegistry<iVFS > (objectReg);
    xml.AttachNew(new csTinyDocumentSystem);
    graphics2D =  csQueryRegistry<iGraphics2D > (objectReg);

    defaultFont = defaultScaledFont = NULL;
}


pawsPrefManager::~pawsPrefManager()
{

}

bool pawsPrefManager::LoadPrefFile(const char* fileName)
{
    csRef<iDataBuffer> buf = vfs->ReadFile(fileName);

    if(!buf || !buf->GetSize())
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(buf);
    if(error)
        Error2("ERROR: %s", error);

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("Bad XML file %s", fileName);
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode("prefs");
    if(!topNode)
    {
        Error2("Missing <prefs> tag in %s", fileName);
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        // default font here
        if(strcmp(node->GetValue(), "font") == 0)
        {
            defaultFontName = node->GetAttributeValue("name");
            float size = node->GetAttributeValueAsFloat("size");
            defaultFont = graphics2D->GetFontServer()->LoadFont(defaultFontName, (size)?size:(float)DEFAULT_FONT_SIZE);

            size *= PawsManager::GetSingleton().GetFontFactor();

            defaultScaledFont = graphics2D->GetFontServer()->LoadFont(defaultFontName, (size)?size:(float)DEFAULT_FONT_SIZE);
            if(!defaultFont)
            {
                Error2("Could not load font: >%s<", (const char*)defaultFontName);
                return false;
            }

            int r = node->GetAttributeValueAsInt("r");
            int g = node->GetAttributeValueAsInt("g");
            int b = node->GetAttributeValueAsInt("b");

            defaultFontColour = graphics2D->FindRGB(r, g, b);
        }

        if(strcmp(node->GetValue(), "border") == 0)
        {
            LoadBorderColours(node);
        }
    }

    return true;
}


bool pawsPrefManager::LoadBorderFile(const char* file)
{
    // A list of the XML tags for borders.
    static const char* borderTags[] =
    {
        "topleft",
        "topright",
        "bottomleft",
        "bottomright",
        "leftmiddle",
        "rightmiddle",
        "topmiddle",
        "bottommiddle",
        0
    };


    csRef<iDataBuffer> buf = vfs->ReadFile(file);

    if(!buf || !buf->GetSize())
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(buf);
    if(error)
        Error2("ERROR: %s\n", error);
    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("XML problem in %s", file);
        return false;
    }

    csRef<iDocumentNode> topNode = root->GetNode("borderlist");
    if(!topNode)
    {
        Error2("Missing <borderlist> tag in %s", file);
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        BorderDefinition* definition = new BorderDefinition;
        definition->name = node->GetAttributeValue("name");
        int current = 0;
        // For each border tag ( in the static above ) get that node tag
        // and construct an image description from it.
        while(borderTags[current] != 0)
        {
            csRef<iDocumentNode> tempnode = node->GetNode(borderTags[current]);
            if(!tempnode)
            {
                Error2("XML problem in %s", file);
                delete definition;
                return false;
            }
            csRef<iDocumentNode> image = tempnode->GetNode("image");
            if(!image)
            {
                Error2("Missing <image> tag in %s", file);
                delete definition;
                return false;
            }
            csRef<iPawsImage> drawable;
            drawable.AttachNew(new pawsImageDrawable(image));
            PawsManager::GetSingleton().GetTextureManager()->AddPawsImage(drawable);

            definition->descriptions[current] = image->GetAttributeValue("resource");

            current++;
        }

        borders.Push(definition);
    }
    return true;
}


BorderDefinition* pawsPrefManager::GetBorderDefinition(const char* name)
{
    for(size_t x = 0; x < borders.GetSize(); x++)
    {
        if(borders[x]->name == name)
            return borders[x];
    }

    return 0;
}


void pawsPrefManager::LoadBorderColours(iDocumentNode* node)
{
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    int index = 0;
    while(iter->HasNext())
    {
        csRef<iDocumentNode> colour = iter->Next();

        int r = colour->GetAttributeValueAsInt("r");
        int g = colour->GetAttributeValueAsInt("g");
        int b = colour->GetAttributeValueAsInt("b");

        int col = graphics2D->FindRGB(r, g, b);
        borderColours[index++] = col;
    }
}

