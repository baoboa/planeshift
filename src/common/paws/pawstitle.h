/*
* pawstitle.h
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

#ifndef PAWS_TITLE_HEADER
#define PAWS_TITLE_HEADER

#include "pawsbutton.h"

/**
 * \addtogroup common_paws
 * @{ */

class pawsTitle : public pawsWidget
{
private:
    enum PAWS_TITLE_ALIGN
    {
        PTA_LEFT = 0,
        PTA_CENTER,
        PTA_RIGHT,

        PTA_COUNT
    };

    struct pawsTitleButton
    {
        char             widgetName[64];
        pawsWidget*      buttonWidget;
        PAWS_TITLE_ALIGN align;
        int              offsetx;
        int              offsety;
    };
    csArray<pawsTitleButton> titleButtons;

    PAWS_TITLE_ALIGN titleAlign;
    bool             scaleWidth;
    float            width;
    int              height;
    csString         text;
    int              textOffsetx;
    int              textOffsety;
    PAWS_TITLE_ALIGN textAlign;

    PAWS_TITLE_ALIGN GetAlign(const char* alignText);

public:
    pawsTitle(pawsWidget* parent, iDocumentNode* node);

    pawsTitle(const pawsTitle &origin);

    virtual ~pawsTitle();

    bool Setup(iDocumentNode* node);
    bool PostSetup();
    void SetWindowRect(const csRect &windowRect);
    void Draw();

    void Resize() {}
    void MoveDelta(int /*dx*/, int /*dy*/) {}
    void MoveTo(int /*x*/, int /*y*/) {}
};

/** @} */

#endif // PAWS_TITLE_HEADER
