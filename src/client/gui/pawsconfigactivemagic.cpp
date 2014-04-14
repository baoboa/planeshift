/*
 * pawsconfigactivemagic.cpp - Author: Joe Lyon
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
#include "pawsconfigactivemagic.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsactivemagicwindow.h"
#include "pawsscrollmenu.h"
#include "paws/pawsradio.h"


pawsConfigActiveMagic::pawsConfigActiveMagic() :
    ActiveMagicWindow(NULL),
    showEffects(NULL),
    autoResize(NULL),
    useImages(NULL),
    showWindow(NULL),
    buttonHeight(NULL),
    buttonWidthMode(NULL),
    buttonWidth(NULL),
    leftScroll(NULL),
    rightScroll(NULL),
    textFont(NULL),
    textSize(NULL),
    textSpacing(NULL),
    ActiveMagic(NULL),
    MenuBar(NULL)

{
    loaded= false;
}

bool pawsConfigActiveMagic::Initialize()
{
    LoadFromFile("configactivemagic.xml");
    return true;
}

bool pawsConfigActiveMagic::PostSetup()
{

//get pointer to ActiveMagic window
    psMainWidget*   Main    = psengine->GetMainWidget();
    if( Main==NULL )
    {
        Error1( "pawsConfigActiveMagic::PostSetup unable to get psMainWidget\n");
        return false;
    }

    ActiveMagicWindow = (pawsActiveMagicWindow *)Main->FindWidget( "ActiveMagicWindow",true );
    if( ActiveMagicWindow==NULL )
    {
        Error1( "pawsConfigActiveMagic::PostSetup unable to get ActiveMagic window\n");
        return false;
    }

    MenuBar = (pawsScrollMenu*)(ActiveMagicWindow->FindWidget( "BuffBar",true ));
    if( MenuBar==NULL )
    {
        Error1( "pawsConfigActiveMagic::PostSetup unable to get MenuBar\n");
        return false;
    }


//get form widgets
    showEffects = (pawsRadioButtonGroup*)FindWidget("showEffects");
    if(!showEffects)
    {
        return false;
    }

    autoResize = (pawsCheckBox*)FindWidget("autoResize");
    if(!autoResize)
    {
        return false;
    }

    useImages = (pawsCheckBox*)FindWidget("useImages");
    if(!useImages)
    {
        return false;
    }

    showWindow = (pawsCheckBox*)FindWidget("showWindow");
    if(!showWindow)
    {
        return false;
    }

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
        Error1( "pawsConfigActiveMagic::PostSetup unable to find vfs for font list" );
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

bool pawsConfigActiveMagic::LoadConfig()
{
    useImages->SetState( ActiveMagicWindow->GetUseImages() ); 
    autoResize->SetState( ActiveMagicWindow->GetAutoResize() ); 
    showEffects->SetActive( ActiveMagicWindow->GetShowEffects()?"itemAndSpell":"spellOnly" ); 
    showWindow->SetState( ActiveMagicWindow->IsVisible() ); 

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

    textSize->SetCurrentValue(MenuBar->GetFontSize());

    csString tFontName=csString(((pawsActiveMagicWindow*)ActiveMagicWindow)->GetFontName());
    tFontName.DeleteAt(0,21);
    tFontName.ReplaceAll(".ttf","" );
    textFont->Select( tFontName.GetData() );

    loaded= true;
    dirty = false;

    return true;
}

bool pawsConfigActiveMagic::SaveConfig()
{
    csString xml;
    xml = "<activemagic>\n";
    xml.AppendFmt("<useImages on=\"%s\" />\n",
                     useImages->GetState() ? "yes" : "no");
    xml.AppendFmt("<autoResize on=\"%s\" />\n",
                     autoResize->GetState() ? "yes" : "no");
    xml.AppendFmt("<showEffects active=\"%s\" />\n",
                     showEffects->GetActive().GetData());
    xml.AppendFmt("<showWindow on=\"%s\" />\n",
                     showWindow->GetState() ? "yes" : "no");
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
    xml.AppendFmt("<textSize value=\"%d\" />\n",
                     int(textSize->GetCurrentValue()));
    xml.AppendFmt("<textFont value=\"%s\" />\n",
                     textFont->GetSelectedRowString().GetData());
    xml.AppendFmt("<textSpacing value=\"%d\" />\n",
                     int(textSpacing->GetCurrentValue()));


    xml += "</activemagic>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/configactivemagic.xml",
                                         xml,xml.Length());
}

void pawsConfigActiveMagic::SetDefault()
{
    LoadConfig();
}

bool pawsConfigActiveMagic::OnScroll(int /*scrollDir*/, pawsScrollBar* wdg)
{

    if(!loaded )
        return true;

    if(wdg == buttonWidth )
    {
        if(buttonWidth->GetCurrentValue() < buttonHeight->GetCurrentValue())
            buttonWidth->SetCurrentValue(buttonHeight->GetCurrentValue());
        else if( buttonWidth->GetCurrentValue() > buttonWidth->GetMaxValue() )
            buttonWidth->SetCurrentValue(buttonWidth->GetMaxValue());

        MenuBar->SetButtonWidth( buttonWidth->GetCurrentValue() );

    }
    else if(wdg == buttonHeight )
    {
        if(buttonHeight->GetCurrentValue() < 1)
            buttonHeight->SetCurrentValue(1,false);
        MenuBar->SetButtonHeight( buttonHeight->GetCurrentValue() );
    }
    else if(wdg == textSize)
    {
        if(textSize->GetCurrentValue() < 1)
            textSize->SetCurrentValue(1,false);
        PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
    }
    else if(wdg == textSpacing )
    {
        if( textSpacing->GetCurrentValue() < 1 )
            textSpacing->SetCurrentValue(1,false);
        MenuBar->SetButtonPaddingWidth(  textSpacing->GetCurrentValue() );
    }

    SaveConfig();
    MenuBar->LayoutButtons();
    MenuBar->OnResize();
    ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();

    return true;
}

