/*
 * invitemanager.cpp
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

#include <psconfig.h>
#include <string.h>
#include <memory.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/serverconsole.h"
#include "util/pserror.h"
#include "util/log.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "invitemanager.h"
#include "client.h"
#include "clients.h"
#include "psserver.h"
#include "playergroup.h"
#include "gem.h"
#include "netmanager.h"
#include "globals.h"
#include "bulkobjects/pssectorinfo.h"


PendingInvite::PendingInvite(
    Client* inviter,
    Client* invitee,
    bool penalize,
    const char* question_str,
    const char* accept_button,
    const char* reject_button,
    const char* inviter_explanation,
    const char* invitee_explanation,
    const char* inviter_acceptance,
    const char* invitee_acceptance,
    const char* inviter_rejection,
    const char* invitee_rejection,
    psQuestionMessage::questionType_t invType)
    : PendingQuestion(invitee->GetClientNum(), question_str, invType)
{
    // Disable incoming invitations while dueling (invites could be used to distract and cheat)
    if(invitee->GetDuelClientCount() && invType != psQuestionMessage::duelConfirm)
    {
        psserver->SendSystemError(inviter->GetClientNum(), "%s is distracted with a duel", invitee->GetName());
        ok = false;
        return;
    }

    /* Invite spam is checked for when the penalize flag is on.
     * The penalty level increases with each consecutive decline. (set in invitemanager.h)
     * It can expire with time, or be lowered on acceptace by a "reputable player".
     * Penalties:
     * 1st) 1 min lockout
     * 2nd) 5 min lockout + warning
     * 3rd) 10 min lockout + 10 point fine(s) (advisor) + death
     * 4th) 30 point fine(s) + death + kick from server (level reset to 3rd after this)
     */
    if(penalize && CheckForSpam(inviter, invType))
    {
        ok = false;
        return;
    }

    inviterClientNum   =  inviter->GetClientNum();
    question           =  question_str;
    accept             =  accept_button;
    reject             =  reject_button;
    inviterAcceptance  =  inviter_acceptance;
    inviteeAcceptance  =  invitee_acceptance;
    inviterRejection   =  inviter_rejection;
    inviteeRejection   =  invitee_rejection;
    inviterName        =  inviter->GetName();
    inviteeName        =  invitee->GetName();
    cannotAccept       =  false;

    psserver->SendSystemInfo(invitee->GetClientNum(), invitee_explanation, inviter->GetName());
    psserver->SendSystemInfo(inviter->GetClientNum(), inviter_explanation, invitee->GetName());

    inviter->SetLastInviteTime(csGetTicks());
}

void PendingInvite::HandleAnswer(const csString &answer)
{
    if(answer == "yes")
    {
        psserver->SendSystemOK(clientnum, inviteeAcceptance, inviterName.GetData());
        psserver->SendSystemResult(inviterClientNum, inviterAcceptance, inviteeName.GetData());
    }
    else
    {
        psserver->SendSystemInfo(clientnum, inviteeRejection, inviterName.GetData());
        psserver->SendSystemInfo(inviterClientNum, inviterRejection, inviteeName.GetData());
    }

    HandleSpamPoints(answer == "yes");
}

void PendingInvite::HandleTimeout()
{
    psserver->SendSystemInfo(inviterClientNum, "%s did not respond.  Please try again later.", inviteeName.GetData());
}

bool PendingInvite::CheckForSpam(Client* inviter, psQuestionMessage::questionType_t type)
{
    csTicks now = csGetTicks();
    csTicks then = inviter->GetLastInviteTime();

    // If the inviter has a security level then don't count it as spam
    if(inviter->GetSecurityLevel() > 0)
        return false;

    if(then && now > then + INVITESPAMBANTIME[inviter->GetSpamPoints()]*60*1000)
    {
        // If the player hasn't invited anyone in a while, expire a spamPoint
        // (LastInviteTime is set at the end of the invite, restarting this timer)
        inviter->DecrementSpamPoints();
    }

    csTicks ban = INVITESPAMBANTIME[inviter->GetSpamPoints()-1]*60*1000;

    if(inviter->GetSpamPoints() && !inviter->GetLastInviteResult() && now < then + ban)
    {
        // Warn the player, if not done already this session
        if(inviter->GetSpamPoints() >= 2 && !inviter->HasBeenWarned())
        {
            psserver->SendSystemError(inviter->GetClientNum(), "Invite spamming will be penalized.");
            inviter->SetWarned();
        }

        // Award penalties if due (one penalty per decline)
        else if(inviter->GetSpamPoints() >= 3 && !inviter->HasBeenPenalized())
        {
            inviter->SetPenalized(true);

            bool three = (inviter->GetSpamPoints() == 3);

            int fine = (three)?FIRST_SPAM_FINE:SECOND_SPAM_FINE;

            inviter->IncrementAdvisorPoints(-fine);

            psSystemMessage newmsg(inviter->GetClientNum(), MSG_INFO,
                                   "The nuisance known as %s was %s by %s!\n"
                                   "Let this be a lesson to all...",
                                   inviter->GetName(),
                                   (three)?"struck down":"banished to another realm",
                                   inviter->GetCharacterData()->GetLocation().loc_sector->god_name.GetData());
            newmsg.Multicast(inviter->GetActor()->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);

            inviter->GetActor()->Kill(NULL);

            // A fourth offense gets a kick
            if(!three)
            {
                psserver->RemovePlayer(inviter->GetClientNum(),
                                       "You were automatically kicked from the server for invite spamming.");
                return true;
            }
        }

        // Display time remaining in the ban
        int minutes = (then-now+ban)/(60*1000);
        int seconds = (then-now+ban)/1000 - minutes*60;
        csString message;
        if(minutes)
            message.Format("You need to wait %d minutes and %d seconds before inviting again.", minutes, seconds);
        else
            message.Format("You need to wait %d seconds before inviting again.", seconds);
        psserver->SendSystemError(inviter->GetClientNum(), message.GetData());

        return true; // This invite is spam: cancel message
    }
    else return false;
}

void PendingInvite::HandleSpamPoints(bool accepted)
{
    Client* invitee = psserver->GetConnections()->Find(clientnum);
    Client* inviter = psserver->GetConnections()->Find(inviterClientNum);

    if(!invitee || !inviter)
        return;

    inviter->SetLastInviteResult(accepted);

    if(accepted)
    {
        if(inviter->GetSpamPoints() && invitee->GetSpamPoints() <= 1
                && invitee->GetAdvisorPoints() >=
                3*INVITESPAMBANTIME[inviter->GetSpamPoints()]*INVITESPAMBANTIME[invitee->GetSpamPoints()])
        {
            // Lower spam points on accept from a sufficiently "reputable" player
            inviter->DecrementSpamPoints();
        }
    }
    else
    {
        inviter->IncrementSpamPoints();
    }

    // Reset spam penalty
    inviter->SetPenalized(false);
}

