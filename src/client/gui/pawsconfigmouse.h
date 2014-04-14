/*
 * pawsconfigmouse.h - Author: Andrew Robberts
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

#ifndef PAWS_CONFIG_MOUSE_HEADER
#define PAWS_CONFIG_MOUSE_HEADER

// CS INCLUDES
#include <csutil/array.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "pawsconfigwindow.h"
#include "paws/psmousebinds.h"

class psMouseBinds;

//////////////////////////////////////////////////////////////////////////////
//                        pawsConfigMouse
//////////////////////////////////////////////////////////////////////////////


class pawsConfigMouse : public pawsConfigSectionWindow
{
public:
    virtual ~pawsConfigMouse();
    
    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

     // from pawsWidget:
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

    virtual bool OnChange(pawsWidget* widget);
   
protected:

    bool LoadMouse(const char * fileName);
    
     /** Creates the tree that makes the whole GUI.
      * @return  success.
      */
     bool CreateTree();
     
    void CreateTreeWidgets(pawsTreeNode * subtreeRoot);
        // Adds widgets to all nodes in subtree with root 'subtreeRoot'
        //   -- command label, key combination label and button that opens FingeringWindow
    void SetActionLabels(pawsTreeNode * subtreeRoot);
    // Sets all labels that hold mouse events (in subtree with root 'subtreeRoot')

    pawsTree * tree;                    // the tree that makes whole window GUI
      
    psMouseBinds binds;
    
    /** Update the ingame state depending on the action that was edited.
      */
    void HandleEditedAction( csString& editedAction );
};




class pawsConfigMouseFactory : public pawsWidgetFactory
{
public:
    pawsConfigMouseFactory()
    {
        Register( "pawsConfigMouse" );
    }
    pawsWidget* Create()
    {
        return new pawsConfigMouse();
    }
};



#endif 


