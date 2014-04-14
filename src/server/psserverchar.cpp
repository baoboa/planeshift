/*
 * psserverchar.cpp
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
 * Communicates with the client side version of charmanager.
 */
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/databuff.h>
#include <iutil/object.h>
#include <csutil/csstring.h>
#include <csutil/list.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/serverconsole.h"
#include "util/psxmlparser.h"
#include "util/log.h"
#include "util/mathscript.h"
#include "util/psconst.h"
#include "util/eventmanager.h"
#include "util/psdatabase.h"

#include "rpgrules/factions.h"

#include "net/message.h"
#include "net/messages.h"
#include "net/msghandler.h"
#include "net/charmessages.h"

#include "bulkobjects/pscharacter.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/pstrade.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psmerchantinfo.h"
#include "bulkobjects/psactionlocationinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psserverchar.h"
#include "client.h"
#include "playergroup.h"
#include "clients.h"
#include "psserver.h"
#include "chatmanager.h"
#include "groupmanager.h"
#include "spellmanager.h"
#include "slotmanager.h"
#include "workmanager.h"
#include "netmanager.h"
#include "cachemanager.h"
#include "progressionmanager.h"
#include "creationmanager.h"
#include "exchangemanager.h"
#include "actionmanager.h"
#include "serverstatus.h"
#include "economymanager.h"
#include "weathermanager.h"
#include "globals.h"
#include "events.h"
#include "psproxlist.h"

///This expresses in seconds how many days the char hasn't logon. 60 days, at the moment.
#define MAX_DAYS_NO_LOGON 5184000

/// The number of characters per email account
#define CHARACTERS_ALLOWED 4

/// The max number of items which can be stored in one category at a banker NPC
#define MAX_ITEMS_IN_CATEGORY 200

ServerCharManager::ServerCharManager(CacheManager* cachemanager, GEMSupervisor* gemsupervisor)
{
    cacheManager = cachemanager;
    gemSupervisor = gemsupervisor;
    slotManager = NULL;

    MathScriptEngine* eng = psserver->GetMathScriptEngine();
    calc_item_merchant_price_buy = eng->FindScript("Calc Item Merchant Price Buy");
    calc_item_merchant_price_sell = eng->FindScript("Calc Item Merchant Price Sell");
}

ServerCharManager::~ServerCharManager()
{
    delete slotManager;
    slotManager = NULL;
}

bool ServerCharManager::Initialize()
{
//    clients = ccs;

    Subscribe(&ServerCharManager::HandleInventoryMessage, MSGTYPE_GUIINVENTORY, REQUIRE_READY_CLIENT);
    Subscribe(&ServerCharManager::HandleMerchantMessage, MSGTYPE_GUIMERCHANT, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&ServerCharManager::HandleStorageMessage, MSGTYPE_GUISTORAGE, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&ServerCharManager::ViewItem, MSGTYPE_VIEW_ITEM, REQUIRE_READY_CLIENT);
    Subscribe(&ServerCharManager::UpdateSketch, MSGTYPE_VIEW_SKETCH, REQUIRE_READY_CLIENT);
    Subscribe(&ServerCharManager::UpdateMusicalSheet, MSGTYPE_MUSICAL_SHEET, REQUIRE_READY_CLIENT);
    Subscribe(&ServerCharManager::HandleBookWrite, MSGTYPE_WRITE_BOOK, REQUIRE_READY_CLIENT);
    Subscribe(&ServerCharManager::HandleFaction, MSGTYPE_FACTION_INFO, REQUIRE_READY_CLIENT);

    slotManager = new SlotManager(gemSupervisor, cacheManager);
    if(!(slotManager && slotManager->Initialize()))
        return false;

    if(!calc_item_merchant_price_buy)
    {
        Error1("Failed to find MathScript >Calc Item Merchant Price Buy<!");
        return false;
    }
    if(!calc_item_merchant_price_sell)
    {
        Error1("Failed to find MathScript >Calc Item Merchant Price Sell<!");
        return false;
    }
    return true;
}

void ServerCharManager::ViewItem(MsgEntry* me, Client* client)
{
    psViewItemDescription mesg(me, NULL);
    ViewItem(client, mesg.containerID, (INVENTORY_SLOT_NUMBER) mesg.slotID);
}

void ServerCharManager::UpdateSketch(MsgEntry* me, Client* client)
{
    psSketchMessage sketchMsg(me);

    if(sketchMsg.valid)
    {
        psItem* item   = client->GetCharacterData()->Inventory().FindItemID(sketchMsg.ItemID);
        if(item)
        {
            csString currentTitle = item->GetName();
            if(sketchMsg.name.Length() > 0 && sketchMsg.name != currentTitle)
            {
                currentTitle = sketchMsg.name;
                item->SetName(sketchMsg.name);
                //update the inventory. fixes PS#4406. TODO: we really need a way to send only one item update
                //to clients inventory
                SendInventory(client->GetClientNum(),true);
            }

            // TODO: Probably need to validate the xml here somehow
            // for first sketcher, sets authorship.
            item->SetCreator(client->GetCharacterData()->GetPID(), PSITEMSTATS_CREATOR_VALID);
            if(item->SetSketch(csString(sketchMsg.Sketch)))
            {
                printf("Updated sketch for item %u to: %s\n", sketchMsg.ItemID, sketchMsg.Sketch.GetDataSafe());
                psserver->SendSystemInfo(me->clientnum, "Your drawing has been updated.");
                item->Save(false);
            }
        }
        else
        {
            Error3("Item %u not found in sketch definition message from client %s.",sketchMsg.ItemID, client->GetName());
        }
    }
}

void ServerCharManager::UpdateMusicalSheet(MsgEntry* me, Client* client)
{
    psMusicalSheetMessage musicMsg(me);

    if(musicMsg.valid && !musicMsg.play)
    {
        psItem* item = client->GetCharacterData()->Inventory().FindItemID(musicMsg.itemID);
        if(item != 0)
        {
            // saving
            csString currentTitle = item->GetName();
            if(musicMsg.songTitle.Length() > 0 && musicMsg.songTitle != currentTitle)
            {
                item->SetName(musicMsg.songTitle);
                //update the inventory. fixes PS#4406. TODO: we really need a way to send only one item update
                //to clients inventory
                SendInventory(client->GetClientNum(), true);
            }

            // TODO validate the xml score
            item->SetCreator(client->GetCharacterData()->GetPID(), PSITEMSTATS_CREATOR_VALID);
            if(item->SetMusicalSheet(csString(musicMsg.musicalSheet)))
            {
                psserver->SendSystemInfo(me->clientnum, "Your score has been updated.");
                item->Save(false);
            }
        }
        else
        {
            Error3("Item %u not found in musical sheet message from client %s.", musicMsg.itemID, client->GetName());
        }
    }
}

void ServerCharManager::ViewItem(Client* client, int containerID, INVENTORY_SLOT_NUMBER slotID)
{
    // printf("Viewing item in Container %d, slot %d.\n", containerID, slotID);

    psItem* item = slotManager->FindItem(client, containerID, slotID);

    if(item)
    {
        item->ViewItem(client, containerID, slotID);
    }
    else
    {
        psActionLocation* action = psserver->GetActionManager()->FindAction(containerID);
        if(!action)
        {
            //Error3("No item/action : %d, %d", containerID, slotID);
            return;
        }
        else
        {
            gemActionLocation* gemAction = action->GetGemObject();
            // Check range ignoring Y co-ordinate
            csWeakRef<gemObject> gem = client->GetActor();
            if(gem.IsValid() && gem->RangeTo(gemAction, true, true) > RANGE_TO_SELECT)
            {
                psserver->SendSystemError(client->GetClientNum(),
                                          "You are not in range to see %s.", gemAction->GetName());
                return;
            }

            // Check for container
            if(action->IsContainer())
            {
                // Get container instance
                if(!action->GetRealItem())
                {
                    uint32 InstanceID = action->GetInstanceID();
                    Error3("Invalid instance ID %u in action location %s", InstanceID, action->name.GetDataSafe());
                    return;
                }

                // Get item pointer
                item = action->GetRealItem()->GetItem();
                if(!item)
                {
                    CPrintf(CON_ERROR, "Invalid ItemID in Action Location Response.\n");
                    return;
                }
                item->SendActionContents(client, action);
            }

            // Check for minigames or entrances
            else if(action->IsGameBoard() || action->IsEntrance() || action->IsExamineScript())
            {
                csString description = action->GetDescription();
                if(!description.IsEmpty())
                {
                    psViewActionLocationMessage mesg(client->GetClientNum(), action->name, description.GetData());
                    mesg.SendMessage();
                }
                else
                {
                    Error2("Action location %s XML response does not have a valid description", action->name.GetData());
                    return;
                }
            }
            else
            {
                psViewActionLocationMessage mesg(client->GetClientNum(), action->name, action->response);
                mesg.SendMessage();
            }
        }
    }
}

