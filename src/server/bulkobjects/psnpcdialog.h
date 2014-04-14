/*
 * psnpcdialog.h
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
 *
 */
#ifndef __PS_IMP_NPC_DIALOG__
#define __PS_IMP_NPC_DIALOG__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <cstypes.h>
#include <iutil/comp.h>
#include <iutil/eventh.h>
#include <csutil/scf.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "util/psstring.h"

#include <idal.h>


//=============================================================================
// Local Includes
//=============================================================================
#include "dictionary.h"

class gemNPC;
class NPCDialogDict;
class NpcResponse;
class GemActor;
class Client;

struct iObjectRegistry;

struct KnowledgeArea
{
    csString area;
    int      priority;

    bool operator==(KnowledgeArea &other)
    {
        return (priority==other.priority && area == other.area);
    };
    bool operator<(KnowledgeArea &other)
    {
        if(priority<other.priority)
        {
            return true;
        }
        else if(priority>other.priority)
        {
            return false;
        }
        // Priority is equal so check area.
        if(strcmp(area,other.area)<0)
        {
            return true;
        }
        else
        {
            return false;
        }
    };
};

/**
 * Sentence written by the user represented as sequence of known terms.
 */
class NpcTriggerSentence
{
protected:
    csArray<NpcTerm*> terms;  ///< Not PArray because these ptrs are shared
    csString str;             ///< String version built from array

public:
    NpcTriggerSentence()  {};
    void AddToSentence(NpcTerm* next_word);
    size_t TermLength()
    {
        return terms.GetSize();
    }
    const csString &GetString();
    void operator=(NpcTriggerSentence &other)
    {
        terms = other.terms;
        str=other.str;
    }
    NpcTerm* &Term(size_t i)
    {
        return terms[i];
    }
    const char* GeneralizeTerm(NPCDialogDict* dict,size_t which, size_t depth);
};


/**
 * This class right now holds a simple circular MRU list
 * of responses, so the npc can tell if he is getting
 * the same question over and over.
 */
class DialogHistory
{
protected:
    struct DialogHistoryEntry
    {
        int playerID;    ///< Who was the response said to
        int responseID;  ///< What response was said
        csTicks when;    ///< Timestamp of response
    };
    csArray<DialogHistoryEntry> history;
    int counter;                          ///< Current location in circular buffer
public:
    DialogHistory()
    {
        counter=0;
    }

    void AddToHistory(int playerID, int responseID, csTicks when);
    bool EverSaid(int playerID, int responseID, csTicks &howLongAgo, int &times);
};
#define MAX_HISTORY_LEN 100

/**
 * This class exists per NPC, and holds all dialog triggers, responses
 * and scripts for this particular NPC by holding references to his/her
 * Knowledge Areas.
 */
class psNPCDialog
{
protected:
    gemNPC* self;
    csArray<KnowledgeArea*> knowareas;

    psString antecedent_her,
             antecedent_him,
             antecedent_it,
             antecedent_them;
    iDataConnection* db;
    csRandomGen* randomgen;
    gemActor* currentplayer;
    Client* currentClient;
    DialogHistory dialogHistory;

    NpcResponse* FindResponse(csString &trigger,const char* text, int questID = -1);
    NpcResponse* FindResponseWithAllPrior(const char* area,const char* trigger, int questID = -1);
    bool CheckPronouns(psString &text);
    void UpdateAntecedents(NpcResponse* resp);
    void AddBadText(const char* text,const char* trigger);

protected:
    void CleanPunctuation(psString &str, bool cleanQMark = true);

    /**
     * Recognizes phrases in the text and translates them to synonyms,
     * Ignores any unrecognized phrases. Returns array of recognized phrases.
     */
    void FilterKnownTerms(const psString &text, NpcTriggerSentence &trigger, Client* client);

    /**
     * Find a trigger by trying every combinations of generalization for each
     * term.
     */
    NpcResponse* FindOrGeneralizeTrigger(Client* client,NpcTriggerSentence &trigger,
                                         csArray<int> &gen_terms, int word);


    NpcResponse* ErrorResponse(const psString &text, const char* trigger);
    NpcResponse* RepeatedResponse(const psString &text, NpcResponse* resp, csTicks when, int times);

public:
    psNPCDialog(gemNPC* npc);
    virtual ~psNPCDialog();

    bool Initialize(iDataConnection* db, PID npcID);
    bool Initialize(iDataConnection* db);
    bool LoadKnowledgeAreas(PID npcID);

    NpcResponse* Respond(const char* text,Client* client);

    NpcResponse* FindXMLResponse(Client* client, csString trigger, int questID = -1);

    bool AddWord(const char* word);
    bool AddSynonym(const char* word,const char* synonym);
    bool AddKnowledgeArea(const char* ka_name);
    bool AddResponse(const char* area,const char* words,const char* response,const char* minfaction);
    bool AssignNPCArea(const char* npcname,const char* areaname);

    void SubstituteKeywords(Client* player, csString &response) const;


    /// Add a new trigger to the cached list.
    //    void AddNewTrigger( int databaseID );

    /// Add a new response to the cached list.
    //    void AddNewResponse ( int databaseID );

    void DumpDialog();
};

class psDialogManager;

