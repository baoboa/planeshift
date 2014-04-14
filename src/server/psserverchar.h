/*
 * psserverchar.h
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
#ifndef PS_SERVER_CHAR_MANAGER_H
#define PS_SERVER_CHAR_MANAGER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/vfs.h>
#include <csutil/ref.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/slots.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

class MsgEntry;
class Client;
class psItem;
class SlotManager;
class gemActor;
class gemObject;
class psCharacter;

struct psItemCategory;
class psGUIMerchantMessage;
class psGUIStorageMessage;
class psMerchantInfo;


/// Manages character details over the net.
/**
 * This is used instead of cel persistance because the persistance
 * is an all or nothing sort of thing. By using this we can make
 * more efficient use of the net traffic
 */
class ServerCharManager : public MessageManager<ServerCharManager>
{
public:

    ServerCharManager(CacheManager* cachemanager, GEMSupervisor* gemsupervisor);
    virtual ~ServerCharManager();

    bool Initialize();

    /// Sends the client an inventory
    virtual bool SendInventory(uint32_t clientNum, bool sendUpdatesOnly=true);

    void SendPlayerMoney(Client* client, bool storage = false);

    /// Update all views with items
    virtual bool UpdateItemViews(uint32_t clientNum);

    /** Sends out sound messages to the client
     *
     * @todo Modify to be able to send to all the people around client
     */
    void SendOutPlaySoundMessage(uint32_t clientNum, const char* itemsound, const char* action);

    /**
     * Sends out equipment messages to all the people around client.
     *
     * This is used when a player changes their visible equipment and needs
     * to be reflected on other nearby clients. Visible changes can be either
     * new weapons/shields ( new mesh ) or texture changes for clothes ( new material ).
     *
     * @param actor  The actor that has changed equipment.
     * @param slotID To what slot has changed.
     * @param item   The item that is the piece of visible equipment.
     * @param equipped   The equiping type. One of:
     *               psEquipmentMessage::DEEQUIP
     *               psEquipmentMessage::EQUIP
     *
     */
    void SendOutEquipmentMessages(gemActor* actor,
                                  INVENTORY_SLOT_NUMBER slotID,
                                  psItem* item,
                                  int equipped);

    void ViewItem(Client* client, int containerID, INVENTORY_SLOT_NUMBER slotID);

    /**
     * Check if trading is allowed.
     *
     * Will return true if trading is allowed. Returning a reasion in the errorMessage if a pointer is given.
     *
     */
    static bool TradingCheck(Client* client, gemObject* target, csString* errorMessage = NULL);

    /**
     * Start trading.
     */
    void BeginTrading(Client* client, gemObject* target, const csString &type);

    /** Handles a storing beginning.
     *  @param client The client sending us the message.
     *  @param target The target of the storage operation.
     *  @param type ?
     */
    void BeginStoring(Client* client, gemObject* target, const csString &type);

    bool IsBanned(const char* name);

    ///Checked if the character exists still or if it hasn't connected in two months.
    bool HasConnected(csString name);

protected:

    void HandleBookWrite(MsgEntry* me, Client* client);
    void HandleCraftTransInfo(MsgEntry* me, Client* client);
    /// Handles any incoming messages about the character gui.
    void HandleInventoryMessage(MsgEntry* me, Client* client);
    void HandleFaction(MsgEntry* me, Client* client);
    void ViewItem(MsgEntry* me, Client* client);
    void UpdateSketch(MsgEntry* me, Client* client);
    void UpdateMusicalSheet(MsgEntry* me, Client* client);

    // -------------------Merchant Handling -------------------------------
    int CalculateMerchantPrice(psItem* item, Client* client, bool sellPrice);
    bool SendMerchantItems(Client* client, psCharacter* merchant, psItemCategory* category);
    void HandleMerchantMessage(MsgEntry* me, Client* client);
    void HandleMerchantRequest(psGUIMerchantMessage &msg, Client* client);
    void HandleMerchantCategory(psGUIMerchantMessage &msg, Client* client);
    void HandleMerchantBuy(psGUIMerchantMessage &msg, Client* client);
    void HandleMerchantSell(psGUIMerchantMessage &msg, Client* client);
    void HandleMerchantView(psGUIMerchantMessage &msg, Client* client);
    // ------------------Storage Handling----------------------------------
    /** Handles a message coming from the client for making operations with his storage.
     *  @param me The message received from the client.
     *  @param client The client sending us the message.
     */
    void HandleStorageMessage(MsgEntry* me, Client* client);

    /**
     * Sends to the client the stored items for this category.
     *
     * @param client The client sending us the message.
     * @param character merchant ?????
     * @param category The category we are browsing
     * @return Always TRUE.
     */
    bool SendStorageItems(Client* client, psCharacter* character, psItemCategory* category);

    /** Handles the request to access the storage from the player.
     *  @param msg The cracked message received from the client.
     *  @param client The client sending us the message.
     */
    void HandleStorageRequest(psGUIStorageMessage &msg, Client* client);
    /** Handles the request to change category from the player.
     *  @param msg The cracked message received from the client.
     *  @param client The client sending us the message.
     */
    void HandleStorageCategory(psGUIStorageMessage &msg, Client* client);
    /** Handles the request to withdraw an item from the NPC.
     *  @param msg The cracked message received from the client.
     *  @param client The client sending us the message.
     */
    void HandleStorageWithdraw(psGUIStorageMessage &msg, Client* client);
    /** Handles the request to store an item from the player.
     *  @param msg The cracked message received from the client.
     *  @param client The client sending us the message.
     */
    void HandleStorageStore(psGUIStorageMessage &msg, Client* client);
    /** Handles the request to check informations about a stored item.
     *  @param msg The cracked message received from the client.
     *  @param client The client sending us the message.
     */
    void HandleStorageView(psGUIStorageMessage &msg, Client* client);
    // --------------------------------------------------------------------

    bool SendPlayerItems(Client* client, psItemCategory* category, bool storage);


    /// Return true if all trade params are ok
    bool VerifyTrade(Client* client, psCharacter* character, psCharacter** merchant, psMerchantInfo** info,
                     const char* trade, const char* itemName, PID merchantID);

    /// Return true if all storage params are ok
    bool VerifyStorage(Client* client, psCharacter* character, psCharacter** storage,
                       const char* trade, const char* itemName, PID storageID);

    /// verifies that item dropped in mind slot is a valid goal
    bool VerifyGoal(Client* client, psCharacter* character, psItem* goal);

//    ClientConnectionSet*    clients;

    SlotManager* slotManager;
    CacheManager* cacheManager;
    GEMSupervisor* gemSupervisor;

    MathScript* calc_item_merchant_price_buy;
    MathScript* calc_item_merchant_price_sell;
};

#endif


