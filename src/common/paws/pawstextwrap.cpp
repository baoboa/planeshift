/*
 * pawstextwrap.cpp
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <ctype.h>
#include <ivideo/fontserv.h>
#include <iutil/virtclk.h>
#include <iutil/evdefs.h>
#include <iutil/event.h>
#include <iutil/plugin.h>

#include "pawstextbox.h"
#include "pawsmanager.h"
#include "pawsprefmanager.h"
#include "pawscrollbar.h"
#include "util/log.h"
#include "util/strutil.h"
#include "util/psstring.h"

#define EDIT_TEXTBOX_MOUSE_SCROLL_AMOUNT 3

#define VSCROLLBAR_WIDTH 24

/*
   In order for this control to be implemented correctly, it must pass the following tests:
     1. Takes the last word after a space on the line and moves it to the next line. (wordwrap)
     2. Chops the end of the line and continues on the next line if no space is found. (breakline)
     3. The line separator(OS dependant - \n for our purposes) has adjustments made for it. (line separator)
     4. Combinations of the above must also be considered, such as:
           a. After a breakline occurs, the text will still wrap to the next line.
           b. Before a breakline occurs, the line starts from being wordwrapped on the previous line.
           c. A newline must work from either a line containing a wordwrap or a breakline.
     5. Multiple blank lines must be allowed.
     6. An control with no text must be considered.
     7. Cursor positioning must be considered for all of the above tests.
     8. The vertical scrollbar must be considered for all of the above tests.
     9. Delete, Backspace and insert text must work in the beginning, middle,
          and end of the text.
    10. Font support by control
*/
pawsMultilineEditTextBox::pawsMultilineEditTextBox() : cursorPosition(0),
    cursorLine(0),
    blink(true),
    vScrollBarWidth(VSCROLLBAR_WIDTH),
    topLine(0),
    vScrollBar(NULL),
    maxLen(0),
    spellChecked(false),
    typoColour(0xFF0000)
{
    factory = "pawsMultilineEditTextBox";
    clock = csQueryRegistry<iVirtualClock > (PawsManager::GetSingleton().GetObjectRegistry());

    blinkTicks = clock->GetCurrentTicks();

    // Find the line height;
    int dummy;
    GetFont()->GetMaxSize(dummy, lineHeight);
    OnResize();

    // get the spellchecker plugin
    spellChecker = csQueryRegistryOrLoad<iSpellChecker>(PawsManager::GetSingleton().GetObjectRegistry(), "crystalspace.planeshift.spellchecker");
}

pawsMultilineEditTextBox::pawsMultilineEditTextBox(const pawsMultilineEditTextBox &origin)
    :pawsWidget(origin),
     clock(origin.clock),
     cursorPosition(origin.cursorPosition),
     cursorLine(origin.cursorLine),
     cursorLoc(origin.cursorLoc),
     blink(origin.blink),
     blinkTicks(origin.blinkTicks),
     text(origin.text),
     lineHeight(origin.lineHeight),
     vScrollBarWidth(origin.vScrollBarWidth),
     canDrawLines(origin.canDrawLines),
     topLine(origin.topLine),
     usingScrollBar(origin.usingScrollBar),
     maxWidth(origin.maxWidth),
     maxHeight(origin.maxHeight),
     yPos(origin.yPos),
     tmp(origin.tmp),
     maxLen(origin.maxLen),
     spellChecker(origin.spellChecker),
     spellChecked(origin.spellChecked),
     typoColour(origin.typoColour),
     lineTypos(origin.lineTypos)
{
    for(unsigned int i= 0 ; i < origin.lineInfo.GetSize(); i++)
        lineInfo.Push(new MessageLine(*origin.lineInfo[i]));
    vScrollBar = 0;
    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.vScrollBar == origin.children[i])
            vScrollBar = dynamic_cast<pawsScrollBar*>(children[i]);

        if(vScrollBar!=0)
            break;
    }
}
pawsMultilineEditTextBox::~pawsMultilineEditTextBox()
{

}


