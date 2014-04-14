/*
 * hiresession.h  creator <andersr@pvv.org>
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
#ifndef HIRE_SESSION_HEADER
#define HIRE_SESSION_HEADER
//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================

//====================================================================================
// Local Includes
//====================================================================================
#include <gem.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------

/** The Hire Sessin will manage all aspects related to a specefic hiring of a NPC.
 *
 *  Players is allowed to Hire NPCs to do tasks for them. This can include
 *  selling of items, standing guard for the guild, etc. This session handle
 *  the details for the hire session. The main business logic for hiring is
 *  managed by the HireManager.
 */
class HireSession
{
public:
    /** Constructor.
     */
    HireSession();

    /** Constructor.
     *
     * @param owner The player that start to hire a NPCs.
     */
    HireSession(gemActor* owner);

    /** Destructor.
     */
    virtual ~HireSession();

    /** Load from db.
     */
    bool Load(iResultRow &row);

    /** Save to db.
     *
     *  @param newSession Set to true if this is a new session.
     */
    bool Save(bool newSession = false);

    /** Delete from db.
     */
    bool Delete();

    /** Set pending hire type.
     *
     *  @param name The name of the NPC type to hire.
     *  @param npcType The NPC Type of the NPC type to hire type.
     */
    void SetHireType(const csString &name, const csString &npcType);

    /** Get hire type.
     *
     *  @return The NPC Type of the NPC type to hire type.
     */
    const csString &GetHireTypeName() const;

    /** Get hire type.
     *
     *  @return The NPC Type of the NPC type to hire type.
     */
    const csString &GetHireTypeNPCType() const;

    /** Set pending hire master NPC PID.
     *
     *  @param masterPID The PID of the master NPC.
     */
    void SetMasterPID(PID masterPID);

    /** Get Master PID.
     *
     *  @return The Master PID.
     */
    PID GetMasterPID() const;

    /** Set owner.
     *
     *  @param owner The hired NPC owner.
     */
    void SetOwner(gemActor* owner);

    /** Get owner.
     */
    gemActor* GetOwner() const;

    /** Set hired NPC.
     *
     *  @param hiredNPC The hired NPC.
     */
    void SetHiredNPC(gemNPC* hiredNPC);

    /** Get hired NPC.
     */
    gemNPC* GetHiredNPC() const;

    /** Verify if all requirments for hire is ok.
     *
     *  @return True if ok to confirm hire.
     */
    bool VerifyPendingHireConfigured();

    /** Return the PID of the owner.
     *
     *  @return PID of the owner.
     */
    PID GetOwnerPID();

    /** Return the PID of the hired NPC.
     *
     *  @return PID of the hired NPC.
     */
    PID GetHiredPID();

    /** Return the hired NPC script.
     */
    const csString &GetScript() const;

    /** Set the hired NPC script.
     */
    void SetScript(const csString &newScript);

    /** Get work postion.
     */
    int GetWorkLocationId();

    /** Get work postion.
     */
    Location* GetWorkLocation();

    /** Set a new working location.
     */
    void SetWorkLocation(Location* location);

    /** Check if the script is verified.
     */
    bool IsVerified();

    /** Set verified.
     */
    void SetVerified(bool state);


    /** Return the verified hired NPC script.
     */
    const csString &GetVerifiedScript() const;

    /** Save a verified script.
     */
    void SetVerifiedScript(const csString &newVerifiedScript);

    /** Get work locaiton string.
     */
    csString GetWorkLocationString();

    /** Get work postion string.
     */
    csString GetTempWorkLocationString();

    /** Get work postion.
     */
    Location* GetTempWorkLocation();

    /** Set a new working location.
     */
    void SetTempWorkLocation(Location* location);

    /** Set validity of temp work postion.
     */
    void SetTempWorkLocationValid(bool valid);

    /** Get validity of temp work postion.
     */
    bool GetTempWorkLocationValid();

    /** Apply the script to the server.
     */
    int ApplyScript();

    /** Indicate if script should be loaded upon hired npc attach after first load.
     */
    bool ShouldLoad();

protected:
private:
    PID                 ownerPID;           ///< The PID of the player that hire a NPC.
    PID                 hiredPID;           ///< The PID of the NPC that is hired.
    bool                guild;              ///< True if this is a guild hire.
    csString            script;             ///< Text version of the hired npc script.
    int                 workLocationID;     ///< The work location ID.

    // Data for pending hires.
    csString            hireTypeName;       ///< The name of the type of hire.
    csString            hireTypeNPCType;    ///< The NPC brain of the type of hire.
    PID                 masterPID;          ///< The master NPC PID.

    // Data for pending confirmation of verified scripts.
    bool                verified;           ///< Set to true when script is verified.
    csString            verifiedScript;     ///< Script verified ok. Ready to be confirmed.
    Location*           tempWorkLocation;   ///< Temp work location.
    bool                tempWorkLocationValid; ///< Indicate if temp work location is valid or not.

    csWeakRef<gemActor> owner;              ///< Cached pointer to player actor when online.
    csWeakRef<gemNPC>   hiredNPC;           ///< Cached pointer to NPC actor when loaded.
    Location*           workLocation;       ///< Cached work postion.
    bool                shouldLoad;         ///< True until script is loaded upon start.
};

#endif
