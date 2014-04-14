/*
* minigamemanager.h - Author: Enar Vaikene
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __MINIGAMEMANAGER_H
#define __MINIGAMEMANAGER_H

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/weakref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psminigameboard.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "deleteobjcallback.h"
#include "msgmanager.h"             // Parent class

class Client;
class MiniGameManager;
class gemActionLocation;
class gemObject;
class psMGUpdateMessage;

/**
 * \addtogroup server
 * @{ */

enum MinigameStyle
{
    MG_GAME,    ///< normally 2 or more player mini-game
    MG_PUZZLE   ///< single player mini-game
};

/**
 * Structure to hold player data.
 */
struct MinigamePlayer
{
    uint32_t playerID;
    const char* playerName;

    /// Idle counter for the player with black pieces.
    int idleCounter;

    /// point to next player to move
    MinigamePlayer* nextMover;

    /// identifies colour pieces for player TODO this is temp
    int blackOrWhite;
};

/**
 * Implements one minigame session.
 *
 * Game sessions are bound to a game board (action location) and identified
 * by a unique name. The name of the game board is defined in the action_locations table (name field).
 *
 * The game itself is defined in the gameboards db table.
 * gameboard options include:
 * - name : the name of the board
 * - layout : the layout of the board in terms of tiles and shape
 * - numColumns, numRows : the size x,y of the full board
 * - numPlayers: defines if it's a single player or multiplayer minigame
 * - gameboardOptions: White/Black (colors the board white or black), Plain (makes it one color), Checked (checkerboard)
 * - gameRules:
 *     PlayerTurns can be 'Ordered' (order of players' moves enforced)
 *              or 'StrictOrdered' (as Ordered, and all players must be present)
 *              or 'Relaxed' (default - free for all).
 *     MoveType can be 'MoveOnly' (player can only move existing pieces),
 *              or 'PlaceOnly' (player can only place new pieces on the board; cant move others),
 *              or 'PlaceOrMovePiece' (default - either move existing or place new pieces).
 *     MoveDistance can be an integer number
 *     MoveDirection can be 'Vertical', 'Horizontal', 'Cross', 'Diagonal'
 *     MoveablePieces can be 'Own' (player can only move their own pieces)
 *                    or 'Any' (default - player can move any piece in play).
 *     MoveTo can be 'Vacancy' (player can move pieces to vacant squares only)
 *            or 'Anywhere' (default - can move to any square, vacant or occupied).
 * - endgames:
 *  <MGEndGame>
 *   <EndGame Coords="relative"/"absolute" [SourceTile="T"] [Winner="T"]>
 *    <Coord Col="0-15" Row="0-15" Tile="T" [Piece="X"] />
 *   </EndGame>
 *  </MGEndGame>
 *  where T : A = any valid piece / W = any white piece / B = any black piece
 *            E = empty tile / F = follows source piece / S = specific pieces
 *  Each <EndGame> has 1 or more <Coord> spec.
 *  Each <MGEndGame> has 1 or more <EndGame>.
 *  Endgames can be left blank.
 *
 * The response string specifies the name to the record in gameboards
 * and an optional prepared layout & is expected to have the
 * following format:
 * \<Examine\>
 *  \<GameBoard Name='gameboard name' [Layout='board layout'] [Session='personal'|'public'] [EndGame='yes'|'no'] /\>
 *  \<Description\>Description as seen by players\</Description\>
 * \</Examine\>
 *
 * The Layout attribute defines the layout of the game board and optionally also preset game
 * pieces on it. Optional - to override the default set in the database.
 *
 * The Session attribute allows a game to be personal (restricted to one-player games)
 * whereby only the player sees the game, no other players or watchers. One such
 * minigame at one action_location can spawn a session per player who plays their own
 * private game. Set to public is for the traditional board game, and is the default if
 * this option is omitted.
 *
 * The EndGame attribute specifies whether this game session is to adhere to the end-game rules.
 * Optional. Default is No.
 *
 * In the future we will add more attributes like for game options and name of a plugin or
 * script for managed games.

 * Every session can have 0, 1, 2 or more players attached to it. The first player
 * gets white game pieces and the second player black game pieces. All other players
 * can just watch the game.
 */
class psMiniGameSession : public iDeleteObjectCallback
{
public:

    psMiniGameSession(MiniGameManager* mng, gemActionLocation* obj, const char* name);

    ~psMiniGameSession();

    /// Returns the game session ID
    const uint32_t GetID() const
    {
        return id;
    }

    /// Returns the session name.
    const csString &GetName() const
    {
        return name;
    }

    /// Returns the game options.
    uint16_t GetOptions() const
    {
        return options;
    }