void ServerCharManager::HandleBookWrite(MsgEntry* me, Client* client)
{
    psWriteBookMessage mesg(me);

    //if we're getting this, it's gotta be a request or a save
    if(mesg.messagetype == mesg.REQUEST)
    {
        psItem* item = slotManager->FindItem(client, mesg.containerID, (INVENTORY_SLOT_NUMBER) mesg.slotID);

        //is it a writable book?  In our inventory? Are we the author?
        if(item && item->GetIsWriteable() &&
                item->GetOwningCharacter() == client->GetCharacterData() &&
                item->IsThisTheCreator(client->GetCharacterData()->GetPID()))
        {
            //We could maybe let the work manager know that we're busy writing something
            //or track that this is the book we're working on, and only allow saves to a
            //book that was opened for writing.  This would be a good thing.
            //Also check for other writing in progress
            csString theText(item->GetBookText());
            csString theTitle(item->GetName());
            psWriteBookMessage resp(client->GetClientNum(), theTitle, theText, true, (INVENTORY_SLOT_NUMBER)mesg.slotID, mesg.containerID);
            resp.SendMessage();
            // this only does set the creator for first write to book
            item->SetCreator(client->GetCharacterData()->GetPID(), PSITEMSTATS_CREATOR_VALID);
            //  CPrintf(CON_DEBUG, "Sent: %s\n",resp.ToString(NULL).GetDataSafe());
        }
        else if(item && item->GetIsWriteable() &&
                item->GetOwningCharacter() == client->GetCharacterData() &&
                psserver->CheckAccess(client, "write all"))
        {
            csString theText(item->GetBookText());
            csString theTitle(item->GetName());
            psWriteBookMessage resp(client->GetClientNum(), theTitle, theText, true, (INVENTORY_SLOT_NUMBER)mesg.slotID, mesg.containerID);
            resp.SendMessage();
        }
        else
        {
            //construct error message indicating that the item is not editable
            psserver->SendSystemError(client->GetClientNum(), "You cannot write on this item.");
        }
    }
    else if(mesg.messagetype == mesg.SAVE)
    {
        // CPrintf(CON_DEBUG, "Attempt to save book in slot id %d\n",mesg.slotID);
        //something like:
        psItem* item = slotManager->FindItem(client, mesg.containerID, (INVENTORY_SLOT_NUMBER) mesg.slotID);
        if(item && item->GetIsWriteable())
        {
            csString currentTitle = item->GetName();
            if(mesg.title.Length() > 0 && mesg.title != currentTitle)
            {
                currentTitle = mesg.title;
                item->SetName(mesg.title);
                //update the inventory. fixes PS#4406. TODO: we really need a way to send only one item update
                //to clients inventory
                SendInventory(client->GetClientNum(),true);
            }

            // or psItem* item = (find the player)->GetCurrentWritingItem();
            bool saveOK = item->SetBookText(mesg.content);
            psWriteBookMessage saveresp(client->GetClientNum(), currentTitle, saveOK);
            saveresp.SendMessage();
            item->Save(false);
        }
        // clear current writing item

    }
}


void ServerCharManager::HandleFaction(MsgEntry* me,Client* client)
{
    psCharacter* chardata=client->GetCharacterData();
    if(chardata==NULL)
        return;

    psFactionMessage outMsg(me->clientnum, psFactionMessage::MSG_FULL_LIST);

    csHash<FactionStanding*, int>::GlobalIterator iter(chardata->GetFactions()->GetStandings().GetIterator());
    while(iter.HasNext())
    {
        FactionStanding* standing = iter.Next();
        outMsg.AddFaction(standing->faction->name, standing->score);
    }

    outMsg.BuildMsg();
    outMsg.SendMessage();
}


//---------------------------------------------------------------------------
/**
 * This handles all formats of inventory message.
 */
//---------------------------------------------------------------------------
void ServerCharManager::HandleInventoryMessage(MsgEntry* me,Client* client)
{
    psGUIInventoryMessage incoming(me, NULL);
    int     fromClientNumber    = me->clientnum;

    switch(incoming.command)
    {
        case psGUIInventoryMessage::REQUEST:
        case psGUIInventoryMessage::UPDATE_REQUEST:
        {
            csString status;
            status.Format("Handled inventory message request clientnum %u", fromClientNumber);

            if(LogCSV::GetSingletonPtr())
                LogCSV::GetSingleton().Write(CSV_STATUS, status);

            // FIXME: Ugly hack here as a temporary workaround for client-side issue that causes the server
            // to be flooded with inventory requests. Remove after all clients have been updated
            // to stop flooding.
            csTicks currentTime = csGetTicks();
            // Send an inventory message maximum once per 250 ticks (0.25 seconds).
            if(!client->lastInventorySend || client->lastInventorySend + 250 < currentTime)
            {
                client->lastInventorySend = currentTime;
                SendInventory(fromClientNumber, (static_cast<psGUIInventoryMessage::commands>(incoming.command)==psGUIInventoryMessage::UPDATE_REQUEST));
            }
            else
            {
                status.Format("Ignored inventory request message from %u.", client->GetClientNum());
                if(LogCSV::GetSingletonPtr())
                    LogCSV::GetSingleton().Write(CSV_STATUS, status);
            }
            break;
        }
    }
}

