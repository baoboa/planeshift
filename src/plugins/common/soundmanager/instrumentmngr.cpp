/*
 * instrumentmngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "instrumentmngr.h"

//====================================================================================
// Project Includes
//====================================================================================
#include "util/psxmlparser.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "manager.h"
#include "instrument.h"
#include "songhandle.h"

#define DEFAULT_INSTRUMENTS_PATH "/planeshift/art/instruments.xml"


InstrumentManager::InstrumentManager()
{
    // empty
}

InstrumentManager::~InstrumentManager()
{
    Instrument* instr;
    csHash<Instrument*, csString>::GlobalIterator instrIter(instruments.GetIterator());

    while(instrIter.HasNext())
    {
        instr = instrIter.Next();
        delete instr;
    }

    instruments.DeleteAll();
}

bool InstrumentManager::Initialize(iObjectRegistry* objectReg)
{
    const char* instrumentsPath;
    csRef<iDocument> doc;
    csRef<iDocumentNode> root;
    csRef<iDocumentNode> instrRootNode;
    csRef<iDocumentNode> instrumentNode;
    csRef<iDocumentNodeIterator> instrIter;

    // retrieving path to instruments definitions
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        instrumentsPath = configManager->GetStr("Planeshift.Sound.Intruments", DEFAULT_INSTRUMENTS_PATH);
    }
    else
    {
        instrumentsPath = DEFAULT_INSTRUMENTS_PATH;
    }

    // if SoundSystemManager hasn't been initialized instruments cannot
    // load notes from resource sounds
    if(!SoundSystemManager::GetSingleton().Initialised)
    {
        return false;
    }

    doc = ParseFile(objectReg, instrumentsPath);
    if(!doc.IsValid())
    {
        return false;
    }

    root = doc->GetRoot();
    if(!root.IsValid())
    {
        return false;
    }

    instrRootNode = root->GetNode("instruments");
    if(!instrRootNode.IsValid())
    {
        return false;
    }

    // parsing the file
    instrIter = instrRootNode->GetNodes("instrument");
    while(instrIter->HasNext())
    {
        uint polyphony;
        const char* name;
        Instrument* instr;
        csRef<iDocumentNode> note;
        csRef<iDocumentNodeIterator> notesIter;

        instrumentNode = instrIter->Next();
        name = instrumentNode->GetAttributeValue("name");
        polyphony = instrumentNode->GetAttributeValueAsInt("polyphony", 1);
        instr = new Instrument(polyphony);

        // adding instrument
        instruments.Put(name, instr);

        instr->volume = instrumentNode->GetAttributeValueAsFloat("volume", VOLUME_NORM);
        instr->minDist = instrumentNode->GetAttributeValueAsFloat("min_dist");
        instr->maxDist = instrumentNode->GetAttributeValueAsFloat("max_dist");

        notesIter = instrumentNode->GetNodes("note");
        while(notesIter->HasNext())
        {
            int alter;
            uint octave;
            const char* step;
            const char* resource;

            note = notesIter->Next();
            resource = note->GetAttributeValue("resource");
            step = note->GetAttributeValue("step");
            alter = note->GetAttributeValueAsInt("alter");
            octave = note->GetAttributeValueAsInt("octave");
            instr->AddNote(resource, *step, alter, octave);
        }
    }

    return true;
}

bool InstrumentManager::PlaySong(SoundControl* sndCtrl, csVector3 pos, csVector3 dir, SoundHandle* &songHandle,
                                 csRef<iDocument> musicalSheet, const char* instrName)
{
    Instrument* instr;

    if(instrName == 0)
    {
        return false;
    }

    // checking if the instrument is defined
    instr = instruments.Get(instrName, 0);
    if(instr == 0)
    {
        return false;
    }
    if(!instr->IsDefined())
    {
        return false;
    }

    songHandle = new SongHandle(musicalSheet, instr);

    return SoundSystemManager::GetSingleton().Play3DSound("song", DONT_LOOP, 0, 0, instr->volume,
                     sndCtrl, pos, dir, instr->minDist, instr->maxDist, 0, CS_SND3D_ABSOLUTE, songHandle, false);
}
