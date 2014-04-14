/*
 * strutil.h by Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __STRUTIL_H__
#define __STRUTIL_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
#include <csutil/stringarray.h>
#include <csutil/csstring.h>
#include <csgeom/vector3.h>
#include <iengine/sector.h>
#include <csgeom/matrix3.h>
#include <csgeom/transfrm.h>
#include <csutil/stringconv.h>

class psString;

/**
 * \addtogroup common_util
 * @{ */

/**
 * Split a csString into an array of sctrings.
 */
void Split(const csString& str, csArray<csString> & arr);

/**
 * Return the given word number
 *
 * Return word at the position number in the input string str. If a pointer
 * to startpos is given the position of the found word would be return.
 *
 * @param str      The string that are to be search for the word
 * @param number   The position of the the word to be found
 * @param startpos A pointer that if non zero will get the position in the string where word where found.
 * @return Return the word at position number in the string.
 */
csString& GetWordNumber(const csString& str, int number, size_t * startpos = NULL);


/**
 * WordArray is class that parses text command (e.g. "/newguild Insomniac Developers") to words.
 */
class WordArray : protected csStringArray
{
public:
    WordArray(const csString& cmd, bool ignoreQuotes = true);

    size_t GetCount() const
    {
        return GetSize();
    }

    /**
     * Returns given word, or empty string if it does not exist.
     */
    csString Get(size_t wordNum) const
    {
        if(wordNum < GetSize())
            return csStringArray::Get(wordNum);
        else
            return "";
    }
    csString operator[](size_t wordNum) const
    {
        return Get(wordNum);
    }

    /**
     * Delete the n'th element from the word array.
     */
    bool DeleteIndex(size_t n)
    {
        return csStringArray::DeleteIndex(n);
    }

    bool GetString(size_t wordNum, csString &retValue) const
    {
        if(wordNum >= GetSize())
        {
            return false;
        }

        retValue = csStringArray::Get(wordNum);
        return true;
    }

    /**
     * Check if the word is an interger.
     */
    bool IsInt(size_t wordNum) const
    {
        const char* toConvert = Get(wordNum).GetDataSafe();
        char* endPtr = const_cast<char*>(toConvert);

        // TOFIX: gives the following error on windows:
        //        error C2106: '=' : left operand must be l-value
        //errno = 0; /* To distinguish success/failure after call */
        strtol(toConvert, &endPtr, 10);  // 10 base

        // Check for error situations
        if (errno != 0) return false;
        if (toConvert == endPtr) return false;
        if (*endPtr != '\0') return false;  // Further characters after number
        
        return true;
    }

    int GetInt(size_t wordNum) const 
    {
        return atoi(Get(wordNum).GetData());
    }

    bool GetInt(size_t wordNum, int &retValue ) const
    {
        char *end;

        const char *data = Get(wordNum).GetData();
   
        // The string has to contain some chars, otherwise the end pointer will not
        // be able to tell if all data where an integer.
        if (!data || *data == '\0')
        {
            return false;
        }

        int value = (int)strtol(data, &end, 10);

        // If end point to a \0 then everything where read.
        if (*end == '\0')
        {
            retValue = value;
            return true;
        }

        return false;
    }

    /**
     * Check if the word is a float.
     */
    bool IsFloat(size_t wordNum) const
    {
        const char* toConvert = Get(wordNum).GetDataSafe();
        const char* endPtr = const_cast<char*>(toConvert);

        // TOFIX: gives the following error on windows:
        //        error C2106: '=' : left operand must be l-value
        // errno = 0; /* To distinguish success/failure after call */
        CS::Utility::strtof(toConvert, &endPtr);

        // Check for error situations
        if (errno != 0) return false;
        if (toConvert == endPtr) return false;
        if (*endPtr != '\0') return false;  // Further characters after number
        
        return true;
    }

    float GetFloat(size_t wordNum) const
    {
        return atof(Get(wordNum).GetData());
    }

    bool GetFloat(size_t wordNum, float &retValue ) const
    {
        const char *end;

        const char *data = Get(wordNum).GetData();
   
        // The string has to contain some chars, otherwise the end pointer will not
        // be able to tell if all data where an integer.
        if (!data || *data == '\0')
        {
            return false;
        }

        float value = (float)CS::Utility::strtof(data, &end);

        // If end point to a \0 then everything where read.
        if (*end == '\0')
        {
            retValue = value;
            return true;
        }

        return false;
    }

    /* Returns all words, starting at given word */
    csString GetTail(size_t wordNum) const;
    void GetTail(size_t wordNum, csString& dest) const;

    /* Gets all the words from startWord to endWord */
    csString GetWords( size_t startWord, size_t endWord) const;

    size_t FindStr(const csString& str,int start=0) const;

protected:
    size_t AddWord(const csString& cmd, size_t pos);
    size_t AddQuotedWord(const csString& cmd, size_t pos);
    void SkipSpaces(const csString& cmd, size_t & pos);
};


bool psContain(const csString& str, const csArray<csString>& strs);
bool psSentenceContain(const csString& sentence,const csString& word);
const char* PS_GetFileName(const char* path);
csArray<csString> psSplit(csString& str, char delimer);
csArray<csString> psSplit(const char* str, char delimer);
bool isFlagSet(const psString & flagstr, const char * flag);
csString toString(const csVector2& pos);
csString toString(const csVector3& pos);
csString toString(const csVector4& pos);
csString toString(const csVector3& pos, iSector* sector);
csString toString(const csMatrix3& mat);
csString toString(const csTransform& trans);

/**
 * Split text into multiple lines. Used by chat bubbles
 * @param inText text to split
 * @param maxLineLength max length you want the rows to be created
 * @param maxRowLen the resulting max length after the split
 */ 
csArray<csString> splitTextInLines(csString inText, size_t maxLineLength, int& maxRowLen);

/** @} */

#endif

