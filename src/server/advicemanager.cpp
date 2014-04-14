/*
 * Advicemanager.cpp
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
#include "util/eventmanager.h"
#include "util/pserror.h"
#include "util/log.h"
#include "util/strutil.h"

#include "bulkobjects/pscharacterloader.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"
#include "client.h"
#include "clients.h"
#include "playergroup.h"
#include "gem.h"
#include "netmanager.h"
#include "advicemanager.h"
#include "adminmanager.h"

/**
 * This class is the relationship of Advisor to Advisee.
 */
class AdviceSession : public iDeleteObjectCallback
{
protected:

    csWeakRef<Client> advisor;  ///< Safe reference to the client who has indicated they can assist other users.
    csWeakRef<Client> advisee;  ///< Safe refereneces to the client who is requesting help.
    AdviceManager*    manager;  ///< The parent controller

public:

    uint32_t AdvisorClientNum; ///< cache the client number for the Advisor
    uint32_t AdviseeClientNum; ///< cache the client number for the Advisee

    csString adviseeName;      ///< cache the name of the advisee

    psAdviceRequestTimeoutGameEvent* requestEvent; ///< event to cancel the current request
    psAdviceSessionTimeoutGameEvent* timeoutEvent; ///< event to cancel the current session

    csString lastRequest; ///< cache the text of the last request
    int status;           ///< current status of the session -- this should be changed to an enum
    bool answered;        ///< has the initial question been answered?
    int advisorPoints;    ///< current value in advisor points for this session.
    int requestRetries;   ///< how many times the current request has been resent.

    AdviceSession()
    {
        manager = NULL;
        advisorPoints = 0;
        AdviseeClientNum = (uint32_t)-1;
        AdvisorClientNum = (uint32_t)-1;
        requestRetries = 0;
        answered = false;
        status = SESSION_STATUS_UNKNOWN;
        timeoutEvent = NULL;
        requestEvent = NULL;
    };

    AdviceSession(AdviceManager* mgr, Client* Advisor,
                  Client* Advisee, const char* request)
    {
        manager = mgr;

        requestEvent = NULL;
        timeoutEvent = NULL;

        AdviseeClientNum = (uint32_t)-1;
        AdvisorClientNum = (uint32_t)-1;

        advisorPoints = 1;
        requestRetries = 0;
        SetAdvisor(Advisor);
        if(Advisee)
        {
            advisee = Advisee;

            AdviseeClientNum = Advisee->GetClientNum();
            adviseeName = Advisee->GetName();
            advisee->GetActor()->RegisterCallback(this);
        }
        lastRequest = request;

        answered = false;
        status = SESSION_STATUS_UNKNOWN;
    };

    virtual ~AdviceSession()
    {
        if(advisor.IsValid()) advisor->GetActor()->UnregisterCallback(this);
        if(advisee.IsValid()) advisee->GetActor()->UnregisterCallback(this);
        // Cancel any timeouts tied to this session
        if(timeoutEvent)
            timeoutEvent->valid = false;
        if(requestEvent)
            requestEvent->valid = false;
    };

    Client* GetAdvisee()
    {
        return advisee;
    };

    void SetAdvisor(Client* newAdvisor)
    {
        // A NULL advisor is valid here, just clear the current advisor and return
        if(newAdvisor == NULL)
        {
            RemoveAdvisor();
            return;
        }

        // if there is a current advisor, unattach ourselves
        if(advisor.IsValid())
            advisor->GetActor()->UnregisterCallback(this);

        // assign new advisor
        advisor = newAdvisor;
        AdvisorClientNum = advisor->GetClientNum();
        advisor->GetActor()->RegisterCallback(this);
    };

    Client* GetAdvisor()
    {
        return advisor;
    };

    void RemoveAdvisor()
    {
        advisorPoints = 1;
        if(advisor.IsValid())
            advisor->GetActor()->UnregisterCallback(this);
        advisor = NULL;
        AdvisorClientNum = (uint32_t)-1;
    };

    virtual void DeleteObjectCallback(iDeleteNotificationObject* object)
    {
        gemActor* sender = (gemActor*)object;

        if(advisor.IsValid() && sender == advisor->GetActor())
        {
            manager->CancelAdvisorSession(sender, this, "been canceled");
            SetAdvisor(NULL);
            manager->RemoveSession(this);
        }

        else if(advisee.IsValid() && sender == advisee->GetActor())
        {
            manager->CancelAdvisorSession(sender, this, "been canceled");
            SetAdvisor(NULL);
            advisee->GetActor()->UnregisterCallback(this);
            manager->RemoveSession(this);
        }
    };

