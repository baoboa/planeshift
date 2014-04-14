/*
 * pawsconfigdetails.h - Author: Ondrej Hurt
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

#ifndef PAWS_CONFIG_DETAILS_HEADER
#define PAWS_CONFIG_DETAILS_HEADER

// CS INCLUDES
#include <csutil/array.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawscrollbar.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigwindow.h"



class pawsConfigDetails : public pawsConfigSectionWindow
{
public:
    // from pawsWidget:
    virtual bool OnScroll( int scrollDirection, pawsScrollBar* widget );
    virtual bool OnButtonPressed( int button, int keyModifier, pawsWidget* widget );
    virtual bool PostSetup();

    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();
    
protected:

    pawsCheckBox  * adaptCheck;

    pawsWidget * adaptConfig,
               * fixedConfig;

    pawsScrollBar * distScroll,
                  * minDistScroll,
                  * minFPSscroll,
                  * maxFPSscroll,
                  * capFPSscroll;
    pawsEditTextBox * distvalue,
                    * minDistvalue,
                    * minFPSvalue,
                    * maxFPSvalue,
                    * capFPSvalue;

    int GetValue(const csString & name);
    void ConstraintScrollBars();
    void SetScrollValue(const csString & name, int val);
    bool OnChange(pawsWidget* widget);
    void SetModeVisibility();
};


CREATE_PAWS_FACTORY(pawsConfigDetails);


#endif 


