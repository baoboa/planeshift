/*
 * pawstextbox.h - Author: Andrew Craig
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
// pawstextbox.h: interface for the pawsTextBox class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_TEXT_BOX_HEADER
#define PAWS_TEXT_BOX_HEADER

struct iVirtualClock;

#include "pawswidget.h"
#include <csutil/parray.h>
#include <ivideo/fontserv.h>
#include <iutil/virtclk.h>

#include <ispellchecker.h>

/**
 * \addtogroup common_paws
 * @{ */

/**
 * A basic text box widget. Useful for simply displaying text.
 */
class pawsTextBox : public pawsWidget
{
public:
    /**
     * Various positions to center the text on vertically
     */
    enum pawsVertAdjust
    {
        vertTOP,    ///< Adjusted to top
        vertBOTTOM, ///< Adjusted to bottom
        vertCENTRE  ///< Adjusted to centre
    };

    /**
     * Various positions to center the text on horizontally
     */
    enum pawsHorizAdjust
    {
        horizLEFT,  ///< Adjusted to left
        horizRIGHT, ///< Adjusted to right
        horizCENTRE ///< Adjusted to centre
    };

    /**
     * Basic constructor
     */
    pawsTextBox();

    /**
     * Basic deconstructor
     */
    virtual ~pawsTextBox();

    /**
     * Copy constructor
     */
    pawsTextBox(const pawsTextBox &origin);

    /**
     * Sets up the text box from a document node<br>
     * Possible attributes in the node:<ul>
     *   <li>vertical - yes/no</li>
     *   <li>text</li><ul>
     *     <li>horizAdjust - CENTRE/RIGHT/LEFT</li>
     *     <li>vertAdjust - CENTRE/BOTTOM/TOP</li>
     *     <li>string - text</li></ul></ul>
     *
     * @param node The node to pull the attributes from
     * @return True
     */
    bool Setup(iDocumentNode* node);
    /**
     * Sets the text from the node using either the text attribute or the node's content
     * @param node The node to pull the text from
     * @return True if data is pulled, false if the node is empty
     */
    bool SelfPopulate(iDocumentNode* node);

    /// Draws the text box
    void Draw();

    /**
     * Sets the text to display
     * @param text The string
     */
    void SetText(const char* text);

    /**
     * Sets the text to display using formatting
     * @param fmt The format to use to format the string to display
     */
    void FormatText(const char* fmt, ...);

    /**
     * Gets the text that is displayed by this text box
     * @return \ref text
     */
    const char* GetText() const
    {
        return text;
    }

    /**
     * Sets if this text box should be rendered vertically (top to bottom)
     * @param vertical Whether it should be rendered vertically
     */
    void SetVertical(bool vertical);

    /**
     * Alters how the text box is laid out in its container
     * @param horiz Sets the horizontal alignment for the text
     * @param vert Sets the vertical alignment for the text
     */
    void Adjust(pawsHorizAdjust horiz, pawsVertAdjust vert);
    /**
     * Alters how the text box is laid out horizontally in its container
     * @param horiz Sets the horizontal alignment for the text
     */
    void HorizAdjust(pawsHorizAdjust horiz);
    /**
     * Alters how the text box is laid out vertically in its container
     * @param vert Sets the vertical alignment for the text
     */
    void VertAdjust(pawsVertAdjust vert);

    /**
     * Sets the size of the widget to fit the text that it displays
     * @param padX Horizontal padding
     * @param padY Vertical padding
     */
    void SetSizeByText(int padX,int padY);

    /**
     * Normal text boxes should not be focused.  Does nothing
     * @return false
     */
    virtual bool OnGainFocus(bool)
    {
        return false;
    }

    /**
     * Gets the border style for the text box
     * @return Always \ref BORDER_SUNKEN
     */
    virtual int GetBorderStyle()
    {
        return BORDER_SUNKEN;
    }

    /**
     * Sets if the text box is having its text grayed
     * @param g Overwrites \ref grayed
     */
    inline void Grayed(bool g)
    {
        grayed = g;
    }

    /**
     * Gets the font color based on the set colour and if \ref grayed is set
     * @return Gray if grayed is true, colour otherwise
     */
    virtual int GetFontColour();

