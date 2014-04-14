/*
 * npcbehave_unittest.cpp by Andrew Dai
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/databuf.h>
#include <csutil/objreg.h>
#include <ctype.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>


#include "npcbehave.h"
#include "npcclient.h"
#include "npc.h"
// This requires googletest to be installed
#include <gtest/gtest.h>



psNPCClient* npcclient;

class BehaviorSetTest : public testing::Test
{
protected:
    BehaviorSetTest(): behaviorset(&eventmgr)
    {
        if(iSCF::SCF == 0)
            scfInitialize(0);
        objreg = new csObjectRegistry();

        npcclient = new psNPCClient;

        npc = new NPC(npcclient, NULL, NULL, NULL, NULL);
        npc->SetAlive(true);

        behavior = new Behavior("test_behavior");

        behavior->SetDecay(20)->SetGrowth(50)->SetCompletionDecay(100)->SetInitial(200);

        behaviorset.Add(behavior);
    }

    ~BehaviorSetTest()
    {
        delete npc;
        delete npcclient;
        delete objreg;
    }

    iObjectRegistry* objreg;
    BehaviorSet behaviorset;
    Behavior* behavior;
    NPC* npc;
    EventManager eventmgr;
};

TEST_F(BehaviorSetTest, AddSameName)
{
    Behavior* behavior2 = new Behavior("test_behavior");
    EXPECT_FALSE(behaviorset.Add(behavior2));

}

TEST_F(BehaviorSetTest, Find)
{
    EXPECT_EQ(behavior, behaviorset.Find("test_behavior"));
    EXPECT_EQ(NULL, behaviorset.Find("test"));
}

TEST_F(BehaviorSetTest, Advance2Behaviors)
{
    Behavior* behavior2 = new Behavior("test_behavior2");
    behavior2->SetDecay(20)->SetGrowth(100)->SetCompletionDecay(100)->SetInitial(10);
    behaviorset.Add(behavior2);
    EXPECT_EQ(behavior, behaviorset.Advance(10, npc));
    EXPECT_EQ(behavior, behaviorset.Advance(10, npc));
    EXPECT_EQ(behavior2, behaviorset.Advance(1000, npc));
}
