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

#ifndef EEDIT_POSITION_TOOLBOX_WINDOW_HEADER
#define EEDIT_POSITION_TOOLBOX_WINDOW_HEADER

#include "eedittoolbox.h"
#include "paws/pawswidget.h"
#include "paws/pawsfilenavigation.h"
#include "eeditinputboxmanager.h"
#include <csutil/dirtyaccessarray.h>

struct iSpriteCal3DState;
class pawsButton;
class pawsEditTextBox;
class pawsSpinBox;
class pawsRadioButton;

/**
 * \addtogroup eedit
 * @{ */
/** This handles the effect position object.
 */
class EEditPositionToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditPositionToolbox>
{
public:
    EEditPositionToolbox();
    virtual ~EEditPositionToolbox();

    /** Sets the path to the position's mesh indicator, to be filled in the edit box.
     *   @param file the file path to fill in the mesh edit box.
     */
    void SetMeshFile(const csString & file);

    /** Fills the animation list with the cal3d animations.
     *   @param cal3d the cal3d state to get the anims from.
     */
    void FillAnimList(iSpriteCal3DState * cal3d);

    /** Sets the position of the actor.
     *  \param pos  The new position.
     */
    void SetPosition(const csVector3 & pos);

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

    /** Callback class for the anim selection dialog
     */
    class OnAnimSelected : public EEditInputboxManager::iSelectList
    {
        public: 
            /** Constructor - does nothing
             */
            OnAnimSelected() {}

            /** Destructor - does nothing
             */
            virtual ~OnAnimSelected() {}

            virtual void Select(const csString & value, size_t index);
    };
    
    pawsButton      * moreLessButton;
    pawsButton      * animButton;
    pawsButton      * browseMesh;
    pawsEditTextBox * meshFileText;
    pawsSpinBox     * posX;
    pawsSpinBox     * posY;
    pawsSpinBox     * posZ;
    pawsSpinBox     * rot;
    pawsRadioButton * posTypeNone;
    pawsRadioButton * posTypeAxis;
    pawsRadioButton * posTypeCustom;

    csVector3 prevPos;
    float     prevRot;

    csDirtyAccessArray<csString> anims;
};

CREATE_PAWS_FACTORY(EEditPositionToolbox);

/** @} */

#endif 
