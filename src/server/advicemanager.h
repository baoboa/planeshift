/*
 * Advicemanager.h
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


#ifndef __ADVICEMANAGER_H__
#define __ADVICEMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"         // parent class
#include "gem.h"

class psAdviceRequestTimeoutGameEvent;
class psAdviceSessionTimeoutGameEvent;
class Client;
class psDatabase;
class AdviceManager;
class AdviceSession;

#define ADVICE_QUESTION_TIMEOUT    120000
#define ADVICE_QUESTION_RETRIES         5
#define ADVICE_SESSION_TIMEOUT     300000

#define SESSION_STATUS_UNKNOWN 0
#define SESSION_STATUS_CLAIMED 1
#define SESSION_STATUS_OWNED   2

/**
 * Holds data on an 'advisor' in the world.
 *
 * An advisor is self appointed person that answers questions in game that
 * people have. First Time questions are broadcast to all advisors, first
 * advisor to answer gets points and the questioneer for the rest of the session.
 */
struct AdvisorStruct
{
    uint32_t id;
    csString request;
    bool ready;
    bool GM;
};



/**
 * AdviceManager keeps track of all the invitations of any kind
 * that are pending, using a collection of PendingAdvice objects.
 *
 * An invitation is pending between the time that the Noob
 * sends the invitation and when an Advisor sends a response.
 */
class AdviceManager : public MessageManager<AdviceManager>
{
public:

    AdviceManager(psDatabase* db);
    virtual ~AdviceManager();

    void HandleAdviceMessage(MsgEntry* pMsg,Client* client);

    /**
     * Adds a new advisor available to answer questions.
     *
     * @param client The player client.
     */
    void AddAdvisor(Client* client);

    /**
     * Remove advisor available to answer questions.
     *
     * @param id The id of the advisor to remove ( same as player ID )
     * @param connectionId The client id that client dropping advisor role.
     */
    void RemoveAdvisor(uint32_t id, int connectionId);

    /**
     * Resend request for advice due to timeout.
     *
     * @param adviceSession The session that has expired
     */
    void AdviceRequestTimeout(AdviceSession* adviceSession);

    /**
     * Remove advise session due to timeout.
     *
     * @param who The actor.
     * @param adviceSession The session that has expired.
     * @param msg A message for the player.
     */
    void CancelAdvisorSession(gemActor* who, AdviceSession* adviceSession, const char* msg);

    void RemoveSession(AdviceSession* adviceSession);

    /**
     * Retrieve the clients advisor mode.
     *
     * @param id The id of the advisor to remove ( same as player ID )
     * @param connectionId The client id that client dropping advisor role.
     */
    bool GetAdvisorMode(int id, int connectionId);

protected:

    void HandleAdviceRequest(Client* advisee, csString message);
    void HandleAdviceResponse(Client* advisee, csString sAdvisee, csString message);
    void HandleAdviceList(Client* advisor);
    void HandleAdviseeList(Client* advisor);
    void HandleListAdvisors(Client* advisor);

    bool FloodControl(csString &newmessage, Client* client);

    csHash<AdviceSession*> AdviseeList;
    csArray<AdvisorStruct>    advisors;
    size_t advisorPos;
    psDatabase*              database;
};

class psAdviceSessionTimeoutGameEvent : public psGEMEvent
{
protected:
    AdviceManager* advicemanager;
    gemActor* adviseeActor;
    gemActor* advisorActor;

public:
    AdviceSession* adviceSession;

    psAdviceSessionTimeoutGameEvent(AdviceManager* mgr,
                                    int delayticks,
                                    gemActor* advisee,
                                    AdviceSession* adviceRequest);

    virtual ~psAdviceSessionTimeoutGameEvent();

    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    virtual void Trigger();
};

class psAdviceRequestTimeoutGameEvent : public psGEMEvent
{
protected:
    AdviceManager* advicemanager;
    gemActor* adviseeActor;
    gemActor* advisorActor;

public:
    AdviceSession* adviceSession;

    psAdviceRequestTimeoutGameEvent(AdviceManager* mgr,
                                    int delayticks,
                                    gemActor* advisee,
                                    AdviceSession* adviceRequest);

    virtual ~psAdviceRequestTimeoutGameEvent();

    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    virtual void Trigger();
};

#endif



