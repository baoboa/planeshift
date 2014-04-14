/*
 * psxmlparser.cpp
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
#include <csutil/databuf.h>

#include "util/psxmlparser.h"

int psXMLString::FindTag( const char* tagName, int start )
{
    psString temp("<");

    temp+=tagName;

    return FindSubString( temp, start, XML_CASE_INSENSITIVE );
}

size_t psXMLString::FindNextTag( size_t start )
{
    start++;
    const char *myData = GetData();

    while ( start < Size && myData[start] != '<' )
        start++;

    if ( start < Size )
        return start;
    else
        return (size_t)-1;
}

size_t psXMLString::FindMatchingEndTag( int start, const char* tagName )
{
    char termTag[100];
    int end=-1, next;
    int nest = 1;

    strcpy(termTag,"/");
    strcat(termTag,tagName);

    while ( nest )
    {
        end = FindTag( termTag, start );
        if ( end == -1 )
            break;

        next = FindTag(tagName, start+1);
        if ( next == -1 || next > end )
        {
            nest--;
            start = end+1;
        }
        else
        {
            nest++;
            start=next+1;
        }
    }
    return end;
}

size_t psXMLString::GetTagSection(int start, const char* tagName, 
    psXMLString& tagSection)
{
    size_t end = FindMatchingEndTag(start, tagName);
    
    if ( end == (size_t)-1 )
        tagSection = "";
    else
        GetSubString(tagSection, start, end+strlen(tagName)+3);
    
    return tagSection.Length();
}

size_t psXMLString::GetWithinTagSection(int start, const char* tagName, 
                                     psXMLString& tagSection)
{
    size_t end = FindMatchingEndTag( start, tagName );

    if ( end == (size_t)-1 )
        tagSection = psString("");
    else
    {
        psXMLTag startTag;
        GetTag(start, startTag);
        start+=(int)startTag.Length();
        GetSubString(tagSection, start, end);
    }
    return tagSection.Length();
}

size_t psXMLString::GetWithinTagSection(int start, const char* tagName, 
                                     csString& value)
{
    psXMLString tagSection;
    if (GetWithinTagSection(start,tagName,tagSection) > 0)
    {
        value = tagSection;
    }
    return tagSection.Length();
}

size_t psXMLString::GetWithinTagSection(int start, const char* tagName, 
                                     int& value)
{
    psXMLString tagSection;
    if (GetWithinTagSection(start,tagName,tagSection) > 0)
    {
        value = atoi(tagSection.GetData());
    }
    return tagSection.Length();
}
size_t psXMLString::GetWithinTagSection(int start, const char* tagName, 
                                     double& value)
{
    psXMLString tagSection;
    if (GetWithinTagSection(start,tagName,tagSection) > 0)
    {
        value = atof(tagSection.GetData());
    }
    return tagSection.Length();
}

size_t psXMLString::GetTag( int start, psXMLTag& tag )
{
    psString tmp(">");
    int end = FindSubString(tmp, start);

    if ( end == -1 )
        tag.Empty();
    else
        GetSubString(tag, start, end+1);

    return tag.Length();
}

//-------------------------------------------------------------------

void psXMLTag::GetTagParm(const char* parm, csString& value )
{
    psString param(" ");
    param.Append(parm);
    param.Append('=');
 
    int start = FindSubString(param, 0, XML_CASE_INSENSITIVE);

    //Checks to see if the parm is getting mixed up with the tag name.
    if ( start==1 )
    {
        psString tagName;
        GetTagName(tagName);
        start = FindSubString(param,(int)tagName.Length(),XML_CASE_INSENSITIVE); 
    }
        
    psString tempStr;

    value="";

    const char *myData = GetData();

    while ( start != -1 )
    {
        start += (int)param.Length();
        
        if (start >= (int)Length())
            return;
        
        // skip whitespace after parm name
        while (myData[start]==' ')
        {
            start++;
            if (start >= (int)Length())
                return;
        }
        
        size_t end = start+1;  // parm is at least one char
        
        // Determine delimiter, if any
        char chr;
        
        if ( myData[start] == '\"')
            chr = '\"';
        else if ( myData[start] == '[')
            chr = ']';
        else if ( myData[start] == '\'')
            chr = '\'';
        else
            chr = ' ';
        
        while ( end < Length() && myData[end]!=chr && myData[end] != '>')
            end++;
        
        GetSubString(tempStr, start+(chr!=' '), end);
        
        // Replace any xml code with the correct '&'
        const char* what = "&amp;";
        const char* with = "&";
        
        while (tempStr.FindSubString(what) != -1)
            tempStr.ReplaceSubString(what,with); 

       
        what = "&apos;";
        with = "'";
        while (tempStr.FindSubString(what) != -1)
            tempStr.ReplaceSubString(what,with);
     
        value = tempStr;
        return;
    }
}


void psXMLTag::GetTagParm(const char* parm, int &value )
{
    psString str;

    GetTagParm(parm, str);

    if (str!="")
        value = atoi(str);
    else
        value = -1;
}

void psXMLTag::GetTagParm(const char* parm, float &value )
{
    psString str;

    GetTagParm(parm, str);
    value = atof(str);
}

void psXMLTag::GetTagParm(const char* parm, double &value )
{
    psString str;

    GetTagParm(parm, str);
    value = atof(str);
}

void psXMLTag::GetTagName( psString& str)
{
    size_t i=1;
    const char *myData = GetData();

    while ( i < Size && !isspace(myData[i]) && myData[i] != '>')
        i++;

    GetSubString(str, 1,i);
}


csPtr<iDocument> ParseFile(iObjectRegistry* object_reg, const csString & name)
{
    csRef<iVFS> vfs;
    csRef<iDocumentSystem>  xml;
    csRef<iDocument> doc;
    const char* error;
    
    vfs =  csQueryRegistry<iVFS > ( object_reg);
    assert(vfs);
    csRef<iDataBuffer> buff = vfs->ReadFile(name);
    if (buff == NULL)
    {
        Error2("Could not find file: %s", name.GetData());
        return NULL;
    }
    xml =  csQueryRegistry<iDocumentSystem> (object_reg);
    if (!xml.IsValid())
        xml.AttachNew(new csTinyDocumentSystem);
    assert(xml);
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse( buff );
    if ( error )
    {
        Error3("Parse error in %s: %s", name.GetData(), error);
        return NULL;
    }
    return csPtr<iDocument>(doc);    
}

csPtr<iDocument> ParseString(const csString & str, bool notify)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    CS_ASSERT(xml != NULL);
    csRef<iDocument> doc  = xml->CreateDocument();
    const char* error = doc->Parse(str);
    if (error)
    {
        if (notify)
        {
            Error3("Error in XML: %s\nString was: %s", error, str.GetDataSafe());
        }
        return NULL;
    }
    return csPtr<iDocument>(doc);
}

csPtr<iDocumentNode> ParseStringGetNode(const csString & str, const csString & topNodeName, bool notify)
{
    csRef<iDocument> doc = ParseString(str, notify);
    if (doc == NULL) return NULL;
    csRef<iDocumentNode> root = doc->GetRoot();
    if (root == NULL) return NULL;
    return root->GetNode(topNodeName);
}

csString EscpXML(const char * str)
{
    csString ret;
    size_t strLen;

    ret = "";
    if (str != NULL)
    {
        strLen = strlen(str);
        for (size_t i=0; i < strLen; i++)
            switch (str[i])
            {
                case '\"': ret += "&quot;"; break;
                case '\'': ret += "&apos;"; break;
                case '<' : ret += "&lt;"; break;
                case '>' : ret += "&gt;"; break;
                case '&' : ret += "&amp;"; break;
                default: ret += str[i];
            }
    }

    return ret;
}

csString GetNodeXML(iDocumentNode* node, bool childrenOnly)
{
    psString xml;
    csRef<iDocumentNodeIterator> nodes;
    csRef<iDocumentAttributeIterator> attrs;
    
    if (!childrenOnly)
        xml.Format("<%s", node->GetValue());
    
    attrs = node->GetAttributes();
    while (attrs->HasNext())
    {
        csRef<iDocumentAttribute> attr = attrs->Next();
    csString escpxml = EscpXML(attr->GetValue());
        xml.AppendFmt(" %s=\"%s\"", attr->GetName(), escpxml.GetData() );
    }
    if (!childrenOnly)
        xml += ">";
    

    nodes = node->GetNodes();
    if (nodes->HasNext())
    {
        while (nodes->HasNext())
        {
            csRef<iDocumentNode> child = nodes->Next();
            if (child->GetType() == CS_NODE_TEXT)
            {
                // add the node-content to the string..but escape special XML chars in it
                xml += EscpXML(child->GetContentsValue());
            }
            else
            {
                xml += GetNodeXML(child);
            }
        }
    }
    if (!childrenOnly)
    {
        // add the node-content to the string..but escape special XML chars in it
        xml += EscpXML(node->GetContentsValue());
        xml.AppendFmt("</%s>", node->GetValue());
    }
    return xml;
}

void CopyXMLNode(iDocumentNode* source, iDocumentNode* target, int mode)
{
    if (mode == 0)
    {
        target->RemoveNodes();
        target->RemoveAttributes();
    }
    
    // copy nodes
    csRef<iDocumentNodeIterator> nodeIter = source->GetNodes();
    while (nodeIter->HasNext())
    {
        csRef<iDocumentNode> child = nodeIter->Next();
        csRef<iDocumentNode> targetChild = target->GetNode(child->GetValue());
        if (targetChild==NULL || mode==3)  // Mode 3 means don't merge tags but just insert multiples, so we create a new one here every time
        {
            targetChild = target->CreateNodeBefore(child->GetType());
            if (targetChild == NULL)
                assert(!"failed to create XML node, you are probably using wrong XML parser (xmlread instead of xmltiny)");
            targetChild->SetValue(child->GetValue());
        }
        CopyXMLNode(child, targetChild, mode);
    }
    
    // copy attributes
    csRef <iDocumentAttributeIterator> attrIter = source->GetAttributes();
    while (attrIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attrIter->Next();
        const char* attrName = attr->GetName();
        if (mode==1  ||  !target->GetAttribute(attrName))
            target->SetAttribute(attrName, attr->GetValue());
    }
}