bool ServerCharManager::SendInventory(uint32_t clientNum, bool sendUpdatesOnly)
{
    psGUIInventoryMessage* outgoing;

    Client* client = psserver->GetNetManager()->GetClient(clientNum);
    if(client==NULL)
        return false;

    bool exchanging = (client->GetExchangeID() != 0); // When exchanging, only send partial inv of what is not offered

    uint32_t toClientNumber = clientNum;
//    int itemCount, itemRemovedCount;
//    unsigned int z;

    psCharacter* chardata=client->GetCharacterData();
    if(chardata==NULL)
        return false;

    size_t msgsize = 0;

    Notify2(LOG_EXCHANGES,"Sending %zu items...\n", chardata->Inventory().GetInventoryIndexCount()-1);

    // first count how big the buffer needs to be
    for(size_t i=1; i < chardata->Inventory().GetInventoryIndexCount(); i++)
    {
        psCharacterInventory::psCharacterInventoryItem* invitem = chardata->Inventory().GetIndexCharInventoryItem(i);
        psItem* item  = invitem->GetItem();

        int slot  = item->GetLocInParent(true);

        Notify5(LOG_EXCHANGES, "  Inv item %s, slot %d, weight %1.1f, stack count %u\n",item->GetName(), slot, item->GetWeight(), item->GetStackCount());
        msgsize += strlen(item->GetName()) + 1 + sizeof(uint32_t) * 5 + sizeof(float) * 2 + strlen(item->GetImageName()) + 1 + sizeof(uint8_t);
    }

    psMoney m = chardata->Money();
    Exchange* exchange = exchanging ? psserver->exchangemanager->GetExchange(client->GetExchangeID()) : NULL;
    if(exchange)
        m += -exchange->GetOfferedMoney(client);
    msgsize += strlen(m.ToString()) + 1;

    // actually create the message
    outgoing = new psGUIInventoryMessage(toClientNumber,
                                         psGUIInventoryMessage::LIST,
                                         (uint32_t)chardata->Inventory().GetInventoryIndexCount()-1,  // skip item 0
                                         (uint32_t)0, chardata->Inventory().MaxWeight(),
                                         chardata->Inventory().GetInventoryVersion(), msgsize);

    for(size_t i=1; i < chardata->Inventory().GetInventoryIndexCount(); i++)
    {
        psCharacterInventory::psCharacterInventoryItem* invitem = chardata->Inventory().GetIndexCharInventoryItem(i);
        psItem* item  = invitem->GetItem();

        int slot  = item->GetLocInParent(true);

        int invType = CONTAINER_INVENTORY_BULK;

        if(slot < PSCHARACTER_SLOT_BULK1)   // equipped if < than first bulk
            invType = CONTAINER_INVENTORY_EQUIPMENT;

        Notify5(LOG_EXCHANGES, "  Inv item %s, slot %d, weight %1.1f, stack count %u\n",item->GetName(), slot, item->GetWeight(), item->GetStackCount());
        outgoing->AddItem(item->GetName(),
                          item->GetMeshName(),
                          item->GetTextureName(),
                          invType,
                          slot,
                          (exchanging && invitem->exchangeOfferSlot != PSCHARACTER_SLOT_NONE) ? item->GetStackCount() - invitem->exchangeStackCount : item->GetStackCount(),
                          item->GetWeight(),
                          item->GetTotalStackSize(),
                          item->GetImageName(),
                          item->GetPurifyStatus(),
                          cacheManager->GetMsgStrings());
    }


    outgoing->AddMoney(m);

    if(outgoing->valid)
    {
        outgoing->msg->ClipToCurrentSize();
        psserver->GetEventManager()->SendMessage(outgoing->msg);

        // Increase the inventory version, since this version was sent.
        chardata->Inventory().IncreaseInventoryVersion();

        // server now can believe the clients inventory cache is upto date
        //        inventoryCache->SetCacheStatus(psCache::VALID);
    }
    else
    {
        Bug2("Could not create valid psGUIInventoryMessage for client %u.\n",toClientNumber);
        CS_ASSERT(false);
    }

    delete outgoing;

    return true;
}

bool ServerCharManager::UpdateItemViews(uint32_t clientNum)
{
    Client* client = psserver->GetNetManager()->GetClient(clientNum);
    if(!client)
    {
        return false;
    }

    // If inventory window is up, update it
    SendInventory(clientNum);

    // If glyph window is up, update it
    psserver->GetSpellManager()->SendGlyphs(NULL,client);

    return true;
}

bool ServerCharManager::IsBanned(const char* name)
{
    // Check if the name is banned
    csString nName = NormalizeCharacterName(name);
    for(int i = 0; i < (int)cacheManager->GetBadNamesCount(); i++)
    {
        csString name = cacheManager->GetBadName(i); // Name already normalized
        if(name == nName)
            return true;
    }
    return false;
}


bool ServerCharManager::HasConnected(csString name)
{
    int secondsLastLogin;
    secondsLastLogin = 0;

    //Query to the db that calculates already the amount of seconds since the last login.
    Result result(db->Select("SELECT last_login, UNIX_TIMESTAMP() - UNIX_TIMESTAMP(last_login) as seconds_since_last_login FROM characters WHERE name = '%s'", name.GetData()));
    //There is no character with such a name.
    if(!result.IsValid() || result.Count() == 0)
    {
        return false;
    }
    //We check when the char was last online.
    secondsLastLogin = result[0].GetInt(1);

    if(secondsLastLogin > MAX_DAYS_NO_LOGON)    // More than 2 months since last login.
    {
        return false;
    }

    //The char has connected recently.
    return true;
}

bool ServerCharManager::TradingCheck(Client* client, gemObject* target, csString* errorMessage)
{
    psCharacter* merchant = target->GetCharacterData();
    PSCHARACTER_MODE mode = client->GetActor()->GetMode();

    // Make sure that we are not busy with something else
    if((mode != PSCHARACTER_MODE_PEACE) && (mode != PSCHARACTER_MODE_SIT) &&
            (mode != PSCHARACTER_MODE_OVERWEIGHT) && (mode != PSCHARACTER_MODE_EXHAUSTED))
    {
        if(errorMessage)
        {
            *errorMessage = "You cannot trade because you are already busy.";
        }
        return false;
    }

    if(client->GetExchangeID())
    {
        if(errorMessage)
        {
            *errorMessage = "You are already busy with another trade.";
        }
        return false;
    }

    if(!merchant)
    {
        if(errorMessage)
        {
            *errorMessage = "Merchant not found.";
        }
        return false;
    }

    if(!target->IsAlive())
    {
        if(errorMessage)
        {
            *errorMessage = "Can't trade with a dead merchant.";
        }
        return false;
    }

    if(!merchant->IsMerchant())
    {
        if(errorMessage)
        {
            errorMessage->AppendFmt("%s isn't a merchant.", merchant->GetCharName());
        }
        return false;
    }

    return true;
}


void ServerCharManager::BeginTrading(Client* client, gemObject* target, const csString &type)
{
    psCharacter* merchant = target->GetCharacterData();
    int clientnum = client->GetClientNum();
    psCharacter* character = client->GetCharacterData();

    ///////////
    // Check preconditions for trading.
    // Range check is outside TradingCheck to prevent dublicate range checks in gemNPC::SendBehaviorMessage

    // Check within select range
    if(client->GetActor()->RangeTo(target) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(clientnum, "You are not in range to trade with %s.", merchant->GetCharName());
        return;
    }

    // Check all other preconditions
    csString errorMessage;
    if(!TradingCheck(client, target, &errorMessage))
    {
        psserver->SendSystemInfo(clientnum, errorMessage);
        return;
    }
    // End of preconditons
    ///////////

    psserver->SendSystemInfo(clientnum, "You start trading with %s.",merchant->GetCharName());

    if(type == "SELL")
    {
        csString commandData;
        commandData.Format("<MERCHANT ID=\"%d\" TRADE_CMD=\"%d\" />",
                           merchant->GetPID().Unbox(), psGUIMerchantMessage::SELL);

        psGUIMerchantMessage msg1(clientnum,psGUIMerchantMessage::MERCHANT,commandData);
        msg1.SendMessage();
        character->SetTradingStatus(psCharacter::SELLING,merchant);
    }
    else
    {
        csString commandData;
        commandData.Format("<MERCHANT ID=\"%d\" TRADE_CMD=\"%d\" />",
                           merchant->GetPID().Unbox(), psGUIMerchantMessage::BUY);
        psGUIMerchantMessage msg1(clientnum,psGUIMerchantMessage::MERCHANT,commandData);
        psserver->GetEventManager()->SendMessage(msg1.msg);
        character->SetTradingStatus(psCharacter::BUYING,merchant);
    }

    // Build category list
    csString categoryList("<L>");
    csString buff;

    psMerchantInfo* merchantInfo = merchant->GetMerchantInfo();

    for(size_t z = 0; z < merchantInfo->categories.GetSize(); z++)
    {
        psItemCategory* category = merchantInfo->categories[z];
        if(type == "SELL" || merchant->Inventory().hasItemCategory(category,false,true))
        {
            csString escpxml = EscpXML(category->name);
            buff.Format("<CATEGORY ID=\"%d\" "
                        "NAME=\"%s\" />",category->id,
                        escpxml.GetData());
            categoryList.Append(buff);
        }
    }
    categoryList.Append("</L>");

    psGUIMerchantMessage msg2(clientnum,psGUIMerchantMessage::CATEGORIES,categoryList.GetData());
    if(msg2.valid)
        psserver->GetEventManager()->SendMessage(msg2.msg);
    else
    {
        Bug2("Could not create valid psGUIMerchantMessage for client %u.\n",clientnum);
    }


    SendPlayerMoney(client);
}

