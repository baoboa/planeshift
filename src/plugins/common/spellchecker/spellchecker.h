/*
* spellchecker.h, Author: Fabian Stock (AiwendilH@googlemail.com)
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

#ifndef _SPELLCHECKER_H_
#define _SPELLCHECKER_H_

// crystalSpace includes
#include <iutil/comp.h>

// Hunspell includes
#include <hunspell.hxx>

//my own includes
#include <ispellchecker.h>

struct iObjectRegistry;

/**
 * CS Plugin for a hunspell spellchecker
 */
class SpellChecker : public scfImplementation2<SpellChecker, iSpellChecker, iComponent>
{
    public:
	SpellChecker(iBase* parent);
	virtual ~SpellChecker();
	
	//from iComponent
	virtual bool Initialize(iObjectRegistry* objReg);
	
	//from iSpellChecker
	virtual void addWord(csString newWord);
	virtual const csArray<csString>& getPersonalDict() {return personalDict;};
	virtual void clearPersonalDict();
	virtual bool hasDicts() {return hunspellChecker.GetSize() != 0;};
	virtual bool correct(csString wordToCheck);
    protected:
	/** Helper method that removes chars from a string which confuse the spellchecker. Called from "bool correct(csString wordToCheck)"
	*/
	virtual void removeSpecialChars(csString& str);      
    private:
	/** array hunspell spellchecker class (for each found dictionary=
	*/
	csArray<Hunspell*> hunspellChecker;
	/** array containing the words of the personal dictionary
	*/
	csArray<csString> personalDict;	
	
};


#endif // _SPELLCHECKER_H_