    /**
     * Called whenever subscribed data gets updated
     * Sets the text to either the data's string value or a properly formatted float representation
     * @param dataname Unused
     * @param data The data to update the text with
     */
    virtual void OnUpdateData(const char* dataname,PAWSData &data);

    /**
     * Utility function to calculate number of code points in a substring
     * @param text Text to calculate on
     * @param start Index to start on
     * @param len Length to perform calculation on or -1 for start to end
     * @return Number of code points
     */
    static int CountCodePoints(const char* text, int start = 0, int len = -1);

    /**
     * Utility function to rewind a UTF-8 string by a certain number of codepoints
     * @param text Text to calculate on
     * @param start Index to begin rewinding at
     * @param count Number of code points to rewind by
     * @return Resulting index
     */
    static int RewindCodePoints(const char* text, int start, int count);

    /**
     * Utility function to skip a UTF-8 string by a certain number of codepoints
     * @param text Text to calculate on
     * @param start Index to begin rewinding at
     * @param count Number of code points to rewind by
     * @return Resulting index
     */
    static int SkipCodePoints(const char* text, int start, int count);

protected:

    /**
     * Calculates textX and textY attributes
     */
    void CalcTextPos();

    /**
     * Calculates size of text
     * @param width Target to store width of text into
     * @param height Target to store height of text into
     */
    void CalcTextSize(int &width, int &height);

    /**
     * Fills 'letterSizes' array. Can be called only when vertical==true
     */
    void CalcLetterSizes();

    /// The text for this box.
    csString text;

    /// The colour of the text.
    int colour;

    /// Whether the text is currently being grayed out
    bool grayed;

    /// The current horizontal alignment for the text box
    pawsHorizAdjust horizAdjust;

    /// The current vertical alignment for the text box
    pawsVertAdjust  vertAdjust;

    /// Whether the text box is rendering vertically (up-down rather than left-right)
    bool vertical;

    /// X-Position of text inside the widget (relative to adjustment)
    int textX;

    /// X-Position of text inside the widget (relative to adjustment)
    int textY;

    /// Width of vertical column of letters (only used when \ref vertical is true)
    int textWidth;

    /// Array containing the size of each letter in text (only used when \ref vertical is true)
    psPoint* letterSizes;

};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsTextBox);



//--------------------------------------------------------------------------

#define MESSAGE_TEXTBOX_MOUSE_SCROLL_AMOUNT 3
/** This is a special type of text box that is used for messages.
 * This text box allows each 'message' to be stored as it's own line with
 * it's own colours.
 */
class pawsMessageTextBox : public pawsWidget
{
public:
    struct MessageSegment
    {
        int x;
        csString text;
        int colour;
        int size;

        MessageSegment() : x(0), text(""), colour(0), size(0) { }
    };
    struct MessageLine
    {
        csString text;
        int colour;
        int size;
        csArray<MessageSegment> segments;

        MessageLine()
        {
            text = "";
            colour = 0;
            size = 0;
        }
        MessageLine(const MessageLine &origin):text(origin.text),colour(origin.colour),size(origin.size)
        {
            for(unsigned int i = 0 ; i < origin.segments.GetSize(); i++)
                segments.Push(origin.segments[i]);
        }
    };

    pawsMessageTextBox();
    pawsMessageTextBox(const pawsMessageTextBox &origin);
    virtual ~pawsMessageTextBox();
    virtual bool Setup(iDocumentNode* node);
    virtual bool Setup(void);
    virtual bool PostSetup();

    /**
     * This function allows a widget to fill in its own contents from an xml node
     * supplied.  This is NOT the same as Setup, which is defining height, width,
     * styles, etc.  This is data.
     */
    bool SelfPopulate(iDocumentNode* node);

    virtual void Draw();

    /** Add a new message to this box.
     * @param data The new message to add.
     * @param colour The colour this message should be. -1 means use the default
     *               colour that is available.
     */
    void AddMessage(const char* data, int colour = -1);

    void AppendLastMessage(const char* data);
    void ReplaceLastMessage(const char* data);