void ServerCharManager::HandleMerchantRequest(psGUIMerchantMessage &msg, Client* client)
{
    csRef<iDocumentNode> exchangeNode = ParseStringGetNode(msg.commandData, "R");
    if(!exchangeNode)
        return;

    csRef<iDocumentAttribute> attr = exchangeNode->GetAttribute("TARGET");
    csString type = exchangeNode->GetAttributeValue("TYPE");

    gemObject* target = NULL;
    if(attr)
    {
        csString targetName = attr->GetValue();
        target = gemSupervisor->FindObject(targetName);
        if(!target)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "Merchant '%s' not found.", targetName.GetData());
            return;
        }
    }
    else
    {
        target = client->GetTargetObject();
        if(!target)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You have no target selected.");
            return;
        }
    }

    BeginTrading(client, target, type);
}

void ServerCharManager::HandleMerchantCategory(psGUIMerchantMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();

    csRef<iDocumentNode> merchantNode = ParseStringGetNode(msg.commandData, "C");
    if(!merchantNode)
        return;

    psCharacter* merchant;
    psMerchantInfo* merchantInfo;

    if(VerifyTrade(client, character,&merchant,&merchantInfo,
                   "category","",merchantNode->GetAttributeValueAsInt("ID")))
    {
        csString category = merchantNode->GetAttributeValue("CATEGORY");
        psItemCategory* itemCategory = merchantInfo->FindCategory(category);
        if(!itemCategory)
        {
            CPrintf(CON_DEBUG, "Player %s fails to get items in category %s. Unkown category!\n",
                    character->GetCharName(), (const char*)category);
            return;
        }
        if(!merchant->GetActor()->IsAlive())
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You can't trade with a dead merchant.");
            return;
        }

        // Send item list for given category
        if(character->GetTradingStatus() == psCharacter::BUYING)
        {
            SendMerchantItems(client, merchant, itemCategory);
        }
        else
        {
            SendPlayerItems(client, itemCategory, false);
        }
    }
}

void ServerCharManager::HandleMerchantBuy(psGUIMerchantMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();
    csRef <iDocumentNode> merchantNode = ParseStringGetNode(msg.commandData, "T");
    if(!merchantNode)
        return;

    csString itemName = merchantNode->GetAttributeValue("ITEM");
    int count = merchantNode->GetAttributeValueAsInt("COUNT");
    int merchantID = merchantNode->GetAttributeValueAsInt("ID");
    uint32 itemID = (uint32)merchantNode->GetAttributeValueAsInt("ITEM_ID");

    // check negative item counts to avoid integer overflow
    if(count <= 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "You can't trade this amount of items.");
        return;
    }

    psCharacter* merchant;
    psMerchantInfo* merchantInfo;

    if(VerifyTrade(client, character,&merchant,&merchantInfo,
                   "buy",itemName,merchantID))
    {
        psItem* item = merchant->Inventory().FindItemID(itemID);
        if(!item || count > item->GetStackCount())
        {
            psserver->SendSystemError(client->GetClientNum(), "Merchant does not have %i %s.", count, itemName.GetData());
            return;
        }
        if(!merchant->GetActor()->IsAlive())
        {
            psserver->SendSystemError(client->GetClientNum(),"That merchant is dead");
            return;
        }
        psMoney price = CalculateMerchantPrice(item, client, false);
        psMoney money = character->Money();

        if(price*count > money)
        {
            psserver->SendSystemError(client->GetClientNum(),"You need more money");
            return;
        }

        int canFit = (int)character->Inventory().HowManyCanFit(item); // count that actually fit into buyer's inventory

        if(count > canFit)
        {
            count = canFit;
            // Notify the buyer that their inventory is full.  (will purchase what fits)
            psserver->SendSystemError(client->GetClientNum(),"Your inventory is full");
            if(count <= 0) return;
        }

        csList<psItem*> newitem;
        bool error = false;
        // if item is to be personalised, then duplicate the base it on the item_stats and personalise
        // by given it a unique name for the purchaser.
        if(item->GetBuyPersonalise())
        {
            for(int i = 0; i < count; i++)
            {
                psItem* newpersonaliseditem;
                PSITEMSTATS_CREATORSTATUS creatorStatus;
                item->GetCreator(creatorStatus);
                csString itemName(item->GetName());
                if(creatorStatus != PSITEMSTATS_CREATOR_PUBLIC)
                    itemName.AppendFmt(" of %s", client->GetName());

                csString personalisedName(itemName);

                // instantiate it
                newpersonaliseditem = item->GetBaseStats()->InstantiateBasicItem();

                if(!newpersonaliseditem)
                {
                    error = true;
                    break;
                }
                newpersonaliseditem->SetName(personalisedName);
                //copies the template creative data from the item stats in the item instances for use
                //by the player
                newpersonaliseditem->PrepareCreativeItemInstance();
                newpersonaliseditem->SetLoaded();
                newitem.PushBack(newpersonaliseditem);

            }
        }
        else // normal purchase
        {
            int maxcount = MAX_STACK_COUNT;
            if(item->GetIsContainer() || !item->GetIsStackable())
                maxcount = 1;

            // split it into several stacks if it doesn't fit into a single one
            for(int i = 0; i < count/maxcount && !error; i++)
                if(!*newitem.PushBack(item->Copy(maxcount)))
                    error = true;

            if(error || (count % maxcount && !*newitem.PushBack(item->Copy(count % maxcount))))
                error = true;
        }

        if(error)
        {
            Error2("Error: failed to create item %s.", itemName.GetData());
            psserver->SendSystemError(client->GetClientNum(), "Error: failed to create item %s.", itemName.GetData());

            // destroy the created items
            for(csList<psItem*>::Iterator newitemitr(newitem); newitemitr.HasNext();)
            {
                psItem* currentitem = newitemitr.Next();
                if(!currentitem)
                    continue;

                cacheManager->RemoveInstance(currentitem);
            }

            return;
        }

        csList<psBuyEvent> buyevents;
        psMoney cost;
        int newcount = 0; // count what we really got

        for(csList<psItem*>::Iterator newitemitr(newitem); newitemitr.HasNext();)
        {
            psItem* &currentitem = newitemitr.Next();
            if(!currentitem)
                continue;

            bool stackable = currentitem->GetIsStackable();
            int partcount = 1;

            // if it's stackable, try to add it on existing stacks, first
            if(stackable)
            {
                psItem* newstack;
                while((newstack = character->Inventory().AddStacked(currentitem, partcount)) != NULL)
                {
                    newcount += partcount;

                    psMoney partcost((price * partcount).Normalized());
                    cost += partcost;

                    psBuyEvent evt(
                        character->GetPID(),
                        character->GetCharName(),
                        merchant->GetPID(),
                        merchant->GetCharName(),
                        newstack->GetUID(),
                        newstack->GetName(),
                        partcount,
                        (int)newstack->GetCurrentStats()->GetQuality(),
                        partcost.GetTotal()
                    );
                    buyevents.PushBack(evt);
                }

                partcount = currentitem->GetStackCount();
            }

            if(character->Inventory().Add(currentitem, false, stackable))
            {
                newcount += partcount;
                psMoney partcost((price * partcount).Normalized());
                cost += partcost;

                psBuyEvent evt(
                    character->GetPID(),
                    character->GetCharName(),
                    merchant->GetPID(),
                    merchant->GetCharName(),
                    currentitem->GetUID(),
                    currentitem->GetName(),
                    partcount,
                    (int)currentitem->GetCurrentStats()->GetQuality(),
                    partcost.GetTotal()
                );
                buyevents.PushBack(evt);
            }
            else
            {
                cacheManager->RemoveInstance(currentitem);
            }
        }

        if(newcount != count)  // not enough empty or stackable slots
            psserver->SendSystemError(client->GetClientNum(),"You're carrying too many items [%d/%d]", newcount, count);

        if(!newcount)
            return;

        // If we managed to buy some items, we pay some money
        character->SetMoney(money - cost);

        psserver->SendSystemOK(client->GetClientNum(), "You bought %d %s for %s a total of %d Trias.",
                               newcount, itemName.GetData(), cost.ToUserString().GetDataSafe(),cost.GetTotal());

        for(csList<psBuyEvent>::Iterator buyeventsitr(buyevents); buyeventsitr.HasNext();)
            buyeventsitr.Next().FireEvent();

        // Update client views
        SendPlayerMoney(client);
        SendMerchantItems(client, merchant, item->GetCategory());

        // Update all client views
        UpdateItemViews(client->GetClientNum());
    }
}

