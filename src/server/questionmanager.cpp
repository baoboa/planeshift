/*
 * questionmanager.cpp
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


#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/log.h"
#include "util/serverconsole.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "questionmanager.h"
#include "client.h"
#include "clients.h"
#include "globals.h"


unsigned PendingQuestion::nextID = 0;

/**************************************************************
*                   class PendingQuestion
***************************************************************/

PendingQuestion::PendingQuestion(int clientnum, const csString &question, psQuestionMessage::questionType_t type)
{
    id = nextID++;
    event = NULL;
    this->clientnum = clientnum;
    this->question  = question;
    this->type      = type;
    ok = true;
}

/**************************************************************
*              class psQuestionCancelEvent
***************************************************************/

/** This class is used to cancel questions that were left unanswered for too long */
class psQuestionCancelEvent : public psGameEvent
{
protected:
    QuestionManager* manager;

public:
    PendingQuestion* question;
    bool valid;

    psQuestionCancelEvent(QuestionManager* mgr,
                          int delayticks,
                          PendingQuestion* question);
    virtual void Trigger();  // Abstract event processing function
};

psQuestionCancelEvent::psQuestionCancelEvent
(QuestionManager* mgr,int delayticks, PendingQuestion* q)
    : psGameEvent(0,delayticks,"psQuestionCancelEvent")
{
    manager = mgr;
    question        = q;
    valid         = true;
}

void psQuestionCancelEvent::Trigger()
{
    // This is set to false by the Acceptance handler, so this won't cancel later
    if(valid)
        manager->CancelQuestion(question);
}

/**************************************************************
*                   class QuestionManager
***************************************************************/

QuestionManager::QuestionManager()
{
    Subscribe(&QuestionManager::HandleQuestionResponse, MSGTYPE_QUESTIONRESPONSE, REQUIRE_READY_CLIENT);
}

QuestionManager::~QuestionManager()
{
    csHash<PendingQuestion*>::GlobalIterator iter(questions.GetIterator());
    while(iter.HasNext())
        delete iter.Next();
}

void QuestionManager::HandleQuestionResponse(MsgEntry* me,Client* client)
{
    psQuestionResponseMsg msg(me);
    if(!msg.valid)
    {
        psserver->SendSystemError(me->clientnum, "Invalid QuestionResponse Message.");
        Error2("Failed to parse psQuestionResponseMsg from client %u.",me->clientnum);
        return;
    }

    // Find the question that we got response to
    PendingQuestion* question = questions.Get(msg.questionID, NULL);
    if(!question)
    {
        Error2("Received psQuestionResponseMsg from client %u that was not questioned.",me->clientnum);
        return;
    }

    if(question->event)
    {
        question->event->valid = false;  // This keeps the cancellation timeout from firing.
    }

    question->HandleAnswer(msg.answer);

    questions.DeleteAll(msg.questionID);  // Question is no longer pending.
    delete question;
}

void QuestionManager::SendQuestion(PendingQuestion* question)
{
    if(!question->ok)
        return;

    psQuestionMessage msg(question->clientnum,
                          question->id,
                          question->question,
                          question->type);
    msg.SendMessage();

    questions.Put(question->id, question);

    Client* questionedClient = psserver->GetConnections()->Find(question->clientnum);
    if(questionedClient != NULL)
    {
        psQuestionCancelEvent* ev = new psQuestionCancelEvent(this,60000,question);
        question->event = ev;
        psserver->GetEventManager()->Push(ev);
    }
    else
        Error2("Could not find client ! %i", question->clientnum);
}

void QuestionManager::CancelQuestion(PendingQuestion* question)
{
    question->HandleTimeout();

    psQuestionCancelMessage cancel(question->clientnum, question->id);
    cancel.SendMessage();

    questions.DeleteAll(question->id);
    delete question;
}

