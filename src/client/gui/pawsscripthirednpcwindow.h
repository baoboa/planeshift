/*
 * pawsscripthirednpcwindow.h  creator <andersr@pvv.org>
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_SCRIPT_HIRED_NPC_WSINOW_HEADER
#define PAWS_SCRIPT_HIRED_NPC_WSINOW_HEADER

//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================
#include "net/cmdbase.h"
#include "net/messages.h"

#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawsstringpromptwindow.h"

//====================================================================================
// Local Includes
//====================================================================================

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------


/** A window that allows scripting of hired NPCs.
 *
 *  Communicate with the HireManager in the Server.
 */
class pawsScriptHiredNPCWindow : public pawsWidget, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:
    /** Constructor.
     */
    pawsScriptHiredNPCWindow();

    /** Destructor.
     */
    virtual ~pawsScriptHiredNPCWindow();

    /** Do post setup of the window.
     */
    virtual bool PostSetup();

    /** Handle reception of subscribed messages.
     */
    void HandleMessage(MsgEntry* me);

    /** Handle the script message.
     */
    void HandleHiredNPCScript(const psHiredNPCScriptMessage &msg);

    /** Handle button pressed.
     */
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

    /** Handle enter string dialog result.
     */
    virtual void OnStringEntered(const char* name, int param,const char* value);

    /** Called when some of the chiled parents change.
     */
    virtual bool OnChange(pawsWidget* widget);

    /** Set Verified status on dialog.
     *
     *  Update the verified status, ok button and verify button avalability.
     */
    void SetVerified(bool status);

protected:
private:
    bool                         verified; // Set to true when script is verified ok.
    pawsMultilineEditTextBox*    script;
    pawsButton*                  verifyButton;
    pawsButton*                  okButton;
    EID                          hiredEID;
};

CREATE_PAWS_FACTORY(pawsScriptHiredNPCWindow);

#endif