void ServerCharManager::HandleMerchantSell(psGUIMerchantMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();
    csRef <iDocumentNode> merchantNode = ParseStringGetNode(msg.commandData, "T");
    if(!merchantNode)
        return;

    csString itemName = merchantNode->GetAttributeValue("ITEM");
    int count = merchantNode->GetAttributeValueAsInt("COUNT");
    int merchantID = merchantNode->GetAttributeValueAsInt("ID");

    // check negative item counts to avoid integer overflow
    if(count <= 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "You can't trade this amount of items.");
        return;
    }

    psCharacter* merchant;
    psMerchantInfo* merchantInfo;

    if(VerifyTrade(client, character,&merchant,&merchantInfo,
                   "sell",itemName,merchantID))
    {
        uint32 itemID =(uint32) merchantNode->GetAttributeValueAsInt("ITEM_ID");
        psItem* item = character->Inventory().FindItemID(itemID);
        if(!item)
            return;
        if(character->Inventory().GetContainedItemCount(item) > 0)
        {
            psserver->SendSystemError(client->GetClientNum(), "You must take your items out of the container first.");
            return;
        }
        if(!merchant->GetActor()->IsAlive())
        {
            psserver->SendSystemError(client->GetClientNum(), "You can't trade with a dead merchant.");
            return;
        }

        psMoney price = CalculateMerchantPrice(item, client, true);
        psMoney money = character->Money();

        count = csMin(count, (int)item->GetStackCount());
        csString name(item->GetName());

        item = character->Inventory().RemoveItem(NULL,item->GetLocInParent(true), count);
        if(item == NULL)
        {
            Error3("RemoveItem failed while selling to merchant %s %i", name.GetDataSafe(), count);
            return;
        }

        psMoney cost;
        cost = (price * count).Normalized();

        character->SetMoney(money + cost);

        psserver->SendSystemOK(client->GetClientNum(), "You sold %d %s for %s a total of %d Trias.",
                               count, itemName.GetData(), cost.ToUserString().GetDataSafe(),cost.GetTotal());

        // Record
        psSellEvent evt(character->GetPID(),
                        character->GetCharName(),
                        merchant->GetPID(),
                        merchant->GetCharName(),
                        item->GetUID(),
                        item->GetName(),
                        count,
                        (int)item->GetCurrentStats()->GetQuality(),
                        cost.GetTotal());
        evt.FireEvent();

        ServerStatus::sold_items += count;
        ServerStatus::sold_value += (price * count).GetTotal();

        // Update client views
        SendPlayerMoney(client);
        SendPlayerItems(client, item->GetCategory(), false);

        // items are not currently given to merchant, they are just destroyed
        psItemStats* itemStats = item->GetBaseStats();
        bool uniqueItem = itemStats->GetUnique();
        cacheManager->RemoveInstance(item);
        // if the item is unique, like a player-written book, remove the item stats too
        if(uniqueItem)
            cacheManager->RemoveItemStats(itemStats);

        // Update all client views
        UpdateItemViews(client->GetClientNum());
    }
}

void ServerCharManager::HandleMerchantView(psGUIMerchantMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();
    csRef <iDocumentNode> merchantNode = ParseStringGetNode(msg.commandData, "V");
    if(!merchantNode)
        return;

    csString itemName = merchantNode->GetAttributeValue("ITEM");
    int merchantID    = merchantNode->GetAttributeValueAsInt("ID");
    uint32 itemID     = (uint32)merchantNode->GetAttributeValueAsInt("ITEM_ID");
    int tradeCommand  = merchantNode->GetAttributeValueAsInt("TRADE_CMD");

    psCharacter* merchant;
    psMerchantInfo* merchantInfo;

    if(VerifyTrade(client, character,&merchant,&merchantInfo,
                   "view",itemName,merchantID))
    {
        if(!merchant->GetActor()->IsAlive())
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You can't trade with a dead merchant.");
            return;
        }
        psItem* item;
        if(tradeCommand == psGUIMerchantMessage::SELL)
            item = character->Inventory().FindItemID(itemID);
        else
            item = merchant->Inventory().FindItemID(itemID);

        if(!item)
        {
            CPrintf(CON_DEBUG, "Player %s failed to view item %s. No item!\n",
                    client->GetName(), (const char*)itemName);
            return;
        }

        item->SendItemDescription(client);
    }
}

void ServerCharManager::HandleMerchantMessage(MsgEntry* me, Client* client)
{
    psGUIMerchantMessage msg(me);
    if(!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psGUIMerchantMessage from client %u.\n",me->clientnum);
        return;
    }

    //    CPrintf(CON_DEBUG, "ServerCharManager::HandleMerchantMessage (%s, %d,%s)\n",
    //           (const char*)client->GetName(),msg.command, (const char*)msg.commandData);

    switch(msg.command)
    {
            // This handles the initial request to buy or sell from a merchant.
            // A list of categories that this merchant handles is sent
        case psGUIMerchantMessage::REQUEST:
        {
            HandleMerchantRequest(msg,client);
            break;
        }
        //  This handles
        case psGUIMerchantMessage::CATEGORY:
        {
            HandleMerchantCategory(msg,client);
            break;
        }
        case psGUIMerchantMessage::BUY:
        {
            HandleMerchantBuy(msg,client);
            break;
        }
        case psGUIMerchantMessage::SELL:
        {
            HandleMerchantSell(msg,client);
            break;
        }
        case psGUIMerchantMessage::VIEW:
        {
            HandleMerchantView(msg,client);
            break;
        }
        case psGUIMerchantMessage::CANCEL:
        {
            client->GetCharacterData()->SetTradingStatus(psCharacter::NOT_TRADING,0);
            break;
        }

    }
}

bool ServerCharManager::VerifyTrade(Client* client, psCharacter* character, psCharacter** merchant, psMerchantInfo** info,
                                    const char* trade,const char* itemName, PID merchantID)
{
    *merchant = character->GetMerchant();
    if(!*merchant)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You can only buy from a merchant.");
        Debug4(LOG_EXCHANGES,client->GetClientNum(),"Player %s failed to %s item %s. No merchant!\n",
               character->GetCharName(), trade, itemName);
        return false;
    }
    *info = (*merchant)->GetMerchantInfo();
    if(!*info)
    {
        CPrintf(CON_DEBUG, "Player %s failed to %s item %s. No merchant info!\n",
                character->GetCharName(), trade, itemName);
        return false;
    }
    // Check if player is trading with this merchant.
    if(character->GetTradingStatus() == psCharacter::NOT_TRADING)
    {
        CPrintf(CON_DEBUG, "Player %s failed to %s item %s. No trading status!\n",
                character->GetCharName(), trade, itemName);
        return false;
    }
    // Check if this is correct merchant
    if(merchantID != (*merchant)->GetPID())
    {
        CPrintf(CON_DEBUG, "Player %s failed to %s item %s. Different merchant!\n",
                character->GetCharName(), trade, itemName);
        return false;
    }
    // Check range and actor validity
    if(!((*merchant)->GetActor()) || character->GetActor()->RangeTo((*merchant)->GetActor()) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"Merchant is out of range.");
        CPrintf(CON_DEBUG, "Player %s failed to %s item %s. Out of range!\n",
                character->GetCharName(), trade, itemName);
        return false;
    }

    return true;
}


void ServerCharManager::SendOutPlaySoundMessage(uint32_t clientNum, const char* itemsound, const char* action)
{
    if(clientNum == 0 || itemsound == NULL || action == NULL)
        return;

    csString sound = itemsound;

    if(sound == "item.nosound")
        return;

    sound += ".";
    sound += action;

    Debug3(LOG_SOUND, clientNum, "Sending sound %s to client %d", sound.GetData(), clientNum);

    psPlaySoundMessage msg(clientNum, sound);
    psserver->GetEventManager()->SendMessage(msg.msg);

// TODO: Sounds should really be multicasted, so others can hear them
//    psserver->GetEventManager()->Multicast(msg, fromClient->GetActor()->GetMulticastClients(), 0, range );
}

