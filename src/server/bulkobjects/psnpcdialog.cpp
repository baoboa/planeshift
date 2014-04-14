/*
* psnpcdialog.cpp
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
#include <ctype.h>


//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>
#include <iutil/objreg.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <iutil/evdefs.h>
#include <iutil/virtclk.h>

#include <imesh/sprite3d.h>

#include <csutil/databuf.h>
#include <csutil/plugmgr.h>
#include <iengine/movable.h>
#include <iengine/engine.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/strutil.h"
#include "util/psstring.h"
#include "util/serverconsole.h"
#include "util/log.h"
#include "util/psdatabase.h"

#include <idal.h>

#include "../psserver.h"
#include "../weathermanager.h"
#include "../gem.h"
#include "../playergroup.h"
#include "../client.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "dictionary.h"
#include "psnpcdialog.h"
#include "psraceinfo.h"
#include "psquest.h"
extern "C" {
#include "../tools/wordnet/wn.h"
}

//----------------------------------------------------------------------------

void NpcTriggerSentence::AddToSentence(NpcTerm* next_word)
{
    assert(next_word!=NULL);
    terms.Push(next_word);
    str="";
}


const csString &NpcTriggerSentence::GetString()
{
    if(str.Length())
        return str;

    for(size_t i=0; i<terms.GetSize(); i++)
    {
        str.Append(terms[i]->term);
        if(i<terms.GetSize()-1)
            str.Append(' ');
    }
    return str;
}

const char* NpcTriggerSentence::GeneralizeTerm(NPCDialogDict* dict,size_t which, size_t depth)
{
    if(which >= terms.GetSize())
    {
        return NULL;
    }

    str.Clear();

    return terms[which]->GetInterleavedHypernym(depth);
}




//----------------------------------------------------------------------------



psNPCDialog::psNPCDialog(gemNPC* npc)
{
    db = NULL;
    randomgen = NULL;
    self = npc;
}

psNPCDialog::~psNPCDialog()
{
    {
        csArray<KnowledgeArea*>::Iterator it(knowareas.GetIterator());
        while(it.HasNext())
        {
            delete it.Next();
        }
        knowareas.Empty();
    }
}

bool psNPCDialog::Initialize(iDataConnection* db)
{
    this->db = db;

    if(!dict)
    {
        dict = new NPCDialogDict;

        if(!dict->Initialize(db))
        {
            delete dict;
            dict=NULL;
            return false;
        }
    }
    return true;
}


bool psNPCDialog::Initialize(iDataConnection* db, PID NPCID)
{
    randomgen = psserver->rng;
    this->db = db;
    // Initialize base dictionary
    if(!dict.IsValid())
    {
        dict.AttachNew(new NPCDialogDict());

        if(!dict->Initialize(db))
        {
            return false;
        }
    }

    return LoadKnowledgeAreas(NPCID);
}

bool psNPCDialog::LoadKnowledgeAreas(PID npcID)
{
    Result result(db->Select("select area,priority"
                             "  from npc_knowledge_areas"
                             " where player_id=%u order by priority ASC", npcID.Unbox()));

    if(!result.IsValid())
    {
        Error1("Cannot load knowledge areas into dictionary from database.");
        return false;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        KnowledgeArea* newarea = new KnowledgeArea;

        // Downcase KA area before inserting into tree
        csString area = result[i]["area"];
        newarea->area = area.Downcase();

        newarea->priority = result[i].GetInt("priority");

        knowareas.Push(newarea);
    }
    return true;
}

void psNPCDialog::DumpDialog()
{
    CPrintf(CON_CMDOUTPUT,"Pri Knowledge area\n");
    for(size_t z = 0; z < knowareas.GetSize(); z++)
    {
        KnowledgeArea* ka = knowareas[z];
        CPrintf(CON_CMDOUTPUT,"%3d %s\n",ka->priority,ka->area.GetDataSafe());
    }
}

bool psNPCDialog::CheckPronouns(psString &text)
{
    int wordnum=1;
    psString word("temp");

    while(word.Length())
    {
        word = GetWordNumber(text,wordnum++);

        if(word == "him" || word=="he")
        {
            if(!antecedent_him.IsEmpty())
                text.ReplaceSubString(word,antecedent_him);
        }

        if(word == "her" || word=="she")
        {
            if(!antecedent_her.IsEmpty())
                text.ReplaceSubString(word,antecedent_her);
        }

        if(word == "them" || word=="they")
        {
            if(!antecedent_them.IsEmpty())
                text.ReplaceSubString(word,antecedent_them);
        }

        if(word == "it")
        {
            if(!antecedent_them.IsEmpty())
                text.ReplaceSubString(word,antecedent_them);
        }
    }
    return true;
}

void psNPCDialog::CleanPunctuation(psString &str, bool cleanQMark)
{
    for(unsigned int i=0; i<str.Length(); i++)
    {
        if(ispunct(str.GetAt(i)) && str.GetAt(i)!='\'' && (str.GetAt(i)!='?' || cleanQMark))
        {
            str.DeleteAt(i);
            i--;
        }
    }

    // we also use this function to remove the NPC's own name from the dialog
    psString wordInName,npc_name;
    wordInName = "bla";
    npc_name = self->GetName();
    int num = 1;
    while(wordInName.Length())
    {
        wordInName = GetWordNumber(npc_name,num);
        if(!wordInName.Length())
            break;
        int pos = (int)str.FindSubString(wordInName,0,XML_CASE_INSENSITIVE);
        if(pos != -1)
        {
            str.DeleteAt(pos,wordInName.Length());
        }
        num++;
    }
}

void psNPCDialog::FilterKnownTerms(const psString &text, NpcTriggerSentence &trigger, Client* client)
{
    const size_t MAX_SENTENCE_LENGTH = 4;
    NpcTerm* term;

    WordArray words(text);
    size_t numWordsInPhrase = words.GetCount();
    size_t firstWord=0;
    csString candidate;
    bool morphed = false;

    if(!dict)    // Pointless to try if no dictionary loaded.
        return;

    Debug2(LOG_NPC, client->GetClientNum(),"Recognizing phrases in '%s'", text.GetData());

    while(firstWord<words.GetCount() && trigger.TermLength()<MAX_SENTENCE_LENGTH)
    {
        if(!morphed)
            candidate = words.GetWords(firstWord, firstWord+numWordsInPhrase);
        term  = dict->FindTermOrSynonym(candidate);
        if(term)
        {
            trigger.AddToSentence(term);
            firstWord += numWordsInPhrase;
            numWordsInPhrase = words.GetCount() - firstWord;
            morphed = false;
            continue;
        }
        if(numWordsInPhrase > 1)
        {
            numWordsInPhrase--;
            continue;
        }
        if(!morphed)   // try stemming the word to see if it matches
        {
            // assume all words are nouns
            const char* morphedWord = morphword(const_cast<char*>(candidate.GetData()), NOUN);
            if(morphedWord)
            {
                candidate = morphedWord;
                morphed = true;
                continue;
            }
        }
        // stemming failed to match so move on

        morphed = false;
        firstWord ++;
        numWordsInPhrase = (int)words.GetCount() - firstWord;
    }

    Debug2(LOG_NPC, client->GetClientNum(),"Phrases recognized: '%s'", trigger.GetString().GetData());
}

void psNPCDialog::AddBadText(const char* text, const char* trigger)
{
    csString escText,escTrigger;
    csString escName;
    csString escSelfName;

    db->Escape(escText, text);
    db->Escape(escName, currentClient->GetName());
    db->Escape(escSelfName, self->GetName());
    db->Escape(escTrigger, trigger);
    int ret = db->Command("insert into npc_bad_text "
                          "(badtext,triggertext,player,npc,occurred) "
                          "values ('%s','%s','%s','%s',Now() )",
                          escText.GetData(),
                          escTrigger.GetData(),
                          escName.GetData(),
                          escSelfName.GetData());
    if((uint)ret == QUERY_FAILED)
    {
        Error2("Inserting npc_bad_text failed: %s",db->GetLastError());
    }

    self->AddBadText(text,trigger);  // Save bad text in RAM cache so Settings can get at it easily in-game.
}


//Check current trigger with all prior responses in quests, and in general
NpcResponse* psNPCDialog::FindResponseWithAllPrior(const char* area,const char* trigger, int questID)
{
    NpcResponse* resp = NULL;
    int lastresponse = -1;
    bool TestedWithoutLastResponse = false;

    //first try with last responses of all assigned quests
    for(size_t q = 0; currentClient && q < currentClient->GetCharacterData()->GetQuestMgr().GetNumAssignedQuests(); q++)
    {
        lastresponse = currentClient->GetCharacterData()->GetQuestMgr().GetAssignedQuestLastResponse(q);
        if(lastresponse == -1)
        {
            if(!TestedWithoutLastResponse)
            {
                resp = dict->FindResponse(self, area,trigger,0, lastresponse ,currentClient, questID);
                TestedWithoutLastResponse = true;
            }
        }
        else
        {
            resp = dict->FindResponse(self, area,trigger,0, lastresponse ,currentClient, questID);
        }

        if(resp)
            break;
    }
    if(!resp)  //else, try old way with general last response
    {
        lastresponse = currentClient ? currentClient->GetCharacterData()->GetLastResponse() : -1;
        if(lastresponse == -1)
        {
            if(!TestedWithoutLastResponse)
            {
                resp = dict->FindResponse(self, area,trigger,0,lastresponse,currentClient, questID);
                TestedWithoutLastResponse = true;
            }
        }
        else
        {
            resp = dict->FindResponse(self, area,trigger,0,lastresponse,currentClient, questID);
        }
    }
    if(currentClient && !resp && TestedWithoutLastResponse)
    {
        //set general last response to -1 so it is not tested again later on
        //currentClient->GetCharacterData()->SetLastResponse(-1);
        // I commented out the above because searching unsuccessfully should not have the side effect of blowing away the last valid response.  KWF
    }
    return resp;
}

NpcResponse* psNPCDialog::FindResponse(csString &trigger,const char* text, int questID)
{
    KnowledgeArea* area;
    NpcResponse* resp = NULL;
    csString trigger_error;

    //May be it is safe not to check for characterdata (now needed for GetLastRespons())
    if(currentClient && currentClient->GetCharacterData() == NULL)
    {
        Error1("NpcResponse *psNPCDialog::FindResponse(csString& trigger,const char *text) called with "
               "currentClient->GetCharacterData() returning NULL.");
        return NULL;
    }

    if(trigger.GetData() == NULL)
        return NULL;

    trigger.Downcase();

    trigger_error = trigger;
    trigger_error.Append(" error");

    for(size_t z = 0; z < knowareas.GetSize(); z++)
    {
        area = knowareas[z];
        Debug5(LOG_NPC, currentClient ? currentClient->GetClientNum() : 0,"NPC checking %s for trigger %s , with lastResponseID %d and questID %d...",
               (const char*)area->area,(const char*)trigger, currentClient ? currentClient->GetCharacterData()->GetLastResponse() : -1, questID);

        resp = FindResponseWithAllPrior(area->area, trigger, questID);
        if(!resp)  // If no response found, try search for error trigger
        {
            resp = FindResponseWithAllPrior(area->area, trigger_error, questID);
            if(!resp)  // If no response found, try search without last response
            {
                if(!currentClient || currentClient->GetCharacterData()->GetLastResponse() == -1)
                {
                    // No point testing without last response
                    // if last response where no last response.
                    continue;
                }

                resp = dict->FindResponse(self, area->area,trigger,0,-1,currentClient, questID);
                if(!resp)  // If no response found, try search for error trigger without last response
                {
                    resp = dict->FindResponse(self, area->area,trigger_error,0,-1,currentClient, questID);
                    if(resp)
                    {
                        // Force setting of type Error if error trigger found
                        resp->type = NpcResponse::ERROR_RESPONSE;
                    }
                }
            }
            else
            {
                // Force setting of type Error if error trigger found
                resp->type = NpcResponse::ERROR_RESPONSE;
            }
        }

        if(resp)
        {
            Debug4(LOG_NPC, currentClient ? currentClient->GetClientNum() : 0,"Found response %d: %s (%s)",resp->id,resp->GetResponse(), resp->GetResponseScript().GetData());
            resp->triggerText = trigger;
            UpdateAntecedents(resp);
            break;
        }
    }
    return resp;
}

void psNPCDialog::SubstituteKeywords(Client* player, csString &resp) const
{
    psString dollarsign("$"),response(resp);
    int where = response.FindSubString(dollarsign);

    while(where!=-1)
    {
        psString word2,word;
        response.GetWord(where+1,word2,psString::NO_PUNCT);

        word = "$";
        word.Append(word2);  // include $sign in subst.

        if(word == "$playername")
        {
            if(!response.ReplaceSubString(word,player->GetName()))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        else if(word == "$playerrace")
        {
            if(!response.ReplaceSubString(word, player->GetCharacterData()->GetRaceInfo()->GetName()))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        else if(word == "$sir")
        {
            csString sir(player->GetCharacterData()->GetRaceInfo()->GetHonorific());

            if(!response.ReplaceSubString(word,sir))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        else if(word == "$his")
        {
            csString his(player->GetCharacterData()->GetRaceInfo()->GetPossessive());
            if(!response.ReplaceSubString(word,his))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        else if(word == "$time")  //changes a $time variable with a sentence like night morning afternoon or evening
        {
            csString timestr; //used to store the string which will be replaced in the text

            int clockHour = psserver->GetWeatherManager()->GetGameTODHour(); //gets the current hour

            //checks what type of string should be applied according to the hour
            if(clockHour < 6)
                timestr = "night";
            else if(clockHour < 12)
                timestr = "morning";
            else if(clockHour < 18)
                timestr = "afternoon";
            else
                timestr = "evening";
            if(!response.ReplaceSubString(word,timestr))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),timestr.GetData());
            }
        }
        else if(word == "$npc")
        {
            if(response.ReplaceSubString(word,self->GetName()))
            {
                Error4("Failed to replace substring %s in %s with %s",word.GetData(),response.GetData(),player->GetName());
            }
        }
        where = response.FindSubString(dollarsign,where+1);
    }
    resp = response;
}

NpcResponse* psNPCDialog::FindOrGeneralizeTrigger(Client* client,NpcTriggerSentence &trigger,
        csArray<int> &gen_terms, int word)
{
    NpcResponse* resp;
    csStringArray generalized;

    bool hit;

    // Perform breadth-first search on generalisations

    // Copy all terms into stringarray
    for(size_t i=0; i<trigger.TermLength(); i++)
        generalized.Push(trigger.Term(i)->term);

    // We do at least one search with no generalisations
    size_t depth = 0;

    do
    {
        hit = false;

        for(size_t i=0; i<gen_terms.GetSize(); i++)
        {
            const char* hypernym = trigger.Term(gen_terms[i])->GetInterleavedHypernym(depth);
            if(hypernym)
            {
                generalized.Put(gen_terms[i],hypernym);
                hit = true;
            }

            csString generalized_copy;

            // Merge string
            for(size_t i=0; i<generalized.GetSize(); i++)
            {
                generalized_copy.Append(generalized[i]);
            }

            Debug2(LOG_NPC, 0, "Searching for generalized trigger: %s'\n", generalized_copy.GetDataSafe());

            dict->CheckForTriggerGroup(generalized_copy);  // substitute master trigger if this is child trigger in group

            resp = FindResponse(generalized_copy, trigger.GetString());
            if(resp)
            {
                Debug2(LOG_NPC, client->GetClientNum(),"Found response to: '%s'", generalized_copy.GetData());

                //         // Removed till we find a better way to manage repeated responses
                //         // At the moment are annoying since you cannot restart the conversation from
                //         // a certain point
                //         int times;
                //         csTicks when;
                //         if (dialogHistory.EverSaid(client->GetPID(), resp->id, when, times))
                //         {
                //             return RepeatedResponse(trigger.GetString(), resp, when, times);
                //         }
                //         else
                //             dialogHistory.AddToHistory(client->GetPID(), resp->id, csGetTicks() );


                return resp; // Found what we are looking for
            }
        }
        depth++;
    }
    while(hit);

    return NULL;
}

NpcResponse* psNPCDialog::Respond(const char* text,Client* client)
{
    NpcResponse* resp = NULL;
    NpcTriggerSentence trigger,generalized;
    psString pstext(text);
    int questIDHint = -1;

    // Cut the first { } as quest hint, if it's there.
    if(pstext.GetAt(0) == '{')
    {
        // As the starting element was found search the closing one.
        size_t endPos = pstext.Find("}");
        if(endPos != (size_t)-1)
        {
            csString id = pstext.Slice(1, endPos - 1);
            // Quest id can only be positive.
            unsigned long value = strtoul(id.GetData(), NULL, 0);

            // Assign to the questIDHint and check for overflow attempts
            questIDHint = value;
            if(questIDHint < -1)
            {
                questIDHint = -1;
            }

            // Remove the special command from the trigger.
            pstext = pstext.Slice(endPos + 1);
        }
    }

    //limit text to the maximum allocated in wordnet.
    pstext.Truncate(WORDBUF);

    currentplayer = client->GetActor();
    currentClient = client;

    // Removes everything except alphanumeric character and spaces
    // Removes the NPC name
    CleanPunctuation(pstext);

    // Replace him/he,her/she,them/they,it with the stored
    // antecedent
    if(!CheckPronouns(pstext))
    {
        Debug2(LOG_NPC, currentClient->GetClientNum(),"Failed pronouns check for \"%s\"",pstext.GetDataSafe());
        return ErrorResponse(pstext,"(none)");
    }

    // Replace custom known terms to get standard terms
    // eg. hello is replaced with greetings
    FilterKnownTerms(pstext, trigger, client);

    if(trigger.TermLength() == 0)
    {
        Debug1(LOG_NPC, currentClient->GetClientNum(),"Failed filter known terms check");
        psString pstextError(text);
        CleanPunctuation(pstextError, false);
        return ErrorResponse(pstextError,"(no known words)");
    }

    csString copy;
    copy = trigger.GetString();
    dict->CheckForTriggerGroup(copy);  // substitute master trigger if this is child trigger in group

    // This is the generic version that does not use WordNet
    // First check with the hint, then check without the hint, no need to check also in the next part
    // as these should be precise hits as generated from the menu.
    if(questIDHint >= 0)
    {
        resp = FindResponse(copy, trigger.GetString(), questIDHint);
    }

    // If there is no hint, or the quest hint yielded nothing try without the hint.
    if(!resp)
    {
        resp = FindResponse(copy, trigger.GetString());
    }


    // Try each word (in reverse order) of the trigger by itself before giving up
    if(!resp)
    {
        WordArray words(trigger.GetString());
        if(words.GetCount()>1)  //no need to do this if there is only one word.
        {
            for(size_t i=words.GetCount(); i>0; i--)
            {
                csString word(words.Get(i-1));
                Debug2(LOG_NPC, currentClient->GetClientNum(), "psNPCDialog::Respond: Trying word: '%s'\n", word.GetDataSafe());

                resp = FindResponse(word, text);
                if(resp)
                {
                    resp->triggerText = trigger.GetString();
                    break;
                }
            }
        }
    }

    if(resp)
    {
        Debug2(LOG_NPC, currentClient->GetClientNum(),"Found response to: '%s'", copy.GetData());

        //May be it is safe not to check for characterdata (now needed for GetLastRespons())
        if(currentClient->GetCharacterData() == NULL)
        {
            Error1("NpcResponse *psNPCDialog::Respond(const char * text,Client *client) called with "
                   "currentClient->GetCharacterData() returning NULL.");
        }
        else
        {
            Debug3(LOG_NPC, currentClient->GetClientNum(),"Setting last response %d: %s",resp->id,resp->GetResponse());
            currentClient->GetCharacterData()->SetLastResponse(resp->id);
            Debug4(LOG_NPC, currentClient->GetClientNum(),"Setting last response for quest '%s', %d: %s",
                   resp->quest ? resp->quest->GetName() : "none", resp->id, resp->GetResponse());
            currentClient->GetCharacterData()->GetQuestMgr().SetAssignedQuestLastResponse(resp->quest,resp->id, currentClient->GetTargetObject());
        }

        return resp; // Found what we are looking for
    }
    else
    {
        Debug1(LOG_NPC, currentClient->GetClientNum(),"No response found");
        return ErrorResponse(pstext,trigger.GetString());
    }
}

NpcResponse* psNPCDialog::FindXMLResponse(Client* client, csString trigger, int questID)
{
    if(!client)
    {
        currentplayer = NULL;
        currentClient = NULL;
    }
    else
    {
        currentplayer = client->GetActor();
        currentClient = client;
    }

    return FindResponse(trigger, trigger.GetDataSafe(), questID);
}

NpcResponse* psNPCDialog::RepeatedResponse(const psString &text, NpcResponse* resp, csTicks when, int times)
{
    const char* time_description;
    if(when < 30000)   // 30 seconds
        time_description = "just now";
    else if(when < 300000)  // 5 minutes
        time_description = "recently";
    else
        time_description = "already";

    csString key;
    key.Format("repeat %s %d",time_description,times);
    resp = FindResponse(key,text);
    if(!resp)
    {
        key.Format("repeat %s",time_description);
        resp = FindResponse(key,text);
        if(!resp)
        {
            key = "repeat";
            resp = FindResponse(key,text);
        }
    }
    return resp;
}

NpcResponse* psNPCDialog::ErrorResponse(const psString &text, const char* trigger)
{
    AddBadText(text,trigger);

    psString error("error");

    if(text.Find("?") != (size_t)-1)
    {
        error = "unknown";
    }

    NpcResponse* resp = FindResponse(error,text);

    if(!resp && error.Compare("unknown"))
    {
        error = "error";
        resp = FindResponse(error,text);
    }

    // save the trigger that didn't work for possible display to devs
    if(resp)
        resp->triggerText = trigger;

    return resp;
}

void psNPCDialog::UpdateAntecedents(NpcResponse* resp)
{
    // later this will need to be kept on a per player basis
    // but for now, its just one set

    if(resp->her.Length())
        antecedent_her = resp->her;

    if(resp->him.Length())
        antecedent_him = resp->him;

    if(resp->it.Length())
        antecedent_it = resp->it;

    if(resp->them.Length())
        antecedent_them = resp->them;

}

bool psNPCDialog::AddWord(const char*)
{
    return false;
}

bool psNPCDialog::AddSynonym(const char*,const char*)
{
    return false;
}

bool psNPCDialog::AddKnowledgeArea(const char* ka_name)
{
    KnowledgeArea* newKA = new KnowledgeArea;
    newKA->area = ka_name;
    newKA->priority = 0; // priority only matters when sorting from the db table.  here it will always be last priority
    knowareas.Push(newKA);
    return true;
}

bool psNPCDialog::AddResponse(const char* area,const char* words,const char* response,const char* minfaction)
{
    (void) area;
    (void) words;
    (void) response;
    (void) minfaction;
    return false;
}

bool psNPCDialog::AssignNPCArea(const char* npcname,const char* areaname)
{
    (void) npcname;
    (void) areaname;
    return false;
}


// void psNPCDialog::AddNewTrigger( int databaseID )
// {
//     if ( !dict )
//         return;
//     else
//         dict->AddTrigger( db, databaseID );
// }

// void psNPCDialog::AddNewResponse ( int databaseID )
// {
//     if ( !dict )
//         return;
//     else
//         dict->AddResponse( db, databaseID );
// }



void DialogHistory::AddToHistory(int playerID, int responseID, csTicks when)
{
    DialogHistoryEntry entry;
    entry.playerID  = playerID;
    entry.responseID= responseID;
    entry.when      = when;

    if(history.GetSize() < MAX_HISTORY_LEN)
        history.Push(entry);
    else
    {
        history.Put(counter,entry);
        counter++;
        if(counter == MAX_HISTORY_LEN)
            counter=0;
    }
}

bool DialogHistory::EverSaid(int playerID, int responseID, csTicks &howLongAgo, int &times)
{
    times      = 0;
    howLongAgo = 0;

    for(size_t i=0; i<history.GetSize(); i++)
    {
        if(history[i].playerID   == playerID &&
                history[i].responseID == responseID)
        {
            times++;
            if(history[i].when > howLongAgo)
                howLongAgo = history[i].when;
        }
    }
    if(howLongAgo)
        howLongAgo = csGetTicks() - howLongAgo;  // need the delta here

    return (times > 0);
}


//--------------------------------------------------------------------------

void psDialogManager::AddTrigger(psTriggerBlock* trigger)
{
    triggers.Push(trigger);
}


void psDialogManager::AddSpecialResponse(psSpecialResponse* response)
{
    special.Push(response);
}


int psDialogManager::GetPriorID(int internalID)
{
    for(size_t x = 0; x < triggers.GetSize(); x++)
    {
        for(size_t z = 0; z < triggers[x]->attitudes.GetSize(); z++)
        {
            psAttitudeBlock* currAttitude = triggers[x]->attitudes[z];

            if(currAttitude->responseSet.givenID == internalID)
                return currAttitude->responseSet.databaseID;
        }
    }

    return -1;
}


void psDialogManager::PrintInfo()
{
    const size_t totalTrigs = triggers.GetSize();

    for(uint z = 0; z < totalTrigs; z++)
    {
        CPrintf(CON_CMDOUTPUT,"*******************\n");
        CPrintf(CON_CMDOUTPUT,"Trigger Complete  :\n");
        CPrintf(CON_CMDOUTPUT,"*******************\n");

        psTriggerBlock* currTrigger = triggers[z];

        printf("Prior responseID: %d\n", currTrigger->priorResponseID);
        for(size_t x = 0; x < currTrigger->phraseList.GetSize(); x++)
        {
            CPrintf(CON_CMDOUTPUT,"Phrase: %s\n", currTrigger->phraseList[x]);
        }

        const size_t totalAttitudes = currTrigger->attitudes.GetSize();
        for(uint x = 0; x < totalAttitudes; x++)
        {
            size_t total =
                currTrigger->attitudes[x]->responseSet.responses.GetSize();
            for(uint res = 0; res < total; res++)
            {
                CPrintf(CON_CMDOUTPUT,"Response: %s\n",
                        currTrigger->attitudes[x]->responseSet.responses[res]);
            }
        }
    }

}


int psDialogManager::InsertResponse(csString &response)
{
    csString escape;
    db->Escape(escape, response);
    db->Command("INSERT INTO npc_responses (response1,response2,response3,response4,response5,"
                "pronoun_him, pronoun_her, pronoun_it, pronoun_them, script, quest_id) "
                "VALUES (\"%s\",'','','','','','','','','',0)",
                escape.GetData());


    int id = db->SelectSingleNumber("SELECT last_insert_id()");
    return id;
}

int psDialogManager::InsertResponseSet(psResponse &response)
{
    csStringArray &responseSet = response.responses;
    csString script = response.script;
    //Notify1("Inserting the response Set");

    size_t total = responseSet.GetSize();
    csString buffer;

    csString command("INSERT INTO npc_responses (");
    csString values;

    if(total > 0)
    {
        buffer.Format(" response%d", 1);
        command.Append(buffer);
        buffer.Format(" \"%s\" ", responseSet[0]);
        values.Append(buffer);
    }

    for(uint x = 1; x < 5;  x++)
    {
        buffer.Format(" , response%d ", x+1);
        command.Append(buffer);
        if(x<total)
        {
            buffer.Format(" ,\"%s\" ", responseSet[x]);
            values.Append(buffer);
        }
        else
            values.Append(" ,\"\" ");

    }

    command.Append(", pronoun_him, pronoun_her, pronoun_it, pronoun_them,script");
    if(response.questID == -1)
        command.Append(")");
    else
        command.Append(", quest_id)");

    csString escape;
    db->Escape(escape, script.GetDataSafe());
    buffer.Format(" VALUES ( %s, '%s','%s','%s','%s', '%s' ",
                  (const char*)values,
                  response.pronoun_him.GetDataSafe(),
                  response.pronoun_her.GetDataSafe(),
                  response.pronoun_it.GetDataSafe(),
                  response.pronoun_them.GetDataSafe(),
                  escape.GetData());
    if(response.questID == -1)
        buffer.Append(")");
    else
    {
        buffer.Append(", ");
        buffer.Append(response.questID);
        buffer.Append(")");
    }
    command += csString(buffer);



    //Notify2("Final Command: %s\n", command.GetData());

    if(!db->Command(command.GetData()))
    {
        Error2("MYSQL ERROR: %s", db->GetLastError());
        Error2("LAST QUERY: %s", db->GetLastQuery());
        CPrintf(CON_ERROR,"%s\n",command.GetData());
    }

    int id = db->SelectSingleNumber("SELECT last_insert_id()");

    return id;
}


int psDialogManager::InsertTrigger(const char* trigger, const char* area,
                                   int maxAttitude, int minAttitude,
                                   int responseID, int priorID, int questID)
{
    csString command;

    csString lower(trigger);
    lower.Downcase();

    csString escLower;
    csString escArea;
    db->Escape(escLower, lower);
    db->Escape(escArea, area);

    if(questID == -1)
        db->Command("INSERT INTO npc_triggers ( trigger, "
                    " response_id, "
                    " prior_response_required, "
                    " min_attitude_required, "
                    " max_attitude_required, "
                    " area )"
                    " VALUES ( '%s', "
                    " %d , "
                    " %d , "
                    " %d , "
                    " %d , "
                    " '%s' ) ",
                    escLower.GetData() ,
                    responseID,
                    priorID,
                    minAttitude,
                    maxAttitude,
                    escArea.GetData());
    else
        db->Command("INSERT INTO npc_triggers ( trigger, "
                    " response_id, "
                    " prior_response_required, "
                    " min_attitude_required, "
                    " max_attitude_required, "
                    " area, quest_id )"
                    " VALUES ( '%s', "
                    " %d , "
                    " %d , "
                    " %d , "
                    " %d , "
                    " '%s', %i) ",
                    escLower.GetData(),
                    responseID,
                    priorID,
                    minAttitude,
                    maxAttitude,
                    escArea.GetData(), questID);

    int id = db->SelectSingleNumber("SELECT last_insert_id()");

    return id;
}


bool psDialogManager::WriteToDatabase()
{
    // Add in all the responses
    const size_t totalTriggers = triggers.GetSize();

    for(uint x = 0; x < totalTriggers; x++)
    {
        psTriggerBlock* currTrig = triggers[x];

        // Insert all the responses for this trigger. Each
        // attitude block represents a different response set.
        for(size_t att = 0; att < currTrig->attitudes.GetSize(); att++)
        {
            psAttitudeBlock* currAttitude = currTrig->attitudes[att];

            int id = InsertResponseSet(currAttitude->responseSet);

            // Store the database ID for later use by triggers.
            currAttitude->responseSet.databaseID = id;

            responseIDs.Push(id);
        }
    }


    // Add all the triggers
    for(uint x = 0; x < totalTriggers; x++)
    {
        psTriggerBlock* currTrig = triggers[x];

        // Figure out the correct prior id to use for the database
        int goodPriorResponseID = 0;
        if(currTrig->priorResponseID != 0)
        {
            goodPriorResponseID = GetPriorID(currTrig->priorResponseID);
        }

        CS_ASSERT(goodPriorResponseID != -1);

        for(size_t phi = 0; phi < currTrig->phraseList.GetSize(); phi++)
        {
            csString phrase(currTrig->phraseList[phi]);

            for(size_t att = 0; att < currTrig->attitudes.GetSize(); att++)
            {
                int attMax = currTrig->attitudes[att]->maxAttitude;
                int attMin = currTrig->attitudes[att]->minAttitude;
                int respID = currTrig->attitudes[att]->responseSet.databaseID;

                csString trigArea = currTrig->area;
                if(trigArea.IsEmpty())
                    trigArea = area;

                int databaseID = InsertTrigger(phrase, trigArea.GetDataSafe(),
                                               attMax, attMin,
                                               respID, goodPriorResponseID, currTrig->questID);

                triggerIDs.Push(databaseID);
            }
        }
    }

    // Add the special responses
    size_t totalSpecial = special.GetSize();
    for(uint sp = 0; sp < totalSpecial; sp++)
    {
        psSpecialResponse* spResp = special[sp];
        int responseID = InsertResponse(spResp->response);
        int triggerID = InsertTrigger(spResp->type,
                                      area,
                                      spResp->attMax,
                                      spResp->attMin,
                                      responseID,
                                      0, spResp->questID);

        triggerIDs.Push(triggerID);
        responseIDs.Push(responseID);
    }

    return false;
}

//--------------------------------------------------------------------------

void psTriggerBlock::Create(csRef<iDocumentNode> triggerNode, int questID)
{
    area = triggerNode->GetAttributeValue("area");
    this->questID = questID;
    //=================
    // Read in phrases
    //=================
    csRef<iDocumentNodeIterator> iter = triggerNode->GetNodes("phrase");
    while(iter->HasNext())
    {
        csRef<iDocumentNode> phrasenode = iter->Next();
        phraseList.Push(phrasenode->GetAttributeValue("value"));
    }


    //========================
    // Read in attitude blocks
    //========================
    iter = triggerNode->GetNodes("attitude");
    while(iter->HasNext())
    {
        csRef<iDocumentNode> attitudenode = iter->Next();
        psAttitudeBlock* attitudeBlock = new psAttitudeBlock(manager);

        attitudeBlock->Create(attitudenode, questID);

        attitudes.Push(attitudeBlock);
    }
}

//--------------------------------------------------------------------------

int psAttitudeBlock::responseID = 0;

void psAttitudeBlock::Create(csRef<iDocumentNode> attitudeNode, int questID)
{
    //=========================
    // Read the attitude values
    //=========================
    csRef<iDocumentAttribute> maxattr = attitudeNode->GetAttribute("max");
    csRef<iDocumentAttribute> minattr = attitudeNode->GetAttribute("min");

    if(!minattr || !maxattr)
    {
        maxAttitude = 100;
        minAttitude = -100;
    }
    else
    {
        maxAttitude = maxattr->GetValueAsInt();
        minAttitude = minattr->GetValueAsInt();
    }

    //==========================
    // Read in the response tags
    //==========================
    csRef<iDocumentNodeIterator> iter = attitudeNode->GetNodes("response");
    while(iter->HasNext())
    {
        csRef<iDocumentNode> responseNode = iter->Next();
        responseSet.responses.Push(responseNode->GetAttributeValue("say"));
    }

    iter = attitudeNode->GetNodes("script");

    responseSet.script="";
    while(iter->HasNext())
    {
        csRef<iDocumentNode> scriptNode = iter->Next();
        csString script = scriptNode->GetContentsValue();

        // remove \n and \r chars from the string
        for(unsigned int i=0; i < script.Length(); i++)
            if(script.GetAt(i)!='\r'&&script.GetAt(i)!='\n')
                responseSet.script+=script.GetAt(i);
    }

    csRef<iDocumentNode> pronounNode = attitudeNode->GetNode("pronoun");

    if(pronounNode)
    {
        responseSet.pronoun_him = pronounNode->GetAttributeValue("him");
        responseSet.pronoun_her = pronounNode->GetAttributeValue("her");
        responseSet.pronoun_it = pronounNode->GetAttributeValue("it");
        responseSet.pronoun_them = pronounNode->GetAttributeValue("them");
    }

    responseSet.questID = questID;


    //=======================================================
    // Read in the sub triggers that could be in the attitude
    //=======================================================
    iter = attitudeNode->GetNodes("trigger");

    while(iter->HasNext())
    {
        csRef<iDocumentNode> triggerNode = iter->Next();
        psTriggerBlock* trigger = new psTriggerBlock(manager);

        trigger->Create(triggerNode, questID);

        // The prior id for a sub trigger is the one for THIS attitude.
        trigger->SetPrior(responseSet.givenID);
        manager->AddTrigger(trigger);
    }
}

//--------------------------------------------------------------------------

psSpecialResponse::psSpecialResponse(csRef<iDocumentNode> responseNode, int questID)
{
    attMax = responseNode->GetAttributeValueAsInt("reactionMax");
    attMin = responseNode->GetAttributeValueAsInt("reactionMin");

    type = responseNode->GetAttributeValue("type");
    response = responseNode->GetAttributeValue("value");
    this->questID = questID;
}
