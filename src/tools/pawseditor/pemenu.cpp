/*
* pemenu.cpp
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : menu widget for paws editor
*
*/

#include <psconfig.h>
#include "pemenu.h"
#include "pawseditorglobals.h"
#include "peeditablewidget.h"
#include "pepawsmanager.h"

#include <csutil/cfgfile.h>
#include <cstool/csview.h>

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawscombo.h"
#include "paws/pawsgenericview.h"
#include "paws/pawsmainwidget.h"

bool peMenu::PostSetup()
{
    return true;
}



bool peMenu::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    pePawsManager * pePaws = static_cast<pePawsManager *>(PawsManager::GetSingletonPtr());
    switch ( widget->GetID() )
    {
    case 0:
        editApp->Exit();
        return true;
        break;

    case 101:
    case 102:
        (void) pawsFileNavigation::Create("/this/data/gui/","|*.xml|",
                                   new OnFileSelected( this , widget->GetID() ), "data/pawseditor/filenavigation.xml");
        return true;
        break;
    case 105:
        pePaws->ReloadStyles();

        // Automatically reload widget after reloading styles.
    case 104:
        editApp->ReloadWidget();
        break;
    case 106:
        editApp->ShowSkinSelector();
        return true;
        break;
    }
    return false;
}

void peMenu::OnFileSelected::Execute(const csString & text)
{
    if ( text.Length() == 0 ) return;

    switch ( buttonID )
    {
    case 101:
        editApp->OpenWidget( text );
        break;
    case 102:
        editApp->OpenImageList( text );
        break;
    }
}