void ServerCharManager::SendOutEquipmentMessages(gemActor* actor,
        INVENTORY_SLOT_NUMBER slot,
        psItem* item,
        int equipped)
{
    EID eid = actor->GetEID();

    csString mesh = item->GetMeshName();
    csString part = item->GetPartName();
    csString partMesh = item->GetPartMeshName();
    csString removedMesh = item->GetSlotRemovedMesh((int)slot, actor->GetMesh());

    // If we're doing a 'deequip', there is no texture.
    csString texture;
    if(equipped != psEquipmentMessage::DEEQUIP)
        texture = item->GetTextureName();

    // printf("Sending Equipment Message for %s, slot %d, item %s.\n", actor->GetName(), slot, item->GetName() );

    /* Wield: mesh in a slot (no part or texture)
     * Wear: mesh for standalone; texture on a part when worn
     *
     * We'll send the info the client needs, and it figures the rest out.
     */
    if(part.Length() && texture.Length())
        mesh.Clear();

    psEquipmentMessage msg(0, eid, equipped, slot, mesh, part, texture, partMesh, removedMesh);
    CS_ASSERT(msg.valid);

    psserver->GetEventManager()->Multicast(msg.msg,
                                           actor->GetMulticastClients(),
                                           0, // Multicast to all without exception
                                           PROX_LIST_ANY_RANGE);
}

void ServerCharManager::SendPlayerMoney(Client* client, bool storage)
{
    csString buff;

    if(client->GetCharacterData()==NULL)
        return;

    psMoney money=client->GetCharacterData()->Money();

    csString money_str = money.ToString();
    buff.Format("<M MONEY=\"%s\" />",money_str.GetData());

    if(storage)//sends the correct message
    {
        psGUIStorageMessage msg(client->GetClientNum(),
                                psGUIStorageMessage::MONEY,buff);
        psserver->GetEventManager()->SendMessage(msg.msg);
    }
    else
    {
        psGUIMerchantMessage msg(client->GetClientNum(),
                                 psGUIMerchantMessage::MONEY,buff);
        psserver->GetEventManager()->SendMessage(msg.msg);
    }
}

bool ServerCharManager::SendMerchantItems(Client* client, psCharacter* merchant, psItemCategory* category)
{
    csArray<psItem*> items = merchant->Inventory().GetItemsInCategory(category);

    // Build item list
    csString buff("<L>");
    csString item;

    for(size_t z = 0; z < items.GetSize(); z++)
    {
        if(items[z]->IsEquipped())
            continue;

        csString escpxml_name = EscpXML(items[z]->GetName());
        csString escpxml_imagename = EscpXML(items[z]->GetImageName());
        item.Format("<ITEM ID=\"%u\" "
                    "NAME=\"%s\" "
                    "IMG=\"%s\" "
                    "PRICE=\"%i\" "
                    "COUNT=\"%i\" />",
                    items[z]->GetUID(),
                    escpxml_name.GetData(),
                    escpxml_imagename.GetData(),
                    CalculateMerchantPrice(items[z], client, false),
                    items[z]->GetStackCount());

        buff.Append(item);
    }
    buff.Append("</L>");

    psGUIMerchantMessage msg4(client->GetClientNum(),psGUIMerchantMessage::ITEMS,buff.GetData());
    if(msg4.valid)
        psserver->GetEventManager()->SendMessage(msg4.msg);
    else
    {
        Bug2("Could not create valid psGUIMerchantMessage for client %u.\n",client->GetClientNum());
    }


    return true;
}

int ServerCharManager::CalculateMerchantPrice(psItem* item, Client* client, bool sellPrice)
{
    int basePrice = sellPrice?item->GetSellPrice().GetTotal():item->GetPrice().GetTotal();

    if((sellPrice && !calc_item_merchant_price_sell) || (!sellPrice && !calc_item_merchant_price_buy))
        return basePrice;

    csRef<ItemSupplyDemandInfo> suppInfo = psserver->GetEconomyManager()->GetItemSupplyDemandInfo(item->GetUID());

    MathEnvironment env;
    env.Define("CharData",  client->GetCharacterData());
    env.Define("ItemPrice", basePrice);
    env.Define("Demand",    suppInfo->sold);
    env.Define("Supply",    suppInfo->bought);
    env.Define("Time",      psserver->GetWeatherManager()->GetGameTODHour());
    if(sellPrice)
    {
        calc_item_merchant_price_sell->Evaluate(&env);
    }
    else
    {
        calc_item_merchant_price_buy->Evaluate(&env);
    }

    MathVar* result = env.Lookup("Result");
    return result->GetRoundValue();
}

bool ServerCharManager::SendPlayerItems(Client* client, psItemCategory* category, bool storage)
{
    csArray<psItem*> items = client->GetCharacterData()->Inventory().GetItemsInCategory(category);

    // Build item list
    csString buff("<L>");
    csString item;
    csString itemID;

    for(size_t z = 0; z < items.GetSize(); z++)
    {
        if(items[z]->IsEquipped())
            continue;

        itemID.Format("%u",items[z]->GetUID());
        csString purified;

        if(items[z]->GetPurifyStatus() == 2)
            purified = "yes";
        else
            purified = "no";

        csString itemName;
        if(items[z]->GetCrafterID() != 0)
        {
            itemName.Format("%s %s", items[z]->GetQualityString(),
                            items[z]->GetName());
        }
        else
        {
            itemName = items[z]->GetName();
        }
        csString escpxml_name = EscpXML(itemName);
        csString escpxml_imagename = EscpXML(items[z]->GetImageName());
        item.Format("<ITEM ID=\"%s\" "
                    "NAME=\"%s\" "
                    "IMG=\"%s\" "
                    "PRICE=\"%i\" "
                    "COUNT=\"%i\" "
                    "PURIFIED=\"%s\" />",
                    itemID.GetDataSafe(),
                    escpxml_name.GetData(),
                    escpxml_imagename.GetData(),
                    CalculateMerchantPrice(items[z], client, true),
                    items[z]->GetStackCount(),
                    purified.GetData());

        buff.Append(item);
    }
    buff.Append("</L>");
    if(storage)//send the correct message depending on situation
    {
        psGUIStorageMessage msg4(client->GetClientNum(),psGUIStorageMessage::ITEMS,buff.GetData());
        if(msg4.valid)
            psserver->GetEventManager()->SendMessage(msg4.msg);
        else
        {
            Bug2("Could not create valid psGUIStorageMessage for client %u.\n",client->GetClientNum());
        }
    }
    else
    {
        psGUIMerchantMessage msg4(client->GetClientNum(),psGUIMerchantMessage::ITEMS,buff.GetData());
        if(msg4.valid)
            psserver->GetEventManager()->SendMessage(msg4.msg);
        else
        {
            Bug2("Could not create valid psGUIMerchantMessage for client %u.\n",client->GetClientNum());
        }
    }


    return true;
}

bool ServerCharManager::VerifyGoal(Client* client, psCharacter* character, psItem* goal)
{
    // glyph items can't be goals
    if(goal->GetCurrentStats()->GetIsGlyph())
        return false;

    return true;
}

/*********************************************************************
 * Storage functionalities                                           *
 *********************************************************************/

void ServerCharManager::HandleStorageMessage(MsgEntry* me, Client* client)
{
    psGUIStorageMessage msg(me);
    if(!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psGUIStorageMessage from client %u.\n",me->clientnum);
        return;
    }

    switch(msg.command)
    {
            // This handles the initial request to withdraw or store from a storage
            // A list of categories that this the player has is sent (store is default)
        case psGUIStorageMessage::REQUEST:
        {
            HandleStorageRequest(msg,client);
            break;
        }
        //  This handles the categories in the player or storage inventory and shows them only
        case psGUIStorageMessage::CATEGORY:
        {
            HandleStorageCategory(msg,client);
            break;
        }
        // This handles the withdrawing of the items from the storage
        case psGUIStorageMessage::WITHDRAW:
        {
            HandleStorageWithdraw(msg,client);
            break;
        }
        // This handles the storing of the items in the storage
        case psGUIStorageMessage::STORE:
        {
            HandleStorageStore(msg,client);
            break;
        }
        // This allows showing the item informations for either player inventory and storage
        case psGUIStorageMessage::VIEW:
        {
            HandleStorageView(msg,client);
            break;
        }
        //stops the storage management.
        case psGUIStorageMessage::CANCEL:
        {
            client->GetCharacterData()->SetTradingStatus(psCharacter::NOT_TRADING,0);
            break;
        }

    }
}