bool pawsMultilineEditTextBox::OnKeyDown(utf32_char code, utf32_char key, int modifiers)
{
    bool changed = false;
    size_t position = 0;
    bool repositionCursor = false;

    blink = true;

    switch(key)
    {
        case CSKEY_DEL:
        {
            position = GetCursorPosition(cursorLine, cursorLoc);
            if(text.Length() > 0 && position < text.Length())
            {
                text.DeleteAt(position, csUnicodeTransform::UTF8Skip((const utf8_char*)text.GetData() + position, text.Length() - position));
                changed = true;
            }
            break;
        }
        case CSKEY_BACKSPACE:
        {
            position = GetCursorPosition(cursorLine, cursorLoc);
            if(text.Length() > 0 && position > 0)//&& position <=text.Length())
            {
                position -= csUnicodeTransform::UTF8Rewind((const utf8_char*)text.GetData() + position, position);
                text.DeleteAt(position, csUnicodeTransform::UTF8Skip((const utf8_char*)text.GetData() + position, text.Length() - position));
                repositionCursor = true;
                changed = true;
            }
            break;
        }
        case CSKEY_LEFT:
            position = GetCursorPosition(cursorLine, cursorLoc);
            if(cursorLoc > 0)
                cursorLoc -= csUnicodeTransform::UTF8Rewind((const utf8_char*)text.GetData() + position, position);
            else if(cursorLine > 0)
            {
                cursorLine--;
                cursorLoc = lineInfo[cursorLine]->lineLength - 1;
                break;
            }
            break;
        case CSKEY_RIGHT:
            position = GetCursorPosition(cursorLine, cursorLoc);
            if(cursorLoc < lineInfo[cursorLine]->lineLength-lineInfo[cursorLine]->lineExtra)
            {
                cursorLoc += csUnicodeTransform::UTF8Skip((const utf8_char*)text.GetData() + position, text.Length() - position);
            }
            else if(cursorLine < lineInfo.GetSize()-1)
            {
                cursorLine++;
                cursorLoc = 0;
            }
            else if(cursorLine == lineInfo.GetSize() - 1 && cursorLoc == lineInfo[cursorLine]->lineLength - 1)
            {
                //the case for the very last line, we allow to go to after the last character
                cursorLoc++;
            }
            break;
        case CSKEY_UP:
        {
            if(cursorLine > 0)
                cursorLine--;
            if(cursorLoc > lineInfo[cursorLine]->lineLength - lineInfo[cursorLine]->lineExtra)
                cursorLoc = lineInfo[cursorLine]->lineLength - lineInfo[cursorLine]->lineExtra;
            break;
        }
        case CSKEY_DOWN:
        {
            if(cursorLine < lineInfo.GetSize() - 1)
                cursorLine++;
            if(cursorLoc > lineInfo[cursorLine]->lineLength - lineInfo[cursorLine]->lineExtra)
                cursorLoc = lineInfo[cursorLine]->lineLength - lineInfo[cursorLine]->lineExtra;
            break;
        }
        case CSKEY_END:
        {
            cursorLoc = lineInfo[cursorLine]->lineLength-lineInfo[cursorLine]->lineExtra;
            break;
        }
        case CSKEY_HOME:
        {
            cursorLoc = 0;
            break;
        }
        default:
            if(CSKEY_IS_SPECIAL(key))
                break;

            // Ignore ASCII control characters
            if(key < 128 && !isprint(key) && key != CSKEY_ENTER)
                break;
            if(maxLen && text.Length() == maxLen)
                break;

            //Lookup the position.
            position = GetCursorPosition(cursorLine, cursorLoc);
            if(key == CSKEY_ENTER)
            {
                //our text flow algorithm will then break this into two lines.
                text.Insert(position, '\n');
                cursorLine++;
                cursorLoc = 0;
            }
            else
            {
                utf8_char utf8Char[5];

                int charLen = csUnicodeTransform::UTF32to8(utf8Char, 5, &key, 1);
                text.Insert(position, (char*)utf8Char);
                if(maxLen)
                    text.Truncate(maxLen);
                repositionCursor = true;
                position += charLen - 1;
            }

            changed = true;
    }

    if(changed)
    {
        //Re-flow the text
        LayoutText();

        if(repositionCursor)
            GetCursorLocation(position, cursorLine, cursorLoc);

        if(subscribedVar)
            PawsManager::GetSingleton().Publish(subscribedVar, text);
        parent->OnChange(this);
    }

    if(cursorLine >= topLine + canDrawLines)
        topLine = cursorLine - canDrawLines + 1;
    else if(cursorLine < topLine)
        topLine = cursorLine;
    SetupScrollBar();

    if(!CSKEY_IS_SPECIAL(key) && (key > 128 || isprint(key)))
        return true;

    if(code != CSKEY_ENTER)
    {
        pawsWidget::OnKeyDown(code, key, modifiers);
    }
    return true;
}

