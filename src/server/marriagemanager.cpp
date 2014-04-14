/* marriagemanager.cpp - Author: DivineLight
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 */
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psraceinfo.h"  // for gender 
#include "bulkobjects/pscharacterloader.h"

#include "util/psstring.h"
#include "util/psdatabase.h"
#include "util/eventmanager.h"

#include "net/msghandler.h"
#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "adminmanager.h"
#include "marriagemanager.h"
#include "chatmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "events.h"
#include "gem.h"
#include "netmanager.h"
#include "invitemanager.h"
#include "usermanager.h"
#include "globals.h"

/** Questions client and Handles YesNo response of client for proposals. */
class PendingMarriageProposal : public PendingInvite
{
public:
    PendingMarriageProposal(Client* inviter, Client* invitee,
                            const char* question) : PendingInvite(inviter, invitee, false,
                                        question, "Accept", "Reject",
                                        "You have proposed %s to marry you.",
                                        "%s has made a marriage proposal to you.",
                                        "%s has accepted your proposal.",
                                        "You have accepted the marriage proposal.",
                                        "%s has rejected your marriage proposal.",
                                        "You have rejected %s's marriage proposal.",
                                        psQuestionMessage::marriageConfirm)
    {
    }

    void HandleAnswer(const csString &answer)
    {
        Client* proposedClient = psserver->GetConnections()->Find(clientnum);
        if(!proposedClient)
            return;

        PendingInvite::HandleAnswer(answer);

        Client* proposerClient = psserver->GetConnections()->Find(inviterClientNum);
        if(!proposerClient)
            return;

        psMarriageManager* marriageMgr = psserver->GetMarriageManager();
        if(!marriageMgr)
            return;

        csString tempStr;
        psCharacter* proposedCharData = proposedClient->GetCharacterData();
        psCharacter* proposerCharData = proposerClient->GetCharacterData();
        const char* proposedCharFirstName = proposedCharData->GetCharName();
        const char* proposerFirstName = proposerCharData->GetCharName();

        if(answer == "no")
        {
            // Inform proposer that marriage has been rejected
            tempStr.Format("%s has rejected your Marriage Proposal.", proposedCharFirstName);
            psserver->SendSystemError(proposerClient->GetClientNum(),
                                      tempStr.GetData());

            Debug3(LOG_RELATIONSHIPS, proposerClient->GetClientNum(),"%s rejected the marriage proposal from %s.", proposedCharFirstName, proposerFirstName);
            return;
        }

        //Now it is time to make the act "legal", changing surname if it is the case and such.
        if(!marriageMgr->PerformMarriage(proposedCharData, proposerCharData))
        {
            // Error handling: Should rarely happen.
            psserver->SendSystemError(proposerClient->GetClientNum(), "Unknown occured while completing marriage request.");
            psserver->SendSystemError(proposedClient->GetClientNum(), "Unknown occured while completing marriage request.");
            Error1("\nMSG_MARRIAGE_ACCEPT: Some error while setting spouse names.\n");
            return;
        }

        // Inform proposer & proposed char of successfull marriage
        tempStr.Format("Congratulations! You are now married to %s", proposedCharFirstName);
        psserver->SendSystemResult(proposerClient->GetClientNum(), tempStr.GetData());

        tempStr.Format("Congratulations! You are now married to %s", proposerFirstName);
        psserver->SendSystemResult(proposedClient->GetClientNum(), tempStr.GetData());

        Debug3(LOG_RELATIONSHIPS, proposerClient->GetClientNum(),"\n%s is now married to %s.\n", proposedCharFirstName, proposerFirstName);
    }
};

class PendingMarriageDivorce : public PendingInvite
{
public:
    PendingMarriageDivorce(Client* inviter, Client* invitee,
                           const char* question) : PendingInvite(inviter, invitee, false,
                                       question, "Accept", "Reject",
                                       "You have asked %s to divorce.",
                                       "%s wants to obtain a divorce from you.",
                                       "%s agrees on the divorce.",
                                       "You agreed to divorce.",
                                       "%s doesn't want to divorce.",
                                       "You have declined %s's divorce request.",
                                       psQuestionMessage::generalConfirm)
    {
    }