bool ServerCharManager::VerifyStorage(Client* client, psCharacter* character, psCharacter** storage,
                                      const char* trade,const char* itemName, PID storageID)
{
    *storage = character->GetMerchant();
    if(!*storage)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You can only store item with someone owning a storage.");
        Debug4(LOG_EXCHANGES,client->GetClientNum(),"Player %s failed to %s item %s. No storage!\n",
               character->GetCharName(), trade, itemName);
        return false;
    }
    // Check if player is trading with this merchant.
    if(character->GetTradingStatus() == psCharacter::NOT_TRADING)
    {
        CPrintf(CON_DEBUG, "Player %s failed to %s item %s. No trading status!\n",
                character->GetCharName(), trade, itemName);
        return false;
    }
    // Check if this is correct merchant
    if(storageID != (*storage)->GetPID())
    {
        CPrintf(CON_DEBUG, "Player %s failed to %s item %s. Different merchant!\n",
                character->GetCharName(), trade, itemName);
        return false;
    }
    // Check range and actor validity
    if(!((*storage)->GetActor()) || character->GetActor()->RangeTo((*storage)->GetActor()) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"Storage owner is out of range.");
        CPrintf(CON_DEBUG, "Player %s failed to %s item %s. Out of range!\n",
                character->GetCharName(), trade, itemName);
        return false;
    }

    return true;
}

bool ServerCharManager::SendStorageItems(Client* client, psCharacter* character, psItemCategory* category)
{
    csArray<psItem*> items = character->Inventory().GetItemsInCategory(category,true);

    // Build item list
    csString buff("<L>");
    csString item;

    for(size_t z = 0; z < items.GetSize(); z++)
    {
        if(items[z]->IsEquipped())
            continue;

        // we safely send only the first MAX_ITEMS_IN_CATEGORY stacks to avoid server sending a corrupted list
        if(z>=MAX_ITEMS_IN_CATEGORY)
        {
            psserver->SendSystemError(client->GetClientNum(), "The list of items exceeds the allowed max. Showing only the first %d.",MAX_ITEMS_IN_CATEGORY);
            break;
        }

        csString escpxml_name = EscpXML(items[z]->GetName());
        csString escpxml_imagename = EscpXML(items[z]->GetImageName());
        item.Format("<ITEM ID=\"%u\" "
                    "NAME=\"%s\" "
                    "IMG=\"%s\" "
                    "PRICE=\"%i\" "
                    "COUNT=\"%i\" />",
                    items[z]->GetUID(),
                    escpxml_name.GetData(),
                    escpxml_imagename.GetData(),
                    CalculateMerchantPrice(items[z], client, false),
                    items[z]->GetStackCount());

        buff.Append(item);
    }
    buff.Append("</L>");

    psGUIStorageMessage msg4(client->GetClientNum(),psGUIStorageMessage::ITEMS,buff.GetData());
    if(msg4.valid)
        psserver->GetEventManager()->SendMessage(msg4.msg);
    else
    {
        Bug2("Could not create valid psGUIStorageMessage for client %u.\n",client->GetClientNum());
    }


    return true;
}

void ServerCharManager::BeginStoring(Client* client, gemObject* target, const csString &type)
{
    psCharacter* storage = NULL;
    int clientnum = client->GetClientNum();
    psCharacter* character = client->GetCharacterData();
    PSCHARACTER_MODE mode = client->GetActor()->GetMode();

    // Make sure that we are not busy with something else
    if((mode != PSCHARACTER_MODE_PEACE) && (mode != PSCHARACTER_MODE_SIT) &&
            (mode != PSCHARACTER_MODE_OVERWEIGHT) && (mode != PSCHARACTER_MODE_EXHAUSTED))
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot check the storage because you are already busy.");
        return;
    }

    if(client->GetExchangeID())
    {
        psserver->SendSystemError(client->GetClientNum(), "You are already busy with another trade");
        return;
    }

    storage = target->GetCharacterData();
    if(!storage)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "Storage keeper not found.");
        return;
    }

    if(client->GetActor()->RangeTo(target) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You are not in range to check your storage with %s.",storage->GetCharName());
        return;
    }

    if(!target->IsAlive())
    {
        psserver->SendSystemInfo(client->GetClientNum(), "Can't trade with a dead storage keeper.");
        return;
    }
    if(!storage->IsBanker()) //check if it's a banker for now.
    {
        psserver->SendSystemInfo(client->GetClientNum(), "%s doesn't keep a storage.",storage->GetCharName());
        return;
    }

    psserver->SendSystemInfo(client->GetClientNum(), "You start checking your storage with %s.",storage->GetCharName());

    if(type == "STORE")
    {
        csString commandData;
        commandData.Format("<STORAGE ID=\"%d\" TRADE_CMD=\"%d\" />",
                           storage->GetPID().Unbox(), psGUIStorageMessage::STORE);

        psGUIStorageMessage msg1(clientnum,psGUIStorageMessage::STORAGE,commandData);
        msg1.SendMessage();
        character->SetTradingStatus(psCharacter::STORING,storage);
    }
    else
    {
        csString commandData;
        commandData.Format("<STORAGE ID=\"%d\" TRADE_CMD=\"%d\" />",
                           storage->GetPID().Unbox(), psGUIStorageMessage::WITHDRAW);
        psGUIStorageMessage msg1(clientnum,psGUIStorageMessage::STORAGE,commandData);
        psserver->GetEventManager()->SendMessage(msg1.msg);
        character->SetTradingStatus(psCharacter::WITHDRAWING,storage);
    }

    // Build category list
    csString categoryList("<L>");
    csString buff;

    for(size_t z = 0; z < cacheManager->GetItemCategoryAmount(); z++)
    {
        psItemCategory* category = cacheManager->GetItemCategoryByPos(z);
        //check from what category items in player inventory are.
        if(type == "STORE")
        {
            if(character->Inventory().hasItemCategory(category,false,true,false))
            {
                csString escpxml = EscpXML(category->name);
                buff.Format("<CATEGORY ID=\"%d\" "
                            "NAME=\"%s\" />",category->id,
                            escpxml.GetData());
                categoryList.Append(buff);
            }
        }
        //check from what category items in the storage are.
        else if(character->Inventory().hasItemCategory(category,false,false,true))
        {
            csString escpxml = EscpXML(category->name);
            buff.Format("<CATEGORY ID=\"%d\" "
                        "NAME=\"%s\" />",category->id,
                        escpxml.GetData());
            categoryList.Append(buff);
        }
    }
    categoryList.Append("</L>");

    psGUIStorageMessage msg2(clientnum,psGUIStorageMessage::CATEGORIES,categoryList.GetData());
    if(msg2.valid)
        psserver->GetEventManager()->SendMessage(msg2.msg);
    else
    {
        Bug2("Could not create valid psGUIMerchantMessage for client %u.\n",clientnum);
    }

    SendPlayerMoney(client, true);
}

void ServerCharManager::HandleStorageRequest(psGUIStorageMessage &msg, Client* client)
{
    csRef<iDocumentNode> exchangeNode = ParseStringGetNode(msg.commandData, "R");
    if(!exchangeNode)
        return;

    csRef<iDocumentAttribute> attr = exchangeNode->GetAttribute("TARGET");
    csString type = exchangeNode->GetAttributeValue("TYPE");

    gemObject* target = NULL;
    if(attr)
    {
        csString targetName = attr->GetValue();
        target = gemSupervisor->FindObject(targetName);
        if(!target)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "Storage '%s' not found.", targetName.GetData());
            return;
        }
    }
    else
    {
        target = client->GetTargetObject();
        if(!target)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You have no target selected.");
            return;
        }
    }

    BeginStoring(client, target, type);
}

