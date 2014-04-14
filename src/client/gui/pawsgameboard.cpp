/*
* pawsgameboard.cpp - Author: Enar Vaikene
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


// STANDARD INCLUDE
#include <psconfig.h>
#include "globals.h"

// COMMON/NET INCLUDES
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

#include "util/log.h"

#include "pawsgameboard.h"

using namespace psMiniGame;


//-----------------------------------------------------------------------------

pawsGameBoard::pawsGameBoard()
{

    dragging = false;
    draggingPiece = NULL;
    gameOptions = 0;
    cols = 0;
    rows = 0;

    // Setup the piece ID to art file conversion table
    pieceArt.Put(WHITE_1, "Minigame Piece White 1");
    pieceArt.Put(WHITE_2, "Minigame Piece White 2");
    pieceArt.Put(WHITE_3, "Minigame Piece White 3");
    pieceArt.Put(WHITE_4, "Minigame Piece White 4");
    pieceArt.Put(WHITE_5, "Minigame Piece White 5");
    pieceArt.Put(WHITE_6, "Minigame Piece White 6");
    pieceArt.Put(WHITE_7, "Minigame Piece White 7");
    pieceArt.Put(BLACK_1, "Minigame Piece Black 1");
    pieceArt.Put(BLACK_2, "Minigame Piece Black 2");
    pieceArt.Put(BLACK_3, "Minigame Piece Black 3");
    pieceArt.Put(BLACK_4, "Minigame Piece Black 4");
    pieceArt.Put(BLACK_5, "Minigame Piece Black 5");
    pieceArt.Put(BLACK_6, "Minigame Piece Black 6");
    pieceArt.Put(BLACK_7, "Minigame Piece Black 7");
}

pawsGameBoard::~pawsGameBoard()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_MINIGAME_BOARD);
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_MINIGAME_STARTSTOP);
}

bool pawsGameBoard::PostSetup()
{
    // Subscribe to Minigame messages
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_MINIGAME_BOARD);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_MINIGAME_STARTSTOP);

    return true;
}

void pawsGameBoard::HandleMessage(MsgEntry *message)
{

    if (message->GetType() == MSGTYPE_MINIGAME_BOARD)
    {
        // Got the game board layout from the server
        psMGBoardMessage msg(message);
        if (!msg.valid)
        {
            Error1("Failed to parse psMGBoardMessage from server.");
            return;
        }
        Debug2(LOG_MINIGAMES, 0, "psMGBoardMessage: %s\n", msg.ToString(0).GetData());

        // Verify the message counter
        if (counterSet && !msg.IsNewerThan(currentCounter))
        {
            Error3("Ignoring game board message with version %d because our version is %d.",
                     msg.msgCounter, currentCounter);
            return;
        }
        currentCounter = msg.msgCounter;
        if (!counterSet)
            counterSet = true;

        // if this update is resetting a disallowed move, need to resync
        // with server & other clients, so undo the counter by 1.
        if (msg.msgOptions & DISALLOWED_MOVE)
            currentCounter--;

        // Update or setup the game board
        UpdateBoard(msg);
    }

    else if (message->GetType() == MSGTYPE_MINIGAME_STARTSTOP)
    {
        // Game stopped by the server

        psMGStartStopMessage msg(message);

        if (!msg.valid)
        {
            if (!msg.valid)
            {
                Error1("Failed to parse psMGStartStopMessage from server.");
                return;
            }
        Debug2(LOG_MINIGAMES, 0, "psMGStartStopMessage: %s\n", msg.ToString(0).GetData());

        }

        if (!msg.msgStart && visible)
        {
            pawsWidget::Hide();
        }
    }

}

void pawsGameBoard::Hide()
{
    pawsWidget::Hide();

    // Stop the game
    psMGStartStopMessage msg(0, false);
    msg.SendMessage();
}

void pawsGameBoard::StartGame()
{
    // Clean the board
    CleanBoard();

    // Reset the current counter
    currentCounter = 0;
    counterSet = false;

    // Request to start the game
    psMGStartStopMessage msg(0, true);
    msg.SendMessage();
}

void pawsGameBoard::CleanBoard()
{
    // Remove tiles from the board window
    csArray<pawsGameTile *>::Iterator iter = tiles.GetIterator();
    while (iter.HasNext())
        DeleteChild(iter.Next());

    // Clean the array
    tiles.DeleteAll();

    cols = 0;
    rows = 0;
}

void pawsGameBoard::SetupBoard(psMGBoardMessage &msg)
{

    if (msg.msgCols <= GAMEBOARD_MIN_COLS || msg.msgRows <= GAMEBOARD_MIN_ROWS )
    {
        Error1("Invalid number of colums or rows for the game board.");
        return;
    }

    gameID = msg.msgGameID;

    // Calculate width and height for one game tile
    csRect frame = GetScreenFrame();
    int x = GetActualWidth(32);
    int y = GetActualHeight(32);
    int w = (frame.xmax-frame.xmin - 2*x) / msg.msgCols;
    int h = (frame.ymax-frame.ymin - 2*y) / msg.msgRows;

    // Limit the tile size to 64 or game pieces will look ugly
    if (w > GetActualWidth(64))
        w = GetActualWidth(64);
    if (h > GetActualHeight(64))
        h = GetActualHeight(64);

    // Use the smallest value for width and height, and recalculate starting points
    if (w < h)
        h = w;
    else
        w = h;
    x = (frame.xmax - frame.xmin - w*msg.msgCols)/2;
    y = (frame.ymax - frame.ymin - h*msg.msgRows)/2;

    Debug5(LOG_MINIGAMES, 0, "Using x = %d, y = %d, w = %d and h = %d", x, y, w, h);

    if (w <= 10 || h <= 10)
    {
        Error1("Too many rows or columns for the game board");
        return;
    }

    // Clean the previous board
    CleanBoard();

    cols = msg.msgCols;
    rows = msg.msgRows;

    gameOptions = msg.msgOptions;

    // Start from the bottom-right corner if we play with black pieces
    if (gameOptions & BLACK_PIECES)
    {
        x += w*cols;
        y += h*rows;
    }

    // Start with a white (or black) tile
    bool white = !(gameOptions & BLACK_SQUARE);

    for (int8_t i = 0; i < rows; i++)
    {
        for (int8_t j = 0; j < cols; j++)
        {
            uint8_t state = EMPTY_TILE;

            // Check the layout
            if (msg.msgLayout)
            {
                int idx = i*cols + j;
                uint8_t v = msg.msgLayout[idx / 2];
                if (idx % 2)
                    state = v & 0x0F;
                else
                    state = (v & 0xF0) >> 4;
            }

            if (state != DISABLED_TILE)
            {

                pawsGameTile *tile = new pawsGameTile;
                if (!tile)
                {
                    Error1("Could not create widget pawsGameTile");
                    return;
                }
                tile->SetGameBoard(this);
                tiles.Push(tile);
                AddChild(tile);

                if (white)
                    tile->SetBackground("Minigame Tile White");
                else
                    tile->SetBackground("Minigame Tile Black");

                // Reposition the game tile
                if (gameOptions & BLACK_PIECES)
                    tile->SetRelativeFrame(x-(j+1)*w, y-(i+1)*h, w, h);
                else
                    tile->SetRelativeFrame(x+j*w, y+i*h, w, h);

                tile->SetFade(fade);
                tile->SetState(state);
                tile->SetColumn(j);
                tile->SetRow(i);
            }

            // Flip colors
            if (!(gameOptions & PLAIN_SQUARES))
                white = !white;

        }

        // Flip the color once more if we have an even number of columns
        if (cols % 2 == 0 && !(gameOptions & PLAIN_SQUARES))
            white = !white;

    }

    // List of available pieces
    whitePieces.Empty();
    blackPieces.Empty();
    if (msg.msgNumOfPieces > 0 && msg.msgPieces)
    {
        for (size_t i = 0; i < msg.msgNumOfPieces; i++)
        {
            size_t idx = i/2;
            uint8_t piece;
            if (i%2)
                piece = msg.msgPieces[idx] & 0x0F;
            else
                piece = (msg.msgPieces[idx] & 0xF0) >> 4;
            if (piece >= WHITE_1 && piece <= WHITE_7)
                whitePieces.Push(piece);
            else if (piece >= BLACK_1 && piece <= BLACK_7)
                blackPieces.Push(piece);
            Debug2(LOG_MINIGAMES, 0, "Available Piece: %u", piece);
        }
    }
    else
    {
        // By default we will have only one white and one black variant of pieces
        whitePieces.Push(WHITE_1);
        blackPieces.Push(BLACK_1);
        Debug1(LOG_MINIGAMES, 0, "Available Pieces: Only one black and one white");
    }

    // Show the window
    Show();
}

void pawsGameBoard::UpdateBoard(psMGBoardMessage &msg)
{
    // The number of columns and rows shouldn't change, but if it id, setup a new board
    if (gameID != msg.msgGameID || cols != msg.msgCols || rows != msg.msgRows)
    {
        SetupBoard(msg);
        return;
    }

    // This time we need the layout
    if (!msg.msgLayout)
    {
        Error1("No layout given to update the game board.");
        return;
    }

    gameOptions = msg.msgOptions;

    size_t k = 0;
    for (int8_t i = 0; i < rows; i++)
    {
        for (int8_t j = 0; j < cols; j++)
        {
            uint8_t state;

            // Check the layout
            int idx = i*cols + j;
            unsigned char v = msg.msgLayout[idx / 2];
            if (idx % 2)
                state = v & 0x0F;
            else
                state = (v & 0xF0) >> 4;

            if (state != DISABLED_TILE && k < tiles.GetSize())
            {
                tiles[k++]->SetState(state);
            }

        }

    }

    // List of available pieces may change if it depends on the current board status
    if (msg.msgNumOfPieces > 0 && msg.msgPieces)
    {
        Debug1(LOG_MINIGAMES, 0, "Received update of available Pieces");
        whitePieces.Empty();
        blackPieces.Empty();
        for (size_t i = 0; i < msg.msgNumOfPieces; i++)
        {
            size_t idx = i/2;
            uint8_t piece;
            if (i%2)
                piece = msg.msgPieces[idx] & 0x0F;
            else
                piece = (msg.msgPieces[idx] & 0xF0) >> 4;
            if (piece >= WHITE_1 && piece <= WHITE_7)
                whitePieces.Push(piece);
            else if (piece >= BLACK_1 && piece <= BLACK_7)
                blackPieces.Push(piece);
            Debug2(LOG_MINIGAMES, 0, "Available Piece: %u", piece);
        }
    }
}

void pawsGameBoard::DropPiece(pawsGameTile *tile)
{
    PawsManager::GetSingleton().SetDragDropWidget(NULL);
    dragging = false;
    pawsGameTile *oldTile = draggingPiece;
    draggingPiece = NULL;

    // Don't send updates if the old tile and new tile are the same
    if (oldTile && tile &&
        oldTile->GetColumn() == tile->GetColumn() && oldTile->GetRow() == tile->GetRow())
        return;

    // Fill in the buffer with updates.
    uint8_t updates[4];
    int idx = 0;
    uint8_t cnt = 0;

    // Old location shall be set to empty.
    if (oldTile)
    {
        updates[idx++] = (oldTile->GetColumn() << 4) + oldTile->GetRow();
        updates[idx++] = EMPTY_TILE;
        cnt++;
    }
    // New location.
    if (tile)
    {
        updates[idx++] = (tile->GetColumn() << 4) + tile->GetRow();
        updates[idx++] = tile->GetState();
        cnt++;
    }

    // Send to the server.
    if (cnt > 0)
    {
        psMGUpdateMessage msg(0, ++currentCounter, gameID, cnt, updates);
        msg.SendMessage();
    }
}

void pawsGameBoard::UpdatePiece(pawsGameTile *tile)
{
    if (!tile)
        return;

    // Fill in the buffer with updates.
    uint8_t updates[2];
    updates[0] = (tile->GetColumn() << 4) + tile->GetRow();
    updates[1] = tile->GetState();

    // Send to the server.
    psMGUpdateMessage msg(0, ++currentCounter, gameID, 1, updates);
    msg.SendMessage();
}

void pawsGameBoard::StartDragging(pawsGameTile *tile)
{
    dragging = true;
    draggingPiece = tile;

    pawsGameTile *widget = new pawsGameTile;
    widget->SetGameBoard(this);
    widget->SetParent(NULL);
    widget->SetRelativeFrame(0, 0, tile->GetDefaultFrame().Width(), tile->GetDefaultFrame().Height());
    widget->SetState(tile->GetState());
    PawsManager::GetSingleton().SetDragDropWidget(widget);
}

uint8_t pawsGameBoard::NextPiece(uint8_t current) const
{
    if (current >= WHITE_1 && current <= WHITE_7)
    {
        size_t i = 0;

        // Search for the current white piece
        while (i < whitePieces.GetSize() && whitePieces[i] != current)
            i++;
        if (i >= whitePieces.GetSize())
            return current; // The current piece was not found in the array
        else
        {
            i++; // Next piece
            if (i >= whitePieces.GetSize())
                i = 0;
            return (uint8_t)whitePieces[i];

        }

    }
    else if (current >= BLACK_1 && current <= BLACK_7)
    {
        size_t i = 0;

        // Search for the current black piece
        while (i < blackPieces.GetSize() && blackPieces[i] != current)
            i++;
        if (i >= blackPieces.GetSize())
            return current; // The current piece was not found in the array
        else
        {
            i++; // Next piece
            if (i >= blackPieces.GetSize())
                i = 0;
            return (uint8_t)blackPieces[i];

        }

    }
    else
        return EMPTY_TILE;
}

const csString pawsGameBoard::PieceArtName(uint8_t piece) const
{
    return pieceArt.Get(piece, "");
}

//-----------------------------------------------------------------------------

pawsGameTile::pawsGameTile()
    : pawsWidget()
{
    board = 0;

    visible = true;
    movable = false;
    isResizable = false;
    configurable = false;
    fade = true;

    state = EMPTY_TILE;
    oldState = EMPTY_TILE;
    column = -1;
    row = -1;
}

pawsGameTile::~pawsGameTile()
{
}

void pawsGameTile::SetGameBoard(pawsGameBoard *gameBoard)
{
    board = gameBoard;
}

void pawsGameTile::SetColumn(int8_t newColum)
{
    column = newColum;
}

void pawsGameTile::SetRow(int8_t newRow)
{
    row = newRow;
}

void pawsGameTile::Draw()
{
    pawsWidget::Draw();

    if (state != EMPTY_TILE)
    {
        csRect frame = screenFrame;
        ClipToParent(false);
        image->Draw(frame);
    }
}

bool pawsGameTile::OnMouseDown(int button, int modifiers, int x, int y)
{
    if (!board || board->GetGameOptions() & READ_ONLY)
        return false;

    // Use the EntitySelect mouse bind to select pieces/create player's pieces
    if (psengine->GetMouseBinds()->CheckBind("EntitySelect", button, modifiers))
    {
        pawsGameTile *tile = dynamic_cast<pawsGameTile *>
            (PawsManager::GetSingleton().GetDragDropWidget());
        if (tile)
        {
            SetState(tile->GetState());
            board->DropPiece(this);
        }
        else if (state == EMPTY_TILE)
        {
            SetState(board->GetGameOptions() & BLACK_PIECES ?
                    board->BlackPiecesList(0) : board->WhitePiecesList(0));
            board->UpdatePiece(this);
        }
        else if (!(board->GetGameOptions() & OPTION_PLACE_ONLY))
        {
            board->StartDragging(this);
            SetState(EMPTY_TILE);
        }
        return true;
    }

    // Use the ContextMenu mouse bind to create oponent's pieces
    if (psengine->GetMouseBinds()->CheckBind("ContextMenu", button, modifiers))
    {
        if (state == EMPTY_TILE)
        {
            SetState(board->GetGameOptions() & BLACK_PIECES ?
                    board->WhitePiecesList(0) : board->BlackPiecesList(0));
            board->UpdatePiece(this);
        }
        return true;
    }

    // Forward to the parent
    if (parent)
        return parent->OnMouseDown(button, modifiers, x, y);

    else
        return pawsWidget::OnMouseDown(button, modifiers, x, y);
}

bool pawsGameTile::OnDoubleClick(int button, int modifiers, int x, int y)
{
    if (!board || board->GetGameOptions() & READ_ONLY)
        return false;

    if (psengine->GetMouseBinds()->CheckBind("EntitySelect", button, modifiers) && state != EMPTY_TILE)
    {

        SetState(board->NextPiece(state));
        board->UpdatePiece(this);

        return true;
    }

    // Forward to the parent
    if (parent)
        return parent->OnDoubleClick(button, modifiers, x, y);

    else
        return pawsWidget::OnDoubleClick(button, modifiers, x, y);
}

void pawsGameTile::SetState(uint8_t state)
{
    if (!board)
        return;

    if (state != this->state)
    {
        if (state != EMPTY_TILE && state != DISABLED_TILE)
        {
            csString art = board->PieceArtName(state);
            if (art.IsEmpty())
            {
                Error2("Invalid minigame state %u", state);
                return;
            }

            image = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(art);
            if (!image)
            {
                Error2("Could not create image \"%s\"", art.GetData());
                return;
            }
        }

        oldState = this->state;
        this->state = state;
    }
}

void pawsGameTile::RestoreState()
{
    SetState(oldState);
}