class psTriggerBlock;
class psAttitudeBlock;
class psSpecialResponse;
class psResponse;

/**
 * This is used to handle \<specificknowledge\> and \<specialresponse\> tags.
 */
class psDialogManager
{
public:
    /**
     * Add a trigger to the list of triggers.
     */
    void AddTrigger(psTriggerBlock* trigger);

    /**
     * Add a special response to the internal list.
     */
    void AddSpecialResponse(psSpecialResponse* response);

    /**
     * A debuging function that prints all the trigger data.
     */
    void PrintInfo();

    /**
     * A list of all the triggers.
     *
     * This is a simple list of the all the trigger data that needs
     * to be created. It is of type psTriggerBlock.
     */
    csArray<psTriggerBlock*> triggers;


    /**
     * A list of all the special responses.
     *
     * This is a simple list of the all the special response data that needs
     * to be created. It is of type psSpecialResponse.
     */
    csArray<psSpecialResponse*> special;

    /**
     * Maps a internal ID to a database ID.
     *
     * When responses are loaded from a file they are given internal IDs.
     * When they are added to the database they are given a proper database
     * id.  This function returns that database ID based on the internal one
     * passed in.
     */
    int GetPriorID(int internalID);

    /**
     * Keep track of the database id's of the triggers.
     */
    csArray<int> triggerIDs;

    /**
     * Keep track of the database id's of responses.
     */
    csArray<int> responseIDs;

    csString    area;

    bool WriteToDatabase();

    int  InsertResponse(csString &response);
    int  InsertResponseSet(psResponse &response);
    int  InsertTrigger(const char* trigger, const char* area, int maxAttitude,
                       int minAttitude, int responseID, int priorID, int questID);
};


//--------------------------------------------------------------------------

/**
 * A special NPC response.
 *
 * This holds the data related to a special response. These are usually
 * fall back or error responses for NPCs.  The format of the tag is: <pre>
 *           \<specialresponse reactionMax="max"
 *                             reactionMin="min"
 *                             type="type"
 *                             value="expression"/\></pre>
 */
class psSpecialResponse
{
public:
    /**
     * Create the response based on the given tag.
     */
    psSpecialResponse(csRef<iDocumentNode> responseNode, int questID);

    csString type;        ///< The trigger name.

    csString response;    ///< The response given.

    int attMin;           ///< The min attitude required for this response.

    int attMax;           ///< The max attitude required for this response.

    int questID;
};


//--------------------------------------------------------------------------

/**
 * A set of trigger data.
 *
 * This manages all the data that is related to a trigger.  It consists of
 * a list of phrases and a list of attitudes.  For each attitude and phrase
 * a new trigger will be generated.  So 3 phrases and 2 attitudes will
 * generate 6 triggers.
 */
class psTriggerBlock
{
public:
    psTriggerBlock(psDialogManager* mgr)
    {
        manager = mgr;
        priorResponseID = 0;
        questID = -1;
    }

    /**
     * A list of phrases for this trigger.
     *
     * This allows us to create many triggers to map to many responses.
     */
    csStringArray phraseList;

    /**
     * The list of attitudes.
     *
     * This contain the responses based on an attitude range.
     */
    csArray<psAttitudeBlock*> attitudes;

    psTriggerBlock()
    {
        priorResponseID = 0;
    }

    /**
     * This is a recursive function that creates triggers.
     */
    void Create(csRef<iDocumentNode> triggerNode, int questID);

    /**
     * This sets the prior responseID for this trigger data.
     */
    void SetPrior(int id)
    {
        priorResponseID = id;
    }

    int priorResponseID;              ///< Holds the prior responseID.

    int databaseID;                   ///< Holds the real databaseID of this trigger.

    csString area;                    ///< The knowledge area of this trigger.

    psDialogManager* manager;         ///<  Main mangaer.

    int questID;                      ///< questID of trigger
};


//--------------------------------------------------------------------------

/**
 * A simple response.
 */
class psResponse
{
public:
    /**
     * The response lists.
     *
     * Each response set can have up to 5 different phrases that are
     * randomly picked from.
     */
    csStringArray responses;

    csString script;
    csString pronoun_him,
             pronoun_her,
             pronoun_it,
             pronoun_them;

    int givenID;              ///< The internal ID given by the dialog manager.

    int databaseID;           ///< The actual databaseID.

    int questID;              ///< The questID of the response

};


//--------------------------------------------------------------------------

/**
 * This is an attitude class data that has the responses.
 *
 * The attitude block is a set of responses based on a particular attitude
 * range.  This class keeps track of that data.
 */
class psAttitudeBlock
{
public:
    static int responseID;    ///< Used to generate the internal id's

    psAttitudeBlock(psDialogManager* mgr)
    {
        manager = mgr;
        responseSet.givenID = ++responseID;
    }

    psResponse responseSet;    ///< The response set for this attitude.

    /**
     * Populate the data for this class.
     */
    void Create(csRef<iDocumentNode> attitudeNode, int questID);

    int maxAttitude;           ///< The max attitude that these responses are for.

    int minAttitude;           ///< The min NPC attitude that these responses are for.

    psDialogManager* manager;  ///< Main dialog manager.
};

#endif