void pawsMultilineEditTextBox::OnResize()
{
    canDrawLines = (screenFrame.Height() / lineHeight);
    LayoutText();
    UpdateScrollBar();
}

void pawsMultilineEditTextBox::UpdateScrollBar()
{
    if(!vScrollBar)
        return;

    float barMaxValue; //holds the maximum value of the scrollbar
    float barPosition; //holds the current position of the scrollbar

    if(lineInfo.GetSize() <= canDrawLines) //can all the lines stay on screen?
    {
        vScrollBar->Hide(); //... yes they can then hide the scrollbar and...
        barMaxValue = 0; //set the scrollbar max value and position to zero as we have nothing to scroll
        barPosition = 0;
    }
    else //no they can't so...
    {
        vScrollBar->Show();//... show the scrollbar and...
        barMaxValue = (int)lineInfo.GetSize() - canDrawLines; //... set it's max values and position
        barPosition = (float)topLine;
    }

    //set new values to the scrollbar
    vScrollBar->SetMaxValue(barMaxValue);
    vScrollBar->SetCurrentValue(barPosition);
}

void pawsMultilineEditTextBox::SetupScrollBar()
{
    if(!vScrollBar)
    {
        vScrollBar = new pawsScrollBar;
        vScrollBar->SetParent(this);
        vScrollBar->SetRelativeFrame(screenFrame.Width() - vScrollBarWidth,
                                     0, vScrollBarWidth, screenFrame.Height() - 12);
        vScrollBar->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT);
        vScrollBar->PostSetup();
        vScrollBar->SetTickValue(1.0);
        AddChild(vScrollBar);
    }

    UpdateScrollBar();
}

