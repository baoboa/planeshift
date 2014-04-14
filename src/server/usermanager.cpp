/*
* usermanager.cpp - Author: Keith Fulton
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
#include <ctype.h>
#include <string.h>
#include <memory.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/engine.h>
#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/serverconsole.h"
#include "util/pserror.h"
#include "util/psconst.h"
#include "util/log.h"
#include "util/eventmanager.h"
#include "util/strutil.h"

#include "engine/psworld.h"

#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psactionlocationinfo.h"

#include "rpgrules/factions.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "actionmanager.h"
#include "adminmanager.h"
#include "advicemanager.h"
#include "bankmanager.h"
#include "cachemanager.h"
#include "chatmanager.h"
#include "client.h"
#include "clients.h"
#include "combatmanager.h"
#include "commandmanager.h"
#include "entitymanager.h"
#include "events.h"
#include "gem.h"
#include "globals.h"
#include "gmeventmanager.h"
#include "invitemanager.h"
#include "marriagemanager.h"
#include "netmanager.h"
#include "netmanager.h"
#include "playergroup.h"
#include "progressionmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "usermanager.h"
#include "spawnmanager.h" // needed for handleGroupLootItem()

#define RANGE_TO_CHALLENGE 50
#define GM_LEVEL 1

class psUserStatRegeneration : public psGameEvent
{
protected:
    UserManager* usermanager;

public:
    psUserStatRegeneration(UserManager* mgr,csTicks ticks);

    virtual void Trigger();  // Abstract event processing function
};

/** A structure to hold the clients that are pending on duel challenges.
*/
class PendingDuelInvite : public PendingInvite
{
public:

    PendingDuelInvite(Client* inviter,
                      Client* invitee,
                      const char* question)
        : PendingInvite(inviter, invitee, true,
                        question,"Accept","Decline",
                        "You have challenged %s to a duel.",
                        "%s has challenged you to a duel.",
                        "%s has accepted your challenge.",
                        "You have accepted %s's challenge.",
                        "%s has declined your challenge.",
                        "You have declined %s's challenge.", psQuestionMessage::duelConfirm)
    {
    }

    virtual ~PendingDuelInvite()
    {
    }

    void HandleAnswer(const csString &answer);
};

/** A structure to hold the clients that are pending on mounting invite.
*/
class PendingMountInvite : public PendingInvite
{
public:

    PendingMountInvite(Client* inviter,
                       Client* invitee,
                       const char* question)
        : PendingInvite(inviter, invitee, true,
                        question,"Accept","Decline",
                        "You have asked %s to be your mount.",
                        "You have been asked to be %s's mount.",
                        "%s has accepted to be your mount.",
                        "You have accepted to be %s's mount.",
                        "%s has declined your proposition.",
                        "You have declined %s's proposition.",
                        psQuestionMessage::generalConfirm)
    {
    }

    virtual ~PendingMountInvite() {}

    void HandleAnswer(const csString &answer);
};

/***********************************************************************/

UserManager::UserManager(ClientConnectionSet* cs, CacheManager* cachemanager, BankManager* bankmanager, EntityManager* entitymanager)
{
    clients       = cs;
    cacheManager = cachemanager;
    bankManager = bankmanager;
    entityManager = entitymanager;

    Subscribe(&UserManager::HandleUserCommand, MSGTYPE_USERCMD, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&UserManager::HandleMOTDRequest, MSGTYPE_MOTDREQUEST, REQUIRE_ANY_CLIENT);
    Subscribe(&UserManager::HandleCharDetailsRequest, MSGTYPE_CHARDETAILSREQUEST, REQUIRE_READY_CLIENT);
    Subscribe(&UserManager::HandleCharDescUpdate, MSGTYPE_CHARDESCUPDATE, REQUIRE_READY_CLIENT);
    Subscribe(&UserManager::HandleTargetEvent, MSGTYPE_TARGET_EVENT, NO_VALIDATION);
    Subscribe(&UserManager::HandleEntranceMessage, MSGTYPE_ENTRANCE, REQUIRE_READY_CLIENT);
    Subscribe(&UserManager::HandleClientReady, MSGTYPE_CONNECT_EVENT, REQUIRE_ANY_CLIENT);

    userCommandHash.Put("/admin",        &UserManager::HandleAdminCommand);
    userCommandHash.Put("/assist",       &UserManager::Assist);
    userCommandHash.Put("/attack",       &UserManager::HandleAttack);
    userCommandHash.Put("/queue",        &UserManager::HandleQueueAttack);
    userCommandHash.Put("/bank",         &UserManager::HandleBanking);
    userCommandHash.Put("/buddy",        &UserManager::Buddy);
    userCommandHash.Put("/challenge",    &UserManager::ChallengeToDuel);
    userCommandHash.Put("/die",          &UserManager::HandleDie);
    userCommandHash.Put("/guard",        &UserManager::HandleGuard);
    userCommandHash.Put("/loot",         &UserManager::HandleLoot);
    userCommandHash.Put("/marriage",     &UserManager::HandleMarriage);
    userCommandHash.Put("/motd",         &UserManager::GiveMOTD);
    userCommandHash.Put("/mount",        &UserManager::HandleMount);
    userCommandHash.Put("/npcmenu",      &UserManager::ShowNpcMenu);
    userCommandHash.Put("/pickup",       &UserManager::HandlePickup);
    userCommandHash.Put("/pos",          &UserManager::ReportPosition);
    userCommandHash.Put("/quests",       &UserManager::HandleQuestsCommand);
    userCommandHash.Put("/roll",         &UserManager::RollDice);
    userCommandHash.Put("/rotate",       &UserManager::HandleRotate);
    userCommandHash.Put("/sit",          &UserManager::HandleSit);
//    userCommandHash.Put("/spawn",        &UserManager::MoveToSpawnPos);
    userCommandHash.Put("/stand",        &UserManager::HandleStand);
    userCommandHash.Put("/starttrading", &UserManager::StartTrading);
    userCommandHash.Put("/stopattack",   &UserManager::HandleStopAttack);
    userCommandHash.Put("/stoptrading",  &UserManager::StopTrading);
    userCommandHash.Put("/tip",          &UserManager::GiveTip);
    userCommandHash.Put("/train",        &UserManager::HandleTraining);
    userCommandHash.Put("/unmount",      &UserManager::HandleUnmount);
    userCommandHash.Put("/unstick",      &UserManager::HandleUnstick);
    userCommandHash.Put("/who",          &UserManager::Who);
    userCommandHash.Put("/yield",        &UserManager::HandleYield);
    userCommandHash.Put("/takeall",      &UserManager::HandleTakeAll); // Handle trying to take all items from container
    userCommandHash.Put("/takestackall", &UserManager::HandleTakeStackAll); // Take all items from container and stack them not precisely
}

UserManager::~UserManager()
{
    csHash<EMOTE*, csString>::GlobalIterator it(emoteHash.GetIterator());
    while(it.HasNext())
    {
        EMOTE* emote = it.Next();
        delete emote;
    }
    emoteHash.DeleteAll();
}

void UserManager::HandleMOTDRequest(MsgEntry* me,Client* client)
{
    //Sends MOTD and tip

    unsigned int guildID =0;
    //If data isn't loaded, load from db
    if(!client->GetCharacterData())
    {
        // FIXME: We really shouldn't be loading this from the DB.
        Result result(db->Select("SELECT guild_member_of FROM characters WHERE id = '%u'", client->GetPID().Unbox()));
        if(result.Count() > 0)
            guildID = result[0].GetUInt32(0);
    }
    else
    {
        psGuildInfo* playerGuild = client->GetCharacterData()->GetGuild();

        if(playerGuild)
            guildID = playerGuild->GetID();
        else
            guildID = 0;
    }

    csString tip;
    if(cacheManager->GetTipLength() > 0)
        cacheManager->GetTipByID(psserver->GetRandom(cacheManager->GetTipLength()), tip);

    csString motdMsg(psserver->GetMOTD());

    csString guildMotd("");
    csString guildName("");
    psGuildInfo* guild = cacheManager->FindGuild(guildID);

    if(guild)
    {
        guildMotd=guild->GetMOTD();
        guildName=guild->GetName();
    }

    psMOTDMessage motd(me->clientnum,tip,motdMsg,guildMotd,guildName);
    motd.SendMessage();
    return;
}

void UserManager::HandleUserCommand(MsgEntry* me,Client* client)
{
    if(client->GetActor()->IsFrozen())  //disable most commands
        return;

    psUserCmdMessage msg(me);

    Debug3(LOG_USER, client->GetClientNum(),"Received user command: %s from %s\n",
           me->bytes->payload, (const char*)client->GetName());

    userCmdPointer* cmdPt = userCommandHash.GetElementPointer(msg.command);
    if(cmdPt != NULL)
    {
        (this->**cmdPt)(msg,client);
        return;
    }

    // We don't check for validity for emotes, they always are.
    if(!msg.valid && !CheckForEmote(msg.command, true, client->GetActor()))
    {
        psserver->SendSystemError(me->clientnum,"Command not supported by server yet.");
        return;
    }
}

bool UserManager::CheckForEmote(csString command, bool execute, gemActor* actor)
{
    EMOTE* emotePtr = emoteHash.Get(command, NULL);
    if(emotePtr == NULL)
        return false;

    Emote(emotePtr->general, emotePtr->specific, emotePtr->anim, actor);
    return true;
}

void UserManager::Emote(csString general, csString specific, csString animation, gemActor* actor)
{
    csString cssText("");
    if(actor->GetTargetObject() && (actor->GetTargetObject() != actor))
    {
        cssText.Format(specific, actor->GetName(), actor->GetTargetObject()->GetName());
    }
    else
    {
        cssText.Format(general, actor->GetName());
    }

    // retrieve proximity list
    csArray<PublishDestination> &clients = actor->GetMulticastClients();

    // Create and multicast the message
    psSystemMessage newmsg(actor->GetClientID(), MSG_INFO_BASE, cssText.GetData());
    newmsg.Multicast(clients, 0, CHAT_SAY_RANGE);

    // Save message to clients chat history meeting SAY range (PS#2789)
    for(size_t i = 0; i < clients.GetSize(); i++)
    {
        Client* target = psserver->GetConnections()->Find(clients[i].client);
        if(target && clients[i].dist < CHAT_SAY_RANGE)
            target->GetActor()->LogLine(cssText.GetData()); // No special formatting for emotes. we use gemActor::LogLine directly
    }

    // Send animation message, if animation is set
    if(animation != "noanim")
    {
        psUserActionMessage anim(actor->GetClientID(), actor->GetEID(), animation);
        anim.Multicast(actor->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    }
}

bool UserManager::LoadEmotes(const char* xmlfile, iVFS* vfs)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDataBuffer> buff = vfs->ReadFile(xmlfile);

    if(!buff || !buff->GetSize())
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(buff);
    if(error)
    {
        Error3("%s in %s", error, xmlfile);
        return false;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", xmlfile);
        return false;
    }

    csRef<iDocumentNode> topNode = root->GetNode("emotes");
    if(!topNode)
    {
        Error2("No <emotes> tag in %s", xmlfile);
        return false;
    }
    csRef<iDocumentNodeIterator> emoteIter = topNode->GetNodes("emote");

    EMOTE* emote;

    while(emoteIter->HasNext())
    {
        csRef<iDocumentNode> emoteNode = emoteIter->Next();

        emote = new EMOTE;
        emote->command = emoteNode->GetAttributeValue("command");
        emote->general = emoteNode->GetAttributeValue("general");
        emote->specific = emoteNode->GetAttributeValue("specific");
        emote->anim = emoteNode->GetAttributeValue("anim");

        emoteHash.Put(emote->command, emote);
    }

    return true;
}

