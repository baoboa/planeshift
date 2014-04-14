/*
 * pawsbookreadingwindow.h - Author: Daniel Fryer, based on work by Andrew Craig
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

#ifndef PAWS_BOOK_READING_WINDOW_HEADER
#define PAWS_BOOK_READING_WINDOW_HEADER

#include "paws/pawswidget.h"

class pawsTextBox;
class pawsMultiLineTextBox;

/** A window that shows the description of an item.
 */
class pawsBookReadingWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    pawsBookReadingWindow();
    virtual ~pawsBookReadingWindow();

    bool PostSetup();

    void HandleMessage(MsgEntry* message);

    /**
     * Turn a number of pages.
     *
     * Will turn two and two pages as you turn a book.
     *
     * @param count The number of pages to turn in the book.
     */
    void TurnPage(int count);

    /**
     * Turn to a given page.
     *
     * Will turn to start of a double side where the given page is.
     *
     * @param page The page to view
     */
    void SetPage(int page);
    
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers);
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

private:
    pawsTextBox*            name;
    pawsMultiPageTextBox*   description;
    pawsMultiPageTextBox*   descriptionRight;
    pawsMultiPageTextBox*   descriptionCraft;
    pawsMultiPageTextBox*   descriptionCraftRight;
    pawsWidget*             writeButton;
    pawsWidget*             saveButton;
    pawsWidget*             nextButton;
    pawsWidget*             prevButton;
    bool shouldWrite;
    int slotID;
    int containerID;
    bool usingCraft;
    csString filenameSafe(const csString &original);
    bool isBadChar(char c);

    /// Background image.
    csRef<iPawsImage> bookBgImage;
    virtual void Draw();

    int numPages;

};

CREATE_PAWS_FACTORY(pawsBookReadingWindow);


#endif


