/*
 * zonehandler.h    Keith Fulton <keith@paqrat.com>
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
#ifndef ZONEHANDLER_H
#define ZONEHANDLER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"
#include "paws/pawstexturemanager.h"
#include "psmovement.h"

//=============================================================================
// Local Includes
//=============================================================================

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------
class MsgHandler;
class psCelClient;
class pawsLoadWindow;
class pawsProgressBar;

/** @brief Information for loading a specific zone
 *
 * The ZoneHandler keeps a tree of these structures, loaded
 * from an XML file, which specifies which regions are to
 * be loaded when in that particular sector.  Every sector
 * in the world will need an entry in that xml file, because
 * the client will use this list also when the game loads
 * to make sure all relevant pieces are loaded no matter where
 * the player's starting point is.
 */
class ZoneLoadInfo
{
public:
    csString    inSector;     ///< Which sector this zone applies to
    csString    loadImage;    ///< Optional loading screen image
    csRef<iStringArray> regions; ///< Regions to load on sector load

    /** @brief Basic constructor built from a XML node
     *
     * Builds the zone load information from the XML node. Attributes are mapped
     * from "sector" to \ref inSector and "loadimage" to \ref loadImage.
     * Each child node named "region" gets attribute "map" added to \ref regions
     *
     * @param node The XML node to extract the information out of
     */
    ZoneLoadInfo(iDocumentNode* node);

    /** @brief Basic comparison for equality
     *
     * Compares one ZoneLoadInfo to another using \ref inSector as the comparison
     * field.
     *
     * @param other The ZoneLoadInfo to compare to
     * @return True if \ref inSector == other.inSector, false otherwise
     */
    bool operator==(ZoneLoadInfo &other) const
    {
        return inSector == other.inSector;
    }

    /** @brief Basic comparison for less than
     *
     * Compares one ZoneLoadInfo to another using \ref inSector as the comparison
     * field.
     *
     * @param other The ZoneLoadInfo to compare to
     * @return True if the character comparison of \ref inSector to other.inSector is < 0
     */
    bool operator<(ZoneLoadInfo &other) const
    {
        return (strcmp(inSector,other.inSector)<0);
    }
};


/** @brief Ensures all regions that need to be loaded are
 *
 * Listens for crossing sector boundaries and makes sure that we have all the
 * right stuff loaded in each zone.
 */
class ZoneHandler : public psClientNetSubscriber
{
public:
    /** @brief Basic constructor
     *
     * Sets initial values and calls \ref LoadZoneInfo(). Sets \ref valid to
     * true if everything checked out good
     *
     * @param mh The message handler to subscribe messages to
     * @param cc Used to get player information out of
     */
    ZoneHandler(MsgHandler* mh, psCelClient* cc);

    /** @brief Basic deconstructor
     */
    virtual ~ZoneHandler();

    /** @brief Handles messages from the server
     *
     * Handles \ref psNewSectorMessage by loading the target zone and will call
     * \ref ForceLoadScreen if there is no load delay
     *
     * @param me The message to create the \ref psNewSectorMessage out of
     */
    void HandleMessage(MsgEntry* me);

    /** @brief Loads a zone and places the user into it
     *
     * After the zone is found (from the sector parameter), the player is moved to
     * a temporary off screen location. While there the loading screen is
     * displayed and the zone is loaded. After the zone has finished loading,
     * the player is moved to the target position.
     *
     * @param pos The target position in the new zone
     * @param yrot The target rotation angle in the new zone
     * @param sector The name of the target zone
     * @param vel Target velocity
     * @param force Whether to force the loading of the target zone
     */
    void LoadZone(csVector3 pos, float yrot, const char* sector, csVector3 vel, bool force = false);

    /** @brief Called after drawing on screen has finished.
     *
     * Checks if player just crossed boundary between sectors and loads/unloads
     * needed maps
     */
    void OnDrawingFinished();