void UserManager::HandleCharDetailsRequest(MsgEntry* me,Client* client)
{
    psCharacterDetailsRequestMessage msg(me);

    gemActor* actor;
    if(!msg.isMe)
    {
        actor = dynamic_cast<gemActor*>(client->GetTargetObject());
    }
    else
    {
        actor = client->GetActor();
    }

    if(!actor)
        return;

    SendCharacterDescription(client, actor, false, msg.isSimple, msg.requestor);
}

csString fmtStatLine(const char* const label, unsigned int value, unsigned int buffed)
{
    csString s;
    if(value != buffed)
        s.Format("%s: %u (%u)\n", label, value, buffed);
    else
        s.Format("%s: %u\n", label, value);
    return s;
}

void UserManager::CalculateComparativeDifference(psCharacter* myCharData, psCharacter* theirCharData, int &theirPhysicalLevel, int &theirMagicalLevel, int &physicalDiff,int &magicalDiff,int &overallLevelComparison)
{

    // Begin by gathering stats. We will get overall strength for physical and magical and also a comparison.
    int myPhysicalStat = myCharData->GetCharLevel(true);
    int myMagicalStat = myCharData->GetCharLevel(false);
    int theirPhysicalStat = theirCharData->GetCharLevel(true);
    int theirMagicalStat = theirCharData->GetCharLevel(false);

    // We also take skill into consideration.
    int myBestCombat = 0;
    int myBestMagical = 0;
    int theirBestCombat = 0;
    int theirBestMagical = 0;

    for(size_t i=0; i < cacheManager->GetSkillAmount(); ++i)
    {
        // calculates my best skills
        Skill &mySkill = myCharData->Skills().Get((PSSKILL)i);

        if(mySkill.info->category == PSSKILLS_CATEGORY_COMBAT)
        {
            myBestCombat = myBestCombat<mySkill.rank.Current()?mySkill.rank.Current():myBestCombat;
        }
        else if(mySkill.info->category == PSSKILLS_CATEGORY_MAGIC)
        {
            myBestMagical = myBestMagical<mySkill.rank.Current()?mySkill.rank.Current():myBestMagical;
        }
        else
        {
            continue;
        }

        // calculates their best skills
        Skill &theirSkill = theirCharData->Skills().Get((PSSKILL)i);

        if(theirSkill.info->category == PSSKILLS_CATEGORY_COMBAT)
        {
            theirBestCombat = theirBestCombat<theirSkill.rank.Current()?theirSkill.rank.Current():theirBestCombat;
        }
        else if(theirSkill.info->category == PSSKILLS_CATEGORY_MAGIC)
        {
            theirBestMagical = theirBestMagical<theirSkill.rank.Current()?theirSkill.rank.Current():theirBestMagical;
        }
        else
        {
            continue;
        }
    }

    // We calculate two 'levels', one physical and one magical.
    const int maxLevel = 6;

    theirPhysicalLevel = ((float)(theirPhysicalStat + theirBestCombat * 2) / 800) * maxLevel;
    theirPhysicalLevel = (theirPhysicalLevel > maxLevel) ? maxLevel : theirPhysicalLevel;
    theirMagicalLevel = ((float)(theirMagicalStat + theirBestMagical * 2) / 800) * maxLevel;
    theirMagicalLevel = (theirMagicalLevel > maxLevel) ? maxLevel : theirMagicalLevel;

    int myPhysicalLevel = ((float)(myPhysicalStat + myBestCombat * 2) / 800) * maxLevel;
    myPhysicalLevel = (myPhysicalLevel > maxLevel) ? maxLevel : myPhysicalLevel;
    int myMagicalLevel = ((float)(myMagicalStat + myBestMagical * 2) / 800) * maxLevel;
    myMagicalLevel = (myMagicalLevel > maxLevel) ? maxLevel : myMagicalLevel;

    // And also a comparative difference for each.
    int physicalDiffValue = theirPhysicalLevel - myPhysicalLevel;
    int magicalDiffValue = theirMagicalLevel - myMagicalLevel;
    // Clamp within the allowed range (0-6) considering difference from the median (3)
    physicalDiffValue = (3+physicalDiffValue)>maxLevel?maxLevel:(3+physicalDiffValue);
    physicalDiff = physicalDiffValue<0?0:physicalDiffValue;
    magicalDiffValue = (3+magicalDiffValue)>maxLevel?maxLevel:(3+magicalDiffValue);
    magicalDiff = magicalDiffValue<0?0:magicalDiffValue;

    // Just do the average of physical and magical values
    overallLevelComparison = int ((float)(physicalDiff + magicalDiff) / 2.0f);
}



void UserManager::SendCharacterDescription(Client* client, gemActor* actor, bool full, bool simple, const csString &requestor)
{
    psCharacter* charData = actor->GetCharacterData();

    bool isSelf = (charData->GetPID() == client->GetCharacterData()->GetPID());

    csString charName = charData->GetCharFullName();
    csString raceName;
    csString creationinfo;
    PSCHARACTER_GENDER gender;
    psRaceInfo* charrinfo = charData->GetRaceInfo();
    if(charrinfo != NULL)
    {
        raceName = charrinfo->GetRace();
        gender = charrinfo->gender;
    }
    else
    {
        raceName = "being";
        gender = PSCHARACTER_GENDER_NONE;
    }
    csString desc          = charData->GetDescription();

    csArray<psCharacterDetailsMessage::NetworkDetailSkill> skills;

    if(!simple && psserver->CheckAccess(client, "view stats", false))
    {
        full = true;  // GMs can view the stats list
    }

    //send creation info only if the player is requesting  his info
    if(isSelf || full)
    {
        if(requestor != "pawsCharDescription") //only send all if the requestor is the normal description window
        {
            csString factiondescription; //used to store temporarily the faction dynamic data

            creationinfo += charData->GetCreationInfo();

            if(creationinfo.Length() > 0) //only add separators in case there is something to separate
                creationinfo += "\n";

            //get the dynamically generated faction based life events
            if(charData->GetFactionEventsDescription(factiondescription))
            {
                creationinfo += factiondescription;
                creationinfo += "\n";
            }
            creationinfo += charData->GetLifeDescription();
        }
        else //if the player requested to edit send only what he can actually edit
            creationinfo = charData->GetLifeDescription();
    }

    csString desc_ooc     = charData->GetOOCDescription();

    if(full)
    {
        desc += "\n\n";
        desc.AppendFmt("HP: %d Max HP: %d(%d)", int(charData->GetHP()), int(charData->GetMaxHP().Base()), int(charData->GetMaxHP().Current()));
        desc.AppendFmt("\nMANA: %d Max MANA: %d(%d)", int(charData->GetMana()), int(charData->GetMaxMana().Base()), int(charData->GetMaxMana().Current()));
        SkillSet &sks = charData->Skills();

        for(size_t skill = 0; skill < cacheManager->GetSkillAmount(); skill++)
        {
            psSkillInfo* skinfo;
            skinfo = cacheManager->GetSkillByID((PSSKILL)skill);
            if(skinfo != NULL)
            {
                psCharacterDetailsMessage::NetworkDetailSkill s;
                s.category = skinfo->category;
                SkillRank &rank = sks.GetSkillRank(static_cast<PSSKILL>(skill));
                s.text = fmtStatLine(skinfo->name, rank.Base(), rank.Current());

                skills.Push(s);
            }
        }

        csHash<FactionStanding*, int>::GlobalIterator iter(
            charData->GetFactions()->GetStandings().GetIterator());
        while(iter.HasNext())
        {
            FactionStanding* standing = iter.Next();
            psCharacterDetailsMessage::NetworkDetailSkill s;

            s.category = 5; // faction
            s.text.Format("%s: %d\n", standing->faction->name.GetDataSafe(),
                          standing->score);
            skills.Push(s);
        }
    }

    // No fancy things if simple description is requested
    if(!simple)
    {
        // Don't guess strength if we can't attack the character or if he's
        //  dead or if we are viewing our own description
        if(!charData->GetImperviousToAttack() && actor->GetMode() != PSCHARACTER_MODE_DEAD && !isSelf)
        {

            // And also a comparative difference for each.
            int theirPhysicalLevel, theirMagicalLevel, physicalDiff, magicalDiff, overallLevelComparison;
            CalculateComparativeDifference(client->GetCharacterData(), charData,
                                           theirPhysicalLevel,theirMagicalLevel,
                                           physicalDiff, magicalDiff, overallLevelComparison);

            // Character's magical strength assessment.
            static const char* const MagicalStrengthAssessPhrases[] =
            {
                "to be as intelligent as a stone",
                "to be an apprentice magic user",
                "to be an advanced apprentice in magic use",
                "to be a competant magic user",
                "to be an above average mage",
                "to be highly advanced in the arcane arts",
                "to be surrounded by a powerful magic aura"
            };

            // Character's physical strength assessment.
            static const char* const PhysicalStrengthAssessPhrases[] =
            {
                "to be very frail and feeble in physical combat",
                "to be weak and poorly trained in physical combat",
                "to be a trainee fighter with mediocre fighting abilities",
                "to be fit and have respectable fighting skills",
                "to be in very good shape, with much experience in the fighting arts",
                "to be strong, tough and agile with advanced fighting abilities",
                "to be physically perfect and a master in combat"
            };

            // Character's overall comparison.
            static const char* const OverallComparePhrases[] =
            {
                "wouldn't require any effort to defeat",
                "is noticeably weaker than you",
                "is not quite as strong as you",
                "is about as strong as you",
                "is a fair bit stronger than you",
                "would pose a great challenge to defeat",
                "is significantly more powerful than you"
            };

            desc.AppendFmt("\n\n%s appears %s, while also appearing %s. You judge that %s %s.", charName.GetData(),
                           MagicalStrengthAssessPhrases[theirMagicalLevel], PhysicalStrengthAssessPhrases[theirPhysicalLevel],
                           charName.GetData(), OverallComparePhrases[overallLevelComparison]);
        }

        // Show spouse name if character is married
        if(charData->GetIsMarried() && !isSelf)
            desc.AppendFmt("\n\nMarried to: %s", charData->GetSpouseName());


        // Show owner name if character is a pet
        if(charData->IsPet() && charData->GetOwnerID().IsValid())
        {
            gemActor* owner = gem->FindPlayerEntity(charData->GetOwnerID());
            if(owner)
            {
                desc.AppendFmt("\n\nA pet owned by %s.", owner->GetName());
            }
        } else if(charData->IsHired() && charData->GetOwnerID().IsValid())
        {
            gemActor* owner = gem->FindPlayerEntity(charData->GetOwnerID());
            if(owner)
            {
                desc.AppendFmt("\n\nHired by %s.", owner->GetName());
            }
            else
            {
                // Owner is offline.
                // TODO: Look up the name. Might be good to create a name cache in the cache manager for
                // offline players??
                desc.AppendFmt("\n\nHired by offline player.");
            }
            
        } 
    }

    if(!(charData->IsNPC() || charData->IsPet()) && client->GetSecurityLevel() < GM_LEVEL_0 &&
            !client->GetCharacterData()->Knows(charData) && !isSelf)
    {
        charName = "[Unknown]";
    }

    // Finally send the details message
    psCharacterDetailsMessage detailmsg(client->GetClientNum(), charName,
                                        (short unsigned int)gender, raceName,
                                        desc, skills, desc_ooc, creationinfo, requestor);
    detailmsg.SendMessage();
}

