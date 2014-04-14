/*
 * strutil.cpp by Matze Braun <MatzeBraun@gmx.de>
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
#include <psconfig.h>
#include <ctype.h>

#include <iutil/object.h>

#include "util/strutil.h"
#include "util/psconst.h"
//=============================================================================
// Local Includes
//=============================================================================
#include "psstring.h"


csString& GetWordNumber(const csString& str, int number, size_t *startpos)
{
    static csString erg;
    CS_ASSERT(number>0);

    size_t pos = 0;

    // If we have a startpos pointer initiate position with that start pos
    if (startpos != NULL)
        pos = *startpos;

    if (pos>=str.Length())
    {
        erg.Clear();
        return erg;
    }

    //Skip leading spaces
    while (pos < str.Length() && isspace(str[pos]))
        pos++;

    while (number>1 && pos < str.Length())
    {
        if (isspace(str[pos++]))
        {
            // Skip continuous spaces
            while (pos<str.Length() && isspace(str[pos]))
            {
                pos++;
            }                
            number--;
        }
    }
    size_t pos2=pos;
    
    while (pos2 < str.Length() && !isspace(str[pos2]))
    {
        pos2++;
    }        
    
    // We have found a word, though it might be at the end of the string. 
    erg.Replace(((const char*)str)+pos,pos2-pos);

    // Anyway if we have startpos update it.
    if (startpos != NULL) *startpos = pos;

    return erg;
}


WordArray::WordArray(const csString & cmd, bool ignoreQuotes) : csStringArray()
{
    size_t pos;

    pos = 0;
    while (pos < cmd.Length())
    {
        SkipSpaces(cmd, pos);
        if (pos < cmd.Length())
        {
            if(ignoreQuotes)
            {
                pos = AddWord(cmd, pos);
            }
            else
            {
                pos = AddQuotedWord(cmd, pos);
            }
        }
    }
}

void WordArray::SkipSpaces(const csString & cmd, size_t & pos)
{
    while (pos < cmd.Length()  &&  isspace(cmd.GetAt(pos)))
        pos++;
}

size_t WordArray::AddWord(const csString & cmd, size_t pos)
{
    size_t end = cmd.FindFirst(" \f\n\r\t\v", pos);
    if(end == (size_t) -1)
        end = cmd.Length();
    Push(cmd.Slice(pos, end-pos));

    return end;
}

size_t WordArray::AddQuotedWord(const csString & cmd, size_t pos)
{
    // Check if this begins with a quotation mark
    char c = cmd.GetAt(pos);
    if (c == '\'' || c== '\"')
    {
        size_t end = cmd.FindFirst(c, pos + 1);

        if(end == (size_t) -1)
            // No matching closing quotation mark so we revert to normal behaviour
            return AddWord(cmd, pos);

        // Skip out opening quotation mark
        pos++;
        Push(cmd.Slice(pos, end-pos));

        // Skip out the closing quotation mark
        return ++end;
    }
    else
        return AddWord(cmd, pos);
}

csString WordArray::GetTail(size_t wordNum) const
{
    csString str;

    for (size_t currWordNum = wordNum; currWordNum < GetSize(); currWordNum++)
    {
        if (currWordNum > (size_t)wordNum)
            str += " ";            
        str += Get(currWordNum);
    }
    return str;
}

void WordArray::GetTail(size_t wordNum, csString& buff) const
{
    buff.Clear();

    for (size_t currWordNum = wordNum; currWordNum < GetSize(); currWordNum++)
    {
        if (currWordNum > (size_t)wordNum)
            buff += " ";
            
        buff += Get(currWordNum);
    }    
}


csString WordArray::GetWords( size_t startWord, size_t endWord) const
{
    csString str;

    if ( endWord > GetSize() )
        endWord = GetSize();

    if ( startWord == endWord )
        return Get(startWord);
        
    for (size_t currWordNum = startWord; currWordNum < endWord; currWordNum++)
    {
        if (currWordNum > startWord)
            str += " ";
        str += Get(currWordNum);
    }
    return str;
}

size_t WordArray::FindStr(const csString& str,int start) const
{
    for (size_t i=start; i<GetSize(); i++)
    {
        if (Get(i) == str)
            return i;
    }
    return SIZET_NOT_FOUND;
}




bool psContain(const csString& str, const csArray<csString>& strs)
{
    for (size_t i = 0; i < strs.GetSize(); i++)
    {
        if (strstr(str.GetDataSafe(),strs[i].GetDataSafe()))
          return true;
    }
    return false;
}

bool psSentenceContain(const csString& sentence,const csString& word)
{
    WordArray words(sentence);
    int n = 1;
    csString currentWord;

    do {
        currentWord = words[n-1];
        if (currentWord.IsEmpty())
        {
            return false;
        }
        n++;
    } while (currentWord != word);
    return true;
}


const char* PS_GetFileName(const char* path)
{
    size_t pos = strlen(path);
    for ( ; pos>0; pos--)
    {
        if (path[pos] == '/')
        break;
    }
    
    return path+pos+1;
}

csArray<csString> psSplit(csString& str, char delimer)
{
    csArray<csString> split;

    char *curr = (char*)str.GetDataSafe();
    char *next = curr;
    while((next = strchr(curr,delimer)))
    {
        *next = '\0';
        split.Push(csString(curr));
        *next = delimer;
        curr = ++next;
    }
    split.Push(csString(curr));
    return split;
}

csArray<csString> psSplit(const char* str, char delimer)
{
    csString tempStr(str);
    return psSplit(tempStr, delimer);
}

bool isFlagSet(const psString & flagstr, const char * flag)
{
    return flagstr.FindSubString(flag,0,XML_CASE_INSENSITIVE)!=-1;
}

csString toString(const csVector2& pos)
{
    csString result;
    result.Format("(%.2f,%.2f)",pos.x,pos.y);
    return result;
}

csString toString(const csVector3& pos)
{
    csString result;
    result.Format("(%.2f,%.2f,%.2f)",pos.x,pos.y,pos.z);
    return result;
}

csString toString(const csVector4& pos)
{
    csString result;
    result.Format("(%.2f,%.2f,%.2f,%.2f)",pos.x,pos.y,pos.z,pos.w);
    return result;
}

csString toString(const csVector3& pos, iSector* sector)
{
    csString result;
    result.Format("(%.2f,%.2f,%.2f) in %s",pos.x,pos.y,pos.z,(sector?sector->QueryObject()->GetName():"(null)"));
    return result;
}

csString toString(const csMatrix3& mat)
{
    return "(" + toString(mat.Row1()) + "," + toString(mat.Row2()) + "," +toString(mat.Row3()) + ")";
}


csString toString(const csTransform& trans)
{
    return toString(trans.GetO2T()) + " " + toString(trans.GetO2TTranslation());
}

csArray<csString> splitTextInLines(csString inText, size_t maxLineLength, int& maxRowLen) {

    const size_t textLen = inText.Length();
    const char * text = inText.GetData();

    // create a row character by character while calculating word wrap
    char line[256];
    int linePos = 0;
    int lastSpace = 0;
    size_t a = 0;
    csArray<csString> lines;
    for (a=0; a<textLen; ++a)
    {
        if (text[a] == ' ' && linePos == 0)
            continue;
      
        if (text[a] == ' ')
            lastSpace = linePos;

        line[linePos] = text[a];
        ++linePos;

        if (linePos < (int)maxLineLength)
            continue;
      
        if (lastSpace == 0)
        {
            line[linePos-1] = '-';
            --a;
        }
        else if (text[a] != ' ')
        {
            a -= linePos - lastSpace;
            linePos = lastSpace;
        }

        if (linePos > maxRowLen)
            maxRowLen = linePos;

        lastSpace = 0;
        line[linePos] = 0;
        linePos = 0;
        lines.Push(line);
    }    
    if (linePos > 0)
    {
        if (linePos > maxRowLen)
            maxRowLen = linePos;

        line[linePos] = 0;
        lines.Push(line);
    }

    return lines;
}