    virtual void OnResize();
    virtual void Resize();
    virtual bool OnScroll(int direction, pawsScrollBar* widget);
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);

    void Clear();

    virtual int GetBorderStyle()
    {
        return BORDER_SUNKEN;
    }

    /** Resets scrollbar to starting position (offset=0) */
    void ResetScroll();

    /** Sets the scrollbar to 100% (offset=max) */
    void FullScroll();

    virtual void OnUpdateData(const char* dataname,PAWSData &data);

    bool OnKeyDown(utf32_char code, utf32_char key, int modifiers);


protected:
    /// Renders an entire message and returns the total lines it took.
    int RenderMessage(const char* data, int lineStart, int colour);

    /// List of messages this box has.
    csPDelArray<MessageLine> messages;

    //void AdjustMessages();

    void SplitMessage(const char* newText, int colour, int size, MessageLine* &msgLine, int &startPosition);

    /// Calculates value of the lineHeight attribute
    void CalcLineHeight();

    /**
     * checks for a child of type pawsScrollBar.
     * @return pawsScrollBar* pointer to child or NULL
     */
    pawsScrollBar* GetScrollBar();

    csPDelArray<MessageLine> adjusted;

    int lineHeight;
    size_t maxLines;

    int topLine;

    pawsScrollBar* scrollBar;
    int scrollBarWidth;

private:
    static const int INITOFFSET = 20;
    void WriteMessageLine(MessageLine* &msgLine, csString text, int colour);
    void WriteMessageSegment(MessageLine* &msgLine, csString text, int colour, int startPosition);
    csString FindStringThatFits(csString stringBuffer, int canDrawLength);
};

CREATE_PAWS_FACTORY(pawsMessageTextBox);

#define EDIT_TEXTBOX_MOUSE_SCROLL_AMOUNT 3
#define BLINK_TICKS  1000
/** An edit box widget/
 */
class pawsEditTextBox : public pawsWidget
{
public:
    pawsEditTextBox();
    virtual ~pawsEditTextBox();
    pawsEditTextBox(const pawsEditTextBox &origin);

    virtual void Draw();

    bool Setup(iDocumentNode* node);

    bool OnKeyDown(utf32_char code, utf32_char key, int modifiers);

    const char* GetText()
    {
        return text.GetDataSafe();
    }

    void SetMultiline(bool multi);
    void SetPassword(bool pass); //displays astrices instead of text

    /**
     * Change the text in the edit box.
     *
     * @param text The text that will replace whatever is currently there.
     * @param publish Publish the text.
     */
    void SetText(const char* text, bool publish = true);

    virtual void OnUpdateData(const char* dataname,PAWSData &data);

    /**
     * Sets size of the widget to fit the text that it displays.
     */
    void SetSizeByText();

    void Clear();

    virtual bool OnMouseDown(int button, int modifiers, int x, int y);

    /**
     * Called as a callback to a request for clipboard content if avaliabe.
     *
     * @note Only implemented for unix
     */
    virtual bool OnClipboard(const csString &content);

    virtual int GetBorderStyle()
    {
        return BORDER_SUNKEN;
    }

    /**
     * This function allows a widget to fill in its own contents from an xml node
     * supplied.  This is NOT the same as Setup, which is defining height, width,
     * styles, etc.  This is data.
     */
    bool SelfPopulate(iDocumentNode* node);

    /**
     * Set & Get top line funcs
     */
    unsigned int GetTopLine()
    {
        return topLine;
    }
    void SetTopLine(unsigned int newTopLine)
    {
        topLine = newTopLine;
    }

    void SetCursorPosition(size_t pos)
    {
        cursorPosition = pos;
    }

    virtual bool GetFocusOverridesControls() const
    {
        return true;
    }

