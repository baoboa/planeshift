/*
* dictionary.cpp
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"

#include "util/strutil.h"
#include "util/log.h"
#include "util/psdatabase.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/psxmlparser.h"

#include "../playergroup.h"
#include "../client.h"
#include "../gem.h"
#include "../globals.h"
#include "../psserver.h"
#include "../cachemanager.h"
#include "../questmanager.h"
#include "../entitymanager.h"
#include "../progressionmanager.h"
#include "../cachemanager.h"
#include "../questionmanager.h"
#include "../npcmanager.h"
#include "../adminmanager.h"
#include "../netmanager.h"
#include "../chatmanager.h"
#include "../hiremanager.h"
#include "psraceinfo.h"

#include <idal.h>
extern "C" {
#include "../tools/wordnet/wn.h"
}
#include "rpgrules/factions.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "dictionary.h"
#include "psnpcdialog.h"
#include "psquestprereqops.h"
#include "pscharacter.h"
#include "pscharacterloader.h"
#include "psitem.h"
#include "psitemstats.h"
#include "psguildinfo.h"
#include "adminmanager.h"
#include "scripting.h"

// Global variable exposed from globals.h
csRef<NPCDialogDict> dict;

NPCDialogDict::NPCDialogDict()
{
    dynamic_id = 1000000;
}

NPCDialogDict::~NPCDialogDict()
{
    csHash<NpcTerm*, csString>::GlobalIterator phraseIter(phrases.GetIterator());
    while(phraseIter.HasNext())
        delete phraseIter.Next();
    csHash<NpcTriggerGroupEntry*, csString>::GlobalIterator triggergroupIter(trigger_groups.GetIterator());
    while(triggergroupIter.HasNext())
        delete triggergroupIter.Next();
    NpcTriggerTree::Iterator triggerIter(triggers.GetIterator());
    while(triggerIter.HasNext())
        delete triggerIter.Next();
    csHash<NpcResponse*>::GlobalIterator responsesIter(responses.GetIterator());
    while(responsesIter.HasNext())
        delete responsesIter.Next();
    csHash<NpcDialogMenu*, csString>::GlobalIterator menuIter(initial_popup_menus.GetIterator());
    while(menuIter.HasNext())
        delete menuIter.Next();
    wnclose();
    dict = NULL;
}

bool NPCDialogDict::Initialize(iDataConnection* db)
{
    // Initialise WordNet
    if(wninit() != 0)
    {
        Error1("*****************************\nWordNet failed to initialize.\n"
               "******************************");
    }

    if(LoadDisallowedWords(db))
    {
        if(LoadSynonyms(db))
        {
            if(LoadTriggerGroups(db))
            {
                if(LoadTriggers(db))
                {
                    if(LoadResponses(db))
                    {
                        return true;
                    }
                    else
                        Error1("*********************************\nFailed to load Responses\n****************************");
                }
                else
                    Error1("****************************\nFailed to load Triggers\n****************************");
            }
            else
                Error1("****************************\nFailed to load Trigger Groups\n************************");
        }
        else
            Error1("****************************\nFailed to load Synonyms\n****************************");
    }
    else
        Error1("****************************\nFailed to load Disallowed words\n****************************");
    return false;
}

bool NPCDialogDict::LoadDisallowedWords(iDataConnection* db)
{
    Result result(db->Select("select word from npc_disallowed_words"));

    if(!result.IsValid())
    {
        Error2("Cannot load disallowed words into dictionary from database.\n%s", db->GetLastError());
        return false;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        csString newword = result[i]["word"];
        disallowed_words.Put(newword,true);

    }

    return true;
}

NpcTerm* NPCDialogDict::AddTerm(const char* term)
{
    NpcTerm* npc_term = FindTerm(term);
    if(npc_term) return npc_term;

    NpcTerm* newphrase = new NpcTerm(term);
    phrases.Put(newphrase->term, newphrase);
    return newphrase;
}

bool NPCDialogDict::LoadSynonyms(iDataConnection* db)
{
    Result result(db->Select("select word,"
                             "       synonym_of"
                             "  from npc_synonyms"));

    if(!result.IsValid())
    {
        CPrintf(CON_ERROR, "Cannot load terms into dictionary from database.\n");
        CPrintf(CON_ERROR, db->GetLastError());
        return false;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        NpcTerm* term = AddTerm(result[i]["word"]);

        csString synonym_of(result[i]["synonym_of"]);

        if(synonym_of.Length())
        {
            term->synonym = AddTerm(synonym_of);
        }
    }

    return true;
}

void NPCDialogDict::AddWords(csString &trigger)
{
    int wordnum=1;
    csString word("temp");

    // if it's an exchange trigger just skip the following checks
    if(trigger.GetAt(0)=='<')
        return;

    while(word.Length())
    {
        size_t pos = 0;
        word = GetWordNumber(trigger,wordnum++,&pos);
        word.Downcase();
        if(word.Length()==0)
            continue;

        // Check this word in the trigger for being disallowed.  If so, skip it.
        bool disallowed = disallowed_words.Get(word, false);
        if(disallowed)
        {
            // Comment out warning here because disallowed words are actually skipped
            // to make them *allowed* for Settings people to use to make their writing
            // more natural while avoiding recording them in the actual triggers.
            //CPrintf(CON_DEBUG,"Skipping disallowed word '%s' in trigger '%s' at position %d.\n",
            //      word.GetData(), trigger.GetData(), pos);
            trigger.DeleteAt(pos, word.Length());
            if(strstr(trigger.GetData(),"  "))
                trigger.DeleteAt(strstr(trigger.GetData(),"  ")-trigger.GetData(),1);
            trigger.RTrim();
            trigger.LTrim();
            wordnum--;  // Back up a word since we took one out.
            continue;
        }

        NpcTerm* found;

        found = phrases.Get(word, NULL);
        if(found)
        {
            if(found->synonym)
            {
                CPrintf(CON_WARNING, "Warning: Word %s in trigger '%s' is already a synonym for '%s'.\n",
                        (const char*)found->term,
                        (const char*)trigger,
                        (const char*)found->synonym->term);
            }
        }
        else
        {
            // add word
            found = new NpcTerm(word);
            phrases.Put(found->term, found);
        }
    }
}

int NPCDialogDict::AddTriggerGroupEntry(int id,const char* txt, int equivID)
{
    static int NextID=9000000;

    NpcTriggerGroupEntry* parent=NULL;

    if(id==-1)
        id = NextID++;

    if(equivID)
    {
        parent = trigger_groups_by_id.Get(equivID,NULL);
        if(!parent)
        {
            Error4("Trigger group entry id %d (%s) specified bad parent id of %d.  Skipped.",id,txt,equivID);
            return -1;
        }
    }

    NpcTriggerGroupEntry* newtge = new NpcTriggerGroupEntry(id,txt,parent);

    trigger_groups.Put(newtge->text, newtge);
    trigger_groups_by_id.Put(newtge->id,newtge);

    AddWords(newtge->text); // Make sure these trigger words are in known word list.
    Debug2(LOG_STARTUP,0,"Loaded trigger group entry <%s>",newtge->text.GetData());
    return id;
}

bool NPCDialogDict::LoadTriggerGroups(iDataConnection* db)
{
    Debug1(LOG_STARTUP,0,"Loading Trigger Groups...");

    // First add all the root entries so we find the parents later.
    Result result(db->Select("select id,"
                             "       trigger_text"
                             "  from npc_trigger_groups"
                             " where equivalent_to_id=0"));

    if(!result.IsValid())
    {
        CPrintf(CON_ERROR, "Cannot load trigger groups into dictionary from database.\n");
        CPrintf(CON_ERROR, db->GetLastError());
        return false;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        AddTriggerGroupEntry(result[i].GetInt("id"),result[i]["trigger_text"],0);
    }

    // Now add all the child entries and attach to parents.
    Result result2(db->Select("select id,"
                              "       trigger_text,"
                              "       equivalent_to_id"
                              "  from npc_trigger_groups"
                              " where equivalent_to_id<>0"));

    if(!result2.IsValid())
    {
        CPrintf(CON_ERROR, "Cannot load trigger groups into dictionary from database.\n");
        CPrintf(CON_ERROR, db->GetLastError());
        return false;
    }

    for(unsigned int i=0; i<result2.Count(); i++)
    {
        AddTriggerGroupEntry(result2[i].GetInt("id"),result2[i]["trigger_text"],result2[i].GetInt("equivalent_to_id"));
    }
    return true;
}

bool NPCDialogDict::FindKnowledgeArea(const csString &name)
{
    static csString fallback("not found");

    csString key(name);
    key.Downcase(); // all KAs are supposed to be lowercase
    csString temp = knowledgeAreas.Get(key,fallback);
    return temp != fallback;
}

bool NPCDialogDict::LoadTriggers(iDataConnection* db)
{
    Debug1(LOG_STARTUP,0,"Loading Triggers...");

    Result result(db->Select("select * from npc_triggers"));

    if(!result.IsValid())
    {
        CPrintf(CON_ERROR, "Cannot load triggers into dictionary from database.\n");
        CPrintf(CON_ERROR, db->GetLastError());
        return false;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        NpcTrigger* newtrig = new NpcTrigger;

        if(!newtrig->Load(result[i]))
        {
            Error2("Could not load trigger %s!",result[i]["id"]);
            delete newtrig;
            continue;
        }

        csArray<NpcTrigger*> newTriggers = ParseMultiTrigger(newtrig);

        // As the trigger has been parsed we can delete the new trigger
        delete newtrig;

        for(unsigned int y = 0; y < newTriggers.GetSize(); y++)
        {
            newtrig = newTriggers[y];

            if(newtrig->trigger.Length() == 0)
            {
                Error3("Found bad trigger %d of trigger length 0 in triggers, area %s", newtrig->id, newtrig->area.GetDataSafe());
                delete newtrig;
                continue;
            }

            AddWords(newtrig->trigger); // Make sure these trigger words are in known word list.

            if(!FindKnowledgeArea(newtrig->area))
            {
                Debug2(LOG_QUESTS, 0, "--------Adding KA: %s",newtrig->area.GetDataSafe());
                knowledgeAreas.PutUnique(newtrig->area,newtrig->area);
            }

            // check if there is an equal triggers, in that case just trash this new one.
            if(triggers.Find(newtrig, NULL))
            {
                delete newtrig;
            }
            else
            {
                triggers.Insert(newtrig);
            }
        }
    }
    return true;
}

bool NPCDialogDict::LoadResponses(iDataConnection* db)
{
    Result result(db->Select("SELECT * FROM npc_responses"));

    if(!result.IsValid())
    {
        CPrintf(CON_ERROR, "Cannot load responses into dictionary from database.\n");
        CPrintf(CON_ERROR, db->GetLastError());
        return false;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        NpcResponse* newresp = new NpcResponse;

        if(!newresp->Load(result[i]))
        {
            delete newresp;
            return false;
        }

        int trigger_id = result[i].GetInt("trigger_id");
        if(trigger_id == 0)
        {
            Error1("Response with null trigger.");
            delete newresp;
            return false;
        }
        if(!AddTrigger(db,trigger_id,newresp->id))
        {
            Error2("Failed to load trigger for resp: %d",newresp->id);
            delete newresp;
            return false;
        }



        responses.Put(newresp->id, newresp);
    }
    return true;
}


NpcTerm* NPCDialogDict::FindTerm(const char* term)
{
    csString key = term;
    key.Downcase();
    return phrases.Get(key, NULL);
}

NpcTerm* NPCDialogDict::FindTermOrSynonym(const csString &term)
{
    NpcTerm* termRec = FindTerm(term);

    if(termRec && termRec->synonym)
        return termRec->synonym;
    else
        return termRec;
}

NpcResponse* NPCDialogDict::FindResponse(gemNPC* npc,
        const char* area,
        const char* trigger,
        int faction,
        int priorresponse,
        Client* client,
        int questID)
{
    Debug8(LOG_NPC, client ? client->GetClientNum() : 0,"Entering NPCDialogDict::FindResponse(%s,%s,%s,%d,%d,%s,%d)",
           npc->GetName(),area,trigger,faction,priorresponse,client->GetName(), questID);
    NpcTrigger* trig;
    NpcTrigger key;

    key.area            = area;
    key.trigger         = trigger;
    key.priorresponseID = priorresponse;

    trig = triggers.Find(&key, NULL);

    if(trig)
    {
        Debug3(LOG_NPC, client ? client->GetClientNum() : 0,"NPCDialogDict::FindResponse consider trig(%d): '%s'",
               trig->id,trig->trigger.GetDataSafe());
    }
    else
    {
        Debug1(LOG_NPC, client ? client->GetClientNum() : 0,"NPCDialogDict::FindResponse no trigger found");
        return NULL;
    }


    csArray<int> availableResponseList;

    // Check if not all responses is blocked(Not available in quests, Prequests not fullfitted,...)
    if(!trig || !trig->HaveAvailableResponses(client,npc,this,&availableResponseList, questID))
    {
        Debug1(LOG_NPC, client ? client->GetClientNum() : 0,"NPCDialogDict::FindResponse no available responses found");
        return NULL;
    }

    NpcResponse* resp;

    resp = FindResponse(trig->GetRandomResponse(availableResponseList)); // find one of the specified responses

    return resp;
}

NpcResponse* NPCDialogDict::FindResponse(int responseID)
{
    return responses.Get(responseID, NULL);
}


bool NPCDialogDict::CheckForTriggerGroup(csString &trigger)
{
    NpcTriggerGroupEntry* found = trigger_groups.Get(trigger, NULL);

    if(found && found->parent)
    {
        // Trigger is child, so substitute parent
        trigger = found->parent->text;
        return true;
    }
    return false;
}

csArray<NpcTrigger*> NPCDialogDict::ParseMultiTrigger(NpcTrigger* parsetrig)
{
    csArray<NpcTrigger*> newTriggers;
    psString trig(parsetrig->trigger);
    csStringArray list;

    trig.Split(list,'.');

    // For each trigger create a trigger object rappresenting it.
    for(size_t i = 0; i < list.GetSize(); i++)
    {
        NpcTrigger* newtrig = new NpcTrigger;
        newtrig->id = parsetrig->id;
        newtrig->area = parsetrig->area;
        newtrig->priorresponseID = parsetrig->priorresponseID;
        newtrig->trigger = list[i];
        newTriggers.Push(newtrig);
    }
    return newTriggers;
}

bool NPCDialogDict::AddTrigger(iDataConnection* db, int triggerID , int responseID)
{
    Result result(db->Select("SELECT * from npc_triggers WHERE id=%d", triggerID));

    if(!result.IsValid() || result.Count()!=1)
    {
        Error2("Invalid trigger id %d in npc_triggers table.",triggerID);
        return false;
    }

    NpcTrigger* newtrig = new NpcTrigger;

    if(!newtrig->Load(result[0]))
    {
        delete newtrig;
        return false;
    }

    csArray<NpcTrigger*> newTriggers = ParseMultiTrigger(newtrig);

    // As the trigger has been parsed we can delete the new trigger
    delete newtrig;

    CS_ASSERT(responseID != -1);

    for(size_t i = 0; i < newTriggers.GetSize(); i++)
    {
        newtrig = newTriggers[i];
        newtrig->responseIDlist.Push(responseID);

        NpcTrigger* trig;

        AddWords(newtrig->trigger);

        trig = triggers.Find(newtrig, NULL);
        if(trig)
        {
            // There are already a trigger with this combination of
            // triggertext, KA, and prior respose so pushing the trigger
            // response on the same trigger.
            CS_ASSERT(responseID != -1);
            trig->responseIDlist.Push(responseID);

            delete newtrig;
        }
        else
        {
            triggers.Insert(newtrig);
        }
    }

    return true;
}


void NPCDialogDict::AddResponse(iDataConnection* db, int databaseID)
{
    Result result(db->Select("SELECT * from npc_responses WHERE id=%d",databaseID));

    if(!result.IsValid() || result.Count() != 1)
    {
        Error2("Invalid response id %d specified for npc_responses table.",databaseID);
        return;
    }

    NpcResponse* newresp = new NpcResponse;

    if(!newresp->Load(result[0]))
    {
        delete newresp;
        return;

    }
    responses.Put(newresp->id, newresp);
}

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

NpcResponse* NPCDialogDict::AddResponse(const char* response_text,
                                        const char* pronoun_him,
                                        const char* pronoun_her,
                                        const char* pronoun_it,
                                        const char* pronoun_them,
                                        const char* npc_name,
                                        int &new_id,
                                        psQuest* quest,
                                        const char* audio_path)
{
    NpcResponse* newresp = new NpcResponse;

    // Need to chop the KA name up so that /narrate only has to match first name
    csString npc_first_name(npc_name);
    size_t space = npc_first_name.FindFirst(' ');
    if(space != SIZET_NOT_FOUND)
    {
        npc_first_name = npc_first_name.Slice(0, space);
    }

    if(!new_id)
    {
        new_id = dynamic_id++;
    }

    newresp->id = new_id;

    csString opStr;

    size_t start=0,end=0;
    opStr = "<response>";
    csString resp(response_text);

    // printf("\nGot: %s\n",response_text);

    while(end != resp.Length())
    {
        while(start < resp.Length() && (resp.GetAt(start) == ']' || resp.GetAt(start) == ' '))
        {
            start++;
        }
        if(start == resp.Length())
            break;

        end = resp.FindFirst("[.!?",start);  // action delimiter or sentence delimiter whichever is first
        while(end != SIZET_NOT_FOUND && end+1 < resp.Length() && resp.GetAt(end) != '[' && resp.GetAt(end+1) != ' ')
        {
            end = resp.FindFirst("[.!?",end+1); //checks if we have a space after (to avoid problems with !!! ... ???)
        }
        if(end == SIZET_NOT_FOUND)
        {
            end = resp.Length();
        }
        if(end < resp.Length() && (resp.GetAt(end)=='.' || resp.GetAt(end) == '!' || resp.GetAt(end) == '?'))  // include the period in this substring
        {
            end++;
        }

        if(end-start > 0)
        {
            csString saySegment;
            resp.SubString(saySegment,start,end-start); // pull out the part before the [ ]
            // printf("Say: %s\n",(const char *)saySegment);
            opStr.AppendFmt("<say text=\"%s\"/>", EscpXML(saySegment.GetDataSafe()).GetDataSafe());
        }
        if(end == resp.Length())   // stop if at end of string already
            break;

        if(end > 0 && (resp.GetAt(end-1)=='.' || resp.GetAt(end-1)=='!' || resp.GetAt(end-1)=='?') && resp.GetAt(end) != '[')  // skip action brackets if this was a sentence break
        {
            start = end;
            continue;
        }

        // Now get the part in brackets
        start = end;
        end = resp.Find("]", start); // find end of action
        if(end == SIZET_NOT_FOUND)
        {
            Error2("Unmatched open '[' in npc response %s.", response_text);
            delete newresp;
            return NULL;
        }

        if(end - start > 1)
        {
            csString actionSegment;
            resp.SubString(actionSegment,start+1,end-start-1);
            // If action does not start with npc's name, it is a 3rd person statement, not /me
            if(strncasecmp(actionSegment,npc_first_name,npc_first_name.Length()))
            {
                opStr.AppendFmt("<narrate text=\"%s\"/>", EscpXML(actionSegment.GetDataSafe()).GetDataSafe());
            }
            else // now look for /me or /my because the npc name matches
            {
                size_t spc = actionSegment.FindFirst(" ");
                // apostrophe after name means /my
                if(npc_first_name.Length() < actionSegment.Length() &&
                        actionSegment[npc_first_name.Length()] == '\'')
                {
                    actionSegment.DeleteAt(0,spc+1);
                    opStr.AppendFmt("<actionmy text=\"%s\"/>", EscpXML(actionSegment.GetDataSafe()).GetDataSafe());
                }
                else // this is a /me command
                {
                    actionSegment.DeleteAt(0,spc+1);
                    opStr.AppendFmt("<action text=\"%s\"/>", EscpXML(actionSegment.GetDataSafe()).GetDataSafe());
                }
            }
        }
        start = end;
    }

    newresp->him  = pronoun_him;
    newresp->her  = pronoun_her;
    newresp->it   = pronoun_it;
    newresp->them = pronoun_them;
    newresp->voiceAudioPath[0] = audio_path;

    newresp->type = NpcResponse::VALID_RESPONSE;

    newresp->quest = quest;
    if(quest && quest->GetPrerequisite())
    {
        newresp->prerequisite = quest->GetPrerequisite()->Copy();
    }
    else
    {
        newresp->prerequisite = NULL;
    }

    if(opStr.Length() > 8)
    {
        // printf("Got resulting opStr of: %s\n", opStr.GetDataSafe() );
        opStr.Append("</response>");
    }
    else
    {
        opStr.Clear();
    }
    newresp->ParseResponseScript(opStr.GetDataSafe());

    responses.Put(newresp->id, newresp);


    return newresp;  // Make response available for script additions
}

NpcResponse* NPCDialogDict::AddResponse(const char* script)
{
    NpcResponse* newresp = new NpcResponse;
    newresp->id = dynamic_id++;
    newresp->type = NpcResponse::VALID_RESPONSE;
    newresp->ParseResponseScript(script);

    responses.Put(newresp->id, newresp);

    return newresp;
}

void NPCDialogDict::DeleteTriggerResponse(NpcTrigger* trigger, int responseId)
{
    csHash<NpcResponse*>::Iterator iter(responses.GetIterator(responseId));
    while(iter.HasNext())
        delete iter.Next();
    responses.DeleteAll(responseId);

    if(trigger)
    {
        trigger->responseIDlist.Delete(responseId);
        if(trigger->responseIDlist.GetSize() == 0)
        {
            triggers.Delete(trigger);
            delete trigger;
        }
    }
}

NpcTrigger* NPCDialogDict::AddTrigger(const char* k_area,const char* mytrigger,int prior_response, int trigger_response)
{
    NpcTrigger* trig;
    NpcTrigger key;
    // Both 0 and -1 can be used for no precodition, make sure we use -1
    if(prior_response == 0) prior_response = -1;

    // If the trigger to be added is already present, then the trigger response
    // is just added as an alternative to the existing responses specified for
    // this trigger.  These are chosen from randomly when triggering later.
    csString temp_k_area(k_area);
    key.area            = temp_k_area.Downcase();
    key.trigger         = mytrigger;
    key.priorresponseID = prior_response;

    trig = triggers.Find(&key, NULL);

    if(trig)
    {
        Debug2(LOG_QUESTS,0,"Found existing trigger so adding response %d as an alternative response to it.\n",trigger_response);
        CS_ASSERT(trigger_response != -1);
        trig->responseIDlist.Push(trigger_response);
        return trig;
    }
    else
    {
        NpcTrigger* newtrig = new NpcTrigger;

        newtrig->id              = 0;
        newtrig->area            = temp_k_area;
        newtrig->trigger         = mytrigger;
        newtrig->priorresponseID = prior_response;
        CS_ASSERT(trigger_response != -1);
        newtrig->responseIDlist.Push(trigger_response);

        AddWords(newtrig->trigger);

        NpcTrigger* oldtrig;
        if((oldtrig = triggers.Find(newtrig, NULL)))
        {
            // There are already a trigger with this combination of
            // triggertext, KA, and prior respose so pushing the trigger
            // response on the same trigger.
            int respID = newtrig->responseIDlist.Top();
            CS_ASSERT(respID != -1);
            oldtrig->responseIDlist.Push(respID);

            delete newtrig;
            return oldtrig;
        }
        triggers.Insert(newtrig);

        if(!FindKnowledgeArea(newtrig->area))
        {
            Debug2(LOG_QUESTS, 0,"--------Adding KA: %s",newtrig->area.GetDataSafe());
            knowledgeAreas.PutUnique(newtrig->area,newtrig->area);
        }


        return newtrig;
    }
}

NpcDialogMenu* NPCDialogDict::FindMenu(const char* name)
{
    return initial_popup_menus.Get(csString(name),0);
}

void NPCDialogDict::AddMenu(const char* name, NpcDialogMenu* menu)
{
    NpcDialogMenu* found = FindMenu(name);
    if(found)   // merge with existing
    {
        found->Add(menu);
        delete menu;  // delete here if we don't keep it above
    }
    else // add a new menu
    {
        initial_popup_menus.PutUnique(csString(name),menu);
    }
}


void PrintTrigger(NpcTrigger* trig)
{
    CPrintf(CON_CMDOUTPUT ,"Trigger [%d] %s : \"%-60.60s\" %8d /",
            trig->id,trig->area.GetData(),trig->trigger.GetDataSafe(),trig->priorresponseID);
}

void PrintResponse(NpcResponse* resp)
{
    csString script = resp->GetResponseScript();
    CPrintf(CON_CMDOUTPUT ,"Response [%8d] Script : %s\n",resp->id,script.GetData());
    if(resp->quest)
    {
        CPrintf(CON_CMDOUTPUT ,"                    Quest  : %s\n",resp->quest->GetName());
    }
    if(resp->prerequisite)
    {
        csString prereq = resp->prerequisite->GetScript();
        if(!prereq.IsEmpty())
        {
            CPrintf(CON_CMDOUTPUT ,"                    Prereq : %s\n",prereq.GetDataSafe());
        }
    }
    if(resp->him.Length() || resp->her.Length() || resp->it.Length() || resp->them.Length())
    {
        CPrintf(CON_CMDOUTPUT,"                    Pronoun: him='%s' her='%s' it='%s' them='%s'\n",
                resp->him.GetDataSafe(),resp->her.GetDataSafe(),
                resp->it.GetDataSafe(),resp->them.GetDataSafe());
    }
    for(int n = 0; n < MAX_RESP; n++)
    {
        if(resp->response[n].Length())
        {
            CPrintf(CON_CMDOUTPUT ,"%21d) %s\n",n+1,resp->response[n].GetData());
        }
    }
}

void NpcTerm::Print()
{
    CPrintf(CON_CMDOUTPUT ,"%-30s",term.GetDataSafe());
    if(synonym)
    {
        CPrintf(CON_CMDOUTPUT ," %-30s :",synonym->term.GetDataSafe());
    }
    else
    {
        CPrintf(CON_CMDOUTPUT ," %-30s :","");
    }

    if(hypernymSynNet == NULL)
        BuildHypernymList();

    if(hypernyms.GetSize())
    {
        for(size_t i=0; i < hypernyms.GetSize(); i++)
        {
            if(i!=0)
            {
                CPrintf(CON_CMDOUTPUT ,", ");
            }
            CPrintf(CON_CMDOUTPUT ,"%s",hypernyms[i]);
        }
    }

    CPrintf(CON_CMDOUTPUT ,"\n");
}



void NPCDialogDict::Print(const char* area)
{
    CPrintf(CON_CMDOUTPUT ,"\n");
    CPrintf(CON_CMDOUTPUT ,"NPC Dictionary\n");
    CPrintf(CON_CMDOUTPUT ,"\n");

    if(area!=NULL && strlen(area))
    {
        CPrintf(CON_CMDOUTPUT ,"----------- Triggers/Responses of area %s----------\n",area);
        NpcTriggerTree::Iterator trig_iter(triggers.GetIterator());
        NpcTrigger* trig;
        while(trig_iter.HasNext())
        {
            trig = trig_iter.Next();
            // filter on given area
            if(area!=NULL && strcasecmp(trig->area.GetDataSafe(),area)!=0)
                continue;

            PrintTrigger(trig);
            CPrintf(CON_CMDOUTPUT ,"\n");

            for(size_t i = 0; i < trig->responseIDlist.GetSize(); i++)
            {
                NpcResponse* resp = dict->FindResponse(trig->responseIDlist[i]);
                if(resp)
                {
                    PrintResponse(resp);
                }
                else
                    CPrintf(CON_CMDOUTPUT ,"Response [%d]: Error. Response not found!!!\n",trig->responseIDlist[i]);
            }
            CPrintf(CON_CMDOUTPUT ,"\n");
        }
        return;
    }

    CPrintf(CON_CMDOUTPUT ,"----------- All Triggers ----------\n");
    NpcTriggerTree::Iterator trig_iter(triggers.GetIterator());
    NpcTrigger* trig;
    while(trig_iter.HasNext())
    {
        trig = trig_iter.Next();
        PrintTrigger(trig);
        for(size_t i = 0; i < trig->responseIDlist.GetSize(); i++)
        {
            CPrintf(CON_CMDOUTPUT ," %d",trig->responseIDlist[i]);
        }
        CPrintf(CON_CMDOUTPUT ,"\n");
    }

    CPrintf(CON_CMDOUTPUT ,"----------- All Responses ---------\n");
    csHash<NpcResponse*>::GlobalIterator resp_iter(responses.GetIterator());
    NpcResponse* resp;
    while(resp_iter.HasNext())
    {
        resp = resp_iter.Next();
        PrintResponse(resp);
    }

    CPrintf(CON_CMDOUTPUT ,"\n");

    CPrintf(CON_CMDOUTPUT ,"----------- All Terms ---------\n");
    csHash<NpcTerm*,csString>::GlobalIterator term_iter(phrases.GetIterator());
    NpcTerm* term;
    while(term_iter.HasNext())
    {
        term = term_iter.Next();
        term->Print();
    }

    CPrintf(CON_CMDOUTPUT ,"\n");
}

bool NpcTerm::IsNoun()
{
    char* baseform = morphstr(const_cast<char*>(term.GetData()), NOUN);

    if(!baseform)
        baseform = const_cast<char*>(term.GetData());

    return in_wn(baseform, NOUN) != 0;
}

const char* NpcTerm::GetInterleavedHypernym(size_t which)
{
    if(hypernymSynNet == NULL)
        BuildHypernymList();

    if(which < hypernyms.GetSize())
        return hypernyms.Get(which);
    else
        return NULL;
}

void NpcTerm::BuildHypernymList()
{
    hypernymSynNet = findtheinfo_ds(const_cast<char*>(term.GetData()), NOUN, -HYPERPTR, ALLSENSES);

    // Hypernym 0 is the original word
    hypernyms.Put(0, term.GetData());

    bool hit;

    SynsetPtr sense[50];
    SynsetPtr current = sense[0] = hypernymSynNet;

    if(current == NULL)
        return;

    // We interleave hypernyms through the senses
    size_t sensecount = 0;
    while(current->nextss)
    {
        sensecount++;
        current = current->nextss;
        sense[sensecount] = current;
    }

    // Perform breadth-first search of hypernyms/word senses
    do
    {
        // Each iteration goes one level deeper
        hit = false;
        for(size_t j = 0; j <= sensecount; j++)
        {
            if(sense[j] == NULL)
                continue;

            sense[j] = sense[j]->ptrlist;

            // We have found a hypernym
            if(sense[j])
            {
                hit = true;
                // Check if we have this word already before we add it.
                bool found = false;
                for(size_t i = 0; i < hypernyms.GetSize() && !found; i++)
                {
                    if(strcmp(*sense[j]->words,hypernyms[i])==0)
                    {
                        found = true;
                    }
                }
                if(!found)
                {
                    hypernyms.Push(*sense[j]->words);
                }

                // Check if we have new senses.
                current = sense[j];
                while(current->nextss)
                {
                    sensecount++;
                    current = current->nextss;
                    sense[sensecount] = current;
                }

            }
        }
    }
    while(hit);
}

bool NpcTrigger::Load(iResultRow &row)
{
    id              = row.GetInt("id");
    csString temp_area(row["area"]);
    area            = temp_area.Downcase();
    trigger         = row["trigger_text"];
    priorresponseID = row.GetInt("prior_response_required");
    // Both 0 and -1 can be used for no precodition, make sure we use -1
    if(priorresponseID == 0) priorresponseID = -1;

    return true;
}

bool NpcTrigger::HaveAvailableResponses(Client* client, gemNPC* npc, NPCDialogDict* dict, csArray<int>* availableResponseList, int questID)
{
    bool haveAvail = false;

    for(size_t n = 0; n < responseIDlist.GetSize(); n++)
    {
        NpcResponse* resp = dict->FindResponse(responseIDlist[n]);
        if(resp)
        {
            if(client && (resp->quest || resp->prerequisite))
            {
                // Check if all prerequisites are true, and available(no lockout)
                if(((!resp->prerequisite || client->GetCharacterData()->CheckResponsePrerequisite(resp)) &&    //checks if prerequisites are in order
                        (!resp->quest || (resp->quest->Active() &&  // checks if the quest is active.
                                          client->GetCharacterData()->GetQuestMgr().CheckQuestAvailable(resp->quest,npc->GetPID()))))  //checks if the player can get the quest
                        /*overrides the above while mantaining quest consistency in case of questtester */
                        ||(client->GetCharacterData()->GetActor() && client->GetCharacterData()->GetActor()->questtester &&
                           (!resp->quest || !resp->quest->GetParentQuest())))
                {
                    // Check if the quest is the wanted one. It's checked here to avoid questtester to get in the middle.
                    if(!resp->quest || (questID == -1 || // Check if quest is there and that the client provided an hint.
                                        (resp->quest->GetParentQuest() && resp->quest->GetParentQuest()->GetID() == questID) || // Check if it's a subquest.
                                        resp->quest->GetID() == questID)) // Check as main quest.
                    {
                        Debug2(LOG_QUESTS,client->GetClientNum(),"Pushing quest response: %d",resp->id);
                        // This is a available response that is connected to a available quest
                        haveAvail = true;
                        if(availableResponseList)
                            availableResponseList->Push(resp->id);
                    }
                }
            }
            else
            {
                Debug2(LOG_QUESTS, client ? client->GetClientNum() : 0,"Pushing non quest response: %d",resp->id);
                // This is a available responses that isn't connected to a quest
                haveAvail = true;
                if(availableResponseList) availableResponseList->Push(resp->id);
            }
        }
    }

    return haveAvail;
}

