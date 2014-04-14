/*
 * pawsstyles.h - Author: Ondrej Hurt
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 * http://www.atomicblue.org)
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

#ifndef PAWS_STYLES_HEADER
#define PAWS_STYLES_HEADER

#include <csutil/csstring.h>
#include <csutil/ref.h>
#include <csutil/hash.h>
#include <iutil/vfs.h>
#include <util/psxmlparser.h>

/**
 * \addtogroup common_paws
 * @{ */

/**
 * This macro is used to define hash tables of different types
 * that have keys of type "const char *"
 */
#define STRING_HASH(V) csHash <V, csString>

/**
 * Class pawsStyles keeps definitions of PAWS styles - a PAWS style is collection
 * of XML attributes and XML nodes that are automatically applied to all PAWS XML widgets
 * that are using this PAWS style.
 *
 * styles.xml
 * ----------
 *   \<!-- defines standard letter look --\>
 *   \<style name="standard font"\>
 *       \<font name="cupandtalon.ttf" r="-1" g="-1" b="-1" size="12"
 *             sr="0" sg="0" sb="0"/\>
 *   \</style\>
 *
 *   \<!-- style "big" overrides font size --\>
 *   \<style name="big" inherit="standard font"\>
 *       \<font size="20"/\>
 *   \</style\>
 *
 * mywidget.zml
 * ------------
 *   \<!-- this widget uses "big" style but changes font face to "verdana" --\>
 *   \<widget name="version" factory="pawsTextBox" style="big"\>
 *       \<frame x="100" y="100" width="330" height="30" /\>
 *       \<font name="LiberationSans-Regular.ttf"/\>
 *       \<text string="omgwtf"  horizAdjust="CENTRE" /\>
 *   \</widget\>
 *
 */
class pawsStyles
{
public:
    pawsStyles(iObjectRegistry* objectReg);

    /** Loads style definitions from file. Return true = success */
    bool LoadStyles(const csString &fileName);

    /** Applies style with given name to 'target'.
      * It adds all attributes and child nodes to 'target'. If target already
      * contains some of these attributes/nodes, then they are not overwritten
      */
    /* Styles need to be redesigned, the current design is too fragile. So do not use unless absolutely necessary. */
    void ApplyStyle(const char* style, iDocumentNode* target);
    void ApplyStyle(iDocumentNode* style, iDocumentNode* target);

protected:
    /** All styles inherit attributes/nodes from parent styles */
    void InheritStyles();

    /** Inherits from parent - copies nodes/attributes from parent of 'style' to 'style'.
      * 'alreadyInh' contains all styles with finished inheritance (so that we won't recalc it more times)
      * 'beingInh' contains all styles whose inheritance is just being recursively calculated
                   - this is used to detect cycles in the inheritance relationships
      */
    void InheritFromParent(iDocumentNode* style, STRING_HASH(bool) & alreadyInh, STRING_HASH(bool) & beingInh);

    iObjectRegistry* objectReg;

    /** Collection of styles - iDocumentNodes accessed by style names */
    STRING_HASH(csRef<iDocumentNode>) styles;
};


/** @} */

#endif