    /**
     * Sets the max length for the text box
     * @param maxlen The value to set for the max length
     */
    void SetMaxLength(unsigned int maxlen);
    /**
     * Returns the max length for the text box
     * @return maxLen
     */
    inline unsigned int GetMaxLength() const
    {
        return maxLen;
    }
    /**
     * Gets the remaining characters that can be input
     * @return Remaining characters or UINT_MAX if no max length specified
     */
    inline unsigned int GetRemainingChars() const
    {
        if(maxLen)
            return (unsigned int)(maxLen - text.Length());
        return UINT_MAX;
    }
    /**
     * returns if the spellchecker is used in this widget
     */
    inline bool getSpellChecked() const
    {
        return spellChecked;
    };
    /**
     * set if the spellchecker should be used
     * @param check true if the spellchecker should be used (the spellchecker will still be only used if the plugin is available)
     */
    inline void setSpellChecked(const bool check)
    {
        spellChecked = check;
    };
    /**
     * toggels the current setting of the spellchecker
     */
    inline void toggleSpellChecked()
    {
        spellChecked = !spellChecked;
    };
    /** Method the get the current colour used for typos
     */
    unsigned int getTypoColour()
    {
        return typoColour;
    };
    /** Method to set the colour used for typos
     *  @param col The new colour to be used for typos
     */
    void setTypoColour(unsigned int col)
    {
        typoColour = col;
    };
protected:
    /** Helper method that does the actual spellchecking. Called from OnKeyDown
     */
    void checkSpelling();
    /** struct to contain the end boundry of a word and it's spellcheck status
     */
    struct Word
    {
        bool correct;
        int endPos;
    };

    bool password;
    csRef<iVirtualClock> clock;

    /// Position of first character that we display
    int start;

    /// The position of the cursor blink (in code units not points)
    size_t cursorPosition;
    unsigned int cursorLine;

    /// The current blink status
    bool blink;

    /// Keep track of ticks for flashing
    csTicks blinkTicks;

    /// Current input line
    csString text;

    bool multiLine;

    int lineHeight;
    size_t maxLines;

    // This value will always be one less than the number of used lines
    size_t usedLines;

    unsigned int topLine;
    /// The maximum length the text is allowed to be
    unsigned int maxLen;

    pawsScrollBar* vScrollBar;
    /** PS spellChecker plugin
     */
    csRef<iSpellChecker> spellChecker;
    /** Should the text be checked by the spellchecker plugin (if available)
     */
    bool spellChecked;
    /** Colour used for typos
     */
    unsigned int typoColour;
    /** Array that contains the boundries of the words and if the spellchecker recognizes them or not
     */
    csArray<Word> words;
};

CREATE_PAWS_FACTORY(pawsEditTextBox);


//---------------------------------------------------------------------------------------


#define MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT 3
class pawsMultiLineTextBox : public pawsWidget
{
public:
    pawsMultiLineTextBox();
    pawsMultiLineTextBox(const pawsMultiLineTextBox &origin);
    virtual ~pawsMultiLineTextBox();

    bool Setup(iDocumentNode* node);
    bool PostSetup();

    void Draw();

    void SetText(const char* text);
    const char* GetText()
    {
        return text;
    }
    void Resize();
    bool OnScroll(int direction, pawsScrollBar* widget);
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);

    // Normal text boxes should not be focused.
    virtual bool OnGainFocus(bool /*notifyParent*/ = true)
    {
        return false;
    }
    virtual void OnUpdateData(const char* dataname,PAWSData &data);

protected:

    void OrganizeText(const char* text);

    /**
     * checks for a child of type pawsScrollBar.
     * @return pawsScrollBar* pointer to child or NULL
     */
    pawsScrollBar* GetScrollBar();

    /// The text for this box.
    csString text;
    /// The text, broken into separate lines by OrganizeText()
    csArray<csString> lines;

    //where our scrollbar at?
    size_t startLine;
    //Number of lines that we can fit in the box
    size_t canDrawLines;
    bool usingScrollBar;
    int maxWidth;
    int maxHeight;
    pawsScrollBar* scrollBar;
};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsMultiLineTextBox);


class pawsMultiPageTextBox : public pawsWidget
{
public:
    pawsMultiPageTextBox();
    pawsMultiPageTextBox(const pawsMultiPageTextBox &origin);
    virtual ~pawsMultiPageTextBox();

    bool Setup(iDocumentNode* node);
    bool PostSetup();




    void Draw();

    void SetText(const char* text);
    const char* GetText()
    {
        return text;
    }
    void Resize();
    bool OnScroll(int direction, pawsScrollBar* widget);

    virtual bool OnMouseDown(int button, int modifiers, int x, int y);

    // Normal text boxes should not be focused.
    virtual bool OnGainFocus(bool /*notifyParent*/ = true)
    {
        return false;
    }
    virtual void OnUpdateData(const char* dataname,PAWSData &data);

