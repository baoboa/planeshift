/*
 * pawsconfigspellchecker.h - Author: Fabian Stock (AiwendilH@googlemail.com)
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_SPELLCHECKER_HEADER
#define PAWS_CONFIG_SPELLCHECKER_HEADER

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigwindow.h"


class pawsChatWindow;

class pawsConfigSpellChecker : public pawsConfigSectionWindow
{
public:
       pawsConfigSpellChecker();

    //from pawsWidget:
       virtual bool PostSetup();

    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

       // from pawsWidget
       bool OnChange(pawsWidget* /*widget*/) { dirty = true; return true; }
       virtual bool OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* /*widget*/)
       {
           dirty = true;
           return true;
       }
       virtual void OnListAction(pawsListBox* /*selected*/, int /*status*/) {dirty = true;};

private:
       /**
        * Helper function which checks if a word already exists in the personal dictionary.
        * 
        * @param spellChecker Reference to the object handling the spell checking.
        * @param word The word to search for in the personal dictionary.
        * @return TRUE if the word was found.
        */
       bool WordExists(csRef<iSpellChecker> spellChecker, csString word);
       pawsChatWindow* chatWindow;
       // needed gui elemets
       pawsMultilineEditTextBox* personalDictBox;
       pawsCheckBox* enabled;
       pawsEditTextBox* colorR;
       pawsEditTextBox* colorG;
       pawsEditTextBox* colorB;
};

CREATE_PAWS_FACTORY( pawsConfigSpellChecker );

#endif //PAWS_CONFIG_SPELLCHECKER_HEADER
