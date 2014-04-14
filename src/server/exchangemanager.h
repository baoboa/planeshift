/*
 * exchangemanager.h
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
#ifndef __EXCHANGEMANAGER_H__
#define __EXCHANGEMANAGER_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/refarr.h>
#include <csutil/parray.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/pscharinventory.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class

class ClientConnectionSet;
class psSpawnManager;
class psServer;
class Exchange;
class ExchangeManager;
class psNPCDialog;
class psDatabase;

/**
 * \addtogroup server
 * @{ */

/**
 * ExchangingCharacter holds relevant information about character that is
 * participating in exchange of items with another character.
 */
class ExchangingCharacter
{
public:
    ExchangingCharacter(int client, psCharacterInventory &inv);

    /**
     * Inserts item to position 'itemNum'.
     *
     * If there is another item at this position already, it tries to stack them.
     *
     * @return -1 on failure otherwise the number of remaining items
     */
    bool OfferItem(int exchSlotNum, INVENTORY_SLOT_NUMBER invSlot, int stackcount, bool test);

    /**
     * Removes items from position 'itemNum'.
     *
     * Can return NULL if there is no item.
     * Returned item instance now belongs to caller.
     *
     * @param itemNum The position to remove from.
     * @param count Count=-1 means take everything.
     * @param remain [CHANGES]  The number of items left in the slot ( if any )
     */
    bool RemoveItemFromOffer(int itemNum, int count, int &remain);

    bool MoveOfferedItem(int fromSlot,int stackcount,int toSlot);

    /**
     * Make sure all offered items still have legal stack counts.
     */
    bool IsOfferingSane();

    /**
     * Transfers offered items and money to character 'target'.
     *
     * Target can be 0 - in this case everything is just destroyed.
     */
    void TransferOffer(int targetClientNum);

    /**
     * Transfers offered items and money to an NPC 'npc'.
     *
     */
    void TransferOffer(psCharacter* npc);

    /**
     * Transfers the trias from this exchanging player to a different player.
     *
     * @param targetClientNum The client id of the client that should be
     *                        receiving this characters offering money.
     */
    void TransferMoney(int targetClientNum);

    /**
     * Transfers the trias from this exchanging player to another character.
     */
    void TransferMoney(psCharacter* target);

    psMoney GetOfferedMoney()
    {
        return offeringMoney;
    }

    /**
     * Change the amount of money this character is offering in the exchange.
     *
     * @param coin Is one of the 4 coin types.  Defined in common/rpgrules/psmoney.h
     * @param delta The amount to change by. Can be +/-
     */
    void AdjustMoney(int coin, int delta);
    void AdjustMoney(const psMoney &amount);

    void GetOfferingCSV(csString &buff);

    /**
     * Constructs an XML list of the items that are being offered.
     *
     * The format of the xml is:
     * \<L MONEY=psMoney.ToString\>
     *     \<ITEM N=name SLT_POS=slotposition C=count WT=weight IMG=icon image\>
     *      ...
     * \</L\>
     *
     * @param buff [CHANGES] Is where the resulting xml message is built.
     */
    void GetOffering(csString &buff);

    psItem* GetOfferedItem(int slot);

    /**
     * Constructs a simple XML list of the items that are being offered.
     *
     * The format of the xml is when exact:
     * \<L MONEY="psMoney.ToString"\>\<ITEM N=name C=count Q=quality /\>...\</L\>
     * else:
     * \<L MONEY="0,0,0,psMoney.GetTotal"\>\<ITEM N=name C=count Q=quality /\>...\</L\>
     *
     * @param buff [CHANGES] Is where the resulting xml message is built.
     * @param chr The character
     * @param exact Should the offer be exactly
     */
    void GetSimpleOffering(csString &buff, psCharacter* chr, bool exact = true);

    /**
     * Update the money box for money that will be received by this character.
     */
    void UpdateReceivingMoney(psMoney &money);

    /**
     * Update the money box for money that is being offered by this character.
     */
    void UpdateOfferingMoney();

    psMoney offeringMoney;

    /**
     * Gets the total size of all the items that are offered.
     */
    float GetSumSize();

    /**
     * Gets the total weight of all the items that are offered.
     */
    float GetSumWeight();

