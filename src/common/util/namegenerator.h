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

#ifndef __NAMEGENERATOR_H__
#define __NAMEGENERATOR_H__


#include <csutil/parray.h>
#include <csutil/randomgen.h>

struct iObjectRegistry;

/**
 * \addtogroup common_util
 * @{ */


/* Name generation functions.
 *  These classes allow for generation of random names.
 *
 *  The current implementation uses a table of phonetic spellings for the english language.
 *
 *  Usage:
 *
 *  Create a new NameGenerationSystem() after the database connection has been established.
 *  Call NameGenerationSystem::GenerateName(TYPE,csString *namebuffer,max_low,max_high).
 *   On return, the passed csString will be filled with the name as requested.
 *
 *  The type should be one of the enum below. Currently:
 *  NAMEGENERATOR_FEMALE_FIRST_NAME
 *  NAMEGENERATOR_MALE_FIRST_NAME
 *  NAMEGENERATOR_FAMILY_NAME
 *
 *  max_low and max_high are not hard maximums. Rather, a random number is picked between low and high.  As name generation progresses, if 
 *   the length of the name exceeds this number, and ending sequence is added.  The resulting string will be 1-9 characters longer than the
 *   number picked.
 *
 *  Some rough guidelines and notes:
 *
 *  Male names seem to come out much better when shorter.  Try max_low = 3 and max_high = 5 for male first names.
 *  Female names seem to work well with max_low=7 max_high=7 .
 *  Surnames (family names) work ok with 8,10 .
 *
 *  
 *
 */


/*  This implementation works by classifying phonetic bits according to 5 properties:
 *  Likelyness to begin a name
 *  Likelyness to end a name
 *  Likelyness to appear in the middle of a name
 *  Begins in a vowel
 *  Ends in a vowel
 *
 *  The last two properties are not strictly tied to vowels, but generally follow that pattern.  They help the name generator
 *  decide which sequences can appear next to each other without resulting in unpronouncable names.
 *
 *  The first three values are determined by running a list of a particular type of name (male first names for example) through a
 *   filter that searches for occurances of each phonetic bit and adjusts the weight accordingly.
 *  The current filter is written in perl.
 *
 */



enum {
    NAMEGENERATOR_FEMALE_FIRST_NAME=0,
    NAMEGENERATOR_MALE_FIRST_NAME,
    NAMEGENERATOR_FAMILY_NAME,
    NAMEGENERATOR_MAX
};


#define PHONIC_PREJOINER    0x01
#define PHONIC_POSTJOINER   0x02


class PhonicEntry
{
public:
    PhonicEntry();
    PhonicEntry(char *ph,float b_p,float e_p,float m_p,unsigned int fl);
    ~PhonicEntry();

public:
    char phonic[6];
    float begin_probability,end_probability,middle_probability;
    unsigned int flags;
};


class NameGenerator
{
public:
    NameGenerator(iDocumentNode* node);
    ~NameGenerator();

    void GenerateName(csString &namebuffer,int length_low,int length_high);

private:
    float begin_total,end_total,prejoin_total,nonprejoin_total;
    int entry_count;
    csPDelArray<PhonicEntry> entries;
    csRandomGen *randomgen;

    PhonicEntry *GetRandomBeginner();
    PhonicEntry *GetRandomEnder(bool prejoiner);
    PhonicEntry *GetRandomPreJoiner();
    PhonicEntry *GetRandomNonPreJoiner();

};

class NameGenerationSystem
{
public:
    NameGenerationSystem();
    ~NameGenerationSystem();

    /**
    * Load all name rules into memory for future use.
    */
    bool LoadDatabase( iObjectRegistry* objectReg );
    /** Generate a name of the requested type, fill it into namebuffer.
    * 
    */
    void GenerateName(int generator_type,csString &namebuffer,int length_low,int length_high);

private:
    NameGenerator *generators[NAMEGENERATOR_MAX];
};

/** @} */

#endif