    void SessionTimeout()
    {
        manager->CancelAdvisorSession(NULL, this, "timed out");
        RemoveAdvisor();
    };
};

/****************************************************************************/

psAdviceRequestTimeoutGameEvent::psAdviceRequestTimeoutGameEvent(AdviceManager* mgr,
        int delayticks,
        gemActor* advisee,
        AdviceSession* adviceRequest)
    : psGEMEvent(0, delayticks, NULL,"psAdviceRequestTimeoutGameEvent")
{
    advicemanager  = mgr;
    adviceSession  = adviceRequest;

    //Handle dependencies ourselves since we have two.
    if(adviceSession->GetAdvisee())
    {
        adviseeActor = adviceSession->GetAdvisee()->GetActor();
        if(adviseeActor) adviseeActor->RegisterCallback(this);
    }
    else adviseeActor = NULL;

    if(adviceSession->GetAdvisor())
    {
        advisorActor = adviceSession->GetAdvisor()->GetActor();
        if(advisorActor) advisorActor->RegisterCallback(this);
    }
    else advisorActor = NULL;
}

psAdviceRequestTimeoutGameEvent::~psAdviceRequestTimeoutGameEvent()
{
    if(adviseeActor != NULL) adviseeActor->UnregisterCallback(this);
    adviseeActor = NULL;

    if(advisorActor != NULL) advisorActor->UnregisterCallback(this);
    advisorActor = NULL;

    if(adviceSession->requestEvent == this) adviceSession->requestEvent = NULL;
}

void psAdviceRequestTimeoutGameEvent::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    gemActor* sender = (gemActor*)object;

    if(adviseeActor)
    {
        if(sender == adviseeActor)
        {
            psGEMEvent::DeleteObjectCallback(object);
            adviseeActor->UnregisterCallback(this);
            if(advisorActor) advisorActor->UnregisterCallback(this);
            adviseeActor = advisorActor = NULL;
            return;
        }
    }

    if(advisorActor)
    {
        if(sender == advisorActor)
        {
            advisorActor->UnregisterCallback(this);
            advisorActor = NULL;
            adviceSession->RemoveAdvisor();

            advicemanager->AdviceRequestTimeout(adviceSession);
        }
    }
}

void psAdviceRequestTimeoutGameEvent::Trigger()
{
    advicemanager->AdviceRequestTimeout(adviceSession);
}


/****************************************************************************/

/****************************************************************************/
psAdviceSessionTimeoutGameEvent::psAdviceSessionTimeoutGameEvent(AdviceManager* mgr,
        int delayticks,
        gemActor* advisee,
        AdviceSession* adviceRequest)
    : psGEMEvent(0, delayticks, NULL,"psAdviceSessionTimeoutGameEvent")
{
    adviseeActor   = NULL;
    advisorActor   = NULL;
    advicemanager  = mgr;
    adviceSession  = adviceRequest;

    //Handle dependencies ourselves since we have two.
    if(adviceSession->GetAdvisee() != NULL)
    {
        adviseeActor = adviceSession->GetAdvisee()->GetActor();
        if(adviseeActor) adviseeActor->RegisterCallback(this);
    }

    // This should only happen when this is an unanswered question.
    // This event is canceled and recreated each time there is any activity on the session,
    // so this should be filled in when an advisor either claims or answers the question.
    if(adviceSession->GetAdvisor() != NULL)
    {
        advisorActor = adviceSession->GetAdvisor()->GetActor();
        if(advisorActor) advisorActor->RegisterCallback(this);
    }

}

psAdviceSessionTimeoutGameEvent::~psAdviceSessionTimeoutGameEvent()
{
    if(adviseeActor != NULL) adviseeActor->UnregisterCallback(this);
    adviseeActor = NULL;

    if(advisorActor != NULL) advisorActor->UnregisterCallback(this);
    advisorActor = NULL;

    if(adviceSession->timeoutEvent == this)adviceSession->timeoutEvent = NULL;
}

void psAdviceSessionTimeoutGameEvent::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    psGEMEvent::DeleteObjectCallback(object);

    //Handle dependencies ourselves since we have two.
    if(adviseeActor != NULL) adviseeActor->UnregisterCallback(this);
    adviseeActor = NULL;

    if(advisorActor != NULL) advisorActor->UnregisterCallback(this);
    advisorActor = NULL;
}