    bool GetExchangedItems(csString &text);

protected:

    psCharacter* GetClientCharacter(int clientNum);

    /**
     * Stuff that was removed from character inventory and offered to the other character
     */
//    psItem *offering[EXCHANGE_SLOT_COUNT];
    // csArray<psItem*> itemsOffered;

    int client;
    psCharacterInventory* chrinv;
};

//------------------------------------------------------------------------------

/**
 * A basic exchange.
 *
 * This shares the qualities of any exchange and stores the
 * players involved as well as functionality to start/stop the exchange.
 */
class Exchange : public iDeleteObjectCallback
{
public:
    Exchange(Client* starter, bool automaticExchange, ExchangeManager* manager);
    virtual ~Exchange();

    /**
     * One of the clients has ended this exchange.
     *
     * This will close the exchange
     * on both the clients without transfer of items.
     * @param client The client that has ended the exchange.
     */
    virtual void HandleEnd(Client* client) = 0;

    /**
     * One of the client has accepted the exchange.
     *
     * If both clients have accepted
     * the exchange will end and the items will be exchanged.
     * @param client The client that has accepted the exchange.
     * @return true If the exchange has ended.
     */
    virtual bool HandleAccept(Client* client) = 0;
    // virtual bool Involves(psCharacter * character) = 0;
    // virtual bool Involves(psItem * item) = 0;

    /**
     * Add a new item to the exchange.
     *
     * Will add to the offering slot of the person that
     * offered it an the receiving slot of the other person. Sends out the relevant
     * network messages.
     *
     * @param fromClient The client that this item came from.
     * @param fromSlot The originating slot in the exchange window for the item moved.
     * @param stackCount The number of items being moved.
     * @param toSlot Where in the exchange the item is to be placed.
     */
    virtual bool AddItem(Client* fromClient, INVENTORY_SLOT_NUMBER fromSlot, int stackCount, int toSlot);

    /**
     * Move an item from one slot to another in the exchange.
     *
     * Will swap items if another
     * item is already in the destination slot. Sends out the relevant
     * network messages.
     *
     * @param client The client that this item came from.
     * @param fromSlot The originating slot in the exchange window for the item moved.
     * @param stackCount The number of items being moved.
     * @param toSlot Where in the exchange the item is to be placed.
     */
    virtual void MoveItem(Client* client, int fromSlot, int stackCount, int toSlot);

    /**
     * Removes an item from the exchange.
     *
     * Sends out the network messages to update
     * the clients' views.
     *
     * @param fromClient The client that has removed an item.
     * @param slot The slot that the items were removed from.
     * @param stackCount The amount to remove from that slot.
     *
     * @return A new psItem for the items removed.
     */
    virtual bool RemoveItem(Client* fromClient, int slot, int stackCount);

    virtual psMoney GetOfferedMoney(Client* client);
    virtual bool AdjustMoney(Client* client, int moneyType, int newMoney);
    virtual bool AdjustMoney(Client* client, const psMoney &money);
    int GetID() const
    {
        return id;
    }
    bool CheckRange(int clientNum, gemObject* ourActor, gemObject* otherActor);

    Client* GetStarterClient()
    {
        return starterClient;    // NILAYA: Should axe.
    }

    psItem* GetStarterOffer(int slot);
    virtual psItem* GetTargetOffer(int slot)
    {
        return NULL;
    }

protected:
    virtual void SendAddItemMessage(Client* fromClient, int slot, psCharacterInventory::psCharacterInventoryItem* item);
    virtual void SendRemoveItemMessage(Client* fromClient, int slot);

    void SendEnd(int clientNum);
    static int next_id;

    /** unique exchange ID */
    int id;

    /** Information about the player that initiated the exchange */
    ExchangingCharacter starterChar;
    Client* starterClient;
    uint32_t player;

    bool exchangeEnded;    ///< exchange ended and should be deleted
    bool exchangeSuccess; ///< exchange was successful and items should not be returned to owners

    bool automaticExchange; ///< the exchange is done entirely server side don't open windows on the client. This is used only with NPC!

    ExchangeManager* exchangeMgr;
};

//------------------------------------------------------------------------------

