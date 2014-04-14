/*
 * cmdguilds.cpp - Author: Keith Fulton
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

#include "util/strutil.h"

#include "paws/pawsmanager.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawscheckbox.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "cmdguilds.h"
#include "globals.h"



psGuildCommands::psGuildCommands( ClientMsgHandler* mh,
                                  CmdHandler* ch,
                                  iObjectRegistry* obj )
  : psCmdBase(mh,ch,obj)
{
    cmdsource->Subscribe("/guildinfo",this);    // displays
    cmdsource->Subscribe("/newguild",this);     // create new guild
    cmdsource->Subscribe("/endguild",this);     // disband guild
    cmdsource->Subscribe("/guildinvite",this);  // ask player to join guild
    cmdsource->Subscribe("/guildremove",this);  // remove player from guild
    cmdsource->Subscribe("/guildlevel",this);   // name guild level something (ranks)
    cmdsource->Subscribe("/guildpromote",this); // promote player in guild to level #
    cmdsource->Subscribe("/getmemberpermissions",this); // pgets the permissions of a particular player
    cmdsource->Subscribe("/setmemberpermissions",this); // sets the permissions of a particular player
    cmdsource->Subscribe("/guildmembers",this); // see list of members (optional level #)
    cmdsource->Subscribe("/guildpoints",this);  // view "karma points" of named guild
    cmdsource->Subscribe("/guildname",this);  // view "karma points" of named guild
    cmdsource->Subscribe("/guildsecret", this);
    cmdsource->Subscribe("/guildweb", this);
    cmdsource->Subscribe("/guildmotd", this);
    cmdsource->Subscribe("/guildwar", this);
    cmdsource->Subscribe("/guildyield", this);

    cmdsource->Subscribe("/newalliance",this);
    cmdsource->Subscribe("/allianceinvite",this);
    cmdsource->Subscribe("/allianceremove",this);
    cmdsource->Subscribe("/allianceleave",this);
    cmdsource->Subscribe("/allianceleader",this);
    cmdsource->Subscribe("/endalliance",this);
}

psGuildCommands::~psGuildCommands()
{
    cmdsource->Unsubscribe("/guildinfo",this);
    cmdsource->Unsubscribe("/newguild",this);
    cmdsource->Unsubscribe("/endguild",this);
    cmdsource->Unsubscribe("/guildinvite",this);
    cmdsource->Unsubscribe("/guildremove",this);
    cmdsource->Unsubscribe("/guildlevel",this);
    cmdsource->Unsubscribe("/guildpromote",this);
    cmdsource->Unsubscribe("/getmemberpermissions",this);
    cmdsource->Unsubscribe("/setmemberpermissions",this);
    cmdsource->Unsubscribe("/guildmembers",this);
    cmdsource->Unsubscribe("/guildpoints",this);
    cmdsource->Unsubscribe("/guildname", this);
    cmdsource->Unsubscribe("/guildsecret", this);
    cmdsource->Unsubscribe("/guildweb", this);
    cmdsource->Unsubscribe("/guildmotd", this);
    cmdsource->Unsubscribe("/guildwar", this);
    cmdsource->Unsubscribe("/guildyield", this);

    cmdsource->Unsubscribe("/newalliance", this);
    cmdsource->Unsubscribe("/allianceinvite", this);
    cmdsource->Unsubscribe("/allianceremove", this);
    cmdsource->Unsubscribe("/allianceleave", this);
    cmdsource->Unsubscribe("/allianceleader", this);
    cmdsource->Unsubscribe("/endalliance", this);
}

const char *psGuildCommands::HandleCommand(const char *cmd)
{
    WordArray words(cmd);

    if (words.GetCount() == 0)
        return "";

    if (words[0] == "/guildinfo")
    {
        psGUIGuildMessage msg(psGUIGuildMessage::SUBSCRIBE_GUILD_DATA, "<x/>");
        msg.SendMessage();
        if ( words.GetCount() == 2 )
        {
            if ((words[1]!="yes")&&(words[1]!="no"))
                return "Syntax: /guildinfo yes|no";
            if(PawsManager::GetSingleton().FindWidget("GuildWindow")) //we can't be sure this is loaded
            {
                pawsCheckBox* onlineOnly = (pawsCheckBox*)PawsManager::GetSingleton().FindWidget("GuildWindow")->FindWidget("OnlineOnly");
                if(onlineOnly)
                    onlineOnly->SetState(words[1]=="yes");
            }
            csString command;
            command.Format("<r onlineonly=\"%s\"/>", (const char*)words[1] );
            psGUIGuildMessage msg2(psGUIGuildMessage::SET_ONLINE, command );
            msg2.SendMessage();
        }
        else
        {
            csString command;
            pawsCheckBox* onlineOnly = NULL;
            if(PawsManager::GetSingleton().FindWidget("GuildWindow")) //we can't be sure this is loaded
                onlineOnly = (pawsCheckBox*)PawsManager::GetSingleton().FindWidget("GuildWindow")->FindWidget("OnlineOnly");
            command.Format("<r onlineonly=\"%s\"/>", !onlineOnly || onlineOnly->GetState() ? "yes":"no");
            psGUIGuildMessage msg2(psGUIGuildMessage::SET_ONLINE, command );
            msg2.SendMessage();
        }
    }
    else
    {
        psGuildCmdMessage cmdmsg(cmd);
        cmdmsg.SendMessage();
    }

    return NULL;
}

void psGuildCommands::HandleMessage(MsgEntry *msg)
{
    (void)msg;
}
