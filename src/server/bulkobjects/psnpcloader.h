/*
 * psnpcloader.h - Author: Ian Donderwinkel
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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


#ifndef __PSNPCLOADER_H__
#define __PSNPCLOADER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================



// adminmessage contains classes that are used to read the
// responses, triggers, etc (psDialogManager)
class psCharacter;
class psDialogManager;
struct psTrainerSkill;

class psNPCLoader
{
public:

    // loads npc data from a xml file and stores it in the database
    bool LoadFromFile(csString &filename);

    // reads npc data from the database and stores it in a xml file
    bool SaveToFile(int id, csString &filename);

    // removes a npc from the database
    bool RemoveFromDatabase(int npcid);

    // loads npc dialogs from a xml file and stores them in the database
    bool LoadDialogsFromFile(csString &filename);

    // reads npc dialogs from the database and stores them in a xml file
    bool SaveDialogsToFile(csString &area, csString &filename, int questid, bool quest);

private:

    csRef<iDocumentNode> npcRoot;

    int npcID;
    csString area;
    int questID;

    psCharacter*                 npc;
    psDialogManager*             dialogManager;
    csArray<csString>           knowledgeAreas;
    csArray<int>                knowledgeAreasPriority;
    csArray<psTrainerSkill>     trainerSkills;
    csString                    factionStandings;
    csArray<int>                buys;
    csArray<int>                sells;

    // will be used to keep track of which triggers are already exported
    csArray<int>                triggers;

    // functions to read various sections of the npc xml data
    bool ReadBasicInfo();
    bool ReadDescription();
    bool ReadLocation();
    void ReadSkills();
    void ReadFactions();
    void ReadKnowledgeAreas();
    void ReadSpecificKnowledge(int questID = -1);
    void ReadSpecialResponses(int questID = -1);
    void ReadTrainerInfo();
    void ReadMerchantInfo();
    void ReadMoney();
    void ReadStats();
    void ReadTraits();

    void SetupEquipment();

    // writes all data to the database
    bool WriteToDatabase();


    // functions to read npc data from the database.
    // this will be written to a file in the SaveToFile method.
    void WriteBasicInfo();
    void WriteDescription();
    void WriteKnowledgeAreas();
    bool WriteResponse(csRef<iDocumentNode> attitudeNode, int id, int questID);
    bool WriteTrigger(csRef<iDocumentNode> specificsNode, csString &trigger, int priorID, int questID = -1);
    bool WriteSpecificKnowledge(int questid = -1);
    void WriteFactions();
    void WriteSkills();
    void WriteMerchantInfo();
    void WriteTrainerInfo();
    void WriteStats();
    void WriteMoney();
    void WriteLocation();
    void WriteEquipment();
};


#endif

