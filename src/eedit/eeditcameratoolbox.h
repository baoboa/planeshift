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

#ifndef EEDIT_CAMERA_TOOLBOX_HEADER
#define EEDIT_CAMERA_TOOLBOX_HEADER

#include "eedittoolbox.h"
#include "paws/pawswidget.h"

class pawsButton;

/**
 * \addtogroup eedit
 * @{ */

/**
 * This handles the camera controls toolbox.
 */
class EEditCameraToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditCameraToolbox>
{
public:
    EEditCameraToolbox();
    virtual ~EEditCameraToolbox();

    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);
    virtual size_t GetType() const;
    virtual const char * GetName() const;

    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnButtonReleased(int mouseButton, pawsWidget* widget);
    
private:
    pawsButton * camLeft;
    pawsButton * camRight;
    pawsButton * camUp;
    pawsButton * camDown;
    pawsButton * camForward;
    pawsButton * camBack;
};

CREATE_PAWS_FACTORY(EEditCameraToolbox);

/** @} */

#endif 