void ServerCharManager::HandleStorageCategory(psGUIStorageMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();

    csRef<iDocumentNode> storageNode = ParseStringGetNode(msg.commandData, "C");
    if(!storageNode)
        return;

    psCharacter* storage;

    if(VerifyStorage(client, character,&storage,"category","",storageNode->GetAttributeValueAsInt("ID")))
    {
        csString category = storageNode->GetAttributeValue("CATEGORY");
        // check if category exists
        psItemCategory* itemCategory = cacheManager->GetItemCategoryByName(category);
        if(!itemCategory)
        {
            CPrintf(CON_DEBUG, "Player %s fails to get items in category %s. Unknown category!\n",
                    character->GetCharName(), (const char*)category);
            return;
        }
        // check if banker is alive
        if(!storage->GetActor()->IsAlive())
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You can't manage your storage with a dead storage owner.");
            return;
        }

        // Send banker item list for given category
        if(character->GetTradingStatus() == psCharacter::WITHDRAWING)
        {
            SendStorageItems(client, character, itemCategory);
        }
        // Send player item list for given category
        else
        {
            SendPlayerItems(client, itemCategory, true);
        }
    }
}

void ServerCharManager::HandleStorageWithdraw(psGUIStorageMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();
    csRef <iDocumentNode> storageNode = ParseStringGetNode(msg.commandData, "T");
    if(!storageNode)
        return;

    csString itemName = storageNode->GetAttributeValue("ITEM");
    int count = storageNode->GetAttributeValueAsInt("COUNT");
    PID storageID = storageNode->GetAttributeValueAsInt("ID");
    uint32 itemID = (uint32)storageNode->GetAttributeValueAsInt("ITEM_ID");

    // check negative item counts to avoid integer overflow
    if(count <= 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "You can't trade this amount of items.");
        return;
    }

    psCharacter* storage;
    if(VerifyStorage(client, character,&storage,"withdraw",itemName,storageID))
    {
        psItem* item = character->Inventory().FindItemID(itemID, true);
        if(!item || count > item->GetStackCount())
        {
            psserver->SendSystemError(client->GetClientNum(), "Storage does not have %i %s.", count, itemName.GetData());
            return;
        }
        if(!storage->GetActor()->IsAlive())
        {
            psserver->SendSystemError(client->GetClientNum(),"You can't manage your storage with a dead storage owner.");
            return;
        }

        int canFit = (int)character->Inventory().HowManyCanFit(item); // count that actually fit into inventory

        if(count > canFit)
        {
            count = canFit;
            // Notify the buyer that their inventory is full.  (will get what fits)
            psserver->SendSystemError(client->GetClientNum(),"Your inventory is full");
            if(count <= 0) return;
        }

        psItem* currentitem = character->Inventory().RemoveItemID(itemID,count,true);

        int newcount = 0; // count what we really got

        if(!currentitem)
        {
            psserver->SendSystemOK(client->GetClientNum(), "Unable to get the item from storage %d %s.", count, itemName.GetData());
            return;
        }

        bool stackable = currentitem->GetIsStackable();
        int partcount = 1;

        if(stackable)  // if it's stackable, try to add in on existing stacks, first
        {
            while(character->Inventory().AddStacked(currentitem, partcount))
            {
                newcount += partcount;
            }

            partcount = currentitem->GetStackCount();
        }

        if(character->Inventory().Add(currentitem, false, stackable))
        {
            newcount += partcount;
        }
        else
        {
            //put back in storage
            character->Inventory().AddStorageItem(currentitem);
            currentitem->SetLocInParent(PSCHARACTER_SLOT_STORAGE);
            currentitem->Save(false);

            if(newcount != count)  // not enough empty or stackable slots
                psserver->SendSystemError(client->GetClientNum(),"You're carrying too many items [%d/%d] the rest was put back in storage.", newcount, count);
        }

        if(!newcount)
        {
            psserver->SendSystemError(client->GetClientNum(),"You're carrying too many items. The item was put back in the storage.");
            return;
        }
        newcount = count;

        // Update client views
        SendPlayerMoney(client, true);
        SendStorageItems(client, character, currentitem->GetCategory());

        psserver->SendSystemOK(client->GetClientNum(), "You got %d %s from the storage.",
                               newcount, itemName.GetData());

        // Update all client views
        UpdateItemViews(client->GetClientNum());
    }
}

void ServerCharManager::HandleStorageStore(psGUIStorageMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();
    csRef <iDocumentNode> storageNode = ParseStringGetNode(msg.commandData, "T");
    if(!storageNode)
        return;

    csString itemName = storageNode->GetAttributeValue("ITEM");
    int count = storageNode->GetAttributeValueAsInt("COUNT");
    PID storageID = storageNode->GetAttributeValueAsInt("ID");

    // check negative item counts to avoid integer overflow
    if(count <= 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "You can't trade this amount of items.");
        return;
    }

    psCharacter* storage;

    if(VerifyStorage(client, character,&storage,"store",itemName,storageID))
    {
        uint32 itemID =(uint32) storageNode->GetAttributeValueAsInt("ITEM_ID");
        psItem* item = character->Inventory().FindItemID(itemID);
        if(!item)
            return;
        if(character->Inventory().GetContainedItemCount(item) > 0)
        {
            psserver->SendSystemError(client->GetClientNum(), "You must take your items out of the container first.");
            return;
        }
        if(!storage->GetActor()->IsAlive())
        {
            psserver->SendSystemError(client->GetClientNum(), "You can't manage your storage with a dead storage owner.");
            return;
        }

        // Check if category didn't exceed the max number of items
        // We allow max 200 items in a category to avoid network size message overflow
        csArray<psItem*> items = client->GetCharacterData()->Inventory().GetItemsInCategory(item->GetCategory(),true);
        if(items.GetSize()>=MAX_ITEMS_IN_CATEGORY)
        {
            psserver->SendSystemError(client->GetClientNum(), "You can't store more than %d stacks in a single category.",MAX_ITEMS_IN_CATEGORY);
            return;
        }

        count = csMin(count, (int)item->GetStackCount());
        csString name(item->GetName());

        item = character->Inventory().RemoveItemID(itemID, count);
        if(item == NULL)
        {
            Error3("RemoveItem failed while selling to merchant %s %i", name.GetDataSafe(), count);
            return;
        }

        // Update client views
        SendPlayerMoney(client, true);
        SendPlayerItems(client, item->GetCategory(), true);

        // move item to player storage
        character->Inventory().AddStorageItem(item);
        item->SetLocInParent(PSCHARACTER_SLOT_STORAGE);
        item->Save(false);

        psserver->SendSystemOK(client->GetClientNum(), "You've stored %d %s.", count, itemName.GetData());

        // Update all client views
        UpdateItemViews(client->GetClientNum());
    }
}

void ServerCharManager::HandleStorageView(psGUIStorageMessage &msg, Client* client)
{
    psCharacter* character = client->GetCharacterData();
    csRef <iDocumentNode> storageNode = ParseStringGetNode(msg.commandData, "V");
    if(!storageNode)
        return;

    csString itemName = storageNode->GetAttributeValue("ITEM");
    PID storageID    = storageNode->GetAttributeValueAsInt("ID");
    uint32 itemID     = (uint32)storageNode->GetAttributeValueAsInt("ITEM_ID");
    int tradeCommand  = storageNode->GetAttributeValueAsInt("TRADE_CMD");

    psCharacter* storage;

    if(VerifyStorage(client, character,&storage,"view",itemName,storageID))
    {
        if(!storage->GetActor()->IsAlive())
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You can't manage your storage with a dead storage owner.");
            return;
        }
        psItem* item;
        if(tradeCommand == psGUIStorageMessage::STORE)
            item = character->Inventory().FindItemID(itemID);
        else
            item = character->Inventory().FindItemID(itemID, true);

        if(!item)
        {
            CPrintf(CON_DEBUG, "Player %s failed to view item %s. No item!\n",
                    client->GetName(), (const char*)itemName);
            return;
        }

        item->SendItemDescription(client);
    }
}


