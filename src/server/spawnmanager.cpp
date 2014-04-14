/*
 * spawnmanager.cpp by Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/randomgen.h>
#include <csutil/csstring.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iutil/object.h>
#include <iengine/engine.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psitemstats.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/pssectorinfo.h"

#include "net/msghandler.h"

#include "rpgrules/vitals.h"

#include "util/eventmanager.h"
#include "util/psconst.h"
#include "util/psdatabase.h"
#include "util/psxmlparser.h"
#include "util/serverconsole.h"
#include "util/slots.h"
#include "util/strutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#define SPAWNDEBUG
#include "spawnmanager.h"
#include "cachemanager.h"
#include "client.h"
#include "clients.h"
#include "entitymanager.h"
#include "events.h"
#include "gem.h"
#include "invitemanager.h"
#include "netmanager.h"
#include "playergroup.h"
#include "progressionmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "serverstatus.h"
#include "globals.h"
#include "lootrandomizer.h"
#include "npcmanager.h"


/** A structure to hold the clients that are pending a group loot question.
 */
class PendingLootPrompt : public PendingQuestion
{
public:
    psItem* item;    ///< The looted item we're prompting for
    PID looterID;    ///< The ID of the player who attempted to pick up the item
    PID rollerID;    ///< The ID of the player who would have won it in a roll
    PID looteeID;

    csString lootername;
    csString rollername;
    csString looteename;

    PendingLootPrompt(Client* looter,
                      Client* roller,
                      psItem* loot,
                      psCharacter* dropper,
                      const char* question,
                      CacheManager* cachemanager,
                      GEMSupervisor* gemsupervisor)
        : PendingQuestion(roller->GetClientNum(),
                          question,
                          psQuestionMessage::generalConfirm)
    {
        cacheManager = cachemanager;
        gemSupervisor = gemsupervisor;


        item = loot;
        looterID = looter->GetActor()->GetPID();
        rollerID = roller->GetActor()->GetPID();
        looteeID = dropper->GetPID();

        // These might not be around later, so save their names now
        lootername = looter->GetName();
        rollername = roller->GetName();
        looteename = dropper->GetCharName();
    }

    virtual ~PendingLootPrompt() { }

    void HandleAnswer(const csString &answer)
    {
        PendingQuestion::HandleAnswer(answer);

        if(dynamic_cast<psItem*>(item) == NULL)
        {
            Error2("Item held in PendingLootPrompt with id %u has been lost", id);
            return;
        }

        gemActor* looter = gemSupervisor->FindPlayerEntity(looterID);
        gemActor* roller = gemSupervisor->FindPlayerEntity(rollerID);

        gemActor* getter = (answer == "yes") ? looter : roller ;

        // If the getter left the world, default to the other player
        if(!getter /*|| !getter->InGroup()*/)
        {
            getter = (answer == "yes") ? roller : looter ;

            // If the other player also vanished, get rid of the item and be done with it...
            if(!getter /*|| !getter->InGroup()*/)
            {
                cacheManager->RemoveInstance(item);
                return;
            }
        }

        // Create the loot message
        csString lootmsg;
        lootmsg.Format("%s was %s a %s by roll winner %s",
                       lootername.GetData(),
                       (getter == looter) ? "allowed to loot" : "stopped from looting",
                       item->GetName(),
                       rollername.GetData());

        // Attempt to give to getter
        bool dropped = getter->GetCharacterData()->Inventory().AddOrDrop(item);

        if(!dropped)
            lootmsg.Append(", but can't hold anymore");

        // Send out the loot message
        psSystemMessage loot(getter->GetClientID(), MSG_LOOT, lootmsg.GetData());
        getter->SendGroupMessage(loot.msg);

        psLootEvent evt(
            looteeID,
            looteename,
            getter->GetCharacterData()->GetPID(),
            lootername,
            item->GetUID(),
            item->GetName(),
            item->GetStackCount(),
            (int)item->GetCurrentStats()->GetQuality(),
            0
        );
        evt.FireEvent();
    }

    void HandleTimeout()
    {
        HandleAnswer("yes");
    }
private:
    CacheManager* cacheManager;
    GEMSupervisor* gemSupervisor;
};

SpawnManager::SpawnManager(psDatabase* db, CacheManager* cachemanager, EntityManager* entitymanager, GEMSupervisor* gemsupervisor)
{
    database  = db;
    cacheManager = cachemanager;
    entityManager = entitymanager;
    gem = gemsupervisor;

    //get a reference to the loot randomizer as we use it when making random loot items
    lootRandomizer = cachemanager->getLootRandomizer();

    PreloadDatabase();

    Subscribe(&SpawnManager::HandleLootItem, MSGTYPE_LOOTITEM, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&SpawnManager::HandleDeathEvent, MSGTYPE_DEATH_EVENT, NO_VALIDATION);
}

SpawnManager::~SpawnManager()
{

    csHash<LootEntrySet*>::GlobalIterator it(looting.GetIterator());
    while(it.HasNext())
    {
        LootEntrySet* loot = it.Next();
        delete loot;
    }

    csHash<SpawnRule*>::GlobalIterator ruleIt(rules.GetIterator());
    while(ruleIt.HasNext())
        delete ruleIt.Next();
}

void SpawnManager::PreloadLootRules()
{
    // loot_rule_id=0 is not valid and not loaded
    Result result(db->Select("select * from loot_rule_details where loot_rule_id>0"));
    if(!result.IsValid())
    {
        Error2("Could not load loot rule details due to database error: %s",
               db->GetLastError());
        return;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        int id = result[i].GetInt("loot_rule_id");
        LootEntrySet* currset = looting.Get(id,NULL);
        if(!currset)
        {
            currset = new LootEntrySet(id, lootRandomizer);
            looting.Put(id,currset);
        }

        LootEntry* entry = new LootEntry;
        int item_id = result[i].GetInt("item_stat_id");

        entry->item = cacheManager->GetBasicItemStatsByID(item_id);
        entry->min_item = result[i].GetInt("min_item");
        entry->max_item = result[i].GetInt("max_item");
        entry->probability = result[i].GetFloat("probability");
        entry->min_money = result[i].GetInt("min_money");
        entry->max_money = result[i].GetInt("max_money");
        entry->randomize = (result[i].GetInt("randomize") ? true : false);
        entry->randomizeProbability = result[i].GetFloat("randomize_probability");

        if(!entry->item && item_id != 0)
        {
            Error2("Could not find specified loot item stat: %d", item_id);
            delete entry;
            continue;
        }

        currset->AddLootEntry(entry);
    }
}

