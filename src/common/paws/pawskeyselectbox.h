/*
 * Author: Andrew Robberts
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
 *
 */

#ifndef PAWS_KEY_SELECT_BOX_HEADER
#define PAWS_KEY_SELECT_BOX_HEADER

#include "pawswidget.h"

/**
 * \addtogroup common_paws
 * @{ */

/**
 * A widget that can be used to select key and display combinations.
 */
class pawsKeySelectBox : public pawsWidget
{
public:
    pawsKeySelectBox()
    {
        factory = "pawsKeySelectBox";
    }
    virtual ~pawsKeySelectBox();
    pawsKeySelectBox(const pawsKeySelectBox &origin);
    bool Setup(iDocumentNode* node);

    void Draw();

    bool OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers);

    /** Sets the key for this key select box.
     *   \param _key        The new key.
     *   \param _modifiers  The new modifiers.
     */
    void SetKey(int _key, int _modifiers=0);

    int GetKey() const
    {
        return key;
    }
    int GetModifiers() const
    {
        return modifiers;
    }

    /** Gets a text representation of the key combination.
     *   \return    A text representation of the key combination of this key select box.
     */
    const char* GetText() const;

    /** Sets the key combination of this text box by parsing a representing key string.
     *   \param keyText     The string representation of the key combination.
     */
    void SetText(const char* keyText);

    virtual int GetBorderStyle();

    virtual bool GetFocusOverridesControls() const
    {
        return true;
    }

protected:

    /** Calculates the position of the text after centering.
     */
    void CalcTextPos();

    /// String representing the key combination
    csString text;

    int key;
    int modifiers;

    int textX, textY;        // Position of text inside the widget
};
CREATE_PAWS_FACTORY(pawsKeySelectBox);

/** @} */

#endif