    void HandleAnswer(const csString &answer)
    {
        Client* divorcedClient = psserver->GetConnections()->Find(clientnum);
        if(!divorcedClient)
        {
            return;
        }

        PendingInvite::HandleAnswer(answer);

        Client* divorcerClient = psserver->GetConnections()->Find(inviterClientNum);
        if(!divorcerClient)
        {
            return;
        }

        psMarriageManager* marriageMgr = psserver->GetMarriageManager();
        if(!marriageMgr)
        {
            return;
        }

        csString tempStr;
        psCharacter* divorcedCharData = divorcedClient->GetCharacterData();
        psCharacter* divorcerCharData = divorcerClient->GetCharacterData();
        csString divorcedFirstName = divorcedCharData->GetCharName();
        csString divorcerFirstName = divorcerCharData->GetCharName();

        HandleSpamPoints(answer == "yes");

        if(answer == "no")
        {
            // Inform proposer that divorce has been rejected
            tempStr.Format("%s has rejected your divorce request.",
                           divorcedFirstName.GetDataSafe());
            psserver->SendSystemError(divorcerClient->GetClientNum(),
                                      tempStr.GetData());

            Debug3(LOG_RELATIONSHIPS, clientnum, "%s rejected the divorce request from %s.",
                   divorcedFirstName.GetDataSafe(), divorcerFirstName.GetDataSafe());
            return;
        }

        // Delete entries of both character's from DB
        marriageMgr->DeleteMarriageInfo(divorcerCharData);

        // Inform divorcer and divorced of successfull divorce
        psserver->SendSystemResult(divorcerClient->GetClientNum(), "You are now divorced.");
        psserver->SendSystemResult(divorcedClient->GetClientNum(), "You are now divorced.");

        Debug3(LOG_RELATIONSHIPS, divorcedClient->GetClientNum(),"%s divorced from %s.", divorcerFirstName.GetData(), divorcedFirstName.GetData());

    }
};

class PendingMarriageDivorceConfirm : public PendingQuestion
{
protected:
    csString divorceMsg;

public:
    PendingMarriageDivorceConfirm(int cnum, const char* question, const char* msg)
        : PendingQuestion(cnum, question, psQuestionMessage::generalConfirm)
    {
        divorceMsg = msg;
    }

    void HandleAnswer(const csString &answer)
    {
        if(answer == "yes")
        {
            Client* client = psserver->GetConnections()->FindAny(clientnum);
            psserver->GetMarriageManager()->Divorce(client,divorceMsg);
        }
    }

    void HandleTimeout()
    {
        HandleAnswer("no");
    }
};

void psMarriageManager::Propose(Client* client, csString proposedCharName, csString proposeMsg)
{
    if(!client)
        return;

    csString proposerName = client->GetCharacterData()->GetCharName();

    // Character cannot propose itself
    if(proposedCharName.CompareNoCase(proposerName))
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot marry yourself.");
        return;
    }

    // Make sure the proposer is not already married
    if(client->GetCharacterData()->GetIsMarried())
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You are already married to %s",
                                  client->GetCharacterData()->GetSpouseName());
        return;
    }

    // Make sure the character is old enough (24 hours = 86400 seconds), but not apply if tester, or more
    if(client->GetAccountTotalOnlineTime() < 86400 && client->GetSecurityLevel() < GM_TESTER)
    {
        psserver->SendSystemError(client->GetClientNum(), "You have not played long enough (24 hours) to marry someone.");
        return;
    }

    // Find the client of proposed char so we can send proposal
    Client* proposedClient = psserver->GetCharManager()->FindPlayerClient(proposedCharName);
    if(proposedClient && !proposedClient->IsSuperClient())
    {
        // Make sure both parties know each other
        if(!client->GetCharacterData()->Knows(proposedClient->GetCharacterData()))
        {
            psserver->SendSystemResult(client->GetClientNum(), "You haven't asked %s name yet!", proposedClient->GetCharacterData()->GetRaceInfo()->GetPossessive());
            return;
        }
        if(!proposedClient->GetCharacterData()->Knows(client->GetCharacterData()))
        {
            psserver->SendSystemResult(client->GetClientNum(), "You haven't told %s your name yet!", proposedClient->GetCharacterData()->GetRaceInfo()->GetObjectPronoun());
            return;
        }

        // Make sure that the proposed char is also single
        if(proposedClient->GetCharacterData()->GetIsMarried())
        {
            psserver->SendSystemResult(client->GetClientNum(), "%s is already married to %s",
                                       proposedCharName.GetData(), proposedClient->GetCharacterData()->GetSpouseName());
            return;
        }

        // Don't let same gender marriages - genderless is an exception)
        if((client->GetCharacterData()->GetRaceInfo()->gender != PSCHARACTER_GENDER_NONE)
                && (client->GetCharacterData()->GetRaceInfo()->gender ==
                    proposedClient->GetCharacterData()->GetRaceInfo()->gender))
        {
            psserver->SendSystemResult(client->GetClientNum(),
                                       "You cannot marry a person of your same gender.");
            return;
        }

        // If proposer is genderless - He can only marry other genderless
        if((client->GetCharacterData()->GetRaceInfo()->gender == PSCHARACTER_GENDER_NONE))
        {
            // Don't progress if proposed char is not Kran also
            if(proposedClient->GetCharacterData()->GetRaceInfo()->gender != PSCHARACTER_GENDER_NONE)
            {
                psserver->SendSystemError(client->GetClientNum(), "You can only marry other genderless.");
                return;
            }
        }
        // Proposer is not a Kran, then he cannot marry a Kran
        else
        {
            if(proposedClient->GetCharacterData()->GetRaceInfo()->gender == PSCHARACTER_GENDER_NONE)
            {
                psserver->SendSystemError(client->GetClientNum(), "Only genderless can marry other genderless.");
                return;
            }
        }

        // Make sure that Proposer & ProposedChar are near enough
        if(proposedClient->GetActor()->RangeTo(client->GetActor()) > RANGE_TO_SELECT)
        {
            psserver->SendSystemError(client->GetClientNum(), "You are not near enough to %s to propose.",
                                      proposedCharName.GetData());
            return;
        }

        csString tempStr;
        tempStr.Format("%s has made a marriage proposal telling you: '%s' Do you agree?",
                       proposerName.GetData(), proposeMsg.GetData());

        PendingQuestion* invite = new PendingMarriageProposal(client, proposedClient, tempStr.GetData());
        psserver->questionmanager->SendQuestion(invite);

        Debug3(LOG_RELATIONSHIPS, client->GetClientNum(),"%s has proposed %s for Marriage", proposerName.GetData(), proposedCharName.GetData());
    }
    else
    {
        psserver->SendSystemResult(client->GetClientNum(), "%s is not in Yliakum right now.", proposedCharName.GetData());
    }
}

