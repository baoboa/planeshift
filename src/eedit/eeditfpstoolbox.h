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



#ifndef EEDIT_FPS_TOOLBOX_WINDOW_HEADER
#define EEDIT_FPS_TOOLBOX_WINDOW_HEADER



#include "eedittoolbox.h"

#include "paws/pawswidget.h"



class pawsTextBox;

class pawsSpinBox;



/**
 * \addtogroup eedit
 * @{ */

/**
 * This handles the displaying of FPS and controlling the cap.
 */
class EEditFPSToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditFPSToolbox>
{
public:
    EEditFPSToolbox();

    virtual ~EEditFPSToolbox();



    /**
     * Retrieves the current FPS.
     *
     *  @return    The current FPS.
     */
    float GetFPS() const;

    /**
     * Retrieves the current target FPS.
     *
     * @return    The current target FPS.
     */
    float GetTargetFPS() const;

    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);

    virtual size_t GetType() const;

    virtual const char * GetName() const;

    

    // inheritted from pawsWidget
    virtual bool PostSetup(); 

    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

private:

    pawsTextBox * fpsDisplay;

    pawsSpinBox * fpsTarget;

    float fps;

    unsigned int framesCount;

    unsigned int framesElapsed;

};

CREATE_PAWS_FACTORY(EEditFPSToolbox);

/** @} */

#endif 

