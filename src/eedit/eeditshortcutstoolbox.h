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

#ifndef EEDIT_SHORTCUTS_TOOLBOX_WINDOW_HEADER
#define EEDIT_SHORTCUTS_TOOLBOX_WINDOW_HEADER

#include <csutil/array.h>
#include "eedittoolbox.h"
#include "paws/pawswidget.h"

class pawsListBox;

/**
 * \addtogroup eedit
 * @{ */

struct EEditShortcutKey
{
    EEditShortcutKey(const char * _command, int _key, int _modifiers)
        : command(_command), key(_key), modifiers(_modifiers)
    {
    }

    csString command;
    int key;
    int modifiers;
};

/** This manages the shortcuts.
 */
class EEditShortcutsToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditShortcutsToolbox>
{
public:
    EEditShortcutsToolbox();
    virtual ~EEditShortcutsToolbox();

    /** Adds a shortcut command to the list.
     *   \param name    The name of the shortcut.
     */
    void AddShortcut(const char * name);

    /** Executes all commands associated with the given key+modifiers combination.
     *   \param key         The keycode of the main key.
     *   \param modifiers   The modifiers.
     */
    void ExecuteShortcutCommand(int key, int modifiers=0) const;

    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);
    virtual size_t GetType() const;
    virtual const char * GetName() const;
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual void Show();
    virtual void OnListAction(pawsListBox * selected, int status);

private:

    /** Saves shortcuts from the widget list to the actual shortcuts list.
     */
    void SaveShortcuts();

    /** Applies the actual shortcuts list values to the widget list.
     */
    void RedrawShortcuts();

    pawsListBox *   shortcutsList;
    pawsButton *    applyButton;
    pawsButton *    cancelButton;
    pawsButton *    okButton;

    csArray<EEditShortcutKey>   shortcuts;
};

CREATE_PAWS_FACTORY(EEditShortcutsToolbox);

/** @} */

#endif 