    // Added these member functions for dealing with pages
    int GetNumPages();
    void SetCurrentPageNum(int n);
    int GetCurrentPageNum();
protected:

    // splits the given text in separate lines filling the 'lines' variable
    void OrganizeText(const char* text);

    /**
     * checks for a child of type pawsScrollBar.
     * @return pawsScrollBar* pointer to child or NULL
     */
    pawsScrollBar* GetScrollBar();


    /// The text for this box.
    csString text;
    /// The text, broken into separate lines by OrganizeText()
    csArray<csString> lines;

    //where our scrollbar at?and that d
    size_t startLine;
    //Number of lines that we can fit in the box
    size_t canDrawLines;
    bool usingScrollBar;
    int maxWidth;
    int maxHeight;
    pawsScrollBar* scrollBar;
    int currentPageNum;
    int numPages;
};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsMultiPageTextBox);


/* An edit box widget */
class pawsMultilineEditTextBox : public pawsWidget
{
public:
    pawsMultilineEditTextBox();
    virtual ~pawsMultilineEditTextBox();
    pawsMultilineEditTextBox(const pawsMultilineEditTextBox &origin);
    struct MessageLine
    {
        size_t lineLength; //Stores length of the line.
        size_t breakLine; //Stores real text position of where break occurs.
        int lineExtra; //Stores 1 if line ends with a \n, or 0 if it does not.
    };

    bool Setup(iDocumentNode* node);

    virtual void Draw();
    void DrawWidgetText(const char* text, size_t x, size_t y, int style, int fg, int visLine);

    /**
     * Change the text in the edit box.
     *
     * @param text The text that will replace whatever is currently there.
     * @param publish Publish the text.
     */
    void SetText(const char* text, bool publish = true);
    const char* GetText();
    void UpdateScrollBar();
    virtual void SetupScrollBar();

    bool OnScroll(int direction, pawsScrollBar* widget);
    virtual void OnResize();
    virtual bool OnMouseUp(int button, int modifiers, int x, int y);
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    /** Called as a callback to a request for clipboard content if avaliabe.
     *
     * @note Only implemented for unix
     */
    virtual bool OnClipboard(const csString &content);
    virtual void OnUpdateData(const char* dataname,PAWSData &data);
    virtual bool OnKeyDown(utf32_char code, utf32_char key, int modifiers);
    virtual void CalcMouseClick(int x, int y, size_t &cursorLine, size_t &cursorChar);

    virtual bool GetFocusOverridesControls() const
    {
        return true;
    }

    void PushLineInfo(size_t lineLength, size_t lineBreak, int lineExtra);

    virtual int GetBorderStyle()
    {
        return BORDER_SUNKEN;
    }
    void LayoutText();

    /**
     * This function allows a widget to fill in its own contents from an xml node
     * supplied.  This is NOT the same as Setup, which is defining height, width,
     * styles, etc.  This is data.
     */
    bool SelfPopulate(iDocumentNode* node);

    /**
     * Set & Get top line funcs
     */
    size_t GetTopLine()
    {
        return topLine;
    }
    void SetTopLine(size_t newTopLine)
    {
        topLine = newTopLine;
    }
    size_t GetCursorPosition();
    size_t GetCursorPosition(size_t destLine, size_t destCursor);
    void SetCursorPosition(size_t pos)
    {
        cursorPosition = pos;
    }
    void Clear()
    {
        SetText("");
    }
    csString GetLine(size_t line);
    int GetLineWidth(int lineNumber);
    void GetLinePos(size_t lineNumber, size_t &start, size_t &end);
    void GetLineRelative(size_t pos, size_t &start, size_t &end);
    bool EndsWithNewLine(int lineNumber);
    void GetCursorLocation(size_t pos, size_t &destLine, size_t &destCursor);
    char GetAt(size_t destLine, size_t destCursor);

