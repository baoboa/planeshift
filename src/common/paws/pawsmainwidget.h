/*
 * pawsmainwidget.h - Author: Andrew Craig
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
// pawsmainwidget.h: interface for the pawsMainWidget class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_MAIN_WIDGET_HEADER
#define PAWS_MAIN_WIDGET_HEADER

#include <iengine/camera.h>

#include "pawswidget.h"
#include "psmousebinds.h"

/**
 * \addtogroup common_paws
 * @{ */

/** A paws script key.
 * This is read from the guikeys.xml file to handle gui keys.
 */
struct pawsScriptKey
{
    csKeyEventData key;
    csString widgetName;
    csString action;
};


/** The main or desktop widget.
 */
class pawsMainWidget : public pawsWidget
{
public:
    pawsMainWidget();
    virtual ~pawsMainWidget();

    virtual bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);

    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnMouseUp(int button, int modifiers, int x, int y);
    virtual bool OnDoubleClick(int button, int modifiers, int x, int y);

    virtual void ApplyWindowSettingsOnChildren(pawsWidget* caller, int alphaMin, int alphaMax, float fadeSpeed, bool fade, bool scaleFont);

protected:
    bool LoadGUIKeys(const char* fileName);

    //iCamera* guiCamera;
    csPDelArray<pawsScriptKey> keys;
};

/** @} */

#endif
