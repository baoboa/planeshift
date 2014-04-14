/*
 * questionmanager.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef QUESTION_MANAGER_HEADER
#define QUESTION_MANAGER_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"


class psQuestionCancelEvent;

/**
 * "Questions" are general requests of information from user.
 *
 * The lifetime of a question:
 * 1) It is sent to some client
 * 2) Client shows appropriate GUI to present the question to user
 * 3) User chooses answer from the GUI
 * 4) Answer is sent to server which somehow reacts to user's decision
 *
 * Unanswered questions automatically time out after some time
 * - the are removed from questionmanager and from user's GUI.
 *
 * The questioning framework can be extended to support any forms of questions:
 *    e.g. plain text, XML, images
 * If you want to create a new kind of questioning GUI on the client, you will need
 * to add a new question type and write its handler on client side.
 *
 * One common use for questions are invites - see invitemanager.h
 */


/**
  * This class is the superclass for all questions,
  * If you need a new kind of question (for example new handler of answers),
  * inherit from this class.
  */
class PendingQuestion
{
public:
    unsigned id;           // unique id
    static unsigned nextID;
    bool ok; // Set to false to stop sending

    /** type of question - clients handles each question type differently */
    psQuestionMessage::questionType_t type;

    /** Holds data that will be somehow displayed to user
            - can be anything, plain text, XML, encoded bitmap.
        Client will interpret it according to 'type' */
    csString question;

    int clientnum;        // the questioned client
    psQuestionCancelEvent* event;

    PendingQuestion()
    {
        event = NULL;
    }
    PendingQuestion(int clientnum, const csString &question, psQuestionMessage::questionType_t type);
    virtual ~PendingQuestion() {};


    /** This is called when user sends answer to this question, or when
        the question times out (because the user didn't respond in time)

        Override this method to react to the answers sent by questioned users */

    virtual void HandleAnswer(const csString &answer) {};
    virtual void HandleTimeout() {};
};

/**
 * QuestionManager keeps track of all the questions of any kind
 * that are pending
 */
class QuestionManager : public MessageManager<QuestionManager>
{
public:

    QuestionManager();
    virtual ~QuestionManager();

    /** Sends a question to a client */
    void SendQuestion(PendingQuestion* question);

    /** Removes specified question from list and from GUI on questioned client */
    void CancelQuestion(PendingQuestion* question);

protected:

    void HandleQuestionResponse(MsgEntry* pMsg,Client* client);

    csHash<PendingQuestion*> questions; /** questions indexed by IDs */
};


#endif
