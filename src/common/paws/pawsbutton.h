/*
 * pawsbutton.h - Author: Andrew Craig
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
// pawsbutton.h: interface for the pawsButton class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_BUTTON_HEADER
#define PAWS_BUTTON_HEADER

#include "pawswidget.h"

/**
 * \addtogroup common_paws
 * @{ */

/* Types of flash buttons, used in chat window */
enum FLASH_STATE
{
    FLASH_REGULAR = 0,
    FLASH_HIGHLIGHT,
    FLASH_SPECIAL
};


/** A simple button widget.
 */
class pawsButton : public pawsWidget
{
public:
    pawsButton();
    pawsButton(const pawsButton &pb);
    virtual ~pawsButton();

    virtual bool Setup(iDocumentNode* node);
    bool SelfPopulate(iDocumentNode* node);

    void SetUpImage(const csString &image);
    void SetDownImage(const csString &image);
    void SetGreyUpImage(const csString &greyUpImage);
    void SetGreyDownImage(const csString &greyDownImage);

    /* Specify the image to use for special flashing in chat window */
    void SetOnSpecialImage(const csString &image);

    void SetSound(const char* sound);

    virtual void Draw();

    virtual bool OnMouseEnter();
    virtual bool OnMouseExit();
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnMouseUp(int button, int modifiers, int x, int y);
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);

    virtual bool IsDown()
    {
        return down;
    }
    virtual void SetState(bool isDown, bool publish=true);
    virtual bool GetState()
    {
        return down;
    }
    /// Set the toggle attribute. To change toggle state use SetState
    virtual void SetToggle(bool t)
    {
        toggle = t;
    }
    virtual void SetEnabled(bool enabled);
    virtual bool IsEnabled() const;
    virtual void SetNotify(pawsWidget* widget);

    void SetText(const char* text);
    const char* GetText()
    {
        return buttonLabel;
    }

    /* Button flashing used mainly in chatwindow */
    virtual void Flash(bool state, FLASH_STATE type = FLASH_REGULAR)
    {
        if(state && !down)
        {
            flash = 1;
        }
        else
        {
            flash = 0;
            if(flashtype == FLASH_HIGHLIGHT)
                SetColour(originalFontColour);
        }
        flashtype = type;
    }


protected:

    virtual bool CheckKeyHandled(int keyCode);

    /// Track to see if the button is down.
    bool down;

    /// Image to draw when button is pressed or when the mouse enters.
    csRef<iPawsImage> pressedImage;

    /// Image to draw when button is released or when the mouse exits.
    csRef<iPawsImage> releasedImage;

    csRef<iPawsImage> greyUpImage;
    csRef<iPawsImage> greyDownImage;

    /// Image to draw when button is released.
    csRef<iPawsImage> specialFlashImage;

    /// Check to see if this is a toggle button.
    bool toggle;

    /// Text shown in button
    csString buttonLabel;

    /// Keyboard equivalent of clicking on this button
    char keybinding;

    /// Widget to which event notifications are sent. If NULL, notifications go to parent
    pawsWidget* notify;

    /// Style -- right now only ShadowText supported
    int style;

    bool enabled;

    /// The state if the button is flashing, 0 is no flashing
    int flash;

    /// Type of flash (regular/special)
    FLASH_STATE flashtype;

    /// Button can trigger sound effects with this
    csString sound_click;
    int upTextOffsetX;
    int upTextOffsetY;
    int downTextOffsetX;
    int downTextOffsetY;

    ///Used when restoring from highlight state
    int originalFontColour;

    /// Whether or not to change image on mouse enter/exit.
    bool changeOnMouseOver;
};

//----------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsButton);

/** @} */

#endif