bool pawsMultilineEditTextBox::OnMouseDown(int button, int modifiers, int x, int y)
{
    if(button == csmbLeft)
    {
        return true;
    }
    else if(button == csmbWheelUp)
    {
        if(vScrollBar)
            vScrollBar->SetCurrentValue(vScrollBar->GetCurrentValue() - EDIT_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if(button == csmbWheelDown)
    {
        if(vScrollBar)
            vScrollBar->SetCurrentValue(vScrollBar->GetCurrentValue() + EDIT_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }



#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
    if(button == csmbMiddle)
    {
        // Only included for platforms using Middle button to past.

        // Request Clipboard. Reuslt will be available in OnClipboard
        PawsManager::GetSingleton().RequestClipboardContent();

        return true;
    }
#endif


    return pawsWidget::OnMouseDown(button, modifiers, x ,y);
}

bool pawsMultilineEditTextBox::OnClipboard(const csString &content)
{
    Debug2(LOG_PAWS, 0, "Received from clipboard: %s", content.GetDataSafe());

    size_t position = GetCursorPosition(cursorLine, cursorLoc);
    if(position >= text.Length())
    {
        text.Append(content);
    }
    else
    {
        text.Insert(position, content);
    }

    if(maxLen)
        text.Truncate(maxLen);
    position += content.Length();

    LayoutText();

    GetCursorLocation(position, cursorLine, cursorLoc);
    if(cursorLine >= topLine + canDrawLines)
        topLine = cursorLine - canDrawLines + 1;
    else if(cursorLine < topLine)
        topLine = cursorLine;
    SetupScrollBar();

    if(subscribedVar)
    {
        PawsManager::GetSingleton().Publish(subscribedVar, text);
    }

    parent->OnChange(this);

    return true;
}

bool pawsMultilineEditTextBox::OnMouseUp(int button, int modifiers, int x, int y)
{
    if(button == csmbLeft)
    {
        size_t endLine = 0;
        size_t endChar = 0;
        CalcMouseClick(x, y, endLine, endChar);

        //Set the cursor location.
        cursorLine = (unsigned int) endLine;
        cursorLoc = endChar;
        cursorPosition = GetCursorPosition();

        return true;
    }
    return pawsWidget::OnMouseUp(button, modifiers, x ,y);
}

void pawsMultilineEditTextBox::CalcMouseClick(int x, int y, size_t &cursorLine, size_t &cursorChar)
{
    // Adjust x to be relative to the text box
    x -= screenFrame.xmin;

    //Force the cursor to blink
    blink = true;

    // Find the line the mouse is on and move the cursor there
    cursorLine = (y - screenFrame.ymin) / lineHeight + topLine;

    //Determine if the click was after the text area
    if(cursorLine >= lineInfo.GetSize() && lineInfo.GetSize() > 0)
    {
        cursorLine = (unsigned int) lineInfo.GetSize() - 1;
        //*after* the last letter of last line
        cursorChar = lineInfo[(int)lineInfo.GetSize() - 1]->lineLength;
        return;
    }
    // Determine if the click was before text area
    if(x <= 0 || lineInfo[cursorLine]->lineLength <= 0)
    {
        cursorChar = 0;
        return;
    }

    //Get line width
    int width = 0;
    int height = 0;
    csString line = GetLine(cursorLine);
    GetFont()->GetDimensions(line, width, height);
    //Determine if the click was at the end of a line...
    if(x >= width)
    {
        if(line.GetAt(lineInfo[cursorLine]->lineLength - 1) == '\n')
        {
            //if it was, make sure to ignore the newline character.
            cursorChar = lineInfo[cursorLine]->lineLength-1;
            return;
        }
    }
    //should position cursor before clicked letter
    cursorChar = GetFont()->GetLength(GetLine(cursorLine), x);
}


void pawsMultilineEditTextBox::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    // This is called automatically whenever subscribed data is published.
    SetText(value.GetStr(), false);
}

bool pawsMultilineEditTextBox::OnScroll(int /*direction*/, pawsScrollBar* widget)
{
    topLine = (int)widget->GetCurrentValue();
    // if spellchecking is enabled we need to check now as the visible part of the text might have changed
    if(spellChecked)
    {
        checkSpelling();
    }
    return true;
}
void pawsMultilineEditTextBox::PushLineInfo(size_t lineLength, size_t lineBreak, int lineExtra)
{
    MessageLine* msg = new MessageLine;
    msg->lineLength = lineLength;
    msg->breakLine = lineBreak;
    msg->lineExtra = lineExtra;
    lineInfo.Push(msg);
}

void pawsMultilineEditTextBox::LayoutText()
{
    // we need to font for calculation the pixel length of words
    csRef<iFont> font = GetFont();
    // and of course we need the size we have available in the widget
    int screenWidth = screenFrame.Width() - vScrollBarWidth;

    int width = 0;          // will hold the width of the current word
    int height = 0;         // will hold the height of any examined string...not really needed for anything.
    csString tempString = "";    // content of the current line before the currently examined word
    csString word;          // the word to examine
    size_t  srcPos = 0;     // the first char that hasn't yet been grabbed.

    // we start with an empty array of lines and build it from scratch from the text
    lineInfo.Empty();
    size_t totalCount = 0;
    int tailWidth = 0;      // the length in pixel of everything on the current line before the currenly examined word.

    // lets process the whole text
    while(srcPos < text.Length())
    {
        //try to grab a word.
        size_t a = text.FindFirst(" \t\n", srcPos);
        //if a is '-1', the char wasn't found so we grab to end of the text,
        //else we grab up to and including the found character
        if(a == (size_t)-1)
            word = text.Slice(srcPos,text.Length() - srcPos);
        else
            word = text.Slice(srcPos, a - srcPos + 1);

        //Gets dimensions of current word.
        font->GetDimensions(word, width, height);

        //if it'll fit..
        if(width + tailWidth <= screenWidth)
        {
            // seems the end of the current line is not reached yet
            //so attach the word
            tempString.Append(word);
            // and set tailWidth to the width of everything we have so far
            font->GetDimensions(tempString, tailWidth, height);
            // and of course we don't forget to move foreward in the text
            srcPos += word.Length();
        }
        // okay..if we get here we know already the word won't fit in the line anymore
        else if(tempString.FindFirst(" ") != (size_t)-1)
        {
            // but great...we have a whitespace in the current line already...so let's just put this word in the next line.
            //If wordwrap->linebreak on same line,
            // push linebreak to next line
            totalCount += tempString.Length();
            // the line we have so far can be saved
            PushLineInfo(tempString.Length(), totalCount, 1);
            // and we start a new line
            tempString.Clear();
            word = "a"; //set position holder (never actually drawn) (needed for the newline check later)
            font->GetDimensions(tempString, tailWidth, height);
            //Note: srcPos is not set. So next time the currently processed word is either added to the line if it fits
            //...or breaken into smaller parts...see next else
        }
        /*else if(width > screenWidth)
        {
            printf("  ----test---- word must go on the next line\n");
            //Wordwrap
            totalCount += tempString.Length();
            PushLineInfo(tempString.Length(), totalCount, 1);
            tempString = word;
            font->GetDimensions(tempString, tailWidth, height);
            srcPos += word.Length();
        }*/ // seriously...I have not the slightest clue what this should do or did in the past...but at the moment it causes problems
        else
        {
            //breakline - the case where we have a single
            //unbroken word wider than our page

            // how many of the chars of this word will fit in one line?
            int maxChars = font->GetLength(word, screenWidth);
            word = word.Slice(0, maxChars);             //Get string for the line
            totalCount += word.Length();                //Get absolute char position in text for line break
            PushLineInfo(word.Length(), totalCount, 0); //Push the line information to the stack
            //Set srcPosition for this loop, please notice this is the length of the cut word...not the whole word anymore
            //the rest of the word will be processed in the next while-cyle
            srcPos += word.Length();
            //Note: Tempstring is not set here.
            font->GetDimensions(tempString,tailWidth,height);
        }
        //does our word end in a newline?
        if(word[word.Length()-1] == '\n')
        {
            //standard 'create a new destination line' procedure
            totalCount += tempString.Length();
            PushLineInfo(tempString.Length(),totalCount,1);
            tempString.Clear();
            font->GetDimensions(tempString,tailWidth,height);
        }
    }

    //Make sure to add the last line if it exists.
    totalCount += tempString.Length();
    PushLineInfo(tempString.Length(), totalCount, 0);

    // text layout changed...so checking spelling
    if(spellChecked)
    {
        checkSpelling();
    }
}

//our draw is based on static, wrapped widget
void pawsMultilineEditTextBox::Draw()
{
    yPos = 0;

    if(clock->GetCurrentTicks() - blinkTicks > BLINK_TICKS)
    {
        blink = !blink;
        blinkTicks = clock->GetCurrentTicks();
    }

    pawsWidget::Draw();
    pawsWidget::ClipToParent(false);

    for(size_t line = topLine; line <= (topLine + canDrawLines); line++)
    {
        if(line >= lineInfo.GetSize())
            break;
        //chomp newline characters
        if(lineInfo[line]->lineLength > 0)
        {
            tmp = GetLine(line).Slice(0, lineInfo[line]->lineLength - lineInfo[line]->lineExtra);
            DrawWidgetText(tmp, (int)screenFrame.xmin, (int)(screenFrame.ymin + yPos * lineHeight), -1, GetFontColour(), line-topLine);
            yPos++;
        }
    }

    if(blink && hasFocus) // Draw the cursor
    {
        int width, height;
        GetFont()->GetDimensions(GetLine(cursorLine).Slice(0, cursorLoc).GetDataSafe(), width, height);
        size_t realHeight = lineHeight * (cursorLine - topLine);

        graphics2D->DrawLine((float)(screenFrame.xmin + width + 1),
                             (float)(screenFrame.ymin + realHeight),
                             (float)(screenFrame.xmin + width + 1),
                             (float)(screenFrame.ymin + realHeight + lineHeight),
                             GetFontColour());
    }

}

void pawsMultilineEditTextBox::DrawWidgetText(const char* text, size_t x, size_t y, int style, int fg, int visLine)
{
    csRef<iFont> font = GetFont();
    if(style == -1)
        style = GetFontStyle();

    if(fg == -1)
        fg = GetFontColour();

    if(parent && !parent->GetBackground().IsEmpty() && parent->isFadeEnabled() && parent->GetMaxAlpha() != parent->GetMinAlpha())
    {
        int a = (int)(255 - (parent->GetMinAlpha() + (parent->GetMaxAlpha() - parent->GetMinAlpha()) * parent->GetFadeVal() * 0.010));
        int r, g, b;

        if(style & FONT_STYLE_DROPSHADOW)
        {
            graphics2D->GetRGB(GetFontShadowColour(), r, g, b);
            if(spellChecked && spellChecker)
            {
                drawTextSpellChecked(font, (int) x + 2, (int) y + 2, graphics2D->FindRGB(r, g, b, a), text, visLine);
            }
            else
            {
                graphics2D->Write(font, (int) x + 2, (int) y + 2, graphics2D->FindRGB(r, g, b, a), -1, text);
            }
        }
        graphics2D->GetRGB(GetFontColour(), r, g, b);
        // if spellchecking is enabled we need to use drawTextSpellChecked for the different colours
        if(spellChecked && spellChecker)
        {
            drawTextSpellChecked(font, (int) x, (int) y, graphics2D->FindRGB(r, g, b, a), text, visLine);
        }
        else
        {
            graphics2D->Write(font, (int) x, (int) y, graphics2D->FindRGB(r, g, b, a), -1, text);
        }
    }
    else
    {
        if(style & FONT_STYLE_DROPSHADOW)
        {
            if(spellChecked && spellChecker)
            {
                drawTextSpellChecked(font, (int) x+2, (int) y+2, GetFontShadowColour(), text, visLine);
            }
            else
            {
                graphics2D->Write(font, (int) x+2, (int) y+2, GetFontShadowColour(), -1, text);
            }
        }
        // if spellchecking is enabled we need to use drawTextSpellChecked for the different colours
        if(spellChecked && spellChecker)
        {
            drawTextSpellChecked(font, (int) x, (int) y, fg, text, visLine);
        }
        else
        {
            graphics2D->Write(font, (int) x, (int) y, fg, -1, text);
        }
    }
}

void pawsMultilineEditTextBox::drawTextSpellChecked(iFont* font, int x, int y, int fg, csString str, int visLine)
{
    int wordStart = 0;
    for(size_t i = 0; i < lineTypos[visLine].GetSize(); i++)
    {
        // first we find out what colour we need for this word
        int color;
        if(lineTypos[visLine][i].correct)
        {
            // no type, just use the colour I got
            color = fg;
        }
        else
        {
            //typo...using the alpha of the colour I got with the typoColour
            int r, g, b, a;
            graphics2D->GetRGB(fg, r, g, b, a);
            // overwriting r, g, b with typoColour
            graphics2D->GetRGB(typoColour, r, g, b);
            color = graphics2D->FindRGB(r, g, b, a);
        }
        // now we need the string representing the word
        int wordEnd = lineTypos[visLine][i].endPos;
        csString tmp = str.Slice(wordStart, wordEnd-wordStart);
        // print the word
        graphics2D->Write(font, x, y, color, -1, tmp.GetDataSafe());
        // and advance x to the start of the next word
        int textWidth, textHeight;
        font->GetDimensions(tmp.GetDataSafe(), textWidth, textHeight);
        x += textWidth;
        // and also advance to the start of the next word
        wordStart = wordEnd;
    }
}

bool pawsMultilineEditTextBox::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        SetText(node->GetAttributeValue("text"));
        return true;
    }
    else
        return false;
}