class PlayerToPlayerExchange : public Exchange
{
public:
    PlayerToPlayerExchange(Client* client, Client* target, ExchangeManager* manager);
    virtual ~PlayerToPlayerExchange();

    bool CheckExchange(uint32_t clientNum, bool checkRange=false);
    bool HandleAccept(Client* client);
    void HandleEnd(Client* client);
    void SendExchangeStatusToBoth();
    psMoney GetOfferedMoney(Client* client);
    bool AdjustMoney(Client* client, int moneyType, int newMoney);
    bool AdjustMoney(Client* client, const psMoney &money);

    /**
     * Add an offering item from a client.
     *
     * @param fromClient The client that is offering the item.
     * @param fromSlot The originating slot in the exchange window for the item moved.
     * @param stackCount The number of items being moved.
     * @param toSlot The slot that the item should go in.
     *
     * @return True if the item was added to the exchange correctly.
     */
    virtual bool AddItem(Client* fromClient, INVENTORY_SLOT_NUMBER fromSlot, int stackCount, int toSlot);

    /**
     * Removes an item from the exchange.
     *
     * Sends out the network messages to update
     * the clients' views.
     *
     * @param fromClient The client that has removed an item.
     * @param slot The slot that the items were removed from.
     * @param stackCount The amount to remove from that slot.
     *
     * @return A new psItem for the items removed.
     */
    virtual bool RemoveItem(Client* fromClient, int slot, int stackCount);

    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    virtual psItem* GetTargetOffer(int slot);

protected:
    virtual void SendAddItemMessage(Client* fromClient, int slot, psCharacterInventory::psCharacterInventoryItem* item);
    virtual void SendRemoveItemMessage(Client* fromClient, int slot);

private:
    /// Sends a message to both clients that an exchange has started.
    void SendRequestToBoth();

    Client* GetOtherClient(Client* client) const;

    PlayerToPlayerExchange();

    /** Information about the player that has been asked to exchange by another player*/
    Client* targetClient;
    ExchangingCharacter targetChar;
    int target;

    bool starterAccepted, targetAccepted;
};

//------------------------------------------------------------------------------

class PlayerToNPCExchange : public Exchange
{
public:
    PlayerToNPCExchange(Client* starter, gemObject* target, bool automaticExchange, int questID, ExchangeManager* manager);
    virtual ~PlayerToNPCExchange();
    gemObject* GetTargetGEM();
    bool CheckExchange(uint32_t clientNum, bool checkRange=false);
    virtual void HandleEnd(Client* client);
    virtual bool HandleAccept(Client* client);
    //virtual bool Involves(psCharacter * character);
    //virtual bool Involves(psItem * item);
    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

protected:
    /**
     * A check to se if a exchange trigger trigg any response from the npc
     *
     * @param client The client
     * @param dlg    The dialog of the selected NPC
     * @param trigger A XML string of the items exchanged.
     * @return Return true if a trigger was run with sucess.
     */
    bool CheckXMLResponse(Client* client, psNPCDialog* dlg, csString trigger);

    gemObject* target;
    int questID;
};

/* Maintains a list of all the exchanges that are ongoing.
 */
class ExchangeManager : public MessageManager<ExchangeManager>
{
public:

    ExchangeManager(ClientConnectionSet* pCCS);
    virtual ~ExchangeManager();

    static bool ExchangeCheck(Client* client, gemObject* target, csString* errorMessage = NULL);

    void StartExchange(Client* client, bool withPlayer, bool automaticExchange = false, int questID = -1);

    void HandleExchangeRequest(MsgEntry* me,Client* client);
    void HandleExchangeAccept(MsgEntry* me,Client* client);
    void HandleExchangeEnd(MsgEntry* me,Client* client);
    void HandleAutoGive(MsgEntry* me,Client* client);

    /** Utility function to handle exchange objects */
    Exchange* GetExchange(int id);

    /** Adds given exchange to the exchange manager's exchanges list */
    void AddExchange(Exchange* exchange)
    {
        exchanges.Push(exchange);
    }

    /** Removes given exchange from the exchange manger's exchanges list */
    void DeleteExchange(Exchange* exchange);

protected:

    ClientConnectionSet* clients;

    csPDelArray<Exchange> exchanges;
};

/** @} */

#endif
