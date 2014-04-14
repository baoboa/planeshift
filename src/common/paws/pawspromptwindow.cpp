/*
 * pawspromptwindow.cpp - Author: Ondrej Hurt
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


// CS INCLUDES
#include <psconfig.h>

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawspromptwindow.h"
#include "pawsmanager.h"
#include "pawsbutton.h"
#include "pawstextbox.h"


#define DEFAULT_SPACING 10        //distance between inner widgets and border


pawsPromptWindow::pawsPromptWindow()
{
    label          = NULL;
    okButton       = NULL;
    cancelButton   = NULL;
    inputWidget    = NULL;
    helperWidget   = NULL;
    spacing        = DEFAULT_SPACING;
    factory        = "pawsPromptWindow";
}
pawsPromptWindow::pawsPromptWindow(const pawsPromptWindow &origin)
    :pawsWidget(origin),
     spacing(origin.spacing)
{
    inputWidget = 0;
    helperWidget = 0;
    label = 0;
    okButton = 0;
    cancelButton = 0;

    for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
    {
        if(origin.inputWidget == origin.children[i])
            inputWidget = children[i];
        else if(origin.helperWidget == origin.children[i])
            helperWidget = children[i];
        else if(origin.label == origin.children[i])
            label = dynamic_cast<pawsTextBox*>(children[i]);
        else if(origin.okButton == origin.children[i])
            okButton = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.cancelButton == origin.children[i])
            cancelButton = dynamic_cast<pawsButton*>(children[i]);

        if(inputWidget!= 0 && helperWidget!= 0 && label!= 0 && okButton !=0 && cancelButton!=0)
            break;
    }
}
bool pawsPromptWindow::PostSetup()
{
    SetBackground("Scaling Widget Background");
    UseBorder("line");

    label = new pawsTextBox();
    AddChild(label);

    okButton = new pawsButton();
    AddChild(okButton);
    okButton->SetUpImage("Scaling Button Up");
    okButton->SetDownImage("Scaling Button Down");
    okButton->SetRelativeFrameSize(80, 25);
    okButton->SetText(PawsManager::GetSingleton().Translate("OK"));
    okButton->SetSound("gui.ok");

    cancelButton = new pawsButton();
    AddChild(cancelButton);
    cancelButton->SetUpImage("Scaling Button Up");
    cancelButton->SetDownImage("Scaling Button Down");
    cancelButton->SetRelativeFrameSize(80, 25);
    cancelButton->SetText(PawsManager::GetSingleton().Translate("Cancel"));
    cancelButton->SetSound("gui.cancel");

    SetBackgroundAlpha(0);
    return true;
}

void pawsPromptWindow::SetLabel(const csString &label)
{
    this->label->SetText(label);
    this->label->SetSizeByText(5,5);
    LayoutWindow();
}

void pawsPromptWindow::SetSpacing(int spacing)
{
    this->spacing = spacing;
}

void pawsPromptWindow::LayoutWindow()
{
    int windowWidth, windowHeight, buttony;

    assert(label && okButton && cancelButton && inputWidget);

    windowWidth = csMax(
                      2 * spacing + label->GetDefaultFrame().Width(),
                      csMax(
                          4 * spacing + inputWidget->GetDefaultFrame().Width(),
                          3 * spacing + okButton->GetDefaultFrame().Width() + cancelButton->GetDefaultFrame().Width()
                      )
                  );
    windowHeight = 4 * spacing + label->GetDefaultFrame().Height() + inputWidget->GetDefaultFrame().Height()
                   + okButton->GetDefaultFrame().Height();
    if(helperWidget != NULL)
        windowHeight += spacing + helperWidget->GetDefaultFrame().Height();
    SetRelativeFrameSize(windowWidth, windowHeight);

    label->SetRelativeFramePos((windowWidth-label->GetDefaultFrame().Width()) / 2, spacing);

    inputWidget->SetRelativeFramePos((windowWidth-inputWidget->GetDefaultFrame().Width()) / 2,
                                     label->GetDefaultFrame().ymax + spacing);

    if(helperWidget != NULL)
    {
        helperWidget->SetRelativeFramePos((windowWidth-inputWidget->GetDefaultFrame().Width()) / 2,
                                          inputWidget->GetDefaultFrame().ymax + spacing);
        buttony = helperWidget->GetDefaultFrame().ymax + spacing;
    }
    else
        buttony = inputWidget->GetDefaultFrame().ymax + spacing;

    okButton->SetRelativeFramePos((windowWidth-okButton->GetDefaultFrame().Width()-cancelButton->GetDefaultFrame().Width()-spacing) / 2,
                                  buttony);

    cancelButton->SetRelativeFramePos(okButton->GetDefaultFrame().xmax + spacing,
                                      buttony);

    SetAppropriatePos();
}

void pawsPromptWindow::SetAppropriatePos()
{
    psPoint mouse;
    int x, y;
    int width, height;

    width = screenFrame.Width();
    height = screenFrame.Height();

    mouse = PawsManager::GetSingleton().GetMouse()->GetPosition();
    x = mouse.x - width  / 2;
    y = mouse.y - height / 2;
    MoveTo(x, y);
    MakeFullyVisible();
}
