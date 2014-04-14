/*
 * pawsscrollmenu.h - Author: Joe Lyon
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
// pawsscrollmenu.h: interface for the pawsScrollMenu class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_SCROLLMENU_HEADER
#define PAWS_SCROLLMENU_HEADER



#include "globals.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "pawsscrollmenu.h"


//=============================================================================
// Defines
//=============================================================================
#define BUTTON_PADDING          4
#define SHORTCUT_BUTTON_OFFSET  2000

#define ScrollMenuOptionDISABLED    0
#define ScrollMenuOptionENABLED     1
#define ScrollMenuOptionDYNAMIC     2
#define ScrollMenuOptionHORIZONTAL  3
#define ScrollMenuOptionVERTICAL    4

#include "paws/pawswidget.h"
#include "gui/pawsdndbutton.h"





/** A scrolling list of buttons, each with an icon and which accepts drag-n-drop.
 */
class pawsScrollMenu : public pawsWidget
{

public:


    pawsScrollMenu();
    virtual ~pawsScrollMenu();

    bool PostSetup();
    void OnResize();
    void OnResizeStop();
    int  CalcButtonSize(pawsDnDButton* target);
    void LayoutButtons();

    virtual bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);

    bool LoadArrays(csArray<csString> &name, csArray<csString> &icon, csArray<csString> &toolTip, csArray<csString> &actions, int baseIndex, pawsWidget* widget);
    bool LoadSingle(csString name, csString icon, csString toolTip, csString action, int Index, pawsWidget* widget, bool IsEnabled);

    bool RemoveByName(csString name);
    bool Clear();
    int  GetSize();
    int  GetButtonWidth();


    /**
     * Find the width of the widest button
     *
     * @return     zero if there are no buttons,
     *             or the size in pixels based on the text font settings
     *             or the size in pixels of the fixed button width.
     */
    int GetWidestWidth();

    int  GetButtonHeight();
    int  GetButtonHolderWidth();
    int  GetButtonHolderHeight();
    int  GetTotalButtonWidth();
    int  GetTotalButtonWidth(unsigned int targetButton);
    int  GetOrientation();
    int  GetEditMode()
    {
        return EditMode;
    }
    int GetEditLock()
    {
        return EditLockMode;
    }

    virtual bool Setup(iDocumentNode* node);
    bool SelfPopulate(iDocumentNode* node);

    bool ScrollUp();
    bool ScrollDown();
    bool ScrollToPosition( float pos );
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);

    void SetEditLock(int mode);
    bool IsEditable()
    {
        return EditLockButton->GetState();
    }
    void SetLeftScroll(int mode);
    int GetLeftScroll();
    void SetRightScroll(int mode);
    int GetRightScroll();
    void SetButtonWidth(int width);
    void SetButtonHeight(int height);
    void SetScrollIncrement(int incr);
    void SetScrollProportion(float prop);
    bool SetScrollWidget( pawsScrollBar* sb);
    void SetOrientation(int Orientation);
    int  AutoResize();
   /**
    * set edit lock to allow rt-click edit and prevent DND, or to disallow all editing
    */
    void SetEditMode(int val)
    {
        EditMode = val;
    }

    void SetButtonPaddingWidth( int width )
    {
        paddingWidth = width;
    }
    int GetButtonPaddingWidth()
    {
        return paddingWidth;
    }
    void SetButtonBackground( const char* image );
    csString GetButtonBackground();
    void SetButtonFont( const char* Font, int size );
    char const * GetButtonFontName();
    float GetButtonFontSize();

   /**
    * return the name of the font
    */
    char const * GetFontName()
    {
        return fontName;
    }

    void SetTextSpacing( int v );

    void EnableButtonBackground( bool mode );
    bool IsButtonBackgroundEnabled();





protected:

    int                    buttonWidth,
                           buttonHeight,
                           scrollIncrement,
                           currentButton,
                           paddingWidth;

    float                  scrollProportion;
    bool                   buttonWidthDynamic;

    pawsWidget*            ButtonHolder;
    csArray<pawsWidget*>   Buttons;
    int                    buttonLocation;

    pawsButton*            LeftScrollButton;
    int                    LeftScrollMode;
    bool                   LeftScrollVisible;

    pawsButton*            RightScrollButton;
    int                    RightScrollMode;
    bool                   RightScrollVisible;

    pawsButton*            EditLockButton;
    int                    EditLockMode;               //enabled, disabled, (dynamic==>enabled)
    bool                   EditLock;                 //true = editing prevented, false = editing allowed

    pawsWidget*            callbackWidget;

    pawsScrollBar*         scrollBarWidget;

    int                    Orientation;
    int                    EditMode;

};

//----------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsScrollMenu);

/** @} */

#endif
