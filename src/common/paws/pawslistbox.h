/*
 * pawslistbox.h - Author: Andrew Craig
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

#ifndef PAWS_LIST_BOX_HEADER
#define PAWS_LIST_BOX_HEADER

#include "pawswidget.h"
#include "pawsmanager.h"
#include "pawstextbox.h"
#include "util/log.h"

class pawsScrollBar;

/**
 * \addtogroup common_paws
 * @{ */

/** Type of functions that are used to compare listbox rows during sorting: */
typedef int (*listBoxSortingFunc)(pawsWidget* a, pawsWidget* b);

/** Compare function that works with listbox columns that are pawsTextBox widgets: */
int textBoxSortFunc(pawsWidget* widgetA, pawsWidget* widgetB);


//-----------------------------------------------------------------------------
//                            struct ColumnInfo
//-----------------------------------------------------------------------------

/**
 * Defines the structure of the list box table.
 *
 * This describes how a column is defined.  The height should be constant across
 * all the columns for a particular table.
 */
struct ColumnDef
{
    ColumnDef()
    {
        widgetNode  =  NULL;
        sortFunc    =  NULL;
        sortable = true;
        width = 0;
        height = 0;
    }

    int width;
    int height;
    csRef<iDocumentNode> widgetNode;    /// Passed to new column widgets to create
    csString xmlbinding;
    bool sortable;

    listBoxSortingFunc sortFunc;        /// Function used for comparing rows when sorting the listbox
};


#define LISTBOX_HIGHLIGHTED  0x01  /// Single click event
#define LISTBOX_SELECTED     0x02

//-----------------------------------------------------------------------------
//                            class pawsListBoxRow
//-----------------------------------------------------------------------------


/** A List Box Row.
 *  This is just a container of widgets that represent a list box record.
 */
class pawsListBoxRow : public pawsWidget
{
public:
    pawsListBoxRow();
    pawsListBoxRow(const pawsListBoxRow &origin);
    /// get a pointer to one of the columns of this row
    pawsWidget* GetColumn(size_t column);

    /// Get total columns
    size_t GetTotalColumns()
    {
        return columns.GetSize();
    }

    /// adds a column to this row
    void AddColumn(int column, ColumnDef* def);

    /// creates the title row
    void AddTitleColumn(int column, ColumnDef* def);

    bool OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers);

    /// A single mouse click (left button) highlights the row
    bool OnMouseDown(int button, int modifiers, int x, int y);

    /// A double click selects the row.
    bool OnDoubleClick(int button, int modifiers, int x, int y);

    /// Heading rows exist within the list but are not clickable
    void SetHeading(bool flag);

    /// Returns the last index of the row
    int GetLastIndex()const
    {
        return lastIndex;
    }

    /// Sets the last index of the row. Is used by pawsListBox to update lastIndex variable when sorting.
    void SetLastIndex(int index)
    {
        lastIndex = index;
    }

    /**
     * Handles hiding of the row. Used to give focus back to the list box
     * before hiding.
     */
    void Hide();

    bool IsHeading()
    {
        return isHeading;
    }
private:
    /// Store whether this row should be a heading or not
    bool isHeading;

    int lastIndex; ///< index of the related row

    /// Store a list of columns for easy access.
    csArray<pawsWidget*> columns;

};


//-----------------------------------------------------------------------------
//                            class pawsListBox
//-----------------------------------------------------------------------------


#define LISTBOX_MOUSE_SCROLL_AMOUNT 3