void psAdviceSessionTimeoutGameEvent::Trigger()
{
    adviceSession->SessionTimeout();
}

/****************************************************************************/


AdviceManager::AdviceManager(psDatabase* db)
{
    advisors.Empty();
    advisorPos = 0;

    database = db;

    Subscribe(&AdviceManager::HandleAdviceMessage,MSGTYPE_ADVICE,REQUIRE_READY_CLIENT);
}

AdviceManager::~AdviceManager()
{
    csHash<AdviceSession*>::GlobalIterator iter(AdviseeList.GetIterator());
    while(iter.HasNext())
        delete iter.Next();
}

void AdviceManager::HandleAdviceMessage(MsgEntry* me,Client* client)
{
    psAdviceMessage msg(me);
    if(!msg.valid)
    {
        psserver->SendSystemError(me->clientnum, "Invalid AdviceRequest Message.");
        Error2("Failed to parse psAdviceRequestMsg from client %u.",me->clientnum);
        return;
    }

    if(msg.sCommand == "on")    //this case handles /advisor on
    {
        // Is account banned from advising?
        if(client->IsAdvisorBanned())
        {
            psserver->SendSystemError(me->clientnum, "You are banned from advising!");
            return;
        }
        // GM's can always become advisors
        if(client->GetSecurityLevel() < GM_TESTER)
        {
            // Has account been on for long enough?
            if((client->GetAccountTotalOnlineTime() / 3600) < 30)
            {
                psserver->SendSystemError(me->clientnum, "You must be in game for more than 30 hours to become an advisor.");
                return;
            }
        }
        AddAdvisor(client);
    }
    else if(msg.sCommand == "off")    //this case handles /advisor off
    {
        RemoveAdvisor(client->GetClientNum(), me->clientnum);
    }
    else if(msg.sCommand == "sessions")    //this case handles /advisor sessions
    {
        HandleAdviseeList(client);
    }
    else if(msg.sCommand == "requests")    //this case handles /advisor requests
    {
        HandleAdviceList(client);
    }
    else if(msg.sCommand == "list")    //this case handles /advisor list
    {
        HandleListAdvisors(client);
    }
    else if(msg.sCommand == "/help")
    {
        if(!client->IsMute())
        {
            HandleAdviceRequest(client, msg.sMessage);
        }
        else
        {
            //User is muted but tries to chat anyway. Remind the user that he/she/it is muted
            psserver->SendSystemInfo(client->GetClientNum(),"You can't send messages because you are muted.");
        }
        client->FloodControl(CHAT_ADVICE, msg.sMessage, msg.sTarget);

    }
    else if(msg.sCommand == "/advice")
    {
        if(!client->IsMute())
        {
            HandleAdviceResponse(client, msg.sTarget, msg.sMessage);
        }
        else
        {
            //User is muted but tries to chat anyway. Remind the user that he/she/it is muted
            psserver->SendSystemInfo(client->GetClientNum(),"You can't send messages because you are muted.");
        }
        if(msg.sMessage.Length() != 0)
            client->FloodControl(CHAT_ADVICE, msg.sMessage, msg.sTarget);
    }
    else //Last case. Should happen only with /advisor being used alone
    {
        if(client->GetAdvisor())
        {
            psserver->SendSystemInfo(client->GetClientNum(),"You are an advisor.");
        }
        else
        {
            psserver->SendSystemInfo(client->GetClientNum(),"You are not an advisor.");
        }
    }
}

void AdviceManager::HandleAdviseeList(Client* advisor)
{
    if(!advisor->GetAdvisor())
    {
        psserver->SendSystemInfo(advisor->GetClientNum(),"You need to be an advisor to use this command.");
        return;
    }

    // find existing Advicee in the List
    AdviceSession* activeSession;
    csString message("You are currently advising:");
    csHash< AdviceSession* >::GlobalIterator loop(AdviseeList.GetIterator());
    bool found = false;
    while(loop.HasNext())
    {
        activeSession = loop.Next();
        if(activeSession && activeSession->adviseeName.GetData() && activeSession->AdvisorClientNum == advisor->GetClientNum())
        {
            message.Append("\n     ");
            message.Append(activeSession->adviseeName.GetData());
            found = true;
        }
        else
        {
            // bad activeSession here
        }
    }
    if(!found) message.Append("\n     noone");
    psserver->SendSystemInfo(advisor->GetClientNum(), message.GetData());
}

void AdviceManager::HandleAdviceList(Client* advisor)
{
    if(!advisor->GetAdvisor())
    {
        psserver->SendSystemInfo(advisor->GetClientNum(),"You need to be an advisor to use this command.");
        return;
    }

    // find existing Advicee in the List
    AdviceSession* activeSession;

    csHash< AdviceSession* >::GlobalIterator loop(AdviseeList.GetIterator());

    bool found = false;

    while(loop.HasNext())
    {
        activeSession = loop.Next();
        if(activeSession && activeSession->adviseeName.GetData() && activeSession->lastRequest.GetData()
                && activeSession->status == SESSION_STATUS_UNKNOWN && !activeSession->answered)
        {
            psChatMessage msgAdvisor(advisor->GetClientNum(), 0, activeSession->adviseeName, 0, activeSession->lastRequest, CHAT_ADVICE_LIST, false);
            msgAdvisor.SendMessage();
            found = true;
        }
        else
        {
            // bad activeSession here
        }
    }

    if(!found) psserver->SendSystemInfo(advisor->GetClientNum(),"There are no messengers waiting for an advisor.");

}

void AdviceManager::HandleListAdvisors(Client* advisor)
{
    psserver->SendSystemInfo(advisor->GetClientNum(),"There are %u advisors online.", advisors.GetSize());

    // If your not a GM that's all you get to know
    if(advisor->GetSecurityLevel() < GM_TESTER || !advisors.GetSize())
        return;

    psserver->SendSystemInfo(advisor->GetClientNum(),"They are:", advisors.GetSize());
    for(size_t i = 0; i < advisors.GetSize(); i++)
    {
        Client* client = psserver->GetConnections()->Find(advisors[i].id);
        //An advisor might have left the game in the same exact moment we are parsing this
        //so better to check if it's still valid before going on
        if(client)
            psserver->SendSystemInfo(advisor->GetClientNum(),"%s %s", client->GetName(), advisors[i].GM ? "(GM)" : "");

    }
}

