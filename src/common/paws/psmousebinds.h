/*
 * psmousebinds.h - Author: Andrew Robberts
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

#ifndef PAWS_MOUSE_BINDS_HEADER
#define PAWS_MOUSE_BINDS_HEADER

#include <csutil/ref.h>
#include <csutil/list.h>
#include <csutil/parray.h>
#include <csutil/array.h>
#include <iutil/event.h>
#include <csutil/parray.h>

#include <iutil/vfs.h>
#include <iutil/document.h>

struct iObjectRegistry;
struct iEvent;

/**
 * \addtogroup common_paws
 * @{ */

// structure to hold each action and mouse actions that is associated with it
struct psMouseBind
{
    csString          action;
    csMouseEventData  event;
};

// structure to hold boolean mouse settings
struct psMouseOnOff
{
    csString option;
    bool     value;
};

// structure to hold integer mouse settings
struct psMouseInt
{
    csString option;
    int value;
};

/**
 * psMouseBinds holds set of psMouseAction
 */
class psMouseBinds
{
public:
    psMouseBinds() {}

    /**
     * Loads the mouse actions from a file.
     *
     * @param object_reg The object registry
     * @param filename the vfs filename and path
     * @return true on success
     **/
    bool LoadFromFile(iObjectRegistry* object_reg, const csString &filename);


    /**
     * Saves the mouse actions to a file.
     *
     * @param object_reg The object registry
     * @param filename the vfs filename and path
     * @return true on success
     **/
    bool SaveToFile(iObjectRegistry* object_reg, const csString &filename);


    /**
     * Binds a mouse event to an action.
     *
     * @param action the action that will be bound
     * @param event the mouse event that will trigger the action
     **/
    void Bind(const csString &action, csMouseEventData &event);

    void Bind(const csString &action, csString &event, csString &ctrl);
    void Bind(const csString &action, int button, int modifier);


    /**
     * Sets the boolean setting of the specified option.
     *
     * @param option the name of the option to set
     * @param value the value to set the option to
     **/
    void SetOnOff(const csString &option, csString &value);
    void SetOnOff(const csString &option, bool value);


    /**
     * Sets the integer setting of the specified option.
     *
     * @param option the name of the option to set
     * @param value the value to set the option to
     **/
    void SetInt(const csString &option, csString &value);
    void SetInt(const csString &option, int value);


    /**
     * Gets the mouse event that triggers the specified action.
     *
     * @param action the action to query
     * @param event a variable that will hold the event
     * @return true if the action was found
     **/
    bool GetBind(const csString &action, csMouseEventData &event);
    bool GetBind(const csString &action, csString &button);

    /**
     * Checks if a specific bind matches the given button and modifiers.
     *
     * @param action the action to query.
     * @param button the button that you want to check against.
     * @param modifiers the modifiers that you want to check against.
     * @return true if the action matches the given button and modifiers.
     */
    bool CheckBind(const csString &action, int button, int modifiers);

    /**
     * Gets the boolean value of the specified option.
     *
     * @param option the option to query
     * @param value a variable that will hold the option value
     * @return true if the option was found
     **/
    bool GetOnOff(const csString &option, csString &value);
    bool GetOnOff(const csString &option, bool &value);


    /**
     * Gets the integer value of the specified option.
     *
     * @param option the option to query
     * @param value a variable that will hold the option value
     * @return true if the option was found
     **/
    bool GetInt(const csString &option, csString &value);
    bool GetInt(const csString &option, int &value);


    /**
     * Removes an action.
     *
     * @param action the action to remove
     **/
    void Unbind(const csString &action);


    /**
     * Removes an OnOff option.
     *
     * @param option the option to remove
     **/
    void RemoveOnOff(const csString &option);


    /**
     * Removes an int option.
     *
     * @param option the option to remove
     **/
    void RemoveInt(const csString &option);


    /**
     * Removes all actions/options.
     *
     **/
    void UnbindAll();

    static csString MouseButtonToString(uint button, uint32 modifiers);
    static bool StringToMouseButton(const csString &str, uint &button, uint32 &modifiers);

protected:

    /**
     * Finds the action in the list.
     *
     * @param action the action to search for
     * @return NULL iterator if action isn't found
     **/
    psMouseBind* FindAction(const csString &action);


    /**
     * Finds the OnOff option in the list.
     *
     * @param option the option to search for
     * @return NULL iterator if option isn't found
     **/
    psMouseOnOff* FindOnOff(const csString &option);


    /**
     * Finds the int option in the list.
     *
     * @param option the option to search for
     * @return NULL iterator if option isn't found
     **/
    psMouseInt* FindInt(const csString &option);

    /// list of the binds
    csPDelArray<psMouseBind> binds;
    csPDelArray<psMouseOnOff> boolOptions;
    csPDelArray<psMouseInt> intOptions;
};

/** @} */

#endif