    /**
     * Loads the game.
     *
     * The Load() function loads the game from the given action location response string.
     *
     * @param[in] responseString The string with minigame options and board layout.
     * @return Returns true if the game was loaded and false if not.
     */
    bool Load(csString &responseString);

    /// Restarts the game.
    void Restart();

    /// Adds a player to the session.
    void AddPlayer(Client* client);

    /// Removes a player from the session.
    void RemovePlayer(Client* client);

    /// Returns true if it is valid for this player to update the game board.
    bool IsValidToUpdate(Client* client) const;

    /// Updates the game board. NB! Call IsValidToUpdate() first to verify that updating is valid.
    void Update(Client* client, psMGUpdateMessage &msg);

    /**
     * Sends the current game board to the given player.
     *
     * @param[in] clientID The ID of the client.
     * @param[in] modOptions Modifier for game options.
     */
    void Send(uint32_t clientID, uint32_t modOptions);

    /// Broadcast the current game board layout to all the players/watchers.
    void Broadcast();

    /// Handles disconnected players.
    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    /// Checks for idle players and players too far away
    void Idle();

    /// Checks if game is actually active at all
    bool GameSessionActive(void);

    /// Sets session to be reset (i.e. deleted and restarted next play)
    void SetSessionReset(void)
    {
        toReset = true;
    }

    /// Gets reset status of session
    bool GetSessionReset(void)
    {
        return toReset;
    }

    /// returns whether this is a public session or not.
    bool IsSessionPublic(void);

protected:

    /// Game manager.
    MiniGameManager* manager;

    /// The game session ID (equals to the action location ID)
    uint32_t id;

    /// The game session name.
    csString name;

    /// Action location object
    csWeakRef<gemObject> actionObject;

    /// Game options
    uint16_t options;

    /// Minigame players
    csPDelArray<MinigamePlayer> players;

    /// Watchers.
    csArray<uint32_t> watchers;

    /// Current message counter for versionin.
    uint8_t currentCounter;

    /// The current game board.
    psMiniGameBoard gameBoard;

    /// if game session marked for reset
    bool toReset;

    /// pointer to next player to move
    MinigamePlayer* nextPlayerToMove;

private:
    /// flag for when end game reached (i.e. game over).
    bool endgameReached;

    /// count of players in session
    uint8_t playerCount;

    /// progression script for winner
    csString winnerScript;

    /// style of minigame
    MinigameStyle minigameStyle;

    /// parameters to pass to a script
    csString paramZero;
    csString paramOne;
    csString paramTwo;

    /// resend board layouts as was, e.g. correcting an illegal move
    void ResendBoardLayout(MinigamePlayer* player);

    /// Game rule checking function
    bool GameMovePassesRules(MinigamePlayer* player,
                             int8_t col1, int8_t row1, int8_t state1,
                             int8_t col2=-1, int8_t row2=-1, int8_t state2=-1);
};


/**
 * Handles minigame sessions.
 *
 * This is the manager class of mini-games, handling overall mini-game definitions,
 * action-location gameboards and game sessions.
 */
class MiniGameManager : public MessageManager<MiniGameManager>
{

public:

    MiniGameManager();

    ~MiniGameManager();

    /// returns session by its id.
    psMiniGameSession* GetSessionByID(uint32_t id);

    /// Idle function to check for idle players and players too far away.
    void Idle();

    /// Initialise Mini Game manager
    bool Initialise();

    /// Find & return game board definition
    psMiniGameBoardDef* FindGameDef(csString gameName);

    /// reset all sessions, prob due to reloading action locations
    void ResetAllGameSessions(void);

protected:

    /// Handle start and stop messages
    void HandleStartStop(MsgEntry* me, Client* client);

    /// handles a client that has made a move.
    void HandleGameUpdate(MsgEntry* me, Client* client);

    /// client requests start of game session.
    void HandleStartGameRequest(Client* client);

    /// client handles stopping of game session.
    void HandleStopGameRequest(Client* client);

    /// a client is removed from a game session.
    void RemovePlayerFromSessions(psMiniGameSession* session, Client* client, uint32_t clientID);

    /// requests a session to be reset.
    void ResetGameSession(psMiniGameSession* sessionToReset);

    /// function parses game options string from gameboards DB table.
    int16_t ParseGameboardOptions(psString optionsStr);
    /// Game sessions.
    csPDelArray<psMiniGameSession> sessions;

    /// Maps players to game sessions for quicker access.
    csHash<psMiniGameSession*, uint32_t> playerSessions;

    /// Minigame board definitions by name
    csHash<psMiniGameBoardDef*, csString> gameBoardDef;
};

/** @} */

#endif
