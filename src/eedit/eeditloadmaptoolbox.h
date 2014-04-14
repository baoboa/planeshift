/*
 * Author: Andrew Robberts
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

#ifndef EEDIT_LOAD_MAP_TOOLBOX_WINDOW_HEADER
#define EEDIT_LOAD_MAP_TOOLBOX_WINDOW_HEADER

#include "eedittoolbox.h"
#include "paws/pawswidget.h"
#include "paws/pawsfilenavigation.h"
#include "eeditinputboxmanager.h"
#include <csutil/dirtyaccessarray.h>

class pawsButton;
class pawsEditTextBox;

/**
 * \addtogroup eedit
 * @{ */

/** This allows loading of a new map.
 */
class EEditLoadMapToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditLoadMapToolbox>
{
public:
    EEditLoadMapToolbox();
    virtual ~EEditLoadMapToolbox();

    /**
     * Sets the text for the map field.
     *
     * @param mapFile     The new map.
     */
    void SetMapFile(const char * mapFile);

    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);
    virtual size_t GetType() const;
    virtual const char * GetName() const;
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    
private:
    
    /** Callback class for the pawsFileNavigation dialog
     */
    class OnFileSelected : public iOnFileSelectedAction
    {
        private:
            pawsEditTextBox * textBox;
            
        public:
            /** Accepts a pointer to a valid pawsEditTextBox that will accept the selected file path.
             *   @param widget the pawsEditTextBox
             */
            OnFileSelected(pawsEditTextBox * widget) : textBox(widget)
            {}

            /** Destructor - does nothing atm.
             */
            virtual ~OnFileSelected() {}

            /** This is called when a file has been selected.
             *   @param text the selected file
             */
            virtual void Execute(const csString & text);
    };

    pawsEditTextBox * mapPath;
    pawsButton      * loadMap;
    pawsButton      * browseMap;
};

CREATE_PAWS_FACTORY(EEditLoadMapToolbox);

/** @} */

#endif 