void psMarriageManager::ContemplateDivorce(Client* client, csString divorceMsg)
{
    if(!client)
        return;

    if(!client->GetCharacterData()->GetIsMarried())
    {
        psserver->SendSystemError(client->GetClientNum(), "Before divorcing, you should get married.");
        return;
    }

    csString promptMsg;
    promptMsg.Format("Are you sure you want to divorce your dear spouse %s?",
                     client->GetCharacterData()->GetSpouseName());

    PendingQuestion* prompt = new PendingMarriageDivorceConfirm(client->GetClientNum(), promptMsg, divorceMsg);
    psserver->questionmanager->SendQuestion(prompt);
}

void psMarriageManager::Divorce(Client* client, csString divorceMsg)
{
    if(!client)
        return;

    // We are sure 200% that we are gonna divorce.
    csString divorcerName = client->GetCharacterData()->GetCharName();

    psString divorcedCharFirstName;
    psString divorcedChar = client->GetCharacterData()->GetSpouseName();
    divorcedChar.GetWord(0, divorcedCharFirstName);

    Client* divorcedClient = psserver->GetConnections()->Find(divorcedCharFirstName.GetData());
    if(divorcedClient)
    {
        // Make sure that the divorcer & divorced are near enough
        if(divorcedClient->GetActor()->RangeTo(client->GetActor()) > RANGE_TO_SELECT)
        {
            psserver->SendSystemError(client->GetClientNum(),
                                      "You are not near enough to %s to discuss a divorce.",
                                      divorcedChar.GetData());
            return;
        }

        csString tempStr;
        tempStr.Format("%s wants to divorce for this reason '%s.' Do you agree?", divorcerName.GetData(),
                       divorceMsg.GetData());
        PendingQuestion* invite = new PendingMarriageDivorce(client, divorcedClient, tempStr.GetData());
        psserver->questionmanager->SendQuestion(invite);

        Debug3(LOG_RELATIONSHIPS, client->GetClientNum(),"%s wants to divorce from %s", divorcerName.GetData(), divorcedChar.GetData());

    }
    else
    {
        if(psserver->GetCharManager()->HasConnected(divorcedCharFirstName))
        {
            //If we are here, the partner char is still existing and it has been connecting recently. Wait to be online.
            psserver->SendSystemInfo(client->GetClientNum(), "%s is not in Yliakum right now.", divorcedChar.GetData());
        }
        else
        {
            //The partner hasn't logged on in the latest two months
            psserver->SendSystemError(client->GetClientNum(), "%s hasn't walked in Yliakum in a long time!", divorcedChar.GetData());
            psserver->SendSystemInfo(client->GetClientNum(), "Contact a Game Master for obtaining a divorce.");
        }
    }
}