bool pawsMultilineEditTextBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> settingNode = node->GetNode("pawsScrollBar");
    if(settingNode)
    {
        csRef<iDocumentAttribute> widthAttribute = settingNode->GetAttribute("width");
        if(widthAttribute)
            vScrollBarWidth = widthAttribute->GetValueAsInt();
    }

    settingNode = node->GetNode("text");

    if(settingNode)
    {
        csRef<iDocumentAttribute> textAttribute = settingNode->GetAttribute("string");
        if(textAttribute)
        {
            SetText(textAttribute->GetValue());
        }
    }
    else
    {
        SetText("");
    }
    // lets see if we find some options for the spellchecker
    settingNode = node->GetNode("spellChecker");
    if(settingNode)
    {
        // should the spellchecekr be used for this instance of the widget?
        spellChecked = settingNode->GetAttributeValueAsBool("enable", false);
        // What colour should we use for typos?
        int r = settingNode->GetAttributeValueAsInt("r", 255);
        int g = settingNode->GetAttributeValueAsInt("g", 0);
        int b = settingNode->GetAttributeValueAsInt("b", 0);
        typoColour = graphics2D->FindRGB(r, g, b);
    }

    return true;
}


void pawsMultilineEditTextBox::SetText(const char* newText, bool publish)
{
    if(publish && subscribedVar)
        PawsManager::GetSingleton().Publish(subscribedVar, newText);

    cursorPosition = 0;
    cursorLine = 0;
    topLine = 0;

    text = newText;

    if(maxLen)
        text.Truncate(maxLen);
    LayoutText();

    SetupScrollBar();

    cursorLoc = lineInfo[cursorLine]->lineLength - lineInfo[cursorLine]->lineExtra;

}


