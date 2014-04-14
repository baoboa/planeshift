/*
 * invitemanager.h
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


#ifndef __INVITEMANAGER_H__
#define __INVITEMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "questionmanager.h"


#define FIRST_SPAM_FINE 10
#define SECOND_SPAM_FINE 30
static const int INVITESPAMBANTIME[5] = {1, 5, 10, // Ban times in minutes
                                        30, 10
                                        };  // spamPoints expire based on SP+1's ban time


class psInviteGameEvent;
class Client;

/**
 * This class is the superclass for all player-to-player invitations,
 * such as inviting into a group, a guild, a duel, an alliance, a
 * guild war, a tourney, etc.
 *
 * Subclass this and then hand over to QuestionManager
 */
class PendingInvite : public PendingQuestion
{
public:

    int inviterClientNum;

    /** the text on buttons used to either accept or reject invitation */
    csString accept, reject;

    /** text sent via chat when invitation has been accepted */
    csString inviterAcceptance, inviteeAcceptance;

    /** text sent via chat when invitation has been rejected */
    csString inviterRejection, inviteeRejection;

    /** names of the two players */
    csString inviterName, inviteeName;

    bool cannotAccept;  /// if the user cannot accept this invitation (used when inviting member of secret guild)

    PendingInvite(Client* inviter,
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
                  psQuestionMessage::questionType_t invType);

    virtual void HandleAnswer(const csString &answer);
    virtual void HandleTimeout();


protected:

    bool CheckForSpam(Client* inviter, psQuestionMessage::questionType_t type);
    void HandleSpamPoints(bool accepted);
};


#endif

