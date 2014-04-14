/*
* minigame.h - Author: Enar Vaikene
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


#ifndef __MINIGAME_H
#define __MINIGAME_H

/**
 * \addtogroup common_util
 * @{ */

/// Globals for minigames
namespace psMiniGame
{
/**
 * Game board limits
 */
#define GAMEBOARD_MIN_COLS 1
#define GAMEBOARD_MAX_COLS 16
#define GAMEBOARD_MIN_ROWS 1
#define GAMEBOARD_MAX_ROWS 16

    /// Minigame options
    enum GameOptions
    {
        MANAGED_GAME         = 0x01, ///< A game managed by the server.
        BLACK_PIECES         = 0x02, ///< Player with black pieces.
        READ_ONLY            = 0x04, ///< The game is read-only (for watchers).
        PERSONAL_GAME        = 0x08, ///< The game is personal & private
        BLACK_SQUARE         = 0x10, ///< Top left/all squares Black. Else white.
        PLAIN_SQUARES        = 0x20, ///< Board squares all plain. Else checked.
        DISALLOWED_MOVE      = 0x40, ///< Last move disallowed
        OBSERVE_ENDGAME      = 0x80, ///< observe endgame play
        OPTION_PLACE_ONLY    = 0x8000 ///< don't pick up pieces
    };

    /// Minigame tile state values
    enum TileStates
    {
        EMPTY_TILE         = 0,     ///< An empty game tile.

        WHITE_1            = 1,     ///< White regular game piece
        WHITE_2,
        WHITE_3,
        WHITE_4,
        WHITE_5,
        WHITE_6,
        WHITE_7,

        BLACK_1            = 8,    ///< Black regular game piece.
        BLACK_2,
        BLACK_3,
        BLACK_4,
        BLACK_5,
        BLACK_6,
        BLACK_7,

        DISABLED_TILE      = 15    ///< Disable game tile.
    };

}

/** @} */

#endif