void UserManager::HandleCharDescUpdate(MsgEntry* me,Client* client)
{
    psCharacterDescriptionUpdateMessage descUpdate(me);
    psCharacter* charData = client->GetCharacterData();
    if(!charData)
        return;

    if(descUpdate.desctype == DESC_IC)
        charData->SetDescription(descUpdate.newValue);
    else if(descUpdate.desctype == DESC_OOC)
        charData->SetOOCDescription(descUpdate.newValue);
    else if(descUpdate.desctype == DESC_CC)
        charData->SetLifeDescription(descUpdate.newValue);

    Debug3(LOG_USER, client->GetClientNum(), "Character description updated for %s (%s)\n", charData->GetCharFullName(), ShowID(client->GetAccountID()));
}

void UserManager::HandleSit(psUserCmdMessage &msg, Client* client)
{
    gemActor* actor = client->GetActor();

    actor->Sit();
}

void UserManager::HandleAdminCommand(psUserCmdMessage &msg, Client* client)
{
    psserver->GetAdminManager()->Admin(client->GetClientNum(), client, msg.level);
}

void UserManager::HandleQuestsCommand(psUserCmdMessage &msg, Client* client)
{
    HandleQuests(client);
    HandleGMEvents(client);
}

void UserManager::HandleMarriage(psUserCmdMessage &msg, Client* client)
{
    if(msg.action == "propose")
    {
        if(msg.player.IsEmpty() || msg.text.IsEmpty())
        {
            psserver->SendSystemError(client->GetClientNum(), "Usage: /marriage propose [first name] [message]");
            return;
        }

        // Send propose message
        psserver->GetMarriageManager()->Propose(client,  msg.player,  msg.text);
    }
    else if(msg.action == "divorce")
    {
        if(msg.text.IsEmpty())
        {
            psserver->SendSystemError(client->GetClientNum(), "Usage: /marriage divorce [message]");
            return;
        }

        // Send divorce prompt
        psserver->GetMarriageManager()->ContemplateDivorce(client,  msg.text);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "Usage: /marriage [propose|divorce]");
    }
}

void UserManager::HandleStand(psUserCmdMessage &msg, Client* client)
{
    gemActor* actor = client->GetActor();

    actor->Stand();
}

void UserManager::HandleTargetEvent(MsgEntry* me, Client* notused)
{
    psTargetChangeEvent targetevent(me);

    Client* targeter = NULL;
    Client* targeted = NULL;
    if(targetevent.character)
    {
        Debug2(LOG_USER, targetevent.character->GetClientID(),"UserManager handling target event for %s\n", targetevent.character->GetName());
        targeter = clients->Find(targetevent.character->GetClientID());
    }
    if(!targeter)
        return;

    if(targetevent.target)
        targeted = clients->Find(targetevent.target->GetClientID());

    psserver->GetCombatManager()->StopAttack(targeter->GetActor());

    if(!targeted
            && dynamic_cast<gemNPC*>(targetevent.target)
            && targetevent.character->GetMode() == PSCHARACTER_MODE_COMBAT) // NPC?
    {
        csString msg;

        if(targeter->GetActor()->IsAllowedToAttack(targetevent.target, msg))
        {
            SwitchAttackTarget(targeter, targeted);
        }
        else
        {
            psserver->SendSystemError(targeter->GetClientNum(), msg);
        }

        return;
    }
    else if(!targeted)
        return;

    if(targeted->IsReady() && targeter->IsReady())
    {
        if(targetevent.character->GetMode() == PSCHARACTER_MODE_COMBAT)
            SwitchAttackTarget(targeter, targeted);
    }
    else
    {
        psserver->SendSystemError(targeter->GetClientNum(),"Target is not ready yet");
    }
}

void UserManager::HandleEntranceMessage(MsgEntry* me, Client* client)
{
    bool secure = (client->GetSecurityLevel() > GM_LEVEL_0) ? true : false;
    psEntranceMessage mesg(me);
    psActionLocation* action = psserver->GetActionManager()->FindAction(mesg.entranceID);
    if(!action)
    {
        if(secure)
            psserver->SendSystemInfo(client->GetClientNum(), "No item/action: %s", ShowID(mesg.entranceID));
        Error2("No item/action: %s", ShowID(mesg.entranceID));
        return;
    }

    gemActor* actor = client->GetActor();
    CS_ASSERT(actor);

    // Check range
    gemObject* gemAction = action->GetGemObject();
    if(gemAction && actor->RangeTo(gemAction, false, true) > RANGE_TO_SELECT)
    {
        psserver->SendSystemError(client->GetClientNum(), "You are no longer in range to do this.");
        return;
    }

    // Check for entrance
    if(!action->IsEntrance())
    {
        if(secure) psserver->SendSystemInfo(client->GetClientNum(),"No <Entrance> tag in action response");
        Error1("No <Entrance> tag in action response");
        return;
    }

    // Check for a lock
    uint32 instanceID = action->GetInstanceID();
    if(instanceID  != 0)
    {

        // find lock to to test if locked
        gemItem* realItem = gem->FindItemEntity(instanceID);
        if(!realItem)
        {
            if(secure) psserver->SendSystemInfo(client->GetClientNum(),"Invalid instance ID %u in action location %s", instanceID, action->name.GetDataSafe());
            Error3("Invalid instance ID %u in action location %s", instanceID, action->name.GetDataSafe());
            return;
        }

        // get real item
        psItem* item = realItem->GetItem();
        if(!item)
        {
            if(secure) psserver->SendSystemInfo(client->GetClientNum(),"Invalid ItemID in Action Location Response.\n");
            Error1("Invalid ItemID in Action Location Response.\n");
            return;
        }

        // Check if locked
        if(item->GetIsLocked())
        {
            if(!client->GetCharacterData()->Inventory().HaveKeyForLock(item->GetUID()))
            {
                psserver->SendSystemInfo(client->GetClientNum(),"Entrance is locked.");
                return;
            }
        }
    }

    // Get entrance attributes
    csVector3 pos = action->GetEntrancePosition();
    float rot = action->GetEntranceRotation();
    csString sectorName = action->GetEntranceSector();

    // Check for different entrance types
    csString entranceType = action->GetEntranceType();

    // Send player to main game instance
    if(entranceType == "Prime")
    {
        if(secure) psserver->SendSystemInfo(client->GetClientNum(),"Teleporting to sector %s", sectorName.GetData());
        actor->Teleport(sectorName, pos, rot, DEFAULT_INSTANCE);
    }

    // Send player to unique instance
    else if(entranceType == "ActionID")
    {
        InstanceID instance = action->id;
        if(secure) psserver->SendSystemInfo(client->GetClientNum(),"Teleporting to sector %s", sectorName.GetData());
        actor->Teleport(sectorName, pos, rot, instance);
    }

    //send player to defined instance
    else if(entranceType == "Instanced")
    {
        InstanceID instance = action->GetEntranceInstance();
        if(secure) psserver->SendSystemInfo(client->GetClientNum(),"Teleporting to sector %s Instance: %u", sectorName.GetData(), instance);
        actor->Teleport(sectorName, pos, rot, instance);
    }

    // Send player back to starting point
    else if(entranceType == "ExitActionID")
    {
        // Use current player instance to get entrance action location
        psActionLocation* retAction = psserver->GetActionManager()->FindActionByID(client->GetActor()->GetInstance());
        if(!retAction)
        {
            if(secure) psserver->SendSystemInfo(client->GetClientNum(),"No exit item/action : %i", client->GetActor()->GetInstance());
            Error2("No exit item/action : %i", client->GetActor()->GetInstance());
            return;
        }

        // Check for return entrance
        if(!retAction->IsReturn())
        {
            if(secure) psserver->SendSystemInfo(client->GetClientNum(),"No <Return tag in action response %s",retAction->response.GetData());
            Error2("No <Return tag in action response %s",retAction->response.GetData());
            return;
        }

        // Process return attributes and spin us around 180 deg for exit
        csVector3 retPos = retAction->GetReturnPosition();
        float retRot = retAction->GetReturnRotation() + (PI/2);
        csString retSectorName = retAction->GetReturnSector();
        InstanceID instance = retAction->GetReturnInstance();

        // Send player back to return point
        if(secure) psserver->SendSystemInfo(client->GetClientNum(),"Teleporting to sector %s", sectorName.GetData());
        actor->Teleport(retSectorName, retPos, retRot, instance);
    }
    else
    {
        if(secure) psserver->SendSystemInfo(client->GetClientNum(),"Unknown type in entrance action location");
        Error1("Unknown type in entrance action location");
    }
    //untarget the entrance/exit
    client->SetTargetObject(NULL,true);
}

void UserManager::HandleDie(psUserCmdMessage &msg, Client* client)
{
    gemActor* actor = client->GetActor();
    if(!actor)
        return;
    actor->Kill(actor);
}

void UserManager::Who(psUserCmdMessage &msg, Client* client)
{
    csString message((size_t) MAXSYSTEMMSGSIZE);
    csString headerMsg("Players Currently Online");

    if(!msg.filter.IsEmpty())
    {
        size_t pos = msg.filter.FindFirst('%');
        while(pos != (size_t) -1)
        {
            msg.filter.DeleteAt(pos);
            pos = msg.filter.FindFirst('%');
        }

        StrToLowerCase(msg.filter);

        headerMsg.Append(" (Applying Filter: '*");
        headerMsg.Append(msg.filter);
        headerMsg.Append("*')");
    }

    message.Append(headerMsg);
    headerMsg.Append("\n-------------------------------------");

    unsigned count = 0;

    if(msg.filter.StartsWith("area:"))
    {
        csArray<csString> filters = DecodeCommandArea(client, msg.filter);
        csArray<csString>::Iterator it(filters.GetIterator());
        while(it.HasNext())
        {
            csString pid = it.Next();
            gemObject* object = FindObjectByString(pid,client->GetActor());
            Client* thisclient = object->GetClient();
            if(thisclient)
            {
                if(!WhoProcessClient(thisclient, client->GetGuildID(), &message, msg.filter,
                                     false, &count))
                    break;
            }
        }
    }
    else
    {
        // Guild rank, guild and title should come from acraig's player prop class.
        ClientIterator i(*clients);
        while(i.HasNext())
        {
            Client* curr = i.Next();
            if(!WhoProcessClient(curr, client->GetGuildID(), &message,
                                 msg.filter, true, &count))
                break;
        }
    }

    // Could be about to overflow by now, so check.
    csString temp;
    temp.Format("%u shown from %zu players online\n", count, clients->CountReadyPlayers());

    message.Append('\n');
    message.Append(temp);

    psSystemMessageSafe newmsg(client->GetClientNum(), MSG_WHO, message);
    newmsg.SendMessage();
}

