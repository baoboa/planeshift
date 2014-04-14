/*
 * pawsconfigmarriage.h
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_MARRIAGE_HEADER
#define PAWS_CONFIG_MARRIAGE_HEADER

// CS INCLUDES
#include <csutil/array.h>
#include <iutil/document.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "util/psxmlparser.h"


class pawsRadioButtonGroup;

/**
 * Currently only used to provide an option to always deny marriage invites.
 * @brief Options screen for configuring marriages
 */
class pawsConfigMarriage : public pawsConfigSectionWindow
{
public:
    //from pawsWidget:
    /**
     * Assign confirmRadioGroup appropriately
     * @return False if radio group was not found in XML, True otherwise
     */
    virtual bool PostSetup();

    /**
     * Called whenever a child widget changes.
     * If the widget is the radio group this window becomes dirty
     * @return True
     */
    virtual bool OnChange(pawsWidget * widget);

    // from pawsConfigSectionWindow:
    /**
     * Loads XML layout from configmarriage.xml
     * @return True if the XML successfully loaded, false otherwise
     */
    virtual bool Initialize();

    /**
     * Sets radio groups settings according to user settings
     * @return True
     */
    virtual bool LoadConfig();

    /**
     * Saves the settings to engine and file
     * @return True
     */
    virtual bool SaveConfig();

    /**
     * Sets the confirmation to allow marriage proposals rather than ignoring all
     */
    virtual void SetDefault();

protected:
    pawsRadioButtonGroup * confirmRadioGroup;
};

CREATE_PAWS_FACTORY(pawsConfigMarriage);

#endif

