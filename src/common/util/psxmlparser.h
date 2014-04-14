/*
 * xmlparser.h
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
#ifndef PS_XML_PARSER_H
#define PS_XML_PARSER_H

#include "util/psstring.h"
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>
#include "util/log.h"

/**
 * \addtogroup common_util
 * @{ */

/**
 * Loads and parses a XML file, then returns its parsed XML document.
 */
csPtr<iDocument> ParseFile(iObjectRegistry* object_reg, const csString & name);

/**
 * Parses a XML string, then returns the parsed document.
 */
csPtr<iDocument> ParseString(const csString & str, bool notify = true);

/**
 * Parses a XML string, then returns the top node with name 'topNodeName'.
 */
csPtr<iDocumentNode> ParseStringGetNode(const csString & str, const csString & topNodeName, bool notify = true);

/**
 * Escapes special XML characters in 'str'.
 */
csString EscpXML(const char * str);

/**
 * Generates XML representing given node.
 *
 * If 'childrenOnly' is true, only XML of child nodes will be returned
 */
csString GetNodeXML(iDocumentNode* node, bool childrenOnly = false);

/**
 * Copies/merges children and attributes of 'source' to 'target'.
 *
 * Mode : 0=clear target first (i.e. exact copy)
 *        1=don't clear but overwrite those attributes in 'target' that are in 'source'
 *        2=don't clear and don't overwrite existing attributes, add new only
 */
void CopyXMLNode(iDocumentNode* source, iDocumentNode* target, int mode);

class psXMLTag;

class psXMLString : public psString
{
public:
    psXMLString() { };
    psXMLString(const char *str) : psString(str) { };
    int FindTag( const char* tagName, int start=0 );
    size_t FindNextTag( size_t start );
    size_t GetTag( int start, psXMLTag& tag );
    size_t GetTagSection( int start, const char* tagName, psXMLString& tagSection);
    
    /**
     * GetWithinTagSection return the psXMLString section text of a tag.
     *
     * <pre>\<TAG\>XML\</TAG\></pre>
     *
     * @param start The start position.
     * @param tagName The name of the tag.
     * @param tagSection Return the section text. If tag not found returning "".
     */
    size_t GetWithinTagSection( int start, const char* tagName, psXMLString& tagSection);

    /**
     * GetWithinTagSection return a csString containing the text in a tag section.
     *
     * <pre>\<TAG\>Text\</TAG\></pre>
     *
     * @param start The start position.
     * @param tagName The name of the tag.
     * @param value Return the section text. If TAG not found the string isn't modified. 
     */
    size_t GetWithinTagSection( int start, const char* tagName, csString& value);
    
    size_t GetWithinTagSection( int start, const char* tagName, int& value);
    size_t GetWithinTagSection( int start, const char* tagName, double& value);

    void operator=(const char* str)
    {
        csString::operator=(str);
    }
protected:
    size_t FindMatchingEndTag(int iStart, const char *tagName);
};


//-------------------------------------------------------------------
//
class psXMLTag : public psString
{
public: 
    psXMLTag() {}
    psXMLTag(psXMLString& str, int where)
    {
        if (where<0)
            Replace("");
        else
            str.GetTag( where, *this );
    }

    void GetTagParm(const char* param, csString& value);
    void GetTagParm(const char* param, int& value);
    void GetTagParm(const char* param, double& value);
    void GetTagParm(const char* param, float& value);

    void GetTagName(psString& name);
}; 

/** @} */

#endif