bool UserManager::WhoProcessClient(Client* curr, int guildId, csString* message, csString filter, bool check, unsigned* count)
{
    if(curr->IsSuperClient() || !curr->GetActor())
        return true;

    csString temp((size_t) MAXSYSTEMMSGSIZE);
    csString playerName(curr->GetName());
    csString guildTitle;
    csString guildName;
    csString format("%s"); // Player name.

    psGuildInfo* guild = curr->GetActor()->GetGuild();
    if(guild != NULL)
    {
        if(guild->GetID() && (!guild->IsSecret() || guild->GetID()
                              == guildId))
        {
            psGuildLevel* level = curr->GetActor()->GetGuildLevel();
            if(level)
            {
                format.Append(", %s in %s"); // Guild level title.
                guildTitle = level->title;
            }
            else
            {
                format.Append(", %s");
            }
            guildName = guild->GetName();
        }
    }
    temp.Format(format.GetData(), curr->GetName(), guildTitle.GetData(),
                guildName.GetData());

    if(check)
    {
        csString lower(temp);
        StrToLowerCase(lower);
        if(!filter.IsEmpty() && lower.FindStr(filter) == (size_t) -1)
            return true;
    }

    /* if message + new text + 50 spare bytes for the footer exceed the maximum package size do not append */
    if(temp.Length() + message->Length() + 50 > MAXSYSTEMMSGSIZE)
        return false;
    else
        message->Append('\n');
    message->Append(temp);

    (*count)++;
    return true;
}

void UserManager::StrToLowerCase(csString &str)
{
    for(register size_t i = 0; i < str.Length(); ++i)
        str[i] = tolower(str[i]);
}

void UserManager::Buddy(psUserCmdMessage &msg,Client* client)
{
    bool onoff = false;  ///< Holds if the player is forcing add (true) or remove
    bool toggle = false; ///< Holds if the player didn't force add or remove and so we are going to toggle

    psCharacter* chardata=client->GetCharacterData();

    if(chardata==NULL)
    {
        Error3("Client for account '%s' attempted to manage buddy '%s' but has no character data!",client->GetName(),msg.player.GetData());
        return;
    }

    if(msg.player.Length() == 0)  //if no imput was provided other than the command send the buddylist
    {
        BuddyList(client,client->GetClientNum(), UserManager::ALL_PLAYERS);
        return;
    }

    msg.player = NormalizeCharacterName(msg.player);

    PID selfid = chardata->GetPID();

    if(msg.action == "add")          //The player provided an add so add the buddy
        onoff = true;
    else if(msg.action == "remove")  //The player provided a remove so remove the buddy
        onoff = false;
    else                              //The player didn't provide either so toggle the buddy
        toggle = true;

    bool excludeNPCs = true;
    PID buddyid = psServer::CharacterLoader.FindCharacterID(msg.player.GetData(), excludeNPCs);

    if(!buddyid.IsValid())  //Check if the buddy was found
    {
        psserver->SendSystemError(client->GetClientNum(),"Could not add buddy: Character '%s' not found.", msg.player.GetData());
        return;
    }

    //If the player used add or didn't provide arguments and the buddy is missing from the list add it
    if((onoff && !toggle)|| (toggle && !chardata->GetBuddyMgr().IsBuddy(buddyid)))
    {
        if(selfid == buddyid)
        {
            psserver->SendSystemError(client->GetClientNum(),"Cannot add yourself to buddy list.");
            return;
        }

        if(!psserver->AddBuddy(selfid,buddyid))
        {
            psserver->SendSystemError(client->GetClientNum(),"%s is already on your buddy list.",(const char*)msg.player);
            return;
        }

        if(!chardata->GetBuddyMgr().AddBuddy(buddyid, msg.player))
        {
            psserver->SendSystemError(client->GetClientNum(),"%s could not be added to buddy list.",(const char*)msg.player);
            return;
        }

        Client* buddyClient = clients->FindPlayer(buddyid);
        if(buddyClient && buddyClient->IsReady())
        {
            buddyClient->GetCharacterData()->GetBuddyMgr().AddBuddyOf(selfid);
        }

        psserver->SendSystemInfo(client->GetClientNum(),"%s has been added to your buddy list.",(const char*)msg.player);
    }
    else //If the player used remove or the buddy was found in the player list remove it from there
    {
        chardata->GetBuddyMgr().RemoveBuddy(buddyid);
        Client* buddyClient = clients->FindPlayer(buddyid);
        if(buddyClient)
        {
            psCharacter* buddyChar = buddyClient->GetCharacterData();
            if(buddyChar)
                buddyChar->GetBuddyMgr().RemoveBuddyOf(selfid);
        }

        if(!psserver->RemoveBuddy(selfid,buddyid))
        {
            psserver->SendSystemError(client->GetClientNum(),"%s is not on your buddy list.",(const char*)msg.player);
            return;
        }

        psserver->SendSystemInfo(client->GetClientNum(),"%s has been removed from your buddy list.",(const char*)msg.player);
    }

    BuddyList(client, client->GetClientNum(), true);
}

void UserManager::BuddyList(Client* client,int clientnum,bool filter)
{
    psCharacter* chardata=client->GetCharacterData();
    if(chardata==NULL)
    {
        Error2("Client for account '%s' attempted to display buddy list but has no character data!",client->GetName());
        return;
    }

    int totalBuddies = (int)chardata->GetBuddyMgr().GetBuddyList().GetSize(); //psBuddyListMsg should have as parameter a size_t. This is temporary.


    psBuddyListMsg mesg(clientnum, totalBuddies);
    // GM+ see all buddies
    if(client->GetSecurityLevel() >= GM_LEVEL)
    {
        for(int i = 0; i < totalBuddies; i++)
        {
            mesg.AddBuddy(i, chardata->GetBuddyMgr().GetBuddyList()[i].name, (clients->Find(chardata->GetBuddyMgr().GetBuddyList()[i].name)? true : false));
        }
    }
    else // Others only see non-hiding GM+
    {
        for(int i = 0; i < totalBuddies; i++)
        {
            Client* buddy = clients->Find(chardata->GetBuddyMgr().GetBuddyList()[i].name);
            mesg.AddBuddy(i, chardata->GetBuddyMgr().GetBuddyList()[i].name, (buddy&&(!buddy->GetBuddyListHide()) ? true : false));
        }
    }

    mesg.Build();
    mesg.SendMessage();
}

void UserManager::HandleClientReady(MsgEntry* me, Client* client)
{
    // This function catches the event published by AuthentServer when a client is ready for the first time
    psConnectEvent evt(me);
    if(client)
    {
        //notifies normal buddies this player has logged in
        NotifyBuddies(client, LOGGED_ON);
        //notifies guild buddies getting notifications this player has logged in
        NotifyGuildBuddies(client, LOGGED_ON);
        //notifies alliance buddies getting notifications this player has logged in
        NotifyAllianceBuddies(client, LOGGED_ON);
    }
}

void UserManager::NotifyBuddies(Client* client, bool logged_in)
{
    csString name(client->GetActor()->GetFirstName());

    for(size_t i=0; i< client->GetCharacterData()->GetBuddyMgr().GetBuddyOfList().GetSize(); i++)
    {
        Client* buddy = clients->FindPlayer(client->GetCharacterData()->GetBuddyMgr().GetBuddyOfList()[i]);    // name of player buddy

        if(buddy)   // is buddy online at the moment?  if so let him know buddy just logged on
        {
            if(client->GetBuddyListHide() && buddy->GetSecurityLevel() < GM_LEVEL)  // Don't tell none GM+ that a GM+ came online
                continue;

            psBuddyStatus status(buddy->GetClientNum(),  name , logged_in);
            status.SendMessage();

            if(logged_in)
            {
                psserver->SendSystemInfo(buddy->GetClientNum(),"%s just joined PlaneShift",name.GetData());
            }
            else
            {
                psserver->SendSystemInfo(buddy->GetClientNum(),"%s has left PlaneShift",name.GetData());
            }
        }
    }
}

void UserManager::NotifyPlayerBuddies(Client* client, bool logged_in)
{
    csString name(client->GetActor()->GetFirstName());

    for(size_t i=0; i< client->GetCharacterData()->GetBuddyMgr().GetBuddyOfList().GetSize(); i++)
    {
        Client* buddy = clients->FindPlayer(client->GetCharacterData()->GetBuddyMgr().GetBuddyOfList()[i]);    // name of player buddy

        if(buddy)   // is buddy online at the moment?  if so let him know buddy just logged on
        {
            if(buddy->GetSecurityLevel() >= GM_LEVEL)  // Don't tell GM+ as they already know
                continue;

            psBuddyStatus status(buddy->GetClientNum(),  name , logged_in);
            status.SendMessage();

            if(logged_in)
            {
                psserver->SendSystemInfo(buddy->GetClientNum(),"%s just joined PlaneShift",name.GetData());
            }
            else
            {
                psserver->SendSystemInfo(buddy->GetClientNum(),"%s has left PlaneShift",name.GetData());
            }
        }
    }
}

void UserManager::NotifyGuildBuddies(Client* client, bool logged_in)
{
    csString name(client->GetName());
    PID char_id = client->GetCharacterData()->GetPID();
    psGuildInfo* charGuild = client->GetCharacterData()->GetGuild();
    EID clientEID;

    if(client->GetActor())
        clientEID = client->GetActor()->GetEID();

    if(!charGuild)
        return;

    csArray<psGuildMember*>::Iterator mIter = charGuild->GetMemberIterator();
    while(mIter.HasNext())
    {
        psGuildMember* member = mIter.Next();
        psCharacter* notifiedmember = member->character;
        gemActor* notifiedactor = notifiedmember? notifiedmember->GetActor() : NULL;

        if(notifiedactor && notifiedmember && (member->char_id != char_id)
                && notifiedmember->IsGettingGuildNotifications())
        {
            csString text;
            if(logged_in)
                text.Format("/me just joined PlaneShift");
            else
                text.Format("/me has quit");

            psChatMessage guildmsg(notifiedactor->GetClientID(), clientEID, name.GetData(),0,text,CHAT_GUILD, false);
            guildmsg.SendMessage();
        }
    }
}

