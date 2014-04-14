/*
 * pawsLoadWindow.h - Author: Andrew Craig
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 
#ifndef PAWS_LOAD_WINDOW_HEADER
#define PAWS_LOAD_WINDOW_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "paws/pawswidget.h"
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------
class pawsMessageTextBox;


/**
 * This is the window that is displayed when the game is loading the maps.
 */
class pawsLoadWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    virtual ~pawsLoadWindow();
    
    bool PostSetup();
    void HandleMessage( MsgEntry* me );
    void AddText( const char* text );
    void Clear();
    void Show();
    void Hide();
    void Draw();
    
    /**
     * Initializing of the dot animation.
     *
     * @param start A vector representing the start of the animation.
     * @param dest A vector representing the destination of the animation.
     * @param delay
     */
    void InitAnim(csVector2 start, csVector2 dest, csTicks delay);

    /**
     * Sends the motd message to local listeners.
     * 
     * Used to handle the update of newly spawned custom loading windows.
     */
    void PublishMOTD();

private:
    pawsMessageTextBox* loadingText;
    
    bool renderAnim;///<Bool used to switch off or on the dot animation
    csVector2 lastPos;///<Position of the last rendered dot
    csVector2 vel;///<Velocity/space between the dots
    csVector2 diffVector;///<Diffusion vector, used to noise the velocity vector
    csVector2 destination;///<Destination of last dot
    csArray<csVector2> positions;///<The dots positions 
    csTicks delayBetDot;///<The delay between the dots
    csTicks startFrom;///<Time from last dot rendered
    size_t numberDot;///<Number of dots which shall be rendered
    csRef<iPawsImage> dot;///<The dot image

    csString guildName; ///< The name of the guild the user is associated to.
    csString guildMOTD; ///< The MOTD of the guild the user is associated to.

    // Default tip position (used for switch loading/travel screens)
    csRect tipDefaultRect;

    /** 
     *  Render the dot animation.
     *
     *  Called only if renderAnim == true
     */
    void DrawAnim();
};

CREATE_PAWS_FACTORY( pawsLoadWindow );

#endif

 
