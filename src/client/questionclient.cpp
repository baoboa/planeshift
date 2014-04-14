/*
 * questionclient.cpp - Author: Ondrej Hurt
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

#include "globals.h"
#include "questionclient.h"
#include "net/clientmsghandler.h"
#include "paws/pawsmanager.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawsmainwidget.h"


/*******************************************************************
*                   psConfirm
*******************************************************************/

void yesNoCallback(bool dec, void * conf);

/** psConfirm represents questions that want to confirm something 
  * (response is "yes" or "no"). It opens a pawsYesNoWindow
  * and waits for user to take a choice.
  */
class psConfirm : public psQuestion
{
public:
    pawsYesNoBox * wnd;
    psQuestionClient * qc;
    
    psConfirm(uint32_t questionID, const csString & question, 
              psQuestionClient * qc) 
                    : psQuestion(questionID)
    {
        wnd = (pawsYesNoBox*) PawsManager::GetSingleton().FindWidget("YesNoWindow");

        if ( !wnd )
        {
            Error1("Cannot Find YesNo Window");
           qc->DeleteQuestion(questionID);
           return;
        }

        wnd->MoveTo( (PawsManager::GetSingleton().GetGraphics2D()->GetWidth() - wnd->GetActualWidth(512) ) / 2,
                       (PawsManager::GetSingleton().GetGraphics2D()->GetHeight() - wnd->GetActualHeight(256))/2 );
        wnd->SetID();
        wnd->SetCallBack(yesNoCallback, this, question);
        wnd->Show();
        
        //Players should be able to re-equip, close other windows and target before having to accept a pvp duel
        //paws->SetModalWidget(wnd);
        
        this->qc = qc;
    }
    virtual ~psConfirm() {};
    
    /** Destructs window and itself */
    void CleanUp()
    {
        wnd->Hide(); //keep window for reuse.
        qc->DeleteQuestion(questionID);
        //our object has been destructed at this point
    }
    
    virtual void Cancel()
    {
        CleanUp();
        //our object has been destructed at this point
    }
    
    /** This is called to pass in decision made by user 
      * It sends the decision to server and destructs itself */
    virtual void SetDecision(bool dec)
    {
        if (dec)
            qc->SendResponseToQuestion(questionID, "yes");
        else
            qc->SendResponseToQuestion(questionID, "no");
        CleanUp();
        //our object has been destructed at this point
    }
};

/** Called by yesNoWindow */
void yesNoCallback(bool dec, void * conf)
{
    ((psConfirm*)conf) -> SetDecision(dec);
}

/*******************************************************************
*                   psSecretGuildNotify
*******************************************************************/

class psSecretGuildNotify : public psConfirm
{
public:
    psSecretGuildNotify(uint32_t questionID, const csString & question, 
              psQuestionClient * qc)
        : psConfirm(questionID, question, qc) 
    {
    }
    virtual ~psSecretGuildNotify() {};
    virtual void SetDecision(bool /*dec*/)
    {
        // always decline no matter what
        qc->SendResponseToQuestion(questionID, "no");
        CleanUp();
        //our object has been destructed at this point
    }
};

/*******************************************************************
*                   psQuestionClient
*******************************************************************/

psQuestionClient::psQuestionClient(MsgHandler* mh, iObjectRegistry* /*obj*/)
{
    messageHandler = mh;
    
    messageHandler->Subscribe(this,MSGTYPE_QUESTIONCANCEL);
    messageHandler->Subscribe(this,MSGTYPE_QUESTION);
}

psQuestionClient::~psQuestionClient()
{
    messageHandler->Unsubscribe(this,MSGTYPE_QUESTION);
    messageHandler->Unsubscribe(this,MSGTYPE_QUESTIONCANCEL);
}

void psQuestionClient::HandleMessage(MsgEntry *msg)
{
    switch (msg->GetType() )
    {
        case MSGTYPE_QUESTION:
        {
            psQuestionMessage question(msg);
            
            /** Calls correct handler for our question type */
            switch (question.type)
            {
                case psQuestionMessage::generalConfirm: 
                    HandleConfirm(question.questionID, question.question); break;
                case psQuestionMessage::duelConfirm: 
                    HandleDuel(question.questionID, question.question); break;
                case psQuestionMessage::secretGuildNotify:
                    HandleSecretGuildNotify(question.questionID, question.question); break;
                case psQuestionMessage::marriageConfirm:
                    HandleMarriage(question.questionID, question.question); break;
                default: Error2("Received Question of unknown type: %i",question.type);
            }
            break;            
        }
        case MSGTYPE_QUESTIONCANCEL:
        {
            psQuestionCancelMessage cancel(msg);
            psQuestion* q = questions.Get(cancel.questionID, NULL);
            if (q != NULL)
            {
                q->Cancel();
                DeleteQuestion(cancel.questionID);
            }
            break;
        }
    }
}

void psQuestionClient::HandleConfirm(uint32_t questionID, const csString & question)
{
    psConfirm * conf = new psConfirm(questionID, question, this);
    questions.Put(questionID, conf);
}

void psQuestionClient::HandleSecretGuildNotify(uint32_t questionID, const csString & question)
{
    psSecretGuildNotify * conf = new psSecretGuildNotify(questionID, question, this);
    questions.Put(questionID, conf);
}

void psQuestionClient::HandleDuel(uint32_t questionID, const csString & question)
{
    switch (psengine->GetDuelConfirm())
    {
        case 0:  // always deny duels
            SendResponseToQuestion(questionID, "no");
            break;
        case 2: // accept all duels
            SendResponseToQuestion(questionID, "yes");
            break;
        default: // or open the YesNo window
            HandleConfirm(questionID, question);
            break;
    }
}

void psQuestionClient::HandleMarriage(uint32_t questionID, const csString & question)
{
    if(psengine->GetMarriageProposal())
        HandleConfirm(questionID, question);
    else
        SendResponseToQuestion(questionID, "no");
}

void psQuestionClient::SendResponseToQuestion(uint32_t questionID, const csString & answer)
{
    psQuestionResponseMsg msg(0,questionID,answer);
    psengine->GetMsgHandler()->SendMessage(msg.msg);
}

void psQuestionClient::DeleteQuestion(uint32_t questionID)
{
    psQuestion* q = questions.Get(questionID, NULL);
    if (q != NULL)
    {
        questions.Delete(questionID, q);
        delete q;
    }
}
