/*
 * pawsconfigactivemagic.cpp - Author: Joe Lyon
 *
 * Copyright (C) 2014 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "pawsconfigchatfont.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "chatwindow.h"
#include "pawsscrollmenu.h"
#include "paws/pawsradio.h"


pawsConfigChatFont::pawsConfigChatFont() :
    ChatWindow(NULL),
    textFont(NULL),
    textSize(NULL),
    textSpacing(NULL)
{
    loaded= false;
}

bool pawsConfigChatFont::Initialize()
{
    LoadFromFile("configchatfont.xml");
    return true;
}

bool pawsConfigChatFont::PostSetup()
{

//get pointer to Chat window
    psMainWidget*   Main    = psengine->GetMainWidget();
    if( Main==NULL )
    {
        Error1( "pawsConfigChatFont::PostSetup unable to get psMainWidget\n");
        return false;
    }

    ChatWindow = (pawsChatWindow *)Main->FindWidget( "ChatWindow",true );
    if( ChatWindow==NULL )
    {
        Error1( "pawsConfigChatFont::PostSetup unable to get Chat window\n");
        return false;
    }

//get form widgets
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
            fileName.DeleteAt( fileName.Length()-4, 4); //delete the ending ".ttf"
                
            textFont->NewOption( fileName.GetData() );
        }
    }
    else
    {
        Error1( "pawsConfigChatFont::PostSetup unable to find vfs for font list" );
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

bool pawsConfigChatFont::LoadConfig()
{
    csString tFontName=csString(((pawsChatWindow*)ChatWindow)->GetFontName());
    tFontName.DeleteAt(0,21);
    tFontName.DeleteAt( tFontName.Length()-4, 4); //delete the ending ".ttf"
    textFont->Select( tFontName.GetData() );

    textSize->SetCurrentValue(((pawsChatWindow*)ChatWindow)->GetFontSize(),false);
    loaded= true;
    dirty = false;

    return true;
}

bool pawsConfigChatFont::SaveConfig()
{
    csString xml;
    xml = "<chat>\n";
    xml.AppendFmt("<textSize value=\"%d\" />\n",
                     int(textSize->GetCurrentValue()));
    xml.AppendFmt("<textFont value=\"%s\" />\n",
                     textFont->GetSelectedRowString().GetData());
    xml.AppendFmt("<textSpacing value=\"%d\" />\n",
                     int(textSpacing->GetCurrentValue()));


    xml += "</chat>\n";

    dirty = false;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/configchatfont.xml",
                                         xml,xml.Length());
}

void pawsConfigChatFont::SetDefault()
{
    LoadConfig();
}

bool pawsConfigChatFont::OnScroll(int /*scrollDir*/, pawsScrollBar* wdg)
{
    if(wdg == textSize && loaded)
    {
        if(textSize->GetCurrentValue() < 1)
            textSize->SetCurrentValue(1,false);
        PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
    }
    else if(wdg == textSpacing && loaded)
    {
        if( textSpacing->GetCurrentValue() < 1 )
            textSpacing->SetCurrentValue(1,false);
    }

    if( loaded )
        SaveConfig();
    ((pawsChatWindow*)ChatWindow)->Draw();

    return true;
}

bool pawsConfigChatFont::OnButtonPressed(int /*button*/, int /*mod*/, pawsWidget* wdg)
{
    dirty = true;

    switch( wdg->GetID() )
    {
        default :
        {
            Error2( "pawsConfigChatFont::OnButtonPressed got unrecognized widget with ID = %i\n", wdg->GetID() );
            return false;
        }
    }

    if( loaded )
    {
        ((pawsChatWindow*)ChatWindow)->Draw();
        SaveConfig();
    }

    return true;
}

void pawsConfigChatFont::PickText( const char * fontName, int size )
{
    csString    fontPath( "/planeshift/data/ttf/");
    fontPath += fontName;
    fontPath += ".ttf";

    if( loaded )
    {
        ((pawsChatWindow*)ChatWindow)->SetChatWindowFont( fontPath, size );
        SaveConfig();
    }

}

void pawsConfigChatFont::OnListAction(pawsListBox* selected, int status)
{
    PickText( textFont->GetSelectedRowString(),  textSize->GetCurrentValue() );
    ((pawsChatWindow*)ChatWindow)->Draw();
    SaveConfig();
}

void pawsConfigChatFont::Show()
{
    pawsWidget::Show();
}

void pawsConfigChatFont::Hide()
{
    pawsWidget::Hide();
}