const char* pawsMultilineEditTextBox::GetText()
{
    return text;
}

//Get start and endpoints for specified line in text
void pawsMultilineEditTextBox::GetLinePos(size_t lineNumber, size_t &start, size_t &end)
{
    start = 0;
    end = 0;
    for(size_t i = 0; i < lineNumber; i++)
    {
        size_t imp = lineInfo[i]->lineLength;
        start += imp;
    }
    end = start + lineInfo[lineNumber]->lineLength;
}

char pawsMultilineEditTextBox::GetAt(size_t destLine, size_t destCursor)
{
    return GetLine(destLine).GetAt(destCursor);
}

void pawsMultilineEditTextBox::GetLineRelative(size_t pos, size_t &start, size_t &end)
{
    size_t count = 0;

    for(size_t i = 0; i < lineInfo.GetSize(); i++)
    {
        count = lineInfo[i]->breakLine - lineInfo[i]->lineExtra;
        if(pos <= count)
        {
            start = lineInfo[i]->breakLine - lineInfo[i]->lineLength;
            end = lineInfo[i]->breakLine;
            return;
        }
    }
}

int pawsMultilineEditTextBox::GetLineWidth(int /*lineNumber*/)
{
    int width = 0;
    int height = 0;
    csString line = GetLine(cursorLine);
    GetFont()->GetDimensions(line, width, height);
    return width;
}