void AdviceManager::HandleAdviceRequest(Client* advisee, csString message)
{
    //    psserver->SendSystemInfo( advisee->GetClientNum(), "Sorry for the inconvenience, you cannot request advice right now. The advice system is under maintenaince and will be available after a server restart soon.");
    //    return;

    if(advisee->GetAdvisor())
    {
        psserver->SendSystemInfo(advisee->GetClientNum(), "You cannot request advice while you are an advisor. Lay down your advisor role using \"/advisor off\", then request advice.");
        return;
    }

    if(advisors.GetSize() == 0)                                //No advisors are online
    {
        //Send back an message that there is no advisor online
        psserver->SendSystemInfo(advisee->GetClientNum(),"The world has no advisors at the moment. You may ask for further help on IRC or on our forums. You may also try to ask players around you for help with \"/tell <player name> <message>\".");
        return;
    }

    // find existing Advicee in the List
    AdviceSession* activeSession = AdviseeList.Get(advisee->GetClientNum(), NULL);

    // Create an adviceSession if one doesn't exist and the message is valid
    if(!activeSession)
    {
        WordArray words(message);
        // Check to make sure the request is 'good'
        if(words.GetCount() < 2)
        {
            psserver->SendSystemInfo(advisee->GetClientNum(),
                                     "An Advisor will need more information than this to help you, please include as much as you can in your request.");
            return;
        }

        Debug2(LOG_ANY, advisee->GetClientNum(), "Creating AdviceSession for %d", advisee->GetClientNum());
        activeSession = new AdviceSession(this, NULL, advisee, message);
        AdviseeList.Put(activeSession->AdviseeClientNum, activeSession);  // Advice is no longer pending.
    }
    else
    {
        // One unadvised unanswered question at a time
        if(!activeSession->GetAdvisor() && !activeSession->answered)
        {
            psserver->SendSystemError(advisee->GetClientNum(), "Please wait for an advisor to respond to your previous inquiry before asking another question.");
            return;
        }
    }
    //at this point active session will always be valid because it's either recovered from the
    //stored sessions or it was just created.
    // Create psAdviceRequestTimeoutGameEvent to timeout question.
    if(activeSession->timeoutEvent == NULL && activeSession->requestEvent == NULL)
    {
        psAdviceRequestTimeoutGameEvent* ev = new psAdviceRequestTimeoutGameEvent(this, ADVICE_QUESTION_TIMEOUT, advisee->GetActor(), activeSession);
        activeSession->requestEvent = ev;
        psserver->GetEventManager()->Push(ev);
    }
    else
    {
        WordArray words(message);

        // Check to make sure the request is 'good'
        if(words.GetCount() < 2)
        {
            psserver->SendSystemInfo(advisee->GetClientNum(), "An Advisor will need more information than this to help you, please include as much as you can in your request.");
            activeSession->answered = true;
            return;
        }

        if(activeSession->timeoutEvent)
        {
            activeSession->timeoutEvent->valid = false;
        }
        psAdviceSessionTimeoutGameEvent* ev = new psAdviceSessionTimeoutGameEvent(this, activeSession->answered?ADVICE_SESSION_TIMEOUT:ADVICE_SESSION_TIMEOUT/2, advisee->GetActor(), activeSession);
        activeSession->timeoutEvent = ev;
        psserver->GetEventManager()->Push(ev);
    }

    //we set the session as unanswered as we have a new message.
    activeSession->answered = false;
    activeSession->lastRequest = message;

    csString buf;

    if(activeSession->AdvisorClientNum != (uint32_t)-1)
    {
        // Source Client Name, Target Client Name, Message
        buf.Format("%s, %s, \"%s\"", advisee->GetName(), activeSession->GetAdvisor()->GetActor()->GetName(), message.GetData());
        psserver->GetLogCSV()->Write(CSV_ADVICE, buf);

        psChatMessage msgAdvisor(activeSession->AdvisorClientNum, 0, advisee->GetName(), 0, message , CHAT_ADVICE, false);
        msgAdvisor.SendMessage();

        // Send message to GM chars as well
        for(size_t i = 0; i < advisors.GetSize(); i++)
        {
            if(!advisors[i].GM || advisors[i].id == activeSession->AdvisorClientNum)
                continue;
            psChatMessage msgAdvisor(advisors[i].id, 0, advisee->GetName(), 0, message, CHAT_ADVICE, false);
            msgAdvisor.SendMessage();
        }
    }
    else
    {
        // Source Client Name, Target Client Name, Message
        buf.Format("%s, %s, \"%s\"", advisee->GetName(), "All Advisors", message.GetData());
        psserver->GetLogCSV()->Write(CSV_ADVICE, buf);

        for(size_t i = 0; i < advisors.GetSize(); i++)
        {
            psChatMessage msgAdvisor(advisors[i].id, 0, advisee->GetName(), 0, message, CHAT_ADVICE, false);
            msgAdvisor.SendMessage();
        }
    }
    psChatMessage msgChat(advisee->GetClientNum() , 0, advisee->GetName(), 0, message, CHAT_ADVICE, false);
    msgChat.SendMessage();


}

