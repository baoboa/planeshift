/*
 * Author: Andrew Craig
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_BUDDY_HEADER
#define PAWS_BUDDY_HEADER

#include "paws/pawswidget.h"
#include "net/subscriber.h"

#include "paws/pawsstringpromptwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"

class pawsListBox;


/** Prefix for the files which contain the aliases for the buddies.
 *  The format is aliases_charname.xml and it's used to substituite the
 *  names in the buddy list with user choices.
 */
#define ALIASES_FILE_PREFIX       "/planeshift/userdata/aliases_"


/** The buddy window that shows your current list of in game 'friends'.  
 *  This allows you to send them a tell or add/remove buddies.
 */
class pawsBuddyWindow : public pawsControlledWindow, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:
    pawsBuddyWindow();

    bool PostSetup();
    
    /** Handles the messages this widget has subscribed to.
     *  @param me: msgentry containing the message
     */
    void HandleMessage( MsgEntry* me );

    bool OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* widget, int status );
    void OnStringEntered(const char *name,int param,const char *value);

    virtual void Show();

    virtual void OnResize();

private:
    ///pointer to the listbox widget used for the buddy list
    pawsListBox* buddyList;  

    /// Name of the currently selected buddy (NB! alias, not the real name)
    csString currentBuddy;
    
    /// Pointer to the chat window widget, used to subscribe the names in there
    pawsChatWindow* chatWindow;

    /// Real name of the buddy that is being edited
    csString editBuddy;

    /// List of the buddies who are online. Used to populate the buddy list.
    csStringArray onlineBuddies;
    /// List of the buddies who are offline. Used to populate the buddy list.
    csStringArray offlineBuddies;

    /// Alias/name table
    csHash<csString, csString> aliases;

    /** Returns the alias for the name or the name itself if there is no alias for it.
     *  
     *  @param name The name which will be searched for an alias.
     *  @return The same name if it wasn't found else the alias of it.
     */
    csString GetAlias(const csString & name) const;

    /** Reverse (slow) search for the real name.
     * 
     *  @param alias The alias of which we are searching the original name.
     *  @return the real name if found else the provided alias.
     */
    csString GetRealName(const csString & alias) const;

    /** Loads aliases from the xml file.
     *  
     * @param charName The name of the current player's character.
     */
    void LoadAliases(const csString & charName);

    /** Saves aliases to the xml file.
     * 
     *  @param charName The name of the current player's character.
     */
    void SaveAliases(const csString & charName) const;

    /** Changes the alias for the given name.
     *  
     *  @param name The name of the buddy.
     *  @param oldAlias The previous alias of the buddy.
     *  @param newAlias The new alias of the buddy.
     */
    void ChangeAlias(const csString & name, const csString & oldAlias, const csString & newAlias);

    /**
     * Populates the buddy listbox with names from online and offline buddy list arrays.
     *
     * Clears the current buddy list, sorts names and then re-populates the listbox with names
     * from both buddy lists. Also clears the currently selected buddy in currentBuddy.
     */
    void FillBuddyList();

    /**
     * Verifies that the alias is unique
     * 
     * @param alias: The alias to be verified for uniqueness.
     * @return True if the alias is unique.
     */
    bool IsUniqueAlias(const csString & alias) const;

};


CREATE_PAWS_FACTORY( pawsBuddyWindow );
#endif    