void UserManager::NotifyAllianceBuddies(Client* client, bool logged_in)
{
    csString name(client->GetName());
    psGuildInfo* charGuild = client->GetCharacterData()->GetGuild();
    EID clientEID;
    psGuildAlliance* alliance;

    if(client->GetActor())
        clientEID = client->GetActor()->GetEID();

    if(!charGuild)
        return;

    //get the alliance
    alliance = cacheManager->FindAlliance(charGuild->GetAllianceID());

    //if there is no alliance we are done
    if(!alliance)
        return;

    //iterate all alliance members
    for(size_t i = 0; i < alliance->GetMemberCount(); i++)
    {
        //get the guild of a member
        psGuildInfo* currentGuild = alliance->GetMember(i);
        //ignore this if it's missing (shouldn't happen) or it's the same of the player guild.
        //we notify only other guilds in alliance.
        if(!currentGuild || currentGuild == charGuild)
            continue;

        csArray<psGuildMember*>::Iterator mIter = currentGuild->GetMemberIterator();
        while(mIter.HasNext())
        {
            psCharacter* notifiedmember = mIter.Next()->character;
            gemActor* notifiedactor = notifiedmember? notifiedmember->GetActor() : NULL;

            if(notifiedactor && notifiedmember && notifiedmember->IsGettingAllianceNotifications())
            {
                csString text;
                if(logged_in)
                    text.Format("/me from %s just joined PlaneShift", charGuild->GetName().GetData());
                else
                    text.Format("/me has quit");

                psChatMessage alliancemsg(notifiedactor->GetClientID(), clientEID, name.GetData(),0,text,CHAT_ALLIANCE, false);
                alliancemsg.SendMessage();
            }
        }
    }
}

void UserManager::RollDice(psUserCmdMessage &msg,Client* client)
{
    int total=0;

    if(msg.dice > 100)
        msg.dice = 100;
    if(msg.sides > 10000)
        msg.sides = 10000;
    if(msg.dtarget > msg.sides)
        msg.dtarget = msg.sides;

    if(msg.dice < 1)
        msg.dice = 1;
    if(msg.sides < 1)
        msg.sides = 1;
    if(msg.dtarget < 0)
        msg.dtarget = 0;

    for(int i = 0; i<msg.dice; i++)
    {
        // must use msg.sides instead of msg.sides-1 because rand never actually
        // returns max val, and int truncation never results in max val as a result
        if(msg.dtarget)
            total += ((psserver->rng->Get(msg.sides) + 1 >= (uint)msg.dtarget)? 1: 0);
        else
            total += psserver->rng->Get(msg.sides) + 1;
    }
    if(msg.dtarget)
    {
        if(msg.dice > 1)
        {
            psSystemMessage newmsg(client->GetClientNum(),MSG_INFO_BASE,
                                   "Player %s has rolled %d %d-sided dice and had %d of them come up %d or greater.",
                                   client->GetName(),
                                   msg.dice,
                                   msg.sides,
                                   total,
                                   msg.dtarget);

            newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
        }
        else
        {
            psSystemMessage newmsg(client->GetClientNum(),MSG_INFO_BASE,((total)?
                                   "Player %s has rolled a %d-sided dice and had it come up %d or greater.":
                                   "Player %s has rolled a %d-sided dice and had it come up less than %d."),
                                   client->GetName(),
                                   msg.sides,
                                   msg.dtarget);

            newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
        }
    }
    else
    {
        if(msg.dice > 1)
        {
            psSystemMessage newmsg(client->GetClientNum(),MSG_INFO_BASE,
                                   "Player %s has rolled %d %d-sided dice for a %d.",
                                   client->GetName(),
                                   msg.dice,
                                   msg.sides,
                                   total);

            newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
        }
        else
        {
            psSystemMessage newmsg(client->GetClientNum(),MSG_INFO_BASE,
                                   "Player %s has rolled a %d-sided die for a %d.",
                                   client->GetName(),
                                   msg.sides,
                                   total);

            newmsg.Multicast(client->GetActor()->GetMulticastClients(),0, 10);
        }
    }
}

void UserManager::ReportPosition(psUserCmdMessage &msg,Client* client)
{

    if(msg.player.StartsWith("area:"))
    {
        csArray<csString> filters = DecodeCommandArea(client, msg.player);
        csArray<csString>::Iterator it(filters.GetIterator());
        while(it.HasNext())
        {
            msg.player = it.Next();
            ReportPosition(msg, client);
        }
        return;
    }

    gemObject* object = NULL;
    bool self = true;

    bool extras = psserver->CheckAccess(client, "pos extras", false);

    // Allow GMs to get other players' and entities' locations
    if(extras && msg.player.Length())
    {
        self = false;

        if(msg.player == "target")
        {
            object = client->GetTargetObject();
            if(!object)
            {
                psserver->SendSystemError(client->GetClientNum(), "You must have a target selected.");
                return;
            }
        }
        else
        {
            Client* c = FindPlayerClient(msg.player);
            if(c) object = (gemObject*)c->GetActor();
            if(!object) object = FindObjectByString(msg.player,client->GetActor());
        }
    }
    else
        object = client->GetActor();

    if(object)
    {
        csVector3 pos;
        iSector* sector = 0;
        float angle;

        object->GetPosition(pos, angle, sector);
        InstanceID instance = object->GetInstance();

        csString sector_name = (sector) ? sector->QueryObject()->GetName() : "(null)";

        csString name,range;
        if(self)
        {
            name = "Your";
            range = "";
        }
        else
        {
            float dist = 0.0;
            csVector3 char_pos;
            float char_angle;
            iSector* char_sector;
            client->GetActor()->GetPosition(char_pos,char_angle,char_sector);
            dist = entityManager->GetWorld()->Distance(char_pos,char_sector,pos,sector);
            name.Format("%s's",object->GetName());
            range.Format(", range: %.2f",dist);
        }

        // Report extra info to GMs (players will use skills to determine correct direction)
        if(extras)
        {
            // Get the region this sector belongs to
            csString region_name = (sector) ? sector->QueryObject()->GetObjectParent()->GetName() : "(null)";
            // If it's an actor, append their PID to the output.
            csString idtxt;
            if(object->GetActorPtr())
                idtxt.Format(", %s", ShowID(object->GetPID()));

            int degrees = (int)(angle*180.0/PI);
            psserver->SendSystemInfo(client->GetClientNum(),
                                     "%s current position is region: %s %1.2f %1.2f %1.2f %d angle: %d (%f), sector : %s%s%s",
                                     name.GetData(), region_name.GetData(), pos.x, pos.y, pos.z, instance, degrees, angle,
                                     sector_name.GetData(), range.GetData(), idtxt.GetDataSafe());
        }
        else
        {
            psserver->SendSystemInfo(client->GetClientNum(),"%s current position is %s %1.2f %1.2f %1.2f, instance: %d",
                                     name.GetData(), sector_name.GetData(), pos.x, pos.y, pos.z, instance);
        }
    }
}

void UserManager::HandleUnstick(psUserCmdMessage &msg, Client* client)
{
    gemActor* actor = client->GetActor();
    if(!actor)
        return;

    StopAllCombat(client);
    LogStuck(client);

    if(actor->MoveToValidPos())
    {
        csVector3 pos;
        iSector* sector;

        actor->GetPosition(pos, sector);
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "Moving back to valid position in sector %s...",
                                 sector->QueryObject()->GetName());
    }
    else
    {
        int timeRemaining = (UNSTICK_TIME - int(csGetTicks() - actor->GetFallStartTime())) / 1000;
        if(actor->IsFalling() && timeRemaining > 0)
            psserver->SendSystemError(client->GetClientNum(), "You cannot /unstick yet - please wait %d %s and try again.", timeRemaining, timeRemaining == 1 ? "second" : "seconds");
        else
            psserver->SendSystemError(client->GetClientNum(), "You cannot /unstick at this time.");
    }
}

void UserManager::LogStuck(Client* client)
{
    csVector3 pos;
    float yrot;
    iSector* sector;

    if(!client || !client->GetActor())
        return;

    client->GetActor()->GetPosition(pos, yrot, sector);
    psRaceInfo* race = client->GetActor()->GetCharacterData()->GetRaceInfo();

    csString buffer;
    buffer.Format("%s, %s, %d, %s, %.3f, %.3f, %.3f, %.3f", client->GetName(),
                  race->GetName(), race->gender,
                  sector->QueryObject()->GetName(), pos.x, pos.y, pos.z, yrot);
    psserver->GetLogCSV()->Write(CSV_STUCK, buffer);
}

void UserManager::StopAllCombat(Client* client)
{
    if(!client || !client->GetActor())
        return;

    if(client->GetDuelClientCount() > 0)
        YieldDuel(client);
    else
        psserver->GetCombatManager()->StopAttack(client->GetActor());
}

void UserManager::HandleAttack(psUserCmdMessage &msg,Client* client)
{
    Stance stance;

    //it is important to check if actor is in combat mode already when trying to raise/lower stance
    if(msg.stance == "raise" && client->GetActor()->GetMode() == PSCHARACTER_MODE_COMBAT)
        stance = CombatManager::GetRaisedActorStance(cacheManager, client->GetActor());
    else if(msg.stance == "lower" && client->GetActor()->GetMode() == PSCHARACTER_MODE_COMBAT)
        stance = CombatManager::GetLoweredActorStance(cacheManager, client->GetActor());
    else
        stance = CombatManager::GetStance(cacheManager,msg.stance);

    Attack(stance, client);
}

void UserManager::HandleQueueAttack(psUserCmdMessage& msg, Client *client)
{
    psAttack* attack = cacheManager->GetAttackByName(msg.attack);
    if(attack)
    {
        if(attack->CanAttack(client))
        {
            gemActor* attacker = client->GetActor();
            if(attacker->GetMode() != PSCHARACTER_MODE_COMBAT)
            {
                const Stance& stance(psserver->GetCombatManager()->GetStance(psserver->GetCacheManager(), "Normal"));
                Attack(stance, client);
                if(attacker->GetMode() != PSCHARACTER_MODE_COMBAT)
                    return;
            }

            psCharacter* character = client->GetCharacterData();
            if(!character->GetAttackQueue()->Push(attack))
            {
                psserver->SendSystemError(client->GetClientNum(),"The attack queue is currently full");
                return;
            }
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(),"You do not meet the requirements of that attack.");
        }
    }
    else
        psserver->SendSystemError(client->GetClientNum(),"That attack does not exist");
}

void UserManager::HandleStopAttack(psUserCmdMessage &msg,Client* client)
{
    psserver->GetCombatManager()->StopAttack(client->GetActor());
}

void UserManager::Attack(Stance stance, Client* client)
{
    if(!client->IsAlive() || client->GetActor()->IsFrozen())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot fight now.");
        return;
    }

    csString msg;

    gemActor* target = dynamic_cast<gemActor*>(client->GetTargetObject());
    if(client->GetActor()->IsAllowedToAttack(target, msg))
    {
        psserver->GetCombatManager()->AttackSomeone(client->GetActor(), target, stance);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), msg);
    }

}