/**
 * A simple list box widget.
 *
 * A list box is made up a list of another type of widgets called
 * pawsListBoxRows.  Each row is then made up of several widgets that
 * constitue a 'row'.  So a row may be a textbox, image, button.
 *
 * This is how a list box is defined in XML.  You define the basic layout
 * of it and how each column in the rows should be constructed. If you use
 * the autoid="yes" every widget in the listbox will have an
 * id=ListBoxID+ row*numberOfColumns+coloum:


    \<widget name="PetitionList" factory="pawsListBox" xmlbinding="petition_list" id="0" autoid="yes" sortBy="2" sortOrder="asc" \>
        Size of entire list box
        \<frame x="4" y="34" width="592" height="288" border="yes"/\>

        Each row in the list box will be 32 high
        \<columns height="32" xmlbinding="p"\>

            define a column that is 140 wide.  The first column of any row
            will be a textbox widget as defined by the \<widget\>\</widget\> class
            \<column width="140" xmlbinding="GM"\>
                \<widget name="GM" factory="pawsTextBox"\>\</widget\>
            \</column\>

            define other columns:
            \<column width="150"  xmlbinding="stat"\>
                \<widget name="Status" factory="pawsWidget"\>
                    \<bgimage resource="Funny" /\>
                \</widget\>
            \</column\>

            \<column width="302"  xmlbinding="pet"\>
                \<widget name="Petition" factory="pawsTextBox"\>\</widget\>
            \</column\>
        \</columns\>
    \</widget\>

 */
class pawsListBox : public pawsWidget
{
public:
    pawsListBox();
    pawsListBox(const pawsListBox &origin);
    bool Setup(iDocumentNode* node);
    bool PostSetup();

    virtual ~pawsListBox();

    /** Get the row that is currently selected.
     * @return A pointer to the selected row or NULL.
     */

    int GetSelection()
    {
        return selected;
    }

    /// Get selected row
    pawsListBoxRow* GetSelectedRow();

    /// Get text from specified column in the selected row
    const char* GetSelectedText(size_t columnId);

    /// Returns number of selected row (-1 if none is selected)
    int GetSelectedRowNum();

    /// Remove selected row
    pawsListBoxRow* RemoveSelected();

    /// Remove based on widget id. Note this also deletes the row.
    void Remove(int id);

    /// Remove based on row pointer
    void Remove(pawsListBoxRow* rowToRemove);

    void AddRow(pawsListBoxRow* row);

    /// Get number of rows in listboz
    size_t GetRowCount();

    /// Get a particular row
    pawsListBoxRow* GetRow(size_t x);

    /// Creates a new row ( default at the end ) and returns a pointer to it.
    pawsListBoxRow* NewRow(size_t position = (size_t)-1);
    /// Creates a new row ( default at the end ) filled by values from rowEntry and returns a pointer to it.
    pawsListBoxRow* NewTextBoxRow(csList<csString> &rowEntry,size_t position = (size_t)-1);

    void Clear();

    /// Notifies (parent/notifyTarget) widget that a row has been selected
    void SendOnListAction(int status);

    /// Highlights the selected row
    bool Select(pawsListBoxRow* row, bool notify = true);

    /// Highlights the selected row (by index)
    bool SelectByIndex(int index, bool notify = true);

    /// Creates the title row
    void CreateTitleRow();

    virtual bool OnScroll(int direction, pawsScrollBar* widget);

    bool OnMouseDown(int button, int modifiers, int x, int y);

    int GetTotalColumns()
    {
        return totalColumns;
    }

    /**
     * Set how many columns this list box will have.
     *
     * This is usually for code constructed list boxes and creates a new
     * set of column definitions.
     */
    void SetTotalColumns(int numCols);

    /**
     * Set how a column should be constructed.
     *
     * This is usuall used for code constructed list boxes.
     * @param col The column number
     * @param width The width of that column.
     * @param height The height of what rows should be.
     * @param widgetDesc This is an XML description of the widget. Much the same as you
     *                   would find in the .xml files.
     */
    void SetColumnDef(int col, int width, int height, const char* widgetDesc);

    void UseTitleRow(bool yes);

    /**
     * Override the general self populate to handle creation of new rows.
     */
    virtual bool SelfPopulate(iDocumentNode* node);

    bool IsSelectable()
    {
        return selectable;
    }

    /**
     * Is this an autoID listbox
     */
    bool IsAutoID()
    {
        return autoID;
    }