void AdviceManager::HandleAdviceResponse(Client* advisor, csString sAdvisee, csString message)
{
    if(!advisor->GetAdvisor())
    {
        psserver->SendSystemInfo(advisor->GetClientNum(),"You need to be an advisor to use this command.");
        return;
    }

    csString buf;
    // Source Client Name, Target Client Name, Message
    buf.Format("%s, %s, \"%s\"", advisor->GetName(), sAdvisee.GetData() , message.GetData());
    psserver->GetLogCSV()->Write(CSV_ADVICE, buf);

    // Find Advisee Client by name
    if(sAdvisee.Length())
    {
        sAdvisee = NormalizeCharacterName(sAdvisee);
    }
    Client* advisee = psserver->GetConnections()->Find(sAdvisee);
    if(!advisee)
    {
        // Create a new message to report TELL error and send
        // back to original person.
        csString sMsg("No player named ");
        sMsg += sAdvisee;
        sMsg += " is logged on to the system currently.";
        psserver->SendSystemError(advisor->GetClientNum(), sMsg);
        return;
    }

    // Can't allow you to advise yourself
    if(advisee == advisor)
    {
        psserver->SendSystemError(advisor->GetClientNum(), "You are not allowed to advise yourself. Please wait for another advisor.");
        return;
    }

    // find existing Advicee in the List
    AdviceSession key;
    key.AdviseeClientNum = advisee->GetClientNum();
    AdviceSession* activeSession = AdviseeList.Get(advisee->GetClientNum(), NULL);

    if(!activeSession || (activeSession  && (!activeSession->requestEvent) && (activeSession->GetAdvisor() == NULL)))
    {
        psserver->SendSystemError(advisor->GetClientNum(), "%s has not requested help.", advisee->GetName());
        return;
    }

    if(activeSession  && (activeSession->AdviseeClientNum != advisee->GetClientNum()))
    {
        Debug2(LOG_ANY, advisee->GetClientNum(), "Grabbed wrong advisor session: %d", activeSession->AdviseeClientNum);
    }

    if((activeSession->GetAdvisor() != NULL) && (activeSession->AdvisorClientNum != advisor->GetClientNum()))
    {
        psserver->SendSystemError(advisor->GetClientNum(), "%s is being advised already, thank you.",  advisee->GetName());
        return;
    }

    if(message.Length() == 0  && activeSession->status == SESSION_STATUS_UNKNOWN)    // advisor is claiming a session
    {
        // check to make sure advisor has only one claimed session.
        AdviceSession* loopSession;

        csHash< AdviceSession* >::GlobalIterator loop(AdviseeList.GetIterator());

        while(loop.HasNext())
        {
            loopSession = loop.Next();
            if(activeSession->status == SESSION_STATUS_CLAIMED && loopSession->GetAdvisor() == advisor)
            {
                psserver->SendSystemInfo(advisor->GetClientNum(), "You cannot have two messengers waiting for you at the same time, please answer %s's request first." , loopSession->adviseeName.GetData());
                return;
            }
        }

        activeSession->SetAdvisor(advisor);
        psserver->SendSystemInfo(advisee->GetClientNum(), "An advisor is preparing an answer to your question, please be patient.");
        psserver->SendSystemInfo(advisor->GetClientNum(), "You have claimed the session with %s. Please provide an answer." , advisee->GetName());

        for(size_t i = 0; i < advisors.GetSize(); i++)
        {
            if(advisors[i].id != activeSession->AdvisorClientNum)
            {
                psserver->SendSystemInfo(advisors[i].id, "%s has proclaimed they know the answer to %s's question.", advisor->GetName(), advisee->GetName());
            }
        }
        activeSession->status = SESSION_STATUS_CLAIMED;
    }
    else
    {
        if(message.IsEmpty())
        {
            psserver->SendSystemInfo(advisor->GetClientNum(), "Please enter the advice you wish to give.");
            return;
        }

        psChatMessage msgChat(activeSession->AdviseeClientNum, 0, advisor->GetName(), advisee->GetName(), message ,CHAT_ADVISOR,false);

        if(activeSession->GetAdvisor() == NULL || activeSession->status != SESSION_STATUS_OWNED)
        {
            // Check to make sure the advice is 'good'
            // if ( message.Length() < 20 )
            // {
            // psserver->SendSystemInfo(advisor->GetClientNum(), "Please be more specific when answering questions. Your advice has been ignored.");
            // return;
            // }

            //activeSession->AdvisorClientNum = me->clientnum;
            activeSession->SetAdvisor(advisor);
            advisor->IncrementAdvisorPoints(activeSession->advisorPoints);
            psserver->CharacterLoader.SaveCharacterData(advisor->GetCharacterData(), advisor->GetActor(), true);

            // Send Confirmation to advisor
            psserver->SendSystemInfo(advisor->GetClientNum(), "You are now advising %s.",advisee->GetName());

            // Send Confirmation to all other advisors so they know they lost this one.
            for(size_t i = 0; i < advisors.GetSize(); i++)
            {
                if(advisors[i].id != activeSession->AdvisorClientNum)
                {
                    psserver->SendSystemInfo(advisors[i].id, "%s has been assigned to %s.",  advisee->GetName(), advisor->GetName());
                    if(advisors[i].GM)
                        continue;
                    msgChat.msg->clientnum = advisors[i].id;
                    msgChat.SendMessage();
                }
            }
            activeSession->status = SESSION_STATUS_OWNED;
        }

        if(activeSession->requestEvent)
            activeSession->requestEvent->valid = false;  // This keeps the cancellation timeout from firing.

        activeSession->answered = true;

        // Send Message to Advisee
        msgChat.msg->clientnum = activeSession->AdviseeClientNum;
        msgChat.SendMessage();

        // Send Message to advisor
        msgChat.msg->clientnum = activeSession->AdvisorClientNum;
        msgChat.SendMessage();

        // Send message to GM chars as well
        for(size_t i = 0; i < advisors.GetSize(); i++)
        {
            if(!advisors[i].GM || advisors[i].id == activeSession->AdvisorClientNum)
                continue;
            msgChat.msg->clientnum = advisors[i].id;
            msgChat.SendMessage();
        }
    }

    // Add timeout for Advisor-Advisee relationship
    // this will allow later questions by the same client to go to a different advisor
    // spreading the wealth and burden amongst all ;)
    if(activeSession->timeoutEvent)
    {
        activeSession->timeoutEvent->valid = false;
    }
    psAdviceSessionTimeoutGameEvent* ev = new psAdviceSessionTimeoutGameEvent(this, activeSession->answered?ADVICE_SESSION_TIMEOUT:ADVICE_SESSION_TIMEOUT/2, advisee->GetActor(), activeSession);
    activeSession->timeoutEvent = ev;
    psserver->GetEventManager()->Push(ev);

}

