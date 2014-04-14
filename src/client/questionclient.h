/*
 * questionclient.h - Author: Ondrej Hurt
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

#ifndef __QUESTION_CLIENT_H__
#define __QUESTION_CLIENT_H__

#include <csutil/hash.h>
#include "net/cmdbase.h"

class MsgHandler;
struct iObjectRegistry;
class pawsYesNoBox;

/** psQuestion is superclass of all question types.*/
class psQuestion
{
public:
    psQuestion(uint32_t questionID) { this->questionID = questionID; }
    virtual ~psQuestion() {};
    
    /** Cancels answering of this question, for example closes some PAWS window 
      * Question deletes itself. */
    virtual void Cancel() = 0;
protected:
    uint32_t questionID;
};


/**
  * The psQuestionClient class manages answering to various questions sent from server
  * to user. It keeps list of the questions that have not been answered yet.
 */
class psQuestionClient : public psClientNetSubscriber
{
public:
    psQuestionClient(MsgHandler* mh, iObjectRegistry* obj);
    virtual ~psQuestionClient();

    // iNetSubscriber interface
    virtual void HandleMessage(MsgEntry *msg);
    
    /** Delete question from list of pending questions */
    void DeleteQuestion(uint32_t questionID);

    /** Sends response to given question to server */
    void SendResponseToQuestion(uint32_t questionID, const csString & answer);
    
protected:
    
    /**
     * Default handler for a confirm message. Creates a new \ref psQuestion
     * which displays a yes/no dialog and adds it to \ref questions so that 
     * the user may reply to the question as they wish.
     * @param questionID ID of the question
     * @param question Text to be shown in the yes/no dialog
     */
    void HandleConfirm(uint32_t questionID, const csString &question);
    /**
     * Handles duel questions questions according to user settings.
     * If the user has chosen to ignore duels the question will silently
     * be replied "no" to. If the user has chosen to accept all duels the
     * question will silently be replied "yes" to. Otherwise the dialog
     * will appear normally.
     * @param questionID ID of the question
     * @param question Text to be shown in the dialog if it is used.
     */
    void HandleDuel(uint32_t questionID, const csString &question);
    /**
     * Silently handles secret guild notifications. This message is always replied
     * no to but the user sees it anyway because of emulation requirements.
     * @param questionID ID of the question
     * @param question Text that would be shown if this dialog was shown.
     */
    void HandleSecretGuildNotify(uint32_t questionID, const csString &question);
    /**
     * Handles marriage questions according to user settings.
     * If the user has chosen to ignore marriage proposals the question will
     * silently be replied "no" to. Otherwise the dialog will appear normally.
     * @param questionID ID of the question
     * @param question Text to be shown in the dialog if it is used.
     */
    void HandleMarriage(uint32_t questionID, const csString &question);
    
    MsgHandler* messageHandler;
    
    csHash<psQuestion*> questions;
};

#endif