int NpcTrigger::GetRandomResponse(const csArray<int> &availableResponseList)
{
    if(availableResponseList.GetSize() > 1)
        return availableResponseList[ psserver->rng->Get((uint32) availableResponseList.GetSize()) ];
    else
        return availableResponseList[0];
}

bool NpcTrigger::operator==(const NpcTrigger &other) const
{
    return (area==other.area &&
            trigger==other.trigger &&
            priorresponseID==other.priorresponseID);
}

bool NpcTrigger::operator<=(const NpcTrigger &other) const
{
    if(area > other.area)
        return false;
    if(area == other.area)
    {
        if(trigger > other.trigger)
            return false;
        if(trigger == other.trigger &&
                priorresponseID > other.priorresponseID)
            return false;
    }
    return true;
}


NpcResponse::NpcResponse()
{
    quest = NULL;
    menu = NULL;
    active_quest = -1;
}

NpcResponse::~NpcResponse()
{
    delete menu;
}

bool NpcResponse::Load(iResultRow &row)
{
    id             = row.GetInt("id");

    csString respName  = "response ";
    csString AudioName = "audio_path ";
    for(int i=0; i < MAX_RESP; i++)
    {
        respName[respName.Length()-1] = '1'+i;
        AudioName[AudioName.Length()-1] = '1'+i;
        response[i] = row[respName];
        voiceAudioPath[i] = row[AudioName];
        if(voiceAudioPath[i].Length() > 0)
            printf("Got audio file '%s' on response %d (%d).\n", voiceAudioPath[i].GetDataSafe(), id, i);
    }

    him  = row["pronoun_him"];
    her  = row["pronoun_her"];
    it   = row["pronoun_it"];
    them = row["pronoun_them"];

    type = NpcResponse::VALID_RESPONSE;

    // if a quest_id is specified in this response,
    // auto-generate a script op to make sure this
    // quest is active for the player before responding
    int quest_id = row.GetInt("quest_id");
    if(quest_id)
    {
        csString scripttext(row["script"]);
        if(scripttext.Find("<assign") == SIZET_NOT_FOUND)   // can't verify assigned if this step is what assigns
        {
            VerifyQuestAssignedResponseOp* op = new VerifyQuestAssignedResponseOp(quest_id);
            script.Push(op);
        }
    }

    // Parse prerequisite, we reuse the quest prerequiste as preprequisite for triggers as well
    csString prereq = row["prerequisite"];
    if(!ParsePrerequisiteScript(prereq,true))
    {
        Error3("Failed to decode response %d prerequisite: '%s'",id,prereq.GetDataSafe());

        return false;
    }

    return ParseResponseScript(row["script"]);
}