bool pawsMultilineEditTextBox::EndsWithNewLine(int lineNumber)
{
    return GetLine(cursorLine).GetAt(lineInfo[lineNumber]->lineLength - 1) == '\n' ? true : false;
}

//Get specified line in text
csString pawsMultilineEditTextBox::GetLine(size_t lineNumber)
{
    size_t start = 0;
    size_t end = 0;
    GetLinePos(lineNumber, start, end);
    return text.Slice(start, end - start);
}

//Get cursor position relative to entire text.
size_t pawsMultilineEditTextBox::GetCursorPosition()
{
    size_t count = 0;
    for(size_t i = 0; i < cursorLine; i++)
    {
        size_t pos = lineInfo[i]->lineLength;
        count += pos;
    }
    count = count + cursorLoc;
    return count;
}

size_t pawsMultilineEditTextBox::GetCursorPosition(size_t destLine, size_t destCursor)
{
    if(destLine >0)
        return lineInfo[destLine-1]->breakLine+destCursor;
    else
        return destCursor;
}

void pawsMultilineEditTextBox::GetCursorLocation(size_t pos, size_t &destLine, size_t &destCursor)
{
    size_t start = 0;
    size_t end = 0;
    destLine = 0;
    destCursor = 0;

    GetLineRelative(pos, start, end);
    for(size_t i = 0; i < lineInfo.GetSize(); i++)
    {
        //Adjustment for wordwrap so final character is not exceeded by the cursor
        if((pos > (lineInfo[i]->breakLine - lineInfo[i]->lineExtra)) && (i + 1 < lineInfo.GetSize()))
        {
            destLine++;
        }
        else
        {
            destCursor = pos - start;
            return;
        }
    }
}