void AdviceManager::AddAdvisor(Client* client)
{
    if(!client)
        return;

    uint32_t id = client->GetClientNum();

    // make sure we didn't do an "/advisor on" twice...
    for(size_t i = 0; i < advisors.GetSize(); i++)
    {
        if(id == advisors[i].id)
            return;
    }

    //check to see if there is an advice session from this user, and close it
    AdviceSession* adviceSession = AdviseeList.Get(id, NULL);
    if(adviceSession)
    {
        CancelAdvisorSession(NULL, adviceSession, "been canceled");
        RemoveSession(adviceSession);
    }

    // Create new Advisor structure
    AdvisorStruct advisor;
    advisor.id = id;
    advisor.ready = true;
    advisor.GM = client->GetSecurityLevel() >= GM_TESTER;

    advisors.Push(advisor);

    client->SetAdvisor(true);
    //Send message to advisor that he/she is an advisor
    psserver->SendSystemInfo(id,"Your request to become an advisor has been granted.");
    //psserver->SendSystemInfo(id, "You currently have %d advisor points.",  client->GetAdvisorPoints() );
}

void AdviceManager::RemoveAdvisor(uint32_t id, int connectionId)
{
    //Remove an advisor
    for(size_t i = 0; i < advisors.GetSize(); i++)
    {
        if(advisors[i].id == id)
        {
            Client* client = psserver->GetConnections()->Find(advisors[i].id);
            if(client)
                client->SetAdvisor(false);

            advisors.DeleteIndex(i);
            break;
        }
    }

    AdviceSession* activeSession = AdviseeList.Get(id, NULL);

    while(activeSession)
    {
        // Notify advisee his/her advisor is no longer available.
        CancelAdvisorSession(NULL, activeSession, "been canceled");
        RemoveSession(activeSession);
        activeSession = AdviseeList.Get(id, NULL);
    }

    //Send message to non-advisor that he/she isn't an advisor any more
    psserver->SendSystemInfo(connectionId,"You have just laid down your advisor role.");
}

void AdviceManager::AdviceRequestTimeout(AdviceSession* adviceSession)
{
    Client* adviseeClient = adviceSession->GetAdvisee();
    if(!adviseeClient)   // Questioner has himself gone offline now
        return;

    if(adviceSession->AdvisorClientNum != (uint32_t)-1)
    {
        //adviceSession->advisorPoints = 0;
        psserver->SendSystemInfo(adviceSession->AdviseeClientNum,"Your advisor appears to be busy at the moment, the messenger is still waiting for an answer.");
        psserver->SendSystemInfo(adviceSession->AdvisorClientNum,"%s is still waiting for an answer to their question.",adviseeClient->GetName());
        psChatMessage msgAdvisor(adviceSession->AdvisorClientNum, 0, adviseeClient->GetName(), 0, adviceSession->lastRequest, CHAT_ADVICE_LIST, false);
        msgAdvisor.SendMessage();
    }
    else
    {
        for(size_t i = 0; i < advisors.GetSize(); i++)
        {
            psserver->SendSystemInfo(advisors[i].id,"%s's messenger taps his foot impatiently.",adviseeClient->GetName());
            //psChatMessage msgAdvisor(advisors[i].client_id, 0, adviseeClient->GetName(), "RESEND:" + adviceSession->lastRequest,CHAT_ADVICE,false);
            //msgAdvisor.SendMessage();
        }
        psserver->SendSystemInfo(adviceSession->AdviseeClientNum,"All advisors appear to be busy at the moment, the messenger is still waiting for an answer.");
    }

    adviceSession->requestRetries += 1;
    if(adviceSession->requestRetries < ADVICE_QUESTION_RETRIES)
    {
        psAdviceRequestTimeoutGameEvent* ev = new psAdviceRequestTimeoutGameEvent(this, ADVICE_QUESTION_TIMEOUT, adviseeClient->GetActor(), adviceSession);
        adviceSession->requestEvent = ev;
        psserver->GetEventManager()->Push(ev);
    }
    else
    {
        adviceSession->requestRetries = 0;
        adviceSession->answered = true;
        adviceSession->requestEvent = NULL;
        psserver->SendSystemInfo(adviceSession->AdviseeClientNum,"Your messenger has returned without an answer, please try again later.");
    }

}