void NpcResponse::SetActiveQuest(int max)
{
    active_quest = psserver->rng->Get(max);
}

const char* NpcResponse::GetResponse(int &number)
{
    if(active_quest == -1)
    {
        int i=100;
        while(i--)
        {
            int which = psserver->rng->Get(MAX_RESP);

            if(i < MAX_RESP) which = i;  // Loop through on the last 5 attempts
            // just to be sure we find one.

            number = which;
            if(response[which].Length())
                return response[which];
        }
        return "5 blank responses!";
    }
    else
    {
        number = active_quest;
        return response[active_quest];
    }
}

bool NpcResponse::ParseResponseScript(const char* xmlstr,bool insertBeginning)
{
    if(!xmlstr || strcmp(xmlstr,"")==0)
    {
        SayResponseOp* op = new SayResponseOp("respond", false);
        if(insertBeginning)
            script.Insert(0,op);
        else
            script.Push(op);
        return true;
    }

    int where=0;
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(xmlstr);
    if(error)
    {
        Error3("Error: %s . In XML: %s", error, xmlstr);
        return false;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error2("No xml root in %s", xmlstr);
        return false;
    }

    csRef<iDocumentNode> topNode = root->GetNode("response");
    if(!topNode)
    {
        CPrintf(CON_WARNING,"The npc_response ID %d doesn't have a valid script: %s\n",id,xmlstr);
        return true; // Return true to add the response, but without a vaild script
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // Some Responses need post load functions.
        bool postLoadAssignQuest = false;


        ResponseOperation* op = NULL;

        if(strcmp(node->GetValue(), "respond") == 0 ||
                strcmp(node->GetValue(), "respondpublic") == 0 ||
                strcmp(node->GetValue(), "say") == 0)
        {
            op = new SayResponseOp(node->GetValue(),
                                   strcmp(node->GetValue(), "respondpublic") == 0);    // true for public, false for private
        }
        else if(strcmp(node->GetValue(), "action") == 0 ||
                strcmp(node->GetValue(), "actionmy") == 0 ||
                strcmp(node->GetValue(), "narrate") == 0 ||
                strcmp(node->GetValue(), "actionpublic") == 0 ||
                strcmp(node->GetValue(), "actionmypublic") == 0 ||
                strcmp(node->GetValue(), "narratepublic") == 0)
        {
            op = new ActionResponseOp(node->GetValue(),
                                      strncmp(node->GetValue(), "actionmy", 8) == 0 ,   // true for actionmy, false for action
                                      strncmp(node->GetValue(), "narrate", 7)  == 0,
                                      strcmp(node->GetValue(), "actionpublic") == 0 ||  //true in case it's public
                                      strcmp(node->GetValue(), "actionmypublic") == 0 ||
                                      strcmp(node->GetValue(), "narratepublic") == 0);
        }
        else if(strcmp(node->GetValue(), "npccmd") == 0)
        {
            op = new NPCCmdResponseOp;
        }
        else if(strcmp(node->GetValue(), "verifyquestcompleted") == 0)
        {
            op = new VerifyQuestCompletedResponseOp;
        }
        else if(strcmp(node->GetValue(), "verifyquestassigned") == 0)
        {
            op = new VerifyQuestAssignedResponseOp;
        }
        else if(strcmp(node->GetValue(), "verifyquestnotassigned") == 0)
        {
            op = new VerifyQuestNotAssignedResponseOp;
        }
        else if(strcmp(node->GetValue(), "assign") == 0)
        {
            op = new AssignQuestResponseOp;
            postLoadAssignQuest = true;
        }
        else if(strcmp(node->GetValue(), "complete") == 0)
        {
            op = new CompleteQuestResponseOp;
        }
        else if(strcmp(node->GetValue(), "uncomplete") == 0)
        {
            op = new UncompleteQuestResponseOp;
        }
        else if(strcmp(node->GetValue(), "give") == 0)
        {
            op = new GiveItemResponseOp;
        }
        else if(strcmp(node->GetValue(), "faction") == 0)
        {
            op = new FactionResponseOp;
        }
        else if(strcmp(node->GetValue(), "run") == 0)
        {
            op = new RunScriptResponseOp;
        }
        else if(strcmp(node->GetValue(), "setvariable") == 0)
        {
            op = new SetVariableResponseOp;
        }
        else if(strcmp(node->GetValue(), "train") == 0)
        {
            op = new TrainResponseOp;
        }
        else if(strcmp(node->GetValue(), "unsetvariable") == 0)
        {
            op = new UnSetVariableResponseOp;
        }
        else if(strcmp(node->GetValue(), "guild_award") == 0)
        {
            op = new GuildAwardResponseOp;
        }
        else if(strcmp(node->GetValue(), "hire") == 0)
        {
            op = new HireResponseOp;
        }
        else if(strcmp(node->GetValue(), "offer") == 0)
        {
            op = new OfferRewardResponseOp;
        }
        else if(strcmp(node->GetValue(), "money") == 0)
        {
            op = new MoneyResponseOp;
        }
        else if(strcmp(node->GetValue(), "introduce") == 0)
        {
            op = new IntroduceResponseOp;
        }
        else if(strcmp(node->GetValue(), "doadmincmd") == 0)
        {
            op = new DoAdminCommandResponseOp;
        }
        /*
        else if (strcmp(node->GetValue(), "fire_event") == 0)
        {
            op = new FireEventResponseOp;
        }
        */
        else
        {
            Error2("undefined operation specified in response script %d.",id);
            return false;
        }

        if(!op->Load(node))
        {
            Error3("Could not load <%s> operation in script %d. Error in XML",op->GetName(),id);
            delete op;
            return false;
        }
        if(insertBeginning)
        {
            script.Insert(where++,op);
        }
        else
        {
            script.Push(op);
        }
        // Execute any outstanding post load operations.
        if(postLoadAssignQuest)
        {
            AssignQuestResponseOp* aqr_op = dynamic_cast<AssignQuestResponseOp*>(op);
            if(aqr_op->GetTimeoutMsg())
            {
                CheckQuestTimeoutOp* op2 = new CheckQuestTimeoutOp(aqr_op);
                script.Insert(0,op2);
                where++;
            }
            AssignQuestSelectOp* op3 = new AssignQuestSelectOp(aqr_op);
            script.Insert(0,op3);
            where++;
        }
    }
    return true;
}