    /**
     * Sets the max length for the text box
     * @param maxlen The value to set for the max length
     */
    void SetMaxLength(unsigned int maxlen);
    /**
     * Returns the max length for the text box
     * @return maxLen
     */
    inline unsigned int GetMaxLength() const
    {
        return maxLen;
    }
    /**
     * Gets the remaining characters that can be input
     * @return Remaining characters or UINT_MAX if no max length specified
     */
    inline unsigned int GetRemainingChars() const
    {
        if(maxLen)
            return (unsigned int)(maxLen - text.Length());
        return UINT_MAX;
    }
    /**
     * returns if the spellchecker is used in this widget
     */
    inline bool getSpellChecked() const
    {
        return spellChecked;
    };
    /**
     * set if the spellchecker should be used
     * @param check true if the spellchecker should be used (the spellchecker will still be only used if the plugin is available)
     */
    inline void setSpellChecked(const bool check)
    {
        spellChecked = check;
    };
    /**
     * toggels the current setting of the spellchecker
     */
    inline void toggleSpellChecked()
    {
        spellChecked = !spellChecked;
    };
    /** Method the get the current colour used for typos
     */
    unsigned int getTypoColour()
    {
        return typoColour;
    };
    /** Method to set the colour used for typos
     *  @param col The new colour to be used for typos
     */
    void setTypoColour(unsigned int col)
    {
        typoColour = col;
    };

    int GetLinesPerPage()
    {
        return canDrawLines;
    }
protected:
    /** Helper method that does the actual spellchecking. Only checks the currently
     * visible text. Called from OnKeyDown and LayoutText
     */
    void checkSpelling();
    /** Helper method for printing text if the spellchecker is enabled
     * @param font The font used for the text. Needed to determine the width in pixel of words
     * @param x X-Position where the text should be printed
     * @param y Y-Position where the text should be printed
     * @param fg The color of the text. This is only used for correct words. Typos use the typeColour (but the alpha from this color)
     * @param str The text
     * @param visLine the number (counting from top=0) of the current visual line. Needed for the spellchecker to choose the correct word-boundries from lineTypos
     */
    void drawTextSpellChecked(iFont* font, int x, int y, int fg, csString str, int visLine);


    /** struct to contain the end boundry of a word and it's spellcheck status
     */
    struct Word
    {
        bool correct;
        int endPos;
    };

    csRef<iVirtualClock> clock;

    // The position of the cursor blink
    size_t cursorPosition;

    // The position of the cursor in an array (Used by draw method).
    size_t cursorLine;
    size_t cursorLoc;

    /// The current blink status
    bool blink;

    /// Keep track of ticks for flashing
    csTicks blinkTicks;

    /// Concatenated contents of lines, only valid after GetText()
    csString text;
    csPDelArray<MessageLine> lineInfo;
    int lineHeight;

    // Width of the vertical scroll bar
    int vScrollBarWidth;
    size_t canDrawLines;

    size_t topLine;
    pawsScrollBar* vScrollBar;

    bool usingScrollBar;

    int maxWidth;
    int maxHeight;

    int yPos;
    csString tmp;

    /// The maximum length the text is allowed to be
    unsigned int maxLen;
    /* PS spellChecker plugin
     */
    csRef<iSpellChecker> spellChecker;
    /* Should the text be checked by the spellchecker plugin (if available)
     */
    bool spellChecked;
    /** Colour used for typos
     */
    unsigned int typoColour;

    csArray<csArray<Word> > lineTypos;
};

CREATE_PAWS_FACTORY(pawsMultilineEditTextBox);



/// pawsFadingTextBox - Used for mainly on screen messages, deletes itself upon 100% fade
#define FADE_TIME              1000 // Time for the onscreen message to fade away
#define MESG_TIME              3000 // Time for the onscreen message to be shown ( not including fading)

class pawsFadingTextBox : public pawsWidget
{
public:
    pawsFadingTextBox();
    virtual ~pawsFadingTextBox() {};
    pawsFadingTextBox(const pawsFadingTextBox &origin);
    bool Setup(iDocumentNode* /*node*/)
    {
        return true;   // Shouldn't be created in XML, only by code
    }

    void Draw();

    void Fade(); // Force fading
    void SetText(const char* newtext, iFont* font1,iFont* font2, int color,int time=MESG_TIME,int fadetime=FADE_TIME);

    void GetSize(int &w, int &h);
private:
    void DrawBorderText(const char* text, iFont* font,int x);

    csString text;
    csString first;

    csRef<iFont> firstFont; // used for the first letter
    csRef<iFont> font;      // used for the rest