void AdviceManager::CancelAdvisorSession(gemActor* who, AdviceSession* adviceSession, const char* msg)
{
    Debug1(LOG_ANY, adviceSession->AdviseeClientNum,"Advice session cancelling now.\n");

    if(adviceSession->GetAdvisee())
    {
        if((who == NULL) || (adviceSession->GetAdvisor() &&
                             who == adviceSession->GetAdvisor()->GetActor()))
        {
            if(adviceSession->AdviseeClientNum != (uint32_t)-1)
            {
                psserver->SendSystemInfo(adviceSession->AdviseeClientNum,"Your advice session has %s.", msg);
                switch(adviceSession->status)
                {
                    case SESSION_STATUS_OWNED:
                        psserver->SendSystemInfo(adviceSession->AdviseeClientNum,"Your next request may be handled by a different advisor.");
                        if(adviceSession->requestEvent)
                        {
                            adviceSession->requestEvent->valid = false;
                            adviceSession->requestEvent = NULL;
                        }
                        adviceSession->answered = true;
                        break;
                    case SESSION_STATUS_CLAIMED:
                        psserver->SendSystemInfo(adviceSession->AdviseeClientNum,"Your request will be sent to all advisors.");
                        break;
                    case SESSION_STATUS_UNKNOWN:
                        break;
                }
            }
        }
    }

    if(adviceSession->GetAdvisor())
    {
        if((who == NULL) || ((adviceSession->GetAdvisee() && who == adviceSession->GetAdvisee()->GetActor())))
        {
            if(adviceSession->AdvisorClientNum != (uint32_t)-1)
            {
                if(who == adviceSession->GetAdvisor()->GetActor() || who == NULL)
                {
                    switch(adviceSession->status)
                    {
                        case SESSION_STATUS_OWNED:
                            psserver->SendSystemInfo(adviceSession->AdvisorClientNum,"Your advice session with %s has %s.",adviceSession->adviseeName.GetData(), msg);
                            break;
                        case SESSION_STATUS_CLAIMED:
                            psserver->SendSystemInfo(adviceSession->AdvisorClientNum,"You have forfieted your session with %s.",adviceSession->adviseeName.GetData());
                            // Notify all other advisors
                            for(size_t i = 0; i < advisors.GetSize(); i++)
                            {
                                if(advisors[i].id != adviceSession->AdvisorClientNum)
                                    psserver->SendSystemInfo(advisors[i].id,"%s has relinquished their claim as %s's advisor.",adviceSession->GetAdvisor()->GetName(), adviceSession->adviseeName.GetData());
                            }
                            adviceSession->SetAdvisor(NULL);

                            break;
                        case SESSION_STATUS_UNKNOWN:
                            break;
                    }
                }
                else
                {
                    psserver->SendSystemInfo(adviceSession->AdvisorClientNum,"Your advice session with %s has %s.",adviceSession->adviseeName.GetData(), msg);
                }
            }
        }
    }

    adviceSession->status = SESSION_STATUS_UNKNOWN;
}

void AdviceManager::RemoveSession(AdviceSession* adviceSession)
{
    Debug2(LOG_ANY, adviceSession->AdviseeClientNum, "Removing AdviceSession: %d", adviceSession->AdviseeClientNum);

    if(adviceSession->requestEvent)
    {
        adviceSession->requestEvent->valid = false;
        adviceSession->requestEvent = NULL;
    }

    AdviseeList.DeleteAll(adviceSession->AdviseeClientNum);
    delete adviceSession;
}

#if 0
bool AdviceManager::GetAdvisorMode(int id, int connectionId)
{
    //Search loop that will found the right advisor nr in the advisors array and use it as an id
    for(size_t i = 0; i < advisors.GetSize(); i++)
    {
        if((advisors[i].id == id))
            return true;
    }
    return false;
}
#endif
