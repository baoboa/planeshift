/*
* pawsimagedrawable.cpp
*
* Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/texture.h>
#include <iutil/databuff.h>
#include <iutil/object.h>
#include <igraphic/imageio.h>
#include <igraphic/image.h>

#include <ivideo/txtmgr.h>
#include <ivideo/graph2d.h>
#include <csgeom/math.h>


#include "util/log.h"

#include "pawstexturemanager.h"
#include "pawsimagedrawable.h"
#include "pawsmanager.h"

bool pawsImageDrawable::PreparePixmap()
{
    if(imageFileLocation.Length() == 0)  // tileable background
    {
        isLoaded = true;
        return true;
    }

    // Check if already loaded first.
    csRef<iEngine> engine = csQueryRegistry<iEngine>(PawsManager::GetSingleton().GetObjectRegistry());
    iTextureWrapper* tex = engine ? engine->GetTextureList()->FindByName(imageFileLocation) : nullptr;
    if(tex)
    {
        textureHandle = tex->GetTextureHandle();
    }
    else // Else load.
    {
        csRef<iVFS> vfs = csQueryRegistry<iVFS>(PawsManager::GetSingleton().GetObjectRegistry());
        csRef<iImageIO> imageLoader = csQueryRegistry<iImageIO>(PawsManager::GetSingleton().GetObjectRegistry());
        csRef<iTextureManager> textureManager = PawsManager::GetSingleton().GetGraphics3D()->GetTextureManager();

        int textureFormat = textureManager->GetTextureFormat();

        csRef<iImage> ifile;
        csRef<iDataBuffer> buf(vfs->ReadFile(imageFileLocation, false));

        if(!buf.IsValid())
        {
            Error2("Could not open image: >%s<", (const char*)imageFileLocation);
            return false;
        }

        ifile = imageLoader->Load(buf, textureFormat);


        if(!ifile)
        {
            Error2("Image >%s< could not be loaded by the iImageID",
                   (const char*)imageFileLocation);
            return false;
        }

        textureHandle = textureManager->RegisterTexture(ifile,
                        CS_TEXTURE_2D |
                        CS_TEXTURE_3D |
                        CS_TEXTURE_NOMIPMAPS |
                        CS_TEXTURE_CLAMP |
                        CS_TEXTURE_NPOTS);

        if(!textureHandle)
        {
            Error1("Failed to Register Texture");
            return false;
        }

        // Set texture class to 'cegui' defined in CS.
        textureHandle->SetTextureClass("cegui");

        // Store wrapped handle in the engine for later use.
        if(engine)
        {
            tex = engine->GetTextureList()->NewTexture(textureHandle);
            tex->QueryObject()->SetName(imageFileLocation);
        }

        // If colour key exists.
        if(defaultTransparentColourBlue  != -1 &&
                defaultTransparentColourGreen != -1 &&
                defaultTransparentColourRed   != -1)
        {
            textureHandle->SetKeyColor(defaultTransparentColourRed,
                                       defaultTransparentColourGreen,
                                       defaultTransparentColourBlue);
        }
    }

    // Get other texture data.
    textureHandle->GetOriginalDimensions(width, height);

    if(textureRectangle.Width() == 0 || textureRectangle.Height() == 0)
    {
        textureRectangle.xmax = width;
        textureRectangle.ymax = height;
    }

    //if we reach this point we are sure the image was loaded correctly. (note this doesn't mean image was assigned)
    isLoaded = true;
    return true;
}

pawsImageDrawable::pawsImageDrawable(iDocumentNode* node)
    : scfImplementationType(this)
{
    debugImageErrors = false;
    defaultTransparentColourBlue  = -1;
    defaultTransparentColourGreen = -1;
    defaultTransparentColourRed   = -1;
    isLoaded = false;

    defaultAlphaValue = 0;

    // Read off the image and file vars
    imageFileLocation = node->GetAttributeValue("file");
    resourceName = node->GetAttributeValue("resource");

    tiled = node->GetAttributeValueAsBool("tiled");

    csRef<iDocumentNodeIterator> iter = node->GetNodes();
    while(iter->HasNext())
    {
        csRef<iDocumentNode> childNode = iter->Next();

        // Read the texture rectangle for this image.
        if(strcmp(childNode->GetValue(), "texturerect") == 0)
        {
            textureRectangle.xmin = childNode->GetAttributeValueAsInt("x");
            textureRectangle.ymin = childNode->GetAttributeValueAsInt("y");

            int width = childNode->GetAttributeValueAsInt("width");
            int height = childNode->GetAttributeValueAsInt("height");

            textureRectangle.SetSize(width, height);
        }

        // Read the default alpha value.
        if(strcmp(childNode->GetValue(), "alpha") == 0)
        {
            defaultAlphaValue = childNode->GetAttributeValueAsInt("level");
        }

        // Read the default transparent colour.
        if(strcmp(childNode->GetValue(), "trans") == 0)
        {
            defaultTransparentColourRed   = childNode->GetAttributeValueAsInt("r");
            defaultTransparentColourGreen = childNode->GetAttributeValueAsInt("g");
            defaultTransparentColourBlue  = childNode->GetAttributeValueAsInt("b");
        }
    }

    PreparePixmap();
}

pawsImageDrawable::pawsImageDrawable(const char* file, const char* resource, bool tiled, const csRect &textureRect, int alpha, int transR, int transG, int transB)
    : scfImplementationType(this)
{
    debugImageErrors = false;

    imageFileLocation = file;
    resourceName = resource;
    this->tiled = tiled;
    textureRectangle = textureRect;
    defaultAlphaValue = alpha;
    defaultTransparentColourRed   = transR;
    defaultTransparentColourGreen = transG;
    defaultTransparentColourBlue  = transB;
    isLoaded = false;
    PreparePixmap();
}

pawsImageDrawable::pawsImageDrawable(const char* file, const char* /*resource*/)
    : scfImplementationType(this)
{
    debugImageErrors = false;

    imageFileLocation = file;
    resourceName = file;
    this->tiled = false;
    defaultAlphaValue = 0;
    defaultTransparentColourRed   = -1;
    defaultTransparentColourGreen = -1;
    defaultTransparentColourBlue  = -1;
    isLoaded = false;
    PreparePixmap();
}

