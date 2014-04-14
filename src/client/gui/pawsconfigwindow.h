/*
 * pawsconfigwindow.h - Author: Ondrej Hurt
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

#ifndef PAWS_CONFIG_WINDOW_HEADER
#define PAWS_CONFIG_WINDOW_HEADER

#include <csutil/parray.h>

#include "paws/pawswidget.h"
#include "paws/pawstree.h"
#include "gui/pawscontrolwindow.h"



/* Configuration is divided into sections. Sections are identified by names.
 * The main configuration window consist of a tree of sections on the left and
 * one section window on the right. Each node of the tree represents one section.
 * When a node is selected, a section window is shown on the right. (e.g. keyboard configuration)
 * All section windows are widgets derived from the pawsConfigSectionWindow class.
 *
 * The tree on the left is described in /data/configtree.xml file.
 * Each tree node that is meant to invoke configuration section (pawsConfigSectionWindow subclass)
 * must have two attributes defined:
 *            - SectionName: string that identifies section
 *            - factory: name of pawsConfigSectionWindow subclass that takes care of GUI of this section
 *
 * pawsConfigSectionWindow is the superclass of section windows.
 */
class pawsConfigSectionWindow : public pawsWidget
{
public:
    pawsConfigSectionWindow()
    {
        dirty = false;
    }
    void SetSectionName(const csString &_sectionName)
    {
        sectionName = _sectionName;
    }
    virtual bool Initialize() = 0;
    /// sets content of dialog according to current configuration
    virtual bool LoadConfig() = 0;
    /// were settings modified ?
    virtual bool IsDirty()
    {
        return dirty;
    }
    /// remembers settings in-game and saves them permanently to a file also
    virtual bool SaveConfig() = 0;

    /// sets content of dialog according to default configuration
    virtual void SetDefault() = 0;

protected:
    /* Says what section of configuration is being configured by this window.
     * A pawsConfigSectionWindow could be used by more than one section
     * and 'sectionName' tells which section it is.
     */
    csString sectionName;

    /* Is data in configuration window different from data in configuration file
     * i.e. do we need to save it ?
     * Should be set to true when user changes something, and to false when the data is loaded/saved.
     */
    bool dirty;

};



// The sectWnd_t structure holds a pair [ section name , section window ]
struct sectWnd_t
{
    sectWnd_t(const csString &_sectName, pawsConfigSectionWindow* _sectWnd)
    {
        sectName = _sectName;
        sectWnd = _sectWnd;
    }
    csString sectName;
    pawsConfigSectionWindow* sectWnd;
};



// Class pawsConfigWindow implements the main configuration window that is invoked
// in-game by clicking on the "Options" button.

class pawsConfigWindow : public pawsControlledWindow
{
public:

    pawsConfigWindow();

    // from pawsWidget:
    virtual bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnSelected(pawsWidget* widget);
    virtual bool PostSetup();

    /// Sets which widget should receive OnButtonPressed() notification when the dialog
    /// is closing.
    void SetNotify(pawsWidget* notify);


private:
    /// Finds section window (in the 'sectWnds' list) for section 'sectName'
    pawsConfigSectionWindow* FindSectionWindow(const csString &sectName);

    iObjectRegistry* object_reg;
    csArray<sectWnd_t> sectWnds;

    pawsSimpleTree* sectionTree;
    pawsWidget* sectionParent;
    pawsConfigSectionWindow* currSectWnd;
    pawsWidget* okButton, * resetButton;
    pawsWidget* notify;
};


CREATE_PAWS_FACTORY(pawsConfigWindow);


#endif

