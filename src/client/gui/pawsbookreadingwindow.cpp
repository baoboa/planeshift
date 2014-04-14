/*
 * pawsbookreadingwindow.cpp - Author: Daniel Fryer, based on code by Andrew Craig
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

#include <psconfig.h>
#include "globals.h"

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// CLIENT INCLUDES
#include "pscelclient.h"

// PAWS INCLUDES

#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/log.h"
#include "pawsbookreadingwindow.h"

#define EDIT 1001
#define SAVE 1002
#define NEXT 1003
#define PREV 1004

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsBookReadingWindow::pawsBookReadingWindow():
    name(NULL),description(NULL),descriptionRight(NULL),descriptionCraft(NULL),
    descriptionCraftRight(NULL),writeButton(NULL),saveButton(NULL),nextButton(NULL),
    prevButton(NULL),shouldWrite(false),slotID(0),containerID(0),usingCraft(false),
    numPages(0)
{

}

pawsBookReadingWindow::~pawsBookReadingWindow()
{
    
}


bool pawsBookReadingWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_READ_BOOK);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CRAFT_INFO);

    // Store some of our children for easy access later on.
    name = dynamic_cast<pawsTextBox*>(FindWidget("ItemName"));
    if(!name) return false;

    description = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescription"));
    if(!description) return false;

    // get the right hand window, this is needed for the open book view
    descriptionRight = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescriptionRight"));
    if(!descriptionRight) return false;

    descriptionCraft = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescriptionCraft"));
    //if ( !descriptionCraft ) return false;

    // the same for the craft window, otherwise everything could bug
    descriptionCraftRight = dynamic_cast<pawsMultiPageTextBox*>(FindWidget("ItemDescriptionCraftRight"));
    //if ( !descriptionCraftRight ) return false;

    writeButton = FindWidget("WriteButton");
    //if ( !writeButton ) return false;

    saveButton = FindWidget("SaveButton");
    //if ( !saveButton ) return false;


    nextButton = FindWidget("NextButton");
    prevButton = FindWidget("PrevButton");

    usingCraft = false; // until the message turns up
    return true;
}

void pawsBookReadingWindow::HandleMessage(MsgEntry* me)
{
    switch(me->GetType())
    {
        case MSGTYPE_READ_BOOK:
        {
            Show();
            psReadBookTextMessage mesg(me);
            csRef<iDocumentNode> docnode = ParseStringGetNode(mesg.text, "Contents", false);
            if(docnode)
            {
                // these are casted into the docnode object
                dynamic_cast<pawsMultiPageDocumentView*>(description)->SetText(mesg.text.GetData());
                dynamic_cast<pawsMultiPageDocumentView*>(descriptionRight)->SetText(mesg.text.GetData());
            }
            else
            {
                description->SetText(mesg.text);
                descriptionRight->SetText(mesg.text);
            }
            name->SetText(mesg.name);
            slotID = mesg.slotID;
            containerID = mesg.containerID;
            if(writeButton)
            {
                if(mesg.canWrite)
                {
                    writeButton->Show();
                }
                else
                {
                    writeButton->Hide();
                }
            }
            if(saveButton)
            {
                if(mesg.canWrite)
                {
                    saveButton->Show();
                }
                else
                {
                    saveButton->Hide();
                }
            }
            if(descriptionCraft)
            {
                descriptionCraft->Hide();
            }
            if(descriptionCraftRight)
            {
                descriptionCraftRight->Hide();
            }
            // setup the windows for multi page view

            numPages = description->GetNumPages();

            // set the descriptionRight to be 1 page ahead of description
            descriptionRight->SetCurrentPageNum(description->GetCurrentPageNum()+1) ;

            description->Show();
            descriptionRight->Show();
            //set the background image for the book
            bookBgImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(mesg.backgroundImg);
            usingCraft = false;
            break;
        }
        case MSGTYPE_CRAFT_INFO:
        {
            Show();
            if(writeButton)
            {
                writeButton->Hide();
            }
            if(saveButton)
            {
                saveButton->Hide();
            }
            if(description)
            {
                description->Hide();
            }
            if(descriptionRight)
            {
                descriptionRight->Hide();
            }

            psMsgCraftingInfo mesg(me);
            csString text(mesg.craftInfo);
            text.ReplaceAll( "[[", "   With Higher " );
            text.ReplaceAll( "]]", " skill you could: " );

            if(text && descriptionCraft && descriptionCraftRight)
            {
                // setup the craft windows for multi page view

                descriptionCraft->SetText(text.GetData());
                descriptionCraftRight->SetText(text.GetData());

                numPages = descriptionCraft->GetNumPages();

                // set the descriptionCraftRight to be one page ahead
                descriptionCraftRight->SetCurrentPageNum(descriptionCraft->GetCurrentPageNum()+1) ;

                // show both pages
                descriptionCraft->Show();
                descriptionCraftRight->Show();

                usingCraft = true;
            }
            //name->SetText("You discover you can do the following:");
            name->SetText("");
            break;
        }
    }
}

void pawsBookReadingWindow::TurnPage(int count)
{
    // traverse forwards if there is a page to go forward to
    if(!usingCraft)
    {
        int newPageNum = description->GetCurrentPageNum()+2*count;
        int newPageNumRight = descriptionRight->GetCurrentPageNum() + 2*count;

        if(newPageNum >= 0 && newPageNum <= numPages)
        {
            // pages were set explicitly rather than using the NextPage() functions
            // this is because the pages are connected in this object
            description->SetCurrentPageNum(newPageNum);
            descriptionRight->SetCurrentPageNum(newPageNumRight);
        }
    }
    else
    {
        if(descriptionCraft && descriptionCraftRight)
        {
            int newPageNum = descriptionCraft->GetCurrentPageNum()+2*count;
            int newPageNumRight = descriptionCraftRight->GetCurrentPageNum() + 2*count;

            if(newPageNum >= 0 && newPageNum <= numPages)
            {
                descriptionCraft->SetCurrentPageNum(newPageNum);
                descriptionCraftRight->SetCurrentPageNum(newPageNumRight);
            }
        }
    }
}

void pawsBookReadingWindow::SetPage(int page)
{
    page -= page%2;  // Make sure we are at start of a double page

    if (page < 0)
    {
        page = 0;
    }
    if (page > numPages)
    {
        page = numPages - numPages%2;
    }

    // traverse forwards if there is a page to go forward to
    if(!usingCraft)
    {
        description->SetCurrentPageNum(page);
        descriptionRight->SetCurrentPageNum(page+1);
    }
    else
    {
        if(descriptionCraft && descriptionCraftRight)
        {
            descriptionCraft->SetCurrentPageNum(page);
            descriptionCraftRight->SetCurrentPageNum(page+1);
        }
    }
}


bool pawsBookReadingWindow::OnMouseDown(int button, int modifiers, int x, int y)
{
    if(button == csmbWheelDown)
    {
        TurnPage(1);
        return true;
    }
    else if(button == csmbWheelUp)
    {
        TurnPage(-1);
        return true;
    }

    return pawsWidget::OnMouseDown(button, modifiers, x ,y);
}

bool pawsBookReadingWindow::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers)
{
    //this function handles if the user uses the page up or down buttons on this text box
    //and scroll of exactly the size of lines which is shown in the textbox
    switch (key)
    {
    case CSKEY_PGDN: //go next
    case CSKEY_RIGHT:
    case CSKEY_DOWN:
        {
            TurnPage(1);
            break;
        }
    case CSKEY_PGUP: //go prev
    case CSKEY_LEFT:
    case CSKEY_UP:
        {
            TurnPage(-1);
            break;
        }
    case CSKEY_HOME:
        {
            SetPage(0);
            break;
        }
    case CSKEY_END:
        {
            SetPage(numPages);
            break;
        }
    default:
        {
            return pawsWidget::OnKeyDown(keyCode, key, modifiers);
        }
    }
    return true;
}

bool pawsBookReadingWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(widget->GetID() == EDIT)
    {
        // attempt to write on this book
        psWriteBookMessage msg(slotID, containerID);
        msg.SendMessage();
    }

    if(widget->GetID() == SAVE)
    {
        csRef<iVFS> vfs = psengine->GetVFS();
        csString tempFileNameTemplate = "/planeshift/userdata/books/%s.txt", tempFileName;
        csString bookFormat;
        if(filenameSafe(name->GetText()).Length())
        {
            tempFileName.Format(tempFileNameTemplate, filenameSafe(name->GetText()).GetData());
        }
        else   //The title is made up of Only special chars :-(
        {
            tempFileNameTemplate = "/planeshift/userdata/books/book%d.txt";
            unsigned int tempNumber = 0;
            do
            {
                tempFileName.Format(tempFileNameTemplate, tempNumber);
                tempNumber++;
            }
            while(vfs->Exists(tempFileName));
        }

        bookFormat = description->GetText();
#ifdef _WIN32
        bookFormat.ReplaceAll("\n", "\r\n");
#endif
        vfs->WriteFile(tempFileName, bookFormat, bookFormat.Length());

        psSystemMessage msg(0, MSG_ACK, "Book saved to %s", tempFileName.GetData()+27);
        msg.FireEvent();
        return true;
    }

    if(widget->GetID() == NEXT)
    {
        TurnPage(1);
        return true;

    }
    if(widget->GetID() == PREV)
    {
        TurnPage(-1);
        return true;
    }

    // close the Read window
    Hide();
    PawsManager::GetSingleton().SetCurrentFocusedWidget(NULL);
    return true;
}

bool pawsBookReadingWindow::isBadChar(char c)
{
    csString badChars = "/\\?%*:|\"<>";
    if(badChars.FindFirst(c) == (size_t) -1)
        return false;
    else
        return true;
}

void pawsBookReadingWindow::Draw()
{
    pawsWidget::DrawWindow();

    //draw background
    if(bookBgImage)
        bookBgImage->Draw(screenFrame, 0);


    pawsWidget::DrawForeground();
}

csString pawsBookReadingWindow::filenameSafe(const csString &original)
{
    csString safe;
    size_t len = original.Length();
    for(size_t c = 0; c < len; ++c)
    {
        if(!isBadChar(original[c]))
            safe += original[c];
    }
    return safe;
}