#if 0
bool SpawnManager::LoadWaypointsAsSpawnRanges()
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    const char* xmlfile = "/planeshift/data/npcdefs.xml";

    csRef<iDataBuffer> buff = vfs->ReadFile(xmlfile);

    if(!buff || !buff->GetSize())
    {
        return false;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error    = doc->Parse(buff);
    if(error)
    {
        Error3("Error %s in %s", error, xmlfile);
        return false;
    }

    iDocumentNode* topNode = root->GetNode("waypoints");
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        if(!strcmp(node->GetValue(), "waypointlist"))
        {
            csRef<iDocumentNodeIterator> iter = node->GetNodes();
            while(iter->HasNext())
            {
                csRef<iDocumentNode> node = iter->Next();
                if(node->GetType() != CS_NODE_ELEMENT)
                    continue;
                if(!strcmp(node->GetValue(),"waypoint"))
                {
                    Waypoint* wp = new Waypoint;
                    if(wp->Load(node))
                    {
                        waypoints.Insert(wp,true);
                    }
                    else
                    {
                        CPrintf(CON_ERROR, "Waypoint node had no name specified!\n");
                        delete wp;
                        return false;
                    }
                }
            }
        }
        else if(!strcmp(node->GetValue(), "waylinks"))
        {
            csRef<iDocumentNodeIterator> iter = node->GetNodes();
            while(iter->HasNext())
            {
                csRef<iDocumentNode> node = iter->Next();
                if(node->GetType() != CS_NODE_ELEMENT)
                    continue;
                if(!strcmp(node->GetValue(),"point"))
                {
                    Waypoint key;
                    key.name = node->GetAttributeValue("name");
                    Waypoint* wp = waypoints.Find(&key);
                    if(!wp)
                    {
                        CPrintf(CON_ERROR, "Waypoint called '%s' not defined.\n",key.name.GetData());
                        return false;
                    }
                    else
                    {
                        csRef<iDocumentNodeIterator> iter = node->GetNodes();
                        while(iter->HasNext())
                        {
                            csRef<iDocumentNode> node = iter->Next();
                            if(node->GetType() != CS_NODE_ELEMENT)
                                continue;
                            if(!strcmp(node->GetValue(),"link"))
                            {
                                Waypoint key;
                                key.name = node->GetAttributeValue("name");
                                Waypoint* wlink = waypoints.Find(&key);
                                if(!wlink)
                                {
                                    CPrintf(CON_ERROR, "Waypoint called '%s' not defined.\n",key.name.GetData());
                                    return false;
                                }
                                else
                                {
                                    bool one_way = node->GetAttributeValueAsBool("oneway");
                                    wp->links.Push(wlink);
                                    if(!one_way)
                                        wlink->links.Push(wp);  // bi-directional link is implied
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}
#endif

void SpawnManager::LoadHuntLocations(psSectorInfo* sectorinfo)
{
    csString query;
    csString query_natres;

    query = "SELECT h.*,i.name, s.name sectorname FROM sectors s, hunt_locations h JOIN item_stats i ON i.id = h.itemid WHERE s.id=h.sector";
    query_natres = "SELECT s.name sectorname,loc_x x, loc_y y, loc_z z, item_id_reward itemid, \
            'interval', max_random, n.id, amount, radius 'range', i.name, 0  lock_str, -1 lock_skill, '' flags \
            FROM sectors s, natural_resources n JOIN item_stats i ON i.id = n.item_id_reward \
            WHERE s.id=n.loc_sector_id and amount is not NULL";

    if(sectorinfo)
    {
        query.AppendFmt(" and sector='%s'",sectorinfo->name.GetData());
        query_natres.AppendFmt(" and n.loc_sector_id='%s'",sectorinfo->name.GetData());
    }

    Result result(db->Select(query));

    if(!result.IsValid())
    {
        Error2("Could not load hunt_locations due to database error: %s", db->GetLastError());
        return;
    }

    // Spawn hunt locations
    SpawnHuntLocations(result, sectorinfo);

    Result result_natres(db->Select(query_natres));
    if(!result_natres.IsValid())
    {
        Error2("Could not load hunt_locations from natural_resources table due to database error: %s\n", db->GetLastError());
        return;
    }

    // Spawn hunt locations from natural res table
    SpawnHuntLocations(result_natres, sectorinfo);

}

void SpawnManager::SpawnHuntLocations(Result &result, psSectorInfo* sectorinfo)
{
    for(unsigned int i=0; i<result.Count(); i++)
    {
        // Get some vars to work with
        csString sector = result[i]["sectorname"];
        csVector3 pos(result[i].GetFloat("x"),result[i].GetFloat("y"),result[i].GetFloat("z"));
        uint32 itemid = result[i].GetUInt32("itemid");
        int interval = result[i].GetInt("interval");
        int max_rnd = result[i].GetInt("max_random");
        int id      = result[i].GetInt("id");
        int amount = result[i].GetInt("amount");
        float range = result[i].GetFloat("range");
        csString name = result[i]["name"];
        int lock_str = result[i].GetInt("lock_str");
        int lock_skill = result[i].GetInt("lock_skill");
        csString flags = result[i]["flags"];

        Debug4(LOG_SPAWN,0,"Adding hunt location in sector %s %s %s.\n",sector.GetData(),toString(pos).GetData(),name.GetData());

        // Schedule the item spawn
        psSectorInfo* spawnsector=cacheManager->GetSectorInfoByName(sector);

        if(spawnsector==NULL)
        {
            Error2("hunt_location failed to load, wrong sector: %s", sector.GetData());
            continue;
        }

        iSector* iSec = entityManager->FindSector(sector.GetData());
        if(!iSec)
        {
            Error2("Sector '%s' failed to be found when loading hunt location.", sector.GetData());
            continue;
        }

        csArray<gemObject*> nearlist;
        size_t handledSpawnsCount = 0;

        // Look for nearby items to prevent rescheduling of existing items
        // XXX what about spawning in different instances of map?
        nearlist = gem->FindNearbyEntities(iSec, pos, 0, range);
        size_t nearbyItemsCount = nearlist.GetSize();

        for(size_t i = 0; i < nearbyItemsCount; ++i)
        {
            psItem* item = nearlist[i]->GetItem();
            if(item)
            {
                if(name == item->GetName())  // Correct item?
                {
                    psScheduledItem* schedule = new psScheduledItem(id,itemid,pos,spawnsector,0,interval,max_rnd,range, lock_str, lock_skill, flags);
                    item->SetScheduledItem(schedule);
                    ++handledSpawnsCount;
                }
            }

            if((int) handledSpawnsCount == amount)  // All schedules accounted for
                break;
        }

        for(int i = 0; i < (amount - (int) handledSpawnsCount); ++i)  //Make desired amount of items that are not already existing
        {
            // This object won't get destroyed in a while (until something stops it or psItem is destroyed without moving)
            psScheduledItem* item = new psScheduledItem(id,itemid,pos,spawnsector,0,interval,max_rnd,range, lock_str, lock_skill, flags);

            // Queue it
            psItemSpawnEvent* newevent = new psItemSpawnEvent(item);
            psserver->GetEventManager()->Push(newevent);
        }
    }
}

void SpawnManager::LoadSpawnRanges(SpawnRule* rule)
{
    Result result(db->Select("select npc_spawn_ranges.id,x1,y1,z1,x2,y2,z2,radius,name,range_type_code"
                             "  from npc_spawn_ranges, sectors "
                             "  where npc_spawn_ranges.sector_id = sectors.id"
                             "  and npc_spawn_rule_id=%d", rule->GetID()));

    if(!result.IsValid())
    {
        Error2("Could not load NPC spawn ranges due to database error: %s",
               db->GetLastError());
        return;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        SpawnRange* r = new SpawnRange;

        r->Initialize(result[i].GetInt("id"),
                      rule->GetID(),
                      result[i]["range_type_code"],
                      result[i].GetFloat("x1"),
                      result[i].GetFloat("y1"),
                      result[i].GetFloat("z1"),
                      result[i].GetFloat("x2"),
                      result[i].GetFloat("y2"),
                      result[i].GetFloat("z2"),
                      result[i].GetFloat("radius"),
                      result[i]["name"]);


        rule->AddRange(r);
    }
}

void SpawnManager::PreloadDatabase()
{
    PreloadLootRules();

    Result result(db->Select("select id,min_spawn_time,max_spawn_time,"
                             "       substitute_spawn_odds,substitute_player,"
                             "       fixed_spawn_x,fixed_spawn_y,fixed_spawn_z,"
                             "       fixed_spawn_rot,fixed_spawn_sector,loot_category_id,"
                             "       dead_remain_time,fixed_spawn_instance,min_spawn_spacing_dist"
                             "  from npc_spawn_rules"));
    if(!result.IsValid())
    {
        Error2("Could not load NPC spawn rules due to database error: %s",
               db->GetLastError());
        return;
    }

    for(unsigned int i=0; i<result.Count(); i++)
    {
        LootEntrySet* loot_set = looting.Get(result[i].GetInt("loot_category_id"), NULL);

        SpawnRule* newrule = new SpawnRule;

        newrule->Initialize(result[i].GetInt("id"),
                            result[i].GetInt("min_spawn_time"),
                            result[i].GetInt("max_spawn_time"),
                            result[i].GetFloat("substitute_spawn_odds"),
                            result[i].GetInt("substitute_player"),
                            result[i].GetFloat("fixed_spawn_x"),
                            result[i].GetFloat("fixed_spawn_y"),
                            result[i].GetFloat("fixed_spawn_z"),
                            result[i].GetFloat("fixed_spawn_rot"),
                            result[i]["fixed_spawn_sector"],
                            loot_set,
                            result[i].GetInt("dead_remain_time"),
                            result[i].GetFloat("min_spawn_spacing_dist"),
                            result[i].GetUInt32("fixed_spawn_instance"));

        LoadSpawnRanges(newrule);

        rules.Put(newrule->GetID(), newrule);
    }
}

void SpawnManager::RepopulateLive(psSectorInfo* sectorinfo)
{
    psCharacter** chardatalist = NULL;
    int count;

    chardatalist = psServer::CharacterLoader.LoadAllNPCCharacterData(sectorinfo,count);
    if(chardatalist==NULL)
    {
        Error1("No NPCs found to repopulate.");
        return;
    }

    for(int i=0; i<count; i++)
    {
        if(chardatalist[i] != NULL)
        {
            entityManager->CreateNPC(chardatalist[i]);
        }
        else
        {
            Error1("Failed to repopulate NPC!");
        }
    }

    delete[] chardatalist;
}

void SpawnManager::RepopulateItems(psSectorInfo* sectorinfo)
{
    csArray<psItem*> items;

    // Load list from database
    if(!cacheManager->LoadWorldItems(sectorinfo, items))
    {
        Error1("Failed to load world items.");
        return;
    }

    // Now create entities and meshes, etc. for each one
    int spawned = 0;
    for(size_t i = 0; i < items.GetSize(); i++)
    {
        psItem* item = items[i];
        CS_ASSERT(item);
        // load items not in containers
        if(item->GetContainerID() == 0)
        {
            //if create item returns false, then no spawn occurs
            if(entityManager->CreateItem(item, (item->GetFlags() & PSITEM_FLAG_TRANSIENT) ? true : false))
            {
                // printf("Created item %d: %s\n", item->GetUID(), item->GetName() );
                // item->Save(false);
                item->SetLoaded();
                spawned++;
            }
            else
            {
                printf("Creating item '%s' (%i) failed.\n", item->GetName(), item->GetUID());
                delete item; // note that the dead item is still in the array
            }
        }
        // load items in containers
        else if(item->GetContainerID())
        {
            gemItem* citem = entityManager->GetGEM()->FindItemEntity(item->GetContainerID());
            gemContainer* container = dynamic_cast<gemContainer*>(citem);
            if(container)
            {
                if(!container->AddToContainer(item,NULL,item->GetLocInParent()))
                {
                    Error2("Cannot add item into container slot %i.\n", item->GetLocInParent());
                    delete item;
                }
                item->SetLoaded();
            }
            else
            {
                Error3("Container with id %d not found, specified in item %d.",
                       item->GetContainerID(),
                       item->GetUID());
                delete item;
            }
        }
    }

    Debug2(LOG_SPAWN,0,"Spawned %d items.\n",spawned);
}

void SpawnManager::KillNPC(gemActor* obj, gemActor* killer)
{
    int killer_cnum = 0;

    if(killer)
    {
        killer_cnum = killer->GetClientID();
        Debug3(LOG_SPAWN, 0, "Killer '%s' killed '%s'", killer->GetName(), obj->GetName());
    }
    else
    {
        Debug2(LOG_SPAWN, 0, "Killed NPC:%s", obj->GetName());
    }


    if(!obj->IsAlive())
        return;

    obj->SetAlive(false);

    // Create his loot
    SpawnRule* respawn = NULL;
    int spawnruleid = obj->GetCharacterData()->NPC_GetSpawnRuleID();
    if(spawnruleid)
    {
        respawn = rules.Get(spawnruleid, NULL);
    }

    if(respawn)
    {
        if(respawn->GetLootRules())
        {
            respawn->GetLootRules()->CreateLoot(obj->GetCharacterData());
        }
    }

    int loot_id = obj->GetCharacterData()->GetLootCategory();
    if(loot_id)  // custom loot for this mob also
    {
        LootEntrySet* loot = looting.Get(loot_id,0);
        if(loot)
        {
            Debug2(LOG_LOOT, 0, "Creating loot %d.", loot_id);
            loot->CreateLoot(obj->GetCharacterData());
        }
        else
        {
            Error3("Missing specified loot rule %d in character %s.",loot_id,obj->GetName());
        }
    }

    if(killer)
    {
        csRef<PlayerGroup> grp = killer->GetGroup();
        if(!grp)
        {
            // Check if the killer is owned (pet/familiar)
            gemActor* owner = dynamic_cast<gemActor*>(killer->GetOwner());
            if(owner)
            {
                // Is the owner part of a group? The group code below will add the
                // group and the owner.
                grp = owner->GetGroup();
                if(!grp)
                {
                    // Not part of group so add the owner
                    Debug3(LOG_LOOT, killer_cnum, "Adding owner with pid: %u as able to loot %s.",
                           owner->GetPID().Unbox(), obj->GetName());
                    obj->AddLootablePlayer(owner->GetPID());
                }
                else
                {
                    Debug2(LOG_LOOT, killer_cnum, "Adding from owners %s group.", owner->GetName());
                }
            }
        }

        if(grp)
        {
            for(size_t i=0; i<grp->GetMemberCount(); i++)
            {
                if(grp->GetMember(i)->RangeTo(obj) < RANGE_TO_RECV_LOOT)
                {
                    Debug3(LOG_LOOT, 0, "Adding %s as able to loot %s.", grp->GetMember(i)->GetName(), obj->GetName());
                    obj->AddLootablePlayer(grp->GetMember(i)->GetPID());
                }
                else
                {
                    Debug3(LOG_LOOT, 0, "Not adding %s as able to loot %s, because out of range or because he didn't attack the entity.", grp->GetMember(i)->GetName(), obj->GetName());
                }
            }
        }
        else
        {
            Debug3(LOG_LOOT, killer_cnum, "Adding player with pid: %u as able to loot %s.", killer->GetPID().Unbox(), obj->GetName());
            obj->AddLootablePlayer(killer->GetPID());
        }

    }

    // Notify the OwnerSession
    if(obj->GetCharacterData()->IsPet())
    {
        psserver->GetNPCManager()->PetHasBeenKilled(obj->GetNPCPtr());
    }


    // Set timer for when NPC will disappear
    csTicks delay = (respawn)?respawn->GetDeadRemainTime():5000;
    psDespawnGameEvent* newevent = new psDespawnGameEvent(this, gem, delay,obj);
    psserver->GetEventManager()->Push(newevent);

    Notify3(LOG_SPAWN,"Scheduled NPC %s to be removed in %1.1f seconds.",obj->GetName(),(float)delay/1000.0);
}

void SpawnManager::RemoveNPC(gemObject* obj)
{
    Debug2(LOG_SPAWN,0,"RemoveNPC:%s\n",obj->GetName());

    ServerStatus::mob_deathcount++;

    PID pid = obj->GetPID();

    Notify3(LOG_SPAWN, "Sending NPC %s disconnect msg to %zu clients.\n", ShowID(obj->GetEID()), obj->GetMulticastClients().GetSize());

    if(obj->GetCharacterData()==NULL)
    {
        Error2("Character data for npc character %s was not found! Entity stays dead.", ShowID(pid));
        return;
    }

    SpawnRule* respawn = NULL;
    int spawnruleid = obj->GetCharacterData()->NPC_GetSpawnRuleID();

    if(spawnruleid)
    {
        // Queue for respawn according to rules
        respawn = rules.Get(spawnruleid, NULL);
    }

    if(!respawn)
    {
        if(spawnruleid == 0)  // spawnruleid 0 is for non-respawning NPCs
        {
            Notify2(LOG_SPAWN,"Temporary NPC based on player ID %s has died. Entity stays dead.", ShowID(pid));
        }
        else
        {
            Error3("Respawn rule for player %s, rule %d was not found! Entity stays dead.", ShowID(pid), spawnruleid);
        }

        // Remove mesh, etc from engine
        entityManager->RemoveActor(obj);
        return;
    }

    // Remove mesh, etc from engine
    entityManager->RemoveActor(obj);

    int delay = respawn->GetRespawnDelay();
    PID newplayer = respawn->CheckSubstitution(pid);

    psRespawnGameEvent* newevent = new psRespawnGameEvent(this, delay, newplayer, respawn);
    psserver->GetEventManager()->Push(newevent);

    Notify3(LOG_SPAWN, "Scheduled NPC %s to be respawned in %.1f seconds", ShowID(newplayer), (float)delay/1000.0);
}


#define SPAWN_POINT_TAKEN 999
#define SPAWN_BASE_ITEM   1000

void SpawnManager::Respawn(PID playerID, SpawnRule* spawnRule)
{
    psCharacter* chardata=psServer::CharacterLoader.LoadCharacterData(playerID,false);
    if(chardata==NULL)
    {
        Error2("Character %s to be respawned does not have character data to be loaded!", ShowID(playerID));
        return;
    }

    csVector3 pos;
    float angle;
    csString sectorName;
    iSector* sector = NULL;
    InstanceID instance;

    int count = 4; // For random positions we try 4 times before going with the result.

    // DetermineSpawnLoc will return true if it is a random picked position so for fixed we will fall true
    // on the first try.
    while(spawnRule->DetermineSpawnLoc(chardata, pos, angle, sectorName, instance) && spawnRule->GetMinSpawnSpacingDistance() > 0.0 && count-- > 0)
    {
        sector = psserver->entitymanager->GetEngine()->GetSectors()->FindByName(sectorName);

        CS_ASSERT(sector);

        csArray<gemObject*> nearlist = psserver->entitymanager->GetGEM()->FindNearbyEntities(sector, pos, spawnRule->GetMinSpawnSpacingDistance(), false);
        if(nearlist.IsEmpty())
        {
            break; // Nothing in the neare list so spawn position is ok.
        }
        Debug4(LOG_SPAWN,0,"Spawn position %s is occuplied by %zu entities. %s",
               toString(pos,sector).GetDataSafe(),
               nearlist.GetSize(),count>0?" Will Retry":"Last try");

    }

    Debug1(LOG_SPAWN,0,"Position accepted");
    Respawn(chardata, instance, pos, angle, sectorName);
}


void SpawnManager::Respawn(psCharacter* chardata, InstanceID instance, csVector3 &where, float rot, const char* sector)
{
    psSectorInfo* spawnsector = cacheManager->GetSectorInfoByName(sector);
    if(spawnsector==NULL)
    {
        Error2("Spawn message indicated unresolvable sector '%s'", sector);
        return;
    }

    chardata->SetLocationInWorld(instance, spawnsector, where.x, where.y, where.z, rot);
    chardata->GetHPRate().SetBase(HP_REGEN_RATE);
    chardata->GetManaRate().SetBase(MANA_REGEN_RATE);

    //Restore the vitals of this npc before respawning
    chardata->ResetStats();

    // Here we restore status of items to max quality as this is going to be a *newly born* npc
    chardata->Inventory().RestoreAllInventoryQuality();

    // Now create the NPC as usual
    entityManager->CreateNPC(chardata);

    ServerStatus::mob_birthcount++;
}

void handleGroupLootItem(psItem* item, gemActor* target, Client* client, CacheManager* cacheManager, GEMSupervisor* gem, uint8_t lootAction)
{
    if(!item)
    {
        // Take this out because it is just the result of duplicate loot commands due to lag
        //Warning3(LOG_COMBAT,"LootItem Message from %s specified bad item id of %d.",client->GetName(), msg.lootitem);
        return;
    }

    item->SetLoaded();

    csRef<PlayerGroup> group = client->GetActor()->GetGroup();
    Client* randfriendclient = NULL;
    if(group.IsValid())
    {
        randfriendclient = target->GetRandomLootClient(RANGE_TO_LOOT * 10);
        if(!randfriendclient)
        {
            Error3("GetRandomLootClient failed for loot msg from %s, object %s.", client->GetName(), item->GetName());
            return;
        }
    }

    csString type;
    Client* looterclient;  // Client that gets the item
    if(lootAction == psLootItemMessage::LOOT_SELF || !group.IsValid())
    {
        looterclient = client;
        type = "Loot Self";
    }
    else
    {
        looterclient = randfriendclient;
        type = "Loot Roll";
    }

    // Ask group member before take
    if((lootAction == psLootItemMessage::LOOT_SELF) && group.IsValid() &&
            (client != randfriendclient))
    {
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "Asking roll winner %s if you may take the %s...",
                                 randfriendclient->GetName(), item->GetName());
        csString request;
        request.Format("You have won the roll for a %s, but %s wants to take it."
                       "  Will you allow this action?",
                       item->GetName(), client->GetName());

        // Item will be held in the prompt until answered.

        PendingLootPrompt* p = new PendingLootPrompt(client, randfriendclient, item, target->GetCharacterData(), request, cacheManager, gem);
        psserver->questionmanager->SendQuestion(p);

        type.Append(" Pending");
    }
    // Continue with normal looting if not prompting
    else
    {
        // Create the loot message
        csString lootmsg;
        if(group.IsValid())
            lootmsg.Format("%s won the roll and", looterclient->GetName());
        else
            lootmsg.Format("You");
        lootmsg.AppendFmt(" looted a %s",item->GetName());

        // Attempt to give to looter
        bool dropped = looterclient->GetActor()->GetCharacterData()->Inventory().AddOrDrop(item);

        if(!dropped)
        {
            lootmsg.Append(", but can't hold anymore");
            type.Append(" (dropped)");
        }

        // Send out the loot message
        psSystemMessage loot(client->GetClientNum(), MSG_LOOT, lootmsg.GetData());
        looterclient->GetActor()->SendGroupMessage(loot.msg);

        item->Save(false);
    }

    // Trigger item removal on every client in the group which has intrest
    if(group.IsValid())
    {
        for(int i = 0; i < (int)group->GetMemberCount(); i++)
        {
            PID playerID = group->GetMember(i)->GetPID();
            if(target->IsLootablePlayer(playerID))
            {
                psLootRemoveMessage rem(group->GetMember(i)->GetClientID(), item->GetBaseStats()->GetUID());
                rem.SendMessage();
            }
        }
    }
    else
    {
        psLootRemoveMessage rem(client->GetClientNum(), item->GetBaseStats()->GetUID());
        rem.SendMessage();
    }

    psLootEvent evt(
        target->GetCharacterData()->GetPID(),
        target->GetCharacterData()->GetCharName(),
        looterclient->GetCharacterData()->GetPID(),
        looterclient->GetCharacterData()->GetCharName(),
        item->GetUID(),
        item->GetName(),
        item->GetStackCount(),
        (int)item->GetCurrentStats()->GetQuality(),
        0
    );
    evt.FireEvent();
}

void SpawnManager::HandleLootItem(MsgEntry* me,Client* client)
{
    psLootItemMessage msg(me);

    gemActor* target = dynamic_cast<gemActor*>(gem->FindObject(msg.entity));
    if(!target)
    {
        Error3("LootItem Message from %s specified an erroneous entity id: %s.", client->GetName(), ShowID(msg.entity));
        return;
    }
    psCharacter* targetCharacter = target->GetCharacterData();
    if(!targetCharacter)
    {
        Error3("LootItem Message from %s specified a non-character entity id: %s.", client->GetName(), ShowID(msg.entity));
        return;
    }

    // Check the range to the lootable object.
    if(client->GetActor()->RangeTo(target) > RANGE_TO_LOOT)
    {
        psserver->SendSystemError(client->GetClientNum(), "Too far away to loot %s.", target->GetName());
        return;
    }

    psItem* item = targetCharacter->RemoveLootItem(msg.lootitem);

    handleGroupLootItem(item, target, client, cacheManager, gem, msg.lootaction);
}

void SpawnManager::HandleDeathEvent(MsgEntry* me,Client* notused)
{
    Debug1(LOG_SPAWN,0, "Spawn Manager handling Death Event\n");
    psDeathEvent death(me);

    death.deadActor->GetCharacterData()->KilledBy(death.killer ? death.killer->GetCharacterData() : NULL);
    if(death.killer)
    {
        death.killer->GetCharacterData()->Kills(death.deadActor->GetCharacterData());
    }

    // Respawning is handled with ResurrectEvents for players and by SpawnManager for NPCs
    if(death.deadActor->GetClientID())      // Handle Human Player dying
    {
        ServerStatus::player_deathcount++;
        psResurrectEvent* event = new psResurrectEvent(0,20000,death.deadActor);
        psserver->GetEventManager()->Push(event);
        Debug2(LOG_COMBAT, death.deadActor->GetClientID(), "Queued resurrect event for %s.\n",death.deadActor->GetName());
    }
    else  // Handle NPC dying
    {
        Debug1(LOG_SPAWN, 0,"Killing npc in spawnmanager.\n");
        // Remove NPC and queue for respawn
        KillNPC(death.deadActor, death.killer);
    }

    // Allow Actor to notify listeners of death
    death.deadActor->HandleDeath();
}

/*----------------------------------------------------------------*/

SpawnRule::SpawnRule()
{
    randomgen = psserver->rng;
    id = minspawntime = maxspawntime = 0;
    substitutespawnodds = 0;
    substituteplayer    = 0;
    fixedspawnx = fixedspawny = fixedspawnz = fixedspawnrot = 0;
}
SpawnRule::~SpawnRule()
{
    csHash<SpawnRange*>::GlobalIterator rangeIt(ranges.GetIterator());
    while(rangeIt.HasNext())
        delete rangeIt.Next();
}

void SpawnRule::Initialize(int idval,
                           int minspawn,
                           int maxspawn,
                           float substodds,
                           int substplayer,
                           float x,float y,float z,float angle,
                           const char* sector,
                           LootEntrySet* loot_id,
                           int dead_time,
                           float minSpacing,
                           InstanceID instance)
{
    id = idval;
    minspawntime = minspawn;
    maxspawntime = maxspawn;
    substitutespawnodds = substodds;
    substituteplayer    = substplayer;
    fixedspawnx = x;
    fixedspawny = y;
    fixedspawnz = z;
    fixedspawnrot = angle;
    fixedspawnsector = sector;
    loot = loot_id;
    dead_remain_time = dead_time;
    fixedinstance = instance;
    minSpawnSpacingDistance = minSpacing;
}


int SpawnRule::GetRespawnDelay()
{
    int ticks = maxspawntime-minspawntime;

    return minspawntime + randomgen->Get(ticks);
}


PID SpawnRule::CheckSubstitution(PID originalplayer)
{
    int score = randomgen->Get(10000);

    if(score < 10000.0*substitutespawnodds)
        return substituteplayer;
    else
        return originalplayer;
}

bool SpawnRule::DetermineSpawnLoc(psCharacter* ch, csVector3 &pos, float &angle, csString &sectorname, InstanceID &instance)
{
    // ignore fixed point if there are ranges in this rule

    size_t rcount = ranges.GetSize();

    if(rcount > 0)
    {
        // Got ranges
        // Pick a location using uniform probability:
        // 1. Pick a range with probability proportional to area
        // 2. Pick a point in that range

        csHash<SpawnRange*>::GlobalIterator rangeit(ranges.GetIterator());

        // Compute total area
        float totalarea = 0;
        SpawnRange* range;
        while(rangeit.HasNext())
        {
            range = rangeit.Next();
            totalarea += range->GetArea();
        }
        // aimed area level
        float aimed = randomgen->Get() * totalarea;
        // cumulative area level
        float cumul = 0;
        bool  found = false;
        csHash<SpawnRange*>::GlobalIterator rangeit2(ranges.GetIterator());
        while(rangeit2.HasNext())
        {
            range = rangeit2.Next();
            cumul += range->GetArea();
            if(cumul >= aimed)
            {
                // got it!
                pos = range->PickPos();
                sectorname = range->GetSector();
                if(ch && sectorname == "startlocation")
                {
                    sectorname = ch->GetSpawnLocation().loc_sector->name;
                }
                found = true;
                break;
            }
        }
        if(!found)
        {
            Error1("Failed to find spawn position");
        }


        // randomly choose an angle in [0, 2*PI]
        angle = randomgen->Get() * TWO_PI;
        instance = fixedinstance;

        return true; // This is a random position pick
    }
    else if(ch && fixedspawnsector == "startlocation")
    {
        pos = ch->GetSpawnLocation().loc;
        angle = ch->GetSpawnLocation().loc_yrot;
        sectorname = ch->GetSpawnLocation().loc_sector->name;
        instance = ch->GetSpawnLocation().worldInstance;
        return false; // This is a static position fix.
    }
    else
    {
        // Use fixed spawn point
        pos.x = fixedspawnx;
        pos.y = fixedspawny;
        pos.z = fixedspawnz;
        angle = fixedspawnrot;
        sectorname = fixedspawnsector;
        instance = fixedinstance;
        return false; // This is a static position fix.
    }
}

void SpawnRule::AddRange(SpawnRange* range)
{
    ranges.Put(range->GetID(), range);
}

/*----------------------------------------------------------------*/

SpawnRange::SpawnRange()
{
    id = npcspawnruleid = 0;
    area = 0;
    randomgen = psserver->rng;
}

#define RANGE_FICTITIOUS_WIDTH .5

void SpawnRange::Initialize(int idval,
                            int spawnruleid,
                            const char* type_code,
                            float rx1, float ry1, float rz1,
                            float rx2, float ry2, float rz2,
                            float radiusval,
                            const char* sectorname)
{
    id = idval;
    npcspawnruleid = spawnruleid;
    type = *type_code;

    x1 = rx1;
    x2 = rx2;
    y1 = ry1;
    y2 = ry2;
    z1 = rz1;
    z2 = rz2;

    spawnsector = sectorname;
    float dx = x1 == x2 ? RANGE_FICTITIOUS_WIDTH : x2 - x1;
    float dz = z1 == z2 ? RANGE_FICTITIOUS_WIDTH : z2 - z1;
    radius = radiusval;

    if(type == 'A')
    {
        // Just ignore if one of the deltas are negative. PickPos works anyway.
        area = fabs(dx * dz);
    }
    else if(type=='L')
    {
        float length = sqrt((rx2-rx1)*(rx2-rx1) + (ry2-ry1)*(ry2-ry1) + (rz2-rz1)*(rz2-rz1));
        if(radius > 0.5)
            area = length*radius*2 + radius * radius * PI;  // Area of length and round area at end.
        else
            area = length;
    }
    else if(type == 'C')
    {
        area = radius * radius * PI;
    }

}

csVector3 SpawnRange::PickWithRadius(csVector3 pos, float radius)
{
    if(radius == 0.0f)
    {
        return pos;
    }
    float x;
    float z;

    float xDist;
    float zDist;

    do
    {
        // Pick random point in circumscribed rectangle.
        x = randomgen->Get() * (radius*2.0);
        z = randomgen->Get() * (radius*2.0);
        xDist = radius - x;
        zDist = radius - z;
        // Keep looping until the point is inside a circle.
    }
    while(xDist * xDist + zDist * zDist > radius * radius);

    return csVector3(pos.x - radius + x, pos.y, pos.z - radius + z);
}

const csVector3 SpawnRange::PickPos()
{
    if(type == 'A')
    {
        return csVector3(x1 + randomgen->Get() * (x2 - x1),
                         y1 + randomgen->Get() * (y2 - y1),
                         z1 + randomgen->Get() * (z2 - z1));
    }
    else if(type == 'L') // type 'L' means spawn along line segment with an offset determined by radius (Line swept circle)
    {
        float d = randomgen->Get();

        csVector3 pos = csVector3(x1 + d * (x2 - x1),
                                  y1 + d * (y2 - y1),
                                  z1 + d * (z2 - z1));

        return PickWithRadius(pos, radius);
    }
    else if(type == 'C')  // type 'C' means spawn within a circle centered at the first set of co-ordinates
    {
        return PickWithRadius(csVector3(x1, y1, z1), radius);
    }
    else
    {
        Error2("Unknown spawn range %c!", type);
    }
    return csVector3(0.0, 0.0, 0.0);
}


/*----------------------------------------------------------------*/


psRespawnGameEvent::psRespawnGameEvent(SpawnManager* mgr,
                                       int delayticks,
                                       PID playerID,
                                       SpawnRule* spawnRule)
    : psGameEvent(0,delayticks,"psRespawnGameEvent"),
      spawnmanager(mgr), playerID(playerID), spawnRule(spawnRule)
{
}


void psRespawnGameEvent::Trigger()
{
    spawnmanager->Respawn(playerID ,spawnRule);
}


psDespawnGameEvent::psDespawnGameEvent(SpawnManager* mgr,
                                       GEMSupervisor* gemsupervisor,
                                       int delayticks,
                                       gemObject* obj)
    : psGameEvent(0,delayticks,"psDespawnGameEvent")
{
    spawnmanager = mgr;
    gem = gemsupervisor;
    entity       = obj->GetEID();
}

void psDespawnGameEvent::Trigger()
{
    gemObject* object = gem->FindObject(entity);
    if(!object)
    {
        csString status;
        status.Format("Despawn event triggered for non-existant NPC %s!", ShowID(entity));
        psserver->GetLogCSV()->Write(CSV_STATUS, status);

        Error2("%s", status.GetData());
        return;
    }
    spawnmanager->RemoveNPC(object);
}


LootEntrySet::~LootEntrySet()
{
    LootEntry* e;
    while(entries.GetSize())
    {
        e = entries.Pop();
        delete e;
    }
    lootRandomizer = NULL;
}

void LootEntrySet::AddLootEntry(LootEntry* entry)
{
    entries.Push(entry);
    total_prob += entry->probability;
}

void LootEntrySet::CreateLoot(psCharacter* chr, size_t numModifiers)
{
    // the idea behind this code is that if you have a total probability that's <1
    // then the items listed are mutually exclusive. If you have a total >1 then the
    // server rolls the probability on each item. This disables the possibility to have
    // items with low chances (0.1) that are not mutually exclusive.
    // We have to change this by using an additional field that identifies the item
    // as mutually exclusive. So for now this is commented.
    //if (total_prob < 1+EPSILON)
    //    CreateSingleLoot( chr );
    //else
    CreateMultipleLoot(chr, numModifiers);
}

void LootEntrySet::CreateSingleLoot(psCharacter* chr)
{
    float roll = psserver->rng->Get();
    float prob_so_far = 0;
    float maxcost = lootRandomizer->CalcModifierCostCap(chr);

    for(size_t i=0; i<entries.GetSize(); i++)
    {
        if(prob_so_far < roll && roll < prob_so_far + entries[i]->probability)
        {
            if(entries[i]->item) // We don't always have a item.
            {
                psItem* loot_item = entries[i]->item->InstantiateBasicItem();
                Debug2(LOG_LOOT, 0,"Adding %s to the dead mob's loot.\n",loot_item->GetName());
                if(entries[i]->randomize) loot_item = lootRandomizer->RandomizeItem(loot_item, maxcost);
                chr->AddLootItem(loot_item);
            }

            float pct = psserver->rng->Get();
            int money = entries[i]->min_money + (int)(pct * (float)(entries[i]->max_money - entries[i]->min_money));
            chr->AddLootMoney(money);
            break;
        }
        prob_so_far += entries[i]->probability;
    }
}

void LootEntrySet::CreateMultipleLoot(psCharacter* chr, size_t numModifiers)
{
    float maxcost = 0.0;
    // if chr == NULL then its a randomized loot test; i.e. no character will receive it
    bool lootTesting = chr ? false : true;
    if(!lootTesting)
        maxcost = lootRandomizer->CalcModifierCostCap(chr);

    // cycle on all entries of our loot rule
    for(size_t i=0; i<entries.GetSize(); i++)
    {
        // check we roll successfully on the probability
        float roll = psserver->rng->Get();
        if(roll <= entries[i]->probability)
        {
            // If we have an item in the loot entry (We don't always have a item)
            if(entries[i]->item)
            {
                int itemAmount = entries[i]->min_item + (int)(psserver->rng->Get() *
                                 (float)(entries[i]->max_item - entries[i]->min_item));
                for(int y = 0; y < itemAmount; y++)
                {
                    // create the base item
                    psItem* loot_item = entries[i]->item->InstantiateBasicItem();

                    // if required, generate random modifiers on the item
                    if(entries[i]->randomize && psserver->rng->Get() <= entries[i]->randomizeProbability)
                        loot_item = lootRandomizer->RandomizeItem(loot_item,
                                    maxcost,
                                    lootTesting,
                                    numModifiers);

                    // add the item as loot
                    if(!lootTesting)
                        chr->AddLootItem(loot_item);

                    // if we are in testing mode, then just print out the result of the randomization
                    else
                    {
                        // print out the stats
                        CPrintf(CON_CMDOUTPUT,
                                "Randomized item (%d modifiers) : \'%s\', %s\n"
                                "  Quality : %.2f  Weight : %.2f  Size : %f  Price : %d\n",
                                numModifiers,
                                loot_item->GetName(),
                                loot_item->GetDescription(),
                                loot_item->GetItemQuality(),
                                loot_item->GetWeight(),
                                loot_item->GetItemSize(),
                                loot_item->GetPrice().GetTrias());
                        if(loot_item->GetIsArmor())
                        {
                            CPrintf(CON_CMDOUTPUT,
                                    "Armour stats:\n"
                                    "  Class : %c  Hardness : %.2f\n"
                                    "  Protection Slash : %.2f  Blunt : %.2f  Pierce : %.2f\n",
                                    loot_item->GetBaseStats()->Armor().Class(),
                                    loot_item->GetHardness(),
                                    loot_item->GetDamage(PSITEMSTATS_DAMAGETYPE_SLASH),
                                    loot_item->GetDamage(PSITEMSTATS_DAMAGETYPE_BLUNT),
                                    loot_item->GetDamage(PSITEMSTATS_DAMAGETYPE_PIERCE));
                        }
                        else if(loot_item->GetIsMeleeWeapon() || loot_item->GetIsRangeWeapon())
                        {
                            CPrintf(CON_CMDOUTPUT,
                                    "Weapon stats:\n"
                                    "  Latency : %.2f  Penetration : %.2f\n"
                                    "  Damage Slash : %.2f  Blunt : %.2f  Pierce : %.2f\n"
                                    "  BlockValue Untargeted : %.2f  Targeted : %.2f  Counter : %.2f\n",
                                    loot_item->GetLatency(),
                                    loot_item->GetPenetration(),
                                    loot_item->GetDamage(PSITEMSTATS_DAMAGETYPE_SLASH),
                                    loot_item->GetDamage(PSITEMSTATS_DAMAGETYPE_BLUNT),
                                    loot_item->GetDamage(PSITEMSTATS_DAMAGETYPE_PIERCE),
                                    loot_item->GetUntargetedBlockValue(),
                                    loot_item->GetTargetedBlockValue(),
                                    loot_item->GetCounterBlockValue());
                        }
                        /*CPrintf(CON_CMDOUTPUT,
                            "Equip script: %s\n Un-equip script: %s\n",loot_item->GetProgressionEventEquip().GetData(),
                            loot_item->GetProgressionEventUnEquip().GetData());*/
                    }
                }
            }

            // add money to the loot result if specified by the loot rule
            float pct = psserver->rng->Get();
            int money = entries[i]->min_money + (int)(pct * (float)(entries[i]->max_money - entries[i]->min_money));
            if(!lootTesting) chr->AddLootMoney(money);
        }
    }
}

psItemSpawnEvent::psItemSpawnEvent(psScheduledItem* item)
    : psGameEvent(0,item->MakeInterval(),"psItemSpawnEvent")
{
    if(item->WantToDie())
    {
        schedule = NULL;
        return;
    }

    schedule = item;
    Notify4(LOG_SPAWN,"Spawning item (%u) in %d , sector: %s",item->GetItemID(),triggerticks -csGetTicks(), item->GetSector()->ToString());
}

psItemSpawnEvent::~psItemSpawnEvent()
{
    delete schedule;
}


void psItemSpawnEvent::Trigger()
{
    if(schedule->WantToDie())
        return;

    if(schedule->CreateItem() == NULL)
    {
        CPrintf(CON_ERROR,"Couldn't spawn item %u",schedule->GetItemID());
    }
}
