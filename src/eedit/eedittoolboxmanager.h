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

#ifndef EEDIT_TOOLBOX_MANAGER_HEADER
#define EEDIT_TOOLBOX_MANAGER_HEADER

#include "eedittoolbox.h"

class PawsManager;
class pawsWidget;

/**
 * \addtogroup eedit
 * @{ */

/**
 * A class that manages a group of toolbox windows.
 */
class EEditToolboxManager
{
public:
    EEditToolboxManager();
    ~EEditToolboxManager();

    /**
     * Updates all the toolboxes in the list.
     *
     * @param elapsed the time in milliseconds that has passed since last frame.
     */
    void UpdateAll(unsigned int elapsed);

    /**
     * Loads all the toolbox widgets.
     *
     * @param paws a pointer to a valid PawsManager
     * @return true on success, false otherwise.
     */
    bool LoadWidgets(PawsManager * paws);

    /**
     * Registers all the factories needed by the toolboxes.
     *
     * @return true on success, false otherwise.
     */
    bool RegisterFactories() const;

    /**
     * Counts the number of toolboxes in the list.
     *
     * @return the number of toolboxes.
     */
    size_t GetToolboxCount() const;

    /**
     * Grabs a toolbox of the given type.
     *
     * @param type the toolbox type.
     * @return the toolbox.
     */
    EEditToolbox * GetToolbox(size_t type) const;

    /**
     * Grabs the toolbox widget of the given toolbox type.
     *
     * @param type the toolbox type
     * @return the toolbox as a pawsWidget
     */
    pawsWidget * GetToolboxWidget(size_t type) const;
    
private:
    /// Array of available toolboxes.
    EEditToolbox * toolboxes[EEditToolbox::T_COUNT];

    /// Array of strings giving the file locations of the paws widgets for each toolbox.
    const char * widgetFiles[EEditToolbox::T_COUNT];

    /// Array of strings giving the name of the widget for each toolbox.
    const char * widgetNames[EEditToolbox::T_COUNT];
};

/** @} */

#endif

