/*
 * pawscombopromptwindow.h - Author: Ondrej Hurt
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

#ifndef PAWS_COMBO_PROMPT_WINDOW_HEADER
#define PAWS_COMBO_PROMPT_WINDOW_HEADER

#include <csutil/list.h>
#include <iutil/document.h>
#include "pawspromptwindow.h"
#include "pawscombo.h"

class pawsButton;
class ComboWrapper;

/**
 * \addtogroup common_paws
 * @{ */

class iOnItemChosenAction
{
public:
    virtual void OnItemChosen(const char* name,int param,int itemNum, const csString &itemText) = 0;
    // can be -1
    virtual ~iOnItemChosenAction() {};
};


class ComboWrapper : public pawsWidget
{
public:
    ComboWrapper(int width, int height)
    {
        SetRelativeFrameSize(width, height);
    }
    ComboWrapper() {};
    ComboWrapper(const ComboWrapper &origin)
        :pawsWidget(origin)
    {
    }
    bool PostSetup()
    {
        combo = new pawsComboBox();
        AddChild(combo);
        combo->SetRelativeFrame(0, 0, 200, 40);
        combo->UseBorder("line");
        combo->SetNumRows(6);
        combo->SetRowHeight(20);
        combo->PostSetup();
        combo->SetSorted(false);
        return true;
    }

    pawsComboBox* combo;
};
CREATE_PAWS_FACTORY(ComboWrapper);
/**
 * pawsComboPromptWindow is window that lets the user choose item from combo box
 */
class pawsComboPromptWindow : public pawsPromptWindow
{
public:
    pawsComboPromptWindow();
    pawsComboPromptWindow(const pawsComboPromptWindow &origin);
    //from pawsWidget:
    virtual bool PostSetup();
    virtual void Close();
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

    void NewOption(const csString &text);
    void Select(int optionNum);

    void SetAction(iOnItemChosenAction* action,const char* name, int param)
    {
        this->action = action;
        this->name   = name;
        this->param  = param;
    }

    static pawsComboPromptWindow* Create(const csString &label, iOnItemChosenAction* action,const char* name, int param=0);

protected:
    iOnItemChosenAction* action;
    csString name;
    int param;

    ComboWrapper* wrapper;
};

CREATE_PAWS_FACTORY(pawsComboPromptWindow);

/** @} */

#endif

