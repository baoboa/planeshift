/*
* psminigameboard.cpp
*
* Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "util/minigame.h"
#include "util/psxmlparser.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psminigameboard.h"

using namespace psMiniGame;


//#define DEBUG_MINIGAMES

psMiniGameBoardDef::psMiniGameBoardDef(const uint8_t noCols, const uint8_t noRows,
                                       const char* layoutStr, const char* piecesStr,
                                       const uint8_t defPlayers, const int16_t options)
{
    // rows & columns setup
    rows = noRows;
    cols = noCols;

    // layout setup
    layoutSize = cols * rows / 2;
    if((cols * rows % 2) != 0)
        layoutSize++;

    // Convert the layout string into a packed binary array
    layout = new uint8_t[layoutSize];
    PackLayoutString(layoutStr, layout);

    // Get the list of available pieces
    pieces = NULL;
    numPieces = 0;

    // Pack the list of pieces
    numPieces = (uint8_t)strlen(piecesStr);
    piecesSize = (numPieces + 1) / 2;

    pieces = new uint8_t[piecesSize];

    for(size_t i = 0; i < numPieces; i++)
    {
        uint8_t v = 0;
        if(isxdigit(piecesStr[i]))
        {
            char ch = toupper(piecesStr[i]);
            v = (uint8_t)(ch - '0');
            if(ch >= 'A')
                v -= (uint8_t)('A' - '0') - 10;
        }
        if(i % 2)
            pieces[i/2] |= v;
        else
            pieces[i/2] = (v << 4);
    }

    numPlayers = defPlayers;
    gameboardOptions = options;

    // default game rules
    playerTurnRule = RELAXED;
    movePieceTypeRule = PLACE_OR_MOVE;
    moveablePiecesRule = ANY_PIECE;
    movePiecesToRule = ANYWHERE;
    moveDistanceRule = -1;

    endgames.Empty();
}

psMiniGameBoardDef::~psMiniGameBoardDef()
{
    if(layout)
        delete[] layout;
    if(pieces)
        delete[] pieces;
    while(!endgames.IsEmpty())
    {
        Endgame_Spec* spec = endgames.Pop();
        while(!spec->endgameTiles.IsEmpty())
            delete spec->endgameTiles.Pop();
        delete spec;
    }
}

void psMiniGameBoardDef::PackLayoutString(const char* layoutStr, uint8_t* packedLayout)
{
    for(size_t i = 0; i < strlen(layoutStr); i++)
    {
        uint8_t v = 0x0F;
        if(isxdigit(layoutStr[i]))
        {
            char ch = toupper(layoutStr[i]);
            v = (uint8_t)(ch - '0');
            if(ch >= 'A')
                v -= (uint8_t)('A' - '0') - 10;
        }
        if(i % 2)
            packedLayout[i/2] |= v;
        else
            packedLayout[i/2] = (v << 4);
    }
}

uint8_t psMiniGameBoardDef::PackPiece(char pieceStr)
{
    uint8_t v = 0x0F;
    if(isxdigit(pieceStr))
    {
        char ch = toupper(pieceStr);
        v = (uint8_t)(ch - '0');
        if(ch >= 'A')
            v -= (uint8_t)('A' - '0') - 10;
    }
    return v;
}

bool psMiniGameBoardDef::DetermineGameRules(csString rulesXMLstr, csString name)
{
    if(rulesXMLstr.StartsWith("<GameRules>", false))
    {
        csRef<iDocument> doc = ParseString(rulesXMLstr);
        if(doc)
        {
            csRef<iDocumentNode> root = doc->GetRoot();
            if(root)
            {
                csRef<iDocumentNode> topNode = root->GetNode("GameRules");
                if(topNode)
                {
                    csRef<iDocumentNode> rulesNode = topNode->GetNode("Rules");
                    if(rulesNode)
                    {
                        // PlayerTurns can be 'Ordered' (order of players' moves enforced)
                        // or 'StrictOrdered' (as Ordered, and all players must be present)
                        // or 'Relaxed' (default - free for all).
                        csString playerTurnsVal(rulesNode->GetAttributeValue("PlayerTurns"));
                        playerTurnsVal.Downcase();
                        if(playerTurnsVal == "ordered")
                        {
                            playerTurnRule = ORDERED;
                        }
                        else if(playerTurnsVal == "strictordered")
                        {
                            playerTurnRule = STRICT_ORDERED;
                        }
                        else if(!playerTurnsVal.IsEmpty() && playerTurnsVal != "relaxed")
                        {
                            Error3("\"%s\" Rule PlayerTurns \"%s\" not recognised. Defaulting to \'Relaxed\'.",
                                   name.GetDataSafe(), playerTurnsVal.GetDataSafe());
                        }

                        // MoveType can be 'MoveOnly' (player can only move existing pieces),
                        // 'PlaceOnly' (player can only place new pieces on the board; cant move others),
                        // or 'PlaceOrMovePiece' (default - either move existing or place new pieces).
                        csString moveTypeVal(rulesNode->GetAttributeValue("MoveType"));
                        moveTypeVal.Downcase();
                        if(moveTypeVal == "moveonly")
                        {
                            movePieceTypeRule = MOVE_ONLY;
                        }
                        else if(moveTypeVal == "placeonly")
                        {
                            movePieceTypeRule = PLACE_ONLY;
                        }
                        else if(!moveTypeVal.IsEmpty() && moveTypeVal != "placeormovepiece")
                        {
                            Error3("\"%s\" Rule MoveType \"%s\" not recognised. Defaulting to \'PlaceOrMovePiece\'.",
                                   name.GetDataSafe(), moveTypeVal.GetDataSafe());
                        }

                        // MoveablePieces can be 'Own' (player can only move their own pieces) or
                        // 'Any' (default - player can move any piece in play).
                        csString moveablePiecesVal(rulesNode->GetAttributeValue("MoveablePieces"));
                        moveablePiecesVal.Downcase();
                        if(moveablePiecesVal == "own")
                        {
                            moveablePiecesRule = OWN_PIECES_ONLY;
                        }
                        else if(!moveablePiecesVal.IsEmpty() && moveablePiecesVal != "any")
                        {
                            Error3("\"%s\" Rule MoveablePieces \"%s\" not recognised. Defaulting to \'Any\'.",
                                   name.GetDataSafe(), moveablePiecesVal.GetDataSafe());
                        }

                        // MoveTo can be 'Vacancy' (player can move pieces to vacant squares only) or
                        // 'Anywhere' (default - can move to any square, vacant or occupied).
                        csString moveToVal(rulesNode->GetAttributeValue("MoveTo"));
                        moveToVal.Downcase();
                        if(moveToVal == "vacancy")
                        {
                            movePiecesToRule = VACANCY_ONLY;
                        }
                        else if(!moveToVal.IsEmpty() && moveToVal != "anywhere")
                        {
                            Error3("\"%s\" Rule MoveTo \"%s\" not recognised. Defaulting to \'Anywhere\'.",
                                   name.GetDataSafe(), moveToVal.GetDataSafe());
                        }

                        // MoveDirection can be 'Vertical', 'Horizontal', 'Cross', 'Diagonal' or 'Any' (default)
                        csString moveDirection(rulesNode->GetAttributeValue("MoveDirection"));
                        moveDirection.Downcase();
                        if(moveDirection == "vertical")
                        {
                            moveDirectionRule = VERTICAL;
                        }
                        else if(moveDirection == "horizontal")
                        {
                            moveDirectionRule = HORIZONTAL;
                        }
                        else if(moveDirection == "cross")
                        {
                            moveDirectionRule = CROSS;
                        }
                        else if(moveDirection == "diagonal")
                        {
                            moveDirectionRule = DIAGONAL;
                        }
                        else if(!moveDirection.IsEmpty() && moveDirection != "any")
                        {
                            Error3("\"%s\" Rule MoveDirection \"%s\" not recognised. Defaulting to \'Any\'.",
                                   name.GetDataSafe(), moveDirection.GetDataSafe());
                        }

                        // MoveDistance can be an integer number
                        moveDistanceRule = rulesNode->GetAttributeValueAsInt("MoveDistance");

                        return true;
                    }
                }
            }
        }
    }
    else if(rulesXMLstr.IsEmpty())    // if no rules defined at all, dont worry - keep defaults
    {
        return true;
    }

    Error2("XML error in GameRules definition for \"%s\" .", name.GetDataSafe());
    return false;
}

// Decipher end game XML
// <MGEndGame>
//  <EndGame Coords="relative"/"absolute" [SourceTile="T"] [Winner="T"]>
//   <Coord Col="?" Row="?" Tile="T" />
//  </EndGame>
// </MGEndGame>
bool psMiniGameBoardDef::DetermineEndgameSpecs(csString endgameXMLstr, csString name)
{
    if(endgameXMLstr.StartsWith("<MGEndGame>", false))
    {
        csRef<iDocument> doc = ParseString(endgameXMLstr);
        if(doc)
        {
            csRef<iDocumentNode> root = doc->GetRoot();
            if(root)
            {
                // <MGEndGame>
                csRef<iDocumentNode> topNode = root->GetNode("MGEndGame");
                if(topNode)
                {
                    //  <EndGame Coords="relative"/"absolute" [SourceTile="T"] [Winner="T"]>
                    csRef<iDocumentNodeIterator> egNodes = topNode->GetNodes("EndGame");
                    if(egNodes)
                    {
                        while(egNodes->HasNext())
                        {
                            csRef<iDocumentNode> egNode = egNodes->Next();
                            if(egNode)
                            {
                                // Coords="relative"/"absolute"
                                Endgame_Spec* endgame = new Endgame_Spec;
                                csString posAbsVal(egNode->GetAttributeValue("Coords"));
                                posAbsVal.Downcase();
                                if(posAbsVal == "relative")
                                {
                                    endgame->positionsAbsolute = false;
                                }
                                else if(posAbsVal == "absolute")
                                {
                                    endgame->positionsAbsolute = true;
                                }
                                else
                                {
                                    delete endgame;
                                    Error2("Error in EndGame XML for \"%s\" minigame: Absolute/Relative setting misunderstood.",
                                           name.GetDataSafe());
                                    return false;
                                }

                                //  [SourceTile="T"] (optional)
                                csString srcTileVal(egNode->GetAttributeValue("SourceTile"));
                                Endgame_TileType srcTile;
                                if(EvaluateTileTypeStr(srcTileVal, srcTile) && srcTile != FOLLOW_SOURCE_TILE)
                                {
                                    endgame->sourceTile = srcTile;
                                }
                                else if(endgame->positionsAbsolute == false)
                                {
                                    delete endgame;
                                    Error2("Error in EndGame XML for \"%s\" minigame: SourceTile setting misunderstood.",
                                           name.GetDataSafe());
                                    return false;
                                }

                                //  [Winner="T"] (optional)
                                csString winningPieceVal(egNode->GetAttributeValue("Winner"));
                                Endgame_TileType winningTile;
                                endgame->winner = NO_PIECE;  // default - no winner specified
                                if(EvaluateTileTypeStr(winningPieceVal, winningTile) &&
                                        (winningTile == WHITE_PIECE || winningTile == BLACK_PIECE))
                                {
                                    endgame->winner = winningTile;    // winner is white or black player
                                }

                                //   <Coord Col="?" Row="?" Tile="T" />
                                csRef<iDocumentNodeIterator> coordNodes = egNode->GetNodes("Coord");
                                if(coordNodes)
                                {
                                    while(coordNodes->HasNext())
                                    {
                                        csRef<iDocumentNode> coordNode = coordNodes->Next();
                                        if(coordNode)
                                        {
                                            //   Col="?" Row="?"
                                            Endgame_TileSpec* egTileSpec = new Endgame_TileSpec;
                                            int egCol = coordNode->GetAttributeValueAsInt("Col");
                                            int egRow = coordNode->GetAttributeValueAsInt("Row");
                                            if(abs(egCol) >= GAMEBOARD_MAX_COLS || abs(egRow) >= GAMEBOARD_MAX_ROWS)
                                            {
                                                delete endgame;
                                                delete egTileSpec;
                                                Error2("Error in EndGame XML for \"%s\" minigame: Col/Row spec out of range.",
                                                       name.GetDataSafe());
                                                return false;
                                            }

                                            //   Tile="T"
                                            csString tileVal(coordNode->GetAttributeValue("Tile"));
                                            Endgame_TileType tile;
                                            if(EvaluateTileTypeStr(tileVal, tile))
                                            {
                                                egTileSpec->col = egCol;
                                                egTileSpec->row = egRow;
                                                egTileSpec->tile = tile;
                                                if(tile==SPECIFIC_TILE)
                                                {
                                                    const char* specificPiece = coordNode->GetAttributeValue("Piece");
                                                    egTileSpec->specificTile = psMiniGameBoardDef::PackPiece(specificPiece[0]);
                                                }
                                            }
                                            else
                                            {
                                                while(!endgame->endgameTiles.IsEmpty())
                                                    delete endgame->endgameTiles.Pop();
                                                delete endgame;
                                                delete egTileSpec;
                                                Error2("Error in EndGame XML for \"%s\" minigame: Tile setting misunderstood.",
                                                       name.GetDataSafe());
                                                return false;
                                            }

                                            endgame->endgameTiles.Push(egTileSpec);
                                        }
                                    }
                                }
                                endgames.Push(endgame);
                            }
                        }
                    }

                    return true;
                }
            }
        }
    }
    else if(endgameXMLstr.IsEmpty())    // if no endgames defined at all, then no worries
    {
        return true;
    }

    Error2("XML error in Endgames definition for \"%s\" .", name.GetDataSafe());
    return false;
}

bool psMiniGameBoardDef::EvaluateTileTypeStr(csString TileTypeStr, Endgame_TileType &tileType)
{
    if(TileTypeStr.Length() == 1)
    {
        switch(TileTypeStr.GetAt(0))
        {
            case 'A':
            case 'a':
                tileType = PLAYED_PIECE;        // tile has any played piece on
                break;
            case 'W':
            case 'w':
                tileType = WHITE_PIECE;         // tile has white piece
                break;
            case 'B':
            case 'b':
                tileType = BLACK_PIECE;         // tile has black piece
                break;
            case 'E':
            case 'e':
                tileType = NO_PIECE;            // empty tile
                break;
            case 'F':
            case 'f':
                tileType = FOLLOW_SOURCE_TILE;  // tile has piece as per first tile in pattern
                break;
            case 'S':
            case 's':
                tileType = SPECIFIC_TILE;  // has to be a specific tile
                break;
            default:
                return false;
                break;
        }

        return true;
    }

    return false;
}

//---------------------------------------------------------------------------

psMiniGameBoard::psMiniGameBoard()
    : layout(0)
{
}

psMiniGameBoard::~psMiniGameBoard()
{
    if(layout)
        delete[] layout;
}

void psMiniGameBoard::Setup(psMiniGameBoardDef* newGameDef, uint8_t* preparedLayout)
{
    // Delete the previous layout
    if(layout)
        delete[] layout;

    gameBoardDef = newGameDef;

    layout = new uint8_t[gameBoardDef->layoutSize];

    if(!preparedLayout)
        memcpy(layout, gameBoardDef->layout, gameBoardDef->layoutSize);
    else
        memcpy(layout, preparedLayout, gameBoardDef->layoutSize);
}

uint8_t psMiniGameBoard::Get(uint8_t col, uint8_t row) const
{
    if(col >= gameBoardDef->cols || row >= gameBoardDef->rows || !layout)
        return DISABLED_TILE;

    int idx = row*gameBoardDef->cols + col;
    uint8_t v = layout[idx/2];
    if(idx % 2)
        return v & 0x0F;
    else
        return (v & 0xF0) >> 4;
}

void psMiniGameBoard::Set(uint8_t col, uint8_t row, uint8_t state)
{
    if(col >= gameBoardDef->cols || row >= gameBoardDef->rows || !layout)
        return;

    int idx = row*gameBoardDef->cols + col;
    uint8_t v = layout[idx/2];
    if(idx % 2)
    {
        if((v & 0x0F) != DISABLED_TILE)
            layout[idx/2] = (v & 0xF0) + (state & 0x0F);
    }
    else
    {
        if((v & 0xF0) >> 4 != DISABLED_TILE)
            layout[idx/2] = (v & 0x0F) + ((state & 0x0F) << 4);
    }
}

bool psMiniGameBoard::DetermineEndgame(Endgame_TileType &winningPiece)
{
    if(gameBoardDef->endgames.IsEmpty())
        return false;

    // look through each endgame spec individually...
    csArray<Endgame_Spec*>::Iterator egIterator(gameBoardDef->endgames.GetIterator());
    while(egIterator.HasNext())
    {
        size_t patternsMatched;
        uint8_t tileAtPos = EMPTY_TILE;
        Endgame_Spec* endgame = egIterator.Next();
        if(endgame->positionsAbsolute)
        {
            // endgame absolute pattern: check each position for the required pattern
            // ... and then through each tile for the endgame pattern
            patternsMatched = 0;
            csArray<Endgame_TileSpec*>::Iterator egTileIterator(endgame->endgameTiles.GetIterator());
            while(egTileIterator.HasNext())
            {
                Endgame_TileSpec* endgameTile = egTileIterator.Next();
                if(endgameTile->col > gameBoardDef->cols || endgameTile->row > gameBoardDef->rows ||
                        endgameTile->col < 0 || endgameTile->row < 0)
                    break;

                tileAtPos = Get(endgameTile->col, endgameTile->row);

                if(tileAtPos == DISABLED_TILE || endgame->sourceTile == FOLLOW_SOURCE_TILE)
                    break;
                if(endgameTile->tile == PLAYED_PIECE && tileAtPos == EMPTY_TILE)
                    break;
                if(endgameTile->tile == NO_PIECE && tileAtPos != EMPTY_TILE)
                    break;
                if(endgameTile->tile == WHITE_PIECE && (tileAtPos < WHITE_1 || tileAtPos > WHITE_7))
                    break;
                if(endgameTile->tile == BLACK_PIECE && (tileAtPos < BLACK_1 || tileAtPos > BLACK_7))
                    break;
                if(endgameTile->tile == FOLLOW_SOURCE_TILE)
                    break;
                if(endgameTile->tile == SPECIFIC_TILE && endgameTile->specificTile != tileAtPos)
                    break;

                // if here, then the pattern has another match
                patternsMatched++;
            }

            // if all patterns matched, a winner
            if(endgame->endgameTiles.GetSize() == patternsMatched)
            {
                // determine winning piece - using last tile of the pattern if no explicit winner is defined.
                winningPiece = (endgame->winner != NO_PIECE) ? endgame->winner : EndgameWinner(tileAtPos);
                return true;
            }
        }
        else
        {
            // endgame relative pattern: check each piece in play for pattern relative to the piece
            uint8_t colCount, rowCount;
            for(rowCount=0; rowCount<gameBoardDef->rows; rowCount++)
            {
                for(colCount=0; colCount<gameBoardDef->cols; colCount++)
                {
                    // look for next initial played piece
                    uint8_t initialTile = Get(colCount, rowCount);
                    if((initialTile >= WHITE_1 && initialTile <= WHITE_7 &&
                            (endgame->sourceTile == WHITE_PIECE || endgame->sourceTile == PLAYED_PIECE)) ||
                            (initialTile >= BLACK_1 && initialTile <= BLACK_7 &&
                             (endgame->sourceTile == BLACK_PIECE || endgame->sourceTile == PLAYED_PIECE)))
                    {
                        // ... and then through each tile for the endgame pattern
                        patternsMatched = 0;
                        csArray<Endgame_TileSpec*>::Iterator egTileIterator(endgame->endgameTiles.GetIterator());
                        while(egTileIterator.HasNext())
                        {
                            Endgame_TileSpec* endgameTile = egTileIterator.Next();
                            if(colCount+endgameTile->col > gameBoardDef->cols || rowCount+endgameTile->row > gameBoardDef->rows ||
                                    colCount+endgameTile->col < 0 || rowCount+endgameTile->row < 0)
                                break;
                            tileAtPos = Get(colCount+endgameTile->col, rowCount+endgameTile->row);
                            if(tileAtPos == DISABLED_TILE || endgame->sourceTile == FOLLOW_SOURCE_TILE)
                                break;
                            if(endgameTile->tile == PLAYED_PIECE && (tileAtPos < WHITE_1 || tileAtPos > BLACK_7))
                                break;
                            if(endgameTile->tile == WHITE_PIECE && (tileAtPos < WHITE_1 || tileAtPos > WHITE_7))
                                break;
                            if(endgameTile->tile == BLACK_PIECE && (tileAtPos < BLACK_1 || tileAtPos > BLACK_7))
                                break;
                            if(endgameTile->tile == NO_PIECE && tileAtPos != EMPTY_TILE)
                                break;
                            if(endgameTile->tile == FOLLOW_SOURCE_TILE && tileAtPos != initialTile)
                                break;

                            // if here, then the pattern has another match
                            patternsMatched++;
                        }

                        // if all patterns matched, a winner
                        if(endgame->endgameTiles.GetSize() == patternsMatched)
                        {
                            // determine winning piece - using last tile of the pattern if no explicit winner is defined.
                            winningPiece = (endgame->winner != NO_PIECE) ? endgame->winner : EndgameWinner(tileAtPos);
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

Endgame_TileType psMiniGameBoard::EndgameWinner(uint8_t winningTileState)
{
    // determine winning piece - using passed-in state of winning tile.
    if(winningTileState >= WHITE_1 && winningTileState <= WHITE_7)
    {
        return WHITE_PIECE;
    }
    else if(winningTileState >= BLACK_1 && winningTileState <= BLACK_7)
    {
        return BLACK_PIECE;
    }
    else
    {
        return NO_PIECE; // really means indeterminate winner
    }
}