bool NpcResponse::HasPublicResponse()
{
    for(size_t i=0; i<script.GetSize(); i++)
    {
        ResponseOperation* op = script[i];
        if(op && op->IsPublic())
            return true;
    }
    return false;
}

csTicks NpcResponse::ExecuteScript(gemActor* player, gemNPC* target)
{
    timeDelay = 0; // Say commands, etc. should be delayed by 2-3 seconds to simulate typing

    active_quest = -1;  // not used by default

    int voiceNumber = -1; //default no voice ran yet.

    for(size_t i=0; i<script.GetSize(); i++)
    {
        if(!script[i]->Run(target,player,this,timeDelay,voiceNumber))
        {
            csString resp = script[i]->GetResponseScript();
            Error3("Error running script in %s operation for client %s.",
                   resp.GetData(), player->GetClient() ? player->GetClient()->GetName() : "NPC");
            return (csTicks)-1;
        }
        else if(voiceNumber >= 0) //if it's >= 0. we have to run a voice, else -1 and -2 means we have nothing to do
        {
            //substituites $keywords
            csString voiceFile = this->GetVoiceFile(voiceNumber);
            target->GetNPCDialogPtr()->SubstituteKeywords(player->GetClient(),voiceFile);

            //executes the voice file
            psserver->GetChatManager()->SendMultipleAudioFileHashes(player->GetClient(), voiceFile);

            voiceNumber = -2; //we use -2 to alert that we run an audio file already during the script.
        }
    }
    return timeDelay;
}