    int org_color;
    int color;
    int scolor;
    csTicks start;

    int ymod;

    int time,fadetime;
};

CREATE_PAWS_FACTORY(pawsFadingTextBox);
///PictureInfo hold the information of pictures that would be display in the same row with the same format
struct PictureInfo
{
    unsigned int width; ///< Width of pictures
    unsigned int height;///< Height of pictures
    csString srcString;///< The file or source info for pictures
    unsigned int align;///< Align of the picture in
    unsigned int padding[4];///< The space around pictures
};
class pawsDocumentView: public pawsMultiLineTextBox
{
public:
    pawsDocumentView();
    pawsDocumentView(const pawsDocumentView &origin);
    virtual ~pawsDocumentView();
    /**
     * Set the content that would be displayed in the view.
     *
     * Set the content that would be displayed in the view.
     * The text should be in form of xml style as following:
     * \<Contents\>
     *	 \<Content type="pic" align="0" padding="5 5 5 5" width="32" height="32" src="ButtonSpeak;/paws/real_skin/mouse.png;ButtonOpen;"></Content\>
     *	 \<Content type="text"\>text content\</Content\>
     *	 \<Content type="pic" align="2" padding="5 5 5 5" width="32" height="32" src="ButtonConstruct;/paws/real_skin/mouse.png;ButtonBanking;"\>\</Content\>
     * \</Contents\>
     * @param text The xml content defined what to be display in the view
     */
    void SetText(const char* text);
    /**
     * Draw the content include text and pictures.
     */
    void Draw();
    void Resize();
protected:
    /**
     * Organize the content to display in the view.
     *
     * Organize the content to display in the view. Text should be in the form of xml style
     *
     * @param text The xml content defined what to be display in the view
     */
    void OrganizeContent(const char* text);

    /**
     * Process the \<content type="pic"\>\</content\> node in the xml
     *
     * Process the \<content type="pic"\>\</content\> node in the xml
     *
     * @param node A \<content type="pic"\>\</content\> node
     * @return How many pictures are defined to be displayed in a row in this node
     */
    unsigned int ProcessPictureInfo(iDocumentNode* node);

    csArray<PictureInfo> picsInfo;///< Hold all the info parse from xml document
};
CREATE_PAWS_FACTORY(pawsDocumentView);


class pawsMultiPageDocumentView: public pawsMultiPageTextBox
{
public:
    pawsMultiPageDocumentView();
    pawsMultiPageDocumentView(const pawsMultiPageDocumentView &origin);
    virtual ~pawsMultiPageDocumentView();
    /**
     * Set the content that would be displayed in the view.
     *
     * Set the content that would be displayed in the view.
     * The text should be in form of xml style as following:
     * <pre>
     * \<Contents\>
     *	 \<Content type="pic" align="0" padding="5 5 5 5" width="32" height="32" src="ButtonSpeak;/paws/real_skin/mouse.png;ButtonOpen;"\>\</Content\>
     *	 \<Content type="text"\>text content\</Content\>
     *	 \<Content type="pic" align="2" padding="5 5 5 5" width="32" height="32" src="ButtonConstruct;/paws/real_skin/mouse.png;ButtonBanking;"\></Content\>
     * \</Contents\></pre>
     * @param text The xml content defined what to be display in the view
     */
    void SetText(const char* text);

    /**
     * Draw the content include text and pictures.
     */
    void Draw();

    void Resize();
protected:
    /**
     * Organize the content to display in the view.
     *
     * Organize the content to display in the view. Text should be in the form of xml style
     *
     * @param text The xml content defined what to be display in the view
     */
    void OrganizeContent(const char* text);

    /**
     * Process the \<content type="pic"\>\</content\> node in the xml
     *
     * Process the \<content type="pic"\>\</content\> node in the xml
     *
     * @param node A \<content type="pic"\>\</content\> node
     * @return How many pictures are defined to be displayed in a row in this node
     */
    unsigned int ProcessPictureInfo(iDocumentNode* node);
    csArray<PictureInfo> picsInfo;///< Hold all the info parse from xml document
};
CREATE_PAWS_FACTORY(pawsMultiPageDocumentView);

/** @} */

#endif

