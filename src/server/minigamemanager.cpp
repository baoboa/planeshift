/*
* minigamemanager.cpp - Author: Enar Vaikene
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

#include <psconfig.h>
#include <ctype.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/eventmanager.h"
#include "util/minigame.h"
#include "util/psxmlparser.h"
#include "util/psdatabase.h"

#include "bulkobjects/psactionlocationinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "minigamemanager.h"
#include "psserver.h"
#include "netmanager.h"
#include "client.h"
#include "clients.h"
#include "chatmanager.h"
#include "gem.h"
#include "globals.h"
#include "progressionmanager.h"
#include "scripting.h"

using namespace psMiniGame;


//#define DEBUG_MINIGAMES

/// Tick interval (10 seconds)
#define MINIGAME_TICK_INTERVAL  10*1000

/// Maximum idle time for players (10 minutes)
#define MINIGAME_IDLE_TIME      60
//#define MINIGAME_IDLE_TIME      6

// TODO these are temp
#define WHITE_PLAYER 0
#define BLACK_PLAYER 1

//---------------------------------------------------------------------------

class MiniGameManagerTick : public psGameEvent
{
public:

    MiniGameManagerTick(int offset, MiniGameManager* m);
    virtual void Trigger();

protected:

    MiniGameManager* manager;
};

MiniGameManagerTick::MiniGameManagerTick(int offset, MiniGameManager* m)
    : psGameEvent(0, offset, "psMiniGameManagerTick")
{
    manager = m;
}

void MiniGameManagerTick::Trigger()
{
    manager->Idle();
}


//---------------------------------------------------------------------------

MiniGameManager::MiniGameManager()
{
    Subscribe(&MiniGameManager::HandleStartStop, MSGTYPE_MINIGAME_STARTSTOP, REQUIRE_READY_CLIENT);
    Subscribe(&MiniGameManager::HandleGameUpdate, MSGTYPE_MINIGAME_UPDATE, REQUIRE_READY_CLIENT);

    MiniGameManagerTick* tick = new MiniGameManagerTick(MINIGAME_TICK_INTERVAL, this);
    psserver->GetEventManager()->Push(tick);
}

MiniGameManager::~MiniGameManager()
{
    csHash<psMiniGameBoardDef*, csString>::GlobalIterator iter(gameBoardDef.GetIterator());
    while(iter.HasNext())
        delete iter.Next();
}

bool MiniGameManager::Initialise(void)
{
    // Load gameboard definitions
    psMiniGameBoardDef* gameDef;
    Result gameboards(db->Select("SELECT * from gameboards"));
    if(gameboards.IsValid())
    {
        int i, count = gameboards.Count();
        for(i=0; i<count; i++)
        {
            csString name(gameboards[i]["name"]);
            const char* layout = gameboards[i]["layout"];
            const char* pieces = gameboards[i]["pieces"];
            const uint8_t cols = gameboards[i].GetInt("numColumns");
            const uint8_t rows = gameboards[i].GetInt("numRows");
            int8_t players = gameboards[i].GetInt("numPlayers");
            psString optionsStr(gameboards[i]["gameboardOptions"]);
            csString rulesXML(gameboards[i]["gameRules"]);
            csString endgameXML(gameboards[i]["endgames"]);
            // verify basics of definition
            if(name && layout && pieces &&
                    cols >= GAMEBOARD_MIN_COLS && cols <= GAMEBOARD_MAX_COLS &&
                    rows >= GAMEBOARD_MIN_ROWS && rows <= GAMEBOARD_MAX_ROWS &&
                    strlen(layout) == size_t(rows * cols))
            {
                if(players < GAMEBOARD_MIN_PLAYERS || players > GAMEBOARD_MAX_PLAYERS)
                {
                    Error2("Minigames GameBoard definition %d invalid num players.", i+1);
                    players = GAMEBOARD_DEFAULT_PLAYERS;
                }

                // decipher board layout options
                int16_t options = ParseGameboardOptions(optionsStr);

                gameDef = new psMiniGameBoardDef(cols, rows, layout, pieces, players, options);
                gameBoardDef.Put(name.Downcase(), gameDef);

                gameDef->DetermineGameRules(rulesXML, name);
                if(!gameDef->DetermineEndgameSpecs(endgameXML, name))
                {
                    gameDef->ClearOutEndgames();
                }
            }
            else
            {
                Error2("Minigames GameBoard definition %d invalid.", i+1);
                return false;
            }
        }

        return true;
    }

    return false;
}

void MiniGameManager::HandleStartStop(MsgEntry* me, Client* client)
{
    psMGStartStopMessage msg(me);
    if(!msg.valid)
    {
        Error2("Failed to parse psMGStartStopMessage from client %u.", me->clientnum);
        return;
    }

    if(msg.msgStart)
    {
        HandleStartGameRequest(client);
    }
    else
    {
        HandleStopGameRequest(client);
    }
}

void MiniGameManager::HandleStartGameRequest(Client* client)
{
    if(!client || client->GetClientNum() == (uint32_t)-1)
    {
        Error1("Invalid client in the start minigame request.");
        return;
    }

    uint32_t clientID = client->GetClientNum();

    // Verify the target
    gemObject* target = client->GetTargetObject();
    if(!target || (target->GetALPtr() == NULL))
    {
        psserver->SendSystemError(clientID, "You don't have a game board targeted!");
        return;
    }

    // Object type was verified before, so static cast should be safe here
    gemActionLocation* action = static_cast<gemActionLocation*>(target);
    psActionLocation* actionLocation = action->GetAction();
    if(!actionLocation || !actionLocation->IsGameBoard())
    {
        psserver->SendSystemError(clientID, "You don't have a game board targeted!");
        return;
    }

    // Check the range
    if(client->GetActor()->RangeTo(action, true) > RANGE_TO_LOOT)
    {
        psserver->SendSystemError(clientID, "You are not in range for %s!", actionLocation->name.GetData());
        return;
    }

    // Remove from existing game sessions.
    RemovePlayerFromSessions(playerSessions.Get(clientID, 0), client, clientID);

    // Find an existing session or create a new one.
    // If personal minigame, force new one for player.
    psMiniGameSession* session = GetSessionByID((uint32_t)actionLocation->id);
    if(!session || (session && !session->IsSessionPublic()))
    {
        // Create a new minigame session.
        session = new psMiniGameSession(this, action, actionLocation->name);

        // Try to load the game.
        if(session->Load(actionLocation->response))
        {
            sessions.Push(session);

#ifdef DEBUG_MINIGAMES
            Debug2(LOG_ANY, 0, "Created new minigame session %s", session->GetName().GetData());
#endif

        }
        else
        {
            delete session;
            psserver->SendSystemError(clientID, "Failed to load the game %s", actionLocation->name.GetData());
            return;
        }
    }

    // Add to the player/session map for quicker access.
    playerSessions.Put(clientID, session);

    // And finally add to the session
    session->AddPlayer(client);

}

void MiniGameManager::HandleStopGameRequest(Client* client)
{
    if(!client || client->GetClientNum() == (uint32_t)-1)
    {
        Error1("Invalid client in the stop minigame request.");
        return;
    }

    uint32_t clientID = client->GetClientNum();

    // Remove from existing game sessions.
    RemovePlayerFromSessions(playerSessions.Get(clientID, 0), client, clientID);
}

void MiniGameManager::RemovePlayerFromSessions(psMiniGameSession* session, Client* client, uint32_t clientID)
{
    if(session)
    {
        session->RemovePlayer(client);
        playerSessions.DeleteAll(clientID);

        if(session->GetSessionReset())
        {
            ResetGameSession(session);
        }

        // if session is personal, delete it.
        if(!session->IsSessionPublic())
        {
            sessions.Delete(session);
        }
    }
}

void MiniGameManager::HandleGameUpdate(MsgEntry* me, Client* client)
{
    psMGUpdateMessage msg(me);
    if(!msg.valid)
    {
        Error2("Failed to parse psMGUpdateMessage from client %u.", me->clientnum);
        return;
    }

    if(!client || client->GetClientNum() == (uint32_t)-1)
    {
        Error1("Invalid client in the minigame update message.");
        return;
    }

    uint32_t clientID = client->GetClientNum();

    // Find the session
    psMiniGameSession* session = playerSessions.Get(clientID, 0);
    if(!session)
    {
        Error2("Received minigame update from client %u without an active session.", clientID);
        return;
    }

    // Verify the session
    if(!session->IsValidToUpdate(client))
    {
        session->Send(clientID, READ_ONLY);
        return;
    }

    // Apply updates
    session->Update(client, msg);
}

void MiniGameManager::ResetAllGameSessions(void)
{
    for(size_t i = 0; i < sessions.GetSize(); i++)
    {
        ResetGameSession(sessions.Get(i));
    }
}

psMiniGameSession* MiniGameManager::GetSessionByID(uint32_t id)
{
    for(size_t i = 0; i < sessions.GetSize(); i++)
    {
        if(sessions.Get(i)->GetID() == id)
        {
            return sessions.Get(i);
        }
    }
    return 0;
}

void MiniGameManager::Idle()
{
    for(size_t i = 0; i < sessions.GetSize(); i++)
    {
        sessions.Get(i)->Idle();
    }

    MiniGameManagerTick* tick = new MiniGameManagerTick(MINIGAME_TICK_INTERVAL, this);
    psserver->GetEventManager()->Push(tick);
}

psMiniGameBoardDef* MiniGameManager::FindGameDef(csString gameName)
{
    return gameBoardDef.Get(gameName.Downcase(), NULL);
}

void MiniGameManager::ResetGameSession(psMiniGameSession* sessionToReset)
{
    if(!sessionToReset)
    {
        Error1("Attempt to reset NULL minigame session");
        return;
    }

    if(sessionToReset->GameSessionActive())
    {
        sessionToReset->SetSessionReset();
    }
    else
    {
        if(!sessions.Delete(sessionToReset))
        {
            Error1("Could not remove minigame session");
        }
    }
}

int16_t MiniGameManager::ParseGameboardOptions(psString optionsStr)
{
    uint16_t localOptions = 0;

    // look for black/white. White by default.
    if(optionsStr.FindSubString("black",0,true) != -1)
    {
        localOptions |= BLACK_SQUARE;
    }

    // look for checked/plain. Checked by default.
    if(optionsStr.FindSubString("plain",0,true) != -1)
    {
        localOptions |= PLAIN_SQUARES;
    }

    // parse the options string
    return localOptions;
}

//---------------------------------------------------------------------------

psMiniGameSession::psMiniGameSession(MiniGameManager* mng, gemActionLocation* obj, const char* name)
{
    manager = mng;
    actionObject = obj;
    id = (uint32_t)obj->GetAction()->id;
    this->name = name;
    currentCounter = 0;
    toReset = false;
    nextPlayerToMove = NULL;
    endgameReached = false;
    playerCount = 0;
    winnerScript.Clear();
}

psMiniGameSession::~psMiniGameSession()
{

    // Unregister pending call-backs. Right now clients are disconnected before deleting game
    // sessions, but it might be changed in the future.
    ClientConnectionSet* clients = psserver->GetConnections();
    if(clients)
    {
        csPDelArray<MinigamePlayer>::Iterator pIter = players.GetIterator();
        while(pIter.HasNext())
        {
            MinigamePlayer* p = pIter.Next();
            if(p && p->playerID != (uint32_t)-1)
            {
                Client* client = clients->Find(p->playerID);
                if(client && client->GetActor())
                {
                    client->GetActor()->UnregisterCallback(this);
                }
            }
        }
        players.DeleteAll();

        csArray<uint32_t>::Iterator iter = watchers.GetIterator();
        while(iter.HasNext())
        {
            Client* client = clients->Find(iter.Next());
            if(client && client->GetActor())
            {
                client->GetActor()->UnregisterCallback(this);
            }
        }
        watchers.DeleteAll();
    }
}

bool psMiniGameSession::Load(csString &responseString)
{
#ifdef DEBUG_MINIGAMES
    Debug2(LOG_ANY, 0, "Loading minigame: %s", responseString.GetData());
#endif

    options = 0;

    if(!responseString.StartsWith("<Examine>", false))
    {
        Error2("Invalid response string for minigame %s.", name.GetData());
        return false;
    }

    csRef<iDocument> doc = ParseString(responseString);
    if(!doc)
    {
        Error2("Parse error in minigame %s response string.", name.GetData());
        return false;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in minigame %s response string.", name.GetData());
        return false;
    }

    csRef<iDocumentNode> topNode = root->GetNode("Examine");
    if(!topNode)
    {
        Error2("No <Examine> tag in minigame %s response string.", name.GetData());
        return false;
    }

    csRef<iDocumentNode> boardNode = topNode->GetNode("GameBoard");
    if(!boardNode)
    {
        Error2("No <Board> tag in minigame %s response string.", name.GetData());
        return false;
    }

    const char* gameName = boardNode->GetAttributeValue("Name");
    psMiniGameBoardDef* gameBoardDef = manager->FindGameDef(gameName);
    if(!gameBoardDef)
    {
        Error2("Game board \"%s\" not defined.", gameName);
        return false;
    }

    // Get the optional layout string; will override the default layout
    const char* layoutStr = boardNode->GetAttributeValue("Layout");
    uint8_t* layout = NULL;
    if(layoutStr && strlen(layoutStr) != size_t(gameBoardDef->GetRows() * gameBoardDef->GetCols()))
    {
        Error2("Layout string in action_location minigame %s response string invalid. Using default.", gameName);
    }
    else if(layoutStr)
    {
        layout = new uint8_t[gameBoardDef->GetLayoutSize()];
        gameBoardDef->PackLayoutString(layoutStr, layout);
    }

    // get the gameboard def options
    options |= gameBoardDef->GetGameboardOptions();

    // get the session type. "Personal" means a minigame session per player; each player gets their
    // own personal session (i.e. no watchers, and restricted to single player games). Expected application
    // for personal minigames is in-game puzzles, including quest steps maybe.
    // "Public" is the more traditional, two player leisure games, such as Groffels Toe.
    csString sessionStr(boardNode->GetAttributeValue("Session"));
    sessionStr.Downcase();
    if(sessionStr == "personal")
    {
        options |= PERSONAL_GAME;
    }
    else if(sessionStr.Length() > 0 && sessionStr != "public")
    {
        Error2("Session setting for minigame %s invalid. Defaulting to \'Public\'", gameName);
    }

    // whether to observe the end game settings. If so, the server observes the moves made and
    // can identify the end game pattern(s). Default = No.
    csString endgameStr(boardNode->GetAttributeValue("EndGame"));
    endgameStr.Downcase();
    if(endgameStr == "yes")
    {
        options |= OBSERVE_ENDGAME;
    }
    else if(endgameStr.Length() > 0 && endgameStr != "no")
    {
        Error2("ObserveEndGame setting for %s invalid. Defaulting to \'No\'", gameName);
    }

    // read any script name for winner
    csString scriptStr(boardNode->GetAttributeValue("Script"));
    if(scriptStr.Length() > 0)
    {
        if(options & OBSERVE_ENDGAME)
        {
            winnerScript = scriptStr;
        }
        else
        {
            Error2("Ignoring invalid script for %s.", gameName);
        }
    }
    // read parameters for the script
    csString param0(boardNode->GetAttributeValue("Param0"));
    if(param0.Length() > 0)
        paramZero = param0;
    csString param1(boardNode->GetAttributeValue("Param1"));
    if(param1.Length() > 0)
        paramOne = param1;
    csString param2(boardNode->GetAttributeValue("Param2"));
    if(param2.Length() > 0)
        paramTwo = param2;

    // Setup the game board
    gameBoard.Setup(gameBoardDef, layout);
    delete [] layout;

    // if session is personal, check its a 1-player game
    if((options & PERSONAL_GAME) && gameBoard.GetNumPlayers() > 1)
    {
        Error2("Personal game %s has 2 or more players, which is untenable.", gameName);
        return false;
    }

    // initialise players
    MinigamePlayer* player, *previousPlayer = NULL;
    for(int p=0; p<gameBoard.GetNumPlayers(); p++)
    {
        player = new MinigamePlayer();
        if(!player)
        {
            Error2("Cannot initialise players for %s.", gameName);
            return false;
        }

        player->playerID = (uint32_t)-1;
        player->playerName = NULL;
        player->nextMover = NULL;

        // organise order of play, if appropriate
        if(gameBoard.GetPlayerTurnRule() >= ORDERED)
        {
            if(p == 0)
            {
                nextPlayerToMove = player;
            }
            else
            {
                previousPlayer->nextMover = player;
            }
            previousPlayer = player;
        }

        players.Push(player);
    }
    if(previousPlayer)
    {
        previousPlayer->nextMover = nextPlayerToMove;
    }

    // determine style of minigame: currently based on number of players only
    minigameStyle = (gameBoard.GetNumPlayers()==1) ? MG_PUZZLE : MG_GAME;

    return true;
}

void psMiniGameSession::Restart()
{

}

void psMiniGameSession::AddPlayer(Client* client)
{
    if(!client || client->GetClientNum() == (uint32_t)-1)
        return;

    uint32_t clientID = client->GetClientNum();

    // look for free player position
    bool playerIsAPlayer = false;
    int colour=WHITE_PLAYER; // TODO temp to maintain White/Black: destined to done proper
    csPDelArray<MinigamePlayer>::Iterator pIter = players.GetIterator();
    while(pIter.HasNext())
    {
        MinigamePlayer* p = pIter.Next();
        if(p && p->playerID == (uint32_t)-1)
        {
            p->playerID = clientID;
            p->playerName = client->GetName();
            p->idleCounter = MINIGAME_IDLE_TIME;
            p->blackOrWhite = colour;

            // If this is a managed game, reset the game board
            if(options & MANAGED_GAME)
            {
                Restart();
            }

            // Send notifications
            psSystemMessage* newmsg;
            if(minigameStyle == MG_GAME)
            {
                newmsg = new psSystemMessage(clientID, MSG_INFO, "%s started playing %s with %s pieces.",
                                             p->playerName, name.GetData(), (colour==WHITE_PLAYER)?"white":"black");
            }
            else // MG_PUZZLE
            {
                newmsg = new psSystemMessage(clientID, MSG_INFO, "%s started solving %s.",
                                             p->playerName, name.GetData());
            }
            newmsg->Multicast(client->GetActor()->GetMulticastClients(), 0, CHAT_SAY_RANGE);

            playerCount++;
            playerIsAPlayer = true;
            delete newmsg;
            break;
        }
        colour++;
    }

    // Otherwise add to the list of watchers
    if(!playerIsAPlayer)
    {
        watchers.Push(clientID);
    }

    // Register our call-back function to detect disconnected clients
    client->GetActor()->RegisterCallback(this);

    // Broadcast the game board
    Broadcast();

#ifdef DEBUG_MINIGAMES
    Debug3(LOG_ANY, 0, "Added player %u to the game session \"%s\"\n", clientID, name.GetData());
#endif
}

void psMiniGameSession::RemovePlayer(Client* client)
{
    if(!client || client->GetClientNum() == (uint32_t)-1)
    {
        return;
    }

    client->GetActor()->UnregisterCallback(this);
    uint32_t clientID = client->GetClientNum();

    bool playerRemoved = false;
    csPDelArray<MinigamePlayer>::Iterator pIter = players.GetIterator();
    while(pIter.HasNext())
    {
        MinigamePlayer* p = pIter.Next();
        if(p && p->playerID == clientID)
        {
            p->playerID = (uint32_t)-1;

            // Send notifications
            psSystemMessage* newmsg;
            if(minigameStyle == MG_GAME)
            {
                newmsg = new psSystemMessage(clientID, MSG_INFO, "%s stopped playing %s.",
                                             p->playerName, name.GetData());
            }
            else // MG_PUZZLE
            {
                newmsg = new psSystemMessage(clientID, MSG_INFO, "%s stopped solving %s.",
                                             p->playerName, name.GetData());
            }
            newmsg->Multicast(client->GetActor()->GetMulticastClients(), 0, CHAT_SAY_RANGE);

            p->playerName = NULL;
            playerCount--;
            playerRemoved = true;
            delete newmsg;

            break;
        }
    }

    // Otherwise it is one of the watchers
    if(!playerRemoved)
    {
        watchers.Delete(clientID);
    }

    // Send the STOP game message
    psMGStartStopMessage msg(clientID, false);
    msg.SendMessage();

#ifdef DEBUG_MINIGAMES
    Debug3(LOG_ANY, 0, "Removed player %u from the game session \"%s\"\n", clientID, name.GetData());
#endif
}

void psMiniGameSession::Send(uint32_t clientID, uint32_t modOptions)
{
    modOptions |= options;
    if(gameBoard.GetMovePieceTypeRule() == PLACE_ONLY)
        modOptions |= OPTION_PLACE_ONLY;
    psMGBoardMessage msg(clientID, currentCounter, id, modOptions,
                         gameBoard.GetCols(), gameBoard.GetRows(), gameBoard.GetLayout(),
                         gameBoard.GetNumPieces(), gameBoard.GetPiecesSize(), gameBoard.GetPieces());
    msg.SendMessage();
}

void psMiniGameSession::Broadcast()
{
    csPDelArray<MinigamePlayer>::Iterator pIter = players.GetIterator();
    while(pIter.HasNext())
    {
        MinigamePlayer* p = pIter.Next();
        if(p && p->playerID != (uint32_t)-1)
        {
            Send(p->playerID, (p->blackOrWhite==WHITE_PLAYER)? 0 : BLACK_PIECES);
        }
    }

    csArray<uint32_t>::Iterator iter = watchers.GetIterator();
    while(iter.HasNext())
    {
        uint32_t id = iter.Next();
        if(id != (uint32_t)-1)
        {
            Send(id, READ_ONLY);
        }
    }
}

bool psMiniGameSession::IsValidToUpdate(Client* client) const
{

    if(!client || client->GetClientNum() == (uint32_t)-1)
    {
        return false;
    }

    uint32_t clientID = client->GetClientNum();

    if(!actionObject.IsValid())
    {
        Error2("gemActionLocation for the minigame %s was deleted.", name.GetData());
        psserver->SendSystemError(clientID, "Oops! Game was deleted by the server!");
        return false;
    }

    // TODO: Implement more complex verification for managed games

    csPDelArray<MinigamePlayer>::ConstIterator pIter = players.GetIterator();
    while(pIter.HasNext())
    {
        MinigamePlayer* p = pIter.Next();
        if(p && p->playerID == clientID)
        {
            return true;
        }
    }

    psserver->SendSystemError(clientID, "You are not playing %s!", name.GetData());
    return false;
}

void psMiniGameSession::Update(Client* client, psMGUpdateMessage &msg)
{
    uint32_t clientnum = client->GetClientNum();
    MinigamePlayer* movingPlayer = NULL;
    csPDelArray<MinigamePlayer>::Iterator pIter = players.GetIterator();
    while(pIter.HasNext())
    {
        MinigamePlayer* p = pIter.Next();
        if(p && p->playerID == clientnum)
        {
            movingPlayer = p;
            break;
        }
    }
    if(!movingPlayer)
    {
        Error1("Minigame moving player NULL!");
        return;
    }

    // Verify the message version
    if(!msg.IsNewerThan(currentCounter))
    {
        Error3("Ignoring minigame update message with version %d because our version is %d.",
               msg.msgCounter, currentCounter);
        return;
    }
    currentCounter = msg.msgCounter;

    // Verify range
    if(client->GetActor()->RangeTo(actionObject, true) > RANGE_TO_LOOT)
    {
        psserver->SendSystemError(clientnum, "You are not in range for %s!",
                                  name.GetData());
        // Reset the client's game board
        ResendBoardLayout(movingPlayer);
        return;
    }

    // valid move?
    if(endgameReached)
    {
        if(minigameStyle == MG_GAME)
        {
            psserver->SendSystemError(clientnum, "Game over");
        }
        else  // Puzzle
        {
            psserver->SendSystemError(clientnum, "Puzzle solved");
        }        // Reset the client's game board
        ResendBoardLayout(movingPlayer);
        return;
    }

    if(nextPlayerToMove && nextPlayerToMove != movingPlayer)
    {
        psserver->SendSystemError(clientnum, "It is not your turn to move.");
        // Reset the client's game board
        ResendBoardLayout(movingPlayer);
        return;
    }

    // Check moves and apply updates if rules passed
    bool moveAccepted = false;
    if(msg.msgNumUpdates == 1)
    {
        // 1 update means a new piece is added to the board
        moveAccepted = GameMovePassesRules(movingPlayer,
                                           (msg.msgUpdates[0] & 0xF0) >> 4,
                                           msg.msgUpdates[0] & 0x0F,
                                           msg.msgUpdates[1]);
    }
    else if(msg.msgNumUpdates == 2)
    {
        // 2 updates means a piece has been moved from one tile to another
        moveAccepted = GameMovePassesRules(movingPlayer,
                                           (msg.msgUpdates[0] & 0xF0) >> 4,
                                           msg.msgUpdates[0] & 0x0F,
                                           msg.msgUpdates[1],
                                           (msg.msgUpdates[2] & 0xF0) >> 4,
                                           msg.msgUpdates[2] & 0x0F,
                                           msg.msgUpdates[3]);
    }

    if(!moveAccepted)
    {
        psserver->SendSystemError(clientnum, "Illegal move.");
        // Reset the client's game board
        ResendBoardLayout(movingPlayer);
        return;
    }

    // Apply updates
    for(int i = 0; i < msg.msgNumUpdates; i++)
    {
        gameBoard.Set((msg.msgUpdates[2*i] & 0xF0) >> 4,
                      msg.msgUpdates[2*i] & 0x0F,
                      msg.msgUpdates[2*i+1]);
    }

    movingPlayer->idleCounter = MINIGAME_IDLE_TIME;

    // Broadcast the new game board layout
    Broadcast();

    // see if endgame has been identified?
    Endgame_TileType winningPiece;
    endgameReached = (options & OBSERVE_ENDGAME) ? gameBoard.DetermineEndgame(winningPiece) : false;

    // if endgame reached, determine winner
    if(endgameReached)
    {
        // determine winner
        MinigamePlayer* winningPlayer = NULL;
        csString wonText;
        ProgressionScript* progScript = NULL;

        if(winningPiece == WHITE_PIECE || winningPiece == BLACK_PIECE)
        {
            // cycle on all players
            pIter.Reset();
            while(pIter.HasNext())
            {
                MinigamePlayer* p = pIter.Next();
                // if it's a one player puzzle
                if(minigameStyle == MG_PUZZLE) {
                    winningPlayer = p;
                    break;
                }
                // if it's a multiplayer game
                else if(p && ((p->blackOrWhite == WHITE_PLAYER && winningPiece == WHITE_PIECE) ||
                         (p->blackOrWhite == BLACK_PLAYER && winningPiece == BLACK_PIECE)))
                {
                    winningPlayer = p;
                    break;
                }
            }
            if(!winningPlayer)
            {
                Error1("Minigame winning player NULL!");
                return;
            }

            // is there a progression script to run
            if(winnerScript.Length() > 0)
            {
                if(!(progScript = psserver->GetProgressionManager()->FindScript(winnerScript.GetData())))
                {
                    Error2("Failed to find progression script >%s<.", winnerScript.GetData());
                }
            }

            // announce winner, unless there is a script, in this case the script will notify the winner
            if(!progScript)
            {
                if(minigameStyle == MG_GAME)
                {
                    psserver->SendSystemInfo(winningPlayer->playerID, (GetName() + ": You have won!"));
                }
                else
                {
                    psserver->SendSystemInfo(winningPlayer->playerID, (GetName() + ": You have solved it!"));
                }
                wonText = GetName() + ": ";
                if(movingPlayer->playerName)
                {
                    wonText += csString(winningPlayer->playerName);
                    wonText += (winningPlayer->blackOrWhite == WHITE_PLAYER) ? " (white)" : " (black)";
                }
                else
                {
                    wonText += "Someone";
                }
                if(minigameStyle == MG_GAME)
                {
                    wonText += " has won.";
                }
                else
                {
                    wonText += " has solved it.";
                }
            }

            // now run progression script
            if(progScript)
            {
                ClientConnectionSet* clients = psserver->GetConnections();
                if(clients)
                {
                    Client* winnerClient = clients->Find(winningPlayer->playerID);
                    if(winnerClient)
                    {
                        // passes a parameter to the script to run
                        csString parameters = "Param0='"+paramZero+"';"+"Param1='"+paramOne+"';"+"Param2='"+paramTwo+"'";
                        MathScript* bindings = MathScript::Create("RunScript bindings", parameters);

                        MathEnvironment env;
                        env.Define("Winner", winnerClient->GetActor());
                        env.Define("Target", winnerClient->GetActor()); // needed if called by an action location
                        (void) bindings->Evaluate(&env);
                        if(gameBoard.GetNumPlayers() == 1)
                        {
                            progScript->Run(&env);
                        }
                        else
                        {
                            Client* loserClient;
                            pIter.Reset();
                            while(pIter.HasNext())
                            {
                                MinigamePlayer* p = pIter.Next();
                                if(p && p->playerID != winningPlayer->playerID)
                                {
                                    loserClient = clients->Find(p->playerID);
                                    if(loserClient)
                                    {
                                        env.Define("Loser", loserClient->GetActor());
                                    }
                                    progScript->Run(&env);
                                }
                            }
                        }
                        MathScript::Destroy(bindings);
                    }
                }
            }
        }
        else
        {
            if(minigameStyle == MG_GAME)
            {
                wonText = "Game over.";
            }
            else  // Puzzle
            {
                wonText = "Puzzle solved.";
            }
            psserver->SendSystemInfo(movingPlayer->playerID, wonText);
            winningPlayer = movingPlayer;
        }

        pIter.Reset();
        while(pIter.HasNext())
        {
            MinigamePlayer* p = pIter.Next();
            if(p && p->playerID != winningPlayer->playerID)
            {
                psserver->SendSystemInfo(p->playerID, wonText);
            }
        }
        csArray<uint32_t>::Iterator iter = watchers.GetIterator();
        while(iter.HasNext())
        {
            uint32_t id = iter.Next();
            if(id != (uint32_t)-1)
            {
                psserver->SendSystemInfo(id, wonText);
            }
        }

        // reset the gameboard session
        SetSessionReset();
    }
    else
    {
        // broadcast the move info too
        csString movedText = GetName() + ": ";
        if(movingPlayer->playerName)
        {
            movedText += csString(movingPlayer->playerName);
            if(movingPlayer->blackOrWhite == WHITE_PLAYER)
                movedText +=  " (white)";
            else
                movedText +=  " (black)";
        }
        else
        {
            movedText += "Someone";
        }
        movedText += " has moved.";

        pIter.Reset();
        while(pIter.HasNext())
        {
            MinigamePlayer* p = pIter.Next();
            if(p && p->playerID != movingPlayer->playerID)
            {
                psserver->SendSystemInfo(p->playerID, movedText);
            }
        }
        csArray<uint32_t>::Iterator iter = watchers.GetIterator();
        while(iter.HasNext())
        {
            uint32_t id = iter.Next();
            if(id != (uint32_t)-1)
            {
                psserver->SendSystemInfo(id, movedText);
            }
        }
    }

    // whos move next?
    if(nextPlayerToMove != NULL)
    {
        nextPlayerToMove = movingPlayer->nextMover;
    }
}

void psMiniGameSession::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    gemActor* actor = (gemActor*)object;
    if(!actor)
    {
        return;
    }

    Client* client = actor->GetClient();

    if(client)
    {
        RemovePlayer(client);
    }
}

void psMiniGameSession::Idle()
{
    // Idle counters
    csPDelArray<MinigamePlayer>::Iterator pIter = players.GetIterator();
    while(pIter.HasNext())
    {
        MinigamePlayer* p = pIter.Next();
        if(p && p->playerID != (uint32_t)-1 && p->idleCounter > 0)
        {
#ifdef DEBUG_MINIGAMES
            Debug2(LOG_ANY, 0, "Player idle counter %d", p->idleCounter);
#endif
            if((--p->idleCounter) == 0)
            {
                Client* client = psserver->GetNetManager()->GetClient(p->playerID);
                if(client)
                {
                    RemovePlayer(client);
                }
            }
        }
    }

    // Range check for watchers
    size_t i = 0;
    while(i < watchers.GetSize())
    {
        Client* client = psserver->GetNetManager()->GetClient(watchers.Get(i));
        if(!client)
        {
            ++i;
            continue;
        }

        if(client->GetActor()->RangeTo(actionObject, true) > RANGE_TO_SELECT)
        {
            RemovePlayer(client);
            continue;
        }

        ++i;
    }
}

bool psMiniGameSession::GameSessionActive(void)
{
    csPDelArray<MinigamePlayer>::Iterator pIter = players.GetIterator();
    while(pIter.HasNext())
    {
        MinigamePlayer* p = pIter.Next();
        if(p && p->playerID != (uint32_t)-1)
        {
            return true;
        }
    }

    return (!watchers.IsEmpty());
}

bool psMiniGameSession::IsSessionPublic(void)
{
    return !(options & PERSONAL_GAME);
}

void psMiniGameSession::ResendBoardLayout(MinigamePlayer* player)
{
    if(player->blackOrWhite == WHITE_PLAYER)
    {
        Send(player->playerID, DISALLOWED_MOVE);
    }
    else if(player->blackOrWhite == BLACK_PLAYER)
    {
        Send(player->playerID, BLACK_PIECES | DISALLOWED_MOVE);
    }

    // after a disallowed move, the other clients will be out of sync
    // with server & disallowed client, so resync.
    currentCounter--;
}

// checks move passes rules. If col/row/state-2 are -1 then a new piece is played,
// otherwise an existing piece is moved
bool psMiniGameSession::GameMovePassesRules(MinigamePlayer* mover,
        int8_t col1, int8_t row1, int8_t state1,
        int8_t col2, int8_t row2, int8_t state2)
{
    bool newPiecePlayed = (state2 == -1);

    if(gameBoard.GetPlayerTurnRule() == STRICT_ORDERED && gameBoard.GetNumPlayers() != playerCount)
    {
        return false;
    }

    if((newPiecePlayed && gameBoard.GetMovePieceTypeRule() == MOVE_ONLY) ||
            (!newPiecePlayed && gameBoard.GetMovePieceTypeRule() == PLACE_ONLY))
    {
        return false;
    }

    if(gameBoard.GetMoveablePiecesRule() == OWN_PIECES_ONLY)
    {
        TileStates movingPiece = (TileStates) gameBoard.Get(col1, row1);
        if(mover->blackOrWhite == WHITE_PLAYER && (movingPiece < WHITE_1 || movingPiece > WHITE_7))
        {
            return false;
        }
        else if(mover->blackOrWhite == BLACK_PLAYER && (movingPiece < BLACK_1 || movingPiece > BLACK_7))
        {
            return false;
        }
    }

    if(!newPiecePlayed &&
            gameBoard.GetMovePiecesToRule() == VACANCY_ONLY && gameBoard.Get(col2, row2) != EMPTY_TILE)
    {
        return false;
    }

    // Enforce MoveDirection rule
    Rule_MoveDirection moveDirectionRule = gameBoard.GetMoveDirectionRule();
    if(!newPiecePlayed && moveDirectionRule)
    {
        switch(moveDirectionRule)
        {
                // enforce moving on same column
            case VERTICAL :
                if(col1!=col2) return false;
                break;
                // enforce moving on same row
            case HORIZONTAL :
                if(row1!=row2) return false;
                break;
                // enforce moving on same row
            case CROSS :
                if(row1!=row2 && col1!=col2) return false;
                break;
                // enforce moving diagonal
            case DIAGONAL :
                if(row1==row2 || col1==col2) return false;
                break;
            default:
                break;
        }
    }

    // Enforce MoveDistance rule
    int allowedDistance = gameBoard.GetMoveDistanceRule();
    if(!newPiecePlayed && allowedDistance>0)
    {
        int movedDistance;
        // vertical move
        if(col1==col2) movedDistance = abs(row1-row2);
        // horizontal move
        else if(row1==row2) movedDistance = abs(col1-col2);
        // diagonal move
        else if(abs(row1-row2)-abs(col1-col2)==0)
            movedDistance = abs(row1-row2);
        // free style move
        else
            movedDistance = abs(row1-row2)+abs(col1-col2);

        if(movedDistance>allowedDistance)
            return false;
    }

    return true;
}