csString NpcResponse::GetResponseScript()
{
    csString respScript = "<response>";
    for(size_t i=0; i<script.GetSize(); i++)
    {
        csString op;
        op.Format("<%s/>",script[i]->GetResponseScript().GetData());
        respScript.Append(op);
    }
    respScript.Append("</response>");
    return respScript;
}

bool NpcResponse::ParsePrerequisiteScript(const char* xmlstr,bool insertBeginning)
{
    if(!xmlstr || strcmp(xmlstr,"")==0 || strcmp(xmlstr,"0")==0)
    {
        return true;
    }

    csRef<psQuestPrereqOp> op;
    if(!LoadPrerequisiteXML(op,NULL,xmlstr))
    {
        return false;
    }

    if(op)
    {
        return AddPrerequisite(op,insertBeginning);
    }

    return false;
}

bool NpcResponse::AddPrerequisite(csRef<psQuestPrereqOp> op, bool insertBeginning)
{
    // Make sure that the first op is an AND list if there are an
    // prerequisite from before.
    if(prerequisite)
    {
        // Check if first op is an and list.
        psQuestPrereqOp* cast = prerequisite;
        csRef<psQuestPrereqOpAnd> list = dynamic_cast<psQuestPrereqOpAnd*>(cast);
        if(list == NULL)
        {
            // If not insert an and list.
            list.AttachNew(new psQuestPrereqOpAnd());
            list->Push(prerequisite);
            prerequisite = list;
        }

        if(insertBeginning)
            list->Insert(0,op);
        else
            list->Push(op);
    }
    else
    {
        // No prerequisite from before so just set this.
        prerequisite = op;
    }

    return true;
}


bool NpcResponse::CheckPrerequisite(psCharacter* character)
{
    if(prerequisite)
    {
        return prerequisite->Check(character);
    }

    return true; // No prerequisite so its ok to do this quest
}


////////////////////////////////
/////////// Training ///////////
////////////////////////////////

/** Checks if training is possible (enough money to pay etc.)
  * If not, tells the user why */
bool CheckTraining(gemNPC* who, Client* target, psSkillInfo* skill)
{
    csTicks timeDelay=0;

    if(!who->GetCharacterData()->IsTrainer())
    {
        CPrintf(CON_ERROR, "%s isn't a trainer, but have train in dialog!!",who->GetCharacterData()->GetCharName());
        return false;
    }

    psCharacter* character = target->GetCharacterData();

    CPrintf(CON_DEBUG, "    PP available: %u\n", character->GetProgressionPoints());

    // Test for progression points
    if(character->GetProgressionPoints() <= 0)
    {
        csString str;
        csString downcase = skill->name;
        downcase.Downcase();
        str.Format("You don't have any progression points to be trained in %s, Sorry",downcase.GetData());
        who->Say(str,target,false,timeDelay);
        return false;
    }

    // Test for money
    if(skill->price > character->Money())
    {
        csString str;
        csString downcase = skill->name;
        downcase.Downcase();
        str.Format("Sorry, but I see that you don't have enough money to be trained in %s",downcase.GetData());
        who->Say(str,target,false,timeDelay);
        return false;
    }

    if(!character->CanTrain(skill->id))
    {
        csString str;
        csString downcase = skill->name;
        downcase.Downcase();
        str.Format("You can't train %s higher yet",downcase.GetData());
        who->Say(str,target,false,timeDelay);
        return false;
    }
    return true;
}

/** This class asks user to confirm that he really wants to pay for training */
class TrainingConfirm : public PendingQuestion
{
public:
    TrainingConfirm(const csString &question,
                    gemNPC* who, Client* target, psSkillInfo* skill)
        : PendingQuestion(target->GetClientNum(), question,
                          psQuestionMessage::generalConfirm)
    {
        this->who     =   who;
        this->target  =   target;
        this->skill   =   skill;
    }

    virtual void HandleAnswer(const csString &answer)
    {
        if(answer != "yes")
            return;

        // We better check again, if everything is still ok
        if(!CheckTraining(who, target, skill))
            return;

        psCharacter* character = target->GetCharacterData();

        character->UseProgressionPoints(1);
        character->SetMoney(character->Money()-skill->price);
        character->Train(skill->id,1);

        csString downcase = skill->name;
        downcase.Downcase();
        psserver->SendSystemInfo(target->GetClientNum(), "You've received some %s training", downcase.GetData());
    }

protected:
    gemNPC* who;
    Client* target;
    psSkillInfo* skill;
};

////////////////////////////////////////////////////////////////////
// Responses

bool SayResponseOp::Load(iDocumentNode* node)
{
    sayWhat = NULL;
    if(node->GetAttributeValue("text"))
    {
        sayWhat = new csString(node->GetAttributeValue("text"));
    }

    return true;
}

csString SayResponseOp::GetResponseScript()
{
    psString resp;
    resp = GetName();
    return resp;
}

bool SayResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psString response;

    if(sayWhat)
    {
        response = *sayWhat;
        if(voiceNumber == -1) //this means it's a script so we change it only if we didn't run it yet
            voiceNumber = 0;
    }
    else
        response = owner->GetResponse(voiceNumber);

    who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),response);

    if(target->GetSecurityLevel() >= GM_DEVELOPER)
        response.AppendFmt(" (%s)",owner->triggerText.GetDataSafe());

    who->Say(response,target->GetClient(), saypublic && target->GetVisibility(),timeDelay);

    return true;
}

bool ActionResponseOp::Load(iDocumentNode* node)
{
    anim = node->GetAttributeValue("anim");
    if(node->GetAttributeValue("text"))
        actWhat = new csString(node->GetAttributeValue("text"));
    else
        actWhat = NULL;
    return true;
}

csString ActionResponseOp::GetResponseScript()
{
    psString resp;
    resp = GetName();
    resp.AppendFmt(" anim=\"%s\"",anim.GetDataSafe());
    if (actWhat)
    {
        resp.AppendFmt(" anim=\"%s\"",actWhat->GetDataSafe());
    }
    return resp;
}

bool ActionResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    if(!anim.IsEmpty())
        who->SetAction(anim,timeDelay);

    if(actWhat)
    {
        csString response(*actWhat);
        who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),response);
        who->ActionCommand(actionMy, actionNarrate, response.GetDataSafe(), target->GetClient(), IsPublic() && target->GetVisibility(), timeDelay);
    }

    return true;
}

bool NPCCmdResponseOp::Load(iDocumentNode* node)
{
    cmd = node->GetAttributeValue("cmd");
    return true;
}

csString NPCCmdResponseOp::GetResponseScript()
{
    psString resp;
    resp = GetName();
    resp.AppendFmt(" cmd=\"%s\"",cmd.GetData());
    return resp;
}

bool NPCCmdResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psserver->GetNPCManager()->QueueNPCCmdPerception(who,cmd);
    return true;
}


bool VerifyQuestCompletedResponseOp::Load(iDocumentNode* node)
{
    quest = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("quest"));
    error_msg = node->GetAttributeValue("error_msg");
    if(error_msg=="(null)") error_msg="";

    if(!quest)
    {
        Error2("Quest %s was not found in VerifyQuestCompleted script op! You must have at least one!",node->GetAttributeValue("quest"));
        return false;
    }
    return true;
}

csString VerifyQuestCompletedResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" quest=\"%s\"",quest->GetName());
    if(!error_msg.IsEmpty())
    {
        resp.AppendFmt(" error_msg=\"%s\"",error_msg.GetDataSafe());
    }
    return resp;
}

bool VerifyQuestCompletedResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    bool avail = target->GetCharacterData()->GetQuestMgr().CheckQuestCompleted(quest);
    if(!avail)
    {
        who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),error_msg);
        who->Say(error_msg,target->GetClient(),false,timeDelay);
        return false;
    }
    return true;  // and don't say anything
}

