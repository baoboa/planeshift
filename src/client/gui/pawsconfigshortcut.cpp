/*
 * pawsconfigshortcut.cpp - Author: Joe Lyon
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

// CS INCLUDES
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/stringarray.h>
#include <iutil/vfs.h>


// COMMON INCLUDES
#include "util/log.h"
#include "psmainwidget.h"

// CLIENT INCLUDES
#include "../globals.h"
#include "util/psxmlparser.h"


// PAWS INCLUDES
#include "pawsconfigshortcut.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "shortcutwindow.h"
#include "pawsscrollmenu.h"
#include "paws/pawsradio.h"

pawsConfigShortcut::pawsConfigShortcut() :
    buttonHeight(NULL),
    buttonWidthMode(NULL),
    buttonWidth(NULL),
    editLockMode(NULL),
    leftScroll(NULL),
    rightScroll(NULL),
    enableScrollBar(NULL),
    buttonBackground(NULL),
    healthAndMana(NULL),
    textFont(NULL),
    textSize(NULL),
    textSpacing(NULL),
    ShortcutMenu(NULL),
    MenuBar(NULL)
{
    loaded= false;
}

bool pawsConfigShortcut::Initialize()
{
    LoadFromFile("configshortcut.xml");
    return true;
}

bool pawsConfigShortcut::PostSetup()
{
//get pointers to Shortcut Menu and its Menu Bar
    psMainWidget*   Main    = psengine->GetMainWidget();
    if( Main==NULL )
    {
        Error1( "pawsConfigShortcut::PostSetup unable to get psMainWidget\n");
        return false;
    }

    ShortcutMenu = Main->FindWidget( "ShortcutMenu",true );
    if( ShortcutMenu==NULL )
    {
        Error1( "pawsConfigShortcut::PostSetup unable to get ShortcutMenu\n");
        return false;
    }

    MenuBar = (pawsScrollMenu*)(ShortcutMenu->FindWidget( "MenuBar",true ));
    if( MenuBar==NULL )
    {
        Error1( "pawsConfigShortcut::PostSetup unable to get MenuBar\n");
        return false;
    }

//get form widgets
    buttonHeight = (pawsScrollBar*)FindWidget("buttonHeight");
    if(!buttonHeight)
    {
        return false;
    }
    buttonHeight->SetMaxValue(64);
    buttonHeight->SetCurrentValue(48,false);

    buttonWidthMode = (pawsRadioButtonGroup*)FindWidget("buttonWidthMode");
    if(!buttonWidthMode)
    {
        return false;
    }

    buttonWidth = (pawsScrollBar*)FindWidget("buttonWidth");
    if(!buttonWidth)
    {
        return false;
    }
    buttonWidth->SetMaxValue(512);
    buttonWidth->SetCurrentValue(48,false);


    editLockMode = (pawsRadioButtonGroup*)FindWidget("editLockMode" );
    if(!editLockMode)
    {
        return false;
    }


    leftScroll = (pawsRadioButtonGroup*)FindWidget("leftScroll");
    if(!leftScroll)
    {
        return false;
    }

    rightScroll = (pawsRadioButtonGroup*)FindWidget("rightScroll");
    if(!rightScroll)
    {
        return false;
    }

    enableScrollBar = (pawsRadioButtonGroup*)FindWidget("enableScrollBar");
    if(!enableScrollBar)
    {
        return false;
    }

    buttonBackground = (pawsCheckBox*)FindWidget("buttonBackground");
    if(!buttonBackground)
    {
        return false;
    }

    healthAndMana = (pawsCheckBox*)FindWidget("healthAndMana");
    if(!healthAndMana)
    {
        return false;
    }

    textFont = (pawsComboBox*)FindWidget("textFont");
    if(!textFont)
    {
        return false;
    }
    csRef<iVFS>           vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    if( vfs )
    {
        csRef<iStringArray>   fileList( vfs->FindFiles( "/planeshift/data/ttf/*.ttf" ));
        for (size_t i = 0; i < fileList->GetSize(); i++)
        {
            csString fileName ( fileList->Get (i));
            fileName.DeleteAt( 0, 21 ); // remove the leading path.
            fileName.ReplaceAll( ".ttf", "" );

            textFont->NewOption( fileName.GetData() );
        }
    }
    else
    {
        Error1( "pawsConfigShortcutWindow::PostSetup unable to find vfs for font list" );
    }


    textSize = (pawsScrollBar*)FindWidget("textSize");
    if(!textSize)
    {
        return false;
    }
    textSize->SetCurrentValue(10,false);
    textSize->SetMaxValue(40);

    textSpacing = (pawsScrollBar*)FindWidget("textSpacing");
    if(!textSpacing)
    {
        return false;
    }
    textSpacing->SetCurrentValue(4,false);
    textSpacing->SetMaxValue(20);


    return true;
}

bool pawsConfigShortcut::LoadConfig()
{

        buttonHeight->SetCurrentValue( MenuBar->GetButtonHeight() );
        if( MenuBar->GetButtonWidth()==0 )
        {
            buttonWidthMode->SetActive( "buttonWidthAutomatic" );
            buttonWidth->SetCurrentValue( 0 );
        }
        else
        {
            buttonWidthMode->SetActive( "buttonWidthManual" );
            buttonWidth->SetCurrentValue( MenuBar->GetButtonWidth() );
        }

        if(  MenuBar->GetEditMode()>0 )
        {
            editLockMode->SetActive( "editLockAll" );
        }
        else
        {
             editLockMode->SetActive( "editLockDND" );
        }

        textSpacing->SetCurrentValue( MenuBar->GetButtonPaddingWidth() );

        switch(  MenuBar->GetLeftScroll() )
        {
            case ScrollMenuOptionENABLED :
            {
                leftScroll->SetActive( "buttonScrollOn" );
            }
            break;

            case ScrollMenuOptionDYNAMIC :
            {
                leftScroll->SetActive( "buttonScrollAuto" );
            }
            break;

            case ScrollMenuOptionDISABLED :
            {
                leftScroll->SetActive( "buttonScrollOff" );
            }
            break;

        }

        switch(  MenuBar->GetRightScroll() )
        {
            case ScrollMenuOptionENABLED :
            {
                rightScroll->SetActive( "buttonScrollOn" );
            }
            break;

            case ScrollMenuOptionDYNAMIC :
            {
                rightScroll->SetActive( "buttonScrollAuto" );
            }
            break;

            case ScrollMenuOptionDISABLED :
            {
                rightScroll->SetActive( "buttonScrollOff" );
            }
            break;

        }

        if( MenuBar->IsButtonBackgroundEnabled() )
        {
            buttonBackground->SetState(1);
        }
        else
        {
            buttonBackground->SetState(0);
        }

        healthAndMana->SetState(  ((pawsShortcutWindow*)ShortcutMenu)->GetMonitorState() );


        enableScrollBar->TurnAllOff();

        textSize->SetCurrentValue(MenuBar->GetFontSize());

    csString tFontName=csString(((pawsShortcutWindow*)ShortcutMenu)->GetFontName());
    tFontName.DeleteAt(0,21);
    tFontName.ReplaceAll(".ttf","" );
    textFont->Select( tFontName.GetData() );


    loaded= true;
    dirty = false;
    return true;
}

bool pawsConfigShortcut::SaveConfig()
{

    csString xml;
    xml = "<shortcut>\n";
    xml.AppendFmt("<buttonHeight value=\"%d\" />\n",
                     int(buttonHeight->GetCurrentValue()));
    xml.AppendFmt("<buttonWidthMode active=\"%s\" />\n",
                     buttonWidthMode->GetActive().GetData());
    xml.AppendFmt("<buttonWidth value=\"%d\" />\n",
                     int(buttonWidth->GetCurrentValue()));
    xml.AppendFmt("<leftScroll active=\"%s\" />\n",
                     leftScroll->GetActive().GetData());
    xml.AppendFmt("<rightScroll active=\"%s\" />\n",
                     rightScroll->GetActive().GetData());
    xml.AppendFmt("<editLockMode active=\"%s\" />\n",
                     editLockMode->GetActive().GetData());
    xml.AppendFmt("<enableScrollBar active=\"%s\" />\n",
                     enableScrollBar->GetActive().GetData());
    xml.AppendFmt("<healthAndMana on=\"%s\" />\n",
                     healthAndMana->GetState() ? "yes" : "no");
    xml.AppendFmt("<buttonBackground on=\"%s\" />\n",
                     buttonBackground->GetState() ? "yes" : "no");
    xml.AppendFmt("<textSize value=\"%d\" />\n",
                     int(textSize->GetCurrentValue()));
    xml.AppendFmt("<textFont value=\"%s\" />\n",
                     textFont->GetSelectedRowString().GetData());
    xml.AppendFmt("<textSpacing value=\"%d\" />\n",
                     int(textSpacing->GetCurrentValue()));
    xml += "</shortcut>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/configshortcut.xml",
                                         xml,xml.Length());
}

void pawsConfigShortcut::SetDefault()
{
    LoadConfig();
}

bool pawsConfigShortcut::OnScroll(int /*scrollDir*/, pawsScrollBar* wdg)
{
    //dirty = true;

    if(wdg == buttonWidth && loaded)
    {
        if(buttonWidth->GetCurrentValue() < buttonHeight->GetCurrentValue())
            buttonWidth->SetCurrentValue(buttonHeight->GetCurrentValue());
        else if( buttonWidth->GetCurrentValue() > buttonWidth->GetMaxValue() )
            buttonWidth->SetCurrentValue(buttonWidth->GetMaxValue());

        MenuBar->SetButtonWidth( buttonWidth->GetCurrentValue() );
        MenuBar->LayoutButtons();

    }
    else if(wdg == buttonHeight && loaded)
    {
        if(buttonHeight->GetCurrentValue() < 1)
            buttonHeight->SetCurrentValue(1,false);
        MenuBar->SetButtonHeight( buttonHeight->GetCurrentValue() );
        MenuBar->LayoutButtons();
    }
    else if(wdg == textSize && loaded)
    {
        if(textSize->GetCurrentValue() < 1)
            textSize->SetCurrentValue(1,false);
        PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
        MenuBar->LayoutButtons();
    }
    else if(wdg == textSpacing && loaded)
    {
        if( textSpacing->GetCurrentValue() < 1 )
            textSpacing->SetCurrentValue(1,false);
        MenuBar->SetButtonPaddingWidth(  textSpacing->GetCurrentValue() );
        MenuBar->LayoutButtons();
    }
    
    if( loaded )
        SaveConfig();
    MenuBar->LayoutButtons();
    MenuBar->OnResize();
    ((pawsShortcutWindow*)ShortcutMenu)->Draw();
    return true;
}

