/*
 * texfactory.cpp by Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include "texfactory.h"

#include <iutil/vfs.h>          // iVFS
#include <igraphic/image.h>     // iImage
#include <imap/loader.h>        // iLoader interface
#include <csgfx/rgbpixel.h>     // csRGBPixel
#include <csgfx/imagememory.h>

psTextureFactory::psTextureFactory()
{
    // no init atm
}

psTextureFactory::~psTextureFactory()
{
    // delete all linklist items here
}

bool psTextureFactory::Initialize(iObjectRegistry* object_reg,const char *xmlfilename)
{
    psTextureFactory::object_reg = object_reg;

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (object_reg);
    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem> (object_reg);
    if (!xml)
        xml.AttachNew(new csTinyDocumentSystem);
    if (!vfs || !xml)
        return false;

    csRef<iFile> file = vfs->Open(xmlfilename, VFS_FILE_READ);

    if (!file)
    {
        printf("Cannot open %s.\n",xmlfilename);
        return false;
    }
    csRef<iDocument> doc = xml->CreateDocument();

    const char* error = doc->Parse(file);
    if (error)
    {
        printf("Error Loading Race Part Region Data: %s\n", error);
        return false;
    }

    printf ("Loading Race Part Regions...\n");
   
    csRef<iDocumentNodeIterator> raceIter = doc->GetRoot()->GetNode("RaceDefs")->GetNodes("Race");

    while (raceIter->HasNext())
    {
        csRef<iDocumentNode> raceNode = raceIter->Next();
        printf("Loading xml regions for race...\n");

        if (!LoadRace(raceNode))
        {
            printf("XML Err!\n");
            return false;
        }
    }

    return true;    
}

bool psTextureFactory::LoadRace(iDocumentNode * raceNode)
{
    csString race, part;

    race = raceNode->GetAttributeValue("name");        // race name is "name" attribute of race tag.

    // Now get regions for race.
    csRef<iDocumentNodeIterator> regionIter = raceNode->GetNodes("Region");

    while (regionIter->HasNext())
    {
        csRef<iDocumentNode> regionNode = regionIter->Next();
        part = regionNode->GetAttributeValue("name");

        int left,top,right,bottom;
        csRef<iDocumentAttribute> attr = regionNode->GetAttribute("left");
        
        if (attr)  // rectangle specified in region tag
        {
            left = attr->GetValueAsInt();
            top = regionNode->GetAttributeValueAsInt("top");
            right = regionNode->GetAttributeValueAsInt("right");
            bottom = regionNode->GetAttributeValueAsInt("bottom");

            psImageRegion *rgn = new psImageRegion(race,part);
            printf("Adding texfactory region %s/%s.\n",
                   (const char *)race,(const char *)part);

            if (rgn->CreateRectRegion(left,top,right,bottom))
                regions.Push(rgn);
            else
            {
                delete rgn;
                printf("Could not create region for %s, %s.\n",
                       (const char *)race,(const char *)part);
                return false;
            }
        }
        else    // region scanlines specified
        {
            psImageRegion *rgn = new psImageRegion(race,part);
            if (rgn->CreateRegion(regionNode))
                regions.Push(rgn);
            else
            {
                delete rgn;
                printf("Could not create region for %s, %s.\n",
                       (const char *)race,(const char *)part);
                return false;
            }
        }
    }
    return true;
}

csPtr<iImage> psTextureFactory::CreateTextureImage(const char *xmlspec)
{
    csRef<iDocumentNode> node;
    csRef<iDocumentSystem> xmlDoc =  csQueryRegistry<iDocumentSystem> (object_reg);
    if (!xmlDoc)
        xmlDoc.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> xml = xmlDoc->CreateDocument();
    xml->Parse(xmlspec);

    csString race,part,filename;
    
    // Get race, because it is part of the path to the texture.
    node = xml->GetRoot()->GetNode("texpartlist");
    if (node == NULL)
    {
        return NULL;
    }
    race = node->GetAttributeValue("race");
    filename = node->GetAttributeValue("base");

    race.Downcase();
 
    csRef<iImage> base = GetImage(race,filename);

    if (!base)
    {
        printf("Could not get base image in CreateTextureImage.\n");
        return NULL;
    }

    csRef<iImage> newimage;
    newimage.AttachNew (new csImageMemory (base));

    // Now cycle through all part tags and integrate each region into existing image.
    csRef<iDocumentNodeIterator> iter = xml->GetRoot()->GetNodes("texpart");
    while ( iter->HasNext() )
    {
        node = iter->Next();
    
        part = node->GetAttributeValue("name");
        filename = node->GetAttributeValue("texture");

        csRef<iImage> source = GetImage(race,filename);
        if (!source)
        {
            printf("Texture image file %s/%s not found. Cannot make custom tex.\n",
                   (const char *)race,
                   (const char *)filename);
            return NULL;
        }

        printf("Refcount of source image is %d\n",source->GetRefCount() );

        psImageRegion *rgn = GetRegion(race,part);
        if (rgn)
        {
            rgn->OverlayRegion(newimage,source);
        }
        else
        {
            printf("Texture region %s/%s not found. Cannot make custom tex.\n",
                   (const char *)race,
                   (const char *)part);
            return NULL;
        }

    }

    return csPtr<iImage> (newimage);
}

psImageRegion *psTextureFactory::GetRegion(const char *race,const char *part)
{
    for ( size_t x = 0; x < regions.GetSize(); x++ )
    {
        if (regions[x]->race == race && regions[x]->part == part)
            return regions[x]; 
    }
    
    return NULL;
}

csPtr<iImage> psTextureFactory::GetImage(const char *race,const char *filename)
{
    
    char name[100];
    sprintf(name,"/this/art/textures/races/%s/%s",race,filename);

    for ( size_t x = 0; x < imagecache.GetSize(); x++ )
    {
        if (!strcmp(imagecache[x]->GetName(),name))
        {
            csRef<iImage> image = imagecache[x];
            return csPtr<iImage> (image);
        }      
    }

    // if still not found here, attempt to load
    csRef<iLoader> loader =  csQueryRegistry<iLoader> (object_reg);
    if (!loader) return NULL; // or something else
    csRef<iImage> image = loader->LoadImage(name, CS_IMGFMT_TRUECOLOR);

    if (image && image->GetWidth()!=32) // Save in cache
    {
        imagecache.Push(image);
    }
    else
    {
        return NULL;
    }

    return csPtr<iImage> (image);
}

/*--------------------------------------------------------------------*/

