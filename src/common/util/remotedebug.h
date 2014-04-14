/*
 * remotedebug.h by Anders Reggestad <andersr@pvv.org>
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __REMOTEDEBUG_H__
#define __REMOTEDEBUG_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Library Includes
//=============================================================================
#include "psstdint.h"

//=============================================================================
// Local Includes
//=============================================================================

/**
 * \addtogroup common_util
 * @{ */

// Forward declarations

/**
 * Remote debugging marcro.
 *
 * Define a variadic macro for debugging prints for Remote debugging.
 * In this way only the check IsDebugging is executed for unless debugging
 * is turned on. Using the Printf functions will cause all args to be
 * resolved and that will result in a lot off spilled CPU.
 */
#define RDebug(debugEntity,debugLevel,...) \
    { if (debugEntity->IsDebugging()) { debugEntity->Printf(debugLevel, __VA_ARGS__); }}


/**
 * Keep track of remote debugging.
 *
 * Track debugging clients and debug levels.
 *
 */
class RemoteDebug
{
public:
    /**
     * Constructor.
     */
    RemoteDebug();

    /**
     * Destructor.
     */
    virtual ~RemoteDebug();
    
    /**
     * Check if debugging is enabled.
     */
    inline bool IsDebugging()
    {
        return (highestActiveDebugLevel > 0);
    };
    
    /**
     * Check if this NPC is debugging at the given level.
     *
     * @param debugLevel The debug level.
     */
    bool IsDebugging(int debugLevel)
    {
        return (highestActiveDebugLevel > 0 && debugLevel <= highestActiveDebugLevel);
    };

    /**
     * Switch the local debuging state.
     */
    bool SwitchDebugging();

    /**
     * Set a new debug level.
     *
     * @param debug New debug level, 0 is no debugging
     */
    void SetDebugging(int debugLevel);

    /**
     * Get the local debugging level.
     */
    int GetDebugging() const;

    /**
     * Add a client to receive debug information.
     *
     * @param clientNum The client to add.
     * @param debugLevel The debug level for this client.
     */
    void AddDebugClient(uint clientNum, int debugLevel);

    /**
     * Remove client from list of debug receivers.
     *
     * @param clientNum The client to remove.
     */
    void RemoveDebugClient(uint clientNum);

    /**
     * Utility function to retrive a string with all remote debugging clients for debug outputs.
     */
    csString GetRemoteDebugClientsString() const;
    
    /**
     * Function to generate a debug print.
     *
     * The text will be distributed to all clients currently debugging
     * at the debug level given.
     */
    void Printf(int debugLevel, const char* msg,...);
    
protected:
    /**
     * Callback function to report local debug.
     */
    virtual void LocalDebugReport(const csString& debugString) = 0;
    
    /**
     * Callback function to report remote debug.
     */
    virtual void RemoteDebugReport(uint32_t clientNum, const csString& debugString) = 0;
    
private:
    /**
     * Hold client information for remote debugging clients.
     */
    struct DebugClient
    {
        /**
         * Constructor.
         */
        DebugClient(uint32_t clientNum, int debugLevel):clientNum(clientNum),debugLevel(debugLevel) {};

        /**
         * Debug clients is the same if the clientNum is equal.
         */
        bool operator==(const DebugClient& other) const { return clientNum == other.clientNum; }
            
        
        uint32_t clientNum; ///< Client number
        int debugLevel;     ///< The active debug level for this client.
    };
    
    /**
     * Internal function used by Printf.
     */
    void VPrintf(int debugLevel, const char* msg, va_list args);

    /**
     * Internal function to calculate the hightest active debug level.
     *
     * Combinde the localDebugLevel and the debug level from all clients.
     */
    void CalculateHighestActiveDebugLevel();

    int                  localDebugLevel;         ///< Current local debug level.
    csArray<DebugClient> debugClients;            ///< List of clients doing debugging
    int                  highestActiveDebugLevel; ///< The current highest active debugging level for any clients.

protected:
    csArray<csString>    debugLog;                ///< Local debug log of last n print statments.
    int                  nextDebugLogEntry;       ///< The next entry to use.
};

/** @} */

#endif
