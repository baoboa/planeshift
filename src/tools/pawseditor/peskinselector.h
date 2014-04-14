/*
* peskinselector.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings [Borrillis] <cummings.michael@gmail.com>
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
* Creation Date: 6/23/2005
* Description : new widget to select skin
*
*/

#ifndef PAWS_SKIN_SELECTOR_WINDOW
#define PAWS_SKIN_SELECTOR_WINDOW

#include <iutil/vfs.h>
#include <csutil/ref.h>
#include "paws/pawswidget.h"
#include <iutil/cfgmgr.h>

class pawsComboBox;
class pawsButton;
class pawsMultiLineTextBox;
class pawsCheckBox;

class peSkinSelector : public pawsWidget
{
public:
    ~peSkinSelector();
    
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* widget, int status );
    bool PostSetup();

    // Used to load a resource from the new skin
    bool PreviewResource(const char* resource,const char* resname,const char* mountPath);

    void PreviewSkin(const char* name);
private:

    csString mountedPath;
    csString currentSkin;

    csRef<iVFS> vfs;
    csRef<iConfigManager> config;

    pawsComboBox* skins;
    pawsMultiLineTextBox* desc;
    pawsWidget* preview; // The preview widget
    pawsButton* previewBtn; // The preview button
    pawsCheckBox* previewBox; // The preview checkbox
};

CREATE_PAWS_FACTORY( peSkinSelector );

#endif