void UserManager::Assist(psUserCmdMessage &msg, Client* client)
{
    Client* targetClient = NULL;

    if(!client->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot assist anybody now.");
        return;
    }

    // If the player doesn't provide an argument, use the players current
    // target instead.
    if(msg.player.IsEmpty())
    {
        int currentTarget = client->GetTargetClientID();
        if(currentTarget == -1)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You have no target selected.");
            return;
        }

        if(currentTarget == 0)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You can assist other players only.");
            return;
        }

        targetClient = clients->Find(currentTarget);
        if(targetClient == NULL)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "Internal error - client not found.");
            return;
        }
    }
    else
    {
        csString playerName = NormalizeCharacterName(msg.player);
        targetClient = clients->Find(playerName);

        if(!targetClient)
        {
            psserver->SendSystemInfo(client->GetClientNum(),"Specified player is not online.");
            return;
        }
    }

    if(targetClient == client)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You cannot assist yourself.");
        return;
    }

    if(!client->GetActor()->IsNear(targetClient->GetActor(), ASSIST_MAX_DIST))
    {
        psserver->SendSystemInfo(client->GetClientNum(),"Specified player is too far away.");
        return;
    }

    // bug PS#2402 - Command /assist can be used to determine enemy's target.
    // Add check for actors in the same guild.
    if((client->GetGuildID() == 0) || (client->GetGuildID() != targetClient->GetGuildID()))
    {
        // Add check for grouped actors
        int nGID = client->GetActor()->GetGroupID();
        if((nGID == 0) || (nGID != targetClient->GetActor()->GetGroupID()))
        {
            psserver->SendSystemInfo(client->GetClientNum(),"You can only assist members of your guild or players you're grouped with.");
            return;
        }
    }

    gemObject* targetObject = targetClient->GetTargetObject();
    if(!targetObject)
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "Specified player has no target selected.");
        return;
    }

    client->SetTargetObject(targetObject, true);
}

void UserManager::UserStatRegeneration()
{
    gem->UpdateAllStats();

    // Push a new event
    psUserStatRegeneration* event;
    event = new psUserStatRegeneration(this,csGetTicks() + 1000);
    psserver->GetEventManager()->Push(event);
}

void UserManager::Ready()
{
    UserStatRegeneration();
}

/*
* Check target dead
* Check target lootable by this client
*/
bool UserManager::CheckTargetLootable(gemActor* actor, Client* client)
{
    if(!actor->IsAlive())
    {
        if(client)
            psserver->SendSystemError(client->GetClientNum(), "You are dead, you cannot loot now.");
        else
            Debug1(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "Cannot loot when dead.");
        return false;
    }

    gemObject* target = actor->GetTargetObject();
    if(!target)
    {
        if(client)
            psserver->SendSystemError(client->GetClientNum(), "You don't have a target selected.");
        else
            Debug1(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "No target selected for looting.");
        return false;
    }

    gemNPC* npc = target->GetNPCPtr();
    if(!npc)
    {
        if(client)
        {
            gemActor* targetActor = target->GetActorPtr();
            if(targetActor && targetActor->GetClient())
            {
                if(client->GetClientNum() == targetActor->GetClient()->GetClientNum())
                {
                    psserver->SendSystemError(client->GetClientNum(), "You can't loot yourself.");
                    return false;
                }

                if(targetActor->IsAlive())
                {
                    psserver->SendSystemError(client->GetClientNum(), "You can't loot a person that is alive.");
                    return false;
                }
            }
            psserver->SendSystemError(client->GetClientNum(), "You can only loot NPCs.");
        }
        else
            Debug1(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "Only NPCs are lootable.");
        return false;
    }


    if(target->IsAlive())
    {
        if(client)
            psserver->SendSystemError(client->GetClientNum(), "You can't loot a creature that is alive.");
        else
            Debug1(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "Cannot loot creature that is alive.");
        return false;
    }

    // Check target lootable by this player
    if(!npc->IsLootablePlayer(actor->GetPID()))
    {
        if(client)
        {
            Debug2(LOG_USER, client->GetClientNum(), "Client %d tried to loot mob that wasn't theirs.\n", client->GetClientNum());
            psserver->SendSystemError(client->GetClientNum(),"You are not allowed to loot %s.", target->GetName());
        }
        else
            Debug2(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "Not allowed to loot %s.", target->GetName());

        return false;
    }

    // Check to make sure loot is in range.
    if(actor->RangeTo(target) > RANGE_TO_LOOT)
    {
        if(client)
            psserver->SendSystemError(client->GetClientNum(), "Too far away to loot %s", target->GetName());
        else
            Debug2(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "Too far away to loot %s", target->GetName());
        return false;
    }
    // Target is lootable
    return true;
}

void UserManager::HandleLoot(psUserCmdMessage &msg,Client* client)
{
    gemActor* actor = client->GetActor();

    // check whether target is dead and lootable
    if(!CheckTargetLootable(actor, client))
        return;

    if(msg.action.CompareNoCase("money"))
    {
        LootMoney(actor, client);
    }
    else if(msg.action.CompareNoCase("items"))
    {
        LootMoney(actor, client);
        LootItems(actor, client, msg.text, msg.dice); // dice is either 0 (==LOOT_SELF) or 1 (==LOOT_ROLL)
    }
    else if(msg.action.CompareNoCase("all"))
    {
        LootMoney(actor, client);
        LootItems(actor, client, "all", msg.dice);
    }
    else if(msg.action == "")
    {
        // Loot money and display all lootable items
        LootMoney(actor, client);
        Loot(client);
    }
    else
        psserver->SendSystemInfo(client->GetClientNum(), "Usage: /loot [roll] [all|money|items <categories>]");
}

/**
* Loot all the money
*/
void UserManager::LootMoney(gemActor* actor, Client* client)
{
    gemObject* target = actor->GetTargetObject();
    psCharacter* chr = target->GetCharacterData();
    if(chr)
    {
        int money = chr->GetLootMoney();

        if(!money)
        {
            if(client)
            {
                Debug1(LOG_LOOT, client->GetClientNum(), "Mob has no money to be looted.\n");
                psserver->SendSystemError(client->GetClientNum(), "%s has no money to be looted.", target->GetName());
            }
            else
                Debug2(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "%s has no money to be looted.", target->GetName());
            return;
        }

        csRef<PlayerGroup> group = actor->GetGroup();
        int remainder,each;
        // Split up money among LootablePlayers in group.
        if(group && client) // NPCs don't care about group looting
        {
            Debug2(LOG_LOOT, client->GetClientNum(),"Splitting up %d money.\n", money);
            // Locate the group members who are in range to loot the trias
            csArray<gemActor*> closegroupmembers;
            const csArray<PID> &lootablePlayers = target->GetNPCPtr()->GetLootablePlayers();
            size_t membercount = group->GetMemberCount();
            for(size_t i = 0 ; i < membercount ; i++)
            {
                gemActor* currmember = group->GetMember(i);
                if(!currmember)
                {
                    continue;
                }
                //Copy all lootable players.
                //TODO: a direct interface to gemNPC::lootablePlayers would be better
                if(lootablePlayers.Contains(currmember->GetPID()) != csArrayItemNotFound)
                {
                    closegroupmembers.Push(currmember);
                }
            }

            remainder = money % (int) closegroupmembers.GetSize();
            each      = money / (int) closegroupmembers.GetSize();
            psMoney eachmoney = psMoney(each).Normalized();
            psMoney remmoney  = psMoney(remainder).Normalized();
            csString remstr   = remmoney.ToUserString();
            csString eachstr  = eachmoney.ToUserString();

            if(each)
            {
                psSystemMessage loot(client->GetClientNum(), MSG_LOOT, "Everyone nearby has looted %s.", eachstr.GetData());
                client->GetActor()->SendGroupMessage(loot.msg);

                // Send a personal message to each group member about the money
                for(size_t i = 0; i < membercount; i++)
                {
                    gemActor* currmember = group->GetMember(i);
                    if(closegroupmembers.Contains(currmember) != csArrayItemNotFound)
                    {
                        //Normal loot is not shown in yellow. Until that happens, do not do it for few coins
                        //psserver->SendSystemResult(currmember->GetClient()->GetClientNum(), "You have looted %s.", eachstr.GetData());
                        psserver->SendSystemInfo(currmember->GetClient()->GetClientNum(), "You have looted %s.", eachstr.GetData());

                        psLootEvent evt(
                            chr->GetPID(),
                            chr->GetCharName(),
                            currmember->GetCharacterData()->GetPID(),
                            currmember->GetCharacterData()->GetCharName(),
                            0, "N/A", 0, 0, eachmoney.GetTotal());
                        evt.FireEvent();
                    }
                    else
                    {
                        // Something less intrusive for players who were too far away
                        psserver->SendSystemInfo(currmember->GetClient()->GetClientNum(), "You were too far away to loot.");
                    }

                }

                for(unsigned int i = 0; i < closegroupmembers.GetSize(); i++)
                {
                    closegroupmembers[i]->GetCharacterData()->AdjustMoney(eachmoney, false);
                    psserver->GetCharManager()->SendPlayerMoney(closegroupmembers[i]->GetClient());
                }

            }
            if(remainder)
            {
                psSystemMessage loot2(client->GetClientNum(), MSG_LOOT, "You have looted an extra %s.", remstr.GetData());
                loot2.SendMessage();

                client->GetCharacterData()->AdjustMoney(remmoney, false);
                psserver->GetCharManager()->SendPlayerMoney(client);
                psLootEvent evt(chr->GetPID(),
                                chr->GetCharName(),
                                client->GetCharacterData()->GetPID(),
                                client->GetCharacterData()->GetCharName(),
                                0, "N/A", 0, 0, remmoney.GetTotal());
                evt.FireEvent();
            }
        }
        // No group present, just take all the money
        else
        {
            psMoney m = psMoney(money).Normalized();
            csString mstr = m.ToUserString();

            if(client)
            {
                //Debug2(LOG_LOOT, client->GetClientNum(),"Looting %d money.\n", money); // Needed?
                psSystemMessage loot(client->GetClientNum(), MSG_LOOT, "You have looted %s.", mstr.GetData());
                loot.SendMessage();
            }
            else
                Debug2(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "You have looted %s.", mstr.GetData());

            actor->GetCharacterData()->AdjustMoney(m, false);

            if(client)
            {
                psserver->GetCharManager()->SendPlayerMoney(client);

                psLootEvent evt(chr->GetPID(),
                                chr->GetCharName(),
                                client->GetCharacterData()->GetPID(),
                                client->GetCharacterData()->GetCharName(),
                                0, "N/A", 0, 0, m.GetTotal());
                evt.FireEvent();
            }
        }
    }
}

/**
* Loot all items
*/
void UserManager::LootItems(gemActor* actor, Client* client, csString userInput, uint8_t lootAction)
{
    gemObject* target = actor->GetTargetObject();
    psCharacter* chr = target->GetCharacterData();

    if(chr)
    {
        csArray<csString> categories;

        if(!userInput.CompareNoCase("") && !userInput.CompareNoCase("all"))
        {
            // Slice user inputted categories into an array
            userInput    = userInput.Append(','); // add addiontal coma for loop
            size_t start = 0;
            size_t end   = userInput.Find(",", start);

            while(end != csArrayItemNotFound)
            {
                categories.Push(userInput.Slice(start, (end - start)).Collapse().Downcase());
                start = end + 1;
                end   = userInput.Find(",", start);
            }
        }

        csArray<psItem*> items = chr->RemoveLootItems(categories);

        if(!items.IsEmpty())
        {
            for(size_t x = 0; x < items.GetSize(); x++)
            {
                // if possible, adds item to inventory
                if(client)
                    handleGroupLootItem(items[x], target->GetActorPtr(), client, cacheManager, gem, lootAction);
                else
                {
                    items[x]->SetLoaded();
                    Debug2(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "Looted a %s.", items[x]->GetName());
                    // Attempt to give to looting npc
                    actor->GetCharacterData()->Inventory().AddOrDrop(items[x]);
                    items[x]->Save(false);
                }
            }
        }
        else
        {
            if(client)
                psserver->SendSystemError(client->GetClientNum(), "%s has no items to be looted.", target->GetName());
            else
                Debug2(LOG_SUPERCLIENT, actor->GetEID().Unbox(), "%s has no items to be looted.", target->GetName());
        }

    }
}

