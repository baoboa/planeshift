/*
 * pawsactivemagicwindow.h
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

#ifndef PAWS_ACTIVEMAGIC_WINDOW_HEADER
#define PAWS_ACTIVEMAGIC_WINDOW_HEADER

#include "paws/pawswidget.h"
class pawsTextBox;
class pawsListBox;
class pawsMessageTextBox;
class pawsProgressBar;
class pawsCheckBox;
class pawsConfigPopup;

#include "net/cmdbase.h"
#include "net/subscriber.h"
#include "gui/pawscontrolwindow.h"
#include "gui/pawsscrollmenu.h"

#define CONFIG_ACTIVEMAGIC_FILE_NAME       "/planeshift/userdata/options/activemagic.xml"
#define CONFIG_ACTIVEMAGIC_FILE_NAME_DEF   "/planeshift/data/options/activemagic_def.xml"

/** This handles all the details about how the spell cancel works.
 */
class pawsActiveMagicWindow : public pawsControlledWindow, public psClientNetSubscriber
{
public:

    pawsActiveMagicWindow();
    virtual ~pawsActiveMagicWindow() {}

    bool PostSetup();
    bool Setup(iDocumentNode* node);

    void HandleMessage(MsgEntry* me);
    void OnResize();

    virtual void Close();
    virtual void Hide();
    virtual void Show();

    /** Loads the configuration file
    *   @return true if no errors and false if errors
    */
    bool LoadSetting();

    void SaveSetting();

    void AutoResize();

    /**
      * Check-box which gives the user a opportunity to show or
      * not to show the Active Magic Window
      */
    pawsCheckBox*    showWindow;

    void SetUseImages( bool setting );
    bool GetUseImages();

    void SetAutoResize( bool setting );
    bool GetAutoResize();

    void SetShowEffects( bool setting );
    bool GetShowEffects();

    void SetShowWindow( bool setting );
    bool GetShowWindow();

   /**
    * return the name of the font
    */
    char const * GetFontName()
    {
        return fontName.GetData();
    }


private:

    csRef<iVFS> vfs;

    bool useImages,
         autoResize,
         showEffects,
         show;   ///<true==show spell & item effects; false==show spell effects but not item effects


    pawsScrollMenu*  buffList;
    uint32_t         lastIndex;    ///<Version number of the last list received

    pawsConfigPopup* configPopup;  ///<This is used to point to a instance of ConfigPopup

    int  Orientation;


};

CREATE_PAWS_FACTORY(pawsActiveMagicWindow);

#endif
