/*
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

#ifndef ECONTROLMANAGER_H
#define ECONTROLMANAGER_H

#include <csutil/csstring.h>
#include <iutil/event.h>
#include <csutil/array.h>

/**
 * \addtogroup eedit
 * @{ */

#define CMD_DECLARATION(func)   static void func(eControlManager *obj, bool down)

struct iObjectRegistry;

class eControlManager 
{
public:
    /**
     * Initialise the eControlManager with an object registry.
     *
     * The object registry will be searched for the keymap files and etc.
     */
    eControlManager(iObjectRegistry* object_reg);
    ~eControlManager();

    /**
     * Handles the given even if it is a key even.
     *
     * @param event  The event to handle.
     * @return True if the event has been handled, false otherwise.
     */
    bool HandleEvent(iEvent &event);

    /**
     * Imports key mappings from the given files.
     *
     * @note Note that this adds to the current mappings.
     * @param filename  Name of the file with the mappings to import.
     * @return Returns true upon successfully loading the file, false otherwises.
     */
    bool LoadKeyMap(const char* filename);

    /** Removes all current key mappings
     */
    void ClearKeyMap();

    // Actions
    CMD_DECLARATION( HandleForward      );
    CMD_DECLARATION( HandleBackward     );
    CMD_DECLARATION( HandleRotateLeft   );
    CMD_DECLARATION( HandleRotateRight  );
    CMD_DECLARATION( HandleLookUp       );
    CMD_DECLARATION( HandleLookDown     );

private:
    /**
     * Executes the command mapped to the given key, if one exists.
     *
     * @param key       A csKeyEveentData structure describing the key.
     */
    void ExecuteKeyCommand(csKeyEventData &key);

    /**
     * Adds a mapping between the given action and key to the keymap.
     *
     * @param action    Name of the action.
     * @param data      csKeyEventData describing the key.
     */
    void Map(const char *action, csKeyEventData &data);

    /**
     * Finds the index of the action with the given name in the keyMap array.
     *
     * @param name      Name of the action to search for.
     * @return    Returns the index of the action or -1 if not found.
     */
    size_t StringToAction(const char *name);

    /**
     * Holds the map between a key and its action.
     */
    struct ActionKeyMap
    {
        csString action;        ///< Action name
        csKeyEventData csKey;   ///< Key
    };

    csArray<ActionKeyMap*>  keyMap; ///< An array of mappings between keys and actions 


    iObjectRegistry*    object_reg;
};

/** @} */

#endif
