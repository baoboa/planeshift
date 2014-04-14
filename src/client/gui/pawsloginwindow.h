/*
 * pawsloginwindow.h - Author: Andrew Craig
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

#ifndef PAWS_LOGIN_WINDOW
#define PAWS_LOGIN_WINDOW

#include <iutil/virtclk.h>
#include <csutil/csstring.h>
#include <csutil/parray.h>
#include "paws/pawswidget.h"
#include "paws/pawscheckbox.h"
#include "net/serverpinger.h"

class pawsEditTextBox;
class pawsTextBox;
class pawsListBox;
class pawsMultiLineTextBox;
    
class pawsLoginWindow : public pawsWidget
{
public:
    pawsLoginWindow();
    /// TODO: Copy constructor, useless currently. Would be implemented later.
    pawsLoginWindow(const pawsLoginWindow& origin){}
    bool PostSetup();
    void UpdateUserPasswdFromConfig();
    void Show();
    void Hide();
    void Draw();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    bool OnChange( pawsWidget* widget );
    void OnListAction( pawsListBox* selected, int status );
    
    /** This method notifies the LoginWindow that connection attempt failed */
    void ConnectionFailed();
    
private:

    bool LoadServerList();
    void SaveLoginInformation();
    
    void ConnectToServer(bool automatic = false);

    csString serverIP;
    int serverPort;
    pawsEditTextBox* login;
    pawsEditTextBox* passwd;    
    pawsListBox* listBox;
    pawsMultiLineTextBox* connectingLabel;
    pawsCheckBox* checkBox;
    pawsCheckBox* checkBoxC;

    /// Used to wait to see if there is a server running. 
    csTicks timeout;
    
    /// Used to see if we are waiting for a connection.
    bool connecting;
    bool passwdChanged;
    bool remember;
    bool convert;
    csString storedPasswd;
    csString storedPasswd256; ///< temporary variable for migration from md5 to sha256
    csPDelArray<psServerPinger> servers;
    
};

CREATE_PAWS_FACTORY( pawsLoginWindow );

#endif

