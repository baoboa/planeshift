/*
 * pawsconfigentityinter.h - Author: Ondrej Hurt
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

#ifndef PAWS_CONFIG_ENTITY_INTER_HEADER
#define PAWS_CONFIG_ENTITY_INTER_HEADER

// CS INCLUDES
#include <csutil/array.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "gui/psmainwidget.h"
#include "pawsconfigwindow.h"

class pawsComboBox;
class psEntityType;

/**
 * class pawsConfigEntityInteraction is option screen for configuration 
 * of player's interaction with entities 
 */
class pawsConfigEntityInteraction : public pawsConfigSectionWindow
{
public:
    pawsConfigEntityInteraction();

    //from pawsWidget:
    virtual bool PostSetup();
    virtual void OnListAction( pawsListBox* selected, int status );
    
    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

protected:
    psEntityTypes * entTypes;
};


CREATE_PAWS_FACTORY(pawsConfigEntityInteraction);


#endif 