    /**
     * Convert from an automatic ID to col/row
     */
    bool ConvertFromAutoID(int id, int &row, int &col);

    /**
     *  Moves the select bar up (direction=true) or down (direction=false).
     */
    void MoveSelectBar(bool direction);

    void CalculateDrawPositions();

    /**
     * When enter is pressed the highlighted row will be selected
     * and the widget will be notified
     */
    bool OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers);

    void Resize();

    /**
     * Sets the widget that will be notified when a row is selected.
     * When nothing is set the OnListAction messages will be sent
     * to the parent widget
     */
    void SetNotify(pawsWidget* target)
    {
        notifyTarget = target;
    };

    /**
     * Gets and sets the column that the listbox is sorted by.
     * Number -1 means no sorting, in both cases.
     */
    int  GetSortedColumn();
    void SetSortedColumn(int colNum);

    bool GetSortOrder();
    void SetSortOrder(bool ascOrder);

    /**
     * Sets the function that compares listbox rows when sorting by given column.
     * This makes the column sortable.
     */
    void SetSortingFunc(int colNum, listBoxSortingFunc sortFunc);

    /**
     * Sort rows according to current sort column and sort order.
     * You can use this after you added new rows to listbox;
     */
    void SortRows();


    /**
     * Returns listbox cell of type pawsTextBox or NULL (when the cell does not exist or it is another type).
     */
    pawsTextBox* GetTextCell(int rowNum, int colNum);

    /**
     * Returns value of cell of type pawsTextBox.
     */
    csString GetTextCellValue(int rowNum, int colNum);

    /**
     * Sets value of cell of type pawsTextBox.
     */
    void SetTextCellValue(int rowNum, int colNum, const csString &value);

    void AutoScrollUpdate(bool v)
    {
        autoUpdateScroll = v;
    }

    void SetScrollBarMaxValue();
    void MoveRow(int rownr,int dest);

protected:

    /**
     * Defines border size around child widgets
     *
     * If borders are used by this widget, it offsets position of each child widget and
     * total space designed for child widgets is also increased (by twice size of border)
     */
    static const int BORDER_SIZE = 5;

    void CreateSortingArrow(int colNum);
    void SetSortingArrow(int colNum, bool ascOrder);
    void CheckSortingArrow(int colNum, bool ascOrder);
    void DeleteSortingArrow(int colNum);

    pawsWidget* GetColumnTitle(int colNum);

    /**
     * Gets height of widget after vertical borders are being excluded
     *
     * @return Height of widget after subtracting vertical borders height
     */
    int GetUnborderedHeight();

    bool usingTitleRow;

    pawsScrollBar* scrollBar;
    pawsScrollBar* horzscrollBar;

    int totalColumns;
    int totalRows;
    int columnHeight;
    int rowWidth;
    int topRow;
    int selected;
    int xMod;
    bool autoID;
    bool autoResize;
    bool autoUpdateScroll;

    ColumnDef* columnDef;

    /// the widget that will be notified when a row is selected
    pawsWidget* notifyTarget;

    csArray<pawsListBoxRow*> rows;
    pawsListBoxRow* titleRow;
    csString xmlbinding_row;

    int sortColNum;    // number of sorted column (or -1)
    bool ascOrder;

    /**
     * This is static function that is used as argument to qsort() and a few variables that influence
     * how this function works.
     */
    static int sort_cmp(const void* rowA, const void* rowB);
    static listBoxSortingFunc sort_sortFunc;
    static int sort_sortColNum;
    static bool sort_ascOrder;

    csString highlightImage;
    unsigned int highlightAlpha;

    int         arrowSize;
    int         scrollbarWidth;
    int         scrollbarHeightMod;
    bool        useBorder;
    bool        selectable;     // can the user select listbox rows ?
    csString    arrowUp;
    csString    arrowDown;
};

CREATE_PAWS_FACTORY(pawsListBox);


/** @} */

#endif
