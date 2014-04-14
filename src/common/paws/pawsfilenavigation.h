/*
 * Author: Andrew Mann
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_FILENAVIGATION_HEADER
#define PAWS_FILENAVIGATION_HEADER

#include "paws/pawswidget.h"

class pawsEditTextBox;
class pawsTextBox;
class pawsButton;
class pawsListBox;
class pawsComboBox;

/**
 * \addtogroup common_paws
 * @{ */

class iOnFileSelectedAction
{
public:
    virtual void Execute(const csString &string) = 0;
    // can be -1
    virtual ~iOnFileSelectedAction() {};
};

class pawsFileNavigation : public pawsWidget
{
public:
    pawsFileNavigation();
    virtual ~pawsFileNavigation();
    pawsFileNavigation(const pawsFileNavigation &origin);

    // implemented virtual functions from pawsWidgets
    virtual bool PostSetup();
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnChange(pawsWidget* widget);
    virtual void OnListAction(pawsListBox* selected, int status);
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);

    /// Overridden to reset the selection state on Show()
    virtual void Show();


    /// Called to retrieve the full path of the current browsing concatenated with the text in the
    // filename edit box.
    virtual const char* GetFullPathFilename();

    /// Called to retrieve just the full path of current browsing
    virtual const char* GetCurrentPath();

    /** Called to retrieve the current selection state of the file navigation box.
     * States can be:
     * PAWSFILENAV_SELECTION_UNDETERMINED - The window is still open and neither the perform button nor the cancel button has been pressed.
     * PAWSFILENAV_SELECTION_CANCEL - The cancel button was pressed, or the window was closed.
     * PAWSFILENAV_SELECTION_PERFORM - The perform button was pressed.
     *  The cancel button specifically overrides the perform button.  The perform button does not override the cancel button.
     */
    virtual int GetSelectionState();

    /// Called to set the text of the action/perorm button
    virtual void SetActionButtonText(const char* actiontext);

    /// Called to set the selected filename field.
    virtual bool SetSelectedFilename(const char* filename);

    virtual bool SetFileFilters(const char* filename);

    /// Refreshes the current listing of files based on the current directory
    virtual bool FillFileList();

    void Initialize(const csString &filename, const csString &filter, iOnFileSelectedAction* action);

    static pawsFileNavigation* Create(
        const csString &label, const csString &filter, iOnFileSelectedAction* action, const char* xmlWidget = "filenavigation.xml");

protected:
    /// Called to set the filterfunction.
    virtual bool SetFilterForSelection(const char* filename);
    virtual const char* GetFilterForSelection();
    /// Appends a section of a path to the current path.  Interprets . and .. appropriately
    virtual bool SmartAppendPath(const char* append);
    /// Similar to SmartAppendPath, but a leading / or \ is interpretted from the root
    virtual bool SmartSetPath(const char* path);
    virtual bool UpOnePath();
    virtual bool DownOnePath(const char* pathstring,int pathlen);
    virtual bool RelativeIsFile(const char* filename);

protected:
    /// A reference to the iVFS for the file system, used for navigation
    csRef<iVFS> vfs;

    /// The current virtual path
    csString current_path;

    csArray<csString> zip_mounts;

    /// The directory display listbox child of this widget
    pawsListBox* dirlistbox;

    /// The file display listbox child of this widget
    pawsListBox* filelistbox;

    /// A buffer used to construct the full path and file of a selected file.  Not always valid.
    // reconstructed on each call to GetFullPathFilename()
    char* fullpathandfilename;

    /// Stores the current selection state (none selected, canceled, or perform)
    int selection_state;

    iOnFileSelectedAction* action;
};

CREATE_PAWS_FACTORY(pawsFileNavigation);

/** @} */

#endif // #ifndef PAWS_FILENAVIGATION_HEADER