/**
* Return lootable items list if present
*/
void UserManager::Loot(Client* client)
{
    // Return lootable items list if present
    gemObject* target = client->GetTargetObject();
    psCharacter* chr = target->GetCharacterData();
    if(chr)
    {
        // Send items to looting player
        psLootMessage loot;
        size_t count = chr->GetLootItems(loot,
                                         target->GetEID(),
                                         client->GetClientNum());
        if(count)
        {
            Debug3(LOG_LOOT, client->GetClientNum(), "Sending %zu loot items to %s.\n", count, client->GetActor()->GetName());
            loot.SendMessage();
        }
        else
        {
            Debug1(LOG_LOOT, client->GetClientNum(), "Mob doesn't have loot.\n");
            psserver->SendSystemError(client->GetClientNum(), "%s has no items to be looted.", target->GetName());
        }
    }
}

void UserManager::HandleQuests(Client* client)
{
    psQuestListMessage quests;
    size_t count = client->GetActor()->GetCharacterData()->GetQuestMgr().GetAssignedQuests(quests,client->GetClientNum());

    if(count)
    {
        Debug3(LOG_QUESTS, client->GetClientNum(), "Sending %zu quests to player %s.\n", count, client->GetName());
        quests.SendMessage();
    }
    else
    {
        Debug1(LOG_QUESTS, client->GetClientNum(),"Client has no quests yet.\n");
    }
}

void UserManager::HandleGMEvents(Client* client)
{

    psGMEventListMessage gmEvents;
    size_t count = client->GetActor()->GetCharacterData()->GetAssignedGMEvents(gmEvents, client->GetClientNum());

    if(count)
    {
        Debug3(LOG_QUESTS, client->GetClientNum(),
               "Sending %zu events to player %s.\n",
               count, client->GetName());
        gmEvents.SendMessage();
    }
    else
    {
        Debug1(LOG_QUESTS,
               client->GetClientNum(),
               "Client is not participating in a GM-Event.\n");
    }
}

void UserManager::HandleTraining(psUserCmdMessage &msg, Client* client)
{
    if(!client->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot train your skills now");
        return;
    }

    // Check target is a Trainer
    gemObject* target = client->GetTargetObject();
    if(!target || !target->GetActorPtr())
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "No target selected for training!");
        return;
    }

    // Check range
    if(client->GetActor()->RangeTo(target) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "You are not in range to train with %s.",target->GetCharacterData()->GetCharName());
        return;
    }

    if(!target->IsAlive())
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "Can't train with a dead trainer!");
        return;
    }

    if(client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        csString err;
        err.Format("You can't train while %s.", client->GetActor()->GetModeStr());
        psserver->SendSystemInfo(client->GetClientNum(), err);
        return;
    }

    psCharacter* trainer = target->GetCharacterData();
    if(!trainer->IsTrainer())
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "%s isn't a trainer.",target->GetCharacterData()->GetCharName());
        return;
    }

    psserver->GetProgressionManager()->StartTraining(client,trainer);
}

void UserManager::HandleBanking(psUserCmdMessage &msg, Client* client)
{
    // Check if target is a banker.
    gemObject* target = client->GetTargetObject();
    if(!target || !target->GetActorPtr() || !target->GetActorPtr()->GetCharacterData()->IsBanker())
    {
        psserver->SendSystemError(client->GetClientNum(), "Your target must be a banker!");
        return;
    }

    // Check range
    if(client->GetActor()->RangeTo(target) > RANGE_TO_SELECT)
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You are not within range to interact with %s.",target->GetCharacterData()->GetCharName());
        return;
    }

    // Check that the banker is alive!
    if(!target->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(), "You can't interact with a dead banker!");
        return;
    }

    // Make sure that we're not busy doing something else.
    if(client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        csString err;
        err.Format("You can't access your bank account while %s.", client->GetActor()->GetModeStr());
        psserver->SendSystemError(client->GetClientNum(), err);
        return;
    }

    // Check which account we're trying to access.
    if(msg.action.CompareNoCase("personal"))
    {
        // Open personal bank.
        bankManager->StartBanking(client, false);
        return;
    }
    else if(msg.action.CompareNoCase("guild"))
    {
        // Open guild bank.
        bankManager->StartBanking(client, true);
        return;
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "Usage: /bank [personal|guild]");
        return;
    }
}

void UserManager::HandleTakeAll(psUserCmdMessage &msg, Client* client)
{
    // Check the client has targeted a container.
    gemObject* object = client->GetTargetObject();
    if(object && (object->GetALPtr() || object->GetItem()))
    {
        object->SendBehaviorMessage("takeall", client->GetActor());
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                                  "You need to target a container");
}

void UserManager::HandleTakeStackAll(psUserCmdMessage &msg, Client* client)
{
    // Check the client has targeted a container.
    gemObject* object = client->GetTargetObject();
    if(object && (object->GetALPtr() || object->GetItem()))
    {
        object->SendBehaviorMessage("takestackall", client->GetActor());
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                                  "You need to target a container");
}

void UserManager::HandlePickup(psUserCmdMessage &msg, Client* client)
{
    Pickup(client, msg.target);
}

void UserManager::Pickup(Client* client, csString target)
{
    if(target.StartsWith("area:"))
    {
        csArray<csString> filters = DecodeCommandArea(client, target);
        csArray<csString>::Iterator it(filters.GetIterator());
        while(it.HasNext())
        {
            Pickup(client, it.Next());
        }
        return;
    }
    else if(target.StartsWith("eid:", true))
    {
        gemObject* object= NULL;
        csString eid_str = target.Slice(4);
        EID eID = EID(strtoul(eid_str.GetDataSafe(), NULL, 10));
        if(eID.IsValid())
        {
            object = gem->FindObject(eID);
            if(object)
            {
                if(object->GetItem())
                    object->SendBehaviorMessage("pickup", client->GetActor());
                else
                    psserver->SendSystemError(client->GetClientNum(),
                                              "You can't pickup objects which aren't items");
            }
            else
            {
                psserver->SendSystemError(client->GetClientNum(),
                                          "Object not found %s", target.GetData());
            }
        }
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                                  "Object not found %s", target.GetData());
}

void UserManager::HandleMount(psUserCmdMessage &msg, Client* client)
{
    // Add some check to see if the player is allowed to mount
    if(client->GetActor()->GetMode() == PSCHARACTER_MODE_OVERWEIGHT)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot mount while carrying too much.");
        return;
    }

    if(client->GetActor()->IsFalling())
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot mount while falling.");
        return;
    }

    gemObject* mount;

    if(msg.target.Length() < 5)
    {
        if(client->GetActor()->GetMount())
            HandleUnmount(msg,client);
        else
            psserver->SendSystemError(client->GetClientNum(), "Can't find object to mount.");
        return;
    }

    csString eid_str = msg.target.Slice(4);
    EID targetEID = EID(strtoul(eid_str.GetDataSafe(), NULL, 10));

    if(!targetEID.IsValid())
        return;

    mount = gem->FindObject(targetEID);

    // can only mount mounts
    if(!mount || !mount->GetActorPtr() || client->GetActor() == mount
            || (!mount->GetCharacterData()->IsMount() && (!client->IsGM() || !mount->GetCharacterData()->IsNPC()))) //remove that last test to allow for player mounting
    {
        if(mount)
            psserver->SendSystemError(client->GetClientNum(),
                                      "Can't mount %s.", mount->GetName());
        else
            psserver->SendSystemError(client->GetClientNum(),
                                      "Can't find object to mount.");
        return;
    }

    if(mount->GetCharacterData()->IsPet() && client->GetPID() != mount->GetCharacterData()->GetOwnerID())
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You can't mount %s. It's not yours.", mount->GetName());
        return;
    }

    if(!client->GetActor()->IsNear(mount, RANGE_TO_USE, true))
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You are too far away from the mount.");
        return;
    }

    if(!mount->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You cannot ride a dead corpse.");
        return;
    }

    /*Client *targetClient = mount->GetClient();
    if(targetClient)
    {
        csString question;
        question.Format("You have been asked to be %s's mount. Do you accept?",
            client->GetName() );
        PendingMountInvite *invite = new PendingMountInvite(client,
            targetClient,
            question);
        psserver->questionmanager->SendQuestion(invite);
        return;
    }*/

    // If you are not the rider(passenger), you shouldn't be allowed to move
    // client->GetActor()->SetAllowedToMove(false);

    if(client->GetActor()->GetMount())
        HandleUnmount(msg,client);
    Mount(client->GetActor(), mount->GetActorPtr());

    return;
}

void PendingMountInvite::HandleAnswer(const csString &answer)
{
    Client* client = psserver->GetConnections()->Find(clientnum);
    if(!client)
    {
        return;
    }
    gemActor* mount = client->GetActor();
    if(!mount)
    {
        return;
    }
    Client* inviterClient = psserver->GetConnections()->Find(inviterClientNum);
    if(!inviterClient)
    {
        return;
    }
    gemActor* rider = inviterClient->GetActor();
    if(!rider)
    {
        return;
    }

    PendingInvite::HandleAnswer(answer);

    if(answer == "yes")
    {
        psserver->usermanager->Mount(rider, mount);
    }
}

void UserManager::Mount(gemActor* rider, gemActor* mount)
{
    csString mountName = mount->GetName();
    if(entityManager->AddRideRelation(rider, mount))
        psserver->SendSystemOK(rider->GetClientID(),
                               "You are riding %s", mountName.GetDataSafe());
    else
        psserver->SendSystemError(rider->GetClientID(),
                                  "Could not mount %s", mountName.GetDataSafe());

    return;
}

void UserManager::HandleUnmount(psUserCmdMessage &msg, Client* client)
{
    //client->GetActor()->SetAllowedToMove(true);

    if(client->GetActor()->IsFalling())
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot unmount while falling.");
        return;
    }

    if(!client->GetActor()->GetMount())
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You are not on a mount.");
        return;
    }

    entityManager->RemoveRideRelation(client->GetActor());

    return;
}

void UserManager::HandleGuard(psUserCmdMessage &msg, Client* client)
{
    gemObject* object;
    if(msg.target.StartsWith("area:"))
    {
        csArray<csString> targetList = DecodeCommandArea(client, msg.target);
        csArray<csString>::Iterator targets(targetList.GetIterator());
        while(targets.HasNext())
        {
            object = FindObjectByString(targets.Next(), client->GetActor());
            Guard(client, object, msg.action);
        }
    }
    else if(msg.target.StartsWith("eid:"))
    {
        object = FindObjectByString(msg.target, client->GetActor());
        Guard(client, object, msg.action);
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                                  "Unknown target : %s", msg.target.GetData());
}

