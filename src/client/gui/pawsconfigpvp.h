/*
 * pawsconfigpvp.h - Author: Christian Svensson
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

#ifndef PAWS_CONFIG_PVP_HEADER
#define PAWS_CONFIG_PVP_HEADER

// CS INCLUDES
#include <csutil/array.h>
#include <iutil/document.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "util/psxmlparser.h"


class pawsRadioButtonGroup;

/**
 * class pawsConfigPvP is options screen for configuration of PvP
 */
class pawsConfigPvP : public pawsConfigSectionWindow
{
public:
    //from pawsWidget:
    virtual bool PostSetup();
    virtual bool OnChange(pawsWidget * widget);
    
    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();
    
protected:

    pawsRadioButtonGroup * confirmRadioGroup;
};


CREATE_PAWS_FACTORY(pawsConfigPvP);


#endif 

