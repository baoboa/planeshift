/*
 * pawsmouse.h - Author: Andrew Craig
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
// pawsmouse.h: interface for the pawsMouse class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_MOUSE_HEADER
#define PAWS_MOUSE_HEADER

#include <csgeom/vector2.h>
#include "pawstexturemanager.h"
#include <igraphic/imageio.h>
#include <util/psutil.h>

class PawsManager;

struct iGraphics3D;

/**
 * \addtogroup common_paws
 * @{ */

/** The mouse pointer.
 */
class pawsMouse
{

public:
    pawsMouse();
    virtual ~pawsMouse();

    /// Set the absolute screen position for mouse.
    void SetPosition(int x, int y);

    /// Updates the position of the dragged widget if any.
    void UpdateDragPosition();

    /// Get the absolute position.
    psPoint GetPosition()
    {
        return currentPosition;
    }

    /// Get the deltas from the last call to SetPosition
    psPoint GetDeltas()
    {
        return deltas;
    }

    /** Change mouse pointer to new image.
     * @param imageName A resource name to use as the image.
     */
    void ChangeImage(const char* imageName);
    void ChangeImage(iPawsImage* drawable);

    void Draw();
    void Hide(bool h = true);
    void WantCrosshair(bool h = true)
    {
        crosshair = h;
    }

    struct ImgSize
    {
        int width;
        int height;
    };

    ImgSize GetImageSize()
    {
        ImgSize size;
        size.width = 0;
        size.height = 0;
        if(cursorImage)
        {
            size.height = cursorImage->GetHeight();
            size.width = cursorImage->GetWidth();
        }
        return size;
    }

protected:
    csRef<iGraphics3D> graphics3D;
    csRef<iVFS> vfs;
    csRef<iImageIO>    imageLoader;

    psPoint currentPosition;
    psPoint deltas;

    csRef<iPawsImage> cursorImage;
    csRef<iPawsImage> crosshairImage;
    bool hidden, crosshair;

    bool useOS;
    bool basicCursor;

    int transparentR;
    int transparentG;
    int transparentB;

    csRef<iImage> image;

    void SetOSMouse(iPawsImage* mouseImage);
};

/** @} */

#endif

