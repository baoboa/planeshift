/*
 * Author: Christian Svensson
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_NPC_DIALOG
#define PAWS_NPC_DIALOG

#include "net/subscriber.h"
#include "pscelclient.h"

#include "paws/pawslistbox.h"
#include "paws/pawswidget.h"
#include "paws/pawsstringpromptwindow.h"

#define CONFIG_NPCDIALOG_FILE_NAME       "/planeshift/userdata/options/npcdialog.xml"
#define CONFIG_NPCDIALOG_FILE_NAME_DEF   "/planeshift/data/options/npcdialog_def.xml"


class pawsListBox;

/** This window shows the popup menu of available responses
 *  when talking to an NPC.
 */
class pawsNpcDialogWindow: public pawsWidget, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:
    struct QuestInfo
    {
        csString    title;
        csString    text;
        csString    trig;
    };
    pawsNpcDialogWindow();

    bool PostSetup();
    void HandleMessage(MsgEntry* me);

    void OnListAction(pawsListBox* widget, int status);

    void OnStringEntered(const char *name,int param,const char *value);

    /**
     * Loads the settings from the config files and sets the window
     * appropriately.
     * @return TRUE if loading succeded
     */
    bool LoadSetting();

    /**
     * Shows the window only if it's in bubbles mode, else does nothing.
     */
    void ShowIfBubbles();

    /**
     * empties all bubbles structures and hides them.
     */
    void CleanBubbles();

    /**
     * Hides all bubbles except freetext and bye.
     */
    void ShowOnlyFreeText();

    /**
     * Shows the window and applies some special handling to fix up the window
     * Behaviour and graphics correctly depending if we use the classic menu
     * or the bubble menu
     */
    virtual void Show();
    virtual void Hide();
    bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);
    bool OnMouseDown(int button, int modifiers, int x , int y);
    bool OnButtonPressed(int button, int keyModifier, pawsWidget* widget);

    // This window should always stay on the background
    virtual void BringToTop(pawsWidget* widget) {};

    /**
     * @brief Load quest info from xmlbinding message
     *
     * @param xmlstr An xml string which contains quest info
     *
     */
    void LoadQuest(csString xmlstr);

    /**
     * @brief Display quests in bubbles.
     *
     * @param index From which index in questInfo array the quest info will be displayed in bubbles.
     *
     */
    void DisplayQuestBubbles(unsigned int index);

    /**
     * @brief Display NPC's chat text
     *
     * @param inText Content that the NPC's chat text
     * @param actor The target NPC name
     */
    void NpcSays(csString& inText, GEMClientActor *actor);

    /**
     * Handles timing to make bubbles disappear in the bubbles npc dialog mode.
     */
    virtual void Draw();

    /**
     * Has a real functionality only when using the bubbles npc dialog.
     * It will avoid drawing the background in order to make it transparent
     */
    virtual void DrawBackground();

    /**
     * Getter for useBubbles, which states if we use the bubbles based
     * npcdialog interface.
     * @return TRUE if we are using the bubbles based npc dialog interface
     *         FALSE if we are using the menu based npc dialog interface
     */
    bool GetUseBubbles()
    {
        return useBubbles;
    }
    /**
     * Sets if we have to use the bubbles based npc dialog interface (true)
     * or the classic menu based one (false).
     * @note This doesn't reconfigure the widgets for the new modality.
     */
    void SetUseBubbles(bool useBubblesNew)
    {
        useBubbles = useBubblesNew;
    }


    /**
     * Saves the setting of the menu used for later use
     */
    void SaveSetting();

    /**
     * Sets the widget appropriate for display depending on the type of
     * npc dialog menu used
     */
    void SetupWindowWidgets();


private:
    void AdjustForPromptWindow();
    /**
     * Handles the display of player text in chat
     */
    void DisplayTextInChat(const char *sayWhat);

    bool useBubbles; ///< Stores which modality should be used for the npcdialog (bubbles/menus)

    csArray<QuestInfo> questInfo;  ///< Stores all the quest info and triggers parsed from xml binding.
    unsigned int    displayIndex;  ///< Index to display which quests
    int             cameraMode;    ///< Stores the camera mode
    int             loadOnce;      ///< Stores if bubbles has been loaded
    bool enabledChatBubbles;       ///< Stores the state of chat bubbles.
    bool clickedOnResponseBubble;  ///< flag when player clicks on the response bubble
    bool gotNewMenu;               ///< keeps track of the incoming new menu message
    csTicks timeDelay;                 ///< calculates the time needed to read the last npc say
    int questIDFree;               ///< Keeps the value of the quest if the free text question was triggered.

    pawsListBox* responseList;
    pawsWidget* speechBubble;
    pawsEditTextBox* textBox;
    pawsButton* closeBubble;
    csTicks         ticks;
    EID targetEID; ///< The eid of the current target used to hide the dialog if the actor is removed.
};


CREATE_PAWS_FACTORY(pawsNpcDialogWindow);
#endif
