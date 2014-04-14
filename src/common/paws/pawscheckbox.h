/**
 * pawscheckbox.h
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
#ifndef PAWS_CHECKBOX_HEADER
#define PAWS_CHECKBOX_HEADER


#include "pawsbutton.h"

class pawsTextBox;

/**
 * \addtogroup common_paws
 * @{ */

/** A combination widget that has a check box and a text label.

    This widget is defined in an XML as:

    \<widget name="nameHere" factory="pawsCheckBox" id="intIDNumber"\>
        \<!-- The size of the entire widget including text and button. --\>
        \<frame x="75" y="5" width="70" height="30" /\>

        \<!-- The text label to use and it's position relative to the button --\>
        \<text string="Sell" position="right"/\>
    \</widget\>

    Current supported positions are left/right.

    By default it uses the checkon/checkoff named images and automatically
    assumes that their size is 16x16.
*/
class pawsCheckBox: public pawsWidget
{
public:
    /**
     * @fn pawsCheckBox()
     * @brief This function is not yet documented.
     */
    pawsCheckBox();

    pawsCheckBox(const pawsCheckBox &origin);

    /**
     * @fn ~pawsCheckBox()
     * @brief This function is not yet documented.
     */
    virtual ~pawsCheckBox();

    /**
     * @fn virtual bool Setup( iDocumentNode* node )
     * @brief This function is not yet documented.
     *
     * @param *node TODO.
     */
    virtual bool Setup(iDocumentNode* node);

    /**
     * @fn bool SelfPopulate( iDocumentNode *node);
     * @brief This function is not yet documented.
     *
     * @param *node TODO.
     */
    bool SelfPopulate(iDocumentNode* node);

    /**
     * @fn bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
     * @brief This function is not yet documented.
     *
     * @param mouseButton TODO.
     * @param keyModifier TODO.
     * @param *widget TODO.
     */
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

    /**
     * @fn virtual void SetState( bool state )
     * @brief Set whether the checkbox is "checked" or "unchecked".
     *
     * @param state The desired value: True = checked, False = unchecked.
     */
    virtual void SetState(bool state);

    /**
     * @fn virtual bool GetState()
     * @brief Return whether the checkbox is "checked" or "unchecked".
     *
     * Return the value of the value of the checkbox in boolean form.
     * True = "checked"
     * False = "unchecked"
     */
    virtual bool GetState();

    /**
     * @fn virtual void SetText( const char* string )
     * @brief Set the text contained in the checkbox.
     *
     * @param string The text to which checkbox should be set.
     */
    virtual void SetText(const char* string);

    /**
     * @fn virtual const char* GetText()
     * @brief Return the text contained within the checkbox.
     */
    virtual const char* GetText();

    /**
     * @fn virtual void OnUpdateData(const char *dataname,PAWSData& data)
     * @brief This function is not yet documented.
     *
     * @param *dataname TODO.
     * @param data TODO.
     */
    virtual void OnUpdateData(const char* dataname,PAWSData &data);

    /**
     * @fn virtual void SetImages(const char* up, const char* down)
     * @brief This function is not yet documented.
     *
     * @param *up TODO.
     * @param *down TODO.
     */
    virtual void SetImages(const char* up, const char* down);

    /**
     * @fn virtual double GetProperty(const char * ptr)
     * @brief Get the value of the specified property.
     *
     * @param env Math environment.
     * @param ptr The name of the property to get.
     */
    virtual double GetProperty(MathEnvironment* env, const char* ptr);

    /**
     * @fn virtual void SetProperty(const char * ptr, double value)
     * @brief Set the value of the specified property.
     *
     * @param *ptr The name of the property to set.
     * @param value The new value of the property to be set.
     */
    virtual void SetProperty(const char* ptr, double value);

    /** @@@ Hack: please someone tell me how to do this better? */
    pawsButton* GetButton()
    {
        return checkBox;
    }

private:
    pawsButton* checkBox;
    pawsTextBox* text;
    int textOffsetX;
    int textOffsetY;

    csString checkBoxOff;
    csString checkBoxOn;
    csString checkBoxGrey;

    int checkBoxSize;

};
CREATE_PAWS_FACTORY(pawsCheckBox);

/** @} */

#endif