pawsImageDrawable::~pawsImageDrawable()
{
}

const char* pawsImageDrawable::GetName() const
{
    return resourceName;
}

void pawsImageDrawable::Draw(int x, int y, int alpha)
{
    if(!textureHandle)
        return;
    if(alpha < 0)
        alpha = defaultAlphaValue;
    int w = textureRectangle.Width();
    int h = textureRectangle.Height();
    PawsManager::GetSingleton().GetGraphics3D()->DrawPixmap(textureHandle, x, y, w, h, textureRectangle.xmin, textureRectangle.ymin, w, h, alpha);
}

void pawsImageDrawable::Draw(csRect rect, int alpha)
{
    Draw(rect.xmin, rect.ymin, rect.Width(), rect.Height(), alpha);
}

void pawsImageDrawable::Draw(int x, int y, int newWidth, int newHeight, int alpha)
{
    int w = textureRectangle.Width();
    int h = textureRectangle.Height();

    if(!textureHandle)
    {
        if(debugImageErrors)
            Error3("Image named >%s< (%s) was not loaded and could not be drawn.",resourceName.GetDataSafe(),imageFileLocation.GetDataSafe());
        return;
    }
    if(alpha < 0)
        alpha = defaultAlphaValue;
    if(newWidth == 0)
        newWidth = width;
    if(newHeight == 0)
        newHeight = height;

    if(!tiled)
        PawsManager::GetSingleton().GetGraphics3D()->DrawPixmap(textureHandle, x, y, newWidth, newHeight, textureRectangle.xmin, textureRectangle.ymin, w, h, alpha);
    else
    {
        int left = x;
        int top = y;
        int right = x + newWidth;
        int bottom = y + newHeight;
        for(x=left; x<right;  x+=w)
        {
            for(y=top; y<bottom; y+=h)
            {
                int dw = csMin<int>(w, right - x);
                int dh = csMin<int>(h, bottom - y);
                PawsManager::GetSingleton().GetGraphics3D()->DrawPixmap(textureHandle, x, y, dw, dh, textureRectangle.xmin, textureRectangle.ymin, dw, dh, alpha);
            }
        }
    }
}

int pawsImageDrawable::GetWidth() const
{
    return textureRectangle.Width();
}

int pawsImageDrawable::GetHeight() const
{
    return textureRectangle.Height();
}

int pawsImageDrawable::GetDefaultAlpha() const
{
    return defaultAlphaValue;
}

bool pawsImageDrawable::IsLoaded() const
{
    return isLoaded;
}

//TODO: this piece of code looks dead. It's not called within the class and it's missing from the interface.
iImage* pawsImageDrawable::GetImage()
{
    if(image)
        return image;

    csRef<iVFS> vfs = csQueryRegistry<iVFS>(PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iImageIO> imageLoader =  csQueryRegistry<iImageIO >(PawsManager::GetSingleton().GetObjectRegistry());

    csRef<iDataBuffer> buf(vfs->ReadFile(imageFileLocation, false));

    if(!buf || !buf->GetSize())
    {
        Error2("Could not open image: '%s'", imageFileLocation.GetData());
        return 0;
    }

    image = imageLoader->Load(buf, CS_IMGFMT_ANY | CS_IMGFMT_ALPHA);

    if(!image)
    {
        Error2("Could not load image: '%s'", imageFileLocation.GetData());
        return 0;
    }

    image->SetName(imageFileLocation);
    isLoaded = true;
    return image;
}

int pawsImageDrawable::GetTransparentRed() const
{
    return defaultTransparentColourRed;
}

int pawsImageDrawable::GetTransparentGreen() const
{
    return defaultTransparentColourGreen;
}

int pawsImageDrawable::GetTransparentBlue() const
{
    return defaultTransparentColourBlue;
}

