/*
* peMenu.h
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
* Description : menu widget for pawseditor
*
*/

#ifndef PAWS_EDITOR_MENU_HEADER
#define PAWS_EDITOR_MENU_HEADER

#include "paws/pawswidget.h"
#include "paws/pawsfilenavigation.h"

struct iView;
struct iVFS;

class pawsEditTextBox;
class pawsComboBox;
class pawsButton;
class pawsGenericView;

class peMenu : public pawsWidget
{
public:
    // implemented virtual functions from pawsWidgets
    bool PostSetup();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

private:

    /**
     * Callback class for the pawsFileNavigation dialog.
     */
    class OnFileSelected : public iOnFileSelectedAction
    {
        private:
            peMenu *parent;
            int buttonID;

        public:
            /**
             * Accepts a pointer to a valid pawsEditTextBox that will accept the selected file path.
             *
             * @param Parent The parent.
             * @param button The button.
             */
            OnFileSelected( peMenu *Parent, int button ) { parent = Parent; buttonID = button; }

            /**
             * Destructor - does nothing atm.
             */
            virtual ~OnFileSelected( ) { }

            /**
             * This is called when a file has been selected.
             *
             * @param text the selected file
             */
            virtual void Execute(const csString & text);

    };

};

CREATE_PAWS_FACTORY( peMenu );

#endif
