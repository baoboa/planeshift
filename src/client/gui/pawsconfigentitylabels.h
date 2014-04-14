/*
 * pawsconfigentitylabels.h - Author: Ondrej Hurt
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

#ifndef PAWS_CONFIG_ENTITY_LABELS_HEADER
#define PAWS_CONFIG_ENTITY_LABELS_HEADER

// CS INCLUDES
#include <csutil/array.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "paws/pawscolorpromptwindow.h"

class pawsTree;
class pawsCheckBox;
class pawsRadioButtonGroup;
/**
 * class pawsConfigEntityLabels is options screen for configuration of entity labels (see client/entitylabels.h)
 */
class pawsConfigEntityLabels : public pawsConfigSectionWindow, public iOnColorEnteredAction
{
public:
    virtual ~pawsConfigEntityLabels();
    ///from pawsWidget:
    virtual bool OnChange(pawsWidget * widget);
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    
    /// from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

    ///from iOnColorEnteredAction (set color to param-identified label)
    virtual void OnColorEntered(const char *name,int param,int color);
protected:

    /** Creates the tree that makes the whole GUI.
      * @return success
      */
    bool CreateTree();
    /** Adds widgets to all nodes in subtree with root 'subtreeRoot'.
      * -- color label and button that opens colorPicker    
      */
    pawsTree *              tree;               /// the tree that makes whole window GUI
    pawsColorPromptWindow * colorPicker;        /// pointer to colorPicker window
    pawsCheckBox*           visGuildCheck;      /// check box to select visibility of guild
    pawsRadioButtonGroup*   ItemRBG;            /// Radio button group for set items labels visibility
    pawsRadioButtonGroup*   CreatureRBG;        /// Radio button group for set creatures labels visibility


    int             labelColors[ENTITY_TYPES_AMOUNT];                           /// array of entity labels colors
    int             defLabelColors[ENTITY_TYPES_AMOUNT];  ///array of default entity labels colors
    psEntityLabels * entityLabels;                         /// entity labels properties
};


CREATE_PAWS_FACTORY(pawsConfigEntityLabels);


#endif 