VerifyQuestAssignedResponseOp::VerifyQuestAssignedResponseOp(int quest_id)
{
    name="verifyquestassigned";
    quest = psserver->GetCacheManager()->GetQuestByID(quest_id);
    if(!quest)
    {
        Error2("Quest %d was not found in VerifyQuestAssigned script op!",quest_id);
    }
}

bool VerifyQuestAssignedResponseOp::Load(iDocumentNode* node)
{
    quest = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("quest"));
    error_msg = node->GetAttributeValue("error_msg");
    if(error_msg=="(null)") error_msg="";
    if(!quest && node->GetAttributeValue("quest"))
    {
        Error2("Quest <%s> not found!",node->GetAttributeValue("quest"));
    }
    else if(!quest)
    {
        Error1("Quest name must be specified in VerifyQuestAssigned script op!");
        return false;
    }
    return true;
}

csString VerifyQuestAssignedResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" quest=\"%s\"",quest->GetName());
    if(!error_msg.IsEmpty())
    {
        resp.AppendFmt(" error_msg=\"%s\"",error_msg.GetDataSafe());
    }
    return resp;
}

bool VerifyQuestAssignedResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    bool avail = target->GetCharacterData()->GetQuestMgr().CheckQuestAssigned(quest);
    if(!avail)
    {
        if(error_msg.IsEmpty() || error_msg.Length() == 0)
        {
            error_msg = "I don't know what you are talking about.";  // TODO: use the standard error response for that NPC
        }
        who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),error_msg);
        who->Say(error_msg,target->GetClient(),false,timeDelay);
        return false;
    }
    return true;  // and don't say anything
}

VerifyQuestNotAssignedResponseOp::VerifyQuestNotAssignedResponseOp(int quest_id)
{
    name="verifyquestnotassigned";
    quest = psserver->GetCacheManager()->GetQuestByID(quest_id);
    if(!quest)
    {
        Error2("Quest %d was not found in VerifyQuestNotAssigned script op!",quest_id);
    }
}

bool VerifyQuestNotAssignedResponseOp::Load(iDocumentNode* node)
{
    quest = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("quest"));
    error_msg = node->GetAttributeValue("error_msg");
    if(error_msg=="(null)") error_msg="";
    if(!quest && node->GetAttributeValue("quest"))
    {
        Error2("Quest <%s> not found!",node->GetAttributeValue("quest"));
    }
    else if(!quest)
    {
        Error1("Quest name must be specified in VerifyQuestNotAssigned script op!");
        return false;
    }
    return true;
}

csString VerifyQuestNotAssignedResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" quest=\"%s\"",quest->GetName());
    if(!error_msg.IsEmpty())
    {
        resp.AppendFmt(" error_msg=\"%s\"",error_msg.GetDataSafe());
    }
    return resp;
}

bool VerifyQuestNotAssignedResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    bool avail = target->GetCharacterData()->GetQuestMgr().CheckQuestAssigned(quest);
    if(avail)
    {
        if(error_msg.IsEmpty() || error_msg.Length() == 0)
        {
            error_msg = "I don't know what you are talking about.";  // TODO: use the standard error response for that NPC
        }
        who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),error_msg);
        who->Say(error_msg,target->GetClient(),false,timeDelay);
        return false;
    }
    return true;  // and don't say anything
}


bool AssignQuestResponseOp::Load(iDocumentNode* node)
{
    quest[0] = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("q1"));
    quest[1] = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("q2"));
    quest[2] = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("q3"));
    quest[3] = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("q4"));
    quest[4] = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("q5"));

    if(!quest[0])
    {
        Error2("Quest %s was not found in Assign Quest script op! You must have at least one!",node->GetAttributeValue("q1"));
        return false;
    }
    if(node->GetAttributeValue("timeout_msg"))
    {
        timeout_msg = node->GetAttributeValue("timeout_msg");
        if(timeout_msg=="(null)") timeout_msg="";
    }
    if(quest[4])
        num_quests = 5;
    else if(quest[3])
        num_quests = 4;
    else if(quest[2])
        num_quests = 3;
    else if(quest[1])
        num_quests = 2;
    else if(quest[0])
        num_quests = 1;

    return true;
}

csString AssignQuestResponseOp::GetResponseScript()
{
    psString resp = GetName();
    for(int n = 0; n < 5; n++)
    {
        if(quest[n])
        {
            resp.AppendFmt(" q%d=\"%s\"",n+1,quest[n]->GetName());
        }

    }
    if(!timeout_msg.IsEmpty())
    {
        resp.AppendFmt(" timeout_msg=\"%s\"",timeout_msg.GetDataSafe());
    }
    return resp;
}

bool AssignQuestResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    if(owner->GetActiveQuest() == -1)
    {
        owner->SetActiveQuest(GetMaxQuests());
        Debug4(LOG_QUESTS, target->GetClient()->GetClientNum(),"Selected quest %d out of %d for %s",owner->GetActiveQuest()+1,
               GetMaxQuests(),target->GetCharacterData()->GetCharName());
    }

    if(target->GetCharacterData()->GetQuestMgr().CheckQuestAssigned(quest[owner->GetActiveQuest()]))
    {
        Debug3(LOG_QUESTS, target->GetClient()->GetClientNum(),"Quest(%d) is already assigned for %s",owner->GetActiveQuest()+1,
               target->GetCharacterData()->GetCharName());
        return false;
    }
    timeDelay += 1000;
    psserver->questmanager->Assign(quest[owner->GetActiveQuest()],target->GetClient(),who,timeDelay);
    return true;
}

bool AssignQuestSelectOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    if(owner->GetActiveQuest() == -1)
    {
        owner->SetActiveQuest(quest_op->GetMaxQuests());
        Debug4(LOG_QUESTS, target->GetClient()->GetClientNum(), "Selected quest %d out of %d for %s",owner->GetActiveQuest()+1,
               quest_op->GetMaxQuests(),target->GetCharacterData()->GetCharName());
    }

    if(target->GetCharacterData()->GetQuestMgr().CheckQuestAssigned(quest_op->GetQuest(owner->GetActiveQuest())))
    {
        Debug3(LOG_QUESTS, target->GetClient()->GetClientNum(), "Quest(%d) is already assignd for %s",quest_op->GetQuest(owner->GetActiveQuest())->GetID(),
               target->GetCharacterData()->GetCharName());
        return false;
    }

    return true;
}

csString AssignQuestSelectOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" max_quest=\"%d\"", quest_op->GetMaxQuests());
    return resp;
}

/*
bool FireEventResponseOp::Load(iDocumentNode *node)
{
    event = node->GetAttributeValue("name");
    return true;
}

csString FireEventResponseOp::GetResponseScript()
{
    psString resp = GetName();
    return resp;
}

bool FireEventResponseOp::Run(gemNPC *who, Client *target,NpcResponse *owner,csTicks& timeDelay, int& voiceNumber)
{
    psCharacter *character = target->GetActor()->GetCharacterData();
    character->FireEvent(event);

    return true;
}
*/

bool CheckQuestTimeoutOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    if(owner->GetActiveQuest() == -1)
    {
        owner->SetActiveQuest(quest_op->GetMaxQuests());
        Debug4(LOG_QUESTS, target->GetClient()->GetClientNum(), "Selected quest %d out of %d for %s",owner->GetActiveQuest()+1,
               quest_op->GetMaxQuests(),target->GetCharacterData()->GetCharName());
    }

    bool avail = target->GetCharacterData()->GetQuestMgr().CheckQuestAvailable(quest_op->GetQuest(owner->GetActiveQuest()),
                 who->GetPID());
    if(!avail)
    {
        psString timeOutMsg = quest_op->GetTimeoutMsg();
        who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),timeOutMsg);
        who->Say(timeOutMsg,target->GetClient(),false,timeDelay);
        return false;
    }
    return true;
}

csString CheckQuestTimeoutOp::GetResponseScript()
{
    psString resp = GetName();
    return resp;
}

bool UncompleteQuestResponseOp::Load(iDocumentNode* node)
{
    quest = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("quest_id"));
    if(!quest)
    {
        Error2("Quest '%s' was not found in Uncomplete Quest script op!",node->GetAttributeValue("quest_id"));
        return false;
    }
    error_msg = node->GetAttributeValue("error_msg");
    if(error_msg=="(null)") error_msg="";

    return true;
}

csString UncompleteQuestResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" quest_id=\"%s\"", quest->GetName());
    if(!error_msg.IsEmpty())
    {
        resp.AppendFmt(" error_msg=\"%s\"",error_msg.GetDataSafe());
    }
    return resp;
}

bool UncompleteQuestResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    if(!psserver->questmanager->Uncomplete(quest,target->GetClient(), timeDelay))
    {
        who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),error_msg);
        who->Say(error_msg,target->GetClient(),false,timeDelay);
        return false;
    }
    else
        return true;
}

bool CompleteQuestResponseOp::Load(iDocumentNode* node)
{
    quest = psserver->GetCacheManager()->GetQuestByName(node->GetAttributeValue("quest_id"));
    if(!quest)
    {
        Error2("Quest '%s' was not found in Complete Quest script op!",node->GetAttributeValue("quest_id"));
        return false;
    }
    error_msg = node->GetAttributeValue("error_msg");
    if(error_msg=="(null)") error_msg="";

    return true;
}

csString CompleteQuestResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" quest_id=\"%s\"", quest->GetName());
    if(!error_msg.IsEmpty())
    {
        resp.AppendFmt(" error_msg=\"%s\"",error_msg.GetDataSafe());
    }
    return resp;
}

bool CompleteQuestResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    if(!psserver->questmanager->Complete(quest,target->GetClient(), timeDelay))
    {
        who->GetNPCDialogPtr()->SubstituteKeywords(target->GetClient(),error_msg);
        who->Say(error_msg,target->GetClient(),false,timeDelay);
        return false;
    }
    else
        return true;
}

bool GiveItemResponseOp::Load(iDocumentNode* node)
{
    itemstat = psserver->GetCacheManager()->GetBasicItemStatsByID(node->GetAttributeValueAsInt("item"));
    if(!itemstat)
        itemstat = psserver->GetCacheManager()->GetBasicItemStatsByName(node->GetAttributeValue("item"));

    if(!itemstat)
    {
        Error2("ItemStat '%s' was not found in GiveItem script op!",node->GetAttributeValue("item"));
        return false;
    }

    // Check for count attribute
    if(node->GetAttribute("count"))
    {
        count = node->GetAttributeValueAsInt("count");
        if(count < 1)
        {
            Error1("Tried to give negative or zero count in GiveItem script op!");
            count = 1;
        }

        if(count > 1 && !itemstat->GetIsStackable())
        {
            Error2("ItemStat '%s' isn't stackable in GiveItem script op!", node->GetAttributeValue("item"));
            return false;
        }
    }

    // Check for quality attribute
    if(node->GetAttribute("quality"))
    {
        quality = node->GetAttributeValueAsFloat("quality");
    }

    return true;
}