    /** @brief Moves player to given location
     *
     * @param Pos Target position to move to
     * @param newSector Target sector to move to
     * @param Vel Target velocity
     */
    void MovePlayerTo(const csVector3 &Pos, float yRot, const csString &newSector, const csVector3 &Vel);

    /** @brief Handles delay and dot animation
    *
    * @param loadDelay Delay of loading screen
    * @param start Start of dot animation
    * @param dest Destination of dot animation
    * @param background The loading background
    * @param widgetName The name of the widget to use for this loading.
    */
    void HandleDelayAndAnim(int32_t loadDelay, csVector2 start, csVector2 dest, csString background, csString widgetName);

    /** @brief Returns if this is loading a zone
     *
     * @return \ref loading
     */
    inline bool IsLoading() const
    {
        return loading;
    }

    /** @brief Returns whether the zone handler is valid
     *
     * @return \ref valid
     */
    inline bool IsValid() const
    {
        return valid;
    }

protected:
    csHash<ZoneLoadInfo*, const char*> zonelist;    ///< Mapping of names of zones to their load info
    csRef<MsgHandler>                   msghandler; ///< Message Handler to subscribe to
    psCelClient*                        celclient;  ///< Pointer to cel client instance

    bool valid; ///< Whether the loading was successful
    csString sectorToLoad; ///< The sector that needs to be loaded
    csVector3 newPos; ///< The target location the player will move to after loading
    csVector3 newVel; ///< The velocity the player will have after loading
    float newyrot; ///< The rotation the player will have after loading
    psMoveState moveState;
    bool loading; ///< Whether a new zone is currently being loaded
    csString forcedBackgroundImg; ///<String which holds the background of the loading screen
    csTicks forcedLoadingEndTime;///<Holds how long the loading shall be delayed
    csTicks forcedLoadingStartTime;///<Holds how long the loading shall be delayed
    csString forcedWidgetName;///<Holds the widget name used to replace the load window.
    size_t loadCount; ///< The number of items that are being loaded

    pawsLoadWindow* loadWindow; ///< A load window that can be shown to users while loading
    pawsProgressBar* loadProgressBar; ///< Used for showing users load progress

    /** @brief Finds the loading window
     *
     * Checks if there is a loading window. If there is not a loading window
     * or the loading window does not have a progress bar, false is returned.
     *
     * @param force If true it will force the window to be searched even if we have it already
     * @param widgetName if defined it will hook the loading windows to the defined name
     * @return True if a valid window was found, false otherwise.
     */
    bool FindLoadWindow(bool force = false, const char* widgetName = "LoadWindow");

    /** @brief Extracts zone information out of a XML
     *
     * Checks "/planeshift/data/zoneinfo.xml" for a valid XML. Each element
     * named "zone" under "zonelist" has a \ref ZoneLoadInfo created out of it.
     * All \ref ZoneLoadInfo are then added to zonelist based on their sector
     *
     * @return False if the file failed to load, true otherwise
     */
    bool LoadZoneInfo();

    /** @brief Finds load info for a specific sector
     *
     * Searches \ref zonelist for the key specified in sector. If it is not
     * found null is returned and an error is printed
     *
     * @param sector The sector to look for
     * @return The load info for the sector or null
     */
    ZoneLoadInfo* FindZone(const char* sector) const;

private:
    /** @brief Forces load screen to show.
     *
     * Can also have the load screen stay for a defined amount of time.
     * Also allows to change the background of the load screen.
     * @param backgroundImage The image to put in the load screen.
     * @param length the minimum length the load screen will show up
     * @param widgetName The name of the widget to use during the forced load screen.
     */
    void ForceLoadScreen(csString backgroundImage, uint32_t length, csString widgetName);

    /** Loads the defined widget as load widget if enable is true else
     *  removes the defined widget and reloads the default load window
     *  @param enable Tells if we have to load a new widget or restore the canonical one.
     *  @param loadWindowName The widget name we work on
     *  @return TRUE if it was a success.
     */
    bool ForceLoadWindowWidget(bool enable, csString loadWindowName);
};

#endif
