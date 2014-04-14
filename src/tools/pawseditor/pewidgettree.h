/*
* widgettree.h
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
* Description : control tree for properties editing
*
*/

#ifndef PAWS_EDITOR_WIDGETTREE_HEADER
#define PAWS_EDITOR_WIDGETTREE_HEADER

#include "paws/pawswidget.h"
#include "paws/pawsfilenavigation.h"
#include "paws/pawstree.h"

class pawsEditTextBox;
class pawsButton;

class peWidgetTree : public pawsWidget
{
public:

    void LoadWidget(iDocumentNode* widgetNode);

    // implemented virtual functions from pawsWidgets
    bool PostSetup();
    virtual bool OnSelected( pawsWidget* widget);

private:

    void BuildWidgetTree(pawsSimpleTreeNode *parent, iDocumentNode* widgetNode);

    pawsTree * widgetItemsTree;
    pawsSimpleTreeNode * widgetItemsRoot;

};

CREATE_PAWS_FACTORY( peWidgetTree );

#endif
