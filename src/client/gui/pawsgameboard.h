/*
* pawsgameboard.h - Author: Enar Vaikene
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

#ifndef __PAWSGAMEBOARD_H
#define __PAWSGAMEBOARD_H

#include "util/minigame.h"
#include "net/messages.h"
#include "paws/pawswidget.h"

class pawsGameBoard;

/** Game tile class.
 *
 * Defines one game tile with or without game pieces on it.
 */
class pawsGameTile : public pawsWidget
{

    public:

        pawsGameTile();

        ~pawsGameTile();

        /// Sets the game board field.
        void SetGameBoard(pawsGameBoard *gameBoard);

        /// Sets the column number for this game tile.
        void SetColumn(int8_t newColumn);

        /// Returns the column number for this game tile.
        int8_t GetColumn() const { return column; }

        /// Sets the row number for this game tile.
        void SetRow(int8_t newRow);

        /// Returns the row number for this game tile.
        int8_t GetRow() const { return row; }

        /// Mouse event.
        virtual bool OnMouseDown(int button, int modifiers, int x, int y);

        /// Double-click event.
        virtual bool OnDoubleClick(int button, int modifiers, int x, int y);

        /// Draws the game tile.
        virtual void Draw();

        /// Return the game tile state.
        uint8_t GetState() const { return state; }

        /// Sets the game tile state.
        void SetState(uint8_t state);

        /// Restores the previous game tile state.
        void RestoreState();

    protected:

        /// The game board.
        pawsGameBoard *board;

        /// The column number on the game board.
        int8_t column;

        /// The row number on the game board.
        int8_t row;

        /// The current game tile state.
        uint8_t state;

        /// The previous game tile state.
        uint8_t oldState;

        /// The game piece image.
        csRef<iPawsImage> image;

};

/** The game window class.
 *
 * Minigame PAWS class handles the gameboard on the client:
 * moves from players and updates from the server.
 */
class pawsGameBoard : public pawsWidget, public psClientNetSubscriber
{
    public:

        pawsGameBoard();

        ~pawsGameBoard();

        /// Net messages handler.
        virtual void HandleMessage(MsgEntry *message);

        /// Subscribes to net messages.
        virtual bool PostSetup();

        /// Sends the stop game message to the server.
        virtual void Hide();

        /// Cleans the game board.
        void CleanBoard();

        /// Starts a new game session.
        void StartGame();

        /// Creates the game board.
        void SetupBoard(psMGBoardMessage &msg);

        /// Updates the game board.
        void UpdateBoard(psMGBoardMessage &msg);

        /// Returns the game ID value.
        uint32_t GetGameID() const { return gameID; }

        /// Returns game options.
        uint16_t GetGameOptions() const { return gameOptions; }

        /// Sets game options.
        void SetGameOptions(uint16_t newOptions);

        /// Returns the number of columns.
        int8_t GetCols() const { return cols; }

        /// Returns the number of rows.
        int8_t GetRows() const { return  rows; }

        /// Returns true if a game piece is being dragged.
        bool IsDragging() const { return dragging; }

        /// Starts dragging a game piece.
        void StartDragging(pawsGameTile *tile);

        /// Drops the game piece either to a new game tile or to void.
        void DropPiece(pawsGameTile *tile = 0);

        /// Updates the game when a game piece is changed.
        void UpdatePiece(pawsGameTile *tile);

        /// List of available white pieces.
        uint8_t WhitePiecesList(size_t idx) const
        {
            if (whitePieces.GetSize() > 0)
                return whitePieces.Get(idx);
            else
                return psMiniGame::WHITE_1;
        }

        /// List of available black pieces.
        uint8_t BlackPiecesList(size_t idx) const
        {
            if (blackPieces.GetSize() > 0)
                return blackPieces.Get(idx);
            else
                return psMiniGame::BLACK_1;
        }

        /// Number of available white pieces
        size_t WhitePiecesCount() const
        {
            if (whitePieces.GetSize() > 0)
                return whitePieces.GetSize();
            else
                return 1;
        }

        /// Number of available black pieces
        size_t BlackPiecesCount() const
        {
            if (blackPieces.GetSize() > 0)
                return blackPieces.GetSize();
            else
                return 1;
        }

        /// Next available game piece.
        uint8_t NextPiece(uint8_t current) const;

        /// Returns the name of the art resource.
        const csString PieceArtName(uint8_t piece) const;

    private:
        /// Array of game tiles.
        csArray<pawsGameTile *> tiles;

        /// Current message counter for versioning.
        uint8_t currentCounter;

        /// Indicates that the current message counter is set.
        bool counterSet;

        /// The game ID value (equals to the action location ID value on the server).
        uint32_t gameID;

        /// Game options.
        uint16_t gameOptions;

        /// Number of columns.
        int8_t cols;

        /// Number of rows.
        int8_t rows;

        /// Indicates that a game piece is being dragged.
        bool dragging;

        /// The game piece that is being dragged.
        pawsGameTile *draggingPiece;

        /// Piece ID to art file conversion table.
        csHash<csString, int> pieceArt;

        /// List of available white pieces
        csArray<int> whitePieces;

        /// List of available black pieces
        csArray<int> blackPieces;

};

CREATE_PAWS_FACTORY(pawsGameBoard);
CREATE_PAWS_FACTORY(pawsGameTile);

#endif
