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

// crystalSpace includes
#include <cssysdef.h>
#include <iutil/plugin.h>
#include <iutil/vfs.h>
#include <iutil/stringarray.h>

// my own includes
#include "spellchecker.h"

SCF_IMPLEMENT_FACTORY(SpellChecker)

SpellChecker::SpellChecker(iBase* parent): scfImplementationType(this, parent)
{
    return;
}

SpellChecker::~SpellChecker()
{
    for(size_t i = 0; i < hunspellChecker.GetSize(); i++)
    {
        delete hunspellChecker.Get(i);
    }
    hunspellChecker.DeleteAll();
    return;
}

bool SpellChecker::Initialize(iObjectRegistry* objReg)
{
    // lets register ourselves
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > (objReg);

    csRef<iStringArray> files;
    //search for dic files in the dict folder
    files = vfs->FindFiles("/planeshift/data/dict/");

    if(files.IsValid())
    {
        for(size_t i = 0; i < files->GetSize(); i++)
        {
            csRef<iDataBuffer> AffPath;
            csRef<iDataBuffer> DicPath;
            csString file = files->Get(i);
            csString fileAff;

            // less than 4 chars...can't be a dictionary
            if(file.Length() <= 4)
                continue;

            //check if the file is a dic file
            if(file.Slice(file.Length()-4) != ".dic")
                continue;

            //if so generate also the aff file path and add the dictionary to the list.
            fileAff = file.Slice(0, file.Length()-3) + "aff";
            // Hunspell needs real file pathes...so get them from the VFS
            AffPath = vfs->GetRealPath(fileAff);
            DicPath = vfs->GetRealPath(file);
           // create the spellchecker object
            hunspellChecker.Push(new Hunspell(AffPath->GetData(), DicPath->GetData()));
        }
    }
    return true;
}

void SpellChecker::addWord(csString newWord)
{
    // do nothing if no dictionary was loaded otherwise add the word to the first dictionary in memory
    if (hunspellChecker.GetSize() != 0)
    {
	// remove special chars from the words...not really nice to do this but otherwise a user could add words that by no way could be recognized
	// as those chars will never be presented to the spellchecker
	removeSpecialChars(newWord);
	// now add it to the dictionary...happens only in memory and is not persistant
	if (!(hunspellChecker[0]->add(newWord.GetData())))
	{
	    // also add it to the list of personal dictionary words...needed to save and change those words
	    personalDict.Push(newWord);
	}
    }
}

void SpellChecker::clearPersonalDict()
{
    // do nothing if no dictionary was loaded (means that we also never added any custom words to a dictionary)
    if (hunspellChecker.GetSize() != 0)
    {
	// first we have to remove the words from the dictionary again.
	for (size_t i = 0; i < personalDict.GetSize(); i++)
	{
	  hunspellChecker[0]->remove(personalDict[i].GetData());
	}
	// now we clear the array as well
	personalDict.DeleteAll();
    }
}

bool SpellChecker::correct(csString wordToCheck)
{
    // first we remove some chars that might confuse the spellchecker
    removeSpecialChars(wordToCheck);
    // now we check the word with all available dictionaries
    for(size_t i = 0; i < hunspellChecker.GetSize(); i++)
    {
        if(hunspellChecker.Get(i)->spell(wordToCheck.GetDataSafe()))
        {
            // as soon as the word is in one of the dicts it's considered correct
            return true;
        }
    }
    // not in any dict? Must be a type
    return false;
}

void SpellChecker::removeSpecialChars(csString& str)
{
    // remove all chars that interfere with spellchecking of a word
    // ("'" can't be removed so that "don't" is recognized correctly....
    // this will lead to things like "/me says 'Hello'" being displayed as error
    str.ReplaceAll("?", "");
    str.ReplaceAll("!", "");
    str.ReplaceAll("\"", "");
    str.ReplaceAll("*", "");
    str.ReplaceAll(".", "");
    str.ReplaceAll(",", "");
    str.ReplaceAll(";", "");
    str.ReplaceAll(":", "");
    str.ReplaceAll("-", "");
    str.ReplaceAll("(", "");
    str.ReplaceAll(")", "");
    str.ReplaceAll("{", "");
    str.ReplaceAll("}", "");
    str.ReplaceAll("[", "");
    str.ReplaceAll("]", "");
    str.ReplaceAll("/", "");
}