bool pawsConfigShortcut::OnButtonPressed(int /*button*/, int /*mod*/, pawsWidget* wdg)
{

    dirty = true;

    switch( wdg->GetID() )
    {
        case 1000 : //buttonWidthMode == automtic
        {
            MenuBar->SetButtonWidth( 0 );
        }
        break;

        case 1001 : //buttonWidthMode == manual
        {
            MenuBar->SetButtonWidth( buttonWidth->GetCurrentValue() );
        }
        break;

        case 1002 : //editLock == Prevent all
        {
            MenuBar->SetEditMode( 1 );
        }
        break;

        case 1003 : //editLock == prevent DnD
        {
            MenuBar->SetEditMode( 0 );
        }
        break;

        case 1004 : 
        {
            MenuBar->SetLeftScroll(ScrollMenuOptionENABLED );
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        }
        break;

        case 1005 : 
        {
            MenuBar->SetLeftScroll(ScrollMenuOptionDYNAMIC );
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        }
        break;

        case 1006 : 
        {
            MenuBar->SetLeftScroll(ScrollMenuOptionDISABLED );
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        }
        break;

        case 1007 : 
        {
            MenuBar->SetRightScroll(ScrollMenuOptionENABLED );
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        }
        break;

        case 1008 : 
        {
            MenuBar->SetRightScroll(ScrollMenuOptionDYNAMIC );
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        }
        break;

        case 1009 : 
        {
            MenuBar->SetRightScroll(ScrollMenuOptionDISABLED );
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        }
        break;

        case 1010 : 
        {
            //enable scroll bar
        }
        break;

        case 1011 : 
        {
            //auto scroll bar
        }
        break;

        case 1012 : 
        {
            //disable scroll bar
        }
        break;

        case 1013 : 
        {
            if( ((pawsCheckBox*)wdg)->GetState()==true )
            {
                //enable Health and mana bars
                ((pawsShortcutWindow*)ShortcutMenu)->StartMonitors();
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
            }
            else
            {
                //disable health and mana bars
                ((pawsShortcutWindow*)ShortcutMenu)->StopMonitors();
                ((pawsShortcutWindow*)ShortcutMenu)->Draw();
            }
        }
        break;

        case 1014 : 
        {
            if( ((pawsCheckBox*)wdg)->GetState()==true )
            {
                MenuBar->EnableButtonBackground( true );
                
            }
            else
            {
                MenuBar->EnableButtonBackground(false);
            }
        }
        break;

        default :
        {
            Error2( "pawsConfigShortcut::OnButtonPressed got unrecognized widget with ID = %i\n", wdg->GetID() );
            return false;
        }
    }

    if( loaded )
    {
        MenuBar->LayoutButtons();
        MenuBar->OnResize();
        ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        SaveConfig();    
    }

    return true;
}

void pawsConfigShortcut::PickText( const char * fontName, int size )
{
    csString    fontPath( "/planeshift/data/ttf/");
    fontPath += fontName;
    fontPath += ".ttf";

    if( loaded )
    {
        MenuBar->LayoutButtons();
        MenuBar->OnResize();
        ((pawsShortcutWindow*)ShortcutMenu)->SetFont( fontPath, size );
        MenuBar->SetFont( fontPath, size );
        MenuBar->SetButtonFont( fontPath, size );

        ((pawsShortcutWindow*)ShortcutMenu)->Draw();
        SaveConfig();    
    }
}

void pawsConfigShortcut::OnListAction(pawsListBox* selected, int status)

{
    PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
    MenuBar->LayoutButtons();
    MenuBar->OnResize();
    ((pawsShortcutWindow*)ShortcutMenu)->Draw();
    SaveConfig();
}

void pawsConfigShortcut::Show()
{
    pawsWidget::Show();
}

void pawsConfigShortcut::Hide()
{
    if(dirty)
    {
    }

    pawsWidget::Hide();
}

