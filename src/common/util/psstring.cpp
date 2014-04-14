/*
 * psstring.cpp
 *
 * Copyright (C) 2001-2004 Atomic Blue (http://www.atomicblue.org)
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
 *
 * A slightly improved string over csString.  Adds a couple of functions
 * that will be needed by the XML parser.
 */
#include <psconfig.h>

#include <string.h>
#include <ctype.h>

#include "util/psstring.h"
#include "util/strutil.h"
#include "util/psconst.h"

bool psString::FindString(const char *border, unsigned int & pos, unsigned int & end) const
{
    pos = FindSubString(border, pos);
    if (pos == (unsigned int)-1)
        return false;

    end = pos + (int)strlen(border);
    end = FindSubString(border, end);
    if (end == (unsigned int)-1)
        return false;

    return true;
}

bool psString::FindNumber(unsigned int & pos, unsigned int & end) const
{
    const char *myData = this->GetData();

    while (pos < Length()  &&  !(isdigit(myData[pos]) || myData[pos] == '-' || myData[pos] == '.'))
        pos++;

    if (pos < Length())
    {
        end = pos;
        while (end< Length()  &&  (isdigit(myData[end]) || myData[end] == '-' || myData[end] == '.'))
            end++;

        end--;
        return true;
    }
    else
        return false;
}

int psString::FindSubString(const char *sub, size_t start, bool caseInsense, bool wholeWord) const
{
    const char *myData = this->GetData();

    size_t lensub = strlen(sub);
    if ( IsEmpty() || !lensub || lensub>Length() )
        return -1;

    if ( caseInsense )
    {
        while ( start <= Length() - lensub )
        {
            if (strncasecmp(sub, myData + start, lensub) != 0)
                start++;
            else
            {
                if (wholeWord)
                {
                    if ((start == 0 || !isalnum(myData[start])) &&
                        (!isalnum(myData[start+lensub])))
                        return (int)start;
                    else
                        start++;
                }
                else
                    return (int)start;
            }
        }
        return -1;
    }
    else
    {
        while (true)
        {
            const char* pWhere = strstr(myData + start, sub);
            if (pWhere)
            {
                if (!wholeWord)
                    return pWhere - myData;
                else
                {
                    start = pWhere - myData;

                    if ((start == 0 || !isalnum(myData[start])) &&
                        (!isalnum(myData[start+lensub])))
                        return (int)start;
                    else
                        start++;
                }
            }
            else
                return -1;
        }
    }
}

int psString::FindSubStringReverse(psString& sub, size_t start, bool caseInsense)
{
    const char *myData = this->GetData();

    if ( IsEmpty() || sub.IsEmpty() || sub.Length()>Length() )
        return -1;

    if ( caseInsense )
    {
        while ( start >= 0 + sub.Length() )
        {
            const char* pWhere = myData + start - sub.Length();
            size_t x;
            for ( x = 0; x < sub.Length(); x++ )
            {
                if ( toupper(pWhere[x]) != toupper(sub[x]) )
                    break;
            }
            if ( x < sub.Length() )
                start--;
            else
                return pWhere-myData;
        }
        return -1;
    }
    else
    {
        while ( start >= 0 + sub.Length() )
        {
            const char* pWhere = myData + start - sub.Length();
            size_t x;
            x=0;
            for ( x = 0; x < sub.Length(); x++ )
            {
                if ( pWhere[x] != sub[x] )
                    break;
            }
            if ( x < sub.Length() )
                start--;
            else
                return pWhere-myData;
        }
        return -1;
    }
}

void psString::GetSubString(psString& str, size_t from, size_t to) const
{
    str.Clear();

    if ( from > Size || from > to )
        return;

    size_t len = to-from;
    str.Append ( ((const char*) *this) + from, len);
}

void psString::GetWord(size_t pos, psString &buff, bool wantPunct) const
{
    size_t start = pos;
    size_t end   = pos;

    if (pos > Size)
    {
        buff="";
        return;
    }

    const char *myData = this->GetData();

    // go back to the beginning of the word
    while (start > 0 && (!isspace(myData[start])) &&
           (wantPunct || !ispunct(myData[start])) 
          )
    start--;
    if (isspace(myData[start]) || (!wantPunct && ispunct(myData[start])))
        start++;

    // search end of the word
    while (end<Size && (!isspace(myData[end])) &&
           (wantPunct || !ispunct(myData[end])) 
          )
    end++;

    GetSubString(buff,start,end);
}

void psString::GetWordNumber(int which,psString& buff) const
{
    buff = ::GetWordNumber((csString) *this, which);
}
    
void psString::GetLine(size_t start,csString& line) const
{
    size_t end  = this->FindFirst('\n',start);
    size_t end2 = this->FindFirst('\r',start);
    if (end == SIZET_NOT_FOUND)
        end = Length();
    if (end2 == SIZET_NOT_FOUND)
        end2 = Length();
    if (end2 < end)
        end = end2;

    SubString(line,start,end-start);
}

bool psString::ReplaceSubString(const char* what, const char* with)
{
    int at;
    size_t len = strlen(what);
    if ( (at = FindSubString(what,0,false)) > -1 )
    {
        size_t where = (size_t)at;
        size_t pos = where;
        DeleteAt(where, len);
        Insert(pos, with);
        return true;
    }
    else
        return false;
}

size_t psString::FindCommonLength(const psString& other) const
{
    const char *myData = this->GetData();
    const char *otherData = other.GetData();

    if (!myData || !otherData)
        return 0;

    size_t i;
    for (i=0; i<Length(); i++)
    {
        if (myData[i] != otherData[i])
            return i;
    }
    return i;
}

//---------------------------------------------------------------------------

bool psString::IsVowel(size_t pos)
{
    switch ( GetAt(pos) )
    {
        case 'a': case 'A':
        case 'e': case 'E':
        case 'i': case 'I':
        case 'o': case 'O':
        case 'u': case 'U':
            return true;
            
        default:
            return false;
    }
}

psString& psString::Plural()
{
    // Check exceptions first
    if ( Slice(Size-4).Downcase() == "fish" )
    {
        return *this;
    }

    const char *suffix = "s";

    switch ( GetAt(Size-1) ) // Last char
    {
        case 's': case 'S':
        case 'x': case 'X':
        case 'z': case 'Z':
            suffix = "es";
            break;

        case 'h': case 'H':
            switch ( GetAt(Size-2) ) // Second to last char
            {
                case 'c': case 'C':
                case 's': case 'S':
                    suffix = "es";
                    break;
            }
            break;

        case 'o': case 'O':
            if ( !IsVowel(Size-2) ) // Second to last char is consonant
                suffix = "es";
            break;

        case 'y': case 'Y':
            if ( !IsVowel(Size-2) ) // Second to last char is consonant
            {
                Truncate(Size-1);
                suffix = "ies";
            }
            break;

    }

    Append(suffix);

    return *this;
}

void psString::Split(csStringArray& arr, char delim)
{
    Trim();
    if (!Length())
        return;

    size_t pipePos = FindFirst(delim);
    if (pipePos == size_t(-1))
        arr.Push(GetDataSafe());
    else
    {
        psString first, rest;
        SubString(first, 0, pipePos);
        first.Trim();
        if (first.Length())
        {
            arr.Push(first);
            SubString(rest, pipePos+1, Length()-pipePos-1);
            rest.Split(arr,delim);
        }
    }
}