psImageRegion::psImageRegion(const char *racename,const char *partname)
{
    race = racename;
    part = partname;
}

psImageRegion::~psImageRegion()
{
    // delete all linklist items here
}

bool psImageRegion::CreateRegion(iDocumentNode* node)
{
    csRef<iDocumentNodeIterator> lineIter = node->GetNodes("line");
    while (lineIter->HasNext())
    {
        csRef<iDocumentNode> lineNode = lineIter->Next();

        psScanline *New = new psScanline(lineNode->GetAttributeValueAsInt("x1"),lineNode->GetAttributeValueAsInt("x2"),lineNode->GetAttributeValueAsInt("y"));
        scanlines.Push(New);
    }
    return true;
}


psScanline* psImageRegion::GetLine( int x )
{
    return scanlines[x];
}

bool psImageRegion::CreateRectRegion(int left, int top, int right, int bottom)
{
    for (int i=top; i<=bottom; i++)
    {
        psScanline *New = new psScanline(left,right,i);
        scanlines.Push(New);
    }
    return true;
}

void psImageRegion::OverlayRegion(iImage *dest, iImage *src,int )
{
    csRGBpixel *destpix,*srcpix;

    if (dest->GetFormat() != src->GetFormat())
    {
        printf("Format of %s and %s are not the same!\n",dest->GetName(),src->GetName());
        return;
    }
    if (!dest->GetFormat() & CS_IMGFMT_TRUECOLOR || !src->GetFormat() & CS_IMGFMT_TRUECOLOR)
    {
        printf("Only TRUECOLOR image overlays are supported!\n");
        return;
    }

    destpix = (csRGBpixel *)dest->GetImageData();
    srcpix  = (csRGBpixel *)src->GetImageData();

    if (!destpix || !srcpix)
    {
        printf("OverlayRegion could not get pixels!\n");
        return;
    }

    int h=dest->GetHeight(),w=dest->GetWidth();

    if (h!=src->GetHeight() || w!=src->GetWidth())
    {
        printf("Images must be same size!\n");
        return;
    }

       
    for ( size_t x = 0; x < scanlines.GetSize(); x++ )
    {
        psScanline *sl = scanlines[x];

        int pixel = w * sl->y + sl->x1;
        
        for (int i=sl->x1; i<=sl->x2; i++,pixel++)
        {
            // eventually support blt modes and alpha blending here
            destpix[pixel] = srcpix[pixel];
        }
       
    }

    // successful overlay
}