csString GiveItemResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" item=\"%s\"", itemstat->GetName());
    if(count != 1)
    {
        resp.AppendFmt(" count=\"%d\"", count);
    }
    if(quality >= 1)
    {
        resp.AppendFmt(" quality=\"%f\"", quality);
    }

    return resp;
}

bool GiveItemResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psCharacter* character = target->GetCharacterData();
    if(!character)
        return false;

    psItem* item = itemstat->InstantiateBasicItem(false); // Not a transient item

    if(!item)
    {
        Error3("Couldn't give item %u to player %s!",itemstat->GetUID(), target->GetName());
        return false;
    }

    item->SetStackCount(count);

    if(quality >= 1)
    {
        item->SetItemQuality(quality);
        item->SetMaxItemQuality(quality);
    }

    item->SetLoaded();  // Item is fully created


    csString itemName = item->GetQuantityName();

    if(!character->Inventory().AddOrDrop(item))
    {
        psSystemMessage recv(target->GetClient()->GetClientNum(),MSG_ERROR,"You received %s, but dropped it because you can't carry any more.", itemName.GetData());
        psserver->GetNetManager()->SendMessageDelayed(recv.msg, timeDelay);
    }
    else
    {
        psSystemMessage recv(target->GetClient()->GetClientNum(),MSG_INFO,"You have received %s.", itemName.GetData());
        psserver->GetNetManager()->SendMessageDelayed(recv.msg, timeDelay);
    }
    return true;
}

bool FactionResponseOp::Load(iDocumentNode* node)
{
    faction = psserver->GetCacheManager()->GetFaction(node->GetAttributeValue("name"));
    if(!faction)
    {
        Error2("Error: FactionOp faction(%s) not found",node->GetAttributeValue("name"));
        return false;
    }
    value = node->GetAttributeValueAsInt("value");
    return true;
}

csString FactionResponseOp::GetResponseScript()
{
    psString resp = GetName();
    csString escpxml = EscpXML(faction->name);
    resp.AppendFmt(" name=\"%s\"",escpxml.GetData());
    resp.AppendFmt(" value=\"%d\"",value);
    return resp;
}

bool FactionResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psCharacter* player = target->GetCharacterData();
    if(!player)
    {
        Error1("Error: FactionResponceOp found no character");
        return false;
    }

    player->UpdateFaction(faction,value);

    return true;
}

RunScriptResponseOp::~RunScriptResponseOp()
{
    MathScript::Destroy(bindings);
}

bool RunScriptResponseOp::Load(iDocumentNode* node)
{
    scriptname = node->GetAttributeValue("script");
    if(scriptname.IsEmpty())
    {
        Error1("Progression script name was not specified in Run script op!");
        return false;
    }
    bindingsText = node->GetAttributeValue("with");
    if(!bindingsText.IsEmpty())
    {
        bindings = MathScript::Create("RunScriptResponseOp bindings", bindingsText);
        return bindings != NULL;
    }
    return true;
}

csString RunScriptResponseOp::GetResponseScript()
{
    csString resp = GetName();
    resp.AppendFmt(" script=\"%s\"",scriptname.GetData());
    if(bindings)
        resp.AppendFmt(" with=\"%s\"", bindingsText.GetData());

    return resp;
}

bool RunScriptResponseOp::Run(gemNPC* who, gemActor* target, NpcResponse* owner, csTicks &timeDelay, int &voiceNumber)
{
    ProgressionScript* script;

    /*
    if ((scriptname.GetDataSafe())[0] == '<')
    {
        script = ProgressionScript::Create(...);
    }
    else
    */
    {
        script = psserver->GetProgressionManager()->FindScript(scriptname);
        if(!script)
        {
            Error2("Progression script '%s' was not found in the Progression Manager!", scriptname.GetData());
            return true;
        }
    }

    MathEnvironment env;
    env.Define("NPC", who);
    env.Define("Target", target);
    if(bindings)
    {
        bindings->Evaluate(&env);
    }
    script->Run(&env);

    return true;
}


bool TrainResponseOp::Load(iDocumentNode* node)
{
    if(node->GetAttributeValue("skill"))
        skill = psserver->GetCacheManager()->GetSkillByName(node->GetAttributeValue("skill"));
    else
        skill = NULL;

    if(!skill)
    {
        CPrintf(CON_ERROR, "Couldn't find skill '%s'",node->GetAttributeValue("skill")? node->GetAttributeValue("skill"):"Not Specified");
        return false;
    }

    return true;
}

csString TrainResponseOp::GetResponseScript()
{
    psString resp = GetName();
    return resp;
}

bool TrainResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    if(CheckTraining(who, target->GetClient(), skill))
    {
        csString question;
        question.Format("Do you really want to train %s ? You will be charged %d trias.",
                        skill->name.GetData(), skill->price.GetTotal());
        psserver->questionmanager->SendQuestion(
            new TrainingConfirm(question, who, target->GetClient(), skill));
    }
    return true;
}

bool SetVariableResponseOp::Load(iDocumentNode* node)
{
    variableName = node->GetAttributeValue("name");
    variableValue = node->GetAttributeValue("value");
    return true;
}

csString SetVariableResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" name=\"%s\"",variableName.GetData());
    resp.AppendFmt(" value=\"%s\"",variableValue.GetData());
    return resp;
}

bool SetVariableResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psCharacter* c = target->GetCharacterData();
    c->SetVariable(variableName,variableValue);
    return true;
}

bool UnSetVariableResponseOp::Load(iDocumentNode* node)
{
    variableName = node->GetAttributeValue("name");
    return true;
}

csString UnSetVariableResponseOp::GetResponseScript()
{
    psString resp = GetName();
    resp.AppendFmt(" name=\"%s\"",variableName.GetData());
    return resp;
}

bool UnSetVariableResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psCharacter* c = target->GetCharacterData();
    c->UnSetVariable(variableName);
    return true;
}


bool GuildAwardResponseOp::Load(iDocumentNode* node)
{
    karma = node->GetAttributeValueAsInt("karma");
    return true;
}

csString GuildAwardResponseOp::GetResponseScript()
{
    psString resp = GetName();
    return resp;
}

bool GuildAwardResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psGuildInfo* guild = psserver->GetCacheManager()->FindGuild(target->GetGuildID());
    if(!guild)
    {
        CPrintf(CON_ERROR, "Couldn't find guild (%d). Guild karma points not added\n",target->GetGuildID());
        return false;
    }
    guild->SetKarmaPoints(guild->GetKarmaPoints() + karma);
    // TODO: Notify player and guild of what happened here.
    return true;
}

bool HireResponseOp::Load(iDocumentNode* node)
{
    csString command = node->GetAttributeValue("cmd");
    if (command == "start")
    {
        hireCommand = HIRE_COMMAND_START;
    } else if (command == "type")
    {
        hireCommand = HIRE_COMMAND_TYPE;
        typeName = node->GetAttributeValue("name");
        typeNPCType = node->GetAttributeValue("type");
        if (typeName.IsEmpty() || typeNPCType.IsEmpty())
        {
            return false;
        }
    } else if (command == "master")
    {
        hireCommand = HIRE_COMMAND_MASTER;
        masterNPCId = node->GetAttributeValueAsInt("master");
        if (!masterNPCId.IsValid())
        {
            return false;
        }
    } else if (command == "confirm")
    {
        hireCommand = HIRE_COMMAND_CONFIRM;
    } else if (command == "script")
    {
        hireCommand = HIRE_COMMAND_SCRIPT;
    } else if (command == "release")
    {
        hireCommand = HIRE_COMMAND_RELEASE;
    } else
    {
        return false;
    }
    
    return true;
}

csString HireResponseOp::GetResponseScript()
{
    psString resp = GetName();
    switch (hireCommand)
    {
    case HIRE_COMMAND_START:
        resp.AppendFmt(" cmd=\"%s\"","start");
        break;
    case HIRE_COMMAND_TYPE:
        resp.AppendFmt(" cmd=\"%s\"","type");
        resp.AppendFmt(" name=\"%s\"",typeName.GetDataSafe());
        resp.AppendFmt(" type=\"%s\"",typeNPCType.GetDataSafe());
        break;
    case HIRE_COMMAND_MASTER:
        resp.AppendFmt(" cmd=\"%s\"","master");
        resp.AppendFmt(" master=\"%d\"",masterNPCId.Unbox());
        break;
    case HIRE_COMMAND_CONFIRM:
        resp.AppendFmt(" cmd=\"%s\"","confirm");
        break;
    case HIRE_COMMAND_SCRIPT:
        resp.AppendFmt(" cmd=\"%s\"","script");
        break;
    case HIRE_COMMAND_RELEASE:
        resp.AppendFmt(" cmd=\"%s\"","release");
        break;
    }
    return resp;
}

bool HireResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    HireManager* hireManager = psserver->GetHireManager();
    if (!hireManager)
    {
        Error1("Could not find HireManager");
        return false;
    }

    switch (hireCommand)
    {
    case HIRE_COMMAND_START:
        hireManager->StartHire(target);
        break;
    case HIRE_COMMAND_TYPE:
        hireManager->SetHireType(target, typeName, typeNPCType);
        break;
    case HIRE_COMMAND_MASTER:
        hireManager->SetHireMasterPID(target, masterNPCId);
        break;
    case HIRE_COMMAND_CONFIRM:
        hireManager->ConfirmHire(target);
        break;
    case HIRE_COMMAND_SCRIPT:
        {
            Client * client = who->GetClient();
            if (client)
            {
                hireManager->HandleScriptMessageRequest(client->GetClientNum(), target, who);
            }
            break;
        }
    case HIRE_COMMAND_RELEASE:
        hireManager->ReleaseHire(target, who);
        break;
    }

    return true;
}