void pawsMultilineEditTextBox::SetMaxLength(unsigned int maxlen)
{
    maxLen = maxlen;
    if(maxLen)
        text.Truncate(maxlen);
}

void pawsMultilineEditTextBox::checkSpelling()
{
    // of course we only want to do this if the spellchecker plugin exists
    if(spellChecker)
    {
        //clear the array with the boundries of the typos for each text line
        lineTypos.Empty();
        // now check the spelling for each visible line
        for(size_t line = topLine; line <= (topLine + canDrawLines); line++)
        {
            // we for sure don't want to check more lines than even exist
            if(line >= lineInfo.GetSize())
                break;
            //chomp newline characters
            if(lineInfo[line]->lineLength > 0)
            {
                // okay, we need the string of the line
                csString curLine = GetLine(line);
                // and we remove the newlines (should be only one at the end of a string if this isn't the last text line)
                curLine.ReplaceAll("\n", "");
                csArray<Word> tmpLine;
                // get the postitions of the words by checking for spaces in the textline
                Word tmpWord;
                csString tmpString;
                size_t oldSpace = 0;
                size_t foundSpace = curLine.Find(" ", oldSpace);
                while(foundSpace != (size_t)-1)
                {
                    curLine.SubString(tmpString, oldSpace, foundSpace-oldSpace);
                    // now do the spellchecking
                    tmpWord.correct = spellChecker->correct(tmpString);
                    tmpWord.endPos = foundSpace;
                    // and save everything
                    tmpLine.Push(tmpWord);
                    oldSpace = foundSpace;
                    foundSpace = curLine.Find(" ", oldSpace+1);
                }
                curLine.SubString(tmpString, oldSpace, curLine.Length()-oldSpace);
                // now do the spellchecking
                tmpWord.correct = spellChecker->correct(tmpString);
                tmpWord.endPos = curLine.Length();
                if(tmpWord.endPos > 0)
                {
                    // save only if the word contains something
                    tmpLine.Push(tmpWord);
                }
                // and not to forget to save this all in the array for the lines
                lineTypos.Push(tmpLine);
            }
        }
    }
    else
    {
        lineTypos.Empty();
    }
}