void UserManager::Guard(Client* client, gemObject* object, csString action)
{
    bool onoff = false;
    bool toggle = false;
    if(action == "on")          //The player provided an on so set ignore
        onoff = true;
    else if(action == "off")  //The player provided an off so unset ignore
        onoff = false;
    else                           //The player didn't provide anything so toggle the option
        toggle = true;

    if(!object)
        return;

    psItem* guardItem = object->GetItem();

    if(guardItem)
    {
        if(onoff || (toggle && guardItem->GetGuardingCharacterID() == 0))
        {
            // TODO : Add that check in the security table
            if(client->GetSecurityLevel() > GM_LEVEL_2)
            {
                guardItem->SetGuardingCharacterID(client->GetPID());
                psserver->SendSystemError(client->GetClientNum(), "You have guarded %s", object->GetName());
            }
            else
                psserver->SendSystemError(client->GetClientNum(), "You can't guard %s", object->GetName());
        }
        else
        {
            if(guardItem->GetGuardingCharacterID() == client->GetPID() ||
               client->GetSecurityLevel() > GM_LEVEL_2)
            {
                guardItem->SetGuardingCharacterID(0);
                psserver->SendSystemError(client->GetClientNum(), "You have unguarded %s", object->GetName());
            }
            else
                psserver->SendSystemError(client->GetClientNum(), "You can't unguard %s", object->GetName());
        }
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                                  "Item not found %s", object->GetName());
}

void UserManager::HandleRotate(psUserCmdMessage &msg, Client* client)
{
    gemObject* rotationTarget;
    if(msg.target.StartsWith("area:"))
    {
        csArray<csString> targetList = DecodeCommandArea(client, msg.target);
        csArray<csString>::Iterator targets(targetList.GetIterator());
        while(targets.HasNext())
        {
            rotationTarget = FindObjectByString(targets.Next(), client->GetActor());
            Rotate(client, rotationTarget, msg.action);
        }
        return;
    }
    else if(msg.target.StartsWith("eid:"))
    {
        rotationTarget = FindObjectByString(msg.target, client->GetActor());
        Rotate(client, rotationTarget, msg.action);
    }
    else
        psserver->SendSystemError(client->GetClientNum(),
                                  "Unknown target : %s", msg.target.GetData());
}

void UserManager::Rotate(Client* client, gemObject* target, csString action)
{
    gemItem* rotItem = NULL;
    // only rotate the object if it's an item
    rotItem = dynamic_cast<gemItem*>(target);

    if(rotItem)
    {
        // rotate an item only if the client is guarding it,
        // or has the right to rotate all items
        if(!(rotItem->GetItem()->GetGuardingCharacterID() == client->GetPID()) &&
                !psserver->CheckAccess(client, "rotate all"))
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You can't rotate %s", rotItem->GetItem()->GetName());
            return;
        }

        float oldxrot, oldyrot, oldzrot;
        rotItem->GetRotation(oldxrot, oldyrot, oldzrot);
        // rotation is stored in radians,
        // we are converting the angles to degrees
        oldxrot = oldxrot*180/PI;
        oldyrot = oldyrot*180/PI;
        oldzrot = oldzrot*180/PI;
        // the specified rotation is added to the item's current rotation
        float xrot=oldxrot, yrot=oldyrot, zrot=oldzrot;
        WordArray words(action);
        if(words[0] == "x")
        {
            if(words[1] != "reset")
                xrot = oldxrot + atof(words[1]);
            else
                xrot = 0;
        }
        else if(words[0] == "y")
        {
            if(words[1] != "reset")
                yrot = oldyrot + atof(words[1]);
            else
                yrot = 0;
        }
        else if(words[0] == "z")
        {
            if(words[1] != "reset")
                zrot = oldzrot + atof(words[1]);
            else
                zrot = 0;
        }
        else
        {
            if(words[0] != "reset")
                xrot = oldxrot + atof(words[0]);
            else
                xrot = 0;
            if(words[1] != "reset")
                yrot = oldyrot + atof(words[1]);
            else
                yrot = 0;
            if(words[2] != "reset")
                zrot = oldzrot + atof(words[2]);
            else
                zrot = 0;
        }
        // rotation is given in degrees, converting that to radians
        xrot = xrot/180*PI;
        yrot = yrot/180*PI;
        zrot = zrot/180*PI;
        rotItem->SetRotation(xrot, yrot, zrot);
        rotItem->UpdateProxList(true);
        psserver->SendSystemInfo(client->GetClientNum(), "You have rotated %s", rotItem->GetItem()->GetName());
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "Item not found.");
    }
}

void UserManager::GiveTip(psUserCmdMessage &msg, Client* client)
{
    unsigned int max=cacheManager->GetTipLength();
    unsigned int rnd = psserver->rng->Get(max);

    csString tip;
    cacheManager->GetTipByID(rnd, tip);
    psserver->SendSystemInfo(client->GetClientNum(),tip.GetData());
}

void UserManager::GiveMOTD(psUserCmdMessage &msg, Client* client)
{
    psserver->SendSystemInfo(client->GetClientNum(),psserver->GetMOTD());
}

void UserManager::ShowNpcMenu(psUserCmdMessage &msg, Client* client)
{
    gemNPC* npc = dynamic_cast<gemNPC*>(client->GetTargetObject());
    if(npc)
    {
        if(!client->GetActor()->IsNear(npc, MAX_NPC_DIALOG_DIST, true))
        {
            psserver->SendSystemError(client->GetClientNum(), "You are too far away from the NPC.");
            return;
        }
        npc->ShowPopupMenu(client);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "You must select an NPC first.");
    }
}

void UserManager::ChallengeToDuel(psUserCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    if(!client->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(),"You are dead, you cannot challenge opponents now");
        return;
    }

    //don't allow frozen clients to challenge
    if(client->GetActor()->IsFrozen())
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You can't challenge opponents while being frozen.");
        return;
    }


    // Check target dead
    gemObject* target = client->GetTargetObject();
    if(!target)
    {
        psserver->SendSystemError(clientnum,"You don't have another player targeted.");
        return;
    }

    Client* targetClient = psserver->GetNetManager()->GetClient(target->GetClientID());
    if(!targetClient)
    {
        psserver->SendSystemError(clientnum, "You can challenge other players only");
        return;
    }

    // Distance
    if(client->GetActor()->RangeTo(target) > RANGE_TO_CHALLENGE)
    {
        psserver->SendSystemError(clientnum,target->GetName() + csString(" is too far away"));
        return;
    }

    if(!target->IsAlive())
    {
        psserver->SendSystemError(clientnum, "You can't challenge a dead person");
        return;
    }

    if(targetClient->GetActor()->IsFrozen())
    {
        psserver->SendSystemInfo(client->GetClientNum(), "% was frozen and cannot be challenged", target->GetName());
        return;
    }

    if(targetClient == client)
    {
        psserver->SendSystemError(clientnum, "You can't challenge yourself.");
        return;
    }

    // Check for pre-existing duel with this person
    if(client->IsDuelClient(target->GetClientID()))
    {
        psserver->SendSystemError(clientnum, "You have already agreed on this duel !");
        return;
    }

    // Challenge
    csString question;
    question.Format("%s has challenged you to a duel!  Click on Accept to allow the duel or Reject to ignore the challenge.",
                    client->GetName());
    PendingDuelInvite* invite = new PendingDuelInvite(client,
            targetClient,
            question);
    psserver->questionmanager->SendQuestion(invite);
}

void UserManager::AcceptDuel(PendingDuelInvite* invite)
{
    Client* inviteeClient = clients->Find(invite->clientnum);
    Client* inviterClient = clients->Find(invite->inviterClientNum);

    if(inviteeClient!=NULL  &&  inviterClient!=NULL)
    {
        inviteeClient->AddDuelClient(invite->inviterClientNum);
        inviterClient->AddDuelClient(invite->clientnum);

        // Target eachother and update their GUIs
        inviteeClient->SetTargetObject(inviterClient->GetActor(),true);
        inviterClient->SetTargetObject(inviteeClient->GetActor(),true);
    }
}

void PendingDuelInvite::HandleAnswer(const csString &answer)
{
    Client* client = psserver->GetConnections()->Find(clientnum);
    if(!client  ||  client->IsDuelClient(inviterClientNum))
        return;

    PendingInvite::HandleAnswer(answer);
    if(answer == "yes")
        psserver->usermanager->AcceptDuel(this);
}

void UserManager::StartTrading(psUserCmdMessage &msg,Client* client)
{
    client->GetCharacterData()->SetTradingStopped(false);
    psserver->SendSystemInfo(client->GetClientNum(),"You can trade now.");
}

void UserManager::StopTrading(psUserCmdMessage &msg,Client* client)
{
    client->GetCharacterData()->SetTradingStopped(true);
    psserver->SendSystemInfo(client->GetClientNum(),"You are busy and can't trade.");
}

void UserManager::HandleYield(psUserCmdMessage &msg,Client* client)
{
    YieldDuel(client);
}

void UserManager::YieldDuel(Client* client)
{
    if(client->GetActor()->GetMode() == PSCHARACTER_MODE_DEFEATED || !client->GetActor()->IsAlive())
        return;

    bool canYield = false;
    for(int i = 0; i < client->GetDuelClientCount(); i++)
    {
        Client* duelClient = psserver->GetConnections()->Find(client->GetDuelClient(i));
        if(duelClient && duelClient->GetActor() && duelClient->GetActor()->IsAlive() && duelClient->GetActor()->GetMode() != PSCHARACTER_MODE_DEFEATED)
        {
            canYield = true;
            if(duelClient->GetTargetObject() == client->GetActor())
                psserver->GetCombatManager()->StopAttack(duelClient->GetActor());
            psserver->SendSystemOK(client->GetClientNum(), "You've yielded to %s!", duelClient->GetName());
            psserver->SendSystemOK(duelClient->GetClientNum(), "%s has yielded!", client->GetName());
        }
    }

    if(!canYield)
    {
        psserver->SendSystemError(client->GetClientNum(), "You have no opponents to yield to!");
        return;
    }

    client->GetActor()->Defeat();
}

void UserManager::SwitchAttackTarget(Client* targeter, Client* targeted)
{
    // If we switch targets while in combat, start attacking the new
    // target, unless we no longer have a target.
    if(targeted)
        Attack(targeter->GetActor()->GetCombatStance(), targeter);
    else
        psserver->GetCombatManager()->StopAttack(targeter->GetActor());
}

/*---------------------------------------------------------------------*/

psUserStatRegeneration::psUserStatRegeneration(UserManager* mgr, csTicks ticks)
    : psGameEvent(ticks,0,"psUserStatRegeneration")
{
    usermanager = mgr;
}

void psUserStatRegeneration::Trigger()
{
    usermanager->UserStatRegeneration();
}
