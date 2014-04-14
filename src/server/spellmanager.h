/*
 * spellmanager.h by Anders Reggestad <andersr@pvv.org>
 *
 * Copyright (C) 2001-2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __SPELLMANAGER_H__
#define __SPELLMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/gameevent.h"

//=============================================================================
// Server includes
//=============================================================================
#include "msgmanager.h"   // Subscriber class

//=============================================================================
// Forward Declarations
//=============================================================================

class ClientConnectionSet;
class psSpell;

/**
 * Manager class that handles loading/searching/casting spells.
 *
 * This class also manages a number of psSpell Events.
 */
class SpellManager : public MessageManager<SpellManager>
{
public:

    SpellManager(ClientConnectionSet* clients,
                 iObjectRegistry* object_reg,
                 CacheManager* cachemanager);
    virtual ~SpellManager();

    /**
     * Purifying on a glyph has been complete.
     *
     *  This will send out a network message to the client and update it's inventory
     *  with the new purified glyph.
     *
     *  @param character The character this is for.
     *  @param glyphUID  The unique ID for this item instance of the glyph.
     */
    void EndPurifying(psCharacter* character, uint32 glyphUID);

    /**
     * Sends out the glyphs to a client.
     *
     * Builds and sends psRequestGlyphsMessage for the client.
     *
     * @param notused A not used message entry.
     * @param client  The client that will be sent it's current glyphs.
     */
    void SendGlyphs(MsgEntry* notused, Client* client);

    /**
     * Handles a glyph request from a client.
     *
     * @param notused A not used message entry.
     * @param client  The client that requested glyphs.
     */
    void HandleGlyphRequest(MsgEntry* notused, Client* client);

protected:
    /**
     * Save a spell to the database for when a player has researched it.
     *
     * @param client The client that this is for.
     * @param spellName The name of the spell to save for that player.
     */
    void SaveSpell(Client* client, csString spellName);

    /**
     * Case a particular spell.
     *
     * @param me message entry for the client spell caster message.
     * @param client the clien that cast the spell.
     */
    void Cast(MsgEntry* me, Client* client);

public:
    /**
     * Case a particular spell.
     *
     * @param caster The caster of the spell.
     * @param spellName The name of the spell to cast.
     * @param kFactor The power factor that the spell is cast with.
     * @param client The client that is casting the spell.
     */
    void Cast(gemActor* caster, const csString &spellName, float kFactor, Client* client);

protected:

    void HandleCancelSpell(MsgEntry* notused, Client* client);

    /**
     * Send the player's spell book.
     *
     * @param notused A not used message entry.
     * @param client  The client that will be sent the spell book.
     */
    void SendSpellBook(MsgEntry* notused, Client* client);

    /**
     * Start to purify a glyph.
     *
     * This will also send out notifications to the client about the start of operation.
     *
     * @param me     The inncomming message.
     * @param client The client that this data is for.
     */
    void StartPurifying(MsgEntry* me, Client* client);

    /**
     * Find a spell in the assorted glyphs.
     *
     * This checks ths list of glyphs and see if it matches any
     * known spell. This is for when players are researching spells.
     *
     * @param client The client this data is for.
     * @param assembler A list of glyphs to check for spell match.
     *
     * @return A spell if a match found, NULL otherwise.
     */
    psSpell* FindSpell(Client* client, const glyphList_t &assembler);

    /**
     * Handles a command when player tries to research.
     *
     * @param client The client this is for.
     * @param me The message from that client.
     */
    void HandleAssembler(MsgEntry* me,Client* client);

    ClientConnectionSet* clients;
    iObjectRegistry* object_reg;
    CacheManager* cacheManager;
};

#endif