bool OfferRewardResponseOp::Load(iDocumentNode* node)
{
    // <offer>
    //      <item id="9"/>
    //      <item id="10"/>
    // </offer>

    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    // for each item in the offer list
    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        // get item
        psItemStats* itemStat;
        uint32 itemID = (uint32)node->GetAttributeValueAsInt("id");

        if(itemID)
            itemStat = psserver->GetCacheManager()->GetBasicItemStatsByID(itemID);
        else
            itemStat = psserver->GetCacheManager()->GetBasicItemStatsByName(node->GetAttributeValue("name"));

        // make sure that the item exists
        if(!itemStat)
        {
            Error3("ItemStat #%u/%s was not found in OfferReward script op!", itemID, node->GetAttributeValue("name"));
            return false;
        }

        // Check for count attribute
        int count = 1;
        if(node->GetAttribute("count"))
        {
            count = node->GetAttributeValueAsInt("count");
            if(count < 1)
            {
                Error1("Tried to give negative or zero count in OfferReward script op!");
                count = 1;
            }

            if(count > 1 && !itemStat->GetIsStackable())
            {
                Error3("ItemStat #%u/%s isn't stackable in OfferReward script op!", itemID, node->GetAttributeValue("name"));
                return false;
            }
        }

        // Check for quality attribute
        float quality = 0.0;
        if(node->GetAttribute("quality"))
        {
            quality = node->GetAttributeValueAsFloat("quality");
        }

        // add this item to the list
        struct QuestRewardItem item;
        item.itemstat = itemStat;
        item.count = count;
        item.quality = quality;
        offer.Push(item);
    }
    return true;
}

csString OfferRewardResponseOp::GetResponseScript()
{
    psString resp = GetName();
    for(size_t n = 0; n < offer.GetSize(); n++)
    {
        resp.AppendFmt("><item id=\"%s\"/", offer[n].itemstat->GetName());
        if(offer[n].count > 1)
        {
            resp.AppendFmt(" count=\"%d\"", offer[n].count);
        }
        if(offer[n].quality >= 1)
        {
            resp.AppendFmt(" quality=\"%f\"", offer[n].quality);
        }
    }
    resp.AppendFmt("></offer");
    return resp;
}

bool OfferRewardResponseOp::Run(gemNPC* who, gemActor* target, NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psserver->questmanager->OfferRewardsToPlayer(target->GetClient(), offer, timeDelay);
    return true;
}

bool MoneyResponseOp::Load(iDocumentNode* node)
{
    if(node->GetAttributeValue("value"))
        money = psMoney(node->GetAttributeValue("value"));
    else
        money = psMoney();

    if(money.GetTotal()==0)
    {
        CPrintf(CON_ERROR, "Couldn't load money '%s' or money = 0",node->GetAttributeValue("value"));
        return false;
    }

    return true;
}

csString MoneyResponseOp::GetResponseScript()
{
    psString resp;
    resp = GetName();
    resp.AppendFmt(" value=\"%s\"",money.ToString().GetData());
    return resp;
}

bool MoneyResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psCharacter* character = target->GetCharacterData();

    character->SetMoney(character->Money()+money);

    psserver->SendSystemInfo(target->GetClient()->GetClientNum(), "You've received %s",money.ToUserString().GetData());
    return true;
}

bool IntroduceResponseOp::Load(iDocumentNode* node)
{
    if(node->GetAttributeValue("name"))
        targetName = node->GetAttributeValue("name");
    else
        targetName = "Me";

    return true;
}

csString IntroduceResponseOp::GetResponseScript()
{
    psString resp;
    resp = GetName();
    resp.AppendFmt(" name=\"%s\"",targetName.GetData());
    return resp;
}

bool IntroduceResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    psCharacter* character = target->GetCharacterData();
    psCharacter* npcChar = who->GetCharacterData();

    if(targetName != "Me")
    {
        gemObject* obj = psserver->GetAdminManager()->FindObjectByString(targetName,who);
        if(obj)
        {
            character->Introduce(((gemNPC*)obj)->GetCharacterData());
            obj->Send(target->GetClient()->GetClientNum(), false, false);
            psserver->SendSystemInfo(target->GetClient()->GetClientNum(), "You now know %s",((gemNPC*)obj)->GetName());
        }
    }
    else
    {
        character->Introduce(npcChar);
        who->Send(target->GetClient()->GetClientNum(), false, false);
        psserver->SendSystemInfo(target->GetClient()->GetClientNum(), "You now know %s",who->GetName());
    }

    return true;
}

bool DoAdminCommandResponseOp::Load(iDocumentNode* node)
{
    origCommandString = node->GetAttributeValue("command");
    return true;
}

csString DoAdminCommandResponseOp::GetResponseScript()
{
    psString resp;
    return resp;
}

bool DoAdminCommandResponseOp::Run(gemNPC* who, gemActor* target,NpcResponse* owner,csTicks &timeDelay, int &voiceNumber)
{
    modifiedCommandString = origCommandString;
    csString format;
    format.Format("\"%s\"", target->GetCharacterData()->GetCharFullName());
    modifiedCommandString.ReplaceAll("targetchar", format);
    format.Format("\"%s\"", who->GetNPCPtr()->GetCharacterData()->GetCharFullName());
    modifiedCommandString.ReplaceAll("sourcenpc", format);
    psAdminCmdMessage msg(modifiedCommandString, 0);
    msg.msg->current=0;
    psserver->GetAdminManager()->HandleNpcCommand(msg.msg, target->GetClient());
    return true;
}

NpcDialogMenu::NpcDialogMenu()
{
    counter = 0;
}

void NpcDialogMenu::AddTrigger(const csString &menuText, const csString &trigger, psQuest* quest, psQuestPrereqOp* script)
{
    NpcDialogMenu::DialogTrigger new_trigger;

    new_trigger.menuText     = menuText;
    new_trigger.trigger      = trigger;
    new_trigger.quest        = quest;
    new_trigger.prerequisite = script;
    new_trigger.triggerID    = counter++;

    this->triggers.Push(new_trigger);
}

void NpcDialogMenu::Add(NpcDialogMenu* add)
{
    if(!add)
        return;

    for(size_t i=0; i < add->triggers.GetSize(); i++)
    {
        //printf("Adding '%s' to menu.\n", add->triggers[i].menuText.GetData() );
        AddTrigger(add->triggers[i].menuText, add->triggers[i].trigger, add->triggers[i].quest, add->triggers[i].prerequisite);
    }
    //printf("Added %lu triggers to menu.\n", (unsigned long) add->triggers.GetSize());
}

void NpcDialogMenu::ShowMenu(Client* client,csTicks delay, gemNPC* npc)
{
    if(client == NULL)
        return;

    psDialogMenuMessage menu;

    csString currentQuest;
    int count = 0;

    bool IsTesting = client->GetCharacterData()->GetActor()->questtester;
    bool IsGm = client->IsGM();

    for(size_t i=0; i < counter; i++)
    {
        csString prereq;

        if(triggers[i].quest && !triggers[i].quest->Active() && !IsTesting)
            continue;

        if(triggers[i].prerequisite)
        {
            prereq = triggers[i].prerequisite->GetScript();
        }

        //if (!prereq.IsEmpty())
        //{
        //    printf("Item %lu Prereq : %s\n", (unsigned long) i, prereq.GetDataSafe());
        //}
        //else
        //{
        //    printf("Item %lu has no prereqs.\n", (unsigned long) i);
        //}

        if(triggers[i].prerequisite && !IsTesting)
        {
            if(!triggers[i].prerequisite->Check(client->GetCharacterData()))
            {
                //printf("Prereq check failed. Skipping.\n");
                continue;
            }
        }

        //check availability (as per lockout). Note as gm we show quest even if in lockout as > gm get
        //an error message in system even if they can't get it because testermode is off
        if(triggers[i].quest && !IsGm && !IsTesting && !client->GetCharacterData()->GetQuestMgr().CheckQuestAvailable(triggers[i].quest, npc->GetPID()))
            continue;

        // Check to see about inserting a quest heading
        if(!(currentQuest == (triggers[i].quest ? triggers[i].quest->GetName() : "(Unknown)")))
        {
            currentQuest = triggers[i].quest ? triggers[i].quest->GetName() : "(Unknown)";
            csString temp = "h:", temptrig = "heading";
            temp += currentQuest;
            menu.AddResponse((uint32_t) i, temp, temptrig);
        }


        csString menuText = triggers[i].menuText;
        npc->GetNPCDialogPtr()->SubstituteKeywords(client,menuText);

        // Only add the trigger if it isn't a question, also add the
        // questid if available (for compatibility only when not a question)
        csString trigger;

        // It's a quest
        if(triggers[i].quest)
        {
            // As the trigger is unused client side, being a question,
            // use it to store the questID.
            if(menuText.Find("?=") != SIZET_NOT_FOUND)
            {
                trigger.Format("{%d}", triggers[i].quest->GetID());
            }
            else if(triggers[i].trigger.GetAt(0) != '<')
            {
                // This is a normal trigger add {d} in front of it with the questID
                trigger.Format("{%d} %s", triggers[i].quest->GetID(), triggers[i].trigger.GetData());
            }
            else
            {
                // This is an exchange, add an additional tag, which is unpacked by exchange manager.
                // Simple replacement to add the needed tag: reasoning, there can be only one </l>
                // in the xml so this <l money="0,0,0,0"><item n="Steel Falchion" c="1"/></l>
                // is made this: <l money="0,0,0,0"><item n="Steel Falchion" c="1"/><questid id="1234"/></l>
                trigger = triggers[i].trigger;
                trigger.FindReplace("</l>", csString().Format("<questid id=\"%d\"/></l>", triggers[i].quest->GetID()));
            }
        }
        else //not part of a quest
        {
            if(menuText.Find("?=") != SIZET_NOT_FOUND)
            {
                trigger = "(question)";
            }
            else
            {
                trigger = triggers[i].trigger;
            }
        }

        menu.AddResponse((uint32_t) i,
                         menuText,
                         trigger);
        count++;
    }

    if(count)
    {
        menu.BuildMsg(client->GetClientNum());
        psserver->GetNetManager()->SendMessageDelayed(menu.msg, delay);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "This NPC has no quest for you, but might have other things to say.");
    }
}

void NpcDialogMenu::SetPrerequisiteScript(psQuestPrereqOp* script)
{
    csString prereq;

    if(script)
        prereq = script->GetScript();

    if(!prereq.IsEmpty())
    {
        //printf("Setting menu %p to have trigger prereq : %s\n", this, prereq.GetDataSafe());
    }

    // Each item must have its own prequisite script so they can be different when menus are merged
    // even though they appear to all be set the same here.
    for(size_t i=0; i < counter; i++)
    {
        triggers[i].prerequisite = script;
    }
}
