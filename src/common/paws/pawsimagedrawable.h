/*
* pawsimagedrawable.h
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

#ifndef PAWS_IMAGE_DRAWABLE
#define PAWS_IMAGE_DRAWABLE

#include "pawstexturemanager.h"

/**
 * \addtogroup common_paws
 * @{ */

class pawsImageDrawable : public scfImplementation1<pawsImageDrawable, iPawsImage>
{
private:
    /// The VFS path to the file.
    csString imageFileLocation;

    /// The resource name ( or identifier ) for this description.
    csString resourceName;

    csRef<iTextureHandle> textureHandle;
    csRect   textureRectangle;

    int width;
    int height;

    /// The default alpha value that should be used.
    int      defaultAlphaValue;

    /// The default transparent colour.
    int      defaultTransparentColourRed;
    int      defaultTransparentColourGreen;
    int      defaultTransparentColourBlue;

    bool     tiled;
    bool     debugImageErrors;
    bool     isLoaded;

    /**
    * The actual image.
    * Some code (e.g. the OS mouse code) doesn't need a texture handle
    * but an iImage instead, so some room for storage is provided here.
    */
    csRef<iImage> image;

    bool PreparePixmap();

public:
    pawsImageDrawable(iDocumentNode* node);
    pawsImageDrawable(const char* file, const char* resource, bool tiled, const csRect &textureRect, int alpha, int transR, int transG, int transB);
    pawsImageDrawable(const char* file, const char* resource);

    virtual ~pawsImageDrawable();

    const char* GetName() const;

    void Draw(int x, int y, int alpha=-1);
    void Draw(csRect rect, int alpha=-1);
    void Draw(int x, int y, int newWidth, int newHeight, int alpha=-1);

    int GetWidth() const;
    int GetHeight() const;
    void ExpandClipRect(csRect & /*clipRect*/) {};

    int GetDefaultAlpha() const;

    bool IsLoaded() const;
    iImage* GetImage();
    int GetTransparentRed() const;
    int GetTransparentGreen() const;
    int GetTransparentBlue() const;
};

/** @} */

#endif // PAWS_IMAGE_DRAWABLE
