/*
* adminmanager.cpp
*
* Copyright (C) 2001-2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
* http://www.atomicblue.org )
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
#include <ctype.h>
#include <limits.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/object.h>
#include <iutil/stringarray.h>
#include <iengine/campos.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/engine.h>

//=============================================================================
// Library Includes
//=============================================================================
#include <ibgloader.h>
#include "util/psdatabase.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/strutil.h"
#include "util/eventmanager.h"
#include "util/pspathnetwork.h"
#include "util/waypoint.h"
#include "util/pspath.h"
#include "engine/psworld.h"

#include "net/msghandler.h"

#include "bulkobjects/psnpcdialog.h"
#include "bulkobjects/psnpcloader.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psmerchantinfo.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/pstrait.h"
#include "bulkobjects/pscharinventory.h"

#include "rpgrules/factions.h"

#include "engine/linmove.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "actionmanager.h"
#include "adminmanager.h"
#include "authentserver.h"
#include "cachemanager.h"
#include "chatmanager.h"
#include "clients.h"
#include "commandmanager.h"
#include "creationmanager.h"
#include "entitymanager.h"
#include "gem.h"
#include "globals.h"
#include "gmeventmanager.h"
#include "guildmanager.h"
#include "hiremanager.h"
#include "marriagemanager.h"
#include "netmanager.h"
#include "npcmanager.h"
#include "playergroup.h"
#include "progressionmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "questionmanager.h"
#include "questmanager.h"
#include "spawnmanager.h"
#include "usermanager.h"
#include "weathermanager.h"

//-----------------------------------------------------------------------------

/** This class asks user to confirm that he really wants to do the area target he/she requested */
class AreaTargetConfirm : public PendingQuestion
{
public:
    AreaTargetConfirm(AdminManager* msgmanager, const csString in_msg, csString in_player, const csString &question, Client* in_client)
        : PendingQuestion(in_client->GetClientNum(),question, psQuestionMessage::generalConfirm)
    {
        //save variables for later use
        this->command = in_msg;
        this->client  = in_client;
        this->player  = in_player;
        this->msgmanager = msgmanager;
    }

    /// Handles the user choice
    virtual void HandleAnswer(const csString &answer)
    {
        if(answer != "yes")    //if we haven't got a confirm just get out of here
        {
            return;
        }

        //decode the area command and get the list of objects to work on
        csArray<csString> filters = msgmanager->DecodeCommandArea(client, player);
        csArray<csString>::Iterator it(filters.GetIterator());
        while(it.HasNext())
        {
            csString cur_player = it.Next(); //get the data about the specific entity on this iteration

            gemObject* targetobject = psserver->GetAdminManager()->FindObjectByString(cur_player, client->GetActor());
            if(targetobject == client->GetActor())
            {
                continue;
            }

            csString cur_command = command;  //get the original command inside a variable where we will change it according to cur_player
            cur_command.ReplaceAll(player.GetData(), cur_player.GetData()); //replace the area command with the eid got from the iteration
            psAdminCmdMessage cmd(cur_command.GetData(),client->GetClientNum()); //prepare the new message
            cmd.FireEvent(); //send it to adminmanager
        }
    }

protected:
    csString command;   ///< The complete command sent from the client originally (without any modification)
    Client* client;     ///< Originating client of the command
    csString player;    ///< Normally this should be the area:x:x command extrapolated from the original command
    AdminManager* msgmanager;
};

psRewardData::psRewardData(Reward_Type prewardType)
{
    rewardType = prewardType;
}

psRewardData::~psRewardData()
{

}

bool psRewardData::IsZero()
{
    return true;
}

psRewardDataExperience::psRewardDataExperience(int pexpDelta)
    : psRewardData(REWARD_EXPERIENCE)
{
    expDelta = pexpDelta;
}

bool psRewardDataExperience::IsZero()
{
    return (expDelta == 0);
}

psRewardDataFaction::psRewardDataFaction(csString pfactionName, int pfactionDelta)
    : psRewardData(REWARD_FACTION)
{
    factionName = pfactionName;
    factionDelta = pfactionDelta;
}

bool psRewardDataFaction::IsZero()
{
    return (factionName.IsEmpty() || factionDelta == 0);
}

psRewardDataSkill::psRewardDataSkill(csString pskillName, int pskillDelta, int pskillCap, bool prelativeSkill)
    : psRewardData(REWARD_SKILL)
{
    skillName = pskillName;
    skillDelta = pskillDelta;
    skillCap = pskillCap;
    relativeSkill = prelativeSkill;
}

bool psRewardDataSkill::IsZero()
{
    return (skillName.IsEmpty());
}

psRewardDataPractice::psRewardDataPractice(csString pSkillName, int pPractice)
    : psRewardData(REWARD_PRACTICE)
{
    skillName = pSkillName;
    practice = pPractice;
}

bool psRewardDataPractice::IsZero()
{
    return (skillName.IsEmpty() || practice == 0);
}

psRewardDataMoney::psRewardDataMoney(csString pmoneyType, int pmoneyCount, bool prandom)
    : psRewardData(REWARD_MONEY), moneyType(pmoneyType), moneyCount(pmoneyCount), random(prandom)
{
}

bool psRewardDataMoney::IsZero()
{
    return (moneyType.IsEmpty() || moneyCount == 0);
}

psRewardDataItem::psRewardDataItem(csString pitemName, int pstackCount)
    : psRewardData(REWARD_ITEM)
{
    itemName = pitemName;
    stackCount = pstackCount;
}

bool psRewardDataItem::IsZero()
{
    return (itemName.IsEmpty() || stackCount == 0);
}

// AdminCmdDataFactory
// creation macro for the create function
#define ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(Class) \
AdminCmdData* Class::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client *client, WordArray &words) \
{ \
    return new Class(msgManager, me, msg, client, words); \
}
// creation macro for the create function
#define ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE_WITH_CMD(Class) \
AdminCmdData* Class::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client *client, WordArray &words) \
{ \
    return new Class(command, msgManager, me, msg, client, words); \
}


AdminCmdData::AdminCmdData(csString commandName, WordArray &words)
    : command(commandName), help(false), valid(true)
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // parse nothing, so words array must only contain the command
    if(words.GetCount() != 1)
    {
        valid = false;
    }
}

AdminCmdData* AdminCmdData::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
{
    return new AdminCmdData(command, words);
}

csString AdminCmdData::GetHelpMessage()
{
    // only return the command
    return "Syntax: \"" + command + "\"";
}

void AdminCmdData::ParseError(MsgEntry* me, const char* errmsg)
{
    valid = false;
    if(me)  // only if message entry really contains something
    {
        psserver->SendSystemError(me->clientnum, errmsg);
    }
}

bool AdminCmdData::LogGMCommand(Client* gmClient, const char* cmd)
{
    return LogGMCommand(gmClient, 0, cmd);
}

bool AdminCmdData::LogGMCommand(Client* gmClient, PID playerID, const char* cmd)
{
    if(!strncmp(cmd,"/slide",6))  // don't log all these.  spamming the GM log table.
        return true;

    csString escape;
    db->Escape(escape, cmd);
    int result = db->Command("INSERT INTO gm_command_log "
                             "(account_id,gm,command,player,ex_time) "
                             "VALUES (%u,%u,\"%s\",%u,Now())", gmClient->GetAccountID().Unbox(), gmClient->GetPID().Unbox(), escape.GetData(), playerID.Unbox());
    return (result <= 0);
}

csString AdminCmdTargetParser::GetHelpMessagePartForTarget()
{
    csString help;

    if(IsAllowedTargetType(ADMINCMD_TARGET_UNKNOWN))
    {
        help += "|UNKNW.";
    }

    // explicit targeting is allowed
    if(IsAllowedTargetType(ADMINCMD_TARGET_TARGET))
    {
        help += "|target";
    }
    // pid: followed by a valid PID is allowed
    if(IsAllowedTargetType(ADMINCMD_TARGET_PID))
    {
        help += "|pid:<PID>";
    }
    // target is an area of targets
    if(IsAllowedTargetType(ADMINCMD_TARGET_AREA))
    {
        help += "|area:type:range[:name]";
    }
    // target is the issuing client
    if(IsAllowedTargetType(ADMINCMD_TARGET_ME))
    {
        help += "|me";
    }
    // target is a player (by name)
    if(IsAllowedTargetType(ADMINCMD_TARGET_PLAYER))
    {
        help += "|<player name>";
    }
    // target is a npc (by name)
    if(IsAllowedTargetType(ADMINCMD_TARGET_NPC))
    {
        help += "|<npc name>";
    }
    // target is specified by eid
    if(IsAllowedTargetType(ADMINCMD_TARGET_EID))
    {
        help += "|eid:<EID>";
    }
    // target is any object
    if(IsAllowedTargetType(ADMINCMD_TARGET_OBJECT))
    {
        help += "|<object name>";
    }
    if(IsAllowedTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        help += "|<focus select a target>";
    }

    // remove the starting '|' character
    if(!help.IsEmpty())
    {
        help.SetAt(0, ' ');
        help.LTrim();
    }

    // make the whole target stuff optional, because the clients target
    // can be used as the target for the command.
    if(IsAllowedTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        help = "[" + help + "]";
    }

    return help;
}

bool AdminCmdTargetParser::GetPlayerAccountIDByPID(size_t gmClientNum, const csString &word)
{
    // just to make the whole pid story quite clear
    // getting players by pid in this place should only be used when the player is offline
    // otherwise nasty side effects can and will happen
    // because the players client will not be notified of changes
    // basically the pid is resolved to an account id that is then used to
    // execute the specified command

    //first of all check if db hits are allowed. most commands don't
    //work with the database so there is no use to hit the db for that.
    if(!IsAllowedTargetType(ADMINCMD_TARGET_DATABASE))
        return false;

    PID pid = PID(strtoul(word.Slice(4).GetData(), NULL, 10));
    if(!pid.IsValid())
    {
        psserver->SendSystemError(gmClientNum, "Error, bad PID: %s", ShowID(pid));
        return false;
    }
    Result result(db->Select("SELECT account_id, name FROM characters where id=%u",pid.Unbox()));
    if(!result.IsValid() || !result.Count())
    {
        psserver->SendSystemError(gmClientNum, "Error, not a valid PID: %s", ShowID(pid));
        return false;
    }
    AccountID accountID = AccountID(result[0].GetUInt32("account_id"));
    if(!accountID.IsValid())
    {
        psserver->SendSystemError(gmClientNum, "Error, bad AccountID: %s", ShowID(accountID));
        return false;
    }

    targetID = pid; // pid is valid, store it
    target = result[0]["name"]; //take the name from db as we have the pid already
    targetAccountID = accountID;
    targetTypes |= ADMINCMD_TARGET_PID;

    psserver->SendSystemInfo(gmClientNum, "Account ID of player (pid:%s) %s is %s.",
                             target.GetData(), ShowID(targetID), ShowID(targetAccountID));

    return true;
}

AccountID AdminCmdTargetParser::GetAccountID(size_t gmClientNum)
{
    if(!IsTargetType(ADMINCMD_TARGET_PLAYER) &&
            !IsTargetType(ADMINCMD_TARGET_PID))
    {
        psserver->SendSystemError(gmClientNum,"Can only get AccountIDs for players!");
    }
    else if(!targetAccountID.IsValid())
    {
        // when player is online, then fetch the account through the 'obj' way
        if(IsOnline())
        {
            targetAccountID = targetClient->GetAccountID();
        }
        else
        {
            // otherwise query the database
            GetPlayerAccountIDByName(gmClientNum, target, true);
        }
    }
    return targetAccountID;
}

bool AdminCmdTargetParser::GetPlayerAccountIDByName(size_t gmClientNum, const csString &word, bool reporterror)
{
    //first of all check if db hits are allowed. most commands don't
    //work with the database so there is no use to hit the db for that.
    if(!IsAllowedTargetType(ADMINCMD_TARGET_DATABASE))
        return false;

    // try to find an offline player by name and resolve to a pid
    csString name;
    // normalize and escape name (to avoid security issues)
    db->Escape(name, NormalizeCharacterName(word).GetData());
    Result result(db->Select("SELECT account_id, id FROM characters where name='%s'",name.GetData()));

    //nothing here, try with another name
    if(!result.IsValid() || result.Count() == 0)
    {
        if(reporterror)
        {
            psserver->SendSystemError(gmClientNum,"No player found with the name '%s'!",name.GetData());
        }
        return false;
    }
    else if(result.Count() != 1)
        //more than one result it means we have duplicates, refrain from continuing
    {
        psserver->SendSystemError(gmClientNum,"Multiple characters with same name '%s'. Use pid.",name.GetData());
        return false;
    }
    PID pid = PID(result[0].GetUInt32("id")); //get the PID
    if(!pid.IsValid())
    {
        psserver->SendSystemError(gmClientNum, "Error, bad PID: %s", ShowID(pid));
        return false;
    }
    AccountID accountID = AccountID(result[0].GetUInt32("account_id"));
    if(!accountID.IsValid())
    {
        psserver->SendSystemError(gmClientNum, "Error, bad AccountID: %s", ShowID(accountID));
        return false;
    }

    targetID = pid;
    targetAccountID = accountID;
    return true;
}

bool AdminCmdTargetParser::GetPlayerAccountIDByPIDFromName(size_t gmClientNum, const csString &word, bool reporterror)
{
    if(GetPlayerAccountIDByName(gmClientNum, word, reporterror))
    {
        target = NormalizeCharacterName(word); // store the name
        targetTypes |= ADMINCMD_TARGET_PID;
        return true;
    }
    return false;
}



bool AdminCmdTargetParser::GetPlayerClient(AdminManager* msgManager, size_t gmClientNum, const csString &playerName, bool allowduplicate)
{
    targetClient = msgManager->FindPlayerClient(playerName); // Other player?
    if(targetClient)
    {
        //check that the actor name isn't duplicate
        if(!CharCreationManager::IsUnique(playerName, true))
        {
            duplicateActor = true;
            if(!allowduplicate)
            {
                // when duplicates are disallowed, return false (not unique)
                return false;
            }
        }
        return true;
    }
    // no player online with the given name
    return false;
}

void AdminCmdTargetParser::Reset()
{
    targetTypes = ADMINCMD_TARGET_UNKNOWN;
    target.Clear();
    targetObject = NULL;
    targetClient = NULL;
    targetActor = NULL;
    duplicateActor = false;
    targetID = 0;
    targetAccountID = 0;
}

bool AdminCmdTargetParser::ParseTarget(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, csString word)
{
    // flag to try the client target when everything fails
    bool tryClientTarget = false;

    Reset();

    // Targeting for all commands
    if(word.Length() > 0)
    {
        // the command allows a target selected by the client
        if(IsAllowedTargetType(ADMINCMD_TARGET_TARGET) && word == "target")
        {
            tryClientTarget = true;
            targetTypes |= ADMINCMD_TARGET_TARGET;
        }
        // the command allows to use a PID:
        else if(IsAllowedTargetType(ADMINCMD_TARGET_PID) && word.StartsWith("pid:",true))
        {
            // NOTE: this might not set targetObject, targetActor, targetClient
            // because the target* may not be online
            targetObject = msgManager->FindObjectByString(word, NULL);

            if(!targetObject)
            {
                // player is offline
                psserver->SendSystemInfo(me->clientnum,"PID:%s no online client", ShowID(targetID));
            }
            else
            {
                targetActor = targetObject->GetActorPtr();
                targetClient = targetObject->GetClient();
                target = (targetClient)?targetClient->GetName():targetObject->GetName();
                targetID = targetObject->GetPID();

                psserver->SendSystemInfo(me->clientnum,"%s resolved to online client %s", ShowID(targetID), target.GetData());
            }
        }
        // command allows to target an area
        else if(IsAllowedTargetType(ADMINCMD_TARGET_AREA) && word.StartsWith("area:",true))
        {
            //generate the string
            csString question; //used to hold the generated question for the client
            //first part of the question add also the command which will be used on the area if confirmed (the name not the arguments)
            question.Format("Are you sure you want to execute %s on:\n", msg.cmd.GetData());
            //command.GetDataSafe());
            csArray<csString> filters = msgManager->DecodeCommandArea(client, word); //decode the area command
            csArray<csString>::Iterator it(filters.GetIterator());
            if(filters.GetSize())
            {
                while(it.HasNext())  //iterate the resulting entities
                {
                    csString player = it.Next();
                    targetObject = msgManager->FindObjectByString(player,client->GetActor()); //search for the entity in order to work on it
                    if(targetObject && targetObject != client->GetActor())  //just to be sure
                    {
                        question += targetObject->GetName(); //get the name of the target in order to show it nicely
                        question += '\n';
                    }
                }
                //send the question to the client
                psserver->questionmanager->SendQuestion(new AreaTargetConfirm(msgManager, msg.cmd, word, question, client));
            }
            target = word; // valid destination, store it
            targetTypes |= ADMINCMD_TARGET_AREA;
            return true;
        }
        else
        {
            if(IsAllowedTargetType(ADMINCMD_TARGET_ME) && word == "me")
            {
                targetClient = client; // Self
                targetTypes |= ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER;
            }
            else if(IsAllowedTargetType(ADMINCMD_TARGET_PLAYER))
            {
                // targetclient, online, duplicateActor are set accordingly
                // by GetPlayerClient
                if(GetPlayerClient(msgManager, me->clientnum, word, true))
                {
                    targetTypes |= ADMINCMD_TARGET_PLAYER;
                }
                else if(duplicateActor)
                {
                    psserver->SendSystemError(me->clientnum,"Name resolves to multiple online clients! Try using PID");
                    return false;
                }
                // if the command supports pid handling
                else if(IsAllowedTargetType(ADMINCMD_TARGET_PID))
                {
                    GetPlayerAccountIDByPIDFromName(me->clientnum, word, false);
                }
            }

            if(targetClient)  // Found client
            {
                targetActor = targetClient->GetActor();
                targetObject = (gemObject*)targetActor;
                target = targetClient->GetName(); // valid destination, store it
                targetID = targetObject->GetPID();
            }
            else if(IsAllowedTargetType(ADMINCMD_TARGET_OBJECT) ||
                    IsAllowedTargetType(ADMINCMD_TARGET_NPC)    ||
                    IsAllowedTargetType(ADMINCMD_TARGET_ITEM))
                // Not found yet
            {
                targetObject = msgManager->FindObjectByString(word,client->GetActor()); // Find by ID or name
                if(targetObject)
                {
                    targetTypes |= ADMINCMD_TARGET_OBJECT;
                }
                else
                    // no object selected try the clients target
                {
                    tryClientTarget = true;
                }
            }
            // No target identified yet, so try the client's target
            else
            {
                tryClientTarget = true;
            }
        }
    }
    // word is empty, so it must be the client's target
    // (otherwise there is none)
    else
    {
        tryClientTarget = true;
    }

    // take the target of the client (when client selected something)
    if(tryClientTarget && IsAllowedTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        targetObject = client->GetTargetObject();
        if(!targetObject && word == "target")
        {
            // only in when a 'target' keyword, send an error to the client
            psserver->SendSystemError(me->clientnum,"You must have a target selected.");
            return false;
        }
        else if(targetObject)
        {
            targetTypes |= ADMINCMD_TARGET_CLIENTTARGET;
        }
    }
    // test whether this is a npc or not
    if(targetObject)
    {
        gemNPC* npctarget = dynamic_cast<gemNPC*>(targetObject);
        if(npctarget && npctarget->GetClientID() == 0)
        {
            if(!IsAllowedTargetType(ADMINCMD_TARGET_NPC))
            {
                // it is a npc, but npc is not an allowed target
                psserver->SendSystemError(me->clientnum,"Target is an NPC, but server does not allow this command on an npc.");
                return false;
            }
            else
            {
                // it is definitely not a player (because it is a npc)
                targetTypes &= ~(ADMINCMD_TARGET_PLAYER);
                targetTypes |= ADMINCMD_TARGET_NPC;
            }
        }
    }
    if(targetObject && !targetActor)  // Get the actor, client, and name for a found object
    {
        targetActor = targetObject->GetActorPtr();
        targetClient = targetObject->GetClient();
        target = (targetClient)?targetClient->GetName():targetObject->GetName();
        targetID = targetObject->GetPID();
    }

    // Set additional targetTypes accordingly for me & player
    if(targetClient)
    {
        targetTypes |= ADMINCMD_TARGET_PLAYER;
        if(targetClient == client)
        {
            // check if this is me
            targetTypes |= ADMINCMD_TARGET_ME;
        }
    }

    // Every test for a specific type failed, so if it is allowed to have a
    // string as the target, then this is it.
    if(word.Length() > 0 && targetTypes == ADMINCMD_TARGET_UNKNOWN)
    {
        if(IsAllowedTargetType(ADMINCMD_TARGET_ACCOUNT))
        {
            // account uses lowercase
            Result result;
            csString username = word;
            csString usernameEscaped;
            // normalize and escape the account name (prevents security issues)
            username.Downcase();
            db->Escape(usernameEscaped, username.GetData());
            result = db->Select("SELECT id FROM accounts WHERE username = '%s' LIMIT 1",usernameEscaped.GetData());
            if(result.IsValid() && result.Count() == 1)
            {
                AccountID accountID = AccountID(result[0].GetUInt32("id"));
                if(accountID.IsValid())
                {
                    targetAccountID = accountID;
                    target = word;
                    return true;
                }
            }
        }
        if(IsAllowedTargetType(ADMINCMD_TARGET_STRING))
        {
            target = word; // only assign the item name
            targetTypes = ADMINCMD_TARGET_STRING;
            return true;
        }
    }

    // return false, because the given string is NOT a target, but the aquired
    // target is the target in the gui of the client
    if(IsTargetType(ADMINCMD_TARGET_CLIENTTARGET) && word != "target")
        return false;

    // at the very last (STRING) this must be set
    if(target.IsEmpty())
        return false;

    return true;
}

AdminCmdSubCommandParser::AdminCmdSubCommandParser(csString commandList)
{
    csString cmd;
    size_t pos, next;
    csString cmdList = commandList;

    cmdList.Collapse(); // trim whitespaces and collapse them to one each
    // prefix with a whitespace in order to keep the splitting simple
    //commandList = " " + commandList;

    next = 0;
    for(pos=0; next < cmdList.Length(); pos=next+1)
    {
        // find next space (= end of command)
        next = cmdList.FindFirst(' ',pos);
        // store next command
        cmd = cmdList.Slice(pos, next - pos);
        // default help is the command itself (no params);
        subCommands.Put(cmd,cmd);
    }
}

csString AdminCmdSubCommandParser::GetHelpMessage()
{
    csString help = "";

    // concatenate all subcommands
    csHash<csString, csString>::GlobalIterator it = subCommands.GetIterator();
    while(it.HasNext())
    {
        csTuple2<csString,csString> tuple = it.NextTuple();
        help += "|" + tuple.second;
    }

    // remove the starting '|' character
    help.SetAt(0, ' ');
    help.LTrim();

    return help;
}

csString AdminCmdSubCommandParser::GetHelpMessage(const csString &subcommand)
{
    // if help not found the default is to return an error msg
    // actually this should never happen
    csString emptyHelp("not a subcommand, so no help for:" + subcommand);
    return subcommand + " " + subCommands.Get(subcommand, emptyHelp);
}

bool AdminCmdSubCommandParser::IsSubCommand(const csString &word)
{
    return subCommands.In(word);
}

void AdminCmdSubCommandParser::Push(csString subcommand, csString helpmsg)
{
    // Lazyness^3, prefix the helpmessage automatically with the subcommand
    csString extendedhelpmsg(subcommand + " " + helpmsg);
    subCommands.PutUnique(subcommand, helpmsg);
}

AdminCmdRewardParser::AdminCmdRewardParser()
{
    // initialize with recognized award types and their help message
    rewardTypes.Push("exp","<value>");
    rewardTypes.Push("item","<count> <item>");
    rewardTypes.Push("skill","<skillname>|all [+-]<value> [<max>]");
    rewardTypes.Push("money","<circles|hexas|octas|trias> <value>|random");
    rewardTypes.Push("faction","<factionname> <value>");
    rewardTypes.Push("practice","<skillname>|all <value>");
}

bool AdminCmdRewardParser::ParseWords(size_t index, const WordArray &words)
{
    // temporary variables for the loop
    csString subCmd;
    csString item;
    csString skill;
    int stackCount;
    bool relative;
    int cap, delta;

    // first check that there are enough words left (minimum 2 words left)
    if(words.GetCount() < index + 2)
    {
        return false;
    }

    size_t remaining;
    // doesn't include a check for duplicate award types
    // because several award types can be awarded multiple times
    // so one command now can give several awards at once
    while(index < words.GetCount())
    {
        subCmd = words[index++];
        remaining = words.GetCount() - index;

        // create reward data structures accordingly
        // and parse the data from the command line
        if(subCmd == "exp" && remaining >= 1)
        {
            rewards.Push(new psRewardDataExperience(words.GetInt(index++)));
        }
        // items are specified as: <stackcount> <item>
        else if(subCmd == "item" && remaining >= 2)
        {
            stackCount = words.GetInt(index++);
            item = words[index++];
            rewards.Push(new psRewardDataItem(item,stackCount));
        }
        else if(subCmd == "skill" && remaining >= 2)
        {
            relative = false;
            cap = 0;
            skill = words[index++];

            // check for relative value
            if(words[index].GetAt(0) == '+' || words[index].GetAt(0) == '-')
                relative = true;
            delta = words.GetInt(index++);

            // check for optional maximum
            cap = words.GetInt(index);
            if(cap)
                index++;
            rewards.Push(new psRewardDataSkill(skill, delta, cap, relative));
        }
        else if(subCmd == "money" && remaining >= 2)
        {
            // 3rd parameter means, when there is an argument 'random', then
            // randomize
            csString pmoneyType = words[index++];
            int pmoneyCount = words.GetInt(index++);
            bool prandom = (words[index++] == "random");
            rewards.Push(new psRewardDataMoney(pmoneyType,pmoneyCount,prandom));
        }
        else if(subCmd == "faction" && remaining >= 2)
        {
            rewards.Push(new psRewardDataFaction(words[index++], words.GetInt(index++)));
        }
        else if(subCmd == "practice" && remaining >= 2)
        {
            relative = false;
            skill = words[index++];

            // check for relative value
            if(words[index].GetAt(0) == '+' || words[index].GetAt(0) == '-')
                relative = true;
            delta = words.GetInt(index++);

            rewards.Push(new psRewardDataPractice(skill, delta));
        }
        else // invalid arguments
        {
            //check wether command was invalid or it was too short
            if(rewardTypes.IsSubCommand(subCmd))
            {
                error = "Rewardtype " + subCmd + " is missing parameters";
            }
            else
            {
                error = "Not a valid reward type " + subCmd;
            }
            return false;
        }
    }
    return true;
}

csString AdminCmdRewardParser::GetHelpMessage()
{
    return "REWARD: \n   " +
           rewardTypes.GetHelpMessage("exp") + "\n" +
           "   " + rewardTypes.GetHelpMessage("item") + "\n" +
           "   " + rewardTypes.GetHelpMessage("skill") + "\n" +
           "   " + rewardTypes.GetHelpMessage("money") + "\n" +
           "   " + rewardTypes.GetHelpMessage("faction") + "\n" +
           "   " + rewardTypes.GetHelpMessage("practice");
}

AdminCmdOnOffToggleParser::AdminCmdOnOffToggleParser(ADMINCMD_SETTING_ONOFF defaultValue)
    : value(defaultValue)
{
}

bool AdminCmdOnOffToggleParser::ParseWord(const csString &word)
{
    // check if the word is valid for setting
    // and store the appropriate value
    if(word.CompareNoCase("on"))
    {
        value = ADMINCMD_SETTING_ON;
    }
    else if(word.CompareNoCase("off"))
    {
        value = ADMINCMD_SETTING_OFF;
    }
    else if(word.CompareNoCase("toggle"))
    {
        value = ADMINCMD_SETTING_TOGGLE;
    }
    // everything else is not valid
    else
    {
        error = "Not a valid setting (on|off|toggle)";
    }

    return (value != ADMINCMD_SETTING_UNKNOWN);
}

bool AdminCmdOnOffToggleParser::IsOn()
{
    return (value == ADMINCMD_SETTING_ON);
}

bool AdminCmdOnOffToggleParser::IsOff()
{
    return (value == ADMINCMD_SETTING_OFF);
}

bool AdminCmdOnOffToggleParser::IsToggle()
{
    return (value == ADMINCMD_SETTING_TOGGLE);
}

csString AdminCmdOnOffToggleParser::GetHelpMessage()
{
    return "on|off|toggle";
}

AdminCmdDataTarget::AdminCmdDataTarget(csString commandName, int targetTypes, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData(commandName), AdminCmdTargetParser(targetTypes)
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try to parse the first word for a target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        index++;
    }
    // if first word is not a target and the client has not selected a target
    else if(!IsTargetType(ADMINCMD_TARGET_CLIENTTARGET) &&
            !IsTargetType(ADMINCMD_TARGET_STRING))
    {
        ParseError(me, "No target specified");
    }
}

AdminCmdData* AdminCmdDataTarget::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
{
    return new AdminCmdDataTarget(command, allowedTargetTypes, msgManager, me, msg, client, words);
}

csString AdminCmdDataTarget::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + "\"";
}

bool AdminCmdDataTarget::LogGMCommand(Client* gmClient, const char* cmd)
{
    return AdminCmdData::LogGMCommand(gmClient, targetID, cmd);
}

AdminCmdDataTargetReason::AdminCmdDataTargetReason(csString commandName, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget(commandName, ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // /command [target|player] reason
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }
    // and the client must have a target selected
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        if(words.GetCount() >= index + 1)
        {
            reason = words.GetTail(index);
        }
        else
        {
            ParseError(me, "Missing reason");
        }
    }
    else
    {
        ParseError(me, "No target selected"); // otherwise invalid syntax
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE_WITH_CMD(AdminCmdDataTargetReason)

/*
AdminCmdData* AdminCmdDataTargetReason::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client *client, WordArray &words)
{
    return new AdminCmdDataTargetReason(command, msgManager, me, msg, client, words);
}
*/

csString AdminCmdDataTargetReason::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() +" reason\"";
}

AdminCmdDataDeath::AdminCmdDataDeath(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/death",  ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_EID)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // /command [target|player] requestor
    // try to find a target specifier
    if((found = ParseTarget(msgManager, me, msg, client, words[1])))
    {
        index++;
    }
    // if the target was in the first word, or the client is targeting something
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        if(words.GetCount() == index + 1)
        {
            // requestor can be anything (so no parsing needed)
            requestor = words[index];
        }
        // no target, or not the right size
        else if(words.GetCount() != index)
        {
            ParseError(me, "Too many arguments");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

AdminCmdData* AdminCmdDataDeath::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
{
    return new AdminCmdDataDeath(msgManager, me, msg, client, words);
}

csString AdminCmdDataDeath::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() +" requestor\"";
}

AdminCmdDataDeleteChar::AdminCmdDataDeleteChar(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/deletechar", ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID), requestor(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // /command [target|player] requestor
    // try to find a target specifier
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        index++;
        // next word would be the requestor
        if(words.GetCount() == index+1)
        {
            if(!requestor.ParseTarget(msgManager, me, msg, client, words[index]))
            {
                ParseError(me, "Invalid requestor specified");
            }
        }
        // no target, or not the right size
        else if(words.GetCount() != index)
        {
            ParseError(me, "Too many arguments");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

AdminCmdData* AdminCmdDataDeleteChar::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
{
    return new AdminCmdDataDeleteChar(msgManager, me, msg, client, words);
}

csString AdminCmdDataDeleteChar::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() +" requestor\"";
}


AdminCmdDataUpdateRespawn::AdminCmdDataUpdateRespawn(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/updaterespawn", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found = false; // true when first word is a target string

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target string or client has a valid target selected
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // if only one word remains containing 'here'
        if(words.GetCount() == index+1 && (words[index] == "here"))
        {
            place = words[index++];
        }
        // if there are further words, then error (to many words or not 'here')
        else if(words.GetCount() != index)
        {
            ParseError(me, "Too many parameters");
        }
    }
    // no target found
    else
    {
        ParseError(me, "No target specified");
    }
}

AdminCmdData* AdminCmdDataUpdateRespawn::CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
{
    return new AdminCmdDataUpdateRespawn(msgManager, me, msg, client, words);
}

csString AdminCmdDataUpdateRespawn::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " [here]\"";
}

AdminCmdDataBan::AdminCmdDataBan(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/ban", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_ACCOUNT | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // /ban [target|player] mins hours days
    // try to parse first words as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if not enough remaining words
    if(words.GetCount() <= index + 1)
    {
        ParseError(me, "Not enough parameters");
    }
    // either a target was in the first word or
    // the client has got a valid target
    else if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // try to parse numbers
        minutes = words.GetInt(index);
        hours  = words.GetInt(index+1);
        days   = words.GetInt(index+2);

        banIP = false;

        // the words are not numbers
        if(!minutes && !hours && !days)
        {
            // test for ip ban
            if(words[index].Upcase() == "IP")
            {
                banIP = true;
                reason = words.GetTail(index+1);
            }
            else
                reason = words.GetTail(index);
        }
        else
        {
            // test for ip ban
            if(words[index+3].Upcase() == "IP")
            {
                banIP = true;
                reason = words.GetTail(index+4);
            }
            else
                reason = words.GetTail(index+3);
        }
    }
    // no target
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataBan)

csString AdminCmdDataBan::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " [mins hours days] [IP] reason\"";
}

AdminCmdDataKillNPC::AdminCmdDataKillNPC(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/killnpc", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE), reload(false),damage(0.0)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client has selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // reload is optional
        if(words[index] == "reload")
        {
            index++;
            reload = true;
        }
        else if(words.GetCount() > index)
        {
            damage = words.GetFloat(index++);
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataKillNPC)

csString AdminCmdDataKillNPC::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " [<damage>|reload]\"";
}

AdminCmdDataPercept::AdminCmdDataPercept(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/percept", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client has selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        perception = words[index++];
        if(perception.IsEmpty())
        {
            ParseError(me, "Missing perception");
        }

        type = words[index++];

        if(words.GetCount() > index)
        {
            ParseError(me, "Too many parameters");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataPercept)

csString AdminCmdDataPercept::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " [percept] [type]\"";
}

//---------------------------------------------------------------------------------

AdminCmdDataChangeNPCType::AdminCmdDataChangeNPCType(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/changenpctype", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), npcType("")
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client has selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // the npc type name is mandatory
        if(words.GetCount() == index+1)
        {
            npcType = words[index]; // <Brain> or reload
            index++;
        }
        else if(words.GetCount() > index)
        {
            ParseError(me, "Too many parameters");
        }
        else
        {
            ParseError(me, "Missing parameters");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataChangeNPCType)

csString AdminCmdDataChangeNPCType::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " <brain>|reload\"";
}

//---------------------------------------------------------------------------------

AdminCmdDataDebugNPC::AdminCmdDataDebugNPC(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/debugnpc", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), debugLevel(0)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client has selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // the debug level is mandatory
        if(words.GetCount() == index+1)
        {
            debugLevel = atoi(words[index]);
            index++;
        }
        else if(words.GetCount() > index)
        {
            ParseError(me, "Too many parameters");
        }
        else
        {
            ParseError(me, "Missing parameters");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataDebugNPC)

csString AdminCmdDataDebugNPC::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " <debuglevel>\"";
}

//---------------------------------------------------------------------------------

AdminCmdDataDebugTribe::AdminCmdDataDebugTribe(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/debugtribe", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), debugLevel(0)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client has selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // the debug level is mandatory
        if(words.GetCount() == index+1)
        {
            debugLevel = atoi(words[index]);
            index++;
        }
        else if(words.GetCount() > index)
        {
            ParseError(me, "Too many parameters");
        }
        else
        {
            ParseError(me, "Missing parameters");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataDebugTribe)

csString AdminCmdDataDebugTribe::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " <debuglevel>\"";
}

//---------------------------------------------------------------------------------

AdminCmdDataSetStackable::AdminCmdDataSetStackable(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/setstackable", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET), subCommandList("info on off reset help")
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try to parse first words as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // missing further words
    if(words.GetCount() == index)
    {
        ParseError(me, "Missing subcommand");
    }
    // if enough remaining words and either a target was in the first word or
    // the client has got a valid target
    else if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        if(words.GetCount() == index + 1)
        {
            if(subCommandList.IsSubCommand(words[index]))
            {
                stackableAction = words[index++];
            }
            else
            {
                ParseError(me, "Not a valid subcommand >" + words[index] + "<");
            }
        }
        else
        {
            ParseError(me, "Too many arguments");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSetStackable)

csString AdminCmdDataSetStackable::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " [info|on|off|reset]\"";
}

// loadquest is only responsible for loading and reloading quests
AdminCmdDataLoadQuest::AdminCmdDataLoadQuest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/loadquest")
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // first word is a valid target
    if(words.GetCount() >= index + 1)
    {
        questName = words[index++];
    }
    else if(words.GetCount() == index)
    {
        ParseError(me, "No quest specified");
    }
    else
    {
        ParseError(me, "Too many arguments");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataLoadQuest)

csString AdminCmdDataLoadQuest::GetHelpMessage()
{
    return "Syntax: \"" + command + " <questname>\"";
}

AdminCmdDataInfo::AdminCmdDataInfo(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/info", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE), subCmd("summary")
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try to parse the first word for a target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        index++;
    }
    // if first word is not a target and the client has not selected a target
    else if(!IsTargetType(ADMINCMD_TARGET_CLIENTTARGET) &&
            !IsTargetType(ADMINCMD_TARGET_STRING))
    {
        ParseError(me, "No target given");
    }

    // /info is allowed
    if(words.GetCount() == index)
    {
        // no action required
        subCmd = "summary";
    }
    else if(words.GetCount() == (index + 1))
    {
        if(words[index] == "all" ||
                words[index] == "old" ||  // For now keep old output format with this option.
                words[index] == "pet" ||
                words[index] == "summary")
        {
            subCmd = words[index];
        }
        else
        {
            ParseError(me, csString("Unknown sub command: ") + csString(words[index]));
        }

    }
    else
    {
        ParseError(me, "Not enough parameters");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataInfo)

csString AdminCmdDataInfo::GetHelpMessage()
{
    return "Syntax: \"" + command + " all|old|pet|summary\"";
}

AdminCmdDataItem::AdminCmdDataItem(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/item", ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_STRING), random(false), quality(50), quantity(1)
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // /item is allowed
    if(words.GetCount() == index)
    {
        // no action required
    }
    // /item <item> [random] [<quality>] [<quantity>]
    else if(words.GetCount() >= 2)
    {
        // try first parameter as a target string (item name string)
        if(ParseTarget(msgManager, me, msg, client, words[index]))
        {
            index++;

            // random is optional
            csString cc = words[index];
            if(words[index]=="random")
            {
                index++;
                random = true;
            }
            if(words.GetCount() > index)
            {
                quality = words.GetInt(index++);
            }
            if(words.GetCount() > index)
            {
                quantity = words.GetInt(index++);
            }
            if(words.GetCount() > index)
            {
                ParseError(me, "Too many parameters");
            }
        }
        else
        {
            ParseError(me, "No item name given");
        }
    }
    else
    {
        ParseError(me, "Not enough parameters");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataItem)

csString AdminCmdDataItem::GetHelpMessage()
{
    return "Syntax: \"" + command + " <name>|help [random] [<quality>] [<quantity>]\"";
}

AdminCmdDataKey::AdminCmdDataKey(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/key", ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_CLIENTTARGET),
      subCommandList("make makemaster copy clearlocks skel"),
      subTargetCommandList("changelock makeunlockable securitylockable addlock removelock")
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    //check the subcommand and if it needs a target if the target was found
    if(subCommandList.IsSubCommand(words[index]))
    {
        subCommand = words[index++];
    }
    else if(subTargetCommandList.IsSubCommand(words[index]))
    {
        // if first word a target or client selected a valid target
        if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
        {
            subCommand = words[index++];
        }
        // no target
        else
        {
            ParseError(me, "No target selected");
        }
    }
    // no subcommand
    else
    {
        ParseError(me, "Missing or unknown subcommand >" + words[index] + "<");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataKey)

csString AdminCmdDataKey::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + subCommandList.GetHelpMessage() + subTargetCommandList.GetHelpMessage() + "]\"";
}

AdminCmdDataRunScript::AdminCmdDataRunScript(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/runscript", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET), origObj(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    // when help is requested, return immediate
    if(IsHelp(words[index]))
        return;

    // /runscript <scriptName> <player> <player>

    //first take the name of the script
    scriptName = words[index++];
    //if there are more entries probably we have a target
    if(words.GetCount() >= index + 1)
    {
        //check first the target
        if(ParseTarget(msgManager, me, msg, client, words[index++]))
        {
            //if there is another field we have also an origin
            if(words.GetCount() == index + 1)
            {
                //check the origin if it's valid
                if(!origObj.ParseTarget(msgManager, me, msg, client, words[index++]))
                {
                    ParseError(me,"Invalid origin target given");
                }
            }
            //if we have more commands it means we have too many parameters
            else
            {
                ParseError(me,"Syntax error");
            }
        }
        //the given target is invalid
        else
        {
            ParseError(me,"Invalid target given");
        }
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataRunScript)

csString AdminCmdDataRunScript::GetHelpMessage()
{
    return "Syntax: \"" + command + " <scriptname> " + GetHelpMessagePartForTarget() + " [origin: " + origObj.GetHelpMessagePartForTarget() +"]\"";
}

AdminCmdDataCrystal::AdminCmdDataCrystal(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/hunt_location", ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_STRING)
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if(words.GetCount() == 6)
    {
        interval = words.GetInt(1);
        random   = words.GetInt(2);
        amount = words.GetInt(3);
        range = words.GetFloat(4);
        itemName = words.GetTail(5);
    }
    else
    {
        valid = false;
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataCrystal)

csString AdminCmdDataCrystal::GetHelpMessage()
{
    return "Syntax: \"" + command + "<interval> <random> <amount> <range> <itemname>\"";
}

AdminCmdDataTeleport::AdminCmdDataTeleport(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/teleport", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_DATABASE), destList("here there last spawn restore"), destObj(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM)
{
    destList.Push("map", "(<map name>|here) | (<sector name> <x> <y> <z>) [<instance>]");
    size_t index = 1;

    // when help is requested, return immediately
    if(IsHelp(words[1]))
        return;

    // natoka: this is not the complete parsing
    // destination parsing is partially done by GetTargetOfTeleport()
    // natoka: TODO make this parser better
    // try first word as a target
    if(ParseTarget(msgManager,me,msg,client,words[index]))
    {
        index++;
        destInstance = DEFAULT_INSTANCE;
        destInstanceValid = false;

        if(destList.IsSubCommand(words[index]))
        {
            dest = words[index++];
            // current word is not a default target
            // but it might be 'map'
            if(dest == "map")
            {
                // map target specified by map name
                if(words.GetCount() == index + 1)
                {
                    destMap = words[index++];
                }
                // or sector target specified by coordinates
                // with optional instance id at the end (parsed below)
                else if(words.GetCount() == index + 4 || words.GetCount() == index + 5)
                {
                    destSector = words[index++];
                    x = words.GetFloat(index++);
                    y = words.GetFloat(index++);
                    z = words.GetFloat(index++);
                }
                else
                {
                    ParseError(me, "Missing x,y,z coordinates");
                }
            }
        }
        // when the second word is me|pid:<PID>|playername|...
        else if(destObj.ParseTarget(msgManager, me, msg, client, words[index]))
        {
            index++;
            /*if (destObj.targetType == ADMINCMD_TARGET_ME)
            {
                // optional instance name
                if (words.GetCount() == index + 1)
                {
                    instanceName = words[index++];
                }
                else if (words.GetCount() != index)
                {
                    ParseError(me, "Too many arguments");
                }
            }*/
        }
        else
        {
            ParseError(me, "Invalid destination >" + words[index] + "<");
        }
        if(words.GetCount() == index + 1)
        {
            destInstance = words.GetInt(index++);
            destInstanceValid = true;
        }
        else if(words.GetCount() != index)
        {
            ParseError(me, "Too many arguments");
        }
    }
    else
    {
        valid = false;
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataTeleport)

csString AdminCmdDataTeleport::GetHelpMessage()
{
    return "Syntax: \"" + command + " <subject> <destination>\"\n"
           "Subject    : " + GetHelpMessagePartForTarget() + "\n"
           "Destination: me [<instance>]/target/<object name>/<NPC name>/<player name>/eid:<EID>/pid:<PID>/\n"
           "             here [<instance>]/last/spawn/restore/map [<map name>|here] <x> <y> <z> [<instance>]\n"
           "             there <instance>\n"
           "             restore";
}

AdminCmdDataSlide::AdminCmdDataSlide(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/slide", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try first word as target
    if((found = ParseTarget(msgManager,me,msg,client,words[index])))
    {
        index++;
    }
    // if first word a target or client selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        direction = words[index++];
        slideAmount = words.GetFloat(index++);
    }
    else
    {
        ParseError(me, "No target selected");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSlide)

csString AdminCmdDataSlide::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() +
           " [direction] [distance]\nAllowed directions: U D L R F B T I";
}

AdminCmdDataChangeName::AdminCmdDataChangeName(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/changename", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE), uniqueName(true), uniqueFirstName(true)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try first word as a target
    if(ParseTarget(msgManager,me,msg,client,words[index]))
    {
        index++;
        found = true;
    }

    // if first word a target or client selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // when name doesn't need to be unique
        if(words[index] == "force")
        {
            uniqueFirstName = false;
            index++;
        }
        // turn off uniqueness completely
        else if(words[index] == "forceall")
        {
            uniqueName = false;
            uniqueFirstName = false;
            index++;
        }

        // new name is required
        if(words.GetCount() >= index + 1)
        {
            newName = words[index++];
        }
        else
        {
            ParseError(me, "Missing new name argument");
        }
        // new Lastname is optional
        if(words.GetCount() == index + 1)
        {
            newLastName = words[index++];
        }
        else if(words.GetCount() != index)
        {
            ParseError(me, "Too many arguments");
        }
    }
    else
    {
        ParseError(me, "No target selected");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataChangeName)

csString AdminCmdDataChangeName::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() +
           " [force|forceall] <NewName> [NewLastName]\"";
}

AdminCmdDataChangeGuildName::AdminCmdDataChangeGuildName(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/changeguildname")
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if(words.GetCount() >= 2)
    {
        guildName = words[1];
        newName = words.GetTail(2);
    }
    else
    {
        valid = false;
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataChangeGuildName)

csString AdminCmdDataChangeGuildName::GetHelpMessage()
{
    return "Syntax: \"" + command + " <guildname> <newguildname>\"";
}

AdminCmdDataChangeGuildLeader::AdminCmdDataChangeGuildLeader(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/changeguildleader", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first parameter a valid player name or 'target'
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first param player|target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        guildName = words.GetTail(index);
    }
    else
    {
        ParseError(me, "No valid target selected");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataChangeGuildLeader)

csString AdminCmdDataChangeGuildLeader::GetHelpMessage()
{
    return "Syntax: \"" + command + " {target|player} <guildname>\"";
}

AdminCmdDataPetition::AdminCmdDataPetition(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/petition")
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // at least /petiton <text> is needed, only /petition is not allowed
    if(words.GetCount() >= 2)
    {
        petition = words.GetTail(1);
    }
    else
    {
        ParseError(me, "Missing petition text");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataPetition)

csString AdminCmdDataPetition::GetHelpMessage()
{
    return "Syntax: \"" + command + " <petition question/description>\"";
}

AdminCmdDataImpersonate::AdminCmdDataImpersonate(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/impersonate", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_OBJECT)
{
    size_t index = 1;
    bool found = false;
    duration = 0;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first parameter a valid player name or 'target' or 'text'
    if(words[index] == "text")
    {
        found = true;
        target = words[index];
        index++;
    }
    else if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first param player|target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        commandMod = words[index];
        commandMod.Downcase();
        if(commandMod == "say" || commandMod == "shout" || commandMod == "worldshout")
        {
            // command mode (if supplied)
            index++;
        }
        else if(commandMod == "anim")
        {
            index++;
            //check if a duration is specified
            if(words.GetCount() > index + 1)
            {
                duration = words.GetInt(index++);
            }
        }
        else
        {
            // default is to use say
            commandMod = "say";
        }
        text = words.GetTail(index);
    }
    else
    {
        valid = false;
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataImpersonate)

csString AdminCmdDataImpersonate::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + GetHelpMessagePartForTarget() + "] [say|shout|worldshout|anim] <text/[anim duration] anim name>\"\n"
           "If name is \"text\" the given text is used as it is.";
}

AdminCmdDataDeputize::AdminCmdDataDeputize(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/deputize", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first parameter a valid player name or 'target'
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first param player|target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        securityLevel = words[index];
        securityLevel.Downcase();
        if(securityLevel != "reset" && securityLevel != "player" && securityLevel != "tester" && securityLevel != "gm" && (securityLevel.StartsWith("gm",true) && securityLevel.Length() != 3) && securityLevel != "developer")
        {
            valid = false;
        }
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataDeputize)

csString AdminCmdDataDeputize::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + GetHelpMessagePartForTarget() + "] reset|player|test|gm|developer\"\n"
           "Different gm levels can be attained by using gm1..gm5 instead of gm.";
}

AdminCmdDataAward::AdminCmdDataAward(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/award", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first parameter a valid target of the command
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        index++;
    }

    // parse rewards
    if(!rewardList.ParseWords(index, words))
    {
        // when parsing failed give back the error
        ParseError(me, rewardList.error);
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataAward)

csString AdminCmdDataAward::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + GetHelpMessagePartForTarget() + "] {REWARD}\"\n" +
           rewardList.GetHelpMessage();
}

AdminCmdDataItemTarget::AdminCmdDataItemTarget(csString commandName, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget(commandName, ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA |ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET), stackCount(0)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first parameter a valid player name or 'target'
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first param player|target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        // try to parse an integer value
        stackCount = words.GetInt(index);
        if(stackCount)
        {
            itemName = words.GetTail(++index);
        }
        // stackcount now set to apply to all
        else if(words[index] == "all")
        {
            stackCount = -1;
            itemName = words.GetTail(++index);
        }
        // otherwise only item name is specified
        else
        {
            stackCount = 1;
            itemName = words.GetTail(index);
        }
        // test that there is something in itemName
        if(itemName.IsEmpty())
        {
            ParseError(me, "Missing item name");
        }
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE_WITH_CMD(AdminCmdDataItemTarget)

csString AdminCmdDataItemTarget::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + GetHelpMessagePartForTarget() + "] [quantity|'all'|''] [item|tria]\"";
}

AdminCmdDataCheckItem::AdminCmdDataCheckItem(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataItemTarget("/checkitem", msgManager, me, msg, client, words)
{
    /*
    size_t index = 1;
    bool found = false;

    // when first parameter a valid player name or 'target'
    if (ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first param player|target or the client has selected a target
    if (words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        stackCount = words.GetInt(index);
        if (stackCount)
        {
            itemName = words.GetTail(++index);
        }
        else
        {
            stackCount = 1;
            itemName = words.GetTail(index);
        }
    }
    */
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataCheckItem)

csString AdminCmdDataCheckItem::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + GetHelpMessagePartForTarget() + "] [quantity|''] [item|tria]\"";
}

AdminCmdDataSectorTarget::AdminCmdDataSectorTarget(csString commandName, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData(commandName), isClientSector(false), sectorInfo(NULL)
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if(!ParseTarget(msgManager, me, msg, client, words[1]))
    {
        // first word is not a sector name
        if(!GetSectorOfClient(client))
        {
            // and current sector of client is unknown
            ParseError(me, "Sector either missing or not valid");
        }
    }
}

bool AdminCmdDataSectorTarget::ParseTarget(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, csString target)
{
    bool targetIsSector = false;

    // reset sector name
    sectorName = "";
    // when string to parse is not empty
    if(!target.IsEmpty())
    {
        //if here was provided just get the sector of the client directly
        if(target == "here")
        {
            return GetSectorOfClient(client);
        }
        // retrieve and save sector information
        sectorInfo = psserver->GetCacheManager()->GetSectorInfoByName(target);
        sectorName = target;
        targetIsSector = true;
    }

    // when sectorinfo is not set, neither a sectorname was given
    // nor the current client position revealed a sector
    if(!sectorInfo)
    {
        // also reset the name
        sectorName = "";
        return false;
    }

    return targetIsSector;
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE_WITH_CMD(AdminCmdDataSectorTarget)

csString AdminCmdDataSectorTarget::GetHelpMessage()
{
    return "Syntax: \"" + command + " [sector]\"";
}

bool AdminCmdDataSectorTarget::GetSectorOfClient(Client* client)
{
    // Get the current sector
    iSector* here = NULL;
    if(client->GetActor())
    {
        here = client->GetActor()->GetSector();
    }

    //csVector3 pos;
    //client->GetActor()->GetPosition(pos,here);
    if(!here)
    {
        sectorName.Clear();
        sectorInfo = NULL;
        return false;
    }

    isClientSector = true;
    sectorName = here->QueryObject()->GetName();
    sectorInfo = psserver->GetCacheManager()->GetSectorInfoByName(sectorName);

    return true;
}


AdminCmdDataWeather::AdminCmdDataWeather(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataSectorTarget("/weather"), enabled(false)
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try to parse the first word as a sector or find the current sector
    // of client
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        index++;
    }
    // try to fetch the clients sector
    else if(!GetSectorOfClient(client))
    {
        // fetching failed
        ParseError(me, "Sector either missing or not valid");
    }

    // if a sector was found
    if(sectorInfo)
    {
        if(words.GetCount() == index + 2)
        {

            if(words[index] == "on")
            {
                enabled = true;
            }
            else if(words[index] == "off")
            {
                enabled = false;
            }
            else
            {
                ParseError(me, "State must be either on or off");
            }
            index++;
            if(words[index] == "rain")
            {
                type = psWeatherMessage::RAIN;
            }
            else if(words[index] == "fog")
            {
                type = psWeatherMessage::FOG;
            }
            else if(words[index] == "snow")
            {
                type = psWeatherMessage::SNOW;
            }
            else
            {
                ParseError(me, "Invalid weather type");
            }
        }
        // everything else is a syntax error
        // invalid or no subcommand
        else if(words.GetCount() == index)
        {
            ParseError(me, "Missing subcommand");
        }
        else
        {
            ParseError(me,"invalid subcommand >" +  words[index] + "<");
        }
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataWeather)

csString AdminCmdDataWeather::GetHelpMessage()
{
    return "Syntax: \"" + command + " [<sector>] on|off <type>\"";
}

AdminCmdDataWeatherEffect::AdminCmdDataWeatherEffect(csString commandName, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataSectorTarget(commandName), enabled(false), particleCount(4000), interval(600000), fadeTime(10000)
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try to parse the first word as a sector or find the current sector
    // of client
    if(ParseTarget(msgManager, me, msg, client, words[1]))
    {
        index++;
    }
    // try to fetch the clients sector
    else if(!GetSectorOfClient(client))
    {
        // fetching failed
        ParseError(me, "Sector either missing or not valid");
    }

    // if a sector was found
    if(sectorInfo)
    {
        if(words[index] == "stop")
        {
            enabled = false; // stop the weather effect
        }
        // three words left for parsing
        else if(words.GetCount() == index + 3)
        {
            enabled = true; // enable the weather effect
            particleCount = words.GetInt(2);
            interval = words.GetInt(3);
            fadeTime = words.GetInt(4);
        }
        else if(words[index] == "start")
        {
            enabled = true;
        }
        // everything else is a syntax error
        // invalid or no subcommand
        else if(words.GetCount() == index)
        {
            ParseError(me, "Missing subcommand");
        }
        else
        {
            ParseError(me,"Invalid subcommand >" +  words[index] + "<");
        }
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE_WITH_CMD(AdminCmdDataWeatherEffect)

csString AdminCmdDataWeatherEffect::GetHelpMessage()
{
    return "Syntax: \"" + command + " [sector] [[drops length fade]|stop]\"";
}

AdminCmdDataFog::AdminCmdDataFog(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataSectorTarget("/fog"), enabled(false), density(200), fadeTime(10000), interval(600000), r(200), g(200), b(200)
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try to parse the first word as a sector or find the current sector
    // of client
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        index++;
    }
    // try to fetch the clients sector
    else if(!GetSectorOfClient(client))
    {
        // fetching failed
        ParseError(me, "No valid target sector given");
    }

    // if a sector was found
    if(sectorInfo)
    {
        //This turns off the fog.
        if(words.GetCount() == index+1 && (words[index] == "stop" || words[index] == "-1"))
        {
            enabled = false;
        }
        else if(words.GetCount() == index+6)
        {
            enabled = true;
            density = words.GetInt(index++);
            r = words.GetInt(index++);
            g = words.GetInt(index++);
            b = words.GetInt(index++);
            interval = words.GetInt(index++);
            fadeTime = words.GetInt(index++);
        }
        else if(words[index] == "start")
        {
            enabled = true;
        }
        // no subcommand
        else if(words.GetCount() == index)
        {
            ParseError(me, "Missing subcommand");
        }
        // syntax error
        else
        {
            ParseError(me,"Invalid subcommand >" +  words[index] + "<");
        }
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataFog)

csString AdminCmdDataFog::GetHelpMessage()
{
    return "Syntax: \"" + command + " [sector] [density [r g b lenght fade]|stop]\"";
}

AdminCmdDataModify::AdminCmdDataModify(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/modify", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), attributeList("pickupable unpickable transient npcowned collide settingitem pickupableweak")
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first word is a valid target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first word is a target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        subCommand = words[index++];
        if(subCommand == "intervals" && words.GetCount() == index+2)
        {
            interval = words.GetInt(index++);
            maxinterval = words.GetInt(index++);
        }
        else if(subCommand == "amount" && words.GetCount() == index+1)
        {
            amount = words.GetInt(index);
        }
        else if(subCommand == "picklevel" && words.GetCount() == index+1)
        {
            level = words.GetInt(index);
        }
        else if(subCommand == "range" && words.GetCount() == index+1)
        {
            range = words.GetInt(index);
        }
        else if(subCommand == "move" && (words.GetCount() == index+3 || words.GetCount() == index+4))
        {
            // If rot wasn't specified (6 words), then words.GetFloat(6) will be 0 (atof behavior).
            x = words.GetFloat(index++);
            y = words.GetFloat(index++);
            z = words.GetFloat(index++);
            rot = words.GetFloat(index);
        }
        else if(subCommand == "pickskill")
        {
            skillName = words[index];
        }
        else if(attributeList.IsSubCommand(subCommand))
        {
            if(words.GetCount() == index+1)
            {
                // on/off setting
                if(words[index] == "true")
                {
                    enabled = true;
                }
                else if(words[index] == "false")
                {
                    enabled = false;
                }
                else
                {
                    ParseError(me,"Invalid settings");
                }
            }
            else
            {
                ParseError(me, "attribute is missing true/false setting.");
            }
        }
        // everything else is a syntax error
        else if(subCommand != "remove")
        {
            ParseError(me,"Invalid subcommand");
        }
    }
    // no target -> syntax error
    else
    {
        ParseError(me,"No target given");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataModify)

csString AdminCmdDataModify::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " <SUBCOMMAND>\"\n"
           "SUBCOMMAND: intervals <interval> <intervalcap>\n"
           "            amount <amount>\n"
           "            picklevel <level>\n"
           "            range <range>\n"
           "            move <x> <y> <z> [<y>]\n"
           "            pickskill <skillname>\n"
           "            ATTRIBUTE true|false\n"
           "ATTRIBUTE: " + attributeList.GetHelpMessage();
}

AdminCmdDataMorph::AdminCmdDataMorph(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/morph", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_EID)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first word is a valid target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // always allow list whathever there is a target or not
    if(words.GetCount() == index + 1 && words[index] == "list")
    {
        subCommand = words[index++];
        if(words.GetCount() > index + 1)
            ParseError(me, "Subcommand " + subCommand + " does not have any parameters");
    }
    // when first word is a target or the client has selected a target
    else if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        // test for a subcommand then
        if(words[index] == "list" || words[index] == "reset")
        {
            subCommand = words[index++];
            if(words.GetCount() > index + 1)
                ParseError(me, "Subcommand " + subCommand + " does not have any parameters");
        }
        else
        {
            raceName = words[index++];
            //if there is another entry (specifying the gender)
            //use it else use a default.
            if(words.GetCount() >= index + 1)
            {
                genderName = words[index++];
            }
            else
            {
                genderName = "m";
            }

            // if there is another entry, it's the scale
            if(words.GetCount() >= index + 1)
            {
                scale = words[index++];
            }
            else
            {
                scale = "1";
            }

        }
    }
    // no target -> syntax error
    else
    {
        ParseError(me,"No target given");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataMorph)

csString AdminCmdDataMorph::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + "\" racename|list|reset [gender] [scale]";
}

AdminCmdDataScale::AdminCmdDataScale(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/scale", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_EID)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first word is a valid target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }

    // when first word is a target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        // test for a subcommand then
        if(words[index] == "reset")
        {
            subCommand = words[index++];
            if(words.GetCount() > index + 1)
                ParseError(me, "Subcommand " + subCommand + " does not have any parameters");
        }
        else
        {
            // if there is another entry, it's the scale
            if(words.GetCount() >= index + 1)
            {
                scale = words[index++];
            }
            else
            {
                scale = "";
            }

        }
    }
    // no target -> syntax error
    else
    {
        ParseError(me,"No target given");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataScale)

csString AdminCmdDataScale::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + "\" reset|<scale>";
}

AdminCmdDataSetSkill::AdminCmdDataSetSkill(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/setskill", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), sourcePlayer(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), subCommand(),  skillData("",0,0,false)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first word is a valid target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
        // target is required
    }
    else
    {
        psserver->SendSystemError(me->clientnum,"Provided target is invalid.");
        return;
    }

    // when first word is a target or the client has selected a target
    if(words.GetCount() == index + 2 && (found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET)))
    {
        if(words[index] == "copy")
        {
            subCommand = words[index++];
            // if parsing the word fails
            if(!sourcePlayer.ParseTarget(msgManager,me,msg,client,words[index]))
            {
                // no source specified or
                // target is already the clients target
                if((IsTargetType(ADMINCMD_TARGET_CLIENTTARGET) && sourcePlayer.IsTargetType(ADMINCMD_TARGET_CLIENTTARGET)))
                {
                    ParseError(me,"No source given");
                }
            }
        }
        else
        {
            skillData.skillName = words[index++];
            if(words[index].GetAt(0) == '-' || words[index].GetAt(0) == '+')
            {
                skillData.relativeSkill = true;
            }
            skillData.skillDelta = words.GetInt(index);
        }
    }
    else if(words.GetCount() > index + 2)
    {
        ParseError(me, "Too many arguments");
    }
    else if(words.GetCount() < index + 2)
    {
        ParseError(me, "Not enough arguments");
    }
    // no target -> syntax error
    else
    {
        ParseError(me,"No target given");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSetSkill)

csString AdminCmdDataSetSkill::GetHelpMessage()
{
    return "Syntax: \"" + command + " [TARGET] <skillname>|'all' [+-]<value>\"\n"
           " or \"" + command + " [TARGET] copy <source>\"\n"
           "TARGET: " + GetHelpMessagePartForTarget();
}

AdminCmdDataSet::AdminCmdDataSet(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/set", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_ME), subCommandList("list gm player"), attributeList("invincible invincibility invisibility invisible viewall nevertired nofalldamage infiniteinventory questtester infinitemana instantcast givekillexp attackable buddyhide"), setting(AdminCmdOnOffToggleParser::ADMINCMD_SETTING_TOGGLE)
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first word is a valid target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first word is a target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        // sub commands for doing meta stuff of the command
        if(subCommandList.IsSubCommand(words[index]))
        {
            subCommand = words[index++];
            if(words.GetCount() != index)
            {
                ParseError(me,"Subcommand: " + subCommand + " does not support parameters");
            }
        }
        // check for specific attribute names for setting then on/off
        else if(attributeList.IsSubCommand(words[index]))
        {
            attribute = words[index++];
            if(words.GetCount() == index+1)
            {
                // when parsing on|off|toggle fails
                if(!setting.ParseWord(words[index]))
                {
                    ParseError(me, setting.error);
                }
            }
            else if(words.GetCount() != index)
            {
                ParseError(me,"Attribute: " + subCommand + " too many parameters");
            }
        }
        // not a subcommand or attribute
        else
        {
            ParseError(me, words[index] + " is not a supported attribute");
        }
    }
    // no target -> syntax error
    else
    {
        ParseError(me,"No target given");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSet)

csString AdminCmdDataSet::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() +
           "|" + attributeList.GetHelpMessage() + " [" +
           setting.GetHelpMessage() + "]\"";
}

AdminCmdDataSetLabelColor::AdminCmdDataSetLabelColor(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/setlabelcolor", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA  | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_EID), labelTypeList("normal alive dead npc tester gm gm1 player")
{
    size_t index = 1;
    bool found = false;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // when first word is a valid target
    if(ParseTarget(msgManager, me, msg, client, words[index]))
    {
        found = true;
        index++;
    }
    // when first word is a target or the client has selected a target
    if(words.GetCount() >= index + 1 && (found || (IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))))
    {
        if(labelTypeList.IsSubCommand(words[index]))
        {
            labelType = words[index++];
        }
        else
        {
            ParseError(me, "Not a valid type of color: " + labelType);
        }
        // no further parameters
        if(words.GetCount() != index)
        {
            ParseError(me, "Command: " + labelType + " doesn't have parameters");
        }
    }
    // no target -> syntax error
    else
    {
        ParseError(me,"No target given");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSetLabelColor)

csString AdminCmdDataSetLabelColor::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() +
           " " + labelTypeList.GetHelpMessage() + "\"";
}

AdminCmdDataAction::AdminCmdDataAction(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataSectorTarget("/action"), subCommandList("create_entrance")
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // only one subcommand with 5 parameters
    if(words.GetCount() >= 5)
    {
        if(subCommandList.IsSubCommand(words[index]))
        {
            subCommand = words[index++];
            // parse a word containing a sector target
            if(ParseTarget(msgManager,me,msg,client,words[index]))
            {
                index++;
                guildName = words[index++];
                description = words.GetTail(index);
            }
            // no target -> syntax error
            else
            {
                ParseError(me,"Not a sector:" + words[2]);
            }
        }
        // not a sub command
        else
        {
            ParseError(me,"Unknown subcommand: " + words[index]);
        }
    }
    // not enough words
    else
    {
        ParseError(me,"Not enough arguments");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataAction)

csString AdminCmdDataAction::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + " <sector> <guildname> <description>\"";
}

AdminCmdDataPath::AdminCmdDataPath(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/path"), aliasSubCommandList("add remove rotation"), wpList("waypoints points"), subCmd(), defaultRadius(2.0),radius(0.0)
{
    // register sub commands with their extended help message
    subCommandList.Push("adjust","[<radius>]");
    subCommandList.Push("alias", "[add|remove|rotation]<alias> [<rotation angle>] [<search radius>]");
    subCommandList.Push("display","[points|waypoints]");
    subCommandList.Push("flagclear","[wp|path] <flag> [<search radius>]");
    subCommandList.Push("flagset","[wp|path] <flag> [<search radius>]");
    subCommandList.Push("format","<format> [first]");
    subCommandList.Push("help","[sub command]");
    subCommandList.Push("hide","[points|waypoints]");
    subCommandList.Push("info","[<search radius>]");
    subCommandList.Push("move","[wp|point] <id>|<name>");
    subCommandList.Push("point","add|remove|insert");
    subCommandList.Push("radius","<new radius> [<search radius>]");
    subCommandList.Push("remove","[<search radius>]");
    subCommandList.Push("rename","<name> [<search radius>]");
    subCommandList.Push("select","<search radius>");
    subCommandList.Push("split","<radius> [wp flags]");
    subCommandList.Push("start","<radius> [wp flags] [path flags]");
    subCommandList.Push("stop","<radius> [wp flags]");

    subCommandList.Push("end","See /path help stop");
    subCommandList.Push("show","See /path help display");

    // when help is requested, return immediate
    if(IsHelp(words[1]))
    {
        subCmd = words[2]; // Next word might be a sub Cmd.
        return;
    }

    size_t index = 1;

    subCmd = words[index++];
    if(subCommandList.IsSubCommand(subCmd))
    {
        if(subCmd == "adjust")
        {
            radius = words.GetFloat(index);
        }
        else if(subCmd == "alias")
        {
            aliasSubCmd = words[index++];

            if(aliasSubCommandList.IsSubCommand(aliasSubCmd))
            {
                if(aliasSubCmd == "add")
                {
                    if(!words.GetString(index++,alias))
                    {
                        ParseError(me,"Missing argument <alias>");
                        return;
                    }

                    rotationAngle = words.GetFloat(index++)*PI/180.0;  // Convert from degree to radians

                    radius = words.GetFloat(index++);
                }
                else if(aliasSubCmd == "remove")
                {
                    if(!words.GetString(index++,alias))
                    {
                        ParseError(me,"Missing argument <alias>");
                        return;
                    }
                }
                else if(aliasSubCmd == "rotation")
                {
                    if(!words.GetString(index++,alias))
                    {
                        ParseError(me,"Missing argument <alias>");
                        return;
                    }

                    if(!words.GetFloat(index++,rotationAngle))
                    {
                        ParseError(me,"Missing argument <rotation angle> or isn't a float value.");
                        return;
                    }
                    rotationAngle = rotationAngle*PI/180.0;  // Convert from degree to radians
                }
            }
            else
            {
                ParseError(me,"Unkown subcommand "+ aliasSubCmd +" for alias.");
                return;
            }
        }
        else if(subCmd == "display" || subCmd == "show")
        {
            // Show is only an alias so make sure subCmd is display
            subCmd = "display";
            if(words.GetCount() == index+1 && wpList.IsSubCommand(words[index]))
            {
                if(words[index] == "waypoints")
                {
                    cmdTarget = "W";
                }
                else
                {
                    cmdTarget = "P";
                }
            }
            // error if there is a string
            else if(words.GetCount() != index)
            {
                ParseError(me, "Waypoints or points expected, but >" + words[index] + "< found");
            }
        }
        // flagset|flagclear and at least one further word
        else if((subCmd == "flagset" || subCmd == "flagclear") && words.GetCount() >= index+2)
        {
            csString wpOrPath = words[index++];
            if(wpOrPath == "wp")
            {
                wpOrPathIsWP = true;
            }
            else if(wpOrPath == "path")
            {
                wpOrPathIsWP = false;
            }
            else
            {
                ParseError(me,"You have to select either wp or path.");
            }
            flagName = words[index++];    // Flag
            radius = words.GetFloat(index++); // try to parse an optional radius
        }
        else if(subCmd == "format" && words.GetCount() >= index+1)
        {
            waypointPathName = words[index++];    // Format
            firstIndex = words.GetInt(index++); // First waypointnameindex
        }
        else if(subCmd == "hide")
        {
            if(words.GetCount() == index+1 && wpList.IsSubCommand(words[index]))
            {
                if(words[index] == "waypoints")
                {
                    cmdTarget = "W";
                }
                else
                {
                    cmdTarget = "P";
                }
            }
            // error if there is a string
            else if(words.GetCount() != index)
            {
                ParseError(me,"Waypoints or points expected, but >" + words[index] + "< found");
            }
        }
        else if(subCmd == "info")
        {
            radius = words.GetFloat(index++);
        }
        // move reqire at least one further word
        else if(subCmd == "move")
        {
            csString wpOrPath = words[index++];
            if(wpOrPath != "wp" && wpOrPath != "point")
            {
                ParseError(me,"You have to select either wp or point.");
            }
            else
            {
                wpOrPathIsWP = (wpOrPath == "wp");

                int id = -1;
                if(words.GetCount() <= index)
                {
                    ParseError(me,"Expected either id or name.");
                }
                else if(words.GetInt(index,id))    // Is it an id?
                {
                    // We got an id
                    waypointPathIndex = id;
                    wpOrPathIsIndex = true;
                }
                else
                {
                    // We got a name
                    waypointPathName = words[index];
                    wpOrPathIsIndex = false;

                    if(!wpOrPathIsWP)
                    {
                        ParseError(me,"Name not supported for points.");
                    }
                }
            }
        }
        else if(subCmd == "point")
        {
            csString subSubCmd = words[index++];
            if(subSubCmd.IsEmpty())
            {
                ParseError(me,"No sub command; add, remove, or insert given.");
            }
            else if(subSubCmd == "add")
            {
                subCmd = "point add";
            }
            else if(subSubCmd == "remove")
            {
                subCmd = "point remove";
            }
            else if(subSubCmd == "insert")
            {
                subCmd = "point insert";
            }
        }
        else if(subCmd == "radius")
        {
            // New Radius is mandatory
            if(!words.GetFloat(index++,newRadius))
            {
                ParseError(me, "Radius not given or not a float value.");
            }
            // Search radius is optional
            radius = words.GetFloat(index++);
        }
        else if(subCmd == "remove")
        {
            radius = words.GetFloat(index++);
        }
        else if(subCmd == "rename")
        {
            waypoint = words[index++];
            if(waypoint.IsEmpty())
            {
                ParseError(me, "Missing waypoint");
            }

            radius = words.GetFloat(index++);
        }
        else if(subCmd == "select")
        {
            radius = words.GetFloat(index++);
        }
        else if(subCmd == "split")
        {
            radius = words.GetFloat(index++);
            waypointFlags = words[index++]; // Waypoint flags
        }
        // start and at least radius is given
        else if(subCmd == "start" && words.GetCount() == index+1)
        {
            radius = words.GetFloat(index++);
            waypointFlags = words[index++]; // Waypoint flags
            pathFlags = words[index++]; // Path flags
        }
        // stop and at least radius is given
        else if((subCmd == "stop" || subCmd == "end") && words.GetCount() == index+1)
        {
            // end is an alias (overwrite it)
            subCmd = "stop";
            radius = words.GetFloat(index++);
            waypointFlags = words[index++];
        }
    }
    // no target -> syntax error
    else
    {
        ParseError(me,"No subcommand given");
    }
    // set radius to defaultvalue, if it is zero (for all subcommands)
    if(radius == 0.0)
    {
        radius = defaultRadius;
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataPath)

csString AdminCmdDataPath::GetHelpMessage()
{
    if(!subCmd.IsEmpty())
    {
        return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage(subCmd) + "\"";
    }
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + " [options]\"";
}

AdminCmdDataLocation::AdminCmdDataLocation(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/location")
{

    subCommandList.Push("add","<type> <name> <radius> [<rotation angle>]");
    subCommandList.Push("adjust","");
    subCommandList.Push("display","");
    subCommandList.Push("help","[sub command]");
    subCommandList.Push("hide","");
    subCommandList.Push("info","");
    subCommandList.Push("insert","");
    subCommandList.Push("list","");
    subCommandList.Push("radius","<radius> [<search radius>]");
    subCommandList.Push("select","[<search radius>]");
    subCommandList.Push("type","add|delete [<type>]");

    subCommandList.Push("show","See /path help display");

    // when help is requested, return immediate
    if(IsHelp(words[1]))
    {
        subCommand = words[2]; // Next word might be a sub Cmd.
        return;
    }

    size_t index = 1;

    // first word must be a sub command
    if(subCommandList.IsSubCommand(words[index]))
    {
        subCommand = words[index++];
        if(subCommand == "add")
        {
            // locationtype is required
            if(words.GetCount() >= index + 1)
            {
                locationType = words[index++];
            }
            else
            {
                ParseError(me, "Locationtype is a required argument");
            }
            // locatinname is required
            if(words.GetCount() >= index + 1)
            {
                locationName = words[index++];
            }
            else
            {
                ParseError(me, "Locationname is a required argument");
            }

            if(!words.GetFloat(index++,radius))
            {
                ParseError(me, "Required radius not given or not an float value.");
            }

            // Get optional rotation angle for this location.
            if(!words.GetFloat(index++,rotAngle))
            {
                rotAngle = 0.0;
            }

        }
        else if(subCommand == "display" || subCommand == "show")
        {
            // Show is only an alias so make sure subCmd is display
            subCommand = "display";
        }
        else if(subCommand == "radius")
        {
            // New Radius is mandatory
            if(!words.GetFloat(index++,radius))
            {
                ParseError(me, "Radius not given or not a float value.");
            }
            // Search radius is optional
            searchRadius = words.GetFloat(index++); // Will return 0.0 if not pressent
            if(searchRadius <= 0.0)
            {
                searchRadius = 10.0;
            }
        }
        else if(subCommand == "select")
        {
            // Search radius is optional
            searchRadius = words.GetFloat(index++); // Will return 0.0 if not pressent
            if(searchRadius <= 0.0)
            {
                searchRadius = 10.0;
            }
        }
        else if(subCommand == "type")
        {
            csString subSubCmd = words[index++];
            if(subSubCmd.IsEmpty())
            {
                ParseError(me,"No sub command; add or delete given.");
            }
            else if(subSubCmd == "add")
            {
                subCommand = "type add";
            }
            else if(subSubCmd == "delete" || subSubCmd == "remove")
            {
                subCommand = "type delete";
            }
            if(!words.GetString(index++,locationType))
            {
                ParseError(me,"No type given.");
            }
        }

    }
    // first wird is not a valid subcommand
    else
    {
        ParseError(me, "Invalid or missing subcommand");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataLocation)

csString AdminCmdDataLocation::GetHelpMessage()
{
    if(!subCommand.IsEmpty())
    {
        return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage(subCommand) + "\"";
    }
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + " [options]\"";
}

AdminCmdDataGameMasterEvent::AdminCmdDataGameMasterEvent(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/event"), subCommandList("help list"), subCmd(), player(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET)
{
    // register subcommands along with their help message
    subCommandList.Push("create","<name> <description>");
    subCommandList.Push("reward",(csString)"{REWARD}" + "\n" + rewardList.GetHelpMessage());
    subCommandList.Push("remove","<player>");
    subCommandList.Push("complete","[<name>]");
    subCommandList.Push("register","[range <range> | <player>]");
    subCommandList.Push("control","<name>");
    subCommandList.Push("discard","<name>");

    // when help is requested, return immediate
    if(IsHelp(words[1]))
    {
        subCmd = words[2]; // Next word might be a sub Cmd.
        return;
    }

    size_t index = 1;

    // test if the first word is a subcommand
    if(subCommandList.IsSubCommand(words[index]))
    {
        subCmd = words[index++];

        if(subCmd == "create")
        {
            // must have a description
            if(words.GetCount() >= index+2)
            {
                gmeventName = words[index++];
                gmeventDesc = words.GetTail(index);
            }
            else
                // missing desc
            {
                ParseError(me, "Missing event description");
            }
        }
        else if(subCmd == "register")
        {
            // 'register' expects either 'range' numeric value or a player name.
            if(words[index] == "range")
            {
                index++;
                range = words.GetFloat(index);
                rangeSpecifier = IN_RANGE;
            }
            // parse a player name
            else if(player.ParseTarget(msgManager, me, msg, client, words[index]))
            {
                rangeSpecifier = INDIVIDUAL;
            }
            // not range nor a player name
            else if(words.GetCount() != index)
            {
                ParseError(me, "Not a range or player name");
            }
            // missing
            else if(!player.IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
            {
                ParseError(me, "Missing range or player name");
            }
        }
        else if(subCmd == "reward")
        {
            // "/event reward [range # | all | [player_name]] <#> item"

            if(words[index] == "all")
            {
                commandMod = words[index++];
                rangeSpecifier = ALL;
            }
            else if(words[index] == "range")
            {
                commandMod = words[index++];
                rangeSpecifier = IN_RANGE;
                range = words.GetFloat(index++);
            }
            // if word is a valid target
            else if(player.ParseTarget(msgManager, me, msg, client, words[index]))
            {
                rangeSpecifier = INDIVIDUAL;  // expecting a player by target
                index++;
            }
            // if client has a target selected
            else if(player.IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
            {
                rangeSpecifier = INDIVIDUAL;
            }
            // when there are still parameters left
            else if(words.GetCount() >= index+1)
            {
                ParseError(me, "Target is not a valid player");
            }
            else
            {
                ParseError(me, "No reward target given. Either select a target or specify a target");
            }

            // try to parse rewards from the remaining words of the command line
            if(valid && !rewardList.ParseWords(index, words))
            {
                // if this fails, retrieve the error message from the parser
                ParseError(me, rewardList.error);
            }
        }
        else if(subCmd == "remove")
        {
            if(player.ParseTarget(msgManager,me,msg,client,words[index]))
            {
                index++;
            }
            else if(words.GetCount() >= index && !player.IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
            {
                ParseError(me, "Too many parameters for " + subCmd);
            }
        }
        else if(subCmd == "complete")
        {
            gmeventName = words.Get(index++);
        }
        else if(subCmd == "list")
        {
            // No params
        }
        else if(subCmd == "control")
        {
            gmeventName = words[index++];
        }
        else if(subCmd == "discard")
        {
            gmeventName = words[index++];
        }
    }
    else
    {
        ParseError(me, "Missing subcommand");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataGameMasterEvent)

csString AdminCmdDataGameMasterEvent::GetHelpMessage()
{
    if(!subCmd.IsEmpty() || subCmd == "help")
    {
        return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage(subCmd) + "\"";
    }
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + " [options]\"";
}

AdminCmdDataHire::AdminCmdDataHire(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/hire"), subCommandList("help list"), subCmd(), owner(NULL), hiredNPC(NULL)
{
    // register subcommands along with their help message
    subCommandList.Push("start","");
    subCommandList.Push("type","<Name> <NPC Type>");
    subCommandList.Push("master","<master NPC ID>");
    subCommandList.Push("confirm","");
    subCommandList.Push("script","");
    subCommandList.Push("release","<Hired NPC>");

    // when help is requested, return immediate
    if(IsHelp(words[1]))
    {
        subCmd = words[2]; // Next word might be a sub Cmd.
        return;
    }

    owner = client->GetActor();

    size_t index = 1;

    if(words.GetCount() <= index)
    {
        ParseError(me, "No subcommand.");
    }
    else if(subCommandList.IsSubCommand(words[index]))
    {
        subCmd = words[index++];

        if(subCmd == "start")
        {
            if(words.GetCount() > index)
            {
                ParseError(me, "To many parameters.");
            }
        }
        else if(subCmd == "type")
        {
            if (words.GetCount() <= index)
            {
                ParseError(me, "Parameter not specified.");
            }
            else
            {
                typeName = words[index++];
            }

            if (words.GetCount() <= index)
            {
                ParseError(me, "Parameter not specified.");
            }
            else
            {
                typeNPCType = words[index++];
            }
            
            if(words.GetCount() > index)
            {
                ParseError(me, "To many parameters.");
            }
        }
        else if(subCmd == "master")
        {
            if (words.GetCount() <= index)
            {
                ParseError(me, "Parameter not specified.");
            }
            else
            {
                masterPID = words.GetInt(index++);
                if (!masterPID.IsValid())
                {
                    ParseError(me, "Master PID must be none zero");
                }
            }
            
            if(words.GetCount() > index)
            {
                ParseError(me, "To many parameters.");
            }
        }
        else if(subCmd == "confirm")
        {
            if(words.GetCount() > index)
            {
                ParseError(me, "To many parameters.");
            }
        }
        else if(subCmd == "script")
        {
            hiredNPC = dynamic_cast<gemNPC*>(client->GetTargetObject());
            if (!hiredNPC)
            {
                ParseError(me, "No hired NPC targeted.");
            }
            
            if(words.GetCount() > index)
            {
                ParseError(me, "To many parameters.");
            }
        }
        else if(subCmd == "release")
        {
            hiredNPC = dynamic_cast<gemNPC*>(client->GetTargetObject());
            if (!hiredNPC)
            {
                ParseError(me, "No hired NPC targeted.");
            }
            
            if(words.GetCount() > index)
            {
                ParseError(me, "To many parameters.");
            }
        }
        else
        {
            ParseError(me, "Unhandled subcommand.");
        }
        
    }
    else
    {
        ParseError(me, "Unknown subcommand.");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataHire)

csString AdminCmdDataHire::GetHelpMessage()
{
    if(!subCmd.IsEmpty() || subCmd == "help")
    {
        return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage(subCmd) + "\"";
    }
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + " [options]\"";
}

AdminCmdDataBadText::AdminCmdDataBadText(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/badtext", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client seleted a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // try to read the next parameter as an integer
        first = atoi(words[index]);
        if(first)
        {
            index++;
            // try to read the next parameter as an integer
            last = atoi(words[index]);
            if(last)
            {
                index++;
                if(last < first)
                {
                    ParseError(me, "Last must not be smaller than first");
                }
                // TODO: No Length check here ...
            }
            else
            {
                ParseError(me, "Last is 0 or missing");
            }
        }
        else
        {
            ParseError(me, "Value is 0 or missing");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataBadText)

csString AdminCmdDataBadText::GetHelpMessage()
{
    return "Syntax: \"" + command + " [target|npc] <first> <last>\"";
}

AdminCmdDataQuest::AdminCmdDataQuest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/quest", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE),
      subCommandList("complete list discard assign setvariable unsetvariable")
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // gets a list of quests of the target
        if(words.GetCount() == index)
        {
            subCmd == "list";
        }
        // check for other subcommands
        else if(subCommandList.IsSubCommand(words[index]))
        {
            subCmd = words[index++];
            // all commands can utilize a questname as an argument
            if(words.GetCount() == index + 1)
            {
                questName = words[index++];

                // Check if this is a request to list variables.
                if(subCmd == "list" && questName.CompareNoCase("variables"))
                {
                    subCmd = "list variables";
                }
            }
            else if(words.GetCount() != index)
            {
                ParseError(me, "Too many arguments");
            }
        }
        else
        {
            ParseError(me, "Not a valid subcommand " + words[index]);
        }
    }
    // no valid target
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataQuest)

csString AdminCmdDataQuest::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + subCommandList.GetHelpMessage() +
           "] [questname|variable]|variables\"\n";
}

AdminCmdDataSetQuality::AdminCmdDataSetQuality(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/setquality", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA |ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try the first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // first word is a target or the client selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // required argument quality
        if(words.GetCount() >= index +1)
        {
            quality = words.GetFloat(index++);
            qualityMax = 0.0;
            // optional argument maximum quality
            if(words.GetCount() == index + 1)
            {
                qualityMax = words.GetFloat(index++);
            }
            else if(words.GetCount() > index + 1)
            {
                ParseError(me, "Too many arguments");
            }
        }
        else
        {
            ParseError(me, "Missing quality");
        }
    }
    // no valid target
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSetQuality)

csString AdminCmdDataSetQuality::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " <quality> [<qualityMax>]\"";
}

AdminCmdDataSetTrait::AdminCmdDataSetTrait(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/settrait", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // check first word (or second) to be list
    if(words[index] == "list")
    {
        subCmd = words[index++];
        // 2 required parameters
        if(words.GetCount() == index + 2)
        {
            race = words[index++];
            csString gen = words[index++];

            // parse the different gender types
            if(gen == "m" || gen == "male")
            {
                gender = PSCHARACTER_GENDER_MALE;
            }
            else if(gen == "f" || gen == "female")
            {
                gender = PSCHARACTER_GENDER_FEMALE;
            }
            else if(gen == "n" || gen == "none")
            {
                gender = PSCHARACTER_GENDER_NONE;
            }
            else
            {
                ParseError(me, "Invalid gender");
            }
        }
        else
        {
            ParseError(me, "Not enough arguments");
        }
    }
    // if first word a target or client selected valid target
    else if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // required parameter is the trait name
        if(words.GetCount() == index +1)
        {
            traitName = words[index++];
        }
        else
        {
            ParseError(me, "Not enough arguments for setting trait");
        }
    }
    else
    {
        ParseError(me, "Neither 'list' command invoked, nor target selected");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSetTrait)

csString AdminCmdDataSetTrait::GetHelpMessage()
{
    return "Syntax: \"" + command + " [" + GetHelpMessagePartForTarget() +
           "] <trait>\"\n"
           + command + " list <race> <gender>";
}

AdminCmdDataSetItem::AdminCmdDataSetItem(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/setitemname", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)

{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // name is mandatory
        if(words.GetCount() >= index + 1)
        {
            name = words[index++];
            // description is optional
            if(words.GetCount() == index + 1)
            {
                description = words[index++];
            }
            else if(words.GetCount() > index)
            {
                ParseError(me, "Too many arguments");
            }
        }
        else
        {
            ParseError(me, "Wrong number of arguments");
        }
    }
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSetItem)

csString AdminCmdDataSetItem::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " <newname> [<newdescription>]\"";
}

AdminCmdDataReload::AdminCmdDataReload(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/reload"), itemID(0)
{
    subCommandList.Push("item","<itemID> - reloads the specified item");
    subCommandList.Push("serveroptions","#Reload all the server options");
    subCommandList.Push("mathscript","#Reload all the mathscripts");
    subCommandList.Push("path","#Reload all the path data");
    subCommandList.Push("locations","#Reload all the location data");

    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if(words.GetCount() > index && subCommandList.IsSubCommand(words[index]))
    {
        subCmd = words[index++];
    }

    if(subCmd == "item")
    {
        itemID = words.GetInt(index++);
        if(itemID == 0)
        {
            ParseError(me, "Missing or invalid item id");
        }
    }

    if(words.GetCount() > index)
    {
        ParseError(me, "Too many arguments");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataReload)

csString AdminCmdDataReload::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + "\"";
}

AdminCmdDataListWarnings::AdminCmdDataListWarnings(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/listwarnings", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
{
    size_t index = 1;
    bool found;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    if(!found && !IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataListWarnings)

csString AdminCmdDataListWarnings::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + "\"";
}

AdminCmdDataDisableQuest::AdminCmdDataDisableQuest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/disablequest")
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if(words.GetCount() >= index + 1)
    {
        questName = words[index++];  //name of the quest
        if(words[index] == "save")  //save = save to db
            saveToDb = true;
        else
            saveToDb = false;
    }
    else
    {
        ParseError(me, "Missing questname");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataDisableQuest)

csString AdminCmdDataDisableQuest::GetHelpMessage()
{
    return "Syntax: \"" + command + " <questname> [save]\"";
}

AdminCmdDataSetKillExp::AdminCmdDataSetKillExp(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/setkillexp", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;
    expValue = 0;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // if first word a target or client selected a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // required argumet is the experience value
        if(words.GetCount() == index +1)
        {
            expValue = words.GetInt(index);
        }
        else
        {
            ParseError(me, "Missing kill experience value.");
        }
    }
    // no valid target
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataSetKillExp)

csString AdminCmdDataSetKillExp::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " [<exp>]\"";
}

AdminCmdDataAssignFaction::AdminCmdDataAssignFaction(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdDataTarget("/assignfaction", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
{
    size_t index = 1;
    bool found;
    factionPoints = 0;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    // try to parse first word as a target
    if((found = ParseTarget(msgManager, me, msg, client, words[index])))
    {
        index++;
    }

    // when first word is a target or the client target has a valid target
    if(found || IsTargetType(ADMINCMD_TARGET_CLIENTTARGET))
    {
        // two required arguments
        if(words.GetCount() == index + 2)
        {
            factionName = words[index++];
            factionPoints = words.GetInt(index++);
        }
        // otherwise too long/short
        else if(words.GetCount() < index + 2)
        {
            ParseError(me, "Not enough arguments");
        }
        else
        {
            ParseError(me, "Too many arguments");
        }
    }
    // no valid target
    else
    {
        ParseError(me, "No target specified");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataAssignFaction)

csString AdminCmdDataAssignFaction::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + GetHelpMessagePartForTarget() + " <factionname> <points>\"";
}

AdminCmdDataServerQuit::AdminCmdDataServerQuit(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/serverquit")
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    size_t index = 1;
    if(words.GetCount() >= index + 1)
    {
        time = words.GetInt(index++);
        //optional parameter
        if(words.GetCount() == index + 1)
            reason = words.GetTail(index);
    }
    else
    {
        ParseError(me, "Not enough arguments");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataServerQuit)

csString AdminCmdDataServerQuit::GetHelpMessage()
{
    return "Syntax: \"" + command + " [-1/time] <reason>\"";
}

AdminCmdDataNPCClientQuit::AdminCmdDataNPCClientQuit(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/npcclientquit")
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataNPCClientQuit)

csString AdminCmdDataNPCClientQuit::GetHelpMessage()
{
    return "Syntax: \"" + command + "\"";
}

AdminCmdDataSimple::AdminCmdDataSimple(csString commandName, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData(commandName)
{
    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE_WITH_CMD(AdminCmdDataSimple)

csString AdminCmdDataSimple::GetHelpMessage()
{
    return "Syntax: \"" + command + "\"";
}

AdminCmdDataRndMsgTest::AdminCmdDataRndMsgTest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/rndmsgtest")
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if(words.GetCount() == index + 1)
    {
        if(words[index] == "ordered")
            sequential = true;
        else
            sequential = false;
    }
    else
    {
        ParseError(me, "Missing text");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataRndMsgTest)

csString AdminCmdDataRndMsgTest::GetHelpMessage()
{
    return "Syntax: \"" + command + " [ordered]\"";
}

AdminCmdDataList::AdminCmdDataList(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/list"), subCommandList("map")
{
    size_t index = 1;

    // when help is requested, return immediate
    if(IsHelp(words[1]))
        return;

    if(words.GetCount() == index + 1 && subCommandList.IsSubCommand(words[index]))
    {
        subCommand = words[index++];
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataList)

csString AdminCmdDataList::GetHelpMessage()
{
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + "\"";
}


AdminCmdDataTime::AdminCmdDataTime(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words)
    : AdminCmdData("/time")
{
    // register sub commands with their extended help message
    subCommandList.Push("show","#Show the current game time");
    subCommandList.Push("set","<hour> <minutes>");

    // when help is requested, return immediate
    if(IsHelp(words[1]))
    {
        subCommand = words[2]; // Next word might be a sub Cmd.
        return;
    }

    size_t index = 1;

    // first word must be a sub command
    if(subCommandList.IsSubCommand(words[index]))
    {
        subCommand = words[index++];
        if(subCommand == "set")
        {
            if(!words.GetInt(index++,hour))
            {
                ParseError(me, "Hour not given or not an integer value.");
            }
            else if(hour < 0 || hour > 23)
            {
                ParseError(me, "Hour not between 0 and 23.");
            }

            if(!words.GetInt(index++,minute))
            {
                ParseError(me, "Minute not given or not an integer value.");
            }
            else if(minute < 0 || minute > 59)
            {
                ParseError(me, "Minutes not between 0 and 59.");
            }
        }
    }
    else
    {
        ParseError(me, "Missing subcommand");
    }
}

ADMINCMDFACTORY_IMPLEMENT_MSG_FACTORY_CREATE(AdminCmdDataTime)

csString AdminCmdDataTime::GetHelpMessage()
{
    if(!subCommand.IsEmpty())
    {
        return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage(subCommand) + "\"";
    }
    return "Syntax: \"" + command + " " + subCommandList.GetHelpMessage() + " [options]\"";
}

AdminCmdDataFactory::AdminCmdDataFactory()
{
    // register all AdminCmdData classes here
    RegisterMsgFactoryFunction(new AdminCmdDataTargetReason("/warn"));
    RegisterMsgFactoryFunction(new AdminCmdDataTargetReason("/kick"));
    RegisterMsgFactoryFunction(new AdminCmdDataDeleteChar());
    RegisterMsgFactoryFunction(new AdminCmdDataDeath());
    RegisterMsgFactoryFunction(new AdminCmdDataUpdateRespawn());
    RegisterMsgFactoryFunction(new AdminCmdDataBan());
    RegisterMsgFactoryFunction(new AdminCmdDataKillNPC());
    RegisterMsgFactoryFunction(new AdminCmdDataPercept());
    RegisterMsgFactoryFunction(new AdminCmdDataChangeNPCType());
    RegisterMsgFactoryFunction(new AdminCmdDataDebugNPC());
    RegisterMsgFactoryFunction(new AdminCmdDataDebugTribe());
    RegisterMsgFactoryFunction(new AdminCmdDataSetStackable());
    RegisterMsgFactoryFunction(new AdminCmdDataLoadQuest());
    RegisterMsgFactoryFunction(new AdminCmdDataInfo());
    RegisterMsgFactoryFunction(new AdminCmdDataItem());
    RegisterMsgFactoryFunction(new AdminCmdDataKey());

    RegisterMsgFactoryFunction(new AdminCmdDataRunScript());
    RegisterMsgFactoryFunction(new AdminCmdDataCrystal());
    RegisterMsgFactoryFunction(new AdminCmdDataTeleport());
    RegisterMsgFactoryFunction(new AdminCmdDataSlide());
    RegisterMsgFactoryFunction(new AdminCmdDataChangeName());
    RegisterMsgFactoryFunction(new AdminCmdDataChangeGuildName());
    RegisterMsgFactoryFunction(new AdminCmdDataChangeGuildLeader());
    RegisterMsgFactoryFunction(new AdminCmdDataPetition());
    RegisterMsgFactoryFunction(new AdminCmdDataImpersonate());
    RegisterMsgFactoryFunction(new AdminCmdDataDeputize());

    RegisterMsgFactoryFunction(new AdminCmdDataAward());
    RegisterMsgFactoryFunction(new AdminCmdDataItemTarget("/giveitem"));
    RegisterMsgFactoryFunction(new AdminCmdDataItemTarget("/takeitem"));
    RegisterMsgFactoryFunction(new AdminCmdDataCheckItem());
    RegisterMsgFactoryFunction(new AdminCmdDataSectorTarget("/thunder"));
    RegisterMsgFactoryFunction(new AdminCmdDataWeather());
    RegisterMsgFactoryFunction(new AdminCmdDataWeatherEffect("/snow"));
    RegisterMsgFactoryFunction(new AdminCmdDataWeatherEffect("/rain"));
    RegisterMsgFactoryFunction(new AdminCmdDataFog());
    RegisterMsgFactoryFunction(new AdminCmdDataModify());
    RegisterMsgFactoryFunction(new AdminCmdDataScale());

    RegisterMsgFactoryFunction(new AdminCmdDataMorph());
    RegisterMsgFactoryFunction(new AdminCmdDataSetSkill());
    RegisterMsgFactoryFunction(new AdminCmdDataSet());
    RegisterMsgFactoryFunction(new AdminCmdDataSetLabelColor());
    RegisterMsgFactoryFunction(new AdminCmdDataAction());
    RegisterMsgFactoryFunction(new AdminCmdDataPath());
    RegisterMsgFactoryFunction(new AdminCmdDataLocation());
    RegisterMsgFactoryFunction(new AdminCmdDataGameMasterEvent());
    RegisterMsgFactoryFunction(new AdminCmdDataHire());
    RegisterMsgFactoryFunction(new AdminCmdDataBadText());
    RegisterMsgFactoryFunction(new AdminCmdDataQuest());
    RegisterMsgFactoryFunction(new AdminCmdDataSetQuality());

    RegisterMsgFactoryFunction(new AdminCmdDataSetTrait());
    RegisterMsgFactoryFunction(new AdminCmdDataSetItem());
    RegisterMsgFactoryFunction(new AdminCmdDataReload());
    RegisterMsgFactoryFunction(new AdminCmdDataListWarnings());
    RegisterMsgFactoryFunction(new AdminCmdDataDisableQuest());
    RegisterMsgFactoryFunction(new AdminCmdDataSetKillExp());
    RegisterMsgFactoryFunction(new AdminCmdDataAssignFaction());
    RegisterMsgFactoryFunction(new AdminCmdDataServerQuit());
    RegisterMsgFactoryFunction(new AdminCmdDataNPCClientQuit());
    RegisterMsgFactoryFunction(new AdminCmdDataRndMsgTest());

    RegisterMsgFactoryFunction(new AdminCmdDataList());
    RegisterMsgFactoryFunction(new AdminCmdDataTime());
    RegisterMsgFactoryFunction(new AdminCmdDataSimple("/version"));

    // register commands that only have a target|player|... and no additional
    // options/data
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/banname", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_STRING | ADMINCMD_TARGET_CLIENTTARGET));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/unbanname", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_STRING));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/freeze", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/thaw", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/mute", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET));

    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/unmute", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/unban", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ACCOUNT | ADMINCMD_TARGET_DATABASE));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/banadvisor", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ACCOUNT | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/unbanadvisor", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ACCOUNT | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/charlist", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/inspect", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/npc", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_NPC));
    //has got its own class (but this is not really needed yet)
    // RegisterMsgFactoryFunction(new AdminCmdDataTarget("/setstackable", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)); //natoka: actually here is a need for ADMIN_TARGET_ITEM
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/divorce", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET));

    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/marriageinfo", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE));
    RegisterMsgFactoryFunction(new AdminCmdDataTarget("/targetname", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_ACCOUNT | ADMINCMD_TARGET_DATABASE | ADMINCMD_TARGET_STRING));

}

AdminCmdDataFactory::~AdminCmdDataFactory()
{
    adminCmdDatas.DeleteAll();
}

AdminCmdData* AdminCmdDataFactory::FindFactory(csString datatypename)
{
    for(size_t n = 0; n < adminCmdDatas.GetSize(); n++)
    {
        if(adminCmdDatas[n]->command == datatypename)
            return adminCmdDatas[n];
    }
    return NULL;
}

void AdminCmdDataFactory::RegisterMsgFactoryFunction(AdminCmdData* obj)
{
    AdminCmdData* factory = FindFactory(obj->command);
    if(factory)
    {
        Error2("Multiple factories for %s", obj->command.GetDataSafe());
        return;
    }

    adminCmdDatas.Push(obj);
}

AdminManager::AdminManager()
{
    clients = psserver->GetNetManager()->GetConnections();

    Subscribe(&AdminManager::HandleAdminCmdMessage, MSGTYPE_ADMINCMD, REQUIRE_READY_CLIENT);
    Subscribe(&AdminManager::HandlePetitionMessage, MSGTYPE_PETITION_REQUEST, REQUIRE_READY_CLIENT);
    Subscribe(&AdminManager::HandleGMGuiMessage, MSGTYPE_GMGUI, REQUIRE_READY_CLIENT);
    Subscribe(&AdminManager::SendSpawnItems, MSGTYPE_GMSPAWNITEMS, REQUIRE_READY_CLIENT);
    Subscribe(&AdminManager::SpawnItemInv, MSGTYPE_GMSPAWNITEM, REQUIRE_READY_CLIENT);
    Subscribe(&AdminManager::SendSpawnMods, MSGTYPE_GMSPAWNGETMODS, REQUIRE_READY_CLIENT);

    // this makes sure that the player dictionary exists on start up.
    npcdlg = new psNPCDialog(NULL);
    npcdlg->Initialize(db);

    locations = new LocationManager();
    locations->Load(EntityManager::GetSingleton().GetEngine(),db);

    pathNetwork = new psPathNetwork();
    pathNetwork->Load(EntityManager::GetSingleton().GetEngine(),db,
                      EntityManager::GetSingleton().GetWorld());

    dataFactory = new AdminCmdDataFactory();
}

AdminManager::~AdminManager()
{
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_ADMINCMD);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_PETITION_REQUEST);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GMGUI);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GMSPAWNITEMS);
    psserver->GetEventManager()->Unsubscribe(this,MSGTYPE_GMSPAWNITEM);

    delete dataFactory;
    delete npcdlg;
    delete pathNetwork;
    delete locations;
}


bool AdminManager::IsReseting(const csString &command)
{
    // Grab the first 8 characters after the command and see if we're resetting ourself
    // Everyone is allowed to reset themself via /deputize (should probably use this for other commands, too)
    return command.Slice(command.FindFirst(' ')+1,8) == "me reset";
}

//TODO: To be expanded to make the implementation better than how it is now
//      when an NPC issues an admin command
void AdminManager::HandleNpcCommand(MsgEntry* pMsg, Client* client)
{
    HandleAdminCmdMessage(pMsg, client);
}

void AdminManager::HandleAdminCmdMessage(MsgEntry* me, Client* client)
{
    AdminCmdData* data;
    psAdminCmdMessage msg(me);
    WordArray words(msg.cmd, false);

    // retrieve command data factory for this command
    data = dataFactory->FindFactory(words[0]);

    // unknown admin command encountered (should not happen)
    if(!data)
    {
        csString info("\"" + words[0] + "\" not registered admin command");
        psserver->SendSystemInfo(me->clientnum,info.GetDataSafe());
        return;
    }

    // Decode the string from the message into a struct with data elements
    data = data->CreateCmdData(this,me,msg,client,words);
    if(!data->valid)
    {
        // natoka: when meaningfull error messages are sent, this is obsolete
        psserver->SendSystemInfo(me->clientnum,data->GetHelpMessage());
        delete data;
        return;
    }

    // Security check
    if(me->clientnum != 0 && !IsReseting(msg.cmd) && !psserver->CheckAccess(client, data->command))
    {
        delete data;
        return;
    }

    if(data->help || !data->valid)
    {
        psserver->SendSystemInfo(me->clientnum, data->GetHelpMessage());
        delete data;
        return;
    }

    if(me->clientnum)
    {
        data->LogGMCommand(client, msg.cmd);
    }

    //This will not show anything on client side because for example in area commands
    //we want to parse the confirm based commands and not the original one
    //Still we log the command so we know how the successive commands where originated.
    //If this has to behave differently (for example area commands should be dispatched)
    //override the function.

    if(data->IsQuietInvalid())
    {
        delete data;
        return;
    }

    if(data->command == "/action")
    {
        HandleActionLocation(me, msg, data, client);
    }
    else if(data->command == "/assignfaction")
    {
        AssignFaction(me, msg, data, client);
    }
    else if(data->command == "/award")
    {
        Award(data, client);
    }
    else if(data->command == "/badtext")
    {
        HandleBadText(msg, data, client);
    }
    else if(data->command == "/ban")
    {
        BanClient(me, msg, data, client);
    }
    else if(data->command == "/banadvisor")
    {
        BanAdvisor(me, msg, data, client);
    }
    else if(data->command == "/banname")
    {
        BanName(me, msg, data, client);
    }
    else if(data->command == "/changeguildleader")
    {
        ChangeGuildLeader(me, msg, data, client);
    }
    else if(data->command == "/changeguildname")
    {
        RenameGuild(me, msg, data, client);
    }
    else if(data->command == "/changename")
    {
        ChangeName(me, msg, data, client);
    }
    else if(data->command == "/changenpctype")
    {
        ChangeNPCType(me, msg, data, client);
    }
    else if(data->command == "/charlist")
    {
        GetSiblingChars(me,msg,data, client);
    }
    else if(data->command == "/checkitem")
    {
        CheckItem(me, msg, data);
    }
    else if(data->command == "/death")
    {
        Death(me, msg, data, client);
    }
    else if(data->command == "/debugnpc")
    {
        DebugNPC(me, msg, data, client);
    }
    else if(data->command == "/debugtribe")
    {
        DebugTribe(me, msg, data, client);
    }
    else if(data->command == "/deletechar")
    {
        DeleteCharacter(me, msg, data, client);
    }
    else if(data->command == "/deputize")
    {
        TempSecurityLevel(me, msg, data, client);
    }
    else if(data->command == "/disablequest")
    {
        DisableQuest(me, msg, data, client);
    }
    else if(data->command == "/divorce")
    {
        Divorce(me, data);
    }
    else if(data->command == "/event")
    {
        HandleGMEvent(me, msg, data, client);
    }
    else if(data->command == "/fog")
    {
        Fog(me,msg,data,client);
    }
    else if(data->command == "/freeze")
    {
        FreezeClient(me, msg, data, client);
    }
    else if(data->command == "/giveitem")
    {
        TransferItem(me, msg, data, client);
    }
    else if(data->command == "/hire")
    {
        HandleHire(data, client);
    }
    else if(data->command == "/hunt_location")
    {
        CreateHuntLocation(me,msg,data,client);
    }
    else if(data->command == "/impersonate")
    {
        Impersonate(me, msg, data, client);
    }
    else if(data->command == "/info")
    {
        GetInfo(me,msg,data,client);
    }
    else if(data->command == "/inspect")
    {
        Inspect(me, msg, data, client);
    }
    else if(data->command == "/item")
    {
        CreateItem(me,msg,data,client);
    }
    else if(data->command == "/key")
    {
        ModifyKey(me,msg,data,client);
    }
    else if(data->command == "/kick")
    {
        KickPlayer(me, msg, data, client);
    }
    else if(data->command == "/killnpc")
    {
        KillNPC(me, msg, data, client);
    }
    else if(data->command == "/list")
    {
        HandleList(me, msg, data, client);
    }
    else if(data->command == "/listwarnings")
    {
        HandleListWarnings(msg, data, client);
    }
    else if(data->command == "/loadquest")
    {
        HandleLoadQuest(msg, data, client);
    }
    else if(data->command == "/location")
    {
        HandleLocation(me, msg, data, client);
    }
    else if(data->command == "/marriageinfo")
    {
        ViewMarriage(me, data);
    }
    else if(data->command == "/modify")
    {
        ModifyItem(me, msg, data, client);
    }
    else if(data->command == "/morph")
    {
        Morph(me, msg, data, client);
    }
    else if(data->command == "/mute")
    {
        MutePlayer(me,msg,data,client);
    }
    else if(data->command == "/npc")
    {
        CreateNPC(me,msg,data,client);
    }
    else if(data->command == "/npcclientquit")
    {
        HandleNPCClientQuit(me, msg, data, client);
    }
    else if(data->command == "/path")
    {
        HandlePath(me, msg, data, client);
    }
    else if(data->command == "/percept")
    {
        Percept(me, msg, data, client);
    }
    else if(data->command == "/petition")
    {
        HandleAddPetition(me, msg, data, client);
    }
    else if(data->command == "/quest")
    {
        HandleQuest(me, msg, data, client);
    }
    else if(data->command == "/rain")
    {
        Rain(me,msg,data,client);
    }
    else if(data->command == "/reload")
    {
        HandleReload(msg, data, client);
    }
    else if(data->command == "/rndmsgtest")
    {
        RandomMessageTest(data, client);
    }
    else if(data->command == "/runscript")
    {
        RunScript(me,msg,data,client);
    }
    else if(data->command == "/scale")
    {
        Scale(me, msg, data, client);
    }
    else if(data->command == "/serverquit")
    {
        HandleServerQuit(me, msg, data, client);
    }
    else if(data->command == "/set")
    {
        SetAttrib(me, msg, data, client);
    }
    else if(data->command == "/setkillexp")
    {
        SetKillExp(me, msg, data, client);
    }
    else if(data->command == "/setitemname")
    {
        HandleSetItemName(msg, data, client);
    }
    else if(data->command == "/setlabelcolor")
    {
        SetLabelColor(me, msg, data, client);
    }
    else if(data->command == "/setquality")
    {
        HandleSetQuality(msg, data, client);
    }
    else if(data->command == "/setskill")
    {
        SetSkill(me, msg, data, client);
    }
    else if(data->command == "/setstackable")
    {
        ItemStackable(me, data, client);
    }
    else if(data->command == "/settrait")
    {
        HandleSetTrait(msg, data, client);
    }
    else if(data->command == "/slide")
    {
        Slide(me,msg,data,client);
    }
    else if(data->command == "/snow")
    {
        Snow(me,msg,data,client);
    }
    else if(data->command == "/takeitem")
    {
        TransferItem(me, msg, data, client);
    }
    else if(data->command == "/targetname")
    {
        CheckTarget(msg, data, client);
    }
    else if(data->command == "/teleport")
    {
        Teleport(me,msg,data,client);
    }
    else if(data->command == "/thaw")
    {
        ThawClient(me, msg, data, client);
    }
    else if(data->command == "/thunder")
    {
        Thunder(me,msg,data,client);
    }
    else if(data->command == "/time")
    {
        HandleTime(me, msg, data, client);
    }
    else if(data->command == "/unban")
    {
        UnbanClient(me, msg, data, client);
    }
    else if(data->command == "/unbanadvisor")
    {
        UnbanAdvisor(me, msg, data, client);
    }
    else if(data->command == "/unbanname")
    {
        UnBanName(me, msg, data, client);
    }
    else if(data->command == "/unmute")
    {
        UnmutePlayer(me,msg,data,client);
    }
    else if(data->command == "/updaterespawn")
    {
        UpdateRespawn(data, client);
    }
    else if(data->command == "/version")
    {
        HandleVersion(me, msg, data, client);
    }
    else if(data->command == "/warn")
    {
        WarnMessage(me, msg, data, client);
    }
    else if(data->command == "/weather")
    {
        Weather(me,msg,data,client);
    }
    delete data;
}

void AdminManager::HandleList(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    csString map;
    size_t pos =0;
    size_t next;

    //AdminCmdDataList* data = dynamic_cast<AdminCmdDataList*>(cmddata);

    csString mapnames;
    psserver->entitymanager->GetWorld()->GetAllRegionNames(mapnames);

    psserver->SendSystemInfo(client->GetClientNum(), "Maps loaded by server:");
    next = mapnames.FindFirst('|',pos);
    //for (pos=0; next < mapnames.Length(); pos=next+1)
    do
    {
        // store next command
        map = mapnames.Slice(pos, next - pos);
        // output the map name to the client
        psserver->SendSystemInfo(client->GetClientNum(), "%s", map.GetData());
        // save the start of the next mapname
        pos = next +1;
        // find next delimiter (= end of map)
        next = mapnames.FindFirst('|',pos);
    }
    while(next < mapnames.Length());
}

void AdminManager::HandleTime(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataTime* data = dynamic_cast<AdminCmdDataTime*>(cmddata);

    if(data->subCommand == "show")
    {
        psserver->SendSystemInfo(client->GetClientNum(),"Game time is %d:%02d %d-%d-%d",
                                 psserver->GetWeatherManager()->GetGameTODHour(),
                                 psserver->GetWeatherManager()->GetGameTODMinute(),
                                 psserver->GetWeatherManager()->GetGameTODYear(),
                                 psserver->GetWeatherManager()->GetGameTODMonth(),
                                 psserver->GetWeatherManager()->GetGameTODDay());
        return;
    }
    else if(data->subCommand == "set")
    {
        psserver->GetWeatherManager()->SetGameTime(data->hour,data->minute);
        psserver->SendSystemInfo(client->GetClientNum(), "Current Game Hour set to: %d:%02d\n",
                                 data->hour, data->minute);
        return;
    }
}


void AdminManager::HandleLoadQuest(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataLoadQuest* data = dynamic_cast<AdminCmdDataLoadQuest*>(cmddata);
    uint32 questID = (uint32)-1;

    csString questName;
    db->Escape(questName, data->questName.GetData());
    Result result(db->Select("select * from quests where name='%s'", questName.GetData()));
    if(!result.IsValid() || result.Count() == 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "Quest <%s> not found", data->questName.GetData());
        return;
    }
    else
    {
        questID = result[0].GetInt("id");
    }

    if(!psserver->GetCacheManager()->UnloadQuest(questID))
        psserver->SendSystemError(client->GetClientNum(), "Quest <%s> Could not be unloaded", data->questName.GetData());
    else
        psserver->SendSystemError(client->GetClientNum(), "Quest <%s> unloaded", data->questName.GetData());

    if(!psserver->GetCacheManager()->LoadQuest(questID))
    {
        psserver->SendSystemError(client->GetClientNum(), "Quest <%s> Could not be loaded", data->questName.GetData());
        psserver->SendSystemError(client->GetClientNum(), psserver->questmanager->LastError());
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "Quest <%s> loaded", data->questName.GetData());
    }
}

void AdminManager::GetSiblingChars(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if((!data->target || !data->target.Length()) && !data->targetObject)
    {
        psserver->SendSystemError(me->clientnum, data->GetHelpMessage());
        return;
    }

    if(data->targetObject && !data->targetObject->GetCharacterData())  //no need to go on this isn't an npc or pc characther (most probably an item)
    {
        psserver->SendSystemError(me->clientnum,"Charlist can be used only on Player or NPC characters");
        return;
    }

    AccountID accountId = data->GetAccountID(me->clientnum);
    if(data->duplicateActor)  //we found more than one result so let's alert the user
    {
        psserver->SendSystemInfo(client->GetClientNum(), "Player name isn't unique. It's suggested to use pid.");
    }

    if(accountId.IsValid())
    {
        Result result2(db->Select("SELECT id, name, lastname, last_login FROM characters WHERE account_id = %u", accountId.Unbox()));

        if(result2.IsValid() && result2.Count())
        {

            psserver->SendSystemInfo(client->GetClientNum(), "Characters on this account:");
            for(int i = 0; i < (int)result2.Count(); i++)
            {
                iResultRow &row = result2[i];
                psserver->SendSystemInfo(client->GetClientNum(), "Player ID: %d, %s %s, last login: %s",
                                         row.GetUInt32("id"), row["name"], row["lastname"], row["last_login"]);
            }
        }
        else if(!result2.Count())
        {
            psserver->SendSystemInfo(me->clientnum, "There are no characters on this account.");
        }
        else
        {
            psserver->SendSystemError(me->clientnum, "Error executing SQL-statement to retrieve characters on the account of player %s.", data->target.GetData());
            return;
        }
    }
    else
    {
        psserver->SendSystemInfo(me->clientnum, "The Account ID [%d] of player %s is not valid.", ShowID(accountId), data->target.GetData());
    }
}

void AdminManager::GetInfo(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataInfo* data = static_cast<AdminCmdDataInfo*>(cmddata);

    EID entityId;
    csString sectorName, regionName;
    InstanceID instance = DEFAULT_INSTANCE;
    float loc_x = 0.0f, loc_y = 0.0f, loc_z = 0.0f, loc_yrot = 0.0f;
    int degrees = 0;

    if(data->targetObject)    //If the target is online or is an item or action location get some data about it like position and eid
    {
        entityId = data->targetObject->GetEID();
        csVector3 pos;
        iSector* sector = 0;

        data->targetObject->GetPosition(pos, loc_yrot, sector);
        loc_x = pos.x;
        loc_y = pos.y;
        loc_z = pos.z;
        degrees = (int)(loc_yrot * 180 / PI);

        instance = data->targetObject->GetInstance();

        sectorName = (sector) ? sector->QueryObject()->GetName() : "(null)";

        regionName = (sector) ? sector->QueryObject()->GetObjectParent()->GetName() : "(null)";
    }

    if(data->targetObject && data->targetObject->GetALPtr())  // Action location
    {
        gemActionLocation* item = dynamic_cast<gemActionLocation*>(data->targetObject);
        if(!item)
        {
            psserver->SendSystemError(client->GetClientNum(), "Error! Target is not a valid gemActionLocation object.");
            return;
        }
        psActionLocation* action = item->GetAction();

        csString info;
        info.Format("ActionLocation: %s is at region %s, position (%1.2f, %1.2f, %1.2f) "
                    "angle: %d in sector: %s, instance: %d with ",
                    item->GetName(),
                    regionName.GetData(),
                    loc_x, loc_y, loc_z, degrees,
                    sectorName.GetData(),
                    instance);

        if(action)
            info.AppendFmt("ID %u, and instance ID of the container %u.", action->id, action->GetInstanceID());
        else
            info.Append("no action location information.");

        psserver->SendSystemInfo(client->GetClientNum(), info);

        return;
    }

    if(data->targetObject && data->targetObject->GetItem() && data->targetObject->GetItem()->GetBaseStats())    // Item
    {
        psItem* item = data->targetObject->GetItem();

        csString info;
        info.Format("Item: %s ", item->GetName());

        if(item->GetStackCount() > 1)
            info.AppendFmt("(x%d) ", item->GetStackCount());

        info.AppendFmt("with item stats ID %u, item ID %u, and %s, is at region %s, position (%1.2f, %1.2f, %1.2f) "
                       "angle: %d in sector: %s, instance: %d",
                       item->GetBaseStats()->GetUID(),
                       item->GetUID(),
                       ShowID(entityId),
                       regionName.GetData(),
                       loc_x, loc_y, loc_z, degrees,
                       sectorName.GetData(),
                       instance);

        if(item->GetScheduledItem())
            info.AppendFmt(", spawns with interval %d + %d max modifier",
                           item->GetScheduledItem()->GetInterval(),
                           item->GetScheduledItem()->GetMaxModifier());


        // Get all flags on this item
        int flags = item->GetFlags();
        if(flags)
        {
            info += ", has flags:";

            if(flags & PSITEM_FLAG_CRAFTER_ID_IS_VALID)
                info += " 'valid crafter id'";
            if(flags & PSITEM_FLAG_GUILD_ID_IS_VALID)
                info += " 'valid guild id'";
            if(flags & PSITEM_FLAG_UNIQUE_ITEM)
                info += " 'unique'";
            if(flags & PSITEM_FLAG_USES_BASIC_ITEM)
                info += " 'uses basic item'";
            if(flags & PSITEM_FLAG_PURIFIED)
                info += " 'purified'";
            if(flags & PSITEM_FLAG_PURIFYING)
                info += " 'purifying'";
            if(flags & PSITEM_FLAG_LOCKED)
                info += " 'locked'";
            if(flags & PSITEM_FLAG_LOCKABLE)
                info += " 'lockable'";
            if(flags & PSITEM_FLAG_SECURITYLOCK)
                info += " 'lockable'";
            if(flags & PSITEM_FLAG_UNPICKABLE)
                info += " 'unpickable'";
            if(flags & PSITEM_FLAG_NOPICKUP)
                info += " 'no pickup'";
            if(flags & PSITEM_FLAG_NOPICKUPWEAK)
                info += " 'no pickup weak'";
            if(flags & PSITEM_FLAG_KEY)
                info += " 'key'";
            if(flags & PSITEM_FLAG_MASTERKEY)
                info += " 'masterkey'";
            if(flags & PSITEM_FLAG_TRANSIENT)
                info += " 'transient'";
            if(flags & PSITEM_FLAG_USE_CD)
                info += " 'collide'";
            if(flags & PSITEM_FLAG_SETTINGITEM)
                info += " 'settingitem'";
            if(flags & PSITEM_FLAG_NPCOWNED)
                info += " 'npcowned'";
        }

        psserver->SendSystemInfo(client->GetClientNum(),info);
        return; // Done
    }

    csString name, ipAddress, securityLevel;
    PID playerId;
    AccountID accountId;
    float timeConnected = 0.0f;

    bool banned = false;
    time_t banTimeLeft;
    int daysLeft = 0, hoursLeft = 0, minsLeft = 0;
    csString BanReason;
    bool advisorBanned = false;
    bool ipBanned = false;
    int cheatCount = 0;

    if(data->IsOnline())  // when target is online
    {
        Client* targetclient = data->targetClient;

        playerId = data->targetID;
        if(data->targetObject->GetCharacterData())
            timeConnected = data->targetObject->GetCharacterData()->GetTimeConnected() / 3600;

        if(targetclient)  // Player
        {
            name = targetclient->GetName();
            ipAddress = targetclient->GetIPAddress();
            accountId = data->GetAccountID(me->clientnum);

            // Because of /deputize we'll need to get the real SL from DB
            int currSL = targetclient->GetSecurityLevel();
            int trueSL = GetTrueSecurityLevel(accountId);

            if(currSL != trueSL)
                securityLevel.Format("%d(%d)",currSL,trueSL);
            else
                securityLevel.Format("%d",currSL);

            advisorBanned = targetclient->IsAdvisorBanned();
            cheatCount = targetclient->GetDetectedCheatCount();
        }
        else // NPC
        {
            gemNPC* npc = data->targetObject->GetNPCPtr();
            if(!npc)
            {
                psserver->SendSystemError(client->GetClientNum(), "Error! Target is not a valid gemNPC.");
                return;
            }

            psCharacter* npcChar = npc->GetCharacterData();

            name = npc->GetName();

            float dist = 0.0;
            {
                csVector3 pos,myPos;
                iSector* sector = 0;
                iSector* mySector = 0;
                float yRot, myYRot;

                data->targetObject->GetPosition(pos, yRot, sector);
                client->GetActor()->GetPosition(myPos, myYRot, mySector);


                dist = EntityManager::GetSingleton().GetWorld()->Distance(pos, sector, myPos, mySector);
            }


            psserver->SendSystemInfo(client->GetClientNum(),
                                     "NPC: <%s, %s, %s> is at region %s, position (%1.2f, %1.2f, %1.2f) at range %.2f "
                                     "angle: %d in sector: %s, instance: %d%s%s.",
                                     name.GetData(),
                                     ShowID(playerId),
                                     ShowID(entityId),
                                     regionName.GetData(),
                                     loc_x,
                                     loc_y,
                                     loc_z,
                                     dist,
                                     degrees,
                                     sectorName.GetData(),
                                     instance,
                                     npcChar->GetImperviousToAttack()&ALWAYS_IMPERVIOUS?", is always impervious":"",
                                     npcChar->GetImperviousToAttack()&TEMPORARILY_IMPERVIOUS?", is temp impervious":""
                                    );

            if(client->GetSecurityLevel() >= GM_LEVEL_0)
            {
                if(npcChar->IsPet() && (data->subCmd == "all" || data->subCmd == "pet"))
                {
                    psserver->GetNPCManager()->PetInfo(client, npcChar);
                }

                // Queue info request perception (Perception as command to superclient)
                psserver->GetNPCManager()->QueueInfoRequestPerception(npc, client, data->subCmd);
            }
            return; // Done
        }
    }
    else // Offline
    {
        PID pid;
        Result result;
        if(data->IsTargetType(ADMINCMD_TARGET_PID))
        {
            pid = data->targetID;

            if(!pid.IsValid())
            {
                psserver->SendSystemError(client->GetClientNum(), "%s is invalid!",data->target.GetData());
                return;
            }

            result = db->Select("SELECT c.id as 'id', c.name as 'name', lastname, account_id, time_connected_sec, loc_instance, "
                                "s.name as 'sector', loc_x, loc_y, loc_z, loc_yrot, advisor_ban from characters c join sectors s on s.id = loc_sector_id "
                                "join accounts a on a.id = account_id where c.id=%u", pid.Unbox());
        }
        else
        {
            result = db->Select("SELECT c.id as 'id', c.name as 'name', lastname, account_id, time_connected_sec, loc_instance, "
                                "s.name as 'sector', loc_x, loc_y, loc_z, loc_yrot, advisor_ban from characters c join sectors s on s.id = loc_sector_id "
                                "join accounts a on a.id = account_id where c.name='%s'", data->target.GetData());
        }

        if(!result.IsValid() || result.Count() == 0)
        {
            psserver->SendSystemError(client->GetClientNum(), "Cannot find player %s",data->target.GetData());
            return;
        }
        else
        {
            iResultRow &row = result[0];
            name = row["name"];
            if(row["lastname"] && strcmp(row["lastname"],""))
            {
                name.Append(" ");
                name.Append(row["lastname"]);
            }
            playerId = PID(row.GetUInt32("id"));
            accountId = AccountID(row.GetUInt32("account_id"));
            ipAddress = "(offline)";
            timeConnected = row.GetFloat("time_connected_sec") / 3600;
            securityLevel.Format("%d",GetTrueSecurityLevel(accountId));
            sectorName = row["sector"];
            instance = row.GetUInt32("loc_instance");
            loc_x = row.GetFloat("loc_x");
            loc_y = row.GetFloat("loc_y");
            loc_z = row.GetFloat("loc_z");
            loc_yrot = row.GetFloat("loc_yrot");
            advisorBanned = row.GetUInt32("advisor_ban") != 0;
        }
    }
    BanEntry* ban = psserver->GetAuthServer()->GetBanManager()->GetBanByAccount(accountId);
    if(ban)
    {
        time_t now = time(0);
        if(ban->end > now)
        {
            BanReason = ban->reason;
            banTimeLeft = ban->end - now;
            banned = true;
            ipBanned = ban->banIP;

            banTimeLeft = banTimeLeft / 60; // don't care about seconds
            minsLeft = banTimeLeft % 60;
            banTimeLeft = banTimeLeft / 60;
            hoursLeft = banTimeLeft % 24;
            banTimeLeft = banTimeLeft / 24;
            daysLeft = banTimeLeft;
        }
    }

    if(playerId.IsValid())
    {
        csString info;
        info.Format("Player: %s has ", name.GetData());

        if(securityLevel != "0")
            info.AppendFmt("security level %s, ", securityLevel.GetData());

        info.AppendFmt("%s, %s, ", ShowID(accountId), ShowID(playerId));

        if(ipAddress != "(offline)")
            info.AppendFmt("%s, IP is %s, ", ShowID(entityId), ipAddress.GetData());
        else
            info.Append("is offline, ");

        info.AppendFmt("at region %s, position (%1.2f, %1.2f, %1.2f) angle: %d in sector: %s, instance: %d, ",
                       regionName.GetData(), loc_x, loc_y, loc_z, degrees, sectorName.GetData(), instance);

        info.AppendFmt("total time connected is %1.1f hours", timeConnected);

        info.AppendFmt(" has had %d cheats flagged.", cheatCount);

        if(banned)
        {
            info.AppendFmt(" The player's account is banned for %s! Time left: %d days, %d hours, %d minutes.", BanReason.GetDataSafe(), daysLeft, hoursLeft, minsLeft);

            if(ipBanned)
                info.Append(" The player's ip range is banned as well.");
        }

        if(advisorBanned)
            info.Append(" The player's account is banned from advising.");

        psserver->SendSystemInfo(client->GetClientNum(),info);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "Error!  Object is not an item, player, or NPC.");
    }
}

void AdminManager::HandlePetitionMessage(MsgEntry* me, Client* client)
{
    psPetitionRequestMessage msg(me);

    // Check which message and if this is a GM message or user message
    if(msg.request == "query")
    {
        if(msg.isGM)
            GMListPetitions(me, msg, client);
        else
            ListPetitions(me, msg, client);
    }
    else if(msg.request == "cancel" && !msg.isGM)
    {
        CancelPetition(me, msg, client);
    }
    else if(msg.request == "change" && !msg.isGM)
    {
        ChangePetition(me, msg, client);
    }
    else if(msg.isGM)
    {
        GMHandlePetition(me, msg, client);
    }
}

void AdminManager::HandleGMGuiMessage(MsgEntry* me, Client* client)
{
    psGMGuiMessage msg(me);
    if(msg.type == psGMGuiMessage::TYPE_QUERYPLAYERLIST)
    {
        if(client->GetSecurityLevel() >= GM_LEVEL_0)
            SendGMPlayerList(me, msg,client);
        else
            psserver->SendSystemError(me->clientnum, "You are not a GM.");
    }
    else if(msg.type == psGMGuiMessage::TYPE_GETGMSETTINGS)
    {
        SendGMAttribs(client);
    }
}

void AdminManager::SendGMAttribs(Client* client)
{
    int gmSettings = 0;
    if(client->GetActor()->GetVisibility())
        gmSettings |= 1;
    if(client->GetActor()->GetInvincibility())
        gmSettings |= (1 << 1);
    if(client->GetActor()->GetViewAllObjects())
        gmSettings |= (1 << 2);
    if(client->GetActor()->nevertired)
        gmSettings |= (1 << 3);
    if(client->GetActor()->questtester)
        gmSettings |= (1 << 4);
    if(client->GetActor()->infinitemana)
        gmSettings |= (1 << 5);
    if(client->GetActor()->GetFiniteInventory())
        gmSettings |= (1 << 6);
    if(client->GetActor()->safefall)
        gmSettings |= (1 << 7);
    if(client->GetActor()->instantcast)
        gmSettings |= (1 << 8);
    if(client->GetActor()->givekillexp)
        gmSettings |= (1 << 9);
    if(client->GetActor()->attackable)
        gmSettings |= (1 << 10);
    if(client->GetBuddyListHide())
        gmSettings |= (1 << 11);

    psGMGuiMessage gmMsg(client->GetClientNum(), gmSettings);
    gmMsg.SendMessage();
}

void AdminManager::CreateHuntLocation(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataCrystal* data = static_cast<AdminCmdDataCrystal*>(cmddata);

    if(data->interval < 1 || data->random < 1)
    {
        psserver->SendSystemError(me->clientnum, "Intervals need to be greater than 0");
        return;
    }
    if(data->amount < 1)
    {
        psserver->SendSystemError(me->clientnum, "Amount must be greater than 0");
        return;
    }
    if(data->range < 0)
    {
        psserver->SendSystemError(me->clientnum, "Range must be equal to or greater than 0");
        return;
    }

    // In seconds
    int interval = 1000*data->interval;
    int random   = 1000*data->random;

    psItemStats* rawitem = psserver->GetCacheManager()->GetBasicItemStatsByName(data->itemName);
    if(!rawitem)
    {
        psserver->SendSystemError(me->clientnum, "Invalid item to spawn");
        return;
    }

    // cant create personalised or unique items
    if(rawitem->GetBuyPersonalise() || rawitem->GetUnique())
    {
        psserver->SendSystemError(me->clientnum, "Item is personalised or unique");
        return;
    }

    // Find the location
    csVector3 pos;
    float angle;
    iSector* sector = 0;
    InstanceID instance;

    client->GetActor()->GetPosition(pos, angle, sector);
    instance = client->GetActor()->GetInstance();

    if(!sector)
    {
        psserver->SendSystemError(me->clientnum, "Invalid sector");
        return;
    }

    psSectorInfo* spawnsector = psserver->GetCacheManager()->GetSectorInfoByName(sector->QueryObject()->GetName());
    if(!spawnsector)
    {
        CPrintf(CON_ERROR,"Player is in invalid sector %s!",sector->QueryObject()->GetName());
        return;
    }

    // to db
    db->Command(
        "INSERT INTO hunt_locations"
        "(`x`,`y`,`z`,`itemid`,`sector`,`interval`,`max_random`,`amount`,`range`)"
        "VALUES ('%f','%f','%f','%u','%u','%d','%d','%d','%f')",
        pos.x,pos.y,pos.z, rawitem->GetUID(),spawnsector->uid,interval,random,data->amount,data->range);

    for(int i = 0; i < data->amount; ++i)  //Make desired amount of items
    {
        psScheduledItem* schedule = new psScheduledItem(db->GetLastInsertID(),rawitem->GetUID(),pos,spawnsector,instance,
                interval,random,data->range);
        psItemSpawnEvent* event = new psItemSpawnEvent(schedule);
        psserver->GetEventManager()->Push(event);
    }

    // Done!
    psserver->SendSystemInfo(me->clientnum,"New hunt location created!");
}

void AdminManager::SetAttrib(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSet* data = static_cast<AdminCmdDataSet*>(cmddata);
    gemActor* actor;

    if(data->targetActor)
        actor = data->targetActor;
    else
        actor = client->GetActor();

    if(actor != client->GetActor() && !psserver->CheckAccess(client, "setattrib others"))
    {
        psserver->SendSystemInfo(me->clientnum, "You are not allowed to use this command on others");
        return;
    }

    bool onoff = false;
    bool toggle = false;
    bool already = false;

    onoff = data->setting.IsOn();
    toggle = data->setting.IsToggle();

    if(data->subCommand == "list")
    {
        psserver->SendSystemInfo(me->clientnum, "Current settings for %s is:\n"
                                 "invincible = %s\n"
                                 "invisible = %s\n"
                                 "viewall = %s\n"
                                 "nevertired = %s\n"
                                 "nofalldamage = %s\n"
                                 "infiniteinventory = %s\n"
                                 "questtester = %s\n"
                                 "infinitemana = %s\n"
                                 "instantcast = %s\n"
                                 "givekillexp = %s\n"
                                 "attackable = %s\n"
                                 "buddyhide = %s",
                                 actor->GetName(),
                                 (actor->GetInvincibility())?"on":"off",
                                 (!actor->GetVisibility())?"on":"off",
                                 (actor->GetViewAllObjects())?"on":"off",
                                 (actor->nevertired)?"on":"off",
                                 (actor->safefall)?"on":"off",
                                 (!actor->GetFiniteInventory())?"on":"off",
                                 (actor->questtester)?"on":"off",
                                 (actor->infinitemana)?"on":"off",
                                 (actor->instantcast)?"on":"off",
                                 (actor->givekillexp)?"on":"off",
                                 (actor->attackable)?"on":"off",
                                 (actor->GetClient() &&
                                  actor->GetClient()->GetBuddyListHide())?"on":"off");
        return;
    }
    else if(data->subCommand == "gm")
    {
        actor->SetDefaults(false);
        psserver->SendSystemInfo(me->clientnum, "Set all flags to default for GM.");
        SendGMAttribs(client);
        return;
    }
    else if(data->subCommand == "player")
    {
        actor->SetDefaults(true);
        psserver->SendSystemInfo(me->clientnum, "Set all flags to default for Player.");
        SendGMAttribs(client);
        return;
    }
    else if(data->attribute == "invincible" || data->attribute == "invincibility")
    {
        if(toggle)
        {
            actor->SetInvincibility(!actor->GetInvincibility());
            onoff = actor->GetInvincibility();
        }
        else if(actor->GetInvincibility() == onoff)
            already = true;
        else
            actor->SetInvincibility(onoff);
    }
    else if(data->attribute == "invisible" || data->attribute == "invisibility")
    {
        if(toggle)
        {
            actor->SetVisibility(!actor->GetVisibility());
            onoff = !actor->GetVisibility();
        }
        else if(actor->GetVisibility() == !onoff)
            already = true;
        else
            actor->SetVisibility(!onoff);
    }
    else if(data->attribute == "viewall")
    {
        if(toggle)
        {
            actor->SetViewAllObjects(!actor->GetViewAllObjects());
            onoff = actor->GetViewAllObjects();
        }
        else if(actor->GetViewAllObjects() == onoff)
            already = true;
        else
            actor->SetViewAllObjects(onoff);
    }
    else if(data->attribute == "nevertired")
    {
        if(toggle)
        {
            actor->nevertired = !actor->nevertired;
            onoff = actor->nevertired;
        }
        else if(actor->nevertired == onoff)
            already = true;
        else
            actor->nevertired = onoff;
    }
    else if(data->attribute == "infinitemana")
    {
        if(toggle)
        {
            actor->infinitemana = !actor->infinitemana;
            onoff = actor->infinitemana;
        }
        else if(actor->infinitemana == onoff)
            already = true;
        else
            actor->infinitemana = onoff;
    }
    else if(data->attribute == "instantcast")
    {
        if(toggle)
        {
            actor->instantcast = !actor->instantcast;
            onoff = actor->instantcast;
        }
        else if(actor->instantcast == onoff)
            already = true;
        else
            actor->instantcast = onoff;
    }
    else if(data->attribute == "nofalldamage")
    {
        if(toggle)
        {
            actor->safefall = !actor->safefall;
            onoff = actor->safefall;
        }
        else if(actor->safefall == onoff)
            already = true;
        else
            actor->safefall = onoff;
    }
    else if(data->attribute == "infiniteinventory")
    {
        if(toggle)
        {
            actor->SetFiniteInventory(!actor->GetFiniteInventory());
            onoff = !actor->GetFiniteInventory();
        }
        else if(actor->GetFiniteInventory() == !onoff)
            already = true;
        else
            actor->SetFiniteInventory(!onoff);
    }
    else if(data->attribute == "questtester")
    {
        if(toggle)
        {
            actor->questtester = !actor->questtester;
            onoff = actor->questtester;
        }
        else if(actor->questtester == onoff)
            already = true;
        else
            actor->questtester = onoff;
    }
    else if(data->attribute == "givekillexp")
    {
        if(toggle)
        {
            actor->givekillexp = !actor->givekillexp;
            onoff = actor->givekillexp;
        }
        else if(actor->givekillexp == onoff)
            already = true;
        else
            actor->givekillexp = onoff;
    }
    else if(data->attribute == "attackable")
    {
        if(toggle)
        {
            actor->attackable = !actor->attackable;
            onoff = actor->attackable;
        }
        else if(actor->attackable == onoff)
            already = true;
        else
            actor->attackable = onoff;
    }
    else if(data->attribute == "buddyhide" && actor->GetClient())
    {
        if(toggle)
        {
            actor->GetClient()->SetBuddyListHide(!actor->GetClient()->GetBuddyListHide());
            onoff = actor->GetClient()->GetBuddyListHide();
        }
        else if(actor->GetClient()->GetBuddyListHide() == onoff)
            already = true;
        else
            actor->GetClient()->SetBuddyListHide(onoff);

        if(!already)
            psserver->usermanager->NotifyPlayerBuddies(actor->GetClient(), !onoff);
    }
    else if(!data->attribute.IsEmpty())
    {
        psserver->SendSystemInfo(me->clientnum, "%s is not a supported attribute", data->attribute.GetData());
        return;
    }
    else
    {
        psserver->SendSystemInfo(me->clientnum, "Correct syntax is: \"/set [target] [list|gm|player|attribute] [on|off]\"");
        return;
    }

    psserver->SendSystemInfo(me->clientnum, "%s %s %s",
                             data->attribute.GetData(),
                             (already)?"is already":"has been",
                             (onoff)?"enabled":"disabled");

    SendGMAttribs(client);
}

void AdminManager::SetLabelColor(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSetLabelColor* data = static_cast<AdminCmdDataSetLabelColor*>(cmddata);
    int mask = 0;

    if(!data->targetActor)
    {
        psserver->SendSystemInfo(me->clientnum, "The target was not found online");
        return;
    }

    if(data->labelType == "normal" || data->labelType == "alive")
    {
        mask = data->targetActor->GetSecurityLevel();
    }
    else if(data->labelType == "dead")
    {
        mask = -3;
    }
    else if(data->labelType == "npc")
    {
        mask = -1;
    }
    else if(data->labelType == "player")
    {
        mask = 0;
    }
    else if(data->labelType == "tester")
    {
        mask = GM_TESTER;
    }
    else if(data->labelType == "gm1")
    {
        mask = GM_LEVEL_1;
    }
    else if(data->labelType == "gm")
    {
        mask = GM_LEVEL_2;
    }
    else
    {
        psserver->SendSystemInfo(me->clientnum, data->GetHelpMessage());
        return;
    }
    data->targetActor->SetMasqueradeLevel(mask);
    psserver->SendSystemInfo(me->clientnum, "Label color of %s set to %s",
                             data->target.GetData(), data->labelType.GetData());
}

void AdminManager::Divorce(MsgEntry* me, AdminCmdData* cmddata)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    Client* divorcer = clients->Find(data->target);

    // If the player that wishes to divorce is not online, we can't proceed.
    if(!divorcer)
    {
        psserver->SendSystemInfo(me->clientnum, "The player that wishes to divorce must be online.");
        return;
    }

    psCharacter* divorcerChar = divorcer->GetCharacterData();

    // If the player is not married, we can't divorce.
    if(!divorcerChar->GetIsMarried())
    {
        psserver->SendSystemInfo(me->clientnum, "You can't divorce people who aren't married!");
        return;
    }

    // Divorce the players.
    psMarriageManager* marriageMgr = psserver->GetMarriageManager();
    if(!marriageMgr)
    {
        psserver->SendSystemError(me->clientnum, "Can't load MarriageManager.");
        Error1("MarriageManager failed to load.");
        return;
    }

    // Delete entries of character's from DB
    csString spouseFullName = divorcerChar->GetSpouseName();
    csString spouseName = spouseFullName.Slice(0, spouseFullName.FindFirst(' '));
    marriageMgr->DeleteMarriageInfo(divorcerChar);
    psserver->SendSystemInfo(me->clientnum, "You have divorced %s from %s.", data->target.GetData(), spouseName.GetData());
    Debug3(LOG_RELATIONSHIPS, me->clientnum, "%s divorced from %s.", data->target.GetData(), spouseName.GetData());
}

void AdminManager::ViewMarriage(MsgEntry* me, AdminCmdData* cmddata)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    bool married;
    csString spouse;
    csString playerStr = data->target;

    if(data->targetClient)
    {
        // player is online
        psCharacter* playerData = data->targetClient->GetCharacterData();
        married = playerData->GetIsMarried();

        if(married)
        {
            csString spouseFullName = playerData->GetSpouseName();
            spouse = spouseFullName.Slice(0, spouseFullName.FindFirst(' '));
        }
    }
    else
    {
        // player is offline - hit the db
        Result result;

        result = db->Select(
                     "SELECT name FROM characters WHERE id = "
                     "("
                     "   SELECT related_id FROM character_relationships WHERE character_id = %u AND relationship_type='spouse'"
                     ")", data->targetID.Unbox());

        married = (result.IsValid() && result.Count() != 0);
        if(married)
        {
            iResultRow &row = result[0];
            spouse = row["name"];
        }

        if(result.Count() > 1)  //check for duplicate
            data->duplicateActor = true;
    }

    if(data->duplicateActor)  //report that we found more than one result and data might be wrong
        psserver->SendSystemInfo(me->clientnum, "Player name isn't unique. It's suggested to use pid.");

    if(married)
    {
        // character is married
        if(psserver->GetCharManager()->HasConnected(spouse))
        {
            psserver->SendSystemInfo(me->clientnum, "%s is married to %s, who was last online less than two months ago.", playerStr.GetData(), spouse.GetData());
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "%s is married to %s, who was last online more than two months ago.", playerStr.GetData(), spouse.GetData());
        }
    }
    else
    {
        // character isn't married
        psserver->SendSystemInfo(me->clientnum, "%s is not married.", playerStr.GetData());
    }
}

void AdminManager::TeleportOfflineCharacter(Client* client, AdminCmdDataTeleport* data)
{
    psString sql;
    iSector* gmSector;  // sector of gamemaster issuing the cmd
    csVector3 gmPoint;  // position of the gm
    psSectorInfo* gmSectorInfo = NULL;
    float yRot = 0.0;

    client->GetActor()->GetPosition(gmPoint, yRot, gmSector);

    if(gmSector != NULL)
    {
        gmSectorInfo = psserver->GetCacheManager()->GetSectorInfoByName(gmSector->QueryObject()->GetName());
    }
    if(gmSectorInfo == NULL)
    {
        psserver->SendSystemError(client->GetClientNum(), "Sector not found!");
        return;
    }

    //escape the player name so it's not possible to do nasty things
    csString escapedName;
    db->Escape(escapedName, data->target.GetDataSafe());

    sql.AppendFmt("update characters set loc_x=%10.2f, loc_y=%10.2f, loc_z=%10.2f, loc_yrot=%10.2f, loc_sector_id=%u, loc_instance=%u where name=\"%s\"",
                  gmPoint.x, gmPoint.y, gmPoint.z, yRot, gmSectorInfo->uid, client->GetActor()->GetInstance(), escapedName.GetDataSafe());

    if(db->CommandPump(sql) != 1)
    {
        Error3("Couldn't save character's position to database.\nCommand was "
               "<%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());

        psserver->SendSystemError(client->GetClientNum(), "Offline character %s could not be moved!", data->target.GetData());
    }
    else
    {
        psserver->SendSystemResult(client->GetClientNum(), "%s will next log in at your current location", data->target.GetData());
    }
}

void AdminManager::Teleport(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client) //, gemObject* subject)
{
    AdminCmdDataTeleport* data = static_cast<AdminCmdDataTeleport*>(cmddata);

    if(data->target.IsEmpty())
    {
        psserver->SendSystemInfo(client->GetClientNum(), "Use: /teleport <subject> <destination>\n"
                                 "Subject    : me/target/<object name>/<NPC name>/<player name>/eid:<EID>/pid:<PID>\n"
                                 "Destination: me [<instance>]/target/<object name>/<NPC name>/<player name>/eid:<EID>/pid:<PID>/\n"
                                 "             here [<instance>]/last/spawn/restore/map <map name>|here|list [<x> <y> <z> [<instance>]]\n"
                                 "             there <instance>");
        return;
    }

    // If player is offline and the special argument is called
    if(!data->IsOnline() && data->dest == "restore")
    {
        TeleportOfflineCharacter(client, data);
        return;
    }
    else if(data->targetObject == NULL)
    {
        psserver->SendSystemError(client->GetClientNum(), "Cannot teleport target");
        return;
    }

    if(data->dest == "map" && (data->destSector == "list" || data->destMap == "list"))
    {
        // Build list of sectors
        csString sectorList;
        csHash<psSectorInfo*>::GlobalIterator iter(psserver->GetCacheManager()->GetSectorIterator());
        while(iter.HasNext())
        {
            psSectorInfo* info = iter.Next();
            sectorList += "\n";
            sectorList += info->name;
        }

        psserver->SendSystemInfo(client->GetClientNum(),"List of sectors:%s",
                                 sectorList.GetDataSafe());
        return;
    }

    csVector3 targetPoint;
    float yRot = 0.0;
    iSector* targetSector;
    InstanceID targetInstance;

    if(!GetTargetOfTeleport(client, msg, data, targetSector, targetPoint, yRot, data->targetObject, targetInstance))
    {
        psserver->SendSystemError(client->GetClientNum(), "Cannot teleport %s to %s",
                                  data->target.GetData(), data->destObj.target.GetData());
        return;
    }

    //Error6("tele %s to %s %f %f %f",subject->GetName(), targetSector->QueryObject()->GetName(), targetPoint.x, targetPoint.y, targetPoint.z);

    csVector3 oldpos;
    float oldyrot;
    iSector* oldsector;
    InstanceID oldInstance = data->targetObject->GetInstance();
    data->targetObject->GetPosition(oldpos,oldyrot,oldsector);

    if(oldsector == targetSector && oldpos == targetPoint && oldInstance == targetInstance)
    {
        psserver->SendSystemError(client->GetClientNum(), "destination is identical with current position");
        return;
    }

    Client* superclient = clients->FindAccount(data->targetObject->GetSuperclientID());
    if(superclient && data->targetObject->GetSuperclientID()!=0)
    {
        gemNPC* npc = dynamic_cast<gemNPC*>(data->targetObject);
        if(npc)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "%s is controlled by superclient %s and might not be teleported.",
                                     data->targetObject->GetName(), ShowID(data->targetObject->GetSuperclientID()));
            psserver->GetNPCManager()->QueueTeleportPerception(npc,targetPoint,yRot,targetSector,targetInstance);
        }
    }

    if(!MoveObject(client,data->targetObject,targetPoint,yRot,targetSector,targetInstance))
        return;

    csString destName;
    if(data->destMap.Length())
    {
        destName.Format("map %s", data->destMap.GetData());
    }
    else
    {
        destName.Format("sector %s", targetSector->QueryObject()->GetName());
    }

    if(oldsector != targetSector)
    {
        psserver->SendSystemOK(data->targetObject->GetClientID(), "Welcome to " + destName);
    }

    if(dynamic_cast<gemActor*>(data->targetObject))    // Record old location of actor, for undo
        ((gemActor*)data->targetObject)->SetPrevTeleportLocation(oldpos, oldyrot, oldsector, oldInstance);

    // Send explanations
    bool instance_changed = oldInstance != targetInstance;
    csString instance_str;
    instance_str.Format(" in instance %u",targetInstance);
    csString dest = data->destObj.target;

    if(data->destObj.IsTargetTypeUnknown())
        dest = data->dest;

    psserver->SendSystemResult(client->GetClientNum(), "Teleported %s to %s%s", data->targetObject->GetName(),
                               dest.GetData(),
                               instance_changed?instance_str.GetData():"");

    if(data->targetObject->GetClientID() != client->GetClientNum())
        psserver->SendSystemResult(data->targetObject->GetClientID(), "You were moved by a GM");

    if(data->target == "me"  &&  data->dest != "map"  &&  data->dest != "here")
    {
        psGUITargetUpdateMessage updateMessage(client->GetClientNum(), data->targetObject->GetEID());
        updateMessage.SendMessage();
    }
}

void AdminManager::HandleActionLocation(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataAction* data = static_cast<AdminCmdDataAction*>(cmddata);

    if(data->subCommand == "create_entrance")
    {
        // Create sign
        csString doorLock = "Signpost01";
        psItemStats* itemstats=psserver->GetCacheManager()->GetBasicItemStatsByName(doorLock.GetData());
        if(!itemstats)
        {
            // Try some SVN art
            doorLock = "Claymore";
            itemstats=psserver->GetCacheManager()->GetBasicItemStatsByName(doorLock.GetData());
            if(!itemstats)
            {
                Error2("Error: Action entrance failed to get item stats for item %s.\n",doorLock.GetData());
                return;
            }
        }

        // Make item
        psItem* lockItem = itemstats->InstantiateBasicItem();
        if(!lockItem)
        {
            Error2("Error: Action entrance failed to create item %s.\n",doorLock.GetData());
            return;
        }

        // Get client location for exit
        csVector3 pos;
        iSector* sector = 0;
        csString name = data->guildName;
        float angle;
        gemObject* object = client->GetActor();
        if(!object)
        {
            Error2("Error: Action entrance failed to get client actor pointer for client %s.\n",client->GetName());
            return;
        }

        // Get client position and sector
        object->GetPosition(pos, angle, sector);

        if(!data->sectorInfo)
        {
            Error2("Error: Action entrance failed to get sector using name %s.\n",data->sectorName.GetData());
            return;
        }

        // Setup the sign
        lockItem->SetStackCount(1);
        lockItem->SetLocationInWorld(0,data->sectorInfo,pos.x,pos.y,pos.z,angle);
        lockItem->SetOwningCharacter(NULL);
        lockItem->SetMaxItemQuality(50.0);

        // Assign the lock attributes and save to create ID
        lockItem->SetFlags(PSITEM_FLAG_UNPICKABLE | PSITEM_FLAG_SECURITYLOCK | PSITEM_FLAG_LOCKED | PSITEM_FLAG_LOCKABLE);
        lockItem->SetIsPickupable(false);
        lockItem->SetLoaded();
        lockItem->SetName(name);
        lockItem->Save(false);

        // Create lock in world
        if(!EntityManager::GetSingleton().CreateItem(lockItem,false))
        {
            delete lockItem;
            Error1("Error: Action entrance failed to create lock item.\n");
            return;
        }


        //-------------

        // Create new lock for entrance
        doorLock = "Simple Lock";
        itemstats=psserver->GetCacheManager()->GetBasicItemStatsByName(doorLock.GetData());
        if(!itemstats)
        {
            Error2("Error: Action entrance failed to get item stats for item %s.\n",doorLock.GetData());
            return;
        }

        // Make item
        lockItem = itemstats->InstantiateBasicItem();
        if(!lockItem)
        {
            Error2("Error: Action entrance failed to create item %s.\n",doorLock.GetData());
            return;
        }

        // Setup the lock item in instance 1
        lockItem->SetStackCount(1);
        lockItem->SetLocationInWorld(1,data->sectorInfo,pos.x,pos.y,pos.z,angle);
        lockItem->SetOwningCharacter(NULL);
        lockItem->SetMaxItemQuality(50.0);

        // Assign the lock attributes and save to create ID
        lockItem->SetFlags(PSITEM_FLAG_UNPICKABLE | PSITEM_FLAG_SECURITYLOCK | PSITEM_FLAG_LOCKED | PSITEM_FLAG_LOCKABLE);
        lockItem->SetIsPickupable(false);
        lockItem->SetIsSecurityLocked(true);
        lockItem->SetIsLocked(true);
        lockItem->SetLoaded();
        lockItem->SetName(name);
        lockItem->Save(false);

        // Create lock in world
        if(!EntityManager::GetSingleton().CreateItem(lockItem,false))
        {
            delete lockItem;
            Error1("Error: Action entrance failed to create lock item.\n");
            return;
        }

        // Get lock ID for response string
        uint32 lockID = lockItem->GetUID();

        // Get last targeted mesh
        csString meshTarget = client->GetMesh();

        // Create entrance name
        name.Format("Enter %s",data->guildName.GetData());

        // Create entrance response string
        csString resp = "<Examine><Entrance Type='ActionID' ";
        resp.AppendFmt("LockID='%u' ",lockID);
        resp.Append("X='0' Y='0' Z='0' Rot='3.14' ");
        resp.AppendFmt("Sector=\'%s\' />",data->sectorName.GetData());

        // Create return response string
        resp.AppendFmt("<Return X='%f' Y='%f' Z='%f' Rot='%f' Sector='%s' />",
                       pos.x,pos.y,pos.z,angle,data->sectorName.GetData());

        // Add on description
        resp.AppendFmt("<Description>%s</Description></Examine>",data->description.GetData());

        // Create entrance action location w/ position info since there will be many of these
        psActionLocation* actionLocation = new psActionLocation();
        actionLocation->SetName(name);
        actionLocation->SetSectorName(data->sectorName.GetData());
        actionLocation->SetMeshName(meshTarget);
        actionLocation->SetRadius(2.0);
        actionLocation->SetPosition(pos);
        actionLocation->SetTriggerType(psActionLocation::TRIGGERTYPE_SELECT);
        actionLocation->SetResponseType("EXAMINE");
        actionLocation->SetResponse(resp);
        actionLocation->SetIsEntrance(true);
        actionLocation->SetIsLockable(true);
        actionLocation->SetActive(false);
        actionLocation->Save();

        // Update Cache
        if(!psserver->GetActionManager()->CacheActionLocation(actionLocation))
        {
            Error2("Failed to create action %s.\n", actionLocation->name.GetData());
            delete actionLocation;
        }
        psserver->SendSystemInfo(me->clientnum, "Action location entrance created for %s.",data->sectorName.GetData());
    }
}

int AdminManager::PathPointCreate(int pathID, int prevPointId, csVector3 &pos, csString &sectorName)
{
    const char* fieldnames[]=
    {
        "path_id",
        "prev_point",
        "x",
        "y",
        "z",
        "loc_sector_id"
    };

    psSectorInfo* si = psserver->GetCacheManager()->GetSectorInfoByName(sectorName);
    if(!si)
    {
        Error2("No sector info for %s",sectorName.GetDataSafe());
        return -1;
    }

    psStringArray values;
    values.FormatPush("%u", pathID);
    values.FormatPush("%u", prevPointId);
    values.FormatPush("%10.2f",pos.x);
    values.FormatPush("%10.2f",pos.y);
    values.FormatPush("%10.2f",pos.z);
    values.FormatPush("%u",si->uid);

    unsigned int id = db->GenericInsertWithID("sc_path_points",fieldnames,values);
    if(id==0)
    {
        Error2("Failed to create new Path Point Error %s",db->GetLastError());
        return -1;
    }

    return id;
}

void AdminManager::FindPath(csVector3 &pos, iSector* sector, float radius,
                            Waypoint** wp, float* rangeWP,
                            psPath** path, float* rangePath, int* indexPath, float* fraction,
                            psPath** pointPath, float* rangePoint, int* indexPoint)
{

    if(wp)
    {
        *wp = pathNetwork->FindNearestWaypoint(pos,sector,radius,rangeWP);
    }
    if(path)
    {
        *path = pathNetwork->FindNearestPath(pos,sector,radius,rangePath,indexPath,fraction);
    }
    if(pointPath)
    {
        *pointPath = pathNetwork->FindNearestPoint(pos,sector,radius,rangePoint,indexPoint);
    }
    // If both wp and point only return the one the nearest
    if(wp && pointPath && *wp && *pointPath)
    {
        if(*rangeWP < *rangePoint)
        {
            *pointPath = NULL;
        }
        else
        {
            *wp = NULL;
        }
    }
}

uint32_t AdminManager::GetEffectID()
{
    return psserver->GetCacheManager()->NextEffectUID();
}

void AdminManager::HideAllPaths(bool clearSelected)
{
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();

        // Cleare selected while we are iterating over the clients.
        if(clearSelected)
        {
            client->PathSetPath(NULL);
        }

        if(client->PathIsDisplaying())
        {
            HidePaths(client);
        }
        if(client->WaypointIsDisplaying())
        {
            HideWaypoints(client);
        }
    }
}

void AdminManager::HidePaths(Client* client)
{
    if(client->PathIsDisplaying())
    {
        csList<iSector*>::Iterator iter = client->GetPathDisplaying();
        while(iter.HasNext())
        {
            iSector* sector = iter.Next();
            HidePaths(client, sector);
        }
    }
}

void AdminManager::HideWaypoints(Client* client)
{
    if(client->WaypointIsDisplaying())
    {
        csList<iSector*>::Iterator iter = client->GetWaypointDisplaying();
        while(iter.HasNext())
        {
            iSector* sector = iter.Next();
            HideWaypoints(client, sector);
        }
    }
}

void AdminManager::HidePaths(Client* client, iSector* sector)
{
    csList<psPathPoint*> list;
    if(pathNetwork->FindPointsInSector(sector,list))
    {
        csList<psPathPoint*>::Iterator iter(list);
        while(iter.HasNext())
        {
            psPathPoint* point = iter.Next();

            psStopEffectMessage msg(client->GetClientNum(), point->GetEffectID(this));
            msg.SendMessage();
        }
    }
}

void AdminManager::HideWaypoints(Client* client, iSector* sector)
{
    csList<Waypoint*> list;
    if(pathNetwork->FindWaypointsInSector(sector,list))
    {
        csList<Waypoint*>::Iterator iter(list);
        while(iter.HasNext())
        {
            Waypoint* waypoint = iter.Next();
            psStopEffectMessage msg(client->GetClientNum(), waypoint->GetEffectID(this));
            msg.SendMessage();
        }
    }
}


void AdminManager::RedisplayAllPaths()
{
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();

        if(client->PathIsDisplaying())
        {
            ShowPaths(client);
        }
        if(client->WaypointIsDisplaying())
        {
            ShowWaypoints(client);
        }

    }
}

void AdminManager::ShowPaths(Client* client)
{
    if(client->PathIsDisplaying())
    {
        csList<iSector*>::Iterator iter = client->GetPathDisplaying();
        while(iter.HasNext())
        {
            iSector* sector = iter.Next();
            ShowPaths(client, sector);
        }
    }
}

void AdminManager::ShowWaypoints(Client* client)
{
    if(client->WaypointIsDisplaying())
    {
        csList<iSector*>::Iterator iter = client->GetWaypointDisplaying();
        while(iter.HasNext())
        {
            iSector* sector = iter.Next();
            ShowWaypoints(client, sector);
        }
    }
}

void AdminManager::ShowPaths(Client* client, iSector* sector)
{
    csList<psPathPoint*> list;
    if(pathNetwork->FindPointsInSector(sector,list))
    {
        csList<psPathPoint*>::Iterator iter(list);
        while(iter.HasNext())
        {
            psPathPoint* point = iter.Next();

            // Don't send enpoints of paths
            if(!point->GetWaypoint())
            {
                psEffectMessage msg(client->GetClientNum(),"admin_path_point",
                                    point->GetPosition(),0,0,point->GetEffectID(this),point->GetRadius());
                msg.SendMessage();
            }

            int index = point->GetPathIndex();
            if(index > 0)
            {
                const psPathPoint* prev = point->GetPath()->GetPoint(index-1);

                // Check that the point is in same sector.
                // TODO fix for other sectors
                if(prev->GetSector(EntityManager::GetSingleton().GetEngine()) == sector)
                {
                    csVector3 delta = point->GetPosition() - prev->GetPosition();

                    float angle = atan2(delta.x,delta.z);
                    float length = delta.Norm();

                    psEffectMessage msg(client->GetClientNum(),"admin_path_segment",
                                        point->GetPosition(),0,0,point->GetEffectID(this),angle,length,point->GetRadius(),prev->GetRadius());
                    msg.SendMessage();
                }
            }
        }
    }
}

void AdminManager::ShowWaypoints(Client* client, iSector* sector)
{
    csList<Waypoint*> list;
    if(pathNetwork->FindWaypointsInSector(sector,list))
    {
        csList<Waypoint*>::Iterator iter(list);
        while(iter.HasNext())
        {
            Waypoint* waypoint = iter.Next();

            psEffectMessage msg(client->GetClientNum(),"admin_waypoint",
                                waypoint->GetPosition(),0,0,waypoint->GetEffectID(this),waypoint->GetRadius());
            msg.SendMessage();
        }
    }
}

void AdminManager::UpdateDisplayPath(psPathPoint* point)
{
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();

        if(client->PathIsDisplaying())
        {
            csList<iSector*>::Iterator iter = client->GetPathDisplaying();
            while(iter.HasNext())
            {
                iSector* sector = iter.Next();

                // Hide
                psStopEffectMessage hide(client->GetClientNum(), point->GetEffectID(this));
                hide.SendMessage();
                // Display
                psEffectMessage show(client->GetClientNum(),"admin_path_point",point->GetPosition(),0,0,point->GetEffectID(this),point->GetRadius());
                show.SendMessage();

                int index = point->GetPathIndex();
                if(index > 0)
                {
                    const psPathPoint* prev = point->GetPath()->GetPoint(index-1);

                    // Check that the point is in same sector.
                    // TODO fix for other sectors
                    if(prev->GetSector(EntityManager::GetSingleton().GetEngine()) == sector)
                    {
                        csVector3 delta = point->GetPosition() - prev->GetPosition();

                        float angle = atan2(delta.x,delta.z);
                        float length = delta.Norm();

                        psEffectMessage msg(client->GetClientNum(),"admin_path_segment",
                                            point->GetPosition(),0,0,point->GetEffectID(this),angle,length,point->GetRadius(),prev->GetRadius());
                        msg.SendMessage();
                    }
                }

                if(index < (point->GetPath()->GetNumPoints()-1))
                {
                    UpdateDisplayPath(point->GetPath()->GetPoint(index+1));
                }

            }
        }
    }
}

void AdminManager::UpdateDisplayWaypoint(Waypoint* wp)
{
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();

        if(client->WaypointIsDisplaying())
        {
            // Hide
            psStopEffectMessage hide(client->GetClientNum(), wp->GetEffectID(this));
            hide.SendMessage();

            csList<iSector*>::Iterator iter = client->GetWaypointDisplaying();
            while(iter.HasNext())
            {
                iSector* sector = iter.Next();

                if(client->GetActor()->GetSector() == sector)
                {
                    // Display
                    // TODO: Include sector in psEffectMessage
                    psEffectMessage show(client->GetClientNum(),"admin_waypoint",wp->GetPosition(),0,0,
                                         wp->GetEffectID(this),wp->GetRadius());
                    show.SendMessage();

                    csPDelArray<Edge>::Iterator iter = wp->edges.GetIterator();
                    while(iter.HasNext())
                    {
                        Edge* edge = iter.Next();

                        UpdateDisplayPath(edge->GetStartPoint());
                    }
                }
            }
        }
    }
}

void AdminManager::HandlePath(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataPath* data = static_cast<AdminCmdDataPath*>(cmddata);

    // Some variables needed by most functions
    csVector3 myPos;
    float myRotY;
    iSector* mySector = 0;
    csString mySectorName;

    client->GetActor()->GetPosition(myPos, myRotY, mySector);
    mySectorName = mySector->QueryObject()->GetName();

    if(data->subCmd == "adjust")
    {
        float rangeWP,rangePath,rangePoint,fraction;
        int index,indexPoint;

        Waypoint* wp = NULL;
        psPath* path = NULL;
        psPath* pathPoint = NULL;

        FindPath(myPos,mySector,data->radius,
                 &wp,&rangeWP,
                 &path,&rangePath,&index,&fraction,
                 &pathPoint,&rangePoint,&indexPoint);

        psPathPoint* point = NULL;
        if(pathPoint)
        {
            point = pathPoint->points[indexPoint];
        }

        if(!wp && !pathPoint)
        {
            psserver->SendSystemInfo(me->clientnum, "No path point or waypoint in range of %.2f.",data->radius);
            return;
        }

        if(wp)
        {
            if(wp->Adjust(db,myPos,mySectorName))
            {
                psserver->npcmanager->WaypointAdjusted(wp);

                UpdateDisplayWaypoint(wp);

                psserver->SendSystemInfo(me->clientnum,
                                         "Adjusted waypoint %s(%d) at range %.2f",
                                         wp->GetName(), wp->GetID(), rangeWP);
            }
        }
        if(point)
        {
            if(pathPoint->Adjust(db,indexPoint,myPos,mySectorName))
            {
                psserver->npcmanager->PathPointAdjusted(point);

                UpdateDisplayPath(point);

                psserver->SendSystemInfo(me->clientnum,
                                         "Adjusted point(%d) %d of path %s(%d) at range %.2f",
                                         point->GetID(),indexPoint,path->GetName(),path->GetID(),rangePoint);
            }
        }
    }
    else if(data->subCmd == "alias")
    {
        float rangeWP,rangePoint;
        int indexPoint;

        Waypoint* wp = NULL;
        psPath* pathPoint = NULL;

        FindPath(myPos,mySector,data->radius,
                 &wp,&rangeWP,
                 NULL,NULL,NULL,NULL,
                 &pathPoint,&rangePoint,&indexPoint);

        // adding an alias
        if(data->aliasSubCmd == "add")
        {
            if(!wp)
            {
                psserver->SendSystemError(me->clientnum, "No waypoint nearby.");
                return;
            }

            // Check if alias is used before
            Waypoint* existing = pathNetwork->FindWaypoint(data->alias.GetDataSafe());
            if(existing)
            {
                psserver->SendSystemError(me->clientnum, "Waypoint already exists with the name %s", data->alias.GetDataSafe());
                return;
            }

            // Create the alias in db
            WaypointAlias* alias = wp->CreateAlias(db, data->alias, data->rotationAngle);
            if(alias)
            {
                psserver->npcmanager->WaypointAddAlias(wp,alias);

                psserver->SendSystemInfo(me->clientnum, "Added alias %s to waypoint %s(%d)",
                                         alias->GetName(),wp->GetName(),wp->GetID());
            }
        }
        else if(data->aliasSubCmd == "remove")
        {
            // Check if alias is used before
            Waypoint* wp = pathNetwork->FindWaypoint(data->alias.GetDataSafe());
            if(!wp)
            {
                psserver->SendSystemError(me->clientnum, "No waypoint with %s as alias", data->waypoint.GetDataSafe());
                return;
            }

            if(strcasecmp(wp->GetName(), data->alias.GetDataSafe())==0)
            {
                psserver->SendSystemError(me->clientnum, "Can't remove the name of the waypoint", data->waypoint.GetDataSafe());
                return;
            }


            // Remove the alias from db
            if(wp->RemoveAlias(db, data->alias))
            {
                psserver->npcmanager->WaypointRemoveAlias(wp,data->alias);

                psserver->SendSystemInfo(me->clientnum, "Removed alias %s from waypoint %s(%d)",
                                         data->alias.GetDataSafe(),wp->GetName(),wp->GetID());
            }
        }
        else if(data->aliasSubCmd == "rotation")
        {
            // Check if alias is used before
            WaypointAlias* alias;
            Waypoint* wp = pathNetwork->FindWaypoint(data->alias.GetDataSafe(),&alias);
            if(!wp)
            {
                psserver->SendSystemError(me->clientnum, "No waypoint with %s as alias", data->alias.GetDataSafe());
                return;
            }

            if(!alias)
            {
                psserver->SendSystemError(me->clientnum, "%s isn't a alias of waypoint %s(%d)", data->alias.GetDataSafe(), wp->GetName(),wp->GetID());
                return;
            }

            if(alias->SetRotationAngle(db, data->rotationAngle))
            {
                psserver->npcmanager->WaypointAliasRotation(wp,alias);

                psserver->SendSystemInfo(me->clientnum, "Changed rotation angle for alias %s of waypoint %s(%d) to %.1f",
                                         alias->GetName(),wp->GetName(),wp->GetID(),alias->GetRotationAngle()*180.0/PI);
            }
        }
    }
    else if(data->subCmd == "flagset" || data->subCmd == "flagclear")
    {
        float rangeWP,rangePath;

        Waypoint* wp = NULL;
        psPath* path = NULL;

        FindPath(myPos,mySector,data->radius,
                 &wp,&rangeWP,
                 &path,&rangePath,NULL,NULL,
                 NULL,NULL,NULL);

        bool enable = data->subCmd == "flagset";

        if(data->wpOrPathIsWP)
        {
            if(!wp)
            {
                psserver->SendSystemInfo(me->clientnum, "No waypoint in range of %.2f.",data->radius);
                return;
            }

            if(wp->SetFlag(db, data->flagName, enable))
            {
                psserver->npcmanager->WaypointSetFlag(wp,data->flagName,enable);

                psserver->SendSystemInfo(me->clientnum, "Flag %s %s for %s.",
                                         data->flagName.GetDataSafe(),enable?"updated":"cleared",wp->GetName());
                return;
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum, "Failed to update flag %s for %s.",
                                         data->flagName.GetDataSafe(),wp->GetName());
                return;
            }
        }
        else
        {
            if(!path)
            {
                psserver->SendSystemInfo(me->clientnum, "No path in range of %.2f.",data->radius);
                return;
            }

            if(path->SetFlag(db, data->flagName, enable))
            {
                psserver->npcmanager->PathSetFlag(path,data->flagName,enable);

                psserver->SendSystemInfo(me->clientnum, "Flag %s %s for %s.",
                                         data->flagName.GetDataSafe(),enable?"updated":"cleared",path->GetName());
                return;
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum, "Failed to update flag %s for %s.",
                                         data->flagName.GetDataSafe(),path->GetName());
                return;
            }
        }
    }
    else if(data->subCmd == "radius")
    {
        float rangeWP,rangePoint;
        int indexPoint;

        Waypoint* wp = NULL;
        psPath* pathPoint = NULL;

        FindPath(myPos,mySector,data->radius,
                 &wp,&rangeWP,
                 NULL,NULL,NULL,NULL,
                 &pathPoint,&rangePoint,&indexPoint);

        if(!wp)
        {
            psserver->SendSystemInfo(me->clientnum, "No waypoint in range of %.2f.",data->radius);
            return;
        }

        if(wp->SetRadius(db, data->newRadius))
        {
            psserver->npcmanager->WaypointRadius(wp);

            wp->RecalculateEdges(EntityManager::GetSingleton().GetWorld(),EntityManager::GetSingleton().GetEngine());

            UpdateDisplayWaypoint(wp);
            psserver->SendSystemInfo(me->clientnum, "Waypoint %s updated with new radius %.3f.",
                                     wp->GetName(),wp->GetRadius());
            return;
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "Failed to update radius for %s.",
                                     wp->GetName());
            return;
        }
    }
    else if(data->subCmd == "format")
    {
        client->WaypointSetPath(data->waypointPathName,data->firstIndex);
        csString wp;
        wp.Format(client->WaypointGetPathName(),client->WaypointGetPathIndex());
        psserver->SendSystemInfo(me->clientnum, "New path format, first new WP will be: '%s'",wp.GetDataSafe());
    }
    else if(data->subCmd == "point add")
    {
        psPath* path = client->PathGetPath();
        if(!path)
        {
            psserver->SendSystemError(me->clientnum, "You have no path. Please start/select one.");
            return;
        }

        psPathPoint* point = path->AddPoint(db, myPos, mySectorName);
        if(point)
        {
            // If path hasn't been created yet, don't push to superclients
            if(path->GetID() > 0)
            {
                psserver->npcmanager->AddPoint(path, point);
            }

            UpdateDisplayPath(point);
            psserver->SendSystemInfo(me->clientnum, "Added point.");
        }
    }
    else if(data->subCmd == "point remove")
    {
        psPath* path = client->PathGetPath();
        if(!path)
        {
            psserver->SendSystemError(me->clientnum, "You have no path. Please start/select one.");
            return;
        }

        psPathPoint* point = NULL;

        if((point = pathNetwork->FindNearestPoint(path, myPos, mySector, data->radius)) == NULL)
        {
            psserver->SendSystemError(me->clientnum, "Found no path point near you at selected path %s.",path->GetName());
            return;
        }

        int pointId = point->GetID();

        if(path->RemovePoint(db, point))
        {
            psserver->npcmanager->RemovePoint(path, pointId);

            if(client->PathIsDisplaying())
            {
                psStopEffectMessage msg(me->clientnum, point->GetEffectID(this));
                msg.SendMessage();
            }

            psserver->SendSystemInfo(me->clientnum, "Removed point.");
        }
    }
    else if(data->subCmd == "point insert")
    {
        psPath* path = client->PathGetPath();
        if(!path)
        {
            psserver->SendSystemError(me->clientnum, "You have no path. Please start/select one.");
            return;
        }

        int index;

        if(!pathNetwork->FindPoint(path, myPos, mySector, data->radius, index))
        {
            psserver->SendSystemError(me->clientnum, "Found no path point near you at selected path %s.",path->GetName());
            return;
        }

        psPathPoint* newPoint = path->InsertPoint(db, index+1, myPos, mySectorName);
        if(!newPoint)
        {
            psserver->SendSystemError(me->clientnum, "Failed to insert point.");
            return;
        }


        UpdateDisplayPath(newPoint);
        psserver->SendSystemInfo(me->clientnum, "Inserted point.");
    }
    else if(data->subCmd == "start")
    {
        float range;

        psPath* path = client->PathGetPath();

        if(path)
        {
            if(path->GetID() == -1)  // No ID yet -> Just started not ended.
            {
                psserver->SendSystemError(me->clientnum, "You already have a path started.");
                return;
            }
            else
            {
                psserver->SendSystemError(me->clientnum, "Stopping selected path.");
                client->PathSetPath(NULL);
            }
        }

        if(client->WaypointGetPathName().IsEmpty())
        {
            psserver->SendSystemError(me->clientnum, "No path format set yet.");
            return;
        }

        Waypoint* wp = pathNetwork->FindNearestWaypoint(myPos,mySector,2.0,&range);
        if(wp)
        {
            psserver->SendSystemInfo(me->clientnum, "Starting path, using existing waypoint %s(%d) at range %.2f",
                                     wp->GetName(), wp->GetID(), range);
        }
        else
        {
            csString wpName;
            wpName.Format(client->WaypointGetPathName(),client->WaypointGetNewPathIndex());

            Waypoint* existing = pathNetwork->FindWaypoint(wpName);
            if(existing)
            {
                psserver->SendSystemError(me->clientnum, "Waypoint already exists with the name %s", wpName.GetDataSafe());
                return;
            }

            if((wp = pathNetwork->CreateWaypoint(db, wpName,myPos,mySectorName,data->radius,data->flagName))!=NULL)
            {
                psserver->npcmanager->WaypointCreate(wp);

                UpdateDisplayWaypoint(wp);
                psserver->SendSystemInfo(me->clientnum, "Starting path, using new waypoint %s(%d)",
                                         wp->GetName(), wp->GetID());
            }
        }
        path = new psLinearPath(-1,"",data->pathFlags);
        path->SetStart(wp);
        client->PathSetPath(path);
    }
    else if(data->subCmd == "stop")
    {
        float range;
        psPath* path = client->PathGetPath();

        if(!path)
        {
            psserver->SendSystemError(me->clientnum, "You have no path started.");
            return;
        }

        Waypoint* wp = pathNetwork->FindNearestWaypoint(myPos,mySector,2.0,&range);
        if(wp)
        {
            // A path from a waypoint can't be used to anything unless it has a name
            if(wp == path->start && path->GetName() == csString(""))
            {
                psserver->SendSystemInfo(me->clientnum, "Can't create a path back to same waypoint %s(%d) at "
                                         "range %.2f without name.",
                                         wp->GetName(), wp->GetID(), range);
                return;
            }

            // A path returning to the same point dosn't seams to make any sence unless there is a serten
            // number of path points.
            if(wp == path->start &&  path->GetNumPoints() < 3)
            {
                psserver->SendSystemInfo(me->clientnum, "Can't create a path back to same waypoint %s(%d) at "
                                         "range %.2f without more path points.",
                                         wp->GetName(), wp->GetID(), range);
                return;
            }

            psserver->SendSystemInfo(me->clientnum, "Stoping path using existing waypoint %s(%d) at range %.2f",
                                     wp->GetName(), wp->GetID(), range);
        }
        else
        {
            csString wpName;
            wpName.Format(client->WaypointGetPathName(),client->WaypointGetNewPathIndex());

            Waypoint* existing = pathNetwork->FindWaypoint(wpName);
            if(existing)
            {
                psserver->SendSystemError(me->clientnum, "Waypoint already exists with the name %s", wpName.GetDataSafe());
                return;
            }

            if((wp = pathNetwork->CreateWaypoint(db, wpName,myPos,mySectorName,data->radius,data->waypointFlags)) != NULL)
            {
                psserver->npcmanager->WaypointCreate(wp);

                UpdateDisplayWaypoint(wp);
            }
        }

        client->PathSetPath(NULL);
        path->SetEnd(wp);
        path = pathNetwork->CreatePath(db, path);
        if(!path)
        {
            psserver->SendSystemError(me->clientnum, "Failed to create path");
        }
        else
        {
            psserver->npcmanager->PathCreate(path);

            psserver->SendSystemInfo(me->clientnum, "New path %s(%d) created between %s(%d) and %s(%d)",
                                     path->GetName(),path->GetID(),path->start->GetName(),path->start->GetID(),
                                     path->end->GetName(),path->end->GetID());
        }
    }
    else if(data->subCmd == "display")
    {
        if(data->cmdTarget.IsEmpty() || data->cmdTarget == 'P')
        {
            ShowPaths(client,mySector);

            client->PathSetIsDisplaying(mySector);
            psserver->SendSystemInfo(me->clientnum, "Displaying all path points in sector %s",mySectorName.GetDataSafe());
        }
        if(data->cmdTarget.IsEmpty() || data->cmdTarget == 'W')
        {
            ShowWaypoints(client, mySector);

            client->WaypointSetIsDisplaying(mySector);
            psserver->SendSystemInfo(me->clientnum, "Displaying all waypoints in sector %s",mySectorName.GetDataSafe());
        }
    }
    else if(data->subCmd == "hide")
    {
        if(data->cmdTarget.IsEmpty() || data->cmdTarget == 'P')
        {
            HidePaths(client);

            client->PathClearDisplaying();
            psserver->SendSystemInfo(me->clientnum, "All path points hidden");
        }
        if(data->cmdTarget.IsEmpty() || data->cmdTarget == "W")
        {
            HideWaypoints(client);

            client->WaypointClearDisplaying();
            psserver->SendSystemInfo(me->clientnum, "All waypoints hidden");
        }
    }
    else if(data->subCmd == "info")
    {
        float rangeWP,rangePath,rangePoint,fraction;
        int index,indexPoint;

        Waypoint* wp = NULL;
        psPath* path = NULL;
        psPath* pathPoint = NULL;

        FindPath(myPos,mySector,data->radius,
                 &wp,&rangeWP,
                 &path,&rangePath,&index,&fraction,
                 &pathPoint,&rangePoint,&indexPoint);

        psPathPoint* point = NULL;
        if(pathPoint)
        {
            point = pathPoint->points[indexPoint];
        }

        if(!wp && !path && !point)
        {
            psserver->SendSystemInfo(me->clientnum, "No point, path or waypoint in range of %.2f.",data->radius);
            return;
        }

        if(wp)
        {
            csString links;
            for(size_t i = 0; i < wp->links.GetSize(); i++)
            {
                if(i!=0)
                {
                    links.Append(", ");
                }
                links.AppendFmt("%s(%d)",wp->links[i]->GetName(),wp->links[i]->GetID());
            }

            psserver->SendSystemInfo(me->clientnum,
                                     "Found waypoint %s(%d) at range %.2f\n"
                                     " Radius: %.2f\n"
                                     " Flags: %s\n"
                                     " Aliases: %s\n"
                                     " Group: %s\n"
                                     " Links: %s",
                                     wp->GetName(),wp->GetID(),rangeWP,
                                     wp->loc.radius,wp->GetFlags().GetDataSafe(),
                                     wp->GetAliases().GetDataSafe(),
                                     wp->GetGroup(),
                                     links.GetDataSafe());
        }
        if(point)
        {
            psserver->SendSystemInfo(me->clientnum,
                                     "Found point(%d) %d of path: %s(%d) at range %.2f\n"
                                     " Prev point ID: %d Next point ID: %d\n"
                                     " Start WP: %s(%d) End WP: %s(%d)\n"
                                     " Flags: %s",
                                     point->GetID(),indexPoint,pathPoint->GetName(),
                                     pathPoint->GetID(),rangePoint,
                                     indexPoint-1>=0?pathPoint->points[indexPoint-1]->GetID():-1,
                                     indexPoint+1<(int)pathPoint->points.GetSize()?pathPoint->points[indexPoint+1]->GetID():-1,
                                     pathPoint->start->GetName(),pathPoint->start->GetID(),
                                     pathPoint->end->GetName(),pathPoint->end->GetID(),
                                     pathPoint->GetFlags().GetDataSafe());

        }
        if(path)
        {
            float length = path->GetLength(EntityManager::GetSingleton().GetWorld(),EntityManager::GetSingleton().GetEngine(),index);

            psserver->SendSystemInfo(me->clientnum,
                                     "Found path: %s(%d) at range %.2f\n"
                                     " %.2f from point %d%s and %.2f from point %d%s\n"
                                     " Start WP: %s(%d) End WP: %s(%d)\n"
                                     " Flags: %s",
                                     path->GetName(),path->GetID(),rangePath,
                                     fraction*length,index,(fraction < 0.5?"*":""),
                                     (1.0-fraction)*length,index+1,(fraction >= 0.5?"*":""),
                                     path->start->GetName(),path->start->GetID(),
                                     path->end->GetName(),path->end->GetID(),
                                     path->GetFlags().GetDataSafe());
        }

    }
    else if(data->subCmd == "move")
    {
        if(data->wpOrPathIsWP)  // Request to move a waypoint
        {
            Waypoint* wp = NULL;

            if(data->wpOrPathIsIndex)
            {
                wp = pathNetwork->FindWaypoint(data->waypointPathIndex);
            }
            else
            {
                wp = pathNetwork->FindWaypoint(data->waypointPathName.GetDataSafe());
            }

            if(wp)
            {
                if(wp->Adjust(db,myPos,mySectorName))
                {
                    psserver->npcmanager->WaypointAdjusted(wp);

                    UpdateDisplayWaypoint(wp);

                    psserver->SendSystemInfo(me->clientnum,
                                             "Moved waypoint %s(%d)",
                                             wp->GetName(), wp->GetID());
                }
            }
            else
            {
                if(data->wpOrPathIsIndex)
                {
                    psserver->SendSystemInfo(me->clientnum, "No waypoint found for index: %d", data->waypointPathIndex);
                }
                else
                {
                    psserver->SendSystemInfo(me->clientnum, "No waypoint found for name: %s", data->waypointPathName.GetData());
                }
                return;
            }
        }
        else  // Request to move a path point
        {
            psPathPoint* point = NULL;

            if(data->wpOrPathIsIndex)
            {
                point = pathNetwork->FindPathPoint(data->waypointPathIndex);
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum, "Not supported.");
                // point = pathNetwork->FindPathPoint(data->waypointPathName.GetDataSafe());
                return;
            }

            if(point)
            {
                if(point->Adjust(db,myPos,mySectorName))
                {
                    psserver->npcmanager->PathPointAdjusted(point);

                    UpdateDisplayPath(point);

                    psserver->SendSystemInfo(me->clientnum,
                                             "Adjusted point(%d)",
                                             point->GetID());
                }
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum, "No point found");
                return;
            }
        }
    }
    else if(data->subCmd == "select")
    {
        float range;

        psPath* path = pathNetwork->FindNearestPath(myPos,mySector,100.0,&range);
        if(!path)
        {
            client->PathSetPath(NULL);
            psserver->SendSystemError(me->clientnum, "Didn't find any path close by");
            return;
        }
        client->PathSetPath(path);
        psserver->SendSystemInfo(me->clientnum, "Selected path %s(%d) from %s(%d) to %s(%d) at range %.1f",
                                 path->GetName(),path->GetID(),path->start->GetName(),path->start->GetID(),
                                 path->end->GetName(),path->end->GetID(),range);
    }
    else if(data->subCmd == "split")
    {
        float range;

        psPath* path = pathNetwork->FindNearestPath(myPos,mySector,100.0,&range);
        if(!path)
        {
            psserver->SendSystemError(me->clientnum, "Didn't find any path close by");
            return;
        }

        if(client->WaypointGetPathName().IsEmpty())
        {
            psserver->SendSystemError(me->clientnum, "No path format set yet.");
            return;
        }

        csString wpName;
        wpName.Format(client->WaypointGetPathName(),client->WaypointGetNewPathIndex());

        Waypoint* existing = pathNetwork->FindWaypoint(wpName);
        if(existing)
        {
            psserver->SendSystemError(me->clientnum, "Waypoint already exists with the name %s", wpName.GetDataSafe());
            return;
        }

        Waypoint* wp = pathNetwork->CreateWaypoint(db, wpName,myPos,mySectorName,data->radius,data->waypointFlags);
        if(!wp)
        {
            return;
        }
        UpdateDisplayWaypoint(wp);

        psPath* path1 = pathNetwork->CreatePath(db, "",path->start,wp, "");
        psPath* path2 = pathNetwork->CreatePath(db, "",wp,path->end, "");

        psserver->SendSystemInfo(me->clientnum, "Splitted %s(%d) into %s(%d) and %s(%d)",
                                 path->GetName(),path->GetID(),
                                 path1->GetName(),path1->GetID(),
                                 path2->GetName(),path2->GetID());

        // Warning: This will delete all path points. So they has to be rebuild for the new segments.
        pathNetwork->Delete(path);
    }
    else if(data->subCmd == "remove")
    {
        float range;

        psPath* path = pathNetwork->FindNearestPath(myPos,mySector,data->radius,&range);
        if(!path)
        {
            psserver->SendSystemError(me->clientnum, "Didn't find any path close by");
            return;
        }

        psserver->SendSystemInfo(me->clientnum, "Deleted path %s(%d) between %s(%d) and %s(%d)",
                                 path->GetName(),path->GetID(),
                                 path->start->GetName(),path->start->GetID(),
                                 path->end->GetName(),path->end->GetID());

        // Warning: This will delete all path points. So they has to be rebuild for the new segments.
        pathNetwork->Delete(path);
    }
    else if(data->subCmd == "rename")
    {
        float rangeWP,rangePath,rangePoint,fraction;
        int index,indexPoint;

        Waypoint* wp = NULL;
        psPath* path = NULL;
        psPath* pathPoint = NULL;

        FindPath(myPos,mySector,data->radius,
                 &wp,&rangeWP,
                 &path,&rangePath,&index,&fraction,
                 &pathPoint,&rangePoint,&indexPoint);

        if(!wp && !path)
        {
            psserver->SendSystemInfo(me->clientnum, "No point or path in range of %.2f.",data->radius);
            return;
        }

        if(wp)
        {
            Waypoint* existing = pathNetwork->FindWaypoint(data->waypoint);
            if(existing)
            {
                psserver->SendSystemError(me->clientnum, "Waypoint already exists with the name %s", data->waypoint.GetDataSafe());
                return;
            }

            csString oldName = wp->GetName();
            if(wp->Rename(db, data->waypoint))
            {
                psserver->npcmanager->WaypointRename(wp);

                psserver->SendSystemInfo(me->clientnum, "Renamed waypoint %s(%d) to %s",
                                         oldName.GetDataSafe(),wp->GetID(),wp->GetName());
            }
        }
        else if(path)
        {
            psPath* existing = pathNetwork->FindPath(data->waypoint);
            if(existing)
            {
                psserver->SendSystemError(me->clientnum, "Path already exists with the name %s", data->waypoint.GetDataSafe());
                return;
            }

            csString oldName = path->GetName();
            if(path->Rename(db, data->waypoint))
            {
                psserver->npcmanager->PathRename(path);

                psserver->SendSystemInfo(me->clientnum, "Renamed path %s(%d) to %s",
                                         oldName.GetDataSafe(),path->GetID(),path->GetName());
            }
        }
    }
}


int AdminManager::LocationCreate(int typeID, csVector3 &pos, csString &sectorName, csString &name, int radius)
{
    const char* fieldnames[]=
    {
        "type_id",
        "id_prev_loc_in_region",
        "name",
        "x",
        "y",
        "z",
        "radius",
        "angle",
        "flags",
        "loc_sector_id",

    };

    psSectorInfo* si = psserver->GetCacheManager()->GetSectorInfoByName(sectorName);

    psStringArray values;
    values.FormatPush("%u", typeID);
    values.FormatPush("%d", -1);
    values.FormatPush("%s", name.GetDataSafe());
    values.FormatPush("%10.2f",pos.x);
    values.FormatPush("%10.2f",pos.y);
    values.FormatPush("%10.2f",pos.z);
    values.FormatPush("%d", radius);
    values.FormatPush("%.2f", 0.0);
    values.FormatPush("%s", "");
    values.FormatPush("%u",si->uid);

    unsigned int id = db->GenericInsertWithID("sc_locations",fieldnames,values);
    if(id==0)
    {
        Error2("Failed to create new Location Error %s",db->GetLastError());
        return -1;
    }

    return id;
}

void AdminManager::ShowLocations(Client* client)
{
    if(client->LocationIsDisplaying())
    {
        csList<iSector*>::Iterator iter = client->GetLocationDisplaying();
        while(iter.HasNext())
        {
            iSector* sector = iter.Next();
            ShowLocations(client, sector);
        }
    }
}

void AdminManager::ShowLocations(Client* client, iSector* sector)
{
    csList<Location*> list;
    if(locations->FindLocationsInSector(EntityManager::GetSingleton().GetEngine(), sector, list))
    {
        csList<Location*>::Iterator iter(list);
        while(iter.HasNext())
        {
            Location* location = iter.Next();

            psEffectMessage msg(client->GetClientNum(),"admin_location",
                                location->GetPosition(),0,0,location->GetEffectID(this),location->GetRadius());
            msg.SendMessage();

            if(location->IsRegion())
            {
                for(size_t i = 0; i < location->locs.GetSize(); i++)
                {
                    Location* loc = location->locs[i];
                    Location* next = location->locs[(i+1)%location->locs.GetSize()];

                    csVector3 delta = next->GetPosition() - loc->GetPosition();

                    float angle = atan2(delta.x,delta.z);
                    float length = delta.Norm();

                    psEffectMessage msg(client->GetClientNum(),"admin_location",
                                        next->GetPosition(),0,0,next->GetEffectID(this),0.1f,length,angle);
                    msg.SendMessage();
                }
            }
        }
    }
}

void AdminManager::HideAllLocations(bool clearSelected)
{
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();

        // Cleare selected while we are iterating over the clients.
        if(clearSelected)
        {
            client->SetSelectedLocationID(-1);
        }

        if(client->LocationIsDisplaying())
        {
            HideLocations(client);
        }
    }
}

void AdminManager::HideLocations(Client* client)
{
    if(client->LocationIsDisplaying())
    {
        csList<iSector*>::Iterator iter = client->GetLocationDisplaying();
        while(iter.HasNext())
        {
            iSector* sector = iter.Next();
            HideLocations(client, sector);
        }
    }
}

void AdminManager::HideLocations(Client* client, iSector* sector)
{
    csList<Location*> list;
    if(locations->FindLocationsInSector(EntityManager::GetSingleton().GetEngine(), sector, list))
    {
        csList<Location*>::Iterator iter(list);
        while(iter.HasNext())
        {
            Location* location = iter.Next();
            psStopEffectMessage msg(client->GetClientNum(), location->GetEffectID(this));
            msg.SendMessage();

            if(location->IsRegion())
            {
                for(size_t i = 0; i < location->locs.GetSize(); i++)
                {
                    Location* loc = location->locs[i];
                    psStopEffectMessage msg(client->GetClientNum(), loc->GetEffectID(this));
                    msg.SendMessage();
                }
            }
        }
    }
}

void AdminManager::RedisplayAllLocations()
{
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();

        if(client->LocationIsDisplaying())
        {
            ShowLocations(client);
        }
    }
}

void AdminManager::UpdateDisplayLocation(Location* location)
{
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();

        if(client->LocationIsDisplaying())
        {
            // Hide
            psStopEffectMessage hide(client->GetClientNum(), location->GetEffectID(this));
            hide.SendMessage();
            if(location->IsRegion())
            {
                for(size_t i = 0; i < location->locs.GetSize(); i++)
                {
                    Location* loc = location->locs[i];
                    psStopEffectMessage msg(client->GetClientNum(), loc->GetEffectID(this));
                    msg.SendMessage();
                }
            }

            csList<iSector*>::Iterator iter = client->GetLocationDisplaying();
            while(iter.HasNext())
            {
                iSector* sector = iter.Next();

                if(client->GetActor()->GetSector() == sector)
                {
                    // Display
                    // TODO: Include sector in psEffectMessage
                    psEffectMessage show(client->GetClientNum(),"admin_location",location->GetPosition(),0,0,
                                         location->GetEffectID(this),location->GetRadius());
                    show.SendMessage();
                    if(location->IsRegion())
                    {
                        for(size_t i = 0; i < location->locs.GetSize(); i++)
                        {
                            Location* loc = location->locs[i];
                            psEffectMessage msg(client->GetClientNum(),"admin_location",
                                                loc->GetPosition(),0,0,loc->GetEffectID(this),loc->GetRadius());
                            msg.SendMessage();
                        }
                    }
                }
            }
        }
    }
}

void AdminManager::HandleLocation(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataLocation* data = static_cast<AdminCmdDataLocation*>(cmddata);

    // Some variables needed by most functions
    csVector3 myPos;
    float myRotY;
    iSector* mySector = 0;
    csString mySectorName;

    client->GetActor()->GetPosition(myPos, myRotY, mySector);
    mySectorName = mySector->QueryObject()->GetName();

    if(data->subCommand == "add")
    {
        LocationType* locationType = locations->FindLocation(data->locationType);
        if(!locationType)
        {
            psserver->SendSystemInfo(me->clientnum, "Failed to find locations type %s.",data->locationType.GetDataSafe());
            return;
        }

        Location* location = locations->CreateLocation(db, locationType, data->locationName, myPos, mySector, data->radius, data->rotAngle, "");

        if(location)
        {
            // Setting the current created location as the selected location
            client->SetSelectedLocationID(location->GetID()); // Use ID to prevent pointer problems.

            psserver->npcmanager->LocationCreated(location);

            UpdateDisplayLocation(location);
            psserver->SendSystemInfo(me->clientnum, "Created new Location %s(%d)",location->GetName(),location->GetID());
        }
    }
    else if(data->subCommand == "adjust")
    {

        float distance=0.0;

        Location* location = locations->FindNearestLocation(EntityManager::GetSingleton().GetWorld(),myPos,mySector,10000,&distance);

        if(!location)
        {
            psserver->SendSystemInfo(me->clientnum, "Failed to find any locations.");
            return;
        }

        if(distance >= 10.0)
        {
            psserver->SendSystemInfo(me->clientnum, "To far from any locations to adjust.");
            return;
        }

        if(location->Adjust(db, myPos, mySector))
        {
            psserver->npcmanager->LocationAdjusted(location);

            UpdateDisplayLocation(location);

            psserver->SendSystemInfo(me->clientnum,
                                     "Adjusted location %s(%d) at range %.2f",
                                     location->GetName(), location->GetID(), distance);
        }
    }
    else if(data->subCommand == "display")
    {
        ShowLocations(client,mySector);

        client->LocationSetIsDisplaying(mySector);
        psserver->SendSystemInfo(me->clientnum, "Displaying all locations in sector %s",mySectorName.GetDataSafe());
    }
    else if(data->subCommand == "hide")
    {
        HideLocations(client);

        client->LocationClearDisplaying();
        psserver->SendSystemInfo(me->clientnum, "All locations hidden");
    }
    else if(data->subCommand == "info")
    {
        float distance=0.0;

        Location* location = locations->FindNearestLocation(EntityManager::GetSingleton().GetWorld(),myPos,mySector,10000,&distance);

        if(!location)
        {
            psserver->SendSystemInfo(me->clientnum, "Failed to find any locations.");
            return;
        }

        if(distance >= 10.0)
        {
            psserver->SendSystemInfo(me->clientnum, "To far from any locations.");
            return;
        }

        // Send info to server. Use a space infront to make it visible that this all
        // belong to the location.
        psserver->SendSystemInfo(me->clientnum, "Found Location %s(%d) at range %.2f\n"
                                 " Radius: %.2f\n"
                                 " Type: %s\n"
                                 " RotAngle: %.2f",
                                 location->GetName(),location->GetID(),distance,
                                 location->GetRadius(),
                                 location->GetTypeName(),
                                 location->GetRotationAngle());
    }
    else if(data->subCommand == "insert")
    {
        Location* selectedLocation = locations->FindLocation(client->GetSelectedLocationID());
        if(!selectedLocation)
        {
            psserver->SendSystemInfo(me->clientnum, "No location selected.");
            return;
        }

        Location* location = selectedLocation->Insert(db, myPos, mySector);
        if(location)
        {
            psserver->npcmanager->LocationInserted(location);

            // Update the selected location to the new created location
            client->SetSelectedLocationID(location->GetID()); // Use ID to prevent pointer problems.

            UpdateDisplayLocation(location);

            psserver->SendSystemInfo(me->clientnum,"Inserted new location %s(%d)",location->GetName(),location->GetID());
        }
    }
    else if(data->subCommand == "list")
    {
        csString types;
        types += "Locations:\n";
        csHash<LocationType*, csString>::GlobalIterator iter(locations->GetIterator());
        while(iter.HasNext())
        {
            LocationType* type = iter.Next();
            types.AppendFmt(" %s\n",type->GetName());
        }

        psserver->SendSystemInfo(me->clientnum,types);
    }
    else if(data->subCommand == "radius")
    {
        float distance=0.0;

        Location* location = locations->FindNearestLocation(EntityManager::GetSingleton().GetWorld(),myPos,mySector,data->searchRadius,&distance);

        if(!location)
        {
            psserver->SendSystemInfo(me->clientnum, "Failed to find any locations within %.2fm.",data->searchRadius);
            return;
        }

        if(location->SetRadius(db, data->radius))
        {
            psserver->npcmanager->LocationRadius(location);

            UpdateDisplayLocation(location);

            psserver->SendSystemInfo(me->clientnum,
                                     "Set new radius %.2f for location %s(%d) at range %.2f",
                                     location->GetRadius(), location->GetName(), location->GetID(), distance);
        }
    }
    else if(data->subCommand == "select")
    {
        float distance=0.0;

        Location* location = locations->FindNearestLocation(EntityManager::GetSingleton().GetWorld(),myPos,mySector,data->searchRadius,&distance);

        if(!location)
        {
            psserver->SendSystemInfo(me->clientnum, "Failed to find any locations within %.2fm.",data->searchRadius);
            return;
        }

        client->SetSelectedLocationID(location->GetID()); // Use ID to prevent pointer problems.

        psserver->SendSystemInfo(me->clientnum,
                                 "Selected location %s(%d) at range %.2f.",
                                 location->GetName(), location->GetID(), distance);
    }
    else if(data->subCommand == "type add")
    {
        LocationType* locationType = locations->FindLocation(data->locationType);
        if(locationType)
        {
            psserver->SendSystemInfo(me->clientnum, "Location type %s already exists.",data->locationType.GetDataSafe());
            return;
        }

        locationType = locations->CreateLocationType(db,data->locationType);

        if(locationType)
        {
            psserver->SendSystemInfo(me->clientnum, "New Location type %s(%d) created.",locationType->GetName(),locationType->GetID());
            psserver->npcmanager->LocationTypeAdd(locationType);
        }
    }
    else if(data->subCommand == "type delete")
    {
        LocationType* locationType = locations->FindLocation(data->locationType);
        if(!locationType)
        {
            psserver->SendSystemInfo(me->clientnum, "Found no location type named %s.",data->locationType.GetDataSafe());
            return;
        }

        if(locations->RemoveLocationType(db,data->locationType))
        {
            psserver->SendSystemInfo(me->clientnum, "Deleted Location type %s.",data->locationType.GetDataSafe());
            psserver->npcmanager->LocationTypeRemove(data->locationType);
        }
    }
    else
    {
        Error2("Unknown type of location subCommand %s",data->subCommand.GetDataSafe());
    }


}


bool AdminManager::GetTargetOfTeleport(Client* client, psAdminCmdMessage &msg, AdminCmdData* cmddata, iSector* &targetSector,  csVector3 &targetPoint, float &yRot, gemObject* subject, InstanceID &instance)
{
    AdminCmdDataTeleport* data = static_cast<AdminCmdDataTeleport*>(cmddata);

    instance = DEFAULT_INSTANCE;

    // when not a destination object, but a map command
    if(data->destObj.IsTargetTypeUnknown() && data->dest == "map")
    {
        // specific sector command
        if(data->destSector.Length())
        {
            // Verify the location first. CS cannot handle positions greater than 100000.
            if(fabs(data->x) > 100000 || fabs(data->y) > 100000 || fabs(data->z) > 100000)
            {
                psserver->SendSystemError(client->GetClientNum(), "Invalid location for teleporting");
                return false;
            }

            if(data->destSector == "here")
            {
                targetSector = client->GetActor()->GetSector();
            }
            else
            {
                targetSector = EntityManager::GetSingleton().GetEngine()->FindSector(data->destSector);
            }
            if(!targetSector)
            {
                psserver->SendSystemError(client->GetClientNum(), "Cannot find sector " + data->destSector);
                return false;
            }
            targetPoint = csVector3(data->x, data->y, data->z);
            if(data->destInstanceValid)
            {
                instance = data->destInstance;
            }
            else
            {
                instance = client->GetActor()->GetInstance();
            }
        }
        else
        {
            return GetStartOfMap(client->GetClientNum(), data->destMap, targetSector, targetPoint);
        }
    }
    // when teleporting to the place where we are standing at
    else if(data->destObj.IsTargetTypeUnknown() && data->dest == "here")
    {
        client->GetActor()->GetPosition(targetPoint, yRot, targetSector);
        if(data->destInstanceValid)
        {
            instance = data->destInstance;
        }
        else
        {
            instance = client->GetActor()->GetInstance();
        }
    }
    // Teleport to a different instance in the same position
    else if(data->destObj.IsTargetTypeUnknown() && data->dest == "there")
    {
        subject->GetPosition(targetPoint, yRot, targetSector);
        if(data->destInstanceValid)
        {
            instance = data->destInstance;
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(), "You must specify what instance.");
            return false;
        }
    }
    // Teleport to last valid location (force unstick/teleport undo)
    else if(data->destObj.IsTargetTypeUnknown() && (data->dest == "last" || data->dest == "restore"))
    {
        if(dynamic_cast<gemActor*>(subject))
        {
            ((gemActor*)subject)->GetPrevTeleportLocation(targetPoint, yRot, targetSector, instance);
        }
        else
        {
            return false; // Actors only
        }
    }
    // Teleport to spawn point
    else if(data->destObj.IsTargetTypeUnknown() && data->dest == "spawn")
    {
        if(dynamic_cast<gemActor*>(subject))
        {
            ((gemActor*)subject)->GetSpawnPos(targetPoint, yRot, targetSector);
        }
        else
        {
            return false; // Actors only
        }
    }
    // Teleport to a generic target (parseable by AdminCmdTargetParser)
    else if(!data->destObj.IsTargetTypeUnknown())
    {
        if(!data->destObj.targetObject)
        {
            psserver->SendSystemError(client->GetClientNum(), "Parser Error specified destination object not found.");
            return false;
        }

        data->destObj.targetObject->GetPosition(targetPoint, yRot, targetSector);
        instance = data->destObj.targetObject->GetInstance();
    }
    return true;
}

bool AdminManager::GetStartOfMap(int clientnum, const csString &map, iSector* &targetSector, csVector3 &targetPoint)
{
    iEngine* engine = EntityManager::GetSingleton().GetEngine();
    csRef<iBgLoader> loader = csQueryRegistry<iBgLoader>(psserver->GetObjectReg());

    csRefArray<StartPosition> positions = loader->GetStartPositions();
    for(size_t i = 0; i < positions.GetSize(); ++i)
    {
        if(positions.Get(i)->zone == map || positions.Get(i)->sector == map)
        {
            targetSector = engine->FindSector(positions.Get(i)->sector);
            targetPoint = positions.Get(i)->position;
            return true;
        }
    }

    return false;
}

void AdminManager::Slide(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSlide* data = static_cast<AdminCmdDataSlide*>(cmddata);

    if(!data->targetObject)
    {
        psserver->SendSystemError(me->clientnum, "Invalid target");
        return;
    }

    if(data->direction.IsEmpty())
    {
        psserver->SendSystemError(me->clientnum, data->GetHelpMessage());
        return;
    }

    float slideAmount = (data->slideAmount == 0)?1:data->slideAmount; // default to 1

    if((slideAmount > 1000 || slideAmount < -1000 || slideAmount != slideAmount) &&
            toupper(data->direction.GetAt(0)) != 'I') // Check bounds and NaN
    {
        psserver->SendSystemError(me->clientnum, "Invalid slide amount");
        return;
    }

    csVector3 pos;
    float yrot;
    iSector* sector = 0;
    InstanceID instance = data->targetObject->GetInstance();

    data->targetObject->GetPosition(pos, yrot, sector);

    if(sector)
    {
        switch(toupper(data->direction.GetAt(0)))
        {
            case 'U':
                pos.y += slideAmount;
                break;
            case 'D':
                pos.y -= slideAmount;
                break;
            case 'L':
                pos.x += slideAmount*cosf(yrot);
                pos.z -= slideAmount*sinf(yrot);
                break;
            case 'R':
                pos.x -= slideAmount*cosf(yrot);
                pos.z += slideAmount*sinf(yrot);
                break;
            case 'F':
                pos.x -= slideAmount*sinf(yrot);
                pos.z -= slideAmount*cosf(yrot);
                break;
            case 'B':
                pos.x += slideAmount*sinf(yrot);
                pos.z += slideAmount*cosf(yrot);
                break;
            case 'T':
                slideAmount = (data->slideAmount == 0)?90:data->slideAmount; // defualt to 90 deg
                yrot += slideAmount*PI/180.0; // Rotation units are degrees
                break;
            case 'I':
                instance += (InstanceID)slideAmount;
                break;
            default:
                psserver->SendSystemError(me->clientnum, "Invalid direction given (Use one of: U D L R F B T I)");
                return;
        }

        // Update the object
        if(!MoveObject(client,data->targetObject,pos,yrot,sector,instance))
            return;

        if(data->targetObject->GetActorPtr() && client->GetActor() != data->targetObject->GetActorPtr())
            psserver->SendSystemInfo(me->clientnum, "Sliding %s...", data->targetObject->GetName());
    }
    else
    {
        psserver->SendSystemError(me->clientnum,
                                  "Invalid sector; cannot slide.  Please contact PlaneShift support.");
    }
}

bool AdminManager::MoveObject(Client* client, gemObject* target, csVector3 &pos, float yrot, iSector* sector, InstanceID instance)
{
    // This is a powerful feature; not everyone is allowed to use all of it
    csString responseExtras,responseExtrasWeak;
    if(client->GetActor() != dynamic_cast<gemActor*>(target) && !psserver->CheckAccess(client, "move others"))
        return false;

    if(dynamic_cast<gemItem*>(target))    // Item?
    {
        gemItem* item = (gemItem*)target;

        // Check to see if this client has the admin level to move this particular item
        bool extras = psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "move unpickupables/spawns", responseExtras);
        bool extrasWeak = psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "move weak unpickupables", responseExtrasWeak);

        if(!item->IsPickupable())
        {
            // Check to see if this client has the admin level to move this particular item
            if(!extras)
            {
                if(!item->IsPickupableStrong())
                {
                    psserver->SendSystemError(client->GetClientNum(), responseExtras);
                    return false;
                }
                // This is a weak unpickupable check to see if this client has the admin level to move this particular item
                else if(!extrasWeak)
                {
                    psserver->SendSystemError(client->GetClientNum(), responseExtrasWeak);
                    return false;
                }
            }
        }

        // Move the item
        item->SetPosition(pos, yrot, sector, instance);

        // Check to see if this client has the admin level to move this spawn point
        if(item->GetItem()->GetScheduledItem() && extras)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "Moving spawn point for %s", item->GetName());

            // Update spawn pos
            item->GetItem()->GetScheduledItem()->UpdatePosition(pos,sector->QueryObject()->GetName());
        }

        item->UpdateProxList(true);
    }
    else if(dynamic_cast<gemActor*>(target))    // Actor? (Player/NPC)
    {
        gemActor* actor = (gemActor*) target;
        actor->Teleport(sector, pos, yrot, instance);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(),"Unknown target type");
        return false;
    }

    return true;
}

void AdminManager::CreateNPC(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);
    gemActor* basis = data->targetActor;

    if(!basis || !basis->GetCharacterData())
    {
        psserver->SendSystemError(me->clientnum, "Invalid target");
        return;
    }

    PID masterNPCID;
    gemNPC* masternpc = basis->GetNPCPtr();
    if(masternpc)
    {
        // Return the master npc's id for this npc. If this npc isn't from a master
        // template this function will return the character id. In this way we
        // will copy all the attributes of the master later.
        masterNPCID = masternpc->GetCharacterData()->GetMasterNPCID();
    }
    if(!masterNPCID.IsValid())
    {
        psserver->SendSystemError(me->clientnum, "%s was not found as a valid master NPC", basis->GetName());
        return;
    }

    if(!psserver->GetConnections()->FindAccount(masternpc->GetSuperclientID()))
    {
        psserver->SendSystemError(me->clientnum, "%s's super client is not online", basis->GetName());
        return;
    }

    csVector3 pos;
    float angle;
    psSectorInfo* sectorInfo = NULL;
    InstanceID instance;
    client->GetActor()->GetCharacterData()->GetLocationInWorld(instance, sectorInfo, pos.x, pos.y, pos.z, angle);

    iSector* sector = NULL;
    if(sectorInfo != NULL)
    {
        sector = EntityManager::GetSingleton().FindSector(sectorInfo->name);
    }
    if(sector == NULL)
    {
        psserver->SendSystemError(me->clientnum, "Invalid sector");
        return;
    }

    // Copy the master NPC into a new player record, with all child tables also
    PID newNPCID = EntityManager::GetSingleton().CopyNPCFromDatabase(masterNPCID, pos.x, pos.y, pos.z, angle,
                   sectorInfo->name, instance, NULL, NULL);
    if(!newNPCID.IsValid())
    {
        psserver->SendSystemError(me->clientnum, "Could not copy the master NPC");
        return;
    }

    psserver->npcmanager->NewNPCNotify(newNPCID, masterNPCID, OWNER_ALL);

    // Make new entity
    EID eid = EntityManager::GetSingleton().CreateNPC(newNPCID, false);

    // Get gemNPC for new entity
    gemNPC* npc = psserver->entitymanager->GetGEM()->FindNPCEntity(newNPCID);
    if(npc == NULL)
    {
        psserver->SendSystemError(client->GetClientNum(), "Could not find GEM and set its location");
        return;
    }

    npc->GetCharacterData()->SetLocationInWorld(instance, sectorInfo, pos.x, pos.y, pos.z, angle);
    npc->SetPosition(pos, angle, sector);

    // Add NPC to all Super Clients
    psserver->npcmanager->AddEntity(npc);

    // Check if this NPC is controlled
    psserver->npcmanager->ControlNPC(npc);

    npc->UpdateProxList(true);

    psserver->SendSystemInfo(me->clientnum, "New %s with %s and %s at (%1.2f,%1.2f,%1.2f) in %s.",
                             npc->GetName(), ShowID(newNPCID), ShowID(eid),
                             pos.x, pos.y, pos.z, sectorInfo->name.GetData());
    psserver->SendSystemOK(me->clientnum, "New NPC created!");
}

void AdminManager::CreateItem(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataItem* data = static_cast<AdminCmdDataItem*>(cmddata);

    if(!data->target.Length())
        // If no arg, load up the spawn item GUI
    {
        SendSpawnTypes(me,msg,data,client);
        return;
    }

    Debug4(LOG_ADMIN,me->clientnum,  "Created item %s %s with quality %d\n",data->target.GetDataSafe(),data->random?"random":"",data->quality)

    psGMSpawnItem spawnMsg(
        data->target,
        data->quantity,
        false,
        false,
        "",
        0,
        true,
        true,
        false,
        true,
        false,
        false,
        true,
        data->random,
        data->quality
    );

    // Copy these items into the correct fields because
    // that is what psGMSpawnItem::psGMSpawnItem(MsgEntry* me)
    // would have done.
    spawnMsg.item = data->target;
    spawnMsg.count = data->quantity;
    spawnMsg.lockable = spawnMsg.locked = spawnMsg.collidable = false;
    spawnMsg.pickupable = true;
    spawnMsg.pickupableWeak = true;
    spawnMsg.Transient = true;
    spawnMsg.lskill = "";
    spawnMsg.lstr = 0;
    spawnMsg.random = data->random;
    spawnMsg.quality = data->quality;

    // Spawn using this message
    SpawnItemInv(me, spawnMsg, client);
}

void AdminManager::RunScript(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataRunScript* data = static_cast<AdminCmdDataRunScript*>(cmddata);

    // Find script
    ProgressionScript* script = psserver->GetProgressionManager()->FindScript(data->scriptName);
    if(!script)
    {
        psserver->SendSystemError(me->clientnum, "Progression script \"%s\" not found.",data->scriptName.GetData());
        return;
    }

    // We don't know what the script expects the actor to be called, so define...basically everything.
    MathEnvironment env;
    //checks if an origin was specified, if so check if it's valid and if it's not assign it to the client issuing the command
    gemObject* originActor = data->origObj.IsTargetType(ADMINCMD_TARGET_UNKNOWN) ?
                             client->GetActor() : data->origObj.targetObject ? data->origObj.targetObject : client->GetActor();
    env.Define("Actor",  originActor);
    env.Define("Caster", originActor);
    env.Define("NPC",    originActor);
    env.Define("Target", data->targetObject ? data->targetObject : client->GetActor());
    script->Run(&env);
}

void AdminManager::ModifyKey(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataKey* data = static_cast<AdminCmdDataKey*>(cmddata);

    // Exchange lock on targeted item
    //  this actually removes the ability to unlock this lock from all the keys
    if(data->subCommand == "changelock")
    {
        ChangeLock(me, msg, data, client);
        return;
    }

    // Change lock to allow it to be unlocked
    if(data->subCommand == "makeunlockable")
    {
        MakeUnlockable(me, msg, data, client);
        return;
    }

    // Change lock to allow it to be unlocked
    if(data->subCommand == "securitylockable")
    {
        MakeSecurity(me, msg, data, client);
        return;
    }

    // Make a key out of item in right hand
    if(data->subCommand == "make")
    {
        MakeKey(me, msg, data, client, false);
        return;
    }

    // Make a master key out of item in right hand
    if(data->subCommand == "makemaster")
    {
        MakeKey(me, msg, data, client, true);
        return;
    }

    // Find key item from hands
    psItem* key = client->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if(!key || !key->GetIsKey())
    {
        key = client->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
        if(!key || !key->GetIsKey())
        {
            psserver->SendSystemError(me->clientnum,"You need to be holding the key you want to work on");
            return;
        }
    }

    // Make or unmake key a skeleton key that will open any lock
    if(data->subCommand == "skel")
    {
        bool b = key->GetIsSkeleton();
        key->MakeSkeleton(!b);
        if(b)
            psserver->SendSystemInfo(me->clientnum, "Your %s is no longer a skeleton key", key->GetName());
        else
            psserver->SendSystemInfo(me->clientnum, "Your %s is now a skeleton key", key->GetName());
        key->Save(false);
        return;
    }

    // Copy key item
    if(data->subCommand == "copy")
    {
        CopyKey(me, msg, data, client, key);
    }

    // Clear all locks that key can open
    if(data->subCommand == "clearlocks")
    {
        key->ClearOpenableLocks();
        key->Save(false);
        psserver->SendSystemInfo(me->clientnum, "Your %s can no longer unlock anything", key->GetName());
        return;
    }

    // Add or remove keys ability to lock targeted lock
    if(data->subCommand == "addlock" || data->subCommand == "removelock")
    {
        AddRemoveLock(me, msg, data, client, key);
        return;
    }
}

void AdminManager::CopyKey(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client, psItem* key)
{
    // check if item is master key
    if(!key->GetIsMasterKey())
    {
        psserver->SendSystemInfo(me->clientnum, "Only a master key can be copied.");
        return;
    }

    // get stats
    psItemStats* keyStats = key->GetBaseStats();
    if(!keyStats)
    {
        Error2("Could not get base stats for item (%s).", key->GetName());
        psserver->SendSystemError(me->clientnum,"Could not get base stats for key!");
        return;
    }

    // make a perminent new item
    psItem* newKey = keyStats->InstantiateBasicItem();
    if(!newKey)
    {
        Error2("Could not create item (%s).", keyStats->GetName());
        psserver->SendSystemError(me->clientnum,"Could not create key!");
        return;
    }

    // copy item characteristics
    newKey->SetItemQuality(key->GetItemQuality());
    newKey->SetStackCount(key->GetStackCount());

    // copy over key characteristics
    newKey->SetIsKey(true);
    newKey->SetLockpickSkill(key->GetLockpickSkill());
    newKey->CopyOpenableLock(key);

    // get client info
    if(!client)
    {
        Error1("Bad client pointer for key copy.");
        psserver->SendSystemError(me->clientnum,"Bad client pointer for key copy!");
        return;
    }
    psCharacter* charData = client->GetCharacterData();
    if(!charData)
    {
        Error2("Could not get character data for (%s).", client->GetName());
        psserver->SendSystemError(me->clientnum,"Could not get character data!");
        return;
    }

    // put into inventory
    newKey->SetLoaded();
    if(charData->Inventory().Add(newKey))
    {
        psserver->SendSystemInfo(me->clientnum, "A copy of %s has been spawned into your inventory", key->GetName());
    }
    else
    {
        psserver->SendSystemInfo(me->clientnum, "Couldn't spawn %s to your inventory, maybe it's full?", key->GetName());
        psserver->GetCacheManager()->RemoveInstance(newKey);
    }
    psserver->GetCharManager()->UpdateItemViews(me->clientnum);
}

void AdminManager::MakeUnlockable(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client)
{
    // check if player has something targeted
    gemObject* target = client->GetTargetObject();
    if(!target)
    {
        psserver->SendSystemError(me->clientnum,"You need to target the lock you want to make unlockable.");
    }

    // Check if target is action item
    gemActionLocation* gemAction = dynamic_cast<gemActionLocation*>(target);
    if(gemAction)
    {
        psActionLocation* action = gemAction->GetAction();

        // check if the actionlocation is linked to real item
        InstanceID InstanceID = action->GetInstanceID();
        if(InstanceID == INSTANCE_ALL)
        {
            InstanceID = action->GetGemObject()->GetEID().Unbox(); // FIXME: Understand and comment on conversion magic
        }
        target = psserver->entitymanager->GetGEM()->FindItemEntity(InstanceID);
        if(!target)
        {
            psserver->SendSystemError(me->clientnum,"There is no item associated with this action location.");
            return;
        }
    }

    // Get targeted item
    psItem* item = target->GetItem();
    if(!item)
    {
        Error1("Found gemItem but no psItem was attached!\n");
        psserver->SendSystemError(me->clientnum,"Found gemItem but no psItem was attached!");
        return;
    }

    // Flip the lockability
    if(item->GetIsLockable())
    {
        item->SetIsLockable(false);
        psserver->SendSystemInfo(me->clientnum, "The lock was set to be non unlockable.", item->GetName());
    }
    else
    {
        item->SetIsLockable(true);
        psserver->SendSystemInfo(me->clientnum, "The lock was set to be unlockable.", item->GetName());
    }
}

void AdminManager::MakeSecurity(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client)
{
    // check if player has something targeted
    gemObject* target = client->GetTargetObject();
    if(!target)
    {
        psserver->SendSystemError(me->clientnum,"You need to target the lock you want to make a security lock.");
    }

    // Check if target is action item
    gemActionLocation* gemAction = dynamic_cast<gemActionLocation*>(target);
    if(gemAction)
    {
        psActionLocation* action = gemAction->GetAction();

        // check if the actionlocation is linked to real item
        InstanceID InstanceID = action->GetInstanceID();
        if(InstanceID == INSTANCE_ALL)
        {
            InstanceID = action->GetGemObject()->GetEID().Unbox(); // FIXME: Understand and comment on conversion magic
        }
        target = psserver->entitymanager->GetGEM()->FindItemEntity(InstanceID);
        if(!target)
        {
            psserver->SendSystemError(me->clientnum,"There is no item associated with this action location.");
            return;
        }
    }

    // Get targeted item
    psItem* item = target->GetItem();
    if(!item)
    {
        Error1("Found gemItem but no psItem was attached!\n");
        psserver->SendSystemError(me->clientnum,"Found gemItem but no psItem was attached!");
        return;
    }

    // Flip the security lockability
    if(item->GetIsSecurityLocked())
    {
        item->SetIsSecurityLocked(false);
        psserver->SendSystemInfo(me->clientnum, "The lock was set to be non security lock.", item->GetName());
    }
    else
    {
        item->SetIsSecurityLocked(true);
        psserver->SendSystemInfo(me->clientnum, "The lock was set to be security lock.", item->GetName());
    }
}

void AdminManager::MakeKey(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client, bool masterkey)
{
    psItem* key = client->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if(!key)
    {
        psserver->SendSystemError(me->clientnum,"You need to hold the item you want to make into a key in your right hand.");
        return;
    }
    if(masterkey)
    {
        if(key->GetIsMasterKey())
        {
            psserver->SendSystemError(me->clientnum,"Your %s is already a master key.", key->GetName());
            return;
        }
        key->SetIsKey(true);
        key->SetIsMasterKey(true);
        psserver->SendSystemOK(me->clientnum,"Your %s is now a master key.", key->GetName());
    }
    else
    {
        if(key->GetIsKey())
        {
            psserver->SendSystemError(me->clientnum,"Your %s is already a key.", key->GetName());
            return;
        }
        key->SetIsKey(true);
        psserver->SendSystemOK(me->clientnum,"Your %s is now a key.", key->GetName());
    }

    key->Save(false);
}

void AdminManager::AddRemoveLock(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client, psItem* key)
{
    AdminCmdDataKey* data = static_cast<AdminCmdDataKey*>(cmddata);
    // check if player has something targeted
    gemObject* target = client->GetTargetObject();
    if(!target)
    {
        if(data->subCommand == "addlock")
            psserver->SendSystemError(me->clientnum,"You need to target the item you want to encode the key to unlock");
        else
            psserver->SendSystemError(me->clientnum,"You need to target the item you want to stop the key from unlocking");
        return;
    }

    // Check if target is action item
    gemActionLocation* gemAction = dynamic_cast<gemActionLocation*>(target);
    if(gemAction)
    {
        psActionLocation* action = gemAction->GetAction();

        // check if the actionlocation is linked to real item
        InstanceID InstanceID = action->GetInstanceID();
        if(InstanceID == INSTANCE_ALL)
        {
            InstanceID = action->GetGemObject()->GetEID().Unbox(); // FIXME: Understand and comment on conversion magic
        }
        target = psserver->entitymanager->GetGEM()->FindItemEntity(InstanceID);
        if(!target)
        {
            psserver->SendSystemError(me->clientnum,"There is no item associated with this action location.");
            return;
        }
    }

    // Get targeted item
    psItem* item = target->GetItem();
    if(!item)
    {
        Error1("Found gemItem but no psItem was attached!\n");
        psserver->SendSystemError(me->clientnum,"Found gemItem but no psItem was attached!");
        return;
    }

    if(!item->GetIsLockable())
    {
        psserver->SendSystemError(me->clientnum,"This object isn't lockable");
        return;
    }

    if(data->subCommand == "addlock")
    {
        key->AddOpenableLock(item->GetUID());
        key->Save(false);
        psserver->SendSystemInfo(me->clientnum, "You encoded %s to unlock %s", key->GetName(), item->GetName());
    }
    else
    {
        key->RemoveOpenableLock(item->GetUID());
        key->Save(false);
        psserver->SendSystemInfo(me->clientnum, "Your %s can no longer unlock %s", key->GetName(), item->GetName());
    }
}

void AdminManager::ChangeLock(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client)
{
    // check if player has something targeted
    gemObject* target = client->GetTargetObject();
    if(!target)
    {
        psserver->SendSystemError(me->clientnum,"You need to target the item for which you want to change the lock");
        return;
    }

    // check for action location
    gemActionLocation* gemAction = dynamic_cast<gemActionLocation*>(target);
    if(gemAction)
    {
        psActionLocation* action = gemAction->GetAction();

        // check if the actionlocation is linked to real item
        InstanceID InstanceID = action->GetInstanceID();
        if(InstanceID == INSTANCE_ALL)
        {
            InstanceID = action->GetGemObject()->GetEID().Unbox(); // FIXME: Understand and comment on conversion magic

        }
        target = psserver->entitymanager->GetGEM()->FindItemEntity(InstanceID);
        if(!target)
        {
            psserver->SendSystemError(me->clientnum,"There is no item associated with this action location.");
            return;
        }
    }

    // get the old item
    psItem* oldLock = target->GetItem();
    if(!oldLock)
    {
        Error1("Found gemItem but no psItem was attached!\n");
        psserver->SendSystemError(me->clientnum,"Found gemItem but no psItem was attached!");
        return;
    }

    // get instance ID
    psString buff;
    uint32 lockID = oldLock->GetUID();
    buff.Format("%u", lockID);

    // Get psItem array of keys to check
    Result items(db->Select("SELECT * from item_instances where flags like '%KEY%'"));
    if(items.IsValid())
    {
        for(int i=0; i < (int)items.Count(); i++)
        {
            // load openableLocks except for specific lock
            psString word;
            psString lstr;
            uint32 keyID = items[i].GetUInt32("id");
            psString olstr(items[i]["openable_locks"]);
            olstr.GetWordNumber(1, word);
            for(int n = 2; word.Length(); olstr.GetWordNumber(n++, word))
            {
                // check for matching lock
                if(word != buff)
                {
                    // add space to sparate since GetWordNumber is used to decode
                    if(!lstr.IsEmpty())
                        lstr.Append(" ");
                    lstr.Append(word);

                    // write back to database
                    int result = db->CommandPump("UPDATE item_instances SET openable_locks='%s' WHERE id=%d",
                                                 lstr.GetData(), keyID);
                    if(result < 0)
                    {
                        Error4("Couldn't update item instance lockchange with lockID=%d keyID=%d openable_locks <%s>.",lockID, keyID, lstr.GetData());
                        return;
                    }

                    // reload inventory if key belongs to this character
                    uint32 ownerID = items[i].GetUInt32("char_id_owner");
                    psCharacter* character = client->GetCharacterData();
                    if(character->GetPID() == ownerID)
                    {
                        character->Inventory().Load();
                    }
                    break;
                }
            }
        }
    }
    psserver->SendSystemInfo(me->clientnum, "You changed the lock on %s", oldLock->GetName());
}

void AdminManager::KillNPC(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataKillNPC* data = static_cast<AdminCmdDataKillNPC*>(cmddata);

    if(data->targetObject && data->targetObject->GetClientID())
    {
        psserver->SendSystemError(me->clientnum, "You can not kill a Player with this command!");
        return;
    }


    gemNPC* target = dynamic_cast<gemNPC*>(data->targetObject);
    if(target && target->GetClientID() == 0)
    {
        if(!data->reload)
        {
            if(data->damage > 0.0)
            {
                target->DoDamage(client->GetActor(),data->damage);
            }
            else
            {
                target->Kill(client->GetActor());
            }

        }
        else
        {
            // Get the pid so we know what to restore.
            PID npcid = target->GetCharacterData()->GetPID();

            // Remove the NPC
            EntityManager::GetSingleton().RemoveActor(data->targetObject);

            // Create the new NPC
            psCharacter* npcdata = psServer::CharacterLoader.LoadCharacterData(npcid,true);
            EntityManager::GetSingleton().CreateNPC(npcdata);

            psserver->SendSystemResult(me->clientnum, "NPC (%s) has been reloaded.", npcid.Show().GetData());
        }
        return;
    }
    else if(!target)
    {
        //as the npc was not found but it's id was found from db or direct data from the user
        PID npcid = data->targetID;
        if(npcid.IsValid())  //check if the pid is really one
        {
            //try to load the new npc
            psCharacter* npcdata = psServer::CharacterLoader.LoadCharacterData(npcid,true);
            //if the load was successful we can try completing the load process
            if(npcdata)
            {
                EntityManager::GetSingleton().CreateNPC(npcdata);
                psserver->SendSystemResult(me->clientnum, "NPC (%s) has been loaded.", npcid.Show().GetData());
            }
        }
    }
    psserver->SendSystemError(me->clientnum, "No NPC found to kill.");
}

void AdminManager::Percept(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataPercept* data = static_cast<AdminCmdDataPercept*>(cmddata);

    gemNPC* target = dynamic_cast<gemNPC*>(data->targetObject);
    if(target && target->GetClientID() == 0)
    {

        psserver->SendSystemResult(me->clientnum, "NPC %s(%s) has been percepted '%s' type '%s'.", target->GetName(),ShowID(target->GetEID()),
                                   data->perception.GetDataSafe(),data->type.GetDataSafe());

        psserver->GetNPCManager()->QueuePerceptPerception(target,data->perception,data->type);
        return;
    }

    psserver->SendSystemError(me->clientnum, "No NPC found to percept.");
}

void AdminManager::ChangeNPCType(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataChangeNPCType* data = static_cast<AdminCmdDataChangeNPCType*>(cmddata);

    gemNPC* target = dynamic_cast<gemNPC*>(data->targetObject);
    if(target && target->GetClientID() == 0)
    {
        // Get the pid so we know what to restore.
        PID npcid = target->GetCharacterData()->GetPID();
        //send the change command.
        psserver->GetNPCManager()->ChangeNPCBrain(target, client, data->npcType.GetDataSafe());
        psserver->SendSystemResult(me->clientnum, "NPC (%s) has been commanded to change type.", npcid.Show().GetData());
        return;
    }
    psserver->SendSystemError(me->clientnum, "No NPC found to change type.");
}

void AdminManager::DebugNPC(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataDebugNPC* data = static_cast<AdminCmdDataDebugNPC*>(cmddata);

    gemNPC* target = dynamic_cast<gemNPC*>(data->targetObject);
    if(target && target->GetClientID() == 0)
    {
        //send the change command.
        psserver->GetNPCManager()->DebugNPC(target, client, data->debugLevel);
        return;
    }
    psserver->SendSystemError(me->clientnum, "No NPC found to set debug level for.");
}

void AdminManager::DebugTribe(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataDebugTribe* data = static_cast<AdminCmdDataDebugTribe*>(cmddata);

    gemNPC* target = dynamic_cast<gemNPC*>(data->targetObject);
    if(target && target->GetClientID() == 0)
    {
        //send the change command.
        psserver->GetNPCManager()->DebugTribe(target, client, data->debugLevel);
        return;
    }
    psserver->SendSystemError(me->clientnum, "No NPC found to set debug level for.");
}


void AdminManager::Admin(int clientnum, Client* client, int requestedLevel)
{
    // Set client security level in case security level have
    // changed in database.
    csString commandList;
    int type = client->GetSecurityLevel();

    // for now consider all levels > 30 as level 30.
    if(type > GM_DEVELOPER)
        type = GM_DEVELOPER;

    if(type > 0 && requestedLevel >= 0)
        type = requestedLevel;

    psserver->GetCacheManager()->GetCommandManager()->BuildXML(type, commandList, requestedLevel == -1);
    //NOTE: with only a check for requestedLevel == -1 players can actually make this function add the nonsubscrition flag
    //      but as it brings no real benefits to the player there is no need to check for it. They will just get the commands
    //      of their level and they won't be subscripted in their client

    psAdminCmdMessage admin(commandList.GetDataSafe(), clientnum);
    admin.SendMessage();
}

void AdminManager::WarnMessage(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataTargetReason* data = static_cast<AdminCmdDataTargetReason*>(cmddata);
    if(!data->targetClient)
    {
        psserver->SendSystemError(me->clientnum, "Invalid target to warn");
        return;
    }

    if(data->reason.Length() == 0)
    {
        psserver->SendSystemError(me->clientnum, "Please enter a warn message");
        return;
    }

    // This message will be shown in adminColor (red) in all chat tabs for this player
    psSystemMessage newmsg(data->targetClient->GetClientNum(), MSG_INFO_SERVER, "GM warning from %s: " + data->reason, client->GetName());
    newmsg.SendMessage();

    // This message will be in big red letters on their screen
    psserver->SendSystemError(data->targetClient->GetClientNum(), data->reason);

    //escape the warning so it's not possible to do nasty things
    csString escapedReason;
    db->Escape(escapedReason, data->reason.GetDataSafe());

    db->CommandPump("INSERT INTO warnings VALUES(%u, '%s', NOW(), '%s')", data->targetClient->GetAccountID().Unbox(), client->GetName(), escapedReason.GetDataSafe());

    psserver->SendSystemInfo(client->GetClientNum(), "You warned '%s': " + data->reason, data->targetClient->GetName());
}


void AdminManager::KickPlayer(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataTargetReason* data = static_cast<AdminCmdDataTargetReason*>(cmddata);

    if(!data->targetClient)
    {
        psserver->SendSystemError(me->clientnum, "Invalid target to kick");
        return;
    }

    if(data->reason.Length() < 5)
    {
        psserver->SendSystemError(me->clientnum, "You must specify a reason to kick");
        return;
    }

    // Remove from server and show the reason message
    psserver->RemovePlayer(data->targetClient->GetClientNum(),"You were kicked from the server by a GM. Reason: " + data->reason);

    psserver->SendSystemInfo(me->clientnum,"You kicked '%s' off the server.",(const char*)data->target);
}

void AdminManager::Death(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataDeath* data = static_cast<AdminCmdDataDeath*>(cmddata);

    if(!data->targetActor)
    {
        psserver->SendSystemError(me->clientnum,"You can't kill things that are not alive!");
        return;
    }

    data->targetActor->Kill(NULL);  // Have a nice day ;)

    if(data->targetActor->GetClientID() != 0)
    {
        if(data->requestor.Length() && (client->GetActor() == data->targetActor || psserver->CheckAccess(client, "requested death")))
        {
            csString message = "You were struck down by ";
            if(data->requestor == "god")  //get the god from the sector
            {
                if(data->targetActor->GetCharacterData() && data->targetActor->GetCharacterData()->GetLocation().loc_sector)
                    message += data->targetActor->GetCharacterData()->GetLocation().loc_sector->god_name.GetData();
                else
                    message += "the gods";
            }
            else //use the assigned requestor
                message += data->requestor;

            psserver->SendSystemError(data->targetActor->GetClientID(), message);
        }
        else
        {
            psserver->SendSystemError(data->targetActor->GetClientID(), "You were killed by a GM");
        }
    }
}


void AdminManager::Impersonate(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataImpersonate* data = static_cast<AdminCmdDataImpersonate*>(cmddata);
    if(data->text.IsEmpty())
    {
        psserver->SendSystemError(me->clientnum, "Missing text or anim name");
        return;
    }


    //send an animation
    if(data->commandMod == "anim")
    {
        //this is done for error checking (items can't be animated)
        gemActor* target = client->GetTargetObject()->GetActorPtr();
        if(!target)
        {
            psserver->SendSystemError(me->clientnum, "You can execute animations only on actors");
            return;
        }

        //send an action override message which runs the animation
        psOverrideActionMessage msg(0, target->GetEID(), data->text, data->duration);
        msg.Multicast(target->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
        return;
    }

    csString sendText; // We need specialised say/shout as it is a special GM chat message

    if(data->target == "text")
        sendText = data->text;
    else
        sendText.Format("%s %ss: %s", data->target.GetData(), data->commandMod.GetData(), data->text.GetData());

    psChatMessage newMsg(client->GetClientNum(), 0, data->target, 0, sendText, CHAT_GM, false);

    gemObject* source = (gemObject*)client->GetActor();

    // Invisible; multicastclients list is empty
    if(!source->GetVisibility() && data->commandMod != "worldshout")
    {
        // Try to use target as source
        source = client->GetTargetObject();

        if(source == NULL || source->GetClientID() == client->GetClientNum())
        {
            psserver->SendSystemError(me->clientnum, "Invisible; select a target to use as source");
            return;
        }
    }

    if(data->commandMod == "say")
        newMsg.Multicast(source->GetMulticastClients(), 0, CHAT_SAY_RANGE);
    else if(data->commandMod == "shout")
        newMsg.Multicast(source->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    else if(data->commandMod == "worldshout")
        psserver->GetEventManager()->Broadcast(newMsg.msg, NetBase::BC_EVERYONE);
}

void AdminManager::MutePlayer(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if(!data->targetClient)
    {
        psserver->SendSystemError(me->clientnum, "Invalid target to mute");
        return;
    }

    psserver->MutePlayer(data->targetClient->GetClientNum(),"You were muted by a GM, until log off.");

    // Finally, notify the GM that the client was successfully muted
    psserver->SendSystemInfo(me->clientnum, "You muted '%s' until he/she/it logs back in.",(const char*)data->target);
}


void AdminManager::UnmutePlayer(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if(!data->targetClient)
    {
        psserver->SendSystemError(me->clientnum, "Invalid target to unmute");
        return;
    }

    psserver->UnmutePlayer(data->targetClient->GetClientNum(),"You were unmuted by a GM.");

    // Finally, notify the GM that the client was successfully unmuted
    psserver->SendSystemInfo(me->clientnum, "You unmuted '%s'.",(const char*)data->target);
}


void AdminManager::HandleAddPetition(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataPetition* data = static_cast<AdminCmdDataPetition*>(cmddata);

    // Try and add the petition to the database:
    if(!AddPetition(client->GetPID(), (const char*)data->petition))
    {
        psserver->SendSystemError(me->clientnum,"SQL Error: %s", db->GetLastError());
        return;
    }

    // Tell client the petition was added:
    psserver->SendSystemInfo(me->clientnum, "Your petition was successfully submitted!");

    BroadcastDirtyPetitions(me->clientnum, true);
}

void AdminManager::BroadcastDirtyPetitions(int clientNum, bool includeSelf)
{
    psPetitionMessage dirty(clientNum, NULL, "", true, PETITION_DIRTY, true);
    if(dirty.valid)
    {
        if(includeSelf)
            psserver->GetEventManager()->Broadcast(dirty.msg, NetBase::BC_EVERYONE);
        else
            psserver->GetEventManager()->Broadcast(dirty.msg, NetBase::BC_EVERYONEBUTSELF);
    }
}

bool AdminManager::GetPetitionsArray(csArray<psPetitionInfo> &petitions, Client* client, bool IsGMrequest)
{
    // Try and grab the result set from the database
    // NOTE: As there are differences between the normal use and the gm use we manage them here
    //       the result set will be different depending on this
    iResultSet* rs = GetPetitions(IsGMrequest ? PETITION_GM : client->GetPID(),
                                  IsGMrequest ? client->GetPID() : PETITION_GM);

    if(rs)
    {
        psPetitionInfo info;
        for(unsigned int i=0; i<rs->Count(); i++)
        {
            // Set info
            info.id = atoi((*rs)[i][0]);
            info.petition = (*rs)[i][1];
            info.status = (*rs)[i][2];
            info.created = csString((*rs)[i][3]).Slice(0, 16);
            info.assignedgm = (*rs)[i][4];
            if(!IsGMrequest)  //this is special handling for player petition requests
            {
                if(info.assignedgm.Length() == 0)
                {
                    info.assignedgm = "No GM Assigned";
                }
                info.resolution = (*rs)[i][5];
                if(info.resolution.Length() == 0)
                {
                    info.resolution = "No Resolution";
                }
            }
            else //this is special handling for gm petitions requests
            {
                info.player = (*rs)[i][5];
                info.escalation = atoi((*rs)[i][6]);
                info.online = (clients->Find(info.player) ? true : false);

            }

            // Append to the message:
            petitions.Push(info);
        }
        rs->Release();
        return true;
    }

    return false;
}

void AdminManager::ListPetitions(MsgEntry* me, psPetitionRequestMessage &msg,Client* client)
{
    csArray<psPetitionInfo> petitions;
    // Try and grab the result set from the database
    if(GetPetitionsArray(petitions, client))
    {
        psPetitionMessage message(me->clientnum, &petitions, "List retrieved successfully.", true, PETITION_LIST);
        message.SendMessage();
    }
    else
    {
        // Return no succeed message to client
        csString error;
        error.Format("SQL Error: %s", db->GetLastError());
        psPetitionMessage message(me->clientnum, NULL, error, false, PETITION_LIST);
        message.SendMessage();
    }
}

void AdminManager::CancelPetition(MsgEntry* me, psPetitionRequestMessage &msg,Client* client)
{
    // Tell the database to change the status of this petition:
    if(!CancelPetition(client->GetPID(), msg.id))
    {
        psPetitionMessage error(me->clientnum, NULL, db->GetLastError(), false, PETITION_CANCEL);
        error.SendMessage();
        return;
    }

    csArray<psPetitionInfo> petitions;
    // Try and grab the result set from the database
    if(GetPetitionsArray(petitions, client))
    {
        psPetitionMessage message(me->clientnum, &petitions, "Cancel was successful.", true, PETITION_CANCEL);
        message.SendMessage();
    }
    else
    {
        // Tell client deletion was successful:
        psPetitionMessage message(me->clientnum, NULL, "Cancel was successful.", true, PETITION_CANCEL);
        message.SendMessage();
    }
    BroadcastDirtyPetitions(me->clientnum);
}

void AdminManager::ChangePetition(MsgEntry* me, psPetitionRequestMessage &msg, Client* client)
{
    // Tell the database to change the status of this petition:
    if(!ChangePetition(client->GetPID(), msg.id, msg.desc))
    {
        psPetitionMessage error(me->clientnum, NULL, db->GetLastError(), false, PETITION_CHANGE);
        error.SendMessage();
        return;
    }

    // refresh client list
    ListPetitions(me, msg, client);
}

void AdminManager::GMListPetitions(MsgEntry* me, psPetitionRequestMessage &msg,Client* client)
{
    // Check to see if this client has GM level access
    if(client->GetSecurityLevel() < GM_LEVEL_0)
    {
        psserver->SendSystemError(me->clientnum, "Access denied. Only GMs can manage petitions.");
        return;
    }

    // Try and grab the result set from the database

    // Show the player all petitions.
    csArray<psPetitionInfo> petitions;
    if(GetPetitionsArray(petitions, client, true))
    {
        psPetitionMessage message(me->clientnum, &petitions, "List retrieved successfully.", true, PETITION_LIST, true);
        message.SendMessage();
    }
    else
    {
        // Return no succeed message to GM
        csString error;
        error.Format("SQL Error: %s", db->GetLastError());
        psPetitionMessage message(me->clientnum, NULL, error, false, PETITION_LIST, true);
        message.SendMessage();
    }
}

void AdminManager::GMHandlePetition(MsgEntry* me, psPetitionRequestMessage &msg,Client* client)
{
    // Check to see if this client has GM level access
    if(client->GetSecurityLevel() < GM_LEVEL_0)
    {
        psserver->SendSystemError(me->clientnum, "Access denied. Only GMs can manage petitions.");
        return;
    }

    // Check what operation we are executing based on the request:
    int type = -1;
    bool result = false;
    if(msg.request == "cancel")
    {
        // Cancellation:
        type = PETITION_CANCEL;
        result = CancelPetition(client->GetPID(), msg.id, true);
    }
    else if(msg.request == "close")
    {
        // Closing petition:
        type = PETITION_CLOSE;
        result = ClosePetition(client->GetPID(), msg.id, msg.desc);
    }
    else if(msg.request == "assign")
    {
        // Assigning petition:
        type = PETITION_ASSIGN;
        result = AssignPetition(client->GetPID(), msg.id);
    }
    else if(msg.request == "deassign")
    {
        // Deassigning petition:
        type = PETITION_DEASSIGN;
        result = DeassignPetition(client->GetPID(), client->GetSecurityLevel(), msg.id);
    }
    else if(msg.request == "escalate")
    {
        // Escalate petition:
        type = PETITION_ESCALATE;
        result = EscalatePetition(client->GetPID(), client->GetSecurityLevel(), msg.id);
    }

    else if(msg.request == "descalate")
    {
        // Descalate petition:
        type = PETITION_DESCALATE;
        result = DescalatePetition(client->GetPID(), client->GetSecurityLevel(), msg.id);
    }

    // Check result of operation
    if(!result)
    {
        psPetitionMessage error(me->clientnum, NULL, lasterror, false, type, true);
        error.SendMessage();
        return;
    }

    // Try and grab the result set from the database
    csArray<psPetitionInfo> petitions;
    if(GetPetitionsArray(petitions, client, true))
    {
        // Tell GM operation was successful
        psPetitionMessage message(me->clientnum, &petitions, "Successful", true, type, true);
        message.SendMessage();
    }
    else
    {
        // Tell GM operation was successful even though we don't have a list of petitions
        psPetitionMessage message(me->clientnum, NULL, "Successful", true, type, true);
        message.SendMessage();
    }
    BroadcastDirtyPetitions(me->clientnum);
}

void AdminManager::SendGMPlayerList(MsgEntry* me, psGMGuiMessage &msg,Client* client)
{
    if(client->GetSecurityLevel() < GM_LEVEL_1  &&
            client->GetSecurityLevel() > GM_LEVEL_9  && !client->IsSuperClient())
    {
        psserver->SendSystemError(me->clientnum,"You don't have access to GM functions!");
        CPrintf(CON_ERROR, "Client %d tried to get GM player list, but hasn't got GM access!\n");
        return;
    }

    csArray<psGMGuiMessage::PlayerInfo> playerList;

    // build the list of players
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* curr = i.Next();
        if(curr->IsSuperClient() || !curr->GetActor()) continue;

        psGMGuiMessage::PlayerInfo playerInfo;

        playerInfo.name = curr->GetName();
        playerInfo.lastName = curr->GetCharacterData()->GetCharLastName();
        playerInfo.gender = curr->GetCharacterData()->GetRaceInfo()->gender;

        psGuildInfo* guild = curr->GetCharacterData()->GetGuild();
        if(guild)
            playerInfo.guild = guild->GetName();
        else
            playerInfo.guild.Clear();

        //Get sector name
        csVector3 vpos;
        float yrot;
        iSector* sector;

        curr->GetActor()->GetPosition(vpos,yrot,sector);

        playerInfo.sector = sector->QueryObject()->GetName();

        playerList.Push(playerInfo);
    }

    // send the list of players
    psGMGuiMessage message(me->clientnum, &playerList, psGMGuiMessage::TYPE_PLAYERLIST);
    message.SendMessage();
}

bool AdminManager::EscalatePetition(PID gmID, int gmLevel, int petitionID)
{
    int result = db->CommandPump("UPDATE petitions SET status='Open',assigned_gm=-1,"
                                 "escalation_level=(escalation_level+1) "
                                 "WHERE id=%d AND escalation_level<=%d AND escalation_level<%d "
                                 "AND (assigned_gm=%d OR status='Open')", petitionID, gmLevel, GM_DEVELOPER-20, gmID.Unbox());
    // If this failed if means that there is a serious error
    if(result <= 0)
    {
        lasterror.Format("Couldn't escalate petition #%d.", petitionID);
        return false;
    }
    return true;
}

bool AdminManager::DescalatePetition(PID gmID, int gmLevel, int petitionID)
{

    int result = db->CommandPump("UPDATE petitions SET status='Open',assigned_gm=-1,"
                                 "escalation_level=(escalation_level-1)"
                                 "WHERE id=%d AND escalation_level<=%d AND (assigned_gm=%u OR status='Open' AND escalation_level != 0)", petitionID, gmLevel, gmID.Unbox());
    // If this failed if means that there is a serious error
    if(result <= 0)
    {
        lasterror.Format("Couldn't descalate petition #%d.", petitionID);
        return false;
    }
    return true;
}

bool AdminManager::AddPetition(PID playerID, const char* petition)
{
    /* The columns in the table NOT included in this command
     * have default values and thus we do not need to put them in
     * the INSERT statement
     */
    csString escape;
    db->Escape(escape, petition);
    int result = db->Command("INSERT INTO petitions "
                             "(player,petition,created_date,status,resolution) "
                             "VALUES (%u,\"%s\",Now(),\"Open\",\"Not Resolved\")", playerID.Unbox(), escape.GetData());

    return (result > 0);
}

iResultSet* AdminManager::GetPetitions(PID playerID, PID gmID)
{
    iResultSet* rs;

    // Check player ID, if ID is PETITION_GM (0xFFFFFFFF), get a complete list for the GM:
    if(playerID == PETITION_GM)
    {
        rs = db->Select("SELECT pet.id,pet.petition,pet.status,pet.created_date,gm.name as gmname,pl.name,pet.escalation_level FROM petitions pet "
                        "LEFT JOIN characters gm ON pet.assigned_gm=gm.id, characters pl WHERE pet.player!=%d AND (pet.status='Open' OR pet.status='In Progress') "
                        "AND pet.player=pl.id "
                        "ORDER BY pet.status ASC,pet.escalation_level DESC,pet.created_date ASC", gmID.Unbox());
    }
    else
    {
        rs = db->Select("SELECT pet.id,pet.petition,pet.status,pet.created_date,pl.name,pet.resolution "
                        "FROM petitions pet LEFT JOIN characters pl "
                        "ON pet.assigned_gm=pl.id "
                        "WHERE pet.player=%d "
                        "AND pet.status!='Cancelled' "
                        "ORDER BY pet.status ASC,pet.escalation_level DESC", playerID.Unbox());
    }

    if(!rs)
    {
        lasterror = GetLastSQLError();
    }

    return rs;
}

bool AdminManager::CancelPetition(PID playerID, int petitionID, bool isGMrequest)
{
    // If isGMrequest is true, just cancel the petition (a GM is requesting the change)
    if(isGMrequest)
    {
        int result = db->CommandPump("UPDATE petitions SET status='Cancelled' WHERE id=%d AND assigned_gm=%u", petitionID,playerID.Unbox());
        return (result > 0);
    }

    // Attempt to select this petition; two things can go wrong: it doesn't exist or the player didn't create it
    int result = db->SelectSingleNumber("SELECT id FROM petitions WHERE id=%d AND player=%u", petitionID, playerID.Unbox());
    if(result <= 0)
    {
        // Failure was due to nonexistant petition or ownership rights:
        lasterror.Format("Couldn't cancel the petition. Either it does not exist, or you did not "
                         "create the petition.");
        return false;
    }

    // Update the petition status
    result = db->CommandPump("UPDATE petitions SET status='Cancelled' WHERE id=%d AND player=%u", petitionID, playerID.Unbox());

    return (result > 0);
}

bool AdminManager::ChangePetition(PID playerID, int petitionID, const char* petition)
{
    csString escape;
    db->Escape(escape, petition);

    // If player ID is -1, just change the petition (a GM is requesting the change)
    if(playerID == PETITION_GM)
    {
        unsigned long result = db->Command("UPDATE petitions SET petition=\"%s\" WHERE id=%u", escape.GetData(), playerID.Unbox());
        return (result != QUERY_FAILED);
    }

    // Attempt to select this petition;
    // following things can go wrong:
    // - it doesn't exist
    // - the player didn't create it
    // - it has not the right status
    int result = db->SelectSingleNumber("SELECT id FROM petitions WHERE id=%d AND player=%u AND status='Open'", petitionID, playerID.Unbox());
    if(result <= 0)
    {
        lasterror.Format("Couldn't change the petition. Either it does not exist, or you did not "
                         "create the petition, or it has not correct status.");
        return false;
    }

    // Update the petition status
    result = db->CommandPump("UPDATE petitions SET petition=\"%s\" WHERE id=%d AND player=%u", escape.GetData(), petitionID, playerID.Unbox());

    return (result > 0);
}

bool AdminManager::ClosePetition(PID gmID, int petitionID, const char* desc)
{
    csString escape;
    db->Escape(escape, desc);
    int result = db->CommandPump("UPDATE petitions SET status='Closed',closed_date=Now(),resolution='%s' "
                                 "WHERE id=%d AND assigned_gm=%u", escape.GetData(), petitionID, gmID.Unbox());

    // If this failed if means that there is a serious error, or the GM was not assigned
    if(result <= 0)
    {
        lasterror.Format("Couldn't close petition #%d.  You must be assigned to the petition before you close it.",
                         petitionID);
        return false;
    }

    return true;
}

bool AdminManager::DeassignPetition(PID gmID, int gmLevel, int petitionID)
{
    int result;
    if(gmLevel > GM_LEVEL_5)  //allows to deassing without checks only to a gm lead or a developer
    {
        result = db->CommandPump("UPDATE petitions SET assigned_gm=-1,status=\"Open\" WHERE id=%d", petitionID);
    }
    else
    {
        result = db->CommandPump("UPDATE petitions SET assigned_gm=-1,status=\"Open\" WHERE id=%d AND assigned_gm=%u", petitionID, gmID.Unbox());
    }

    // If this failed if means that there is a serious error, or another GM was already assigned
    if(result <= 0)
    {
        lasterror.Format("Couldn't deassign you to petition #%d.  Another GM is assigned to that petition.",
                         petitionID);
        return false;
    }

    return true;
}

bool AdminManager::AssignPetition(PID gmID, int petitionID)
{
    int result = db->CommandPump("UPDATE petitions SET assigned_gm=%d,status='In Progress' WHERE id=%d AND assigned_gm=-1", gmID.Unbox(), petitionID);

    // If this failed if means that there is a serious error, or another GM was already assigned
    if(result <= 0)
    {
        lasterror.Format("Couldn't assign you to petition #%d.  Another GM is already assigned to that petition.",
                         petitionID);
        return false;
    }

    return true;
}

bool AdminManager::LogGMCommand(AccountID accountID, PID gmID, PID playerID, const char* cmd)
{
    if(!strncmp(cmd,"/slide",6))  // don't log all these.  spamming the GM log table.
        return true;

    csString escape;
    db->Escape(escape, cmd);
    int result = db->Command("INSERT INTO gm_command_log "
                             "(account_id,gm,command,player,ex_time) "
                             "VALUES (%u,%u,\"%s\",%u,Now())", accountID.Unbox(), gmID.Unbox(), escape.GetData(), playerID.Unbox());
    return (result > 0);
}

const char* AdminManager::GetLastSQLError()
{
    if(!db)
        return "";

    return db->GetLastError();
}

void AdminManager::DeleteCharacter(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataDeleteChar* data = static_cast<AdminCmdDataDeleteChar*>(cmddata);
    if(data->IsTargetType(ADMINCMD_TARGET_PLAYER))
        // Deleting by name; verify the petitioner gave us one of their characters
    {
        if(data->requestor.target.IsEmpty())
        {
            psserver->SendSystemInfo(me->clientnum,"Missing requestor");
            return;
        }

        if(data->GetAccountID(me->clientnum) != data->requestor.GetAccountID(me->clientnum))
        {
            psserver->SendSystemInfo(me->clientnum,"Zombie/Requestor Mismatch, no deletion.");
            return;
        }
    }
    else  // Deleting by PID; make sure this isn't a unique or master NPC
    {
        Result result(db->Select("SELECT name, character_type, npc_master_id FROM characters WHERE id='%u'",data->targetID.Unbox()));
        if(!result.IsValid() || result.Count() != 1)
        {
            psserver->SendSystemError(me->clientnum,"No character found with PID %u!",data->targetID.Unbox());
            return;
        }

        iResultRow &row = result[0];
        data->target = row["name"];
        unsigned int charType = row.GetUInt32("character_type");
        unsigned int masterID = row.GetUInt32("npc_master_id");

        if(charType == PSCHARACTER_TYPE_NPC)
        {
            if(masterID == 0)
            {
                psserver->SendSystemError(me->clientnum,"%s is a unique NPC, and may not be deleted", data->target.GetData());
                return;
            }

            if(masterID == data->targetID.Unbox())
            {
                psserver->SendSystemError(me->clientnum,"%s is a master NPC, and may not be deleted", data->target.GetData());
                return;
            }
        }
    }
    if(data->IsOnline())
    {
        psserver->SendSystemError(me->clientnum,"%s is currently online.", data->target.GetData());
        return;
    }

    csString error;
    if(psserver->CharacterLoader.DeleteCharacterData(data->targetID, error))
        psserver->SendSystemInfo(me->clientnum,"Character %s (PID %u) has been deleted.", data->target.GetData(), data->targetID.Unbox());
    else
    {
        if(error.Length())
            psserver->SendSystemError(me->clientnum,"Deletion error: %s", error.GetData());
        else
            psserver->SendSystemError(me->clientnum,"Character deletion got unknown error!", error.GetData());
    }
}

void AdminManager::ChangeName(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataChangeName* data = static_cast<AdminCmdDataChangeName*>(cmddata);
    Client* target = NULL;

    if((!data->target.Length() || !data->newName.Length()) && !data->targetObject)
    {
        psserver->SendSystemInfo(me->clientnum,data->GetHelpMessage());
        return;
    }

    if(data->targetObject)
    {
        if(!data->targetObject->GetCharacterData())  //no need to go on this isn't an npc or pc characther (most probably an item)
        {
            psserver->SendSystemError(me->clientnum,"Changename can be used only on Player or NPC characters");
            return;
        }
        target = data->targetObject->GetClient(); //get the client target, this will return NULL if it's an NPC
    }

    //if we are using the name we must check that it's unique to avoid unwanted changes
    if(data->duplicateActor)
    {
        psserver->SendSystemError(me->clientnum,"Multiple characters with same name '%s'. Use pid.",data->target.GetData());
        return;
    }

    bool online = (target != NULL);

    // Fix names
    data->newName = NormalizeCharacterName(data->newName);
    data->newLastName = NormalizeCharacterName(data->newLastName);
    csString name = NormalizeCharacterName(data->target);

    PID pid;
    unsigned int type = 0;
    unsigned int gid = 0;

    csString prevFirstName,prevLastName;

    // Check the DB if the player isn't online
    if(!online)
    {
        csString query;
        //check if it's an npc
        pid = data->targetID;

        if(pid.IsValid())
        {
            query.Format("SELECT id,name,lastname,character_type,guild_member_of FROM characters WHERE id=%u", pid.Unbox());
        }
        else
        {
            query.Format("SELECT id,name,lastname,character_type,guild_member_of FROM characters WHERE name='%s'",name.GetData());
        }

        Result result(db->Select(query));
        if(!result.IsValid() || result.Count() == 0)
        {
            psserver->SendSystemError(me->clientnum,"No online or offline player found with the name %s!",name.GetData());
            return;
        }
        else if(result.Count() != 1)
        {
            psserver->SendSystemError(me->clientnum,"Multiple characters with same name '%s'. Use pid.",name.GetData());
            return;
        }
        else
        {
            iResultRow &row = result[0];
            prevFirstName = row["name"];
            prevLastName = row["lastname"];
            pid = PID(row.GetUInt32("id"));
            gid = row.GetUInt32("guild_member_of");
            type = row.GetUInt32("character_type");
            if(type == PSCHARACTER_TYPE_NPC)
            {
                if(!psserver->CheckAccess(client, "change NPC names"))
                    return;
            }
        }
    }
    else
    {
        prevFirstName = target->GetCharacterData()->GetCharName();
        prevLastName = target->GetCharacterData()->GetCharLastName();
        pid = target->GetCharacterData()->GetPID();
        gid = target->GetGuildID();
        type = target->GetCharacterData()->GetCharType();
    }

    bool checkFirst=true; //If firstname is same as before, skip DB check
    bool checkLast=true; //If we make the newLastName var the current value, we need to skip the db check on that

    if(data->newLastName.CompareNoCase("no"))
    {
        data->newLastName.Clear();
        checkLast = false;
    }
    else if(data->newLastName.Length() == 0 || data->newLastName == prevLastName)
    {
        data->newLastName = prevLastName;
        checkLast = false;
    }

    if(data->newName == name)
        checkFirst = false;

    if(!checkFirst && !checkLast && data->newLastName.Length() != 0)
        return;

    if(checkFirst && !CharCreationManager::FilterName(data->newName))
    {
        psserver->SendSystemError(me->clientnum,"The name %s is invalid!",data->newName.GetData());
        return;
    }

    if(checkLast && !CharCreationManager::FilterName(data->newLastName))
    {
        psserver->SendSystemError(me->clientnum,"The last name %s is invalid!",data->newLastName.GetData());
        return;
    }

    bool nameUnique = CharCreationManager::IsUnique(data->newName);
    bool allowedToClonename = psserver->CheckAccess(client, "changenameall", false);
    if(!allowedToClonename)
        data->uniqueFirstName=true;

    // If the first name should be unique, check it
    if(checkFirst && data->uniqueFirstName && type == PSCHARACTER_TYPE_PLAYER && !nameUnique)
    {
        psserver->SendSystemError(me->clientnum,"The name %s is not unique!",data->newName.GetData());
        return;
    }

    bool secondNameUnique = CharCreationManager::IsLastNameAvailable(data->newLastName);
    // If the last name should be unique, check it
    if(checkLast && data->uniqueName && data->newLastName.Length() && !secondNameUnique)
    {
        psserver->SendSystemError(me->clientnum,"The last name %s is not unique!",data->newLastName.GetData());
        return;
    }

    if(checkFirst && !data->uniqueFirstName && type == PSCHARACTER_TYPE_PLAYER && !nameUnique)
    {
        psserver->SendSystemResult(me->clientnum,"WARNING: Changing despite the name %s is not unique!",data->newName.GetData());
    }

    if(checkLast && !data->uniqueName && data->newLastName.Length() && !secondNameUnique)
    {
        psserver->SendSystemResult(me->clientnum,"Changing despite the last name %s is not unique!",data->newLastName.GetData());
    }

    // Apply
    csString fullName;
    EID actorId;
    if(online)
    {
        target->GetCharacterData()->SetFullName(data->newName, data->newLastName);
        fullName = target->GetCharacterData()->GetCharFullName();
        target->SetName(data->newName);
        target->GetActor()->SetName(fullName);
        actorId = target->GetActor()->GetEID();

    }
    else if(type == PSCHARACTER_TYPE_NPC || type == PSCHARACTER_TYPE_PET ||
            type == PSCHARACTER_TYPE_MOUNT || type == PSCHARACTER_TYPE_MOUNTPET)
    {
        gemNPC* npc = psserver->entitymanager->GetGEM()->FindNPCEntity(pid);
        if(!npc)
        {
            psserver->SendSystemError(me->clientnum,"Unable to find NPC %s!",
                                      name.GetData());
            return;
        }
        npc->GetCharacterData()->SetFullName(data->newName, data->newLastName);
        fullName = npc->GetCharacterData()->GetCharFullName();
        npc->SetName(fullName);
        actorId = npc->GetEID();
    }

    // Inform
    if(online)
    {
        psserver->SendSystemInfo(
            target->GetClientNum(),
            "Your name has been changed to %s %s by GM %s",
            data->newName.GetData(),
            data->newLastName.GetData(),
            client->GetName()
        );
    }

    psserver->SendSystemInfo(me->clientnum,
                             "%s %s is now known as %s %s",
                             prevFirstName.GetDataSafe(),
                             prevLastName.GetDataSafe(),
                             data->newName.GetDataSafe(),
                             data->newLastName.GetDataSafe()
                            );

    // Update
    if((online || type == PSCHARACTER_TYPE_NPC || type == PSCHARACTER_TYPE_PET
            || type == PSCHARACTER_TYPE_MOUNT || type == PSCHARACTER_TYPE_MOUNTPET) && data->targetObject->GetActorPtr())
    {
        psUpdateObjectNameMessage newNameMsg(0, actorId, fullName);

        csArray<PublishDestination> &clients = data->targetObject->GetActorPtr()->GetMulticastClients();
        newNameMsg.Multicast(clients, 0, PROX_LIST_ANY_RANGE);
    }

    // Need instant DB update if we should be able to change the same persons name twice
    db->CommandPump("UPDATE characters SET name='%s', lastname='%s' WHERE id='%u'",data->newName.GetData(),data->newLastName.GetDataSafe(), pid.Unbox());

    // Resend group list
    if(online)
    {
        csRef<PlayerGroup> group = target->GetActor()->GetGroup();
        if(group)
            group->BroadcastMemberList();
    }

    // Handle guild update
    psGuildInfo* guild = psserver->GetCacheManager()->FindGuild(gid);
    if(guild)
    {
        psGuildMember* member = guild->FindMember(pid);
        if(member)
        {
            member->name = data->newName;
        }
    }

    Client* buddy;
    /*We update the buddy list of the people who have the target in their own buddy list and
      they are online. */
    if(online)
    {
        csArray<PID> buddyOfList = target->GetCharacterData()->GetBuddyMgr().GetBuddyOfList();

        for(size_t i=0; i<buddyOfList.GetSize(); i++)
        {
            buddy = clients->FindPlayer(buddyOfList[i]);
            if(buddy && buddy->IsReady())
            {
                buddy->GetCharacterData()->GetBuddyMgr().RemoveBuddy(pid);
                buddy->GetCharacterData()->GetBuddyMgr().AddBuddy(pid, data->newName);
                //We refresh the buddy list
                psserver->usermanager->BuddyList(buddy, buddy->GetClientNum(), true);
            }
        }
    }
    else
    {
        unsigned int buddyid;

        //If the target is offline then we select all the players online that have him in the buddylist
        Result result2(db->Select("SELECT character_id FROM character_relationships WHERE relationship_type = 'buddy' and related_id = '%u'", pid.Unbox()));

        if(result2.IsValid())
        {
            for(unsigned long j=0; j<result2.Count(); j++)
            {
                iResultRow &row = result2[j];
                buddyid = row.GetUInt32("character_id");
                buddy = clients->FindPlayer(buddyid);
                if(buddy && buddy->IsReady())
                {
                    buddy->GetCharacterData()->GetBuddyMgr().RemoveBuddy(pid);
                    buddy->GetCharacterData()->GetBuddyMgr().AddBuddy(pid, data->newName);
                    //We refresh the buddy list
                    psserver->usermanager->BuddyList(buddy, buddy->GetClientNum(), true);
                }
            }
        }
    }
}

void AdminManager::BanName(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);
    if(!data->target.Length())
    {
        psserver->SendSystemError(me->clientnum, "You have to specify a name to ban");
        return;
    }

    if(data->target.CompareNoCase(client->GetName()))
    {
        psserver->SendSystemError(me->clientnum, "You can't ban your own name!");
        return;
    }

    if(psserver->GetCharManager()->IsBanned(data->target))
    {
        psserver->SendSystemError(me->clientnum, "That name is already banned");
        return;
    }

    psserver->GetCacheManager()->AddBadName(data->target);
    psserver->SendSystemInfo(me->clientnum, "You banned the name '%s'", data->target.GetDataSafe());
}

void AdminManager::UnBanName(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if(!psserver->GetCharManager()->IsBanned(data->target))
    {
        psserver->SendSystemError(me->clientnum,"That name is not banned");
        return;
    }

    psserver->GetCacheManager()->DelBadName(data->target);
    psserver->SendSystemInfo(me->clientnum,"You unbanned the name '%s'",data->target.GetDataSafe());
}

void AdminManager::BanClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* client)
{
    AdminCmdDataBan* data = static_cast<AdminCmdDataBan*>(cmddata);
    const time_t year = 31536000UL; //one year should be enough
    const time_t twodays = (2 * 24 * 60 * 60);

    time_t secs = (data->minutes * 60) + (data->hours * 60 * 60) + (data->days * 24 * 60 * 60);

    if(secs == 0)
        secs = twodays; // Two day ban by default

    if(secs > year)
        secs = year; //some errors if time was too high

    if(secs > twodays && !psserver->CheckAccess(client, "long bans"))
    {
        psserver->SendSystemError(me->clientnum, "You can only ban for up to two days.");
        return;
    }

    if(data->IsTargetTypeUnknown())
    {
        psserver->SendSystemError(me->clientnum, "You must specify a player name or an account name or number.");
        return;
    }

    // Find client to get target
    if(!data->IsTargetType(ADMINCMD_TARGET_PLAYER) && !data->IsTargetType(ADMINCMD_TARGET_PID) &&
            !data->IsTargetType(ADMINCMD_TARGET_ACCOUNT))
    {
        psserver->SendSystemError(me->clientnum, "You can only ban a player!");
        return;
    }

    if(data->targetClient && data->targetClient->GetClientNum() == client->GetClientNum())
    {
        psserver->SendSystemError(me->clientnum, "You can't ban yourself!");
        return;
    }

    if(data->reason.Length() < 5)
    {
        psserver->SendSystemError(me->clientnum, "You must specify a reason to ban");
        return;
    }

    Result result;
    AccountID accountID = data->GetAccountID(me->clientnum);

    if(!accountID.IsValid())
    {
        // not found
        psserver->SendSystemError(me->clientnum, "Couldn't find account with the name %s",data->target.GetDataSafe());
        return;
    }

    result = db->Select("SELECT * FROM accounts WHERE id = '%u' LIMIT 1",accountID.Unbox());
    if(!result.IsValid() || !result.Count())
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find account with id %u",accountID.Unbox());
        return;
    }

    csString user = data->target;

    // Ban by IP range, as well as account
    csString ip_range = Client::GetIPRange(result[0]["last_login_ip"]);

    if(!psserver->GetAuthServer()->GetBanManager()->AddBan(accountID,ip_range,secs,data->reason,data->banIP))
    {
        // Error adding; entry must already exist
        psserver->SendSystemError(me->clientnum, "%s is already banned", user.GetData());
        return;
    }

    // Now we have a valid player target, so remove from server
    if(data->targetClient)
    {
        if(secs < year)
        {
            csString reason;
            if(secs == twodays)
                reason.Format("You were banned from the server by a GM for two days. Reason: %s", data->reason.GetData());
            else
                reason.Format("You were banned from the server by a GM for %d minutes, %d hours and %d days. Reason: %s",
                              data->minutes, data->hours, data->days, data->reason.GetData());

            psserver->RemovePlayer(data->targetClient->GetClientNum(),reason);
        }
        else
            psserver->RemovePlayer(data->targetClient->GetClientNum(),"You were banned from the server by a GM for a year. Reason: " + data->reason);
    }

    csString notify;
    notify.Format("You%s banned '%s' off the server for ", (data->targetClient)?" kicked and":"", user.GetData());
    if(secs == year)
        notify.Append("a year.");
    else if(secs == twodays)
        notify.Append("two days.");
    else
        notify.AppendFmt("%d minutes, %d hours and %d days.", data->minutes, data->hours, data->days);
    if(data->banIP)
        notify.AppendFmt(" They will also be banned by IP range.");

    // Finally, notify the client who kicked the target
    psserver->SendSystemInfo(me->clientnum,notify);
}

void AdminManager::UnbanClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* gm)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);
    if(data->IsTargetTypeUnknown())
    {
        psserver->SendSystemError(me->clientnum, "You must specify a player name or an account name or number.");
        return;
    }

    // Check if the target is online, if he/she/it is he/she/it can't be unbanned (No logic in it).
    if(data->IsOnline())
    {
        psserver->SendSystemError(me->clientnum, "The player is active and is playing.");
        return;
    }

    Result result;
    AccountID accountID = data->GetAccountID(me->clientnum);

    if(!accountID.IsValid())
    {
        // not found
        psserver->SendSystemError(me->clientnum, "Couldn't find account with the name %s",data->target.GetDataSafe());
        return;
    }

    result = db->Select("SELECT * FROM accounts WHERE id = '%u' LIMIT 1",accountID.Unbox());
    if(!result.IsValid() || !result.Count())
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find account with id %u",accountID.Unbox());
        return;
    }

    csString user = result[0]["username"];

    // How long is the ban?
    result = db->Select("SELECT * FROM bans WHERE account = '%u' LIMIT 1",accountID.Unbox());
    if(!result.IsValid() || !result.Count())
    {
        psserver->SendSystemError(me->clientnum, "%s is not banned", user.GetData());
        return;
    }

    const time_t twodays = (2 * 24 * 60 * 60);
    time_t end = result[0].GetUInt32("end");
    time_t start = result[0].GetUInt32("start");

    if((end - start > twodays) && !psserver->CheckAccess(gm, "long bans"))  // Longer than 2 days, must have special permission to unban
    {
        psserver->SendSystemResult(me->clientnum, "You can only unban players with less than a two day ban.");
        return;
    }


    if(psserver->GetAuthServer()->GetBanManager()->RemoveBan(accountID))
        psserver->SendSystemResult(me->clientnum, "%s has been unbanned", user.GetData());
    else
        psserver->SendSystemError(me->clientnum, "%s is not banned", user.GetData());
}

void AdminManager::BanAdvisor(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* gm)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if(data->IsTargetTypeUnknown())
    {
        psserver->SendSystemError(me->clientnum, "You must specify a player name or an account name or number.");
        return;
    }

    // Check if the target is online,
    if(data->IsOnline())
    {
        data->targetClient->SetAdvisorBan(true);
        psserver->SendSystemResult(me->clientnum, "%s has been banned from advising.", data->target.GetData());
        return;
    }

    Result result;
    AccountID accountID = data->GetAccountID(me->clientnum);

    if(!accountID.IsValid())
    {
        // not found
        psserver->SendSystemError(me->clientnum, "Couldn't find account with the name %s",data->target.GetDataSafe());
        return;
    }

    result = db->Select("SELECT * FROM accounts WHERE id = '%u' LIMIT 1",accountID.Unbox());
    if(!result.IsValid() || !result.Count())
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find account with id %u",accountID.Unbox());
        return;
    }

    db->Command("UPDATE accounts SET advisor_ban = 1 WHERE id = %d", accountID.Unbox());

    psserver->SendSystemResult(me->clientnum, "%s has been banned from advising.", data->target.GetData());
}

void AdminManager::UnbanAdvisor(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata,Client* gm)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if(data->IsTargetTypeUnknown())
    {
        psserver->SendSystemError(me->clientnum, "You must specify a player name or an account name or number.");
        return;
    }

    // Check if the target is online,
    if(data->IsOnline())
    {
        data->targetClient->SetAdvisorBan(false);
        psserver->SendSystemResult(me->clientnum, "Ban from advising has been lifted for %s.", data->target.GetData());
        return;
    }

    Result result;
    AccountID accountID = data->GetAccountID(me->clientnum);

    if(!accountID.IsValid())
    {
        // not found
        psserver->SendSystemError(me->clientnum, "Couldn't find account with the name %s",data->target.GetDataSafe());
        return;
    }

    result = db->Select("SELECT * FROM accounts WHERE id = '%u' LIMIT 1",accountID.Unbox());
    if(!result.IsValid() || !result.Count())
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find account with id %u",accountID.Unbox());
        return;
    }

    db->Command("UPDATE accounts SET advisor_ban = 0 WHERE id = %d", accountID.Unbox());

    psserver->SendSystemResult(me->clientnum, "%s has been unbanned from advising.", data->target.GetData());
}

void AdminManager::SendSpawnTypes(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data,Client* client)
{
    csArray<csString> itemCat;
    unsigned int size = 0;

    for(unsigned int i = 1;; i++)
    {
        psItemCategory* cat = psserver->GetCacheManager()->GetItemCategoryByID(i);
        if(!cat)
            break;

        size += (int)strlen(cat->name)+1;
        itemCat.Push(cat->name);
    }

    itemCat.Sort();
    psGMSpawnTypes msg2(me->clientnum,size);

    // Add the numbers of types
    msg2.msg->Add((uint32_t)itemCat.GetSize());

    for(size_t i = 0; i < itemCat.GetSize(); i++)
    {
        msg2.msg->Add(itemCat.Get(i));
    }

    msg2.SendMessage();
}

void AdminManager::SendSpawnItems(MsgEntry* me, Client* client)
{
    psGMSpawnItems msg(me);

    csArray<psItemStats*> items;
    size_t size = 0;
    if(!psserver->CheckAccess(client, "/item"))
    {
        return;
    }

    psItemCategory* category = psserver->GetCacheManager()->GetItemCategoryByName(msg.type);
    if(!category)
    {
        psserver->SendSystemError(me->clientnum, "Category %s is not valid.", msg.type.GetData());
        return;
    }

    // Database hit.
    // Justification:  This is a rare event and it is quicker than us doing a sort.
    //                 Is also a read only event.
    Result result(db->Select("SELECT id FROM item_stats WHERE category_id=%d AND stat_type not in ('U','R') ORDER BY Name ", category->id));
    if(!result.IsValid() || result.Count() == 0)
    {
        psserver->SendSystemError(me->clientnum, "Could not query database for category %s.", msg.type.GetData());
        return;
    }

    bool spawnAll = psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "spawn all");

    for(unsigned int i=0; i < result.Count(); i++)
    {
        unsigned id = result[i].GetUInt32(0);
        psItemStats* item = psserver->GetCacheManager()->GetBasicItemStatsByID(id);
        if(item && !item->IsMoney() && (item->IsSpawnable() || spawnAll))
        {
            csString name(item->GetName());
            csString mesh(item->GetMeshName());
            csString icon(item->GetImageName());
            size += name.Length()+mesh.Length()+icon.Length()+3;
            items.Push(item);
        }
    }

    psGMSpawnItems msg2(me->clientnum,msg.type,size);

    // Add the numbers of types
    msg2.msg->Add((uint32_t)items.GetSize());

    for(size_t i = 0; i < items.GetSize(); i++)
    {
        psItemStats* item = items.Get(i);
        msg2.msg->Add(item->GetName());
        msg2.msg->Add(item->GetMeshName());
        msg2.msg->Add(item->GetImageName());
    }

    Debug4(LOG_ADMIN, me->clientnum, "Sending %zu items from the %s category to client %s\n", items.GetSize(), msg.type.GetData(), client->GetName());

    msg2.SendMessage();
}

void AdminManager::SendSpawnMods(MsgEntry* me, Client* client)
{
    if(!psserver->CheckAccess(client, "/item"))
    {
        return;
    }

    psGMSpawnGetMods msg(me);

    // Get the basic stats
    psItemStats* stats = psserver->GetCacheManager()->GetBasicItemStatsByName(msg.item);
    if(!stats)
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find basic stats for %s!", msg.item.GetDataSafe());
        return;
    }

    LootRandomizer* lootRandomizer = psserver->GetSpawnManager()->GetLootRandomizer();
    csArray<psGMSpawnMods::ItemModifier> mods;

    if(lootRandomizer->GetModifiers(stats->GetUID(), mods))
    {
        psGMSpawnMods msg2(me->clientnum, mods);
        msg2.SendMessage();
    }
    else
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find item modifiers for %s!", msg.item.GetDataSafe());
    }
}

void AdminManager::SpawnItemInv(MsgEntry* me, Client* client)
{
    psGMSpawnItem msg(me);
    SpawnItemInv(me, msg, client);
}

void AdminManager::SpawnItemInv(MsgEntry* me, psGMSpawnItem &msg, Client* client)
{
    if(!psserver->CheckAccess(client, "/item"))
    {
        return;
    }

    psCharacter* charData = client->GetCharacterData();
    if(!charData)
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find your character data!");
        return;
    }

    // Get the basic stats
    psItemStats* stats = psserver->GetCacheManager()->GetBasicItemStatsByName(msg.item);
    if(!stats)
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find basic stats for that item!");
        return;
    }

    // creating money items will confuse the server into creating the money in the db then deleting it again when adding to the inventory
    if(stats->IsMoney())
    {
        psserver->SendSystemError(me->clientnum, "Spawning money items is not permitted. Use /award me money instead");
        return;
    }

    // Check skill
    PSSKILL skill = psserver->GetCacheManager()->ConvertSkillString(msg.lskill);
    if(skill == PSSKILL_NONE && msg.lockable)
    {
        psserver->SendSystemError(me->clientnum, "Couldn't find the lock skill!");
        return;
    }

    if(msg.count < 1 || msg.count > MAX_STACK_COUNT)
    {
        psserver->SendSystemError(me->clientnum, "Invalid stack count!");
        return;
    }

    if(!stats->IsSpawnable() && !psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "spawn all"))
    {
        psserver->SendSystemError(me->clientnum, "This item cannot be spawned!");
        return;
    }

    // Prepare text for error messages.
    csString text;

    psItem* item = stats->InstantiateBasicItem();

    if(stats->GetBuyPersonalise())
    {
        // Spawn personlised item.

        PSITEMSTATS_CREATORSTATUS creatorStatus;
        item->GetCreator(creatorStatus);

        // Prepare name for personalised item.
        csString itemName(item->GetName());
        if(creatorStatus != PSITEMSTATS_CREATOR_PUBLIC)
            itemName.AppendFmt(" of %s", client->GetName());
        csString personalisedName(itemName);

        item->SetName(personalisedName);

        // copies the template creative data from the item stats in the item
        // instances for use by the player
        item->PrepareCreativeItemInstance();
    }
    else
    {
        LootRandomizer* lootRandomizer = psserver->GetSpawnManager()->GetLootRandomizer();
        // Spawn normal item.

        // randomize if requested
        if(msg.random)
        {
            item = lootRandomizer->RandomizeItem(item, msg.quality);
        }
        else
        {
            item = lootRandomizer->SetModifiers(item, msg.mods);
        }

        item->SetStackCount(msg.count);
        item->SetIsPickupableWeak(msg.pickupableWeak);
        item->SetIsCD(msg.collidable);
        item->SetIsTransient(msg.Transient);

        //These are setting only flags. When modify gets valid permission update also here accordly
        if(psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "full modify"))
        {
            item->SetIsSettingItem(msg.SettingItem);
            item->SetIsNpcOwned(msg.NPCOwned);
            item->SetIsPickupable(msg.pickupable);
            item->SetIsLockable(msg.lockable);
            item->SetIsLocked(msg.locked);
            item->SetIsUnpickable(msg.Unpickable);
            if(msg.lockable)
            {
                item->SetLockpickSkill(skill);
                item->SetLockStrength(msg.lstr);
            }
        }

        if(msg.quality > 0)
        {
            item->SetItemQuality(msg.quality);
            // Setting craft quality as well if quality given by user
            item->SetMaxItemQuality(msg.quality);
        }
        else
        {
            item->SetItemQuality(stats->GetQuality());
        }
    }

    item->SetLoaded();  // Item is fully created
    if(charData->Inventory().Add(item))
    {
        if(msg.count > 1)
        {
            text.Format("You spawned %d %s to your inventory.", msg.count, psString(msg.item).Plural().GetData());
        }
        else
        {
            text.Format("You spawned %s to your inventory.", msg.item.GetData());
        }
    }
    else
    {
        text.Format("Couldn't spawn %s to your inventory, maybe it's full?",msg.item.GetData());
        psserver->GetCacheManager()->RemoveInstance(item);
    }

    psserver->SendSystemInfo(me->clientnum,text);
    psserver->GetCharManager()->UpdateItemViews(me->clientnum);
    //TODO:should augument this more.
    LogGMCommand(client->GetAccountID(), client->GetPID(), client->GetPID(), text.GetData());
}

void AdminManager::Award(AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataAward* data = static_cast<AdminCmdDataAward*>(cmddata);

    // check that there is at least one reward
    if(data->rewardList.rewards.GetSize() == 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "Nothing to award.");
        return;
    }
    else
    {
        csPDelArray<psRewardData>::Iterator it = data->rewardList.rewards.GetIterator();
        while(it.HasNext())
        {
            AwardToTarget(client->GetClientNum(), data->targetClient, it.Next());
        }
    }
}

void AdminManager::AwardToTarget(unsigned int gmClientNum, Client* target, psRewardData* data)
{
    if(data->IsZero())
    {
        psserver->SendSystemError(gmClientNum, "Nothing to award.");
        return;
    }

    if(!target)
    {
        psserver->SendSystemError(gmClientNum, "No target selected!");
        return;
    }

    if(!target->GetCharacterData())
    {
        psserver->SendSystemError(gmClientNum, "Invalid target to award");
        return;
    }

    psCharacter* pchar = target->GetCharacterData();
    if(pchar->IsNPC())
    {
        psserver->SendSystemError(gmClientNum, "You can't use this command on npcs!");
        return;
    }

    if(data->rewardType == psRewardData::REWARD_EXPERIENCE)
    {
        psRewardDataExperience* rewardDataExperience = static_cast<psRewardDataExperience*>(data);
        AwardExperienceToTarget(gmClientNum, target, rewardDataExperience->expDelta);
    }

    if(data->rewardType == psRewardData::REWARD_ITEM)  // award item
    {
        psRewardDataItem* rewardDataItem = static_cast<psRewardDataItem*>(data);

        csString text;
        psItemStats* stats = psserver->GetCacheManager()->GetBasicItemStatsByName(rewardDataItem->itemName);
        if(!stats)
        {
            psserver->SendSystemError(gmClientNum, "You have to specify a valid item name");
        }
        else if(stats->IsMoney())
        {
            psserver->SendSystemError(gmClientNum, "Use the 'money' award to award money, not the 'item' one.");
        }
        else if(stats->GetBuyPersonalise())
        {
            psserver->SendSystemError(gmClientNum, "You cannot award personalized items.");
        }
        else if(rewardDataItem->stackCount > MAX_STACK_COUNT)
        {
            text.Format("The value for the stackCount has to be between 0 and %u", MAX_STACK_COUNT);
            psserver->SendSystemError(gmClientNum, text);
        }
        else // eveyrthing alright - create item
        {
            psItem* item = stats->InstantiateBasicItem(true);
            item->SetStackCount(rewardDataItem->stackCount);
            item->SetItemQuality(stats->GetQuality());
            item->SetLoaded();
            pchar->Inventory().AddOrDrop(item);
            psserver->GetCharManager()->UpdateItemViews(target->GetClientNum());

            // notify target
            text.Format("%d %s", rewardDataItem->stackCount, rewardDataItem->itemName.GetDataSafe());
            SendAwardInfo(gmClientNum, target, "item(s)",text.GetData(),1);

            Debug4(LOG_ADMIN, gmClientNum, "Created %d %s for %s\n", rewardDataItem->stackCount, rewardDataItem->itemName.GetDataSafe(), target->GetName());
        }
    }

    if(data->rewardType == psRewardData::REWARD_FACTION)
        // award faction
    {
        psRewardDataFaction* rewardDataFaction = static_cast<psRewardDataFaction*>(data);
        AdjustFactionStandingOfTarget(gmClientNum, target, rewardDataFaction->factionName, rewardDataFaction->factionDelta);
    }

    if(data->rewardType == psRewardData::REWARD_SKILL)  // award skill
    {
        psRewardDataSkill* rewardDataSkill = static_cast<psRewardDataSkill*>(data);

        bool modified = false;
        if(rewardDataSkill->skillName == "all")  // update all skills
        {
            for(size_t i = 0; i < psserver->GetCacheManager()->GetSkillAmount(); i++)
            {
                psSkillInfo* skill = psserver->GetCacheManager()->GetSkillByID(i);
                if(!skill) continue;  // skill doesn't exist -> this should not happen
                modified |= ApplySkill(gmClientNum, target, skill, rewardDataSkill->skillDelta, rewardDataSkill->relativeSkill, rewardDataSkill->skillCap);
            }
        }
        else // update a certain one
        {
            psSkillInfo* skill = psserver->GetCacheManager()->GetSkillByName(rewardDataSkill->skillName);
            modified |= ApplySkill(gmClientNum, target, skill, rewardDataSkill->skillDelta, rewardDataSkill->relativeSkill, rewardDataSkill->skillCap);
        }

        if(modified && target)  // update client view if we changed something
        {
            psserver->GetProgressionManager()->SendSkillList(target, false);
        }
    }

    if(data->rewardType == psRewardData::REWARD_PRACTICE)  // award skill practice
    {
        psRewardDataPractice* rewardDataPractice = static_cast<psRewardDataPractice*>(data);

        bool modified = false;
        if(rewardDataPractice->skillName == "all")  // update all skills
        {
            for(size_t i = 0; i < psserver->GetCacheManager()->GetSkillAmount(); i++)
            {
                psSkillInfo* skill = psserver->GetCacheManager()->GetSkillByID(i);
                if(!skill) continue;  // skill doesn't exist -> this should not happen
                modified |= target->GetCharacterData()->Skills().AddSkillPractice(skill, rewardDataPractice->practice);
            }
        }
        else // update a certain one
        {
            psSkillInfo* skill = psserver->GetCacheManager()->GetSkillByName(rewardDataPractice->skillName);
            modified |= target->GetCharacterData()->Skills().AddSkillPractice(skill, rewardDataPractice->practice);
        }

        if(modified && target)  // update client view if we changed something
        {
            psserver->GetProgressionManager()->SendSkillList(target, false);
        }
    }

    if(data->rewardType == psRewardData::REWARD_MONEY)  // award money
    {
        psRewardDataMoney* rewardDataMoney = static_cast<psRewardDataMoney*>(data);
        bool valid = true;

        // determine money type
        Money_Slots type;
        if(rewardDataMoney->moneyType == "trias")
            type = MONEY_TRIAS;
        else if(rewardDataMoney->moneyType == "hexas")
            type = MONEY_HEXAS;
        else if(rewardDataMoney->moneyType == "octas")
            type = MONEY_OCTAS;
        else if(rewardDataMoney->moneyType == "circles")
            type = MONEY_CIRCLES;
        else
            valid = false;

        if(valid)
        {
            int value;
            if(!rewardDataMoney->random)  // fixed amount
                value = rewardDataMoney->moneyCount;
            else // random amount
                value = psserver->rng->Get(rewardDataMoney->moneyCount)+1;
            psMoney money;
            money.Set(type, value);
            pchar->AdjustMoney(money, false);

            // update client view
            psserver->GetCharManager()->SendPlayerMoney(target);

            // notify target
            csString text;
            text.Format("You have been awarded %d %s.", value, rewardDataMoney->moneyType.GetDataSafe());
            SendAwardInfo(gmClientNum, target, rewardDataMoney->moneyType.GetDataSafe(), text.GetData(), value);

            Debug4(LOG_ADMIN, gmClientNum, "Created %d %s for %s\n", value, rewardDataMoney->moneyType.GetDataSafe(), pchar->GetCharName());
        }
        else
        {
            psserver->SendSystemError(gmClientNum, "Invalid money type");
        }
    }
}

void AdminManager::AwardExperienceToTarget(int gmClientnum, Client* target, int ppAward)
{
    unsigned int pp = target->GetCharacterData()->GetProgressionPoints();

    if(pp == 0 && ppAward < 0)
    {
        psserver->SendSystemError(gmClientnum, "Target has no experience to penalize");
        return;
    }

    if(ppAward < 0 && (unsigned int) abs(ppAward) > pp)  //we need to check for "underflows"
    {
        pp = 0;
        psserver->SendSystemError(gmClientnum, "Target experience got to minimum so requested exp was cropped!");
    }
    else if((uint64) pp+ppAward > UINT_MAX)  //...and for overflows
    {
        pp = UINT_MAX;
        psserver->SendSystemError(gmClientnum, "Target experience got to maximum so requested exp was cropped!");
    }
    else
    {
        pp += ppAward; // Negative changes are allowed
    }

    target->GetCharacterData()->SetProgressionPoints(pp,true);

    csString text;
    text.Format("%d progression points", ppAward);
    SendAwardInfo(gmClientnum, target, "progression points", text.GetData(), ppAward);
}

void AdminManager::SendAwardInfo(size_t gmClientnum, Client* target, const char* awardname, const char* awarddesc, int awarded)
{
    if(awarded > 0)  // it's a real award
    {
        psserver->SendSystemOK(target->GetClientNum(),"You have been awarded %s by a GM", awardname);
        psserver->SendSystemInfo(target->GetClientNum(),"You gained %s.", awarddesc);
    }
    else if(awarded < 0)
    {
        psserver->SendSystemOK(target->GetClientNum(),"You have been penalized %s by a GM", awardname);
        psserver->SendSystemInfo(target->GetClientNum(),"You lost %s.", awarddesc);
    }
    if(gmClientnum != target->GetClientNum())
    {
        psserver->SendSystemInfo(gmClientnum, "%s has been awarded %s.",
                                 target->GetName(), awarddesc);
    }
}

void AdminManager::AdjustFactionStandingOfTarget(int gmClientnum, Client* target, csString factionName, int standingDelta)
{
    Faction* faction = psserver->GetCacheManager()->GetFaction(factionName.GetData());
    if(!faction)
    {
        psserver->SendSystemInfo(gmClientnum, "\'%s\' Unrecognised faction.", factionName.GetData());
        return;
    }

    if(target->GetCharacterData()->UpdateFaction(faction, standingDelta))
    {
        csString text;
        text.Format("%d %s faction points", standingDelta, faction->name.GetData());
        SendAwardInfo(gmClientnum, target, "faction points", text.GetData(), standingDelta);
    }
    else
        psserver->SendSystemError(gmClientnum, "%s\'s standing on \'%s\' faction failed.",
                                  target->GetName(), faction->name.GetData());
}

void AdminManager::TransferItem(MsgEntry* me, psAdminCmdMessage &msg,
                                AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataItemTarget* data = static_cast<AdminCmdDataItemTarget*>(cmddata);

    Client* source = client;
    Client* dest = data->targetClient;

    if(data->command == "/takeitem")
    {
        source = data->targetClient;
        dest = client;
    }

    if(!dest || !dest->GetCharacterData())
    {
        psserver->SendSystemError(me->clientnum, "Invalid character to give to");
        return;
    }

    if(!source || !source->GetCharacterData())
    {
        psserver->SendSystemError(me->clientnum,
                                  "Invalid character to take from");
        return;
    }

    if(source == dest)
    {
        psserver->SendSystemError(me->clientnum,
                                  "Source and target must be different");
        return;
    }

    if(data->stackCount == 0 || data->itemName.IsEmpty())
    {
        psserver->SendSystemError(me->clientnum, data->GetHelpMessage());
        return;
    }

    psCharacter* targetchar = dest->GetCharacterData();
    psCharacter* sourcechar = source->GetCharacterData();

    if(data->itemName.Downcase() == "tria")
    {
        psMoney srcMoney = sourcechar->Money();
        psMoney targetMoney = targetchar->Money();
        int value = data->stackCount;
        if(value == -1)
        {
            value = srcMoney.GetTotal();
        }
        else if(value > srcMoney.GetTotal())
        {
            value = srcMoney.GetTotal();
            psserver->SendSystemError(me->clientnum, "Only %d tria taken.",
                                      srcMoney.GetTotal());
        }
        psMoney transferMoney(0, 0, 0, value);
        transferMoney = transferMoney.Normalized();
        sourcechar->SetMoney(srcMoney - transferMoney);
        psserver->GetCharManager()->UpdateItemViews(source->GetClientNum());
        targetchar->SetMoney(targetMoney + transferMoney);
        psserver->GetCharManager()->UpdateItemViews(dest->GetClientNum());

        // Inform the GM doing the transfer
        psserver->SendSystemOK(me->clientnum,
                               "%d tria transferred from %s to %s", value,
                               source->GetActor()->GetName(), dest->GetActor()->GetName());

        // If we're giving to someone else, notify them
        if(dest->GetClientNum() != me->clientnum)
        {
            psserver->SendSystemOK(dest->GetClientNum(),
                                   "%d tria were given by GM %s.", value,
                                   source->GetActor()->GetName());
        }

        // If we're taking from someone else, notify them
        if(source->GetClientNum() != me->clientnum)
        {
            psserver->SendSystemResult(source->GetClientNum(),
                                       "%d tria were taken by %s.", value,
                                       dest->GetActor()->GetName());
        }
        return;
    }
    else
    {
        psItemStats* itemstats =
            psserver->GetCacheManager()->GetBasicItemStatsByName(data->itemName);

        if(!itemstats)
        {
            psserver->SendSystemError(me->clientnum,
                                      "Cannot find any %s in %s's inventory.",
                                      data->itemName.GetData(), source->GetActor()->GetName());
            return;
        }

        size_t slot = sourcechar->Inventory().FindItemStatIndex(itemstats);

        if(slot == SIZET_NOT_FOUND)
        {
            psserver->SendSystemError(me->clientnum,
                                      "Cannot find any %s in %s's inventory.",
                                      data->itemName.GetData(), source->GetActor()->GetName());
            return;
        }

        InventoryTransaction srcTran(&sourcechar->Inventory());
        psItem* item =
            sourcechar->Inventory().RemoveItemIndex(slot, data->stackCount); // data->stackCount is the stack count to move, or -1
        if(!item)
        {
            Error2("Cannot RemoveItemIndex on slot %zu.\n", slot);
            psserver->SendSystemError(me->clientnum,
                                      "Cannot remove %s from %s's inventory.",
                                      data->itemName.GetData(), source->GetActor()->GetName());
            return;
        }
        psserver->GetCharManager()->UpdateItemViews(source->GetClientNum());

        if(item->GetStackCount() < data->stackCount)
        {
            psserver->SendSystemError(me->clientnum,
                                      "There are only %d, not %d in the stack.",
                                      item->GetStackCount(), data->stackCount);
            return;
        }

        bool wasEquipped = item->IsEquipped();

        //we need to get this before the items are stacked in the destination inventory
        int StackCount = item->GetStackCount();

        // Now here we handle the target machine
        InventoryTransaction trgtTran(&targetchar->Inventory());

        if(!targetchar->Inventory().Add(item))
        {
            psserver->SendSystemError(me->clientnum,
                                      "Target inventory is too full to accept item transfer.");
            return;
        }
        psserver->GetCharManager()->UpdateItemViews(dest->GetClientNum());

        // Inform the GM doing the transfer
        psserver->SendSystemOK(me->clientnum,
                               "%u %s transferred from %s's %s to %s", StackCount, item->GetName(),
                               source->GetActor()->GetName(), wasEquipped ? "equipment"
                               : "inventory", dest->GetActor()->GetName());

        // If we're giving to someone else, notify them
        if(dest->GetClientNum() != me->clientnum)
        {
            psserver->SendSystemOK(dest->GetClientNum(),
                                   "You were given %u %s by GM %s.", StackCount, item->GetName(),
                                   source->GetActor()->GetName());
        }

        // If we're taking from someone else, notify them
        if(source->GetClientNum() != me->clientnum)
        {
            psserver->SendSystemResult(source->GetClientNum(),
                                       "%u %s was taken by GM %s.", StackCount, item->GetName(),
                                       dest->GetActor()->GetName());
        }

        trgtTran.Commit();
        srcTran.Commit();
        return;
    }
}

void AdminManager::CheckItem(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata)
{
    AdminCmdDataCheckItem* data = static_cast<AdminCmdDataCheckItem*>(cmddata);
    Client* targetClient = data->targetClient;

    if(!targetClient || !targetClient->GetCharacterData())
    {
        psserver->SendSystemError(me->clientnum, "Invalid character to check");
        return;
    }

    if(data->stackCount == 0 || data->itemName.IsEmpty())
    {
        psserver->SendSystemError(me->clientnum, data->GetHelpMessage());
        return;
    }

    psItemStats* itemstats = psserver->GetCacheManager()->GetBasicItemStatsByName(data->itemName);
    if(!itemstats)
    {
        psserver->SendSystemError(me->clientnum, "Invalid item name");
        return;
    }

    psCharacter* targetchar = targetClient->GetCharacterData();
    size_t itemIndex = targetchar->Inventory().FindItemStatIndex(itemstats);

    if(itemIndex != SIZET_NOT_FOUND)
    {
        psItem* item;
        item = targetchar->Inventory().GetInventoryIndexItem(itemIndex);
        if(!item)
        {
            Error2("Cannot GetInventoryIndexItem on itemIndex %zu.\n", itemIndex);
            psserver->SendSystemError(me->clientnum, "Cannot check %s from %s's inventory.",
                                      data->itemName.GetData(), targetClient->GetActor()->GetName());
            return;
        }

        if(item->GetStackCount() < data->stackCount)
        {
            psserver->SendSystemOK(me->clientnum, "Cannot find %d %s in %s.", data->stackCount, data->itemName.GetData(), targetClient->GetActor()->GetName());
        }
        else
        {
            psserver->SendSystemOK(me->clientnum, "Found %d %s in %s.", data->stackCount, data->itemName.GetData(), targetClient->GetActor()->GetName());
        }
        return;
    }
    else if(data->itemName == "tria")
    {
        psMoney targetMoney = targetchar->Money();
        if(data->stackCount > targetMoney.GetTotal())
        {
            psserver->SendSystemOK(me->clientnum, "Cannot find %d tria in %s.", data->stackCount, targetClient->GetActor()->GetName());
        }
        else
        {
            psserver->SendSystemOK(me->clientnum, "Found %d tria in %s.", data->stackCount, targetClient->GetActor()->GetName());
        }
        return;
    }
    else
    {
        psserver->SendSystemOK(me->clientnum, "Cannot find %s in %s.", data->itemName.GetData(), targetClient->GetActor()->GetName());
        return;
    }
}

void AdminManager::FreezeClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if(!data->targetClient)
    {
        psserver->SendSystemError(me->clientnum,"Invalid target for freeze");
        return;
    }

    if(data->targetClient->GetActor()->IsFrozen())
    {
        psserver->SendSystemError(me->clientnum,"The player is already frozen");
        return;
    }

    data->targetClient->GetActor()->SetAllowedToMove(false);
    data->targetClient->GetActor()->SetFrozen(true);
    data->targetClient->GetActor()->SetMode(PSCHARACTER_MODE_SIT);
    psserver->SendSystemError(data->targetClient->GetClientNum(), "You have been frozen in place by a GM.");
    psserver->SendSystemInfo(me->clientnum, "You froze '%s'.",(const char*)data->target);
}

void AdminManager::ThawClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);
    if(!data->targetClient)
    {
        psserver->SendSystemError(me->clientnum,"Invalid target for thaw");
        return;
    }

    if(!data->targetClient->GetActor()->IsFrozen())
    {
        psserver->SendSystemError(me->clientnum,"The player is not frozen");
        return;
    }

    data->targetClient->GetActor()->SetAllowedToMove(true);
    data->targetClient->GetActor()->SetFrozen(false);
    data->targetClient->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
    psserver->SendSystemOK(data->targetClient->GetClientNum(), "You have been released by a GM.");
    psserver->SendSystemInfo(me->clientnum, "You released '%s'.",(const char*)data->target);
}

void AdminManager::SetSkill(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSetSkill* data = static_cast<AdminCmdDataSetSkill*>(cmddata);

    if(!data->targetActor || (data->targetActor->GetClient() != client && !psserver->CheckAccess(client, "setskill others")))
    {
        psserver->SendSystemError(me->clientnum, "You have to specify a valid target");
        return;
    }

    if(data->subCommand != "copy")
    {
        if(data->skillData.skillName == "all")
        {
            for(size_t i = 0; i < psserver->GetCacheManager()->GetSkillAmount(); i++)
            {
                psSkillInfo* skill = psserver->GetCacheManager()->GetSkillByID(i);
                if(skill == NULL) continue;

                psRewardDataSkill rewardData(skill->name,data->skillData.skillDelta,0,data->skillData.relativeSkill);
                AwardToTarget(me->clientnum, data->targetActor->GetClient(), &rewardData);
            }
        }
        else
        {
            psRewardDataSkill rewardData(data->skillData.skillName,data->skillData.skillDelta,0,data->skillData.relativeSkill);
            AwardToTarget(me->clientnum, data->targetActor->GetClient(), &rewardData);
        }
        return;
    }

    gemActor* source = NULL;
    psCharacter* schar = NULL;

    if(data->sourcePlayer.targetClient)
    {
        source = data->sourcePlayer.targetActor;
    }

    if(source == NULL)
    {
        psserver->SendSystemError(me->clientnum, "Invalid skill source");
        return;
    }
    else
    {
        schar = source->GetCharacterData();
        if(!schar)
        {
            psserver->SendSystemError(me->clientnum, "No source character data!");
            return;
        }
    }

    psCharacter* pchar = data->targetActor->GetCharacterData();
    if(!pchar)
    {
        psserver->SendSystemError(me->clientnum, "No character data!");
        return;
    }

    if(pchar->IsNPC())
    {
        psserver->SendSystemError(me->clientnum, "You can't use this command on npcs!");
        return;
    }

    bool modified = false;
    for(size_t i = 0; i < psserver->GetCacheManager()->GetSkillAmount(); i++)
    {
        psSkillInfo* skill = psserver->GetCacheManager()->GetSkillByID(i);
        if(skill == NULL) continue;
        modified |= ApplySkill(me->clientnum, data->targetActor->GetClient(), skill, schar->Skills().GetSkillRank(skill->id).Current());
    }

    // Send updated skill list to client
    if(modified && data->targetActor->GetClient())
        psserver->GetProgressionManager()->SendSkillList(data->targetActor->GetClient(), false);
}

bool AdminManager::ApplySkill(int client, Client* target, psSkillInfo* skill, int value, bool relative, int cap)
{
    // perform sanity checks
    if(!skill)
    {
        psserver->SendSystemError(client, "Invalid Skill");
        return false;
    }

    if(!target || !client)  // no target or issuer
        return false;

    psCharacter* pchar = target->GetCharacterData();

    if(!pchar)  // target is no player
        return false;

    if(relative && !value)  // +0 or -0: show current status and return
    {
        int base = pchar->Skills().GetSkillRank(skill->id).Base();
        int current = pchar->Skills().GetSkillRank(skill->id).Current();

        // notify issuer
        if(base == current)
            psserver->SendSystemInfo(client, "Current '%s' of '%s' is %d", skill->name.GetDataSafe(), target->GetName(), base);
        else
            psserver->SendSystemInfo(client, "Current '%s' of '%s' is %d (%d)", skill->name.GetDataSafe(), target->GetName(), base, current);

        return false;
    }
    else // modify skill
    {
        int old_value = pchar->Skills().GetSkillRank(skill->id).Current(); // backup current value
        int new_value = value;
        int max = SKILL_MAX_RANK;

        if(relative)
            new_value += old_value;

        if(cap && cap < max)  // check whether the issuer specified a maximum
            max = cap;

        // adjust the value to be between 0 and the maximum if needed
        if(new_value > max)
            new_value = max;
        else if(new_value < 0)
            new_value = 0;

        pchar->SetSkillRank(skill->id, new_value);

        // notify issuer
        csString text, info;
        info.Format("%s skill", skill->name.GetDataSafe());
        text.Format("'%s' skill going from %d to %d", skill->name.GetDataSafe(),old_value, new_value);
        SendAwardInfo(client, target, info.GetData(), text.GetData(), new_value-old_value);
    }
    return true;
}

void AdminManager::UpdateRespawn(AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataUpdateRespawn* data = static_cast<AdminCmdDataUpdateRespawn*>(cmddata);
    if(!data->targetActor)
    {
        psserver->SendSystemError(client->GetClientNum(),"You need to specify or target a player or NPC");
        return;
    }

    if(!data->targetActor->GetCharacterData())
    {
        psserver->SendSystemError(client->GetClientNum(),"Critical error! The entity hasn't got any character data!");
        return;
    }

    if(!data->valid)
    {
        psserver->SendSystemError(client->GetClientNum(),data->GetHelpMessage());
        return;
    }

    csVector3 pos;
    float yrot;
    iSector* sec;
    InstanceID instance;

    // Update respawn to the NPC's current position or your current position?
    if(!data->place.IsEmpty() && data->place.CompareNoCase("here"))
    {
        client->GetActor()->GetPosition(pos, yrot, sec);
        instance = client->GetActor()->GetInstance();
    }
    else
    {
        data->targetActor->GetPosition(pos, yrot, sec);
        instance = data->targetActor->GetInstance();
    }

    csString sector = sec->QueryObject()->GetName();

    psSectorInfo* sectorinfo = psserver->GetCacheManager()->GetSectorInfoByName(sector);

    data->targetActor->GetCharacterData()->UpdateRespawn(pos, yrot, sectorinfo, instance);

    csString buffer;
    buffer.Format("%s now respawning (%.2f,%.2f,%.2f) <%s> in instance %u", data->targetActor->GetName(), pos.x, pos.y, pos.z, sector.GetData(), instance);
    psserver->SendSystemOK(client->GetClientNum(), buffer);
}


void AdminManager::Inspect(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);
    if(!data->targetActor)
    {
        psserver->SendSystemError(me->clientnum,"You need to specify or target a player or NPC");
        return;
    }

    if(!data->targetActor->GetCharacterData())
    {
        psserver->SendSystemError(me->clientnum,"Critical error! The entity hasn't got any character data!");
        return;
    }

    // We got our target, now let's print it's inventory
    csString message; //stores the formatted item data
    bool npc = (data->targetActor->GetClientID() == 0);

    //sends the heading
    psserver->SendSystemInfo(me->clientnum,"Inventory for %s %s:\nTotal weight is %d / %d\nTotal money is %d",
                             npc?"NPC":"player", data->targetActor->GetName(),
                             (int)data->targetActor->GetCharacterData()->Inventory().GetCurrentTotalWeight(),
                             (int)data->targetActor->GetCharacterData()->Inventory().MaxWeight(),
                             data->targetActor->GetCharacterData()->Money().GetTotal());

    bool found = false;
    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for(size_t i = 1; i < data->targetActor->GetCharacterData()->Inventory().GetInventoryIndexCount(); i++)
    {
        psItem* item = data->targetActor->GetCharacterData()->Inventory().GetInventoryIndexItem(i);
        if(item)
        {
            found = true;
            message = item->GetName();
            message.AppendFmt(" (%d/%d)", (int)item->GetItemQuality(), (int)item->GetMaxItemQuality());
            if(item->GetStackCount() > 1)
                message.AppendFmt(" (x%u)", item->GetStackCount());

            message.Append(" - ");

            const char* slotname = psserver->GetCacheManager()->slotNameHash.GetName(item->GetLocInParent());
            if(slotname)
                message.Append(slotname);
            else
                message.AppendFmt("Bulk %d", item->GetLocInParent(true));

            psserver->SendSystemInfo(me->clientnum,message); //sends one line per item.
            //so we avoid to go over the packet limit
        }
    }
    if(!found)
        psserver->SendSystemInfo(me->clientnum,"(none)");
}

void AdminManager::RenameGuild(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataChangeGuildName* data = static_cast<AdminCmdDataChangeGuildName*>(cmddata);
    psGuildInfo* guild = psserver->GetCacheManager()->FindGuild(data->guildName);
    if(!guild)
    {
        psserver->SendSystemError(me->clientnum,"No guild with the name: %s",
                                  data->guildName.GetData());
        return;
    }

    guild->SetName(data->newName);
    psserver->GetGuildManager()->ResendGuildData(guild->GetID());

    // Notify the guild leader if he is online
    psGuildMember* gleader = guild->FindLeader();
    if(gleader)
    {
        if(gleader->character && gleader->character->GetActor())
        {
            psserver->SendSystemInfo(gleader->character->GetActor()->GetClientID(),
                                     "Your guild has been renamed to %s by a GM",
                                     data->newName.GetData()
                                    );
        }
    }

    psserver->SendSystemOK(me->clientnum,"Guild renamed to '%s'",data->newName.GetData());

    // Get all connected guild members
    csArray<EID> array;

    csArray<psGuildMember*>::Iterator mIter = guild->GetMemberIterator();
    while(mIter.HasNext())
    {
        psGuildMember* member = mIter.Next();
        if(member->character && member->character->GetActor())
        {
            array.Push(member->character->GetActor()->GetEID());
        }
    }

    // Update the labels
    int length = (int)array.GetSize();
    psUpdatePlayerGuildMessage newNameMsg(0, length, data->newName);

    // Copy array
    for(size_t i = 0; i < array.GetSize(); i++)
    {
        newNameMsg.AddPlayer(array[i]);
    }

    // Broadcast to everyone
    psserver->GetEventManager()->Broadcast(newNameMsg.msg,NetBase::BC_EVERYONE);
}

void AdminManager::ChangeGuildLeader(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataChangeGuildLeader* data = static_cast<AdminCmdDataChangeGuildLeader*>(cmddata);
    psGuildInfo* guild = psserver->GetCacheManager()->FindGuild(data->guildName);
    if(!guild)
    {
        psserver->SendSystemError(me->clientnum, "No guild with the name: %s.", data->guildName.GetDataSafe());
        return;
    }

    psGuildMember* member = guild->FindMember(data->target.GetData());
    if(!member)
    {
        psserver->SendSystemError(me->clientnum, "Can't find member %s.", data->target.GetData());
        return;
    }

    // Is the leader the target player?
    psGuildMember* gleader = guild->FindLeader();
    if(member == gleader)
    {
        psserver->SendSystemError(me->clientnum, "%s is already the guild leader.", data->target.GetData());
        return;
    }

    // Change the leader
    // Promote player to leader
    if(!guild->UpdateMemberLevel(member, MAX_GUILD_LEVEL))
    {
        psserver->SendSystemError(me->clientnum, "SQL Error: %s", db->GetLastError());
        return;
    }
    // Demote old leader
    if(!guild->UpdateMemberLevel(gleader, MAX_GUILD_LEVEL - 1))
    {
        psserver->SendSystemError(me->clientnum, "SQL Error: %s", db->GetLastError());
        return;
    }

    psserver->GetGuildManager()->ResendGuildData(guild->GetID());

    psserver->SendSystemOK(me->clientnum,"Guild leader changed to '%s'.", data->target.GetData());

    csString text;
    text.Format("%s has been promoted to '%s' by a GM.", data->target.GetData(), member->guildlevel->title.GetData());
    psChatMessage guildmsg(me->clientnum,0,"System",0,text,CHAT_GUILD, false);
    if(guildmsg.valid)
        psserver->GetChatManager()->SendGuild("server", 0, guild, guildmsg);
}

void AdminManager::Thunder(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSectorTarget* data = static_cast<AdminCmdDataSectorTarget*>(cmddata);

    if(!data->sectorInfo->is_raining)
    {
        psserver->SendSystemError(me->clientnum, "You cannot create a lightning "
                                  "if no rain or rain is fading out!");
        return;
    }

    // Queue thunder
    psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::LIGHTNING, 0, 0, 0,
            data->sectorInfo->name,
            data->sectorInfo,
            client->GetClientNum());

}

void AdminManager::Fog(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataFog* data = static_cast<AdminCmdDataFog*>(cmddata);

    // Queue fog

    // if fog disabled
    if(!data->enabled)
    {
        if(!data->sectorInfo->fog_density)
        {
            psserver->SendSystemInfo(me->clientnum, "You need to have fog in this sector for turning it off.");
            return;
        }
        psserver->SendSystemInfo(me->clientnum, "You have turned off the fog.");
        // Reset fog
        psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::FOG, 0, 0, 0,
                data->sectorInfo->name, data->sectorInfo,0,0,0,0);
    }
    // enable fog
    else
    {
        // Set fog
        psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::FOG,
                data->density, data->interval, data->fadeTime,
                data->sectorInfo->name, data->sectorInfo,0,
                data->r,data->g,data->b); //rgb
    }

}

void AdminManager::Weather(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataWeather* data = static_cast<AdminCmdDataWeather*>(cmddata);

    //check if the requested status is already set for this weather type
    //the same code enables and disables weather
    if(data->sectorInfo->GetWeatherEnabled(data->type) != data->enabled)
    {
        //try to set the requested status for this weather type
        data->sectorInfo->SetWeatherEnabled(data->type, data->enabled);

        //check if the activation was successful (aka the data is valid)
        if(data->sectorInfo->GetWeatherEnabled(data->type) == data->enabled)
        {
            //as there are two separate function call the appropriate one to enable or disable this type of weather
            if(data->enabled)
                psserver->GetWeatherManager()->StartWeather(data->sectorInfo, data->type);
            else
                psserver->GetWeatherManager()->StopWeather(data->sectorInfo, data->type);

            //notify the success of the operation
            psserver->SendSystemInfo(me->clientnum,"Automatic weather %s in sector %s", data->enabled ? "started" : "stopped", data->sectorName.GetDataSafe());
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum,"Automatic weather cannot be %s in sector %s because data is missing",
                                     data->enabled ? "started" : "stopped", data->sectorName.GetDataSafe());
        }
    }
    else
    {
        psserver->SendSystemInfo(me->clientnum,"The weather is already automatic in sector %s", data->enabled ? "automatic" : "off", data->sectorName.GetDataSafe());
    }
}

void AdminManager::Rain(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataWeatherEffect* data = static_cast<AdminCmdDataWeatherEffect*>(cmddata);

    if(data->particleCount < 0 || data->particleCount > WEATHER_MAX_RAIN_DROPS)
    {
        psserver->SendSystemError(me->clientnum, "Rain drops should be between %d and %d",
                                  0,WEATHER_MAX_RAIN_DROPS);
        return;
    }

    // Stop raining
    if(!data->enabled || (data->interval == 0 && data->particleCount == 0))
    {
        if(!data->sectorInfo->is_raining)  //If it is not raining already then you don't stop anything.
        {
            psserver->SendSystemInfo(me->clientnum, "You need some weather, first.");
            return;
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "The weather was stopped.");

            // queue the event
            psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::RAIN,
                    0, 0,
                    data->fadeTime, data->sectorName, data->sectorInfo);
        }
    }
    else
    {
        // queue the event
        psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::RAIN,
                data->particleCount, data->interval,
                data->fadeTime, data->sectorName, data->sectorInfo);
    }
}

void AdminManager::Snow(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataWeatherEffect* data = static_cast<AdminCmdDataWeatherEffect*>(cmddata);

    if(data->particleCount < 0 || data->particleCount > WEATHER_MAX_SNOW_FALKES)
    {
        psserver->SendSystemError(me->clientnum, "Snow flakes should be between %d and %d",
                                  0,WEATHER_MAX_SNOW_FALKES);
        return;
    }

    // Stop snowing
    if(!data->enabled || (data->interval == 0 && data->fadeTime == 0))
    {
        if(!data->sectorInfo->is_snowing)  //If it is not snowing already then you don't stop anything.
        {
            psserver->SendSystemInfo(me->clientnum, "You need some snow, first.");
            return;
        }
        else
        {
            psserver->SendSystemInfo(me->clientnum, "The snow was stopped.");

            // queue the event
            psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::SNOW,
                    0, 0,
                    data->fadeTime, data->sectorName, data->sectorInfo);
        }
    }
    else
    {
        // queue the event
        psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::SNOW, data->particleCount,
                data->interval, data->fadeTime, data->sectorName, data->sectorInfo);
    }
}


void AdminManager::ModifyItem(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataModify* data = static_cast<AdminCmdDataModify*>(cmddata);

    if(!data->targetObject)
    {
        psserver->SendSystemError(me->clientnum,"You need to specify an item in the world with 'target' or 'eid:#'");
        return;
    }

    psItem* item = data->targetObject->GetItem();
    if(!item)
    {
        psserver->SendSystemError(me->clientnum,"You can only use modify on items");
        return;
    }

    bool fullModify = psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "full modify");


    // TODO: Update Sibling spawn points
    if(data->subCommand == "remove" && fullModify)
    {
        if(item->GetScheduledItem())
        {
            item->GetScheduledItem()->Remove();
            psserver->SendSystemInfo(me->clientnum,"Spawn point deleted for %s",item->GetName());
        }

        EntityManager::GetSingleton().RemoveActor(data->targetObject); // Remove from world
        psserver->SendSystemInfo(me->clientnum,"%s was removed from the world",item->GetName());
        item->Destroy(); // Remove from db
        delete item;
        item = NULL;
    }
    else if(data->subCommand == "intervals" && fullModify)
    {
        if(data->interval < 0 || data->maxinterval < 0)
        {
            psserver->SendSystemError(me->clientnum,"Invalid intervals specified");
            return;
        }

        // In seconds
        int interval = 1000*data->interval;
        int random   = 1000*data->maxinterval;

        if(item->GetScheduledItem())
        {
            item->GetScheduledItem()->ChangeIntervals(interval,random);
            psserver->SendSystemInfo(me->clientnum,"Intervals for %s set to %d base + %d max modifier",item->GetName(),data->interval,data->maxinterval);
        }
        else
            psserver->SendSystemError(me->clientnum,"This item does not spawn; no intervals");
    }
    else if(data->subCommand == "amount" && fullModify)
    {
        if(data->amount < 1)
        {
            psserver->SendSystemError(me->clientnum, "Amount must be greater than 0");
            return;
        }

        if(item->GetScheduledItem())
        {
            item->GetScheduledItem()->ChangeAmount(data->amount);
            psserver->SendSystemInfo(me->clientnum,"Amount of spawns for %s set to %d. This will require a restart to take effect.",item->GetName(),data->amount);
        }
        else
            psserver->SendSystemError(me->clientnum,"This item does not spawn; no amount");
    }
    else if(data->subCommand == "range" && fullModify)
    {
        if(data->range < 0)
        {
            psserver->SendSystemError(me->clientnum, "Range must be equal to or greater than 0");
            return;
        }

        if(item->GetScheduledItem())
        {
            item->GetScheduledItem()->ChangeRange(data->range);
            psserver->SendSystemInfo(me->clientnum,"Range of spawns for %s set to %f. This will require a restart to take effect.",item->GetName(),data->range);
        }
        else
            psserver->SendSystemError(me->clientnum,"This item does not spawn; no range");
    }
    else if(data->subCommand == "move" && fullModify)
    {
        gemItem* gItem = dynamic_cast<gemItem*>(data->targetObject);
        if(gItem)
        {
            InstanceID instance = data->targetObject->GetInstance();
            iSector* sector = data->targetObject->GetSector();

            csVector3 pos(data->x, data->y, data->z);
            gItem->SetPosition(pos, data->rot, sector, instance);
        }
    }
    else if(data->subCommand == "pickskill" && fullModify)  //sets the required skill in order to be able to pick the item
    {
        if(data->skillName != "none")  //if the skill name isn't none...
        {
            psSkillInfo* skill = psserver->GetCacheManager()->GetSkillByName(data->skillName);  //try searching for the skill
            if(skill)  //if found...
            {
                item->SetLockpickSkill(skill->id); //set the selected skill for picking the lock to the item
                psserver->SendSystemInfo(me->clientnum,"The skill needed to open the lock of %s is now %s",item->GetName(),skill->name.GetDataSafe());
            }
            else //else alert the user that there isn't such a skill
            {
                psserver->SendSystemError(me->clientnum,"Invalid skill name!");
            }
        }
        else //if the skill is defined as none
        {
            item->SetLockStrength(0); //we reset the skill level required to zero
            item->SetLockpickSkill(PSSKILL_NONE); //and reset the required skill to none
            psserver->SendSystemInfo(me->clientnum,"The skill needed to open the lock of %s was removed",item->GetName());
        }
    }
    else if(data->subCommand == "picklevel" && fullModify)  //sets the required level of the already selected skill in order to be able to pick the item
    {
        if(data->level >= 0)  //check that we didn't get a negative value
        {
            if(item->GetLockpickSkill() != PSSKILL_NONE)  //check that the skill isn't none (not set)
            {
                item->SetLockStrength(data->level); //all went fine so set the skill level required to pick this item
                psserver->SendSystemInfo(me->clientnum,"The skill level needed to open the lock of %s is now %u",item->GetName(),data->level);
            }
            else //alert the user that the item doesn't have a skill for picking it
            {
                psserver->SendSystemError(me->clientnum,"The item doesn't have a pick skill set!");
            }
        }
        else //alert the user that the supplied value isn't valid
        {
            psserver->SendSystemError(me->clientnum,"Invalid skill level!");
        }
    }
    else
    {
        if(data->subCommand == "pickupable" && fullModify)
        {
            item->SetIsPickupable(data->enabled);
            psserver->SendSystemInfo(me->clientnum,"%s is now %s",item->GetName(),(data->enabled)?"pickupable":"un-pickupable");
        }
        if(data->subCommand == "pickupableweak")
        {
            item->SetIsPickupableWeak(data->enabled);
            psserver->SendSystemInfo(me->clientnum,"%s is now %s",item->GetName(),(data->enabled)?"weak pickupable":"weak un-pickupable");
        }
        else if(data->subCommand == "unpickable" && fullModify)  //sets or unsets the UNPICKABLE flag on the item
        {
            item->SetIsUnpickable(data->enabled);
            psserver->SendSystemInfo(me->clientnum,"%s is now %s",item->GetName(),(data->enabled)?"un-pickable":"pickable");
        }
        else if(data->subCommand == "transient")
        {
            item->SetIsTransient(data->enabled);
            psserver->SendSystemInfo(me->clientnum,"%s is now %s",item->GetName(),(data->enabled)?"transient":"non-transient");
        }
        else if(data->subCommand == "npcowned" && fullModify)
        {
            item->SetIsNpcOwned(data->enabled);
            psserver->SendSystemInfo(me->clientnum, "%s is now %s",
                                     item->GetName(), data->enabled ? "npc owned" : "not npc owned");
        }
        else if(data->subCommand == "collide")
        {
            item->SetIsCD(data->enabled);
            psserver->SendSystemInfo(me->clientnum, "%s is now %s",
                                     item->GetName(), data->enabled ? "using collision detection" : "not using collision detection");
            item->GetGemObject()->Send(me->clientnum, false, false);
            item->GetGemObject()->Broadcast(me->clientnum, false);
        }
        else if(data->subCommand == "settingitem" && fullModify)
        {
            item->SetIsSettingItem(data->enabled);
            psserver->SendSystemInfo(me->clientnum, "%s is now %s",
                                     item->GetName(), data->enabled ? "a setting item" : "not a setting item");
        }
        else if(!fullModify)
        {
            psserver->SendSystemInfo(me->clientnum, "This command is not available");
        }
        item->Save(false);
    }
}

#define MORPH_FAKE_ACTIVESPELL ((ActiveSpell*) 0x447)
void AdminManager::Morph(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataMorph* data = static_cast<AdminCmdDataMorph*>(cmddata);

    // lists all races you can morph into
    if(data->subCommand == "list")
    {
        static csString list;
        csStringArray raceNames;

        //construct a list coming from the race info list.
        for(size_t i = 0; i < psserver->GetCacheManager()->GetRaceInfoCount(); i++)
            raceNames.PushSmart(psserver->GetCacheManager()->GetRaceInfoByIndex(i)->GetName());

        // Make alphabetized list
        raceNames.Sort();
        list = "Available races:  ";
        for(size_t i=0; i<raceNames.GetSize(); i++)
        {
            list += raceNames[i];
            if(i < raceNames.GetSize()-1)
                list += ", ";
        }

        psserver->SendSystemInfo(me->clientnum, "%s", list.GetData());
        return;
    }

    // check if the target is valid
    if(!data->targetClient || !data->targetClient->GetActor())
    {
        psserver->SendSystemError(me->clientnum,"Invalid target for morph");
        return;
    }

    //check if there is permissions to morph other players
    if(data->targetClient != client && !psserver->CheckAccess(client, "morph others"))
    {
        psserver->SendSystemError(me->clientnum,"You don't have permission to change race of %s!", data->targetClient->GetName());
        return;
    }

    gemActor* target = data->targetClient->GetActor();

    // if the user issued a reset restore the basic race
    if(data->subCommand == "reset")
    {
        psserver->SendSystemInfo(me->clientnum, "Resetting race for %s", data->targetClient->GetName());
        target->GetCharacterData()->GetOverridableRace().Cancel(MORPH_FAKE_ACTIVESPELL);

        // unset the scale variable
        target->GetCharacterData()->UnSetVariable("scale");
    }
    else
    {
        // otherwise set the defined race by race name and gender
        psRaceInfo* race = psserver->GetCacheManager()->GetRaceInfoByNameGender(data->raceName, psserver->GetCacheManager()->ConvertGenderString(data->genderName)) ;
        if(!race)
        {
            //the race couldn't be found in this case
            psserver->SendSystemError(me->clientnum, "Unable to override race for %s to %s (%s). Race not found.", data->targetClient->GetName(),
                                      data->raceName.GetData(), data->genderName.GetData());
            return;
        }

        // set scale if specified
        if(data->scale)
            target->GetCharacterData()->SetVariable("scale",data->scale);

        // override the race using a fake spell
        psserver->SendSystemInfo(me->clientnum, "Overriding race for %s to %s", data->targetClient->GetName(), data->raceName.GetData());
        target->GetCharacterData()->GetOverridableRace().Override(MORPH_FAKE_ACTIVESPELL, race);
    }
}

void AdminManager::Scale(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataScale* data = static_cast<AdminCmdDataScale*>(cmddata);

    // check if the target is valid
    if(!data->targetClient || !data->targetClient->GetActor())
    {
        psserver->SendSystemError(me->clientnum,"Invalid target for morph");
        return;
    }

    gemActor* target = data->targetClient->GetActor();

    // if the user issued a reset restore the basic race
    if(data->subCommand == "reset")
    {
        psserver->SendSystemInfo(me->clientnum, "Resetting scale for %s", data->targetClient->GetName());
        // unset the scale variable
        target->GetCharacterData()->UnSetVariable("scale");
        target->Broadcast(me->clientnum, false);
        target->Send(me->clientnum, false, false);
    }
    else
    {
        if(data->scale)
        {
            float scale = atof(data->scale);
            if(scale < 0.01 || scale > 100.0)
            {
                psserver->SendSystemError(me->clientnum,"Scale can only be set between 0.01 and 100.0");
                return;
            }
            target->GetCharacterData()->SetVariable("scale",data->scale);
            target->Broadcast(me->clientnum, false);
            target->Send(me->clientnum, false, false);
        }

        // override the race using a fake spell
        psserver->SendSystemInfo(me->clientnum, "Overriding scale for %s to %s", data->targetClient->GetName(), data->scale.GetData());
    }
}

void AdminManager::TempSecurityLevel(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataDeputize* data = static_cast<AdminCmdDataDeputize*>(cmddata);

    if(!data->targetClient || !data->targetClient->GetActor())
    {
        psserver->SendSystemError(me->clientnum,"Invalid target");
        return;
    }

    // Can only set at maximum the same level of the one the client is
    int maxleveltoset = client->GetSecurityLevel();

    int value;

    if(data->securityLevel == "reset")
    {
        int trueSL = GetTrueSecurityLevel(data->targetClient->GetAccountID());
        if(trueSL < 0)
        {
            psserver->SendSystemError(client->GetClientNum(), "Cannot reset access level for %s!", data->targetClient->GetName());
            return;
        }

        data->targetClient->SetSecurityLevel(trueSL);
        data->targetClient->GetActor()->SetSecurityLevel(trueSL);

        // Refresh the label
        data->targetClient->GetActor()->UpdateProxList(true);

        psserver->SendSystemOK(data->targetClient->GetClientNum(),"Your access level was reset");
        if(data->targetClient != client)
            psserver->SendSystemOK(me->clientnum,"Access level for %s was reset",data->targetClient->GetName());

        if(trueSL)
            psserver->SendSystemInfo(data->targetClient->GetClientNum(),"Your access level has been reset to %d",trueSL);

        return;
    }
    else if(data->securityLevel == "player")
        value = 0;
    else if(data->securityLevel == "tester")
        value = GM_TESTER;
    else if(data->securityLevel == "gm")
        value = GM_LEVEL_1;
    else if(data->securityLevel.StartsWith("gm",true) && data->securityLevel.Length() == 3)
        value = atoi(data->securityLevel.Slice(2,1)) + GM_LEVEL_0;
    else if(data->securityLevel == "developer")
        value = GM_DEVELOPER;
    else
    {
        psserver->SendSystemError(me->clientnum,"Valid settings are:  player, tester, GM, developer or reset.  GM levels may be specified:  GM1, ... GM5");
        return;
    }

    if(!psserver->GetCacheManager()->GetCommandManager()->GroupExists(value))
    {
        psserver->SendSystemError(me->clientnum,"Specified access level does not exist!");
        return;
    }

    if(data->targetClient == client && value > GetTrueSecurityLevel(data->targetClient->GetAccountID()))
    {
        psserver->SendSystemError(me->clientnum,"You cannot upgrade your own level!");
        return;
    }
    else if(data->targetClient != client && value > maxleveltoset)
    {
        psserver->SendSystemError(me->clientnum,"Max access level you may set is %d", maxleveltoset);
        return;
    }

    if(data->targetClient->GetSecurityLevel() == value)
    {
        psserver->SendSystemError(me->clientnum,"%s is already at that access level", data->targetClient->GetName());
        return;
    }

    if(value == 0)
    {
        psserver->SendSystemInfo(data->targetClient->GetClientNum(),"Your access level has been disabled for this session.");
    }
    else  // Notify of added/removed commands
    {
        psserver->SendSystemInfo(data->targetClient->GetClientNum(),"Your access level has been changed for this session.");
    }

    if(!psserver->GetCacheManager()->GetCommandManager()->Validate(value, "/deputize"))  // Cannot access this command, but may still reset
    {
        psserver->SendSystemInfo(data->targetClient->GetClientNum(),"You may do \"/deputize me reset\" at any time to reset yourself. "
                                 " The temporary access level will also expire on logout.");
    }


    // Set temporary security level (not saved to DB)
    data->targetClient->SetSecurityLevel(value);
    data->targetClient->GetActor()->SetSecurityLevel(value);

    // Refresh the label
    data->targetClient->GetActor()->UpdateProxList(true);

    Admin(data->targetClient->GetClientNum(), data->targetClient); //enable automatically new commands no need to request /admin

    psserver->SendSystemOK(me->clientnum,"Access level for %s set to %s",data->targetClient->GetName(),data->securityLevel.GetData());
    if(data->targetClient != client)
    {
        psserver->SendSystemOK(data->targetClient->GetClientNum(),"Your access level was set to %s by a GM",data->securityLevel.GetData());
    }
}

int AdminManager::GetTrueSecurityLevel(AccountID accountID)
{
    Result result(db->Select("SELECT security_level FROM accounts WHERE id='%d'", accountID.Unbox()));

    if(!result.IsValid() || result.Count() != 1)
        return -99;
    else
        return result[0].GetUInt32("security_level");
}

void AdminManager::HandleGMEvent(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataGameMasterEvent* data = static_cast<AdminCmdDataGameMasterEvent*>(cmddata);
    GMEventManager* gmeventManager = psserver->GetGMEventManager();

    // add new event
    if(data->subCmd == "create")
    {
        gmeventManager->AddNewGMEvent(client, data->gmeventName, data->gmeventDesc);
        return;
    }

    // register player(s) with the event
    if(data->subCmd == "register")
    {
        /// this looks odd, because the range value is in the 'player' parameter.
        if(data->rangeSpecifier == IN_RANGE)
        {
            gmeventManager->RegisterPlayersInRangeInGMEvent(client, data->range);
        }
        else
        {
            gmeventManager->RegisterPlayerInGMEvent(client, data->player.targetClient);
        }
        return;
    }

    // player completed event
    if(data->subCmd == "complete")
    {
        if(data->gmeventName.IsEmpty())
        {
            gmeventManager->CompleteGMEvent(client, client->GetPID());
        }
        else
        {
            gmeventManager->CompleteGMEvent(client, data->gmeventName);
        }
        return;
    }

    //remove player
    if(data->subCmd == "remove")
    {
        gmeventManager->RemovePlayerFromGMEvent(client, data->player.targetClient);
        return;
    }

    // reward player(s)
    if(data->subCmd == "reward")
    {
        if(data->rewardList.rewards.GetSize() == 0)
        {
            psserver->SendSystemError(client->GetClientNum(), "Nothing to award.");
            return;
        }
        csPDelArray<psRewardData>::Iterator it = data->rewardList.rewards.GetIterator();
        while(it.HasNext())
        {
            gmeventManager->RewardPlayersInGMEvent(client, data->rangeSpecifier, data->range, data->player.targetClient, it.Next());
        }
        return;
    }

    if(data->subCmd == "list")
    {
        gmeventManager->ListGMEvents(client);
        return;
    }

    if(data->subCmd == "control")
    {
        gmeventManager->AssumeControlOfGMEvent(client, data->gmeventName);
        return;
    }

    if(data->subCmd == "discard")
    {
        gmeventManager->EraseGMEvent(client, data->gmeventName);
        return;
    }
}

void AdminManager::HandleHire(AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataHire* data = static_cast<AdminCmdDataHire*>(cmddata);
    HireManager* hireManager = psserver->GetHireManager();

    // add new event
    if(data->subCmd == "start")
    {
        if(hireManager->StartHire(data->owner))
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire ready to be configured.");
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire can not be started.");
        }
    }
    else if(data->subCmd == "type")
    {
        if(hireManager->SetHireType(data->owner, data->typeName, data->typeNPCType))
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire npc name set to %s and type to %s.",
                                      data->typeName.GetDataSafe(), data->typeNPCType.GetDataSafe());
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire setting of hire type failed.");
        }
        
    }
    else if(data->subCmd == "master")
    {
        if(hireManager->SetHireMasterPID(data->owner, data->masterPID))
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire master NPC set to %s.",
                                      ShowID(data->masterPID));
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire setting of hire master NPC failed.");
        }
    }
    else if(data->subCmd == "confirm")
    {
        if(hireManager->ConfirmHire(data->owner) != NULL)
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire confirmed.");
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire not confirmed.");
        }
        
    }
    else if(data->subCmd == "script")
    {
        if(hireManager->HandleScriptMessageRequest(client->GetClientNum(), data->owner, data->hiredNPC))
        {
            psserver->SendSystemError(client->GetClientNum(), "Started scripting.");
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(), "No hire to script for this NPC.");
        }
        
    }
    else if(data->subCmd == "release")
    {
        if(hireManager->ReleaseHire(data->owner, data->hiredNPC))
        {
            psserver->SendSystemError(client->GetClientNum(), "Hire released.");
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(), "Failed to release hire.");
        }
        
    }
    else
    {
        Error2("Unknown hire subcommand: %s",data->subCmd.GetDataSafe());
    }
}


void AdminManager::HandleBadText(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataBadText* data = static_cast<AdminCmdDataBadText*>(cmddata);

    if(!data->targetObject)
    {
        psserver->SendSystemError(client->GetClientNum(), "You must select an npc first.");
        return;
    }
    gemNPC* npc = data->targetObject->GetNPCPtr();
    if(!npc)
    {
        psserver->SendSystemError(client->GetClientNum(), "You must select an npc first.");
        return;
    }

    csStringArray saidArray;
    csStringArray trigArray;

    npc->GetBadText(data->first, data->last, saidArray, trigArray);
    psserver->SendSystemInfo(client->GetClientNum(), "Bad Text for %s", npc->GetName());
    psserver->SendSystemInfo(client->GetClientNum(), "--------------------------------------");

    for(size_t i=0; i<saidArray.GetSize(); i++)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "%s -> %s", saidArray[i], trigArray[i]);
    }
    psserver->SendSystemInfo(client->GetClientNum(), "--------------------------------------");
}

void AdminManager::HandleQuest(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataQuest* data = static_cast<AdminCmdDataQuest*>(cmddata);
    if(!data)
    {
        Error1("Failed to find AdminCmdDataQuest");
        return;
    }
    
    Client* subject = data->targetClient;

    Client* target = NULL; //holds the target of our query
    PID pid;               //used to keep player pid used *only* in offline queries
    csString name;         //stores the char name
    if(!data->IsOnline())  //the target was empty check if it was because it's a command targetting the issuer or an offline player
    {
        pid = data->targetID;
        name = data->target;

        if(!pid.IsValid())  //is the pid valid? (not zero)
        {
            psserver->SendSystemError(me->clientnum,"Error, bad PID");
            return;
        }
    }
    else
    {
        if(!subject)
        {
            psserver->SendSystemError(me->clientnum,"No target client found.");
            return;
        }
        target = subject; //all's normal just get the target
        name = target->GetName(); //get the name of the target for use later
    }

    // Security levels involved:
    // "/quest"              gives access to list and change your own quests.
    // "quest list others"   gives access to list other players' quests.
    // "quest change others" gives access to complete/discard other players' quests.
    const bool listOthers = psserver->CheckAccess(client, "quest list others", false);
    const bool changeOthers = psserver->CheckAccess(client, "quest change others", false);
    const bool listVariables = psserver->CheckAccess(client, "variables list", false);
    const bool modifyVariables = psserver->CheckAccess(client, "variables modify", false);

    if(data->subCmd == "complete")
    {
        if(target != client && !changeOthers)
        {
            psserver->SendSystemError(client->GetClientNum(), "You don't have permission to complete other players' quests.");
            return;
        }

        psQuest* quest = psserver->GetCacheManager()->GetQuestByName(data->questName);
        if(!quest)
        {
            psserver->SendSystemError(me->clientnum, "Quest not found for %s", name.GetData());
            return;
        }

        if(data->IsOnline())
        {
            target->GetActor()->GetCharacterData()->GetQuestMgr().AssignQuest(quest, 0);
            if(target->GetActor()->GetCharacterData()->GetQuestMgr().CompleteQuest(quest))
            {
                psserver->SendSystemInfo(me->clientnum, "Quest %s completed for %s!", data->questName.GetData(), name.GetData());
            }
        }
        else
        {
            if(!quest->GetParentQuest())  //only allow to complete main quest entries (no steps)
            {
                int result = db->CommandPump("insert into character_quests "
                                   "(player_id, assigner_id, quest_id, "
                                   "status, remaininglockout, last_response, last_response_npc_id) "
                                   "values (%d, %d, %d, '%c', %d, %d, %d) "
                                   "ON DUPLICATE KEY UPDATE "
                                   "status='%c',remaininglockout=%ld,last_response=%ld,last_response_npc_id=%ld;",
                                   pid.Unbox(), 0, quest->GetID(), 'C', 0, -1, 0, 'C', 0, -1, 0);
                if(result > 0)
                {
                    psserver->SendSystemInfo(me->clientnum, "Quest %s completed for %s!", data->questName.GetData(), name.GetData());
                }
                else
                {
                    psserver->SendSystemError(me->clientnum,"Unable to complete quest of offline player %s!", name.GetData());
                }
            }
            else
            {
                psserver->SendSystemError(me->clientnum,"Unable to complete substeps: player %s is offline!", name.GetData());
            }
        }
    }
    else if(data->subCmd == "discard")
    {
        if(target != client && !changeOthers)
        {
            psserver->SendSystemError(client->GetClientNum(), "You don't have permission to discard other players' quests.");
            return;
        }

        psQuest* quest = psserver->GetCacheManager()->GetQuestByName(data->questName);
        if(!quest)
        {
            psserver->SendSystemError(me->clientnum, "Quest not found for %s!", name.GetData());
            return;
        }

        if(data->IsOnline())  //the player is online so we don't need to hit the database
        {

            QuestAssignment* questassignment = target->GetActor()->GetCharacterData()->GetQuestMgr().IsQuestAssigned(quest->GetID());
            if(!questassignment)
            {
                psserver->SendSystemError(me->clientnum, "Quest was never started for %s!", name.GetData());
                return;
            }
            target->GetActor()->GetCharacterData()->GetQuestMgr().DiscardQuest(questassignment, true);
            psserver->SendSystemInfo(me->clientnum, "Quest %s discarded for %s!", data->questName.GetData(), name.GetData());
        }
        else //the player is offline so we have to hit the database
        {
            int result = db->CommandPump("DELETE FROM character_quests WHERE player_id=%u AND quest_id=%u",pid.Unbox(), quest->GetID());
            if(result > 0)
            {
                psserver->SendSystemInfo(me->clientnum, "Quest %s discarded for %s!", data->questName.GetData(), name.GetData());
            }
            else
            {
                psserver->SendSystemError(me->clientnum, "Quest was never started for %s!", name.GetData());
            }
        }
    }
    else if(data->subCmd == "assign")  //this command will assign the quest to the player
    {
        psQuest* quest = psserver->GetCacheManager()->GetQuestByName(data->questName); //searches for the required quest
        if(!quest)  //if not found send an error
        {
            psserver->SendSystemError(me->clientnum, "Quest not found for %s", name.GetData());
            return;
        }

        if(data->IsOnline())  //check if the player is online
        {
            if(target->GetActor()->GetCharacterData()->GetQuestMgr().AssignQuest(quest, 0))  //assign the quest to him
            {
                psserver->SendSystemInfo(me->clientnum, "Quest %s assigned to %s!", data->questName.GetData(), name.GetData());
            }
        }
        else //TODO: add offline support?
        {
            psserver->SendSystemError(me->clientnum,"Unable to assign quests: player %s is offline!", name.GetData());
        }
    }
    else if(data->subCmd == "list variables")  //this command will list the quest variables for the player
    {
        if(!listVariables && !modifyVariables)
        {
            psserver->SendSystemError(client->GetClientNum(), "You don't have permission to list variables.");
            return;
        }

        if(data->IsOnline())  //check if the player is online
        {
            psCharacter* chardata = target->GetActor()->GetCharacterData();
            if(chardata)
            {
                csHash<charVariable, csString>::ConstGlobalIterator iter = chardata->GetVariables();
                csString result;
                while(iter.HasNext())
                {
                    charVariable var = iter.Next();
                    result.AppendFmt("%s%10s %10s",result.Length()?"\n":"",var.name.GetDataSafe(),var.value.GetDataSafe());
                }
                if(!result.Length())
                {
                    result = "No variables defined.";
                }
                psserver->SendSystemInfo(me->clientnum, "Variables:\n%s",result.GetDataSafe());
            }
        }
        else
        {
            psserver->SendSystemError(me->clientnum,"Target is not online!");
        }

    }
    else if(data->subCmd == "setvariable")  //this command will set a variable for the player
    {
        if(!modifyVariables)
        {
            psserver->SendSystemError(client->GetClientNum(), "You don't have permission to modify variables.");
            return;
        }

        if(data->IsOnline())  //check if the player is online
        {
            psCharacter* chardata = target->GetActor()->GetCharacterData();
            if(chardata)
            {
                chardata->SetVariable(data->questName);
                psserver->SendSystemInfo(me->clientnum, "Variable set: %s",data->questName.GetDataSafe());
            }
        }
        else
        {
            psserver->SendSystemError(me->clientnum,"Target is not online!");
        }
    }
    else if(data->subCmd == "unsetvariable")  //this command will unset a variable for the player
    {
        if(!modifyVariables)
        {
            psserver->SendSystemError(client->GetClientNum(), "You don't have permission to modify variables.");
            return;
        }

        if(data->IsOnline())  //check if the player is online
        {
            psCharacter* chardata = target->GetActor()->GetCharacterData();
            if(chardata)
            {
                chardata->UnSetVariable(data->questName);
                psserver->SendSystemInfo(me->clientnum, "Variable unset: %s",data->questName.GetDataSafe());
            }
        }
        else
        {
            psserver->SendSystemError(me->clientnum,"Target is not online!");
        }
    }
    else // assume "list" (even if it isn't)
    {
        if(target != client && !listOthers) //the first part will evaluate as true if offline which is fine for us
        {
            psserver->SendSystemError(client->GetClientNum(), "You don't have permission to list other players' quests.");
            return;
        }

        psserver->SendSystemInfo(me->clientnum, "Quest list of %s!", name.GetData());

        if(data->IsOnline())  //our target is online
        {


            csArray<QuestAssignment*> &quests = target->GetCharacterData()->GetQuestMgr().GetAssignedQuests();
            for(size_t i = 0; i < quests.GetSize(); i++)
            {
                QuestAssignment* currassignment = quests.Get(i);
                csString QuestName = currassignment->GetQuest()->GetName();
                if(!data->questName.Length() || QuestName.StartsWith(data->questName,true))  //check if we are searching a particular quest
                    psserver->SendSystemInfo(me->clientnum, "Quest name: %s. Status: %c", QuestName.GetData(), currassignment->status);
            }
        }
        else //our target is offline access the db then...
        {
            //get the quest list from the player and their status
            Result result(db->Select("SELECT quest_id, status FROM character_quests WHERE player_id=%u",pid.Unbox()));
            if(result.IsValid())  //we got a good result
            {
                for(uint currResult = 0; currResult < result.Count(); currResult++)  //iterate the results and output info about the quest
                {
                    //get the quest data from the cache so we can print it's name without accessing the db again
                    psQuest* currQuest = psserver->GetCacheManager()->GetQuestByID(result[currResult].GetUInt32("quest_id"));
                    //check if the quest is valid else show it's id
                    csString QuestName = currQuest ? csString(currQuest->GetName()) : csString("Missing quest: ") + result[currResult]["quest_id"];
                    if(!data->questName.Length() || QuestName.StartsWith(data->questName,true))  //check if we are searching a particular quest
                        psserver->SendSystemInfo(me->clientnum, "Quest name: %s. Status: %s", QuestName.GetData(), result[currResult]["status"]);
                }
            }
        }
    }
}

void AdminManager::ItemStackable(MsgEntry* me, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSetStackable* data = static_cast<AdminCmdDataSetStackable*>(cmddata);

    if(!data->targetObject)
    {
        psserver->SendSystemError(client->GetClientNum(), "No target selected");
        return;
    }

    psItem* item = data->targetObject->GetItem();
    if(!item)
    {
        psserver->SendSystemError(client->GetClientNum(), "Not an item");
        return;
    }
    if(data->stackableAction == "info")
    {
        if(item->GetIsStackable())
        {
            psserver->SendSystemInfo(client->GetClientNum(), "This item is currently stackable");
            return;
        }
        psserver->SendSystemInfo(client->GetClientNum(), "This item is currently unstackable");
        return;
    }
    else if(data->stackableAction == "on")
    {
        item->SetIsItemStackable(true);
        item->Save(false);
        psserver->SendSystemInfo(client->GetClientNum(), "ItemStackable flag ON");
        return;
    }
    else if(data->stackableAction == "off")
    {
        item->SetIsItemStackable(false);
        item->Save(false);
        psserver->SendSystemInfo(client->GetClientNum(), "ItemStackable flag OFF");
        return;
    }
    else if(data->stackableAction == "reset")
    {
        item->ResetItemStackable();
        item->Save(false);
        psserver->SendSystemInfo(client->GetClientNum(), "ItemStackable flag removed");
        return;
    }
}

void AdminManager::HandleSetQuality(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSetQuality* data = static_cast<AdminCmdDataSetQuality*>(cmddata);
    if(!data->targetObject)
    {
        psserver->SendSystemError(client->GetClientNum(), "No target selected");
        return;
    }

    psItem* item = data->targetObject->GetItem();
    if(!item)
    {
        psserver->SendSystemError(client->GetClientNum(), "Not an item");
        return;
    }

    item->SetItemQuality(data->quality);
    if(data->qualityMax)
    {
        item->SetMaxItemQuality(data->qualityMax);
    }

    item->Save(false);

    psserver->SendSystemOK(client->GetClientNum(), "Quality changed successfully to: %f/%f", item->GetItemQuality(), item->GetMaxItemQuality());
}

void AdminManager::HandleSetTrait(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSetTrait* data = static_cast<AdminCmdDataSetTrait*>(cmddata);

    if(data->subCmd == "list")
    {
        if(data->race.IsEmpty())
        {
            psserver->SendSystemError(client->GetClientNum(), "Syntax: /settrait list [race] [gender]");
            return;
        }

        // check if given race is valid
        psRaceInfo* raceInfo = psserver->GetCacheManager()->GetRaceInfoByNameGender(data->race.GetData(), data->gender);
        if(!raceInfo)
        {
            psserver->SendSystemError(client->GetClientNum(), "Invalid race!");
            return;
        }

        // collect all matching traits
        CacheManager::TraitIterator ti = psserver->GetCacheManager()->GetTraitIterator();
        csString message = "Available traits:\n";
        bool found = false;
        while(ti.HasNext())
        {
            psTrait* currTrait = ti.Next();
            if(currTrait->race == raceInfo->race && currTrait->gender == data->gender && message.Find(currTrait->name.GetData()) == (size_t)-1)
            {
                message.Append(currTrait->name+", ");
                found = true;
            }
        }

        // get rid of the last semicolon
        if(found)
        {
            message.DeleteAt(message.FindLast(','));
        }

        psserver->SendSystemInfo(client->GetClientNum(), message);
        return;
    }

    if(data->traitName.IsEmpty())
    {
        psserver->SendSystemError(client->GetClientNum(), "Syntax: /settrait [[target] [trait] | list [race] [gender]]");
        return;
    }

    psCharacter* target;
    if(data->targetObject && data->targetObject->GetCharacterData())
    {
        target = data->targetObject->GetCharacterData();
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "Invalid target for setting traits");
        return;
    }

    CacheManager::TraitIterator ti = psserver->GetCacheManager()->GetTraitIterator();
    while(ti.HasNext())
    {
        psTrait* currTrait = ti.Next();
        if(currTrait->gender == target->GetRaceInfo()->gender &&
                currTrait->race == target->GetRaceInfo()->race &&
                currTrait->name.CompareNoCase(data->traitName))
        {
            target->SetTraitForLocation(currTrait->location, currTrait);

            csString str("<traits>");
            do
            {
                str.Append(currTrait->ToXML());
                currTrait = currTrait->next_trait;
            }
            while(currTrait);
            str.Append("</traits>");

            psTraitChangeMessage message(client->GetClientNum(), target->GetActor()->GetEID(), str);
            message.Multicast(target->GetActor()->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
            //update everything needed for bgloader to pick changes correctly.
            data->targetObject->UpdateProxList(true);

            psserver->SendSystemOK(client->GetClientNum(), "Trait successfully changed");
            return;
        }
    }
    psserver->SendSystemError(client->GetClientNum(), "Trait not found");
}

void AdminManager::HandleSetItemName(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSetItem* data = static_cast<AdminCmdDataSetItem*>(cmddata);

    if(!data->targetObject)
    {
        psserver->SendSystemError(client->GetClientNum(), "No target selected");
        return;
    }

    psItem* item = data->targetObject->GetItem();
    if(!item)
    {
        psserver->SendSystemError(client->GetClientNum(), "Not an item");
        return;
    }

    item->SetName(data->name);
    if(!data->description.IsEmpty())
        item->SetDescription(data->description);

    item->Save(false);

    psserver->SendSystemOK(client->GetClientNum(), "Name changed successfully");
}

void AdminManager::HandleReload(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataReload* data = static_cast<AdminCmdDataReload*>(cmddata);

    if(data->subCmd == "item")
    {
        bool bCreatingNew = false;
        psItemStats* itemStats = psserver->GetCacheManager()->GetBasicItemStatsByID(data->itemID);
        iResultSet* rs = db->Select("select * from item_stats where id = %d", data->itemID);
        if(!rs || rs->Count() == 0)
        {
            psserver->SendSystemError(client->GetClientNum(), "Item stats for %d not found", data->itemID);
            return;
        }
        if(itemStats == NULL)
        {
            bCreatingNew = true;
            itemStats = new psItemStats();
        }

        if(!itemStats->ReadItemStats((*rs)[0]))
        {
            psserver->SendSystemError(client->GetClientNum(), "Couldn't load new item stats %d", data->itemID);
            if(bCreatingNew)
                delete itemStats;
            return;
        }

        if(bCreatingNew)
        {
            psserver->GetCacheManager()->AddItemStatsToHashTable(itemStats);
            psserver->SendSystemOK(client->GetClientNum(), "Successfully created new item %d", data->itemID);
        }
        else
            psserver->SendSystemOK(client->GetClientNum(), "Successfully reloaded item %d", data->itemID);
    }
    else if(data->subCmd == "serveroptions")
    {
        if(psserver->GetCacheManager()->ReloadOptions())
            psserver->SendSystemOK(client->GetClientNum(), "Successfully reloaded server options.");
        else
            psserver->SendSystemError(client->GetClientNum(), "Failed to reload server options.");
    }
    else if(data->subCmd == "mathscript")
    {
        psserver->GetMathScriptEngine()->ReloadScripts(db);
        psserver->SendSystemOK(client->GetClientNum(), "Successfully reloaded math scripts.");
    }
    else if(data->subCmd == "path")
    {
        HideAllPaths(true); // And cleare Selected Paths

        delete pathNetwork;
        pathNetwork = new psPathNetwork();
        pathNetwork->Load(EntityManager::GetSingleton().GetEngine(),db,
                          EntityManager::GetSingleton().GetWorld());

        RedisplayAllPaths();

        psserver->SendSystemOK(client->GetClientNum(), "Successfully reloaded path network.");

    }
    else if(data->subCmd == "locations")
    {
        HideAllLocations(true); // And cleare Selected Locations

        delete locations;
        locations = new LocationManager();
        locations->Load(EntityManager::GetSingleton().GetEngine(),db);

        RedisplayAllLocations();

        psserver->SendSystemOK(client->GetClientNum(), "Successfully reloaded locations.");

    }
}

void AdminManager::HandleListWarnings(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataListWarnings* data = static_cast<AdminCmdDataListWarnings*>(cmddata);
    AccountID accountID;
    PID pid; //used when offline to allow the use of pids

    accountID = data->GetAccountID(client->GetClientNum());

    if(accountID.IsValid())
    {
        Result rs(db->Select("SELECT warningGM, timeOfWarn, warnMessage FROM warnings WHERE accountid = %u", accountID.Unbox()));
        if(rs.IsValid())
        {
            csString newLine;
            unsigned long i = 0;
            for(i = 0 ; i < rs.Count() ; i++)
            {
                newLine.Format("%s - %s - %s", rs[i]["warningGM"], rs[i]["timeOfWarn"], rs[i]["warnMessage"]);
                psserver->SendSystemInfo(client->GetClientNum(), newLine.GetData());
            }
            if(i == 0)
                psserver->SendSystemInfo(client->GetClientNum(), "No warnings found for account: %s.", ShowID(accountID));
        }
    }
    else
        psserver->SendSystemError(client->GetClientNum(), "Target wasn't found.");
}

void AdminManager::CheckTarget(psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataTarget* data = static_cast<AdminCmdDataTarget*>(cmddata);

    if(!data->target.Length())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"Syntax: \"/targetname [me/target/eid/pid/area/name]\"");
        return;
    }
    AccountID aid = (!data->IsTargetType(ADMINCMD_TARGET_PLAYER) && !data->IsTargetType(ADMINCMD_TARGET_PID)) ? 0 : data->GetAccountID(client->GetClientNum());
    psserver->SendSystemInfo(client->GetClientNum(),"Targeted: %s (%s, %s, online: %d)", data->target.GetData(),
                             data->targetID.Show().GetData(), aid.Show().GetData(), data->IsOnline() ? "yes" : "no");
}

void AdminManager::DisableQuest(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataDisableQuest* data = static_cast<AdminCmdDataDisableQuest*>(cmddata);
    psQuest* quest = psserver->GetCacheManager()->GetQuestByName(data->questName);  //get the quest associated by name

    if(!quest)  //the quest was not found
    {
        psserver->SendSystemError(client->GetClientNum(), "Unable to find the requested quest.");
        return;
    }

    quest->Active(!quest->Active()); //invert the status of the quest: if enabled, disable it, if disabled, enable it
    if(data->saveToDb)  //if the subcmd is save we save this also on the database
    {
        if(!psserver->CheckAccess(client, "save quest disable", false))  //check if the client has the correct rights to do this
        {
            psserver->SendSystemInfo(client->GetClientNum(),"You can't change the active status of quests: the quest status was changed only temporarily.");
            return;
        }

        Result flags(db->Select("SELECT flags FROM quests WHERE id=%u", quest->GetID())); //get the current flags from the database
        if(!flags.IsValid() || flags.Count() == 0)  //there were results?
        {
            psserver->SendSystemError(client->GetClientNum(), "Unable to find the quest in the database.");
            return;
        }

        uint flag = flags[0].GetUInt32("flags"); //get the flags and assign them to a variable

        if(!quest->Active())  //if active assign the flag
            flag |= PSQUEST_DISABLED_QUEST;
        else //else remove the flag
            flag &= ~PSQUEST_DISABLED_QUEST;

        //save the flags to the db
        db->CommandPump("UPDATE quests SET flags=%u WHERE id=%u", flag, quest->GetID());
    }

    //tell the user that everything went fine
    psserver->SendSystemInfo(client->GetClientNum(),"The quest %s was %s successfully.", quest->GetName(), quest->Active() ? "enabled" : "disabled");
}

void AdminManager::SetKillExp(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataSetKillExp* data = static_cast<AdminCmdDataSetKillExp*>(cmddata);
    gemActor* target = data->targetActor;

    if(data->expValue <= 0)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"Only positive exp values are allowed.");
        return;
    }

    gemActor* actor;

    if(!data->target.Length())
        actor = client->GetActor();
    else
        actor = target;

    //check access
    if(actor != client->GetActor() && !psserver->CheckAccess(client, "setkillexp others", false))
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You are not allowed to setkillexp others.");
        return;
    }

    if(actor && actor->GetCharacterData())
    {
        actor->GetCharacterData()->SetKillExperience(data->expValue);
        //tell the user that everything went fine
        psserver->SendSystemInfo(client->GetClientNum(),"When killed the target will now automatically award %d experience",data->expValue);
    }
    else
    {
        //tell the user that everything went wrong
        psserver->SendSystemInfo(client->GetClientNum(),"Unable to find target.");
    }
}

//This is used as a wrapper for the command version of adjustfactionstanding.
void AdminManager::AssignFaction(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataAssignFaction* data = static_cast<AdminCmdDataAssignFaction*>(cmddata);
    psRewardDataFaction rewardData(data->factionName, data->factionPoints);

    AwardToTarget(me->clientnum, data->targetClient, &rewardData);
}

void AdminManager::HandleServerQuit(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataServerQuit* data = static_cast<AdminCmdDataServerQuit*>(cmddata);
    if(data->time < -1)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"Syntax: \"/serverquit [-1/time] <Reason>\"");
        return;
    }

    if(data->reason.Length())
    {
        psSystemMessage newmsg(0, MSG_INFO_SERVER, "Server Admin: " + data->reason);
        psserver->GetEventManager()->Broadcast(newmsg.msg);
    }

    psserver->QuitServer(data->time, client);
}

void AdminManager::HandleNPCClientQuit(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    //AdminCmdDataNPCClientQuit* data = static_cast<AdminCmdDataNPCClientQuit*>(cmddata);

    psServerCommandMessage message(0, "quit");
    message.Multicast(psserver->GetNPCManager()->GetSuperClients(), -1, PROX_LIST_ANY_RANGE);

    psserver->SendSystemInfo(client->GetClientNum(),"NPC Client quit requested.");
}


void AdminManager::HandleVersion(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client)
{
    //    AdminCmdDataSimple* data = static_cast<AdminCmdDataSimple*>(cmddata);

    psserver->SendSystemInfo(client->GetClientNum(),"Server svn version at last commit was $Rev: 9440 $");
}


void AdminManager::RandomMessageTest(AdminCmdData* cmddata, Client* client)
{
    AdminCmdDataRndMsgTest* data = static_cast<AdminCmdDataRndMsgTest*>(cmddata);
    csArray<int> values;
    for(int i=0; i<10; i++)
    {
        int value = psserver->GetRandom(10) + 1; // range from 1-10, not 0-9
        if(values.Find(value) != SIZET_NOT_FOUND)  // already used
            i--;  // try again
        else
            values.Push(value);
    }

    for(int i=0; i<10; i++)
    {
        psOrderedMessage seq(client->GetClientNum(), values[i], data->sequential ? values[i] : 0);  // 0 means not sequenced so values should show up randomly
        // seq.SendMessage();
        psserver->GetNetManager()->SendMessageDelayed(seq.msg,i*1000);  // send out 1 per second
    }
}
