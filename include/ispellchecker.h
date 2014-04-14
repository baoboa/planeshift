/*
* ispellchecker.h, Author: Fabian Stock (AiwendilH@googlemail.com)
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

#ifndef _ISPELLCHECKER_H_
#define _ISPELLCHECKER_H_

// crystalSpace includes
#include <csutil/scf.h>
#include <csutil/scf_implementation.h>

struct iSpellChecker : public virtual iBase
{
    SCF_INTERFACE(iSpellChecker, 1, 0, 0);

    /** adds a word to the personal dictionary
     * @param newWord. The word that should be added. This will be added temporarly to the first available dictionary and to the personal custom words list
     */
    virtual void addWord(csString newWord) = 0;
    /** get an array with words of the personal dictionary
     * @return An array of all words from the personal dictionary
     */
    virtual const csArray<csString>& getPersonalDict() = 0;
    /** clears the whole personal dictionary
     */
    virtual void clearPersonalDict() = 0;
    /** are any dictionaries loaded?
     * @return true if at least one dictionary was loaded otherwise false
     */
    virtual bool hasDicts() = 0;
    /** is the given word in one of the dictionaries?
     * @param wordToCheck The word we want to check for a typo
     * @return True if the words was found in at least one of the dictionaries.
     */
    virtual bool correct(csString wordToCheck) = 0;
};

#endif // _ISPELLCHECKER_H_