/*
* pawsframedrawable.h
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

#ifndef PAWS_FRAME_DRAWABLE
#define PAWS_FRAME_DRAWABLE

#include "pawstexturemanager.h"

/**
 * \addtogroup common_paws
 * @{ */

class pawsFrameDrawable  : public scfImplementation1<pawsFrameDrawable, iPawsImage>
{
private:
    enum FRAME_DRAWABLE_PIECE
    {
        FDP_TOP_LEFT=0,
        FDP_TOP,
        FDP_TOP_RIGHT,
        FDP_LEFT,
        FDP_MIDDLE,
        FDP_RIGHT,
        FDP_BOTTOM_LEFT,
        FDP_BOTTOM,
        FDP_BOTTOM_RIGHT,

        FDP_COUNT
    };

    struct pawsFrameDrawablePiece
    {
        csRef<iPawsImage> drawable;
        int                  offsetx;
        int                  offsety;
    };
    pawsFrameDrawablePiece pieces[FDP_COUNT];

    enum FRAME_DRAWABLE_TYPE
    {
        FDT_HORIZONTAL=0,
        FDT_VERTICAL,
        FDT_FULL,

        FDT_COUNT
    };
    FRAME_DRAWABLE_TYPE type;

    /// The VFS path to the file.
    csString imageFileLocation;

    /// The resource name ( or identifier ) for this description.
    csString resourceName;

    /// The default alpha value that should be used.
    int      defaultAlphaValue;

    /// The default transparent colour.
    int      defaultTransparentColourRed;
    int      defaultTransparentColourGreen;
    int      defaultTransparentColourBlue;

    void LoadPiece(iDocumentNode* node, FRAME_DRAWABLE_PIECE piece);
    void DrawPiece(FRAME_DRAWABLE_PIECE p, int x, int y, int w, int h, int alpha, bool scaleX=false, bool scaleY=false);
    void DrawPiece(FRAME_DRAWABLE_PIECE p, int x, int y, int alpha);

public:
    pawsFrameDrawable(iDocumentNode* node);

    virtual ~pawsFrameDrawable();

    const char* GetName() const;

    void Draw(int x, int y, int alpha=-1);
    void Draw(csRect rect, int alpha=-1);
    void Draw(int x, int y, int newWidth, int newHeight, int alpha=-1);

    int GetWidth() const;
    int GetHeight() const;

    bool IsLoaded() const
    {
        return true;
    }

    void ExpandClipRect(csRect &clipRect);

    int GetDefaultAlpha() const;
};

/** @} */

#endif // PAWS_FRAME_DRAWABLE