bool psMarriageManager::PerformMarriage(psCharacter* charData,  psCharacter* spouseData)
{
    // Must not be NULL
    CS_ASSERT(charData);
    CS_ASSERT(spouseData);

    const char* charFullName = charData->GetCharFullName();
    const char* spouseFullName = spouseData->GetCharFullName();

    csString query;

    // Create this char's entry in character_marriage_details if not present for both.
    this->CreateMarriageEntry(charData, spouseData);
    this->CreateMarriageEntry(spouseData, charData);

    // Now if character was female then change her last name to her
    // husband's last name
    if(charData->GetRaceInfo()->gender == PSCHARACTER_GENDER_FEMALE)
    {
        // Save old lastname
        charData->SetOldLastName(charData->GetCharLastName());

        charData->SetLastName(spouseData->GetCharLastName());

        UpdateName(charData);

    }
    else if(spouseData->GetRaceInfo()->gender == PSCHARACTER_GENDER_FEMALE)
    {
        // Save old lastname
        spouseData->SetOldLastName(spouseData->GetCharLastName());
        spouseData->SetLastName(charData->GetCharLastName());

        UpdateName(spouseData) ;
    }
    //If not then we have two Krans.

    charData->SetSpouseName(spouseFullName);
    spouseData->SetSpouseName(charFullName);

    Notify3(LOG_RELATIONSHIPS, "SetSpouseName(): %s now has %s as spouse.", charFullName, spouseFullName);
    return true;
}

bool psMarriageManager::CreateMarriageEntry(psCharacter* charData,  psCharacter* spouseData)
{
    csString query;
    Result rs;

    // create entry of this character in 'character_relationships'
    csString spouse_name;
    db->Escape(spouse_name, spouseData->GetCharFullName());

    query.Format("INSERT INTO character_relationships VALUES(%d,%d,'spouse','%s')", charData->GetPID().Unbox(), spouseData->GetPID().Unbox(), spouse_name.GetData());
    if(!db->Command(query.GetData()))
    {
        Error2("CreateMarriageEntry(): DB Error: %s", db->GetLastError());
        return false;
    }

    return true;
}

void psMarriageManager::DeleteMarriageInfo(psCharacter* charData)
{
    csString spouseFullName = charData->GetSpouseName();
    csString spouseName = spouseFullName.Slice(0, spouseFullName.FindFirst(' '));
    Client* otherClient = psserver->GetCharManager()->FindPlayerClient(spouseName.GetData());

    // Step 1: Revert the changed last name of the female (if necessary)
    if(charData->GetRaceInfo()->gender == PSCHARACTER_GENDER_MALE)
    {
        // spouse has to be female
        if(otherClient && otherClient->GetCharacterData())
        {
            // spouse is online (or at least still cached)
            otherClient->GetCharacterData()->SetLastName(otherClient->GetCharacterData()->GetOldLastName());
            otherClient->GetCharacterData()->SetOldLastName("");
            UpdateName(otherClient->GetCharacterData());
        }
        else
        {
            // spouse is offline - hit the database
            csString query;
            query.Format("SELECT id FROM characters WHERE name='%s'", spouseName.GetData());
            Result result(db->Select(query.GetData()));

            if(!result.IsValid() || result.Count() != 1)
            {
                Error2("DeleteMarriageInfo(): DB Error: %s", db->GetLastError());
                return;
            }

            iResultRow &row = result[0];
            unsigned int id = row.GetUInt32("id");

            query.Format("UPDATE characters SET lastname=old_lastname, old_lastname='' WHERE id=%d", id);

            if(!db->Command(query.GetData()))
            {
                Error2("DeleteMarriageInfo(): DB Error: %s", db->GetLastError());
                return;
            }
        }
    }
    else if(charData->GetRaceInfo()->gender == PSCHARACTER_GENDER_FEMALE)
    {
        // divorcer is female
        charData->SetLastName(charData->GetOldLastName());
        charData->SetOldLastName("");
        UpdateName(charData);
    }

    // Step 2: We remove the data about the marriage now.
    csString query;
    query.Format("DELETE FROM character_relationships WHERE (character_id='%d' OR related_id='%d') and relationship_type='spouse'", charData->GetPID().Unbox(), charData->GetPID().Unbox());
    if(!db->Command(query.GetData()))
    {
        Error2("DeleteMarriageInfo(): DB Error: %s", db->GetLastError());
        return;
    }

    // Step 3: Update cached spouse name
    if(otherClient && otherClient->GetCharacterData())
    {
        otherClient->GetCharacterData()->SetSpouseName("");
    }
    charData->SetSpouseName("");
}

void psMarriageManager::UpdateName(psCharacter* charData)
{
    gemActor* actor;

    actor = charData->GetActor();
    if(!actor)
    {
        Error1("UpdateName(): Couldn't find an actor!");
        return;
    }

    psserver->CharacterLoader.SaveCharacterData(charData, actor, true);

    //charData->SetFullName( charData->GetCharName(), lastName );
    actor->SetName(charData->GetCharFullName());

    //Update label
    psUpdateObjectNameMessage newNameMsg(0, actor->GetEID(), charData->GetCharFullName());
    psserver->GetEventManager()->Broadcast(newNameMsg.msg, NetBase::BC_EVERYONE);

}
