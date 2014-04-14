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

#ifndef EEDIT_ERROR_TOOLBOX_WINDOW_HEADER
#define EEDIT_ERROR_TOOLBOX_WINDOW_HEADER

#include <csutil/array.h>
#include "eedittoolbox.h"
#include "paws/pawswidget.h"

class pawsButton;
class pawsMessageTextBox;
class pawsCheckBox;

/**
 * \addtogroup eedit
 * @{ */

class EEditError
{
public:
    EEditError(const char * _desc, int _colour, int _severity, bool _isEffectError)
        : colour(_colour), severity(_severity), isEffectError(_isEffectError)
    {
        if (_desc == 0 || _desc[0] == '\0')
            desc = " ";
        else
            desc = _desc;
    }

    csString    desc;
    int         colour;
    int         severity;
    bool        isEffectError;
};

/** This displays effect errors.
 */
class EEditErrorToolbox : public EEditToolbox, public pawsWidget, public scfImplementation0<EEditErrorToolbox>
{
public:
    EEditErrorToolbox();
    virtual ~EEditErrorToolbox();

    /** Adds an error to the reporter.
     *   \param severity        The severity of the error.  Check csReport for a list of available severities.
     *   \param msgId           An identifier saying where the message came from.
     *   \param description     The actual error description.
     */
    void AddError(int severity, const char * msgId, const char * description);

    /** Sets whether the effects are being loaded, so that they can be handled differently if desired by the user.
     *   \param isLoadingEffects    True if the effects are going to be loaded, false if they are not.
     */
    void SetLoadingEffects(bool isLoadingEffects);

    /** Clears all current effects.
     */
    void Clear();

    /** Builds a filter based on the state of current widgets and filters the list.
     */
    void ResetFilter();
    
    // inheritted from EEditToolbox
    virtual void Update(unsigned int elapsed);
    virtual size_t GetType() const;
    virtual const char * GetName() const;
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual void Show();
    
private:

    /** Displays the given error in the error list.
     *   \param error   The error to display.
     */
    void DisplayError(const EEditError & error);
    
    pawsMessageTextBox * errorText;
    pawsButton *         clearButton;
    pawsButton *         hideButton;
    pawsCheckBox *       onlyEffectErrors;
    pawsCheckBox *       showErrors;
    pawsCheckBox *       showWarnings;
    pawsCheckBox *       showNotifications;

    bool loadingEffects;

    csArray<EEditError> errors;
};

CREATE_PAWS_FACTORY(EEditErrorToolbox);

/** @} */

#endif 
