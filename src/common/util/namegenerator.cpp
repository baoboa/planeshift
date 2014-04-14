/*
* namegenerator.h by Andrew Mann <amann tccgi.com>
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
*/


#include <psconfig.h>

#include <csutil/randomgen.h>
#include <csutil/csstring.h>
#include <csutil/xmltiny.h>
#include <iutil/vfs.h>
#include <csutil/objreg.h>
#include <csutil/databuf.h>
#include "log.h"

#include "namegenerator.h"

#define PHONICS_LIST "/this/data/phonics.xml"


PhonicEntry::PhonicEntry() 
{
    phonic[0]=0x00; 
    begin_probability=end_probability=middle_probability=0.0f;
    flags=0x00; 
}

PhonicEntry::PhonicEntry(char *ph,float b_p,float e_p,float m_p,unsigned int fl)
{
    strncpy(phonic,ph,6);
    phonic[5]=0x00;
    begin_probability=b_p;
    end_probability=e_p;
    middle_probability=m_p;
    flags=fl;
}

PhonicEntry::~PhonicEntry()
{
}



NameGenerator::NameGenerator( iDocumentNode* node )
{
    begin_total = 0; 
    end_total   = 0;
    prejoin_total = 0;
    nonprejoin_total = 0;

    csRef<iDocumentNodeIterator> iter = node->GetNodes();
    
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> phonic = iter->Next();
        
        PhonicEntry* entry = new PhonicEntry;
        strncpy( entry->phonic, phonic->GetAttributeValue("spelling"), 6 );
        entry->phonic[5] = 0x00;
        
        entry->begin_probability = phonic->GetAttributeValueAsFloat("beginner");        
        entry->end_probability   = phonic->GetAttributeValueAsFloat("ender");        
        entry->middle_probability= phonic->GetAttributeValueAsFloat("middle");
        
        csString temp( phonic->GetAttributeValue("prejoiner") );
        if ( temp.GetAt(0)=='Y')
            entry->flags|=PHONIC_PREJOINER;

        temp = phonic->GetAttributeValue("postjoiner");                       
        if (temp.GetAt(0)=='Y')
            entry->flags|=PHONIC_POSTJOINER;
            
        // Update the totals
        begin_total+=entry->begin_probability;
        end_total+=entry->end_probability;
        if (entry->flags & PHONIC_PREJOINER)
            prejoin_total+=entry->middle_probability;
        else
            nonprejoin_total+=entry->middle_probability;
                        
        entries.Push( entry );            
    }
    
       
    randomgen = new csRandomGen;
}


NameGenerator::~NameGenerator()
{
    delete randomgen;

}

void NameGenerator::GenerateName(csString &namebuffer,int length_low,int length_high)
{
    unsigned int length;
    PhonicEntry *lastphonic=NULL;
    namebuffer.Clear();

    length=randomgen->Get((length_high-length_low))+length_low;

    // Pick a beginning
    lastphonic=GetRandomBeginner();
    namebuffer.Append(lastphonic->phonic);

    // Add phonics to the middle while within length
    while (namebuffer.Length() < length)
    {
        if (lastphonic->flags & PHONIC_POSTJOINER)
            lastphonic=GetRandomNonPreJoiner();
        else
            lastphonic=GetRandomPreJoiner();
        namebuffer.Append(lastphonic->phonic);
    }

    // Pick an ending
    lastphonic=GetRandomEnder(!(lastphonic->flags & PHONIC_POSTJOINER));
    namebuffer.Append(lastphonic->phonic);

    namebuffer.Downcase();
}

PhonicEntry *NameGenerator::GetRandomBeginner()
{
    size_t i;
    float randval,currentsum;
    randval=randomgen->Get() * begin_total;
    size_t entry_count = entries.GetSize();

    currentsum=0.0f;
    for (i=0;i<entry_count;i++)
    {
        if (entries[i]->begin_probability)
        {
            currentsum+=entries[i]->begin_probability;
            if (randval < currentsum)
                return (entries[i]);
        }
    }
    // We'll return the last one here, though this shouldn't happen, rounding error?
    return (entries[entry_count-1]);
}