bool pawsConfigActiveMagic::OnButtonPressed(int /*button*/, int /*mod*/, pawsWidget* wdg)
{
    dirty = true;

    switch( wdg->GetID() )
    {
        case 1020 : //spell effects only
        {
            ActiveMagicWindow->SetShowEffects(false);
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
            csString blankSpell;
            blankSpell="";
            psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
            msg.SendMessage();
        }
        break;

        case 1021 : //Item and spell effects
        {
            ActiveMagicWindow->SetShowEffects(true);
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
            csString blankSpell;
            blankSpell="";
            psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
            msg.SendMessage();
        }
        break;

        case 1022 : // use icons (true) or text (false)?
        {
            ActiveMagicWindow->SetUseImages(useImages->GetState());
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
            csString blankSpell;
            blankSpell="";
            psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
            msg.SendMessage();
        }
        break;

        case 1023 : // auto- or manual sizing
        {
            ActiveMagicWindow->SetAutoResize(autoResize->GetState());
            if( ActiveMagicWindow->GetAutoResize() )
            {
                ActiveMagicWindow->AutoResize();
            }
        }
        break;

        case 1024 : // enable or disable the window
        {
            ActiveMagicWindow->SetShowWindow(showWindow->GetState());
            pawsWidget* widget = PawsManager::GetSingleton().FindWidget( "ActiveMagicWindow" );
            if( ActiveMagicWindow->GetShowWindow() )
            {
                widget->Show();
            }
            else
            {
                widget->Hide();
            }
        }
        break;

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

        case 1004 :
        {
            MenuBar->SetLeftScroll(ScrollMenuOptionENABLED );
                ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        }
        break;

        case 1005 :
        {
            MenuBar->SetLeftScroll(ScrollMenuOptionDYNAMIC );
                ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        }
        break;

        case 1006 :
        {
            MenuBar->SetLeftScroll(ScrollMenuOptionDISABLED );
                ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        }
        break;

        case 1007 :
        {
            MenuBar->SetRightScroll(ScrollMenuOptionENABLED );
                ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        }
        break;

        case 1008 :
        {
            MenuBar->SetRightScroll(ScrollMenuOptionDYNAMIC );
                ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        }
        break;
        case 1009 :
        {
            MenuBar->SetRightScroll(ScrollMenuOptionDISABLED );
                ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
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
            Error2( "pawsConfigActiveMagic::OnButtonPressed got unrecognized widget with ID = %i\n", wdg->GetID() );
            return false;
        }
    }

    if( loaded )
    {
        MenuBar->LayoutButtons();
        MenuBar->OnResize();
        ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        SaveConfig();
    }

    return true;
}

void pawsConfigActiveMagic::PickText( const char * fontName, int size )
{
    csString    fontPath( "/planeshift/data/ttf/");
    fontPath += fontName;
    fontPath += ".ttf";

    if( loaded )
    {
        ((pawsActiveMagicWindow*)ActiveMagicWindow)->SetFont( fontPath, size);
        MenuBar->SetFont( fontPath, size );
        MenuBar->SetButtonFont( fontPath, size );

        ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
        SaveConfig();
        MenuBar->LayoutButtons();
        MenuBar->OnResize();
    }

}

void pawsConfigActiveMagic::OnListAction(pawsListBox* selected, int status)
{
    PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
    MenuBar->LayoutButtons();
    MenuBar->OnResize();
    ((pawsActiveMagicWindow*)ActiveMagicWindow)->Draw();
    SaveConfig();
}

void pawsConfigActiveMagic::Show()
{
    pawsWidget::Show();
}

void pawsConfigActiveMagic::Hide()
{
    pawsWidget::Hide();
}

void pawsConfigActiveMagic::SetMainWindowVisible( bool status )
{
    if( loaded )
    {
        showWindow->SetState( status );
    }
}