PhonicEntry *NameGenerator::GetRandomEnder(bool prejoiner)
{
    size_t i;
    float randval,currentsum;
    size_t entry_count = entries.GetSize();    

    csTicks timeLimit=5000;
    csTicks tickBegin=csGetTicks();

    while ((csGetTicks() - tickBegin) < timeLimit)
    {
        randval=randomgen->Get() * end_total;
        currentsum=0.0f;
        for (i=0;i<entry_count;i++)
        {
            if (entries[i]->end_probability)
            {
                currentsum+=entries[i]->end_probability;
                if (randval < currentsum)
                {
                    if (((entries[i]->flags & PHONIC_PREJOINER) && prejoiner) ||
                        (!(entries[i]->flags & PHONIC_PREJOINER) && !prejoiner))
                    {
                        // Found the requested type
                        return (entries[i]);
                    }
                    else
                    {
                        // Didn't find the requested type, reroll
                        break;
                    }
                }
            }
        }
    }

    // We'll return the last one here, though this can't happen
    return (entries[entry_count-1]);
}

PhonicEntry *NameGenerator::GetRandomPreJoiner()
{
    size_t i;
    float randval,currentsum;
    randval=randomgen->Get() * prejoin_total;
	size_t entry_count = entries.GetSize();

    currentsum=0.0f;
    for (i=0;i<entry_count;i++)
    {
        if ((entries[i]->flags & PHONIC_PREJOINER) && entries[i]->middle_probability)
        {
            currentsum+=entries[i]->middle_probability;
            if (randval < currentsum)
                return (entries[i]);
        }
    }
    // We'll return the last one here, though this shouldn't happen, rounding error?
    return (entries[entry_count-1]);
}

PhonicEntry *NameGenerator::GetRandomNonPreJoiner()
{
    size_t i;
    float randval,currentsum;
    randval=randomgen->Get() * nonprejoin_total;
    size_t entry_count = entries.GetSize();

    currentsum=0.0f;
    for (i=0;i<entry_count;i++)
    {
        if (!(entries[i]->flags & PHONIC_PREJOINER) && entries[i]->middle_probability)
        {
            currentsum+=entries[i]->middle_probability;
            if (randval < currentsum)
                return (entries[i]);
        }
    }
    // We'll return the last one here, though this shouldn't happen, rounding error?
    return (entries[entry_count-1]);
}



NameGenerationSystem::NameGenerationSystem()
{
    int i;
    for (i=0;i<NAMEGENERATOR_MAX;i++)
        generators[i]=NULL;
}

NameGenerationSystem::~NameGenerationSystem()
{
    int i;

    for (i=0;i<NAMEGENERATOR_MAX;i++)
    {
        delete generators[i];
        generators[i]=NULL;
    }
}

bool NameGenerationSystem::LoadDatabase( iObjectRegistry* objectReg )
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( objectReg);
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);    
    
    csRef<iDataBuffer> buff = vfs->ReadFile( PHONICS_LIST );

    if ( !buff || !buff->GetSize() )
    {
        Error2( "Could not load XML: %s", PHONICS_LIST );
        return false;
    }
    
    csRef<iDocument> doc=xml->CreateDocument();
    
    const char* error =doc->Parse(buff);
    if(error)
    {
        Error3( "Error parsing XML file %s: %s", PHONICS_LIST, error );
        return false;
    }
    csRef<iDocumentNode> root, familyName, femaleName, maleName;

    if((root=doc->GetRoot())&&(familyName = root->GetNode("FAMILY_NAME")) && (femaleName = root->GetNode("FEMALE_FIRST_NAME"))
        && (maleName = root->GetNode("MALE_FIRST_NAME")))
    {    
        generators[0] = new NameGenerator( femaleName );  
        generators[1] = new NameGenerator( maleName );  
        generators[2] = new NameGenerator( familyName );  

        return true;
    }
    else
    {         
        Error2("Error parsing XML file: %s", PHONICS_LIST);
        return false;
    }
}

void NameGenerationSystem::GenerateName(int generator_type,csString &namebuffer,int length_low,int length_high)
{
    if (generator_type>=0 && generator_type<NAMEGENERATOR_MAX && generators[generator_type]!=NULL)
        generators[generator_type]->GenerateName(namebuffer,length_low,length_high);
}



