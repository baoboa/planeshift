/*
 * pawstextbox.cpp - Author: Andrew Craig
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
// pawstextbox.cpp: implementation of the pawsTextBox class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>
#include <ctype.h>
#include <ivideo/fontserv.h>
#include <iutil/evdefs.h>
#include <iutil/event.h>
#include <iutil/plugin.h>
#include <csutil/csuctransform.h>

#include "pawstextbox.h"
#include "pawsmanager.h"
#include "pawsprefmanager.h"
#include "pawscrollbar.h"
#include "pawsimagedrawable.h"
#include "pawstexturemanager.h"
#include "util/log.h"
#include "util/strutil.h"
#include "util/psstring.h"
#include "util/psconst.h"

#define BORDER_SIZE            2 // For pawsFadingTextBox


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsTextBox::pawsTextBox() : textX(0), textY(0)
{
    horizAdjust = horizLEFT;
    vertAdjust  = vertCENTRE;

    vertical = false;
    grayed = false;
    letterSizes = NULL;
    factory = "pawsTextBox";
}
pawsTextBox::pawsTextBox(const pawsTextBox &origin):
    pawsWidget(origin),
    text(origin.text),
    colour(origin.colour),
    grayed(origin.grayed),
    horizAdjust(origin.horizAdjust),
    vertAdjust(origin.vertAdjust),
    vertical(origin.vertical),
    textX(origin.textX),
    textY(origin.textY),
    textWidth(origin.textWidth)
{
    letterSizes = 0;
    if(origin.letterSizes)
    {
        letterSizes = new psPoint [text.Length()];
        for(unsigned int i = 0 ; i < text.Length() ; i++)
            letterSizes[i] = origin.letterSizes[i];
    }
}
pawsTextBox::~pawsTextBox()
{
    if(letterSizes != NULL)
        delete [] letterSizes;
}

bool pawsTextBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentAttribute> vertAttribute = node->GetAttribute("vertical");
    if(vertAttribute)
        SetVertical(strcmp(vertAttribute->GetValue(), "yes") == 0);

    csRef<iDocumentNode> textNode = node->GetNode("text");

    if(textNode)
    {
        csRef<iDocumentAttribute> fontAdjustAttribute;
        fontAdjustAttribute = textNode->GetAttribute("horizAdjust");
        if(fontAdjustAttribute)
        {
            csString fontAdjust = fontAdjustAttribute->GetValue();
            if(fontAdjust == "CENTRE")
                HorizAdjust(horizCENTRE);
            else if(fontAdjust == "RIGHT")
                HorizAdjust(horizRIGHT);
            else
                HorizAdjust(horizLEFT);
        }

        fontAdjustAttribute = textNode->GetAttribute("vertAdjust");
        if(fontAdjustAttribute)
        {
            csString fontAdjust = fontAdjustAttribute->GetValue();
            if(fontAdjust == "CENTRE")
                VertAdjust(vertCENTRE);
            else if(fontAdjust == "BOTTOM")
                VertAdjust(vertBOTTOM);
            else
                VertAdjust(vertTOP);
        }
        else
            VertAdjust(vertCENTRE);  // default is centered vertically unless specified otherwise

        csRef<iDocumentAttribute> textAttribute = textNode->GetAttribute("string");
        if(textAttribute)
        {
            SetText(PawsManager::GetSingleton().Translate(textAttribute->GetValue()));
        }
    }

    return true;
}

void pawsTextBox::CalcTextPos()
{
    int width, height;

    if(!screenFrame.Height())
        SetSizeByText(0,0);

    if(horizAdjust==horizRIGHT  ||  horizAdjust==horizCENTRE  ||  vertAdjust==vertBOTTOM  ||  vertAdjust==vertCENTRE)
        CalcTextSize(width, height);

    switch(horizAdjust)
    {
        case horizLEFT:
            textX = 0;
            break;
        case horizRIGHT:
            textX = (screenFrame.Width()-margin*2) - width;
            break;
        case horizCENTRE:
            textX = ((screenFrame.Width()-margin*2) - width)  / 2;
            break;
    }

    switch(vertAdjust)
    {
        case vertTOP:
            textY = 0;
            break;
        case vertBOTTOM:
            textY = (screenFrame.Height()-margin*2) - height;
            break;
        case vertCENTRE:
            textY = ((screenFrame.Height()-margin*2) - height) / 2;
            break;
    }
}

void pawsTextBox::CalcLetterSizes()
{
    char letterStr[2];
    unsigned int i;

    assert(vertical);

    if(letterSizes != NULL)
        delete [] letterSizes;

    letterSizes = new psPoint [text.Length()];
    textWidth = 0;
    letterStr[1] = '\0'; // Make sure the string is terminated
    // pos 0 will be filled in later.

    for(i=0; i < text.Length(); i++)
    {
        letterStr[0] = text.GetAt(i);

        GetFont()->GetDimensions(letterStr, letterSizes[i].x, letterSizes[i].y);

        textWidth = csMax(textWidth, letterSizes[i].x);
    }
}

void pawsTextBox::SetVertical(bool vertical)
{
    this->vertical = vertical;

    if(vertical)
        CalcLetterSizes();
    else if(letterSizes != NULL)
    {
        delete [] letterSizes;
        letterSizes = NULL;
    }
    CalcTextPos();
}

void pawsTextBox::Adjust(pawsHorizAdjust horiz, pawsVertAdjust vert)
{
    horizAdjust  = horiz;
    vertAdjust   = vert;
    CalcTextPos();
}

void pawsTextBox::HorizAdjust(pawsHorizAdjust horiz)
{
    horizAdjust  = horiz;
    CalcTextPos();
}

void pawsTextBox::VertAdjust(pawsVertAdjust vert)
{
    vertAdjust   = vert;
    CalcTextPos();
}

void pawsTextBox::CalcTextSize(int &width, int &height)
{
    if(vertical)
    {
        unsigned int i;

        width = 0;
        height = 0;

        for(i=0; i < text.Length(); i++)
        {
            width   = csMax(width, letterSizes[i].x);
            height  += letterSizes[i].y;
        }
    }
    else if(text.GetData() != NULL)
    {
        GetFont()->GetDimensions((const char*)text, width, height);
        //width+=5;
        //height+=5;
    }
    else
    {
        GetFont()->GetDimensions("Sample", width, height);  // Example text with full height caps and descenders
    }
}

bool pawsTextBox::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        SetText(node->GetAttributeValue("text"));
        return true;
    }
    else if(node->GetContentsValue())
    {
        SetText(node->GetContentsValue());
        return true;
    }
    else
        return false;
}

void pawsTextBox::FormatText(const char* fmt, ...)
{
    char text[1024];
    va_list args;

    va_start(args, fmt);
    cs_vsnprintf(text,sizeof(text),fmt,args);
    va_end(args);

    SetText((const char*)text);
}


void pawsTextBox::SetText(const char* newText)
{
    // Make sure any \r characters are replaced by printable \n ones.
    psString str(newText);
    str.ReplaceAll("\r", "\n");

    text.Replace(str.GetData());

    if(vertical)
        CalcLetterSizes();
    CalcTextPos();
}

void pawsTextBox::SetSizeByText(int padX, int padY)
{
    int width, height;

    // These padding parameters used to be hardcoded to 5,5 in all cases inside CalcTextSize.
    CalcTextSize(width, height);
    SetRelativeFrameSize(width+padX, height+padY);
    textX = 0;
    textY = 0;
}

int pawsTextBox::GetFontColour()
{
    if(grayed)
        return graphics2D->FindRGB(150,150,100);
    return pawsWidget::GetFontColour();
}

void pawsTextBox::Draw()
{
    pawsWidget::Draw();

    if(vertical)
    {
        char letterStr[2];
        int letterY;

        letterStr[1] = 0;
        letterY = textY;
        for(unsigned int i = 0; i < text.Length(); i++)
        {
            letterStr[0] = text.GetAt(i);
            DrawWidgetText(letterStr ,
                           screenFrame.xmin + margin + textX + (textWidth-letterSizes[i].x)/2,
                           screenFrame.ymin + margin + letterY);

            letterY += letterSizes[i].y;
        }
    }
    else
    {
        DrawWidgetText((const char*)text,
                       screenFrame.xmin + margin + textX,
                       screenFrame.ymin + margin + textY);
    }
}

void pawsTextBox::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    // This is called automatically whenever subscribed data is published.
    if(subscription_format == "percent")
    {
        float f = value.GetFloat();
        char buff[10];
        sprintf(buff,"%.01f%%",f*100);
        SetText(buff);
    }
    else
        SetText(value.GetStr());
}

int pawsTextBox::CountCodePoints(const char* text, int start, int len)
{
    int codePoints = 0;
    const char* str = text;
    if(len == -1)
        len = strlen(text) - start;
    text += start;
    while(str < text + len)
    {
        str += csUnicodeTransform::UTF8Skip((const utf8_char*) str, text + len - str);
        codePoints++;
    }
    return codePoints;
}

int pawsTextBox::RewindCodePoints(const char* text, int start, int count)
{
    const char* str = text + start;
    while(count > 0 && str > text)
    {
        str -= csUnicodeTransform::UTF8Rewind((const utf8_char*) str, str - text);
        count--;
    }
    return str - text;
}

int pawsTextBox::SkipCodePoints(const char* text, int start, int count)
{
    const char* str = text + start;
    int len = strlen(text);
    while(count > 0 && str < text + len)
    {
        str += csUnicodeTransform::UTF8Skip((const utf8_char*)str, text + len - str);
        count--;
    }
    return str - text + start;
}

//----------------------------------------------------------------------------------------

pawsMessageTextBox::pawsMessageTextBox()
{
    topLine = 0;
    maxLines = 0;
    scrollBarWidth = 25;
    CalcLineHeight();
    scrollBar = NULL;
    factory = "pawsMessageTextBox";
}
pawsMessageTextBox::pawsMessageTextBox(const pawsMessageTextBox &origin)
    :pawsWidget(origin)
{
    maxLines = origin.maxLines;
    //adjusted = origin.adjusted;
    lineHeight = origin.lineHeight;
    topLine = origin.topLine;
    scrollBarWidth = origin.scrollBarWidth;
    for(unsigned int x = 0 ; x < origin.children.GetSize(); x++)
    {
        if(origin.scrollBar == origin.children[x] && x < children.GetSize())
        {
            scrollBar = dynamic_cast<pawsScrollBar*>(children[x]);
        }
    }

    for(unsigned int i = 0 ; i < origin.adjusted.GetSize(); i++)
    {
        adjusted.Push(new MessageLine(*origin.adjusted[i]));
    }
    for(unsigned int i = 0 ; i < origin.messages.GetSize(); i++)
    {
        messages.Push(new MessageLine(*origin.messages[i]));
    }

}
pawsMessageTextBox::~pawsMessageTextBox()
{
    Clear();
}

void pawsMessageTextBox::Clear()
{
    messages.Empty();
    adjusted.Empty();
    topLine = 0;
    if(scrollBar)
        scrollBar->Hide();
}

void pawsMessageTextBox::CalcLineHeight()
{
    int dummy;
    GetFont()->GetMaxSize(dummy, lineHeight);
    lineHeight -=2;
}

pawsScrollBar* pawsMessageTextBox::GetScrollBar()
{
    csArray<pawsWidget*>::Iterator it = children.GetIterator();
    while(it.HasNext())
    {
        pawsScrollBar* child = dynamic_cast<pawsScrollBar*>(it.Next());
        if(child)
            return child;
    }

    return NULL;
}

bool pawsMessageTextBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> scrollBarNode = node->GetNode("pawsScrollBar");
    if(scrollBarNode)
    {
        csRef<iDocumentAttribute> widthAttribute = scrollBarNode->GetAttribute("width");
        if(widthAttribute)
            scrollBarWidth = widthAttribute->GetValueAsInt();
    }

    CalcLineHeight();

    return true;
}

bool pawsMessageTextBox::Setup()
{
    CalcLineHeight();
    return true;
}

bool pawsMessageTextBox::PostSetup()
{
    // Create the optional scroll bar here as well but hidden.
    scrollBar = GetScrollBar();
    if(!scrollBar)
    {
        scrollBar = new pawsScrollBar;
        scrollBar->SetParent(this);
        scrollBar->PostSetup();
        AddChild(scrollBar);

        scrollBar->SetTickValue(1.0);
        scrollBar->SetMaxValue(0.0);
    }

    scrollBar->SetRelativeFrame(defaultFrame.Width() - scrollBarWidth, 6, scrollBarWidth, defaultFrame.Height() - 12);
    int attach = ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT;
    scrollBar->SetAttachFlags(attach);

    OnResize();

    return true;
}

void pawsMessageTextBox::Resize()
{
    pawsWidget::Resize();

    CalcLineHeight();

    adjusted.Empty();

    for(size_t x = 0; x < messages.GetSize(); x++)
    {
        MessageLine* dummy = NULL;
        int dummyX = -1;
        if(messages[x]->segments.IsEmpty())
            SplitMessage(messages[x]->text, messages[x]->colour, messages[x]->size, dummy, dummyX);
        else
        {
            MessageLine* msgLine = NULL;
            int offsetX = 0;
            for(size_t i = 0; i < messages[x]->segments.GetSize(); i++)
                SplitMessage(messages[x]->segments[i].text, messages[x]->segments[i].colour, messages[x]->segments[i].size, msgLine, offsetX);
        }
    }
}

void pawsMessageTextBox::OnResize()
{
    int borderOffSize = 4;
    if(border)
        borderOffSize = 8;

    int rest = (screenFrame.Height()-borderOffSize) % lineHeight;
    if(screenFrame.Height() >= borderOffSize+rest)
        maxLines = (screenFrame.Height()-borderOffSize-rest) / lineHeight;
    else
        maxLines = 0;

    // Re adjust the top line value
    topLine = (int)adjusted.GetSize() - (int)maxLines;
    if(topLine < 0)
        topLine = 0;

    if(scrollBar)
    {
        scrollBar->SetMaxValue(maxLines > adjusted.GetSize() ? 0 : (float)(adjusted.GetSize()-maxLines));
        scrollBar->SetCurrentValue((float)topLine);

        if(maxLines < adjusted.GetSize())
        {
            scrollBar->Show();
            scrollBar->RecalcScreenPositions();
        }
        else
            scrollBar->Hide();
    }
}


void pawsMessageTextBox::Draw()
{
    pawsWidget::Draw();

    ClipToParent(false);

    int yPos = 0;

    if(topLine < 0)
        topLine = 0;

    for(size_t x = topLine; x < (topLine+maxLines); x++)
    {
        if(x < adjusted.GetSize())
        {
            if(adjusted[x]->segments.IsEmpty())
            {
                // Draw shadow
                graphics2D->Write(GetFont(),
                                  screenFrame.xmin + 1,
                                  screenFrame.ymin + yPos*lineHeight + 1,
                                  0,
                                  -1,
                                  (const char*)adjusted[x]->text);
                // Draw actual text
                graphics2D->Write(GetFont(),
                                  screenFrame.xmin,
                                  screenFrame.ymin + yPos*lineHeight,
                                  adjusted[x]->colour,
                                  -1,
                                  (const char*)adjusted[x]->text);
            }
            else
            {
                for(size_t i = 0; i < adjusted[x]->segments.GetSize(); i++)
                {
                    // Draw shadow
                    graphics2D->Write(GetFont(),
                                      screenFrame.xmin + adjusted[x]->segments[i].x + 1,
                                      screenFrame.ymin + yPos*lineHeight + 1,
                                      0,
                                      -1,
                                      (const char*)adjusted[x]->segments[i].text);
                    // Draw actual text
                    graphics2D->Write(GetFont(),
                                      screenFrame.xmin + adjusted[x]->segments[i].x,
                                      screenFrame.ymin + yPos*lineHeight,
                                      adjusted[x]->segments[i].colour,
                                      -1,
                                      (const char*)adjusted[x]->segments[i].text);
                }
            }

            yPos++;
        }
    }
}


bool pawsMessageTextBox::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        Clear();
        AddMessage(node->GetAttributeValue("text"));
        return true;
    }
    return false;
}


void pawsMessageTextBox::AddMessage(const char* data, int msgColour)
{
    // Notify parent of activity
    OnChange(this);
    // Extract \n out of the data and print a newline for each.
    csString message = data;
    csArray<csString> cutMessages;

    size_t pos = message.Length();
    if(pos == 0)
    {
        cutMessages.Push("");
    }

    while(pos > 0)
    {
        size_t last = message.FindLast("\r");
        if(last == message.Length()-1)
        {
            message.Truncate(last);
        }

        last = message.FindLast("\n");
        if(last == SIZET_NOT_FOUND)
        {
            cutMessages.Push(message);
            break;
        }

        if(last == 0)
        {
            cutMessages.Push(message.Slice(1, pos));
            break;
        }

        if(last == message.Length()-1)
        {
            cutMessages.Push("");
        }
        else
        {
            cutMessages.Push(message.Slice(last+1, pos));
        }

        message.Truncate(last);
        pos = last;
    }

    while(!cutMessages.IsEmpty())
    {
        csString messageText = cutMessages.Pop();
        bool onBottom = false;
        int oldTopLine = topLine;

        if(scrollBar->GetCurrentValue() == scrollBar->GetMaxValue())
            onBottom = true;
        //this handles the case in case the scrollbar is not needed yet.
        else if(scrollBar->GetMaxValue() < 1)
            onBottom = true;

        if(topLine < 0)
            topLine = 0;

        // Add it to the main message buffer.
        MessageLine* msg = new MessageLine;

        // Find font info embedded in the data.
        int colour = msgColour;
        int size = 0;

        if(msgColour == -1)
        {
            colour = GetFontColour();
        }

        size_t textStart = 0;
        size_t textEnd = messageText.Length();
        int x = 0;
        MessageLine* msgLine = NULL;
        msg->size = 0;
        msg->colour = msgColour;
        msg->text = messageText;

        // Empty line is a special case here
        if(messageText.Length() == 0)
        {
            SplitMessage(messageText, colour, size, msgLine, x);
        }

        while(textStart < messageText.Length())
        {
            size_t pos = messageText.FindFirst(ESCAPECODE, textStart);
            if(pos == SIZET_NOT_FOUND)
                textEnd = messageText.Length();
            else
                textEnd = pos;

            if(textStart == 0 && pos == SIZET_NOT_FOUND)
            {
                int dummyX = -1;
                SplitMessage(messageText, colour, size, msgLine, dummyX);
                break;
            }
            else if(textEnd > textStart)
            {
                csString subText = messageText.Slice(textStart, textEnd - textStart);
                SplitMessage(subText, colour, size, msgLine, x);

                MessageSegment newSegment;
                newSegment.text = subText;
                newSegment.colour = colour;
                newSegment.size = size;
                msg->segments.Push(newSegment);
            }
            textStart = textEnd;

            if(pos != SIZET_NOT_FOUND && pos + LENGTHCODE <= messageText.Length())
            {
                int r, g, b;
                if(!psColours::ParseColour(messageText.GetData() + pos, r, g, b, size))
                {
                    // Not a colour code so skip
                    textStart++;
                    continue;
                }
                if(r == 0 && g == 0 && b == 0)
                    colour = msgColour;
                else
                    colour = graphics2D->FindRGB(r, g, b);
                messageText.DeleteAt(pos, LENGTHCODE);
            }
        }
        messages.Push(msg);
        if(scrollBar)
        {
            if(adjusted.GetSize() > maxLines)
                scrollBar->ShowBehind();
            else
                scrollBar->Hide();

            scrollBar->SetMaxValue(maxLines > adjusted.GetSize() ? 0 : (float)(adjusted.GetSize()-maxLines));
        }

        topLine = (int)adjusted.GetSize() - (int)maxLines;
        if(topLine < 0)
            topLine = 0;


        if(!onBottom)
        {
            topLine = oldTopLine;
            if(scrollBar)
            {
                scrollBar->SetCurrentValue(float(topLine));
            }
        }
        else if(scrollBar)
        {
            scrollBar->SetCurrentValue(float(topLine));
        }
    }
}

void pawsMessageTextBox::AppendLastMessage(const char* data)
{
    if(messages.IsEmpty())
    {
        AddMessage(data);
        return;
    }

    MessageLine* line = messages.Get(messages.GetSize()-1);
    line->text.Append(data);
    line = adjusted.Get(adjusted.GetSize()-1);
    line->text.Append(data);

    // Trim \n from the end and add a new line.
    while(line->text.FindLast("\n") == line->text.Length()-1)
    {
        line->text.Truncate(line->text.Length()-1);
        AddMessage("");
    }
}

void pawsMessageTextBox::ReplaceLastMessage(const char* rawMessage)
{
    csString data(rawMessage);
    data.ReplaceAll("\r", "");
    if(messages.IsEmpty())
    {
        AddMessage(data);
        return;
    }

    MessageLine* line = messages.Get(messages.GetSize()-1);
    line->text.Replace(data);
    line = adjusted.Get(adjusted.GetSize()-1);
    line->text.Replace(data);

    // Trim \n from the end and add a new line.
    while(line->text.FindLast("\n") == line->text.Length()-1)
    {
        line->text.Truncate(line->text.Length()-1);
        AddMessage("");
    }
}

void pawsMessageTextBox::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    // This is called automatically whenever subscribed data is published.
    if(overwrite_subscription)
        Clear();

    if(value.type == PAWS_DATA_INT_STR)
        AddMessage(value.GetStr(), value.GetInt());
    else
        AddMessage(value.GetStr());
}

void pawsMessageTextBox::ResetScroll()
{
    if(scrollBar)
        scrollBar->SetCurrentValue(0);
}

void pawsMessageTextBox::FullScroll()
{
    if(scrollBar)
        scrollBar->SetCurrentValue(scrollBar->GetMaxValue());

}

void pawsMessageTextBox::SplitMessage(const char* newText, int colour,
                                      int /*size*/, MessageLine* &msgLine, int &startPosition)
{
    csString stringBuffer(newText);

    if(stringBuffer.IsEmpty())
    {
        WriteMessageLine(msgLine, "", colour);
        return;
    }

    while(!stringBuffer.IsEmpty())
    {
        int offSet = INITOFFSET;
        int width = -1;
        int height = -1;
        /// See how many characters can be drawn on a single line.
        if(startPosition != -1)
        {
            GetFont()->GetDimensions(stringBuffer.GetData(), width, height);
            offSet += startPosition;
        }
        int canDrawLength = GetFont()->GetLength(stringBuffer.GetData(),
                            screenFrame.Width() - offSet);

        /// If it can fit the entire string then return.
        if(size_t(canDrawLength) == stringBuffer.Length())
        {
            if(!msgLine)
            {
                WriteMessageLine(msgLine,stringBuffer,colour);
            }
            if(startPosition != -1)
            {
                WriteMessageSegment(msgLine,stringBuffer,colour,startPosition);
                startPosition += width;
            }
            break;
        }
        else
        {
            csString processedString = FindStringThatFits(stringBuffer, canDrawLength);
            stringBuffer.DeleteAt(0, processedString.Length());

            if(!msgLine)
            {
                WriteMessageLine(msgLine,processedString,colour);
            }
            if(startPosition != -1)
            {
                WriteMessageSegment(msgLine,processedString,colour,startPosition);
                startPosition = 0;
            }
            // Next time use a new line!
            msgLine = NULL;
        }
    }
}

void pawsMessageTextBox::WriteMessageLine(MessageLine* &msgLine, csString text, int colour)
{
    msgLine = new MessageLine;
    msgLine->size = text.Length();
    msgLine->text = text;
    msgLine->colour = colour;
    adjusted.Push(msgLine);
}

void pawsMessageTextBox::WriteMessageSegment(MessageLine* &msgLine, csString text, int colour, int startPosition)
{
    MessageSegment seg;
    seg.size = text.Length();
    seg.text = text;
    seg.colour = colour;
    seg.x = startPosition;
    msgLine->segments.Push(seg);
}

csString pawsMessageTextBox::FindStringThatFits(csString stringBuffer, int canDrawLength)
{
    int breakPoint = stringBuffer.FindLast(' ', canDrawLength - 1);
    int spaceAfterBreak = stringBuffer.FindFirst(' ', breakPoint + 1);
    int wordLength = spaceAfterBreak - breakPoint;
    csString wordAfterBreak;

    if(spaceAfterBreak == -1)
    {
        stringBuffer.SubString(wordAfterBreak, breakPoint + 1);
    }
    else
    {
        stringBuffer.SubString(wordAfterBreak, breakPoint + 1, wordLength);
    }

    int width = 0;
    int height = 0;
    GetFont()->GetDimensions(wordAfterBreak.GetData(), width, height);

    csString processedString;
    if(width <= screenFrame.Width() - INITOFFSET)
    {
        processedString.Append(stringBuffer, breakPoint + 1);
    }
    else
    {
        processedString.Append(stringBuffer, canDrawLength);
    }
    return processedString;
}

bool pawsMessageTextBox::OnScroll(int /*direction*/, pawsScrollBar* widget)
{
    topLine = (int)widget->GetCurrentValue();
    return true;
}

bool pawsMessageTextBox::OnMouseDown(int button, int modifiers, int x, int y)
{
    if(button == csmbWheelUp)
    {
        if(scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - MESSAGE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if(button == csmbWheelDown)
    {
        if(scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + MESSAGE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }

    return pawsWidget::OnMouseDown(button, modifiers, x ,y);
}

bool pawsMessageTextBox::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers)
{
    //this function handles if the user uses the page up or down buttons on this text box
    //and scroll of exactly the size of lines which is shown in the textbox
    switch(key)
    {
        case CSKEY_PGUP: //go up
        {
            if(scrollBar)
            {
                scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - maxLines); //maxLines is updated on resize
            }
            break;
        }
        case CSKEY_PGDN: //go down
        {
            if(scrollBar)
            {
                scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + maxLines); //maxLines is updated on resize
            }
            break;
        }
        default:
        {
            return pawsWidget::OnKeyDown(keyCode, key, modifiers);
        }
    }
    return true;
}


//---------------------------------------------------------------------------------


pawsEditTextBox::pawsEditTextBox() : password(false),
    start(0),
    cursorPosition(0),
    blink(true),
    maxLen(0),
    spellChecked(false),
    typoColour(0xFF0000)
{
    factory = "pawsEditTextBox";
    clock =  csQueryRegistry<iVirtualClock > (PawsManager::GetSingleton().GetObjectRegistry());

    blinkTicks = clock->GetCurrentTicks();

    // Find the line height;
    int dummy;
    GetFont()->GetMaxSize(dummy, lineHeight);
    lineHeight -=2;

    // spellchecker works only on linux
#ifdef CS_PLATFORM_UNIX
    //get the spellchecker plugin
    spellChecker = csQueryRegistryOrLoad<iSpellChecker>(PawsManager::GetSingleton().GetObjectRegistry(), "crystalspace.planeshift.spellchecker");
#endif

}
pawsEditTextBox::pawsEditTextBox(const pawsEditTextBox &origin): pawsWidget(origin),
    // spellchecker works only on linux
#ifdef CS_PLATFORM_UNIX
    spellChecker(origin.spellChecker),
    spellChecked(origin.spellChecked),
#endif
    typoColour(origin.typoColour),
    words(origin.words)
{

    clock = origin.clock;
    blink = origin.blink;
    blinkTicks = origin.blinkTicks;
    cursorLine = origin.cursorLine;
    cursorPosition = origin.cursorPosition;
    lineHeight = origin.lineHeight;
    maxLen = origin.maxLen;
    maxLines = origin.maxLines;
    multiLine = origin.multiLine;
    password = origin.password;
    start = origin.start;
    text = origin.text;
    topLine = origin.topLine;
    usedLines = origin.usedLines;
    vScrollBar = 0;

    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.children[i] == origin.vScrollBar)
        {
            vScrollBar = dynamic_cast<pawsScrollBar*>(children[i]);
            break;
        }
    }
}
pawsEditTextBox::~pawsEditTextBox()
{
}

void pawsEditTextBox::SetSizeByText()
{
    int width, height;

    if(GetFont()!=NULL && text.GetData()!=NULL)
    {
        GetFont()->GetDimensions(text.GetData(), width, height);
        SetSize(width+margin*2, height+margin*2);
    }
}

void pawsEditTextBox::SetPassword(bool pass)
{
    password = pass;
}

bool pawsEditTextBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> textNode = node->GetNode("text");
    if(textNode)
    {
        csRef<iDocumentAttribute> stringAttribute = textNode->GetAttribute("string");
        if(stringAttribute)
        {
            SetText(PawsManager::GetSingleton().Translate(stringAttribute->GetValue()));
        }
    }
    csRef<iDocumentNode> settingNode = node->GetNode("spellChecker");
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
    // Find the line height
    int dummy;
    GetFont()->GetMaxSize(dummy, lineHeight);
    lineHeight -=2;

    return true;
}

bool pawsEditTextBox::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        SetText(node->GetAttributeValue("text"));
        return true;
    }
    else
        return false;
}

void pawsEditTextBox::SetText(const char* newText, bool publish)
{
    if(publish && subscribedVar)
        PawsManager::GetSingleton().Publish(subscribedVar, newText);

    text.Replace(newText);
    if(maxLen)
        text.Truncate(maxLen);

    if(newText)
        cursorPosition = strlen(newText);
    else
        cursorPosition = 0;
}

void pawsEditTextBox::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    // This is called automatically whenever subscribed data is published.
    SetText(value.GetStr(), false);
}

void pawsEditTextBox::Draw()
{
    if(clock->GetCurrentTicks() - blinkTicks > BLINK_TICKS)
    {
        blink = !blink;
        blinkTicks = clock->GetCurrentTicks();
    }

    pawsWidget::Draw();

    ClipToParent(false);

    if(cursorPosition>text.Length())
    {
        cursorPosition=text.Length();
    }


    if(start>(int)text.Length())
    {
        start=0;
    }

    if(text.Length() > 0)
    {
        // Get the number of characters to draw
        int maxChars;
        maxChars = GetFont()->GetLength(text.GetData() + start, screenFrame.Width() - margin*2 - 8);

        if((int)cursorPosition > start + maxChars)
        {
            start = (int)cursorPosition - maxChars;
        }
        if(start < 0)
        {
            start = 0;
        }

        // Make the text the correct length
        csString tmp(text.GetData() + start);
        tmp.Truncate(maxChars);

        if(password)  //show astrices instead of text
        {
            for(unsigned int i=0; i<tmp.Length(); i++)
            {
                tmp.SetAt(i,'*');
            }
        }

        // Get size of text
        int textWidth;
        int textHeight;

        GetFont()->GetDimensions(tmp.GetData(), textWidth, textHeight);

        // Get the center
        int textCenterX = 4;
        int textCenterY = (screenFrame.Height()-(margin*2)) / 2  - textHeight / 2;

        if(spellChecker && spellChecked && spellChecker->hasDicts())
        {
            //check if the current widgets text is already spell-checked by comparing the the end of the last word with the length of the text.
            size_t length = 0;
            if(words.GetSize() > 0)
            {
                length = words[words.GetSize()-1].endPos;
            }
            if(length != text.Length())
            {
                checkSpelling();
            }


            if(cursorPosition>text.Length())
                cursorPosition=text.Length();

            int textXPos = screenFrame.xmin + textCenterX + margin;

            int basicFontColor = GetFontColour();

            // now we need to draw every word separately as the color might change
            for(size_t i = 0; i < words.GetSize(); i++)
            {
                int wordStart;
                if(i == 0)
                {
                    wordStart = 0 - start;
                }
                else
                {
                    wordStart=words[i-1].endPos - start;
                }
                int wordEnd = words[i].endPos - start;

                // is the word or parts of it displayed?
                if(!(((wordStart < 0) && (wordEnd < 0)) || ((wordStart >= (int) tmp.Length()) && (wordEnd >= (int) tmp.Length()))))
                {
                    // set correct word borders according to displayed chars
                    if(wordStart < 0)
                    {
                        wordStart = 0;
                    }
                    if(wordEnd >= (int) tmp.Length())
                    {
                        wordEnd = tmp.Length();
                    }
                    // set different fontcolours for correct/incorrect words
                    if(words[i].correct)
                    {
                        SetColour(basicFontColor);
                    }
                    else
                    {
                        SetColour(typoColour);
                    }

                    DrawWidgetText(tmp.Slice(wordStart, wordEnd-wordStart).GetData(), textXPos, screenFrame.ymin + textCenterY + margin);

                    // get the x-offset for the next word
                    int textWidth, textHeight;
                    GetFont()->GetDimensions(tmp.Slice(wordStart, wordEnd-wordStart).GetData(), textWidth, textHeight);
                    textXPos += textWidth;
                }
            }
            //restore default color
            SetColour(basicFontColor);
        }
        else
        {
            // Get the center
            int textCenterX = 4;
            int textCenterY = (screenFrame.Height()-(margin*2)) / 2  - textHeight / 2;

            DrawWidgetText(tmp.GetData(),
                           screenFrame.xmin + textCenterX + margin,
                           screenFrame.ymin + textCenterY + margin);
        }

        if(blink && hasFocus)
        {
            int cursor = (int)cursorPosition - start;
            tmp.Truncate(cursor);

            // Figure out where to put the cursor.
            int width, height;
            GetFont()->GetDimensions(tmp, width, height);

            graphics2D->DrawLine((float)(screenFrame.xmin + margin + textCenterX + width + 1),
                                 (float)(screenFrame.ymin + margin + textCenterY),
                                 (float)(screenFrame.xmin + margin + textCenterX + width + 1),
                                 (float)(screenFrame.ymin + margin + textCenterY+height),
                                 GetFontColour());
        }
    } // End if stored.Length() > 0
    else if(blink && hasFocus)
    {
        graphics2D->DrawLine((float)screenFrame.xmin + margin + 5,
                             (float)screenFrame.ymin + margin + 4,
                             (float)screenFrame.xmin + margin + 5,
                             (float)screenFrame.ymax - (margin + 4),
                             GetFontColour());
    }
}

bool pawsEditTextBox::OnKeyDown(utf32_char code, utf32_char key, int modifiers)
{
    bool changed = false;
    blink = true;

    switch(key)
    {
        case CSKEY_DEL:
        {
            if(cursorPosition >= text.Length())
                break;
            if(cursorPosition != SIZET_NOT_FOUND)
            {
                if(pawsTextBox::CountCodePoints(text, 0, cursorPosition) - pawsTextBox::CountCodePoints(text, 0, start) < 5)
                    start = (int)pawsTextBox::RewindCodePoints(text, cursorPosition, 5);

                if(start < 0)
                    start = 0;

                if(text.Length() > 1)
                {
                    text.DeleteAt(cursorPosition, csUnicodeTransform::UTF8Skip((const utf8_char*)text.GetData() + cursorPosition, text.Length() - cursorPosition));
                }
                else
                {
                    text.Clear();
                }
            } //endif cursor position > 0
            changed = true;
            break;
        }


        case CSKEY_BACKSPACE:
            if(cursorPosition > text.Length())
                cursorPosition = text.Length();
            if(cursorPosition > 0)
            {
                cursorPosition -= csUnicodeTransform::UTF8Rewind((const utf8_char*)text.GetData() + cursorPosition, cursorPosition);
                if(pawsTextBox::CountCodePoints(text, 0, cursorPosition) - pawsTextBox::CountCodePoints(text, 0, start) < 5)
                    start = (int)pawsTextBox::RewindCodePoints(text, cursorPosition, 5);

                if(start < 0)
                    start = 0;

                if(text.Length() > 1)
                {
                    text.DeleteAt(cursorPosition, csUnicodeTransform::UTF8Skip((const utf8_char*)text.GetData() + cursorPosition, text.Length() - cursorPosition));
                }
                else
                {
                    text.Clear();
                }
            }
            changed = true;
            break;
            // End case CSKEY_BACKSPACE

        case CSKEY_LEFT:
            if(cursorPosition > text.Length())
                cursorPosition = text.Length();
            if(cursorPosition > 0)
                cursorPosition -= csUnicodeTransform::UTF8Rewind((const utf8_char*)text.GetData() + cursorPosition, cursorPosition);
            if(pawsTextBox::CountCodePoints(text, 0, cursorPosition) - pawsTextBox::CountCodePoints(text, 0, start) < 5)
                start = (int)pawsTextBox::RewindCodePoints(text, cursorPosition, 5);
            if(start < 0)
                start = 0;

            break;
        case CSKEY_RIGHT:
            if(cursorPosition > text.Length())
                cursorPosition = text.Length();
            if(cursorPosition < text.Length())
            {
                cursorPosition += csUnicodeTransform::UTF8Skip((const utf8_char*)text.GetData() + cursorPosition, text.Length() - cursorPosition);
            }
            break;
        case CSKEY_END:
            cursorPosition=1024;
            break;
        case CSKEY_HOME:
            cursorPosition=0;
            start=0;
            break;
        default:
            if(CSKEY_IS_SPECIAL(key))
                break;

            // Ignore ASCII control characters
            if(key < 128 && !isprint(key))
                break;
            if(maxLen && text.Length()==maxLen)
                break;

            utf8_char utf8Char[5];

            int charLen = csUnicodeTransform::UTF32to8(utf8Char, 5, &key, 1);
            if(cursorPosition >= text.Length())
            {
                text.Append((char*)utf8Char);
            }
            else
            {
                text.Insert(cursorPosition, (char*)utf8Char);
            }
            cursorPosition += charLen - 1;

            changed = true;
    }

    if(changed)
    {
        if(maxLen)
            text.Truncate(maxLen);
        if(subscribedVar)
        {
            PawsManager::GetSingleton().Publish(subscribedVar, text);
        }
        parent->OnChange(this);
    }

    if((key < 128 && !isprint(key)) || CSKEY_IS_SPECIAL(key))
        pawsWidget::OnKeyDown(code, key, modifiers);

    // if spellchecking is enabled do it at every key-press
    if(spellChecked)
    {
        checkSpelling();
    }

    return true;
}

void pawsEditTextBox::Clear()
{
    start = 0;
    cursorPosition = 0;

    text.Clear();
}

bool pawsEditTextBox::OnMouseDown(int button, int modifiers, int x, int y)
{
    x -= (screenFrame.xmin+margin+4); // Adjust x to be relative to the text box

    //Force the cursor to blink
    blink = true;

    // Basic comparisons to see if it was clicked after all the text or before all the text or neither
    if(x <= 0 || text.Length() == 0)
    {
        // The click where before all text. Moving cursor to start of text.
        start=0;
        cursorPosition=0;
    }
    else
    {
        int textWidth;
        int dummy;

        GetFont()->GetDimensions(text.GetData() + start, textWidth, dummy);
        if(x >= textWidth)
        {
            // The click where after all text. Moving cursor to end of text.
            cursorPosition = text.Length();
        }
        else
        {
            // Find the exact letter the mouse was clicked on.
            int xlast;
            int xlast2=0;
            int charLen = 0;

            csString sub;

            for(unsigned int i=start; i<text.Length(); i += charLen)
            {
                charLen = csUnicodeTransform::UTF8Skip((const utf8_char*)text.GetData() + i, text.Length() - i);
                xlast=xlast2;
                text.SubString(sub, i, charLen);
                GetFont()->GetDimensions(sub.GetData(), textWidth, dummy);
                xlast2+=textWidth;
                if(x >= xlast && x < xlast2)
                {
                    cursorPosition=i;
                    break;
                }
            }
        }
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

bool pawsEditTextBox::OnClipboard(const csString &content)
{
    Debug2(LOG_PAWS, 0, "Received from clipboard: %s", content.GetDataSafe());

    if(cursorPosition >= text.Length())
    {
        text.Append(content);
    }
    else
    {
        text.Insert(cursorPosition, content);
    }
    if(maxLen)
        text.Truncate(maxLen);
    cursorPosition += content.Length();

    if(subscribedVar)
    {
        PawsManager::GetSingleton().Publish(subscribedVar, text);
    }

    parent->OnChange(this);

    return true;
}

void pawsEditTextBox::SetMaxLength(unsigned int maxlen)
{
    maxLen = maxlen;
    if(maxLen)
        text.Truncate(maxlen);
}

void pawsEditTextBox::checkSpelling()
{
    if(spellChecker && (!text.IsEmpty()))
    {
        // clear the word-list
        words.Empty();
        // get the postitions of the words by checking for spaces in the widget's text
        Word tmpWord;
        csString tmpString;
        size_t oldSpace = 0;
        size_t foundSpace = text.Find(" ", oldSpace);
        while(foundSpace != SIZET_NOT_FOUND)
        {
            text.SubString(tmpString, oldSpace, foundSpace-oldSpace);
            // now do the spellchecking
            tmpWord.correct = spellChecker->correct(tmpString);
            tmpWord.endPos = foundSpace;
            // and save everything
            words.Push(tmpWord);
            oldSpace = foundSpace;
            foundSpace = text.Find(" ", oldSpace+1);
        }
        text.SubString(tmpString, oldSpace, text.Length()-oldSpace);
        // now do the spellchecking
        tmpWord.correct = spellChecker->correct(tmpString);
        tmpWord.endPos = text.Length();
        if(tmpWord.endPos > 0)
        {
            // save only if the word contains something
            words.Push(tmpWord);
        }
    }
    else
    {
        // no text in widget...still clean the array as the last key could have deleted the last char but the array still contains it
        words.Empty();
    }
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
pawsMultiLineTextBox::pawsMultiLineTextBox()
{
    scrollBar = 0;
    startLine = 0;
    canDrawLines = 0;
    usingScrollBar = true;
    maxHeight = 0;
    maxWidth = 0;
    factory = "pawsMultiLineTextBox";
}
pawsMultiLineTextBox::pawsMultiLineTextBox(const pawsMultiLineTextBox &origin)
    :pawsWidget(origin),
     text(origin.text),
     startLine(origin.startLine),
     canDrawLines(origin.canDrawLines),
     usingScrollBar(origin.usingScrollBar),
     maxWidth(origin.maxWidth),
     maxHeight(origin.maxHeight)
{
    for(unsigned int i = 0 ; i < origin.lines.GetSize(); i++)
        lines.Push(origin.lines[i]);

    scrollBar = 0;
    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.scrollBar == origin.children[i])
            scrollBar = dynamic_cast<pawsScrollBar*>(children[i]);
        if(scrollBar != 0)
            break;
    }
}
pawsMultiLineTextBox::~pawsMultiLineTextBox()
{

}

pawsScrollBar* pawsMultiLineTextBox::GetScrollBar()
{
    csArray<pawsWidget*>::Iterator it = children.GetIterator();
    while(it.HasNext())
    {
        pawsScrollBar* child = dynamic_cast<pawsScrollBar*>(it.Next());
        if(child)
            return child;
    }

    return NULL;
}

bool pawsMultiLineTextBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> textNode = node->GetNode("text");
    if(textNode)
    {
        csRef<iDocumentAttribute> textAttribute = textNode->GetAttribute("string");
        if(textAttribute)
        {
            SetText(textAttribute->GetValue());
        }
    }

    return true;
}

bool pawsMultiLineTextBox::PostSetup()
{
    scrollBar = GetScrollBar();
    if(!scrollBar)
    {
        scrollBar = new pawsScrollBar;
        scrollBar->SetParent(this);
        AddChild(scrollBar);
        scrollBar->SetTickValue(1.0);
        scrollBar->PostSetup();
    }

    scrollBar->SetRelativeFrame(defaultFrame.Width() - 40, 6, 24, defaultFrame.Height() - 12);
    scrollBar->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT);
    pawsWidget::Resize();

    scrollBar->Show();
    scrollBar->SetMaxValue(lines.GetSize() - canDrawLines);
    scrollBar->SetCurrentValue(0);

    return true;
}

void pawsMultiLineTextBox::Resize()
{
    pawsWidget::Resize();
    SetText(text);
}

void pawsMultiLineTextBox::OrganizeText(const char* newText)
{
    csString text(newText);

    // Check if we end with \n
    // Chomp if we do
    if(text.Length() > 0)
    {
        if(text.GetAt(text.Length()-1) == '\n')
        {
            text = text.Slice(0,text.Length()-1);
        }

    }
    text.ReplaceAll("\t","");//remove all the tabs in the string
    startLine = 0;

    GetFont()->GetMaxSize(maxWidth, maxHeight);

    canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    char* dummy = new char[text.Length() + 1];
    char* head = dummy;

    if(text.Length() > 0)
        strcpy(dummy, text);
    else
        dummy[0] = 0;

    int offSet = margin*2;
    if(usingScrollBar)
        offSet += 36;

    while(dummy)
    {
        // See how many characters can be drawn on a single line.
        int canDrawLength =  GetFont()->GetLength(dummy, screenFrame.Width()-offSet);

        // Check for linebreaks
        const char* pos=strchr(dummy,'\n');
        csString loopStr(dummy);
        if(pos)
        {
            while(pos)
            {
                csString temp(loopStr.GetData());
                csString temp2(loopStr.GetData());

                size_t pos1=temp.FindFirst('\n');

                temp= temp.Slice(0,pos1);
                temp2= temp2.Slice(pos1+1,(temp2.Length() - pos1-1));

                OrganizeText(temp.GetData());

                if(temp2.Length())
                {
                    loopStr = temp2;
                    pos=strchr(loopStr.GetData(),'\n');
                    if(!pos)
                    {
                        OrganizeText(temp2.GetData());
                    }
                }
                else
                    break;
            }
            break;
        }

        /// If it can fit the entire string then return.
        if(canDrawLength == (int)strlen(dummy))
        {
            lines.Push(dummy);
            break;
        }
        // We have to push in a new line to the lines bit.
        else
        {
            // Find out the nearest white space to break the line.
            int index = canDrawLength;

            while(index > 0 && dummy[index] != ' ')
            {
                index--;
            }

            if(index == 0)
                index = canDrawLength;

            // Index now points to the whitespace line so we can break it here.

            csString test;
            test.Append(dummy, index+1);
            dummy+=index+1;

            lines.Push(test);

        }
    }

    delete [] head;
}


void pawsMultiLineTextBox::SetText(const char* newText)
{
    lines.Empty();

    psString str(newText);
    size_t pos = str.FindSubString("\r");
    while(pos != SIZET_NOT_FOUND)
    {
        str = str.Slice(0,pos) + "\n" + str.Slice(pos+2,str.Length()-pos-2);
        pos = str.FindSubString("\r");
    }


    usingScrollBar = false;
    if(scrollBar) scrollBar->Hide();

    // try to fit the message into the window, filling 'lines' variable
    OrganizeText(str.GetData());

    // if it fits into the current window...
    if(canDrawLines >= lines.GetSize())
    {
        canDrawLines = lines.GetSize();
        if(scrollBar) //if there is a scrollbar we must setup it correctly else we will scroll in the void
        {
            scrollBar->SetMaxValue(0);
            scrollBar->SetCurrentValue(0);
        }
    }
    // if it doesn't then add a scrollbar
    else
    {
        usingScrollBar = true;
        lines.Empty();
        OrganizeText(str.GetData());
        if(scrollBar)
        {
            scrollBar->ShowBehind();
            scrollBar->SetMaxValue(lines.GetSize() - canDrawLines);
            scrollBar->SetCurrentValue(0);
        }
    }
    startLine = 0;
    text.Replace(str.GetData());
}

void pawsMultiLineTextBox::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    // This is called automatically whenever subscribed data is published.
    SetText(value.GetStr());
}


void pawsMultiLineTextBox::Draw()
{
    pawsWidget::Draw();
    pawsWidget::ClipToParent(false);

    int drawX = screenFrame.xmin+margin;
    int drawY = screenFrame.ymin+margin;

    if(!maxHeight && GetFont())
        GetFont()->GetMaxSize(maxWidth, maxHeight);

    if(!canDrawLines && maxHeight)
        canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    for(size_t x = startLine; x < (startLine+canDrawLines); x++)
    {
        if(x >= lines.GetSize())
            return;
        DrawWidgetText((const char*)lines[x], drawX, drawY);

        drawY+=maxHeight;
    }

}

bool pawsMultiLineTextBox::OnScroll(int /*direction*/, pawsScrollBar* widget)
{
    startLine = (int)widget->GetCurrentValue();
    return true;
}

bool pawsMultiLineTextBox::OnMouseDown(int button, int modifiers, int x, int y)
{
    if(button == csmbWheelUp)
    {
        if(scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    else if(button == csmbWheelDown)
    {
        if(scrollBar)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
        return true;
    }
    return pawsWidget::OnMouseDown(button, modifiers, x ,y);
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
pawsMultiPageTextBox::pawsMultiPageTextBox()
{
    scrollBar = 0;
    startLine = 0;
    canDrawLines = 0;
    maxHeight = 0;
    maxWidth = 0;
    factory = "pawsMultiPageTextBox";
    currentPageNum = 0;

}
pawsMultiPageTextBox::pawsMultiPageTextBox(const pawsMultiPageTextBox &origin)
    :pawsWidget(origin),
     text(origin.text),
     startLine(origin.startLine),
     canDrawLines(origin.canDrawLines),
     usingScrollBar(origin.usingScrollBar),
     maxWidth(origin.maxWidth),
     maxHeight(origin.maxHeight)
{
    currentPageNum = 0;
    for(unsigned int i = 0 ; i < origin.lines.GetSize(); i++)
        lines.Push(origin.lines[i]);

    scrollBar = 0;

    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.scrollBar == origin.children[i])
            scrollBar = dynamic_cast<pawsScrollBar*>(children[i]);
        if(scrollBar != 0)
            break;

    }
}

pawsMultiPageTextBox::~pawsMultiPageTextBox()
{

}

pawsScrollBar* pawsMultiPageTextBox::GetScrollBar()
{
    csArray<pawsWidget*>::Iterator it = children.GetIterator();
    while(it.HasNext())
    {
        pawsScrollBar* child = dynamic_cast<pawsScrollBar*>(it.Next());
        if(child)
            return child;
    }

    return NULL;
}

bool pawsMultiPageTextBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> textNode = node->GetNode("text");
    if(textNode)
    {
        csRef<iDocumentAttribute> textAttribute = textNode->GetAttribute("string");
        if(textAttribute)
        {
            SetText(textAttribute->GetValue());
        }
    }

    return true;
}

bool pawsMultiPageTextBox::PostSetup()
{
    // removed scrollbar
    /*   scrollBar = GetScrollBar();
       if (!scrollBar)
       {
           scrollBar = new pawsScrollBar;
           scrollBar->SetParent( this );
           AddChild( scrollBar );
           scrollBar->SetTickValue( 1.0 );
           scrollBar->PostSetup();
       }

       scrollBar->SetRelativeFrame( defaultFrame.Width() - 40, 6, 24, defaultFrame.Height() - 12 );
       scrollBar->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_RIGHT);
       pawsWidget::Resize();

       scrollBar->Show();
       scrollBar->SetMaxValue(lines.GetSize() - canDrawLines );
       scrollBar->SetCurrentValue(0);
    */
    return true;
}

void pawsMultiPageTextBox::Resize()
{
    pawsWidget::Resize();
    SetText(text);
}

void pawsMultiPageTextBox::OrganizeText(const char* newText)
{
    int greyedOut = 0;
    csString text(newText);

    // Check if we end with \n
    // Chomp if we do
    if(text.Length() > 0)
    {
        if(text.GetAt(text.Length()-1) == '\n')
        {
            text = text.Slice(0,text.Length()-1);
        }

    }
    text.ReplaceAll("\t","");//remove all the tabs in the string
    startLine = 0;

    GetFont()->GetMaxSize(maxWidth, maxHeight);

    canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    char* dummy = new char[text.Length() + 1];
    char* head = dummy;

    if(text.Length() > 0)
        strcpy(dummy, text);
    else
        dummy[0] = 0;

    int offSet = margin*2;
    if(usingScrollBar)
        offSet += 36;


    while(dummy)
    {
        // See how many characters can be drawn on a single line.
        int canDrawLength =  GetFont()->GetLength(dummy, screenFrame.Width()-offSet);

        // Check for linebreaks
        const char* pos=strchr(dummy,'\n');
        csString loopStr(dummy);
        if(pos)
        {
            while(pos)
            {
                csString temp(loopStr.GetData());
                csString temp2(loopStr.GetData());

                size_t pos1=temp.FindFirst('\n');

                temp= temp.Slice(0,pos1);
                temp2= temp2.Slice(pos1+1,(temp2.Length() - pos1-1));

                OrganizeText(temp.GetData());

                if(temp2.Length())
                {
                    loopStr = temp2;
                    pos=strchr(loopStr.GetData(),'\n');
                    if(!pos)
                    {
                        OrganizeText(temp2.GetData());
                    }
                }
                else
                    break;
            }
            break;
        }

        /// If it can fit the entire string then return.
        if(canDrawLength == (int)strlen(dummy))
        {
            // Check for crafting books to grey out the text of transform players cannot do
            if(greyedOut==0)
            {
                char* p1 = NULL,
                      *p2 = NULL;

                p1=strstr(dummy, "   With higher ");
                p2=strstr(dummy, "   With Higher ");
                if(p1!=NULL || p2!=NULL)
                {
                    greyedOut = 1;
                }
            }
            if(greyedOut)
            {
                csString temp;
                temp.Append("\r");
                temp.Append(dummy);
                lines.Push(temp);
                if(strchr(dummy, '.')!=NULL)
                {
                    greyedOut = 0;
                }
                temp.Empty();
            }
            else
            {
                lines.Push(dummy);
            }
            break;
        }
        // We have to push in a new line to the lines bit.
        else
        {
            // Find out the nearest white space to break the line.
            int index = canDrawLength;

            while(index > 0 && dummy[index] != ' ')
            {
                index--;
            }

            if(index == 0)
                index = canDrawLength;

            // Index now points to the whitespace line so we can break it here.

            csString test;
            test.Append(dummy, index+1);
            dummy+=index+1;
            if(greyedOut==0)
            {
                int p1,
                    p2;
                p1=test.Find("   With higher ");
                p2=test.Find("   With Higher ");
                if(p1==0 || p2==0)
                {
                    greyedOut = 1;
                }
            }
            if(greyedOut)
            {
                test.Insert(0, "\r");
                if(test.FindFirst('.', 1)==test.Length()-1)
                {
                    greyedOut = 0;
                }
            }
            lines.Push(test);
        }
    }


    delete [] head;
}


void pawsMultiPageTextBox::SetText(const char* newText)
{
    lines.Empty();
    numPages = 0;
    currentPageNum = 0;
    psString str(newText);
    size_t pos = str.FindSubString("\r");
    while(pos != SIZET_NOT_FOUND)
    {
        str = str.Slice(0,pos) + "\n" + str.Slice(pos+2,str.Length()-pos-2);
        pos = str.FindSubString("\r");
    }


    usingScrollBar = false;
    if(scrollBar) scrollBar->Hide();


    OrganizeText(str.GetData());

    numPages = lines.GetSize() / canDrawLines;

    if(canDrawLines >= lines.GetSize())
    {
        canDrawLines = lines.GetSize();
        // removed
        /* if(scrollBar) //if there is a scrollbar we must setup it correctly else we will scroll in the void
         {
             scrollBar->SetMaxValue(0);
             scrollBar->SetCurrentValue(0);
         }*/
    }
    else
    {
        //  usingScrollBar = true;
        lines.Empty();
        OrganizeText(str.GetData());
        // removed
        /*  if ( scrollBar )
          {
              scrollBar->ShowBehind();
              scrollBar->SetMaxValue(lines.GetSize() / canDrawLines );
              scrollBar->SetCurrentValue(0);
          }*/
    }
    startLine = 0;
    text.Replace(str.GetData());
}

void pawsMultiPageTextBox::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    // This is called automatically whenever subscribed data is published.
    SetText(value.GetStr());
}


void pawsMultiPageTextBox::Draw()
{
    pawsWidget::Draw();
    pawsWidget::ClipToParent(false);

    int drawX = screenFrame.xmin+margin;
    int drawY = screenFrame.ymin+margin;

    if(!maxHeight && GetFont())
        GetFont()->GetMaxSize(maxWidth, maxHeight);

    if(!canDrawLines && maxHeight)
        canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    for(size_t x = startLine; x < (startLine+canDrawLines); x++)
    {

        if(x >= lines.GetSize())
            return;

        DrawWidgetText((const char*)lines[x], drawX, drawY);
        drawY+=maxHeight;
    }

}

bool pawsMultiPageTextBox::OnScroll(int /*direction*/, pawsScrollBar* widget)
{
    // removed, see pawsMultiLineTextBox::OnScroll
    return true;
}

bool pawsMultiPageTextBox::OnMouseDown(int button, int modifiers, int x, int y)
{
    // removed for the moment

    /*   if (button == csmbWheelUp)
       {
           if (scrollBar)
               scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
           return true;
       }
       else if (button == csmbWheelDown)
       {
           if (scrollBar)
               scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + MULTILINE_TEXTBOX_MOUSE_SCROLL_AMOUNT);
           return true;
       }*/
    return pawsWidget::OnMouseDown(button, modifiers, x ,y);
}


void pawsMultiPageTextBox::SetCurrentPageNum(int n)
{
    currentPageNum = n;
    startLine = n*canDrawLines;
}

int pawsMultiPageTextBox::GetCurrentPageNum()
{
    return currentPageNum;
}

int pawsMultiPageTextBox::GetNumPages()
{
    return numPages;
}
//================================================================
//
//================================================================


pawsFadingTextBox::pawsFadingTextBox()
{
    start = 0;
    color = scolor = 0;
    firstFont = NULL;
    font = NULL;
    org_color = 0;
    ymod = 0;
    time = 0;
    fadetime = 0;
    factory = "pawsFadingTextBox";
}
pawsFadingTextBox::pawsFadingTextBox(const pawsFadingTextBox &origin):
    pawsWidget(origin),
    text(origin.text),
    first(origin.first),
    firstFont(origin.firstFont),
    font(origin.font),
    org_color(origin.org_color),
    color(origin.color),
    scolor(origin.scolor),
    start(origin.start),
    ymod(origin.ymod),
    time(origin.time),
    fadetime(origin.fadetime)
{

}
void pawsFadingTextBox::SetText(const char* newtext, iFont* font1,iFont* font2, int color,int time,int fadetime)
{
    // Seperate the first and the rest
    text = newtext;
    first = "XXX";

    first.SetAt(0,text.GetAt(0));
    first.SetAt(1,0);
    text = text.Slice(1,text.Length() -1);

    // Add the other stuff
    firstFont = font1;
    font = font2;
    start = csGetTicks(); //Start the effect

    this->org_color = color;
    this->time = time;
    this->fadetime = fadetime;

}

void pawsFadingTextBox::GetSize(int &w, int &h)
{
    int iw=0,ih=0;
    int dummy=0;
    // Get the sizes
    firstFont->GetDimensions(first,iw,ih,dummy);
    font->GetDimensions(text,w,h,dummy);

    if(iw > w) // Stretch the size to have space for the first letter
        w = iw;
    if(ih > h)
        h = ih;

    w += BORDER_SIZE*2; // Add the border pixels
    h += BORDER_SIZE*2; // Add the border pixels
}

void pawsFadingTextBox::Draw()
{
    if(!text || text.Length() == 0)
        return;

    // Calculate fading
    int alpha = 0;
    if(csGetTicks() > start + time)
    {
        int elapsed = csGetTicks() - (start + time);

        float multi = float(elapsed) / float(fadetime);
        alpha = int(multi * float(255));
    }

    int r=0,g=0,b=0,a=0;
    graphics2D->GetRGB(org_color,r,g,b,a);
    a -= alpha; // Apply alpha

    if(a < 1)
    {
        // Destroy the widget
        DeleteYourself();
        return;
    }

    int newymod = alpha / 2;

    MoveDelta(0,newymod - ymod); // Move downwards
    ymod = newymod;

    // Prepare to paint the text
    color  = graphics2D->FindRGB(r,g,b,a); // Color on text
    scolor = graphics2D->FindRGB(0,0,0,a); // Color on "shadow"

    int iw=0; // Width on the first letter
    int dummy=-1; // Used as return value for 'desc' and the height

    firstFont->GetDimensions(first,iw,dummy,dummy);

    // Draw
    //ClipToParent();

    DrawBorderText(first,firstFont,0);
    DrawBorderText(text,font,iw);
}

void pawsFadingTextBox::Fade()
{
    if(start < csGetTicks() - time) // Already fading?
        return;

    start = csGetTicks() - time;
}

void pawsFadingTextBox::DrawBorderText(const char* text, iFont* font,int x)
{
    int y = 0;
    x += screenFrame.xmin;
    y += screenFrame.ymin;

    graphics2D->Write(font,x+BORDER_SIZE,y+BORDER_SIZE,scolor,-1,text); // Right down
    graphics2D->Write(font,x-BORDER_SIZE,y+BORDER_SIZE,scolor,-1,text); // Left down
    graphics2D->Write(font,x+BORDER_SIZE,y-BORDER_SIZE,scolor,-1,text); // Right up
    graphics2D->Write(font,x-BORDER_SIZE,y-BORDER_SIZE,scolor,-1,text); // Left up

    graphics2D->Write(font,x,y+BORDER_SIZE,scolor,-1,text); // Down
    graphics2D->Write(font,x,y-BORDER_SIZE,scolor,-1,text); // Up
    graphics2D->Write(font,x+BORDER_SIZE,y,scolor,-1,text); // Right
    graphics2D->Write(font,x-BORDER_SIZE,y,scolor,-1,text); // Left

    // Draw the letter
    graphics2D->Write(font,x,y,color,-1,text);
}
pawsDocumentView::pawsDocumentView()
{

}

pawsDocumentView::pawsDocumentView(const pawsDocumentView &origin)
    :pawsMultiLineTextBox(origin)
{
    for(unsigned int i = 0 ; i < origin.picsInfo.GetSize() ; i++)
        picsInfo.Push(origin.picsInfo[i]);
}

pawsDocumentView::~pawsDocumentView()
{

}
void pawsDocumentView::Draw()
{
    pawsWidget::Draw();
    pawsWidget::ClipToParent(false);

    int drawX = screenFrame.xmin+margin;
    int drawY = screenFrame.ymin+margin;

    if(!maxHeight && GetFont())
        GetFont()->GetMaxSize(maxWidth, maxHeight);

    if(!canDrawLines && maxHeight)
        canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    int offSet = margin*2;
    if(usingScrollBar)
        offSet += 36;

    //lets check if the first visibe line is actually a line for a image:
    if((lines.GetSize()>startLine) && (lines[startLine].StartsWith("#pic#")))
    {
        // nice...this means that the first line might not actually be the first line defining the image but one of the lines "coverd" by the image
        // so lets find out where the picture really starts
        if(startLine > 0)  // okay...if we really start at the first line it's fine to assume that this is the first line of the image as well ;)
        {
            int picStart = startLine - 1;
            while((picStart >= 0) && (lines[picStart]==lines[startLine]))
            {
                picStart--;
            }
            // how many lines are invisble?
            int invisibleLines;
            if(picStart == 0)
            {
                // all lines before are from this image
                invisibleLines = startLine;
            }
            else
            {
                // oh..and yes...we are one line before the first picture line...so lets add the one again.
                invisibleLines = startLine - picStart + 1;
            }
            // of course we don't really want to know how many lines we have to move the image but how many pixels
            drawY-= (invisibleLines * maxHeight);
        }
    }

    for(size_t x = startLine; x < (startLine+canDrawLines); x++)
    {
        if(x >= lines.GetSize())
            return;

        csString &currentStr = lines[x];
        if(!currentStr.StartsWith("#pic#"))
        {
            DrawWidgetText((const char*)lines[x], drawX, drawY);

            drawY+=maxHeight;
        }
        else
        {
            csArray<int> indices;
            csString sub;
            currentStr.SubString(sub, 5);
            int index = 0;

            for(unsigned int i = 0 ; i < sub.Length() ; i++)
            {
                if(sub[i] != ',')
                {
                    index *= 10;
                    index += sub[i]- '0';
                }
                else
                {
                    indices.Push(index);
                    index = 0;
                }
            }
            indices.Push(index);
            //draw pictures
            PictureInfo &pi = picsInfo[indices[0]];
            unsigned int sz = indices.GetSize();
            unsigned int sx = drawX;
            if(pi.align == 0) //align left
                sx += pi.padding[0];
            else
            {
                if(pi.align == 1)//align center
                {
                    unsigned int w = pi.width;
                    unsigned int pad = pi.padding[0]+pi.padding[2];
                    unsigned int dsx = (screenFrame.Width() - w * sz - pad * (sz - 1) - offSet + margin)/2  ;
                    sx = screenFrame.xmin + dsx;
                }
                else
                {
                    if(pi.align == 2)//align right
                    {
                        unsigned int w = pi.width;
                        unsigned int pad = pi.padding[0]+pi.padding[2];
                        unsigned int dsx = w * sz + pad * (sz - 1) + pi.padding[2] + margin;
                        if(usingScrollBar)
                            dsx += 36;
                        sx += (screenFrame.Width() - dsx);
                    }
                    else sx += pi.padding[0];
                }
            }

            unsigned int sy = drawY + pi.padding[1];

            for(unsigned int i = 0 ; i< sz ; i++)
            {
                //draw an image at sx sy
                csRef<iPawsImage> drawable;
                if(picsInfo[i].srcString.StartsWith("/"))
                {
                    drawable.AttachNew(new pawsImageDrawable(picsInfo[indices[i]].srcString.GetData(), picsInfo[indices[i]].srcString.GetData()));
                    PawsManager::GetSingleton().GetTextureManager()->Remove(picsInfo[i].srcString.GetData());
                    PawsManager::GetSingleton().GetTextureManager()->AddPawsImage(drawable);
                }
                else
                {
                    //a resource
                    drawable =  PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(picsInfo[indices[i]].srcString.GetData());
                }
                // only draw if the image was found
                if(drawable)
                {
                    drawable->Draw(sx, sy, pi.width, pi.height);
                }
                sx += pi.padding[0] + pi.padding[2] + pi.width;
            }
            drawY += pi.height + pi.padding[3];
        }

        for(unsigned i = x+1 ; i < (startLine+canDrawLines) ; i++)
        {
            if(i >= lines.GetSize()) break;
            if(currentStr == lines[i])
                ++x;
            else break;
        }

    }
}

unsigned int pawsDocumentView::ProcessPictureInfo(iDocumentNode* node)
{
    //process a picture node. If the src attribute of the node contains more than one picture location,
    //this function would fill create more than one PictureInfo structure into picsInfo array.
    csString srcs = node->GetAttributeValue("src");

    //all the pictures in current node hold the same format
    unsigned int align = node->GetAttributeValueAsInt("align");
    unsigned int width = node->GetAttributeValueAsInt("width");
    unsigned int height = node->GetAttributeValueAsInt("height");
    csString paddingStr = node->GetAttributeValue("padding");
    paddingStr = paddingStr.Trim();
    unsigned int padding[4] = {0, 0, 0, 0};
    unsigned int index = 0;

    for(unsigned int i = 0 ; i < paddingStr.Length() ; i++)
    {
        if(paddingStr[i] != ' ')
        {
            //none numbers of padding parameters are not checked here
            padding[index] *= 10;
            padding[index] += paddingStr[i]- '0';
        }
        else ++index;
    }

    csString temp = srcs;
    size_t pos;
    size_t sz = 0;

    //process the src attribute and push corresponding PictureInfo into picsInfo array
    while((pos = temp.FindFirst(';')) != SIZET_NOT_FOUND)
    {
        csString sub;
        temp.SubString(sub,0,pos);
        PictureInfo pi;
        pi.align = align;
        pi.height = height;
        pi.padding[0] = padding[0];
        pi.padding[1] = padding[1];
        pi.padding[2] = padding[2];
        pi.padding[3] = padding[3];
        pi.srcString = sub;
        pi.width = width;
        picsInfo.Push(pi);
        temp.SubString(sub, pos+1);
        temp = sub;
        sz++;
    }
    return sz;
}
void pawsDocumentView::OrganizeContent(const char* newtext)
{
    csString str(newtext);
    csRef<iDocumentNode> root = ParseStringGetNode(str, "Contents", false);

    if(root == 0)
        return;

    csRef<iDocumentNodeIterator> childIter = root->GetNodes();
    picsInfo.Empty();

    int offSet = margin*2;
    if(usingScrollBar)
        offSet += 36;

    while(childIter->HasNext())
    {
        csRef<iDocumentNode> childNode = childIter->Next();
        csString type(childNode->GetAttributeValue("type"));

        if(type == "text")
        {
            //a text node, processed by the based method
            csString textpart = childNode->GetContentsValue();
            OrganizeText(textpart.GetData());
        }
        else if(type == "pic") //a picture node
        {
            //process the picture node,return how many pictures in the row
            unsigned int sz = ProcessPictureInfo(childNode);
            if(sz == 0)
                continue;

            unsigned int picIndex = picsInfo.GetSize() - 1;
            unsigned int width = picsInfo[picIndex].width;
            unsigned int height = picsInfo[picIndex].height;
            unsigned int* padding = picsInfo[picIndex].padding;
            unsigned int widthPerPic = padding[0] + padding[2] + width;

            unsigned int needLength = widthPerPic*sz;
            unsigned int canDrawLength = screenFrame.Width() - offSet;
            GetFont()->GetMaxSize(maxWidth, maxHeight);
            if(needLength <= canDrawLength)
            {
                //all pictures could be drawn in a row
                csString infoStr="#pic#";
                for(; sz > 0; sz--)
                {
                    unsigned int index = picIndex - sz + 1;


                    if(sz != 1)
                        infoStr = infoStr.Append(index) + ",";
                    else infoStr.Append(index);
                }
                unsigned int occupied_lines = (padding[1] + padding[3] + height)/maxHeight + 1;
                for(unsigned int i = 0 ; i < occupied_lines ; i++)
                    lines.Push(infoStr);
            }
            else
            {
                //all pictures defined in this node could not be drawn in one row
                unsigned int howmanycandraw = canDrawLength/widthPerPic;
                if(howmanycandraw == 0) //the box could not hold the picture in, then the picture will no show
                    continue;
                canDrawLength = howmanycandraw * widthPerPic;
                unsigned int rows = needLength%canDrawLength ? needLength/canDrawLength + 1 : needLength/canDrawLength;

                for(; rows>0; rows--)
                {
                    csString infoStr="#pic#";
                    unsigned int temp = howmanycandraw;

                    for(; temp>0; temp--)
                    {
                        unsigned int index = picIndex - sz + 1;
                        if(index > picIndex)
                            break;

                        if(temp != 1 && index != picIndex)
                            infoStr = infoStr.Append(index) + ",";
                        else infoStr.Append(index);
                        sz--;
                    }

                    unsigned int occupied_lines = (padding[1] + padding[3] + height)/maxHeight + 1;
                    for(unsigned int i = 0 ; i < occupied_lines ; i++)
                        lines.Push(infoStr);
                }
            }//end else
        }//end pic type
    }
}

void pawsDocumentView::Resize()
{
    pawsWidget::Resize();
    SetText(text);
}
void pawsDocumentView::SetText(const char* newtext)
{
    lines.Empty();

    psString str(newtext);
    size_t pos = str.FindSubString("\r");
    while(pos != SIZET_NOT_FOUND)
    {
        str = str.Slice(0, pos) + "\n" + str.Slice(pos+2, str.Length()-pos-2);
        pos = str.FindSubString("\r");
    }


    usingScrollBar = false;
    if(scrollBar) scrollBar->Hide();

    OrganizeContent(str.GetData());

    if(canDrawLines >= lines.GetSize())
    {
        canDrawLines = lines.GetSize();
        if(scrollBar) //if there is a scrollbar we must setup it correctly else we will scroll in the void
        {
            scrollBar->SetMaxValue(0);
            scrollBar->SetCurrentValue(0);
        }
    }
    else
    {
        usingScrollBar = true;
        lines.Empty();
        OrganizeContent(str.GetData());//the scroll bar will show, we should organize the content again
        if(scrollBar)
        {
            scrollBar->ShowBehind();
            scrollBar->SetMaxValue(lines.GetSize() - canDrawLines);
            scrollBar->SetCurrentValue(0);
        }
    }
    startLine = 0;
    text.Replace(str.GetData());
}



pawsMultiPageDocumentView::pawsMultiPageDocumentView()
{

}

pawsMultiPageDocumentView::pawsMultiPageDocumentView(const pawsMultiPageDocumentView &origin)
    :pawsMultiPageTextBox(origin)
{
    for(unsigned int i = 0 ; i < origin.picsInfo.GetSize() ; i++)
        picsInfo.Push(origin.picsInfo[i]);
}

pawsMultiPageDocumentView::~pawsMultiPageDocumentView()
{

}
void pawsMultiPageDocumentView::Draw()
{
    int fontColor = GetFontColour();

    pawsWidget::Draw();
    pawsWidget::ClipToParent(false);

    int drawX = screenFrame.xmin+margin;
    int drawY = screenFrame.ymin+margin;

    if(!maxHeight && GetFont())
        GetFont()->GetMaxSize(maxWidth, maxHeight);

    if(!canDrawLines && maxHeight)
        canDrawLines = (screenFrame.Height()-(margin*2)) / maxHeight;

    int offSet = margin*2;
    if(usingScrollBar)
        offSet += 36;

    //lets check if the first visibe line is actually a line for a image:
    if((lines.GetSize()>startLine) && (lines[startLine].StartsWith("#pic#")))
    {
        // nice...this means that the first line might not actually be the first line defining the image but one of the lines "coverd" by the image
        // so lets find out where the picture really starts
        if(startLine > 0)  // okay...if we really start at the first line it's fine to assume that this is the first line of the image as well ;)
        {
            int picStart = startLine - 1;
            while((picStart >= 0) && (lines[picStart]==lines[startLine]))
            {
                picStart--;
            }
            // how many lines are invisble?
            int invisibleLines;
            if(picStart == 0)
            {
                // all lines before are from this image
                invisibleLines = startLine;
            }
            else
            {
                // oh..and yes...we are one line before the first picture line...so lets add the one again.
                invisibleLines = startLine - picStart + 1;
            }
            // of course we don't really want to know how many lines we have to move the image but how many pixels
            drawY-= (invisibleLines * maxHeight);
        }
    }

    for(size_t x = startLine; x < (startLine+canDrawLines); x++)
    {
        if(x >= lines.GetSize())
            return;

        csString &currentStr = lines[x];
        if(!currentStr.StartsWith("#pic#"))
        {
            csString temp;

            temp.Append(lines[x]);
            //This is special case logic to grey out lines in the Craft Books that
            //tell the player about things they can't yet do. the marking of the lines with "\r"'s is done in pawsMultiLineTextBox::OrganizeText
            if(lines[x].FindStr("\r", 0)==0)
            {
                temp.DeleteAt(0,1);
                SetColour(0x888888);
            }
            DrawWidgetText(temp, drawX, drawY);
            SetColour(fontColor);
            temp.Empty();
            drawY+=maxHeight;
        }
        else
        {
            csArray<int> indices;
            csString sub;
            currentStr.SubString(sub, 5);
            int index = 0;

            for(unsigned int i = 0 ; i < sub.Length() ; i++)
            {
                if(sub[i] != ',')
                {
                    index *= 10;
                    index += sub[i]- '0';
                }
                else
                {
                    indices.Push(index);
                    index = 0;
                }
            }
            indices.Push(index);
            //draw pictures
            PictureInfo &pi = picsInfo[indices[0]];
            unsigned int sz = indices.GetSize();
            unsigned int sx = drawX;
            if(pi.align == 0) //align left
                sx += pi.padding[0];
            else
            {
                if(pi.align == 1)//align center
                {
                    unsigned int w = pi.width;
                    unsigned int pad = pi.padding[0]+pi.padding[2];
                    unsigned int dsx = (screenFrame.Width() - w * sz - pad * (sz - 1) - offSet + margin)/2  ;
                    sx = screenFrame.xmin + dsx;
                }
                else
                {
                    if(pi.align == 2)//align right
                    {
                        unsigned int w = pi.width;
                        unsigned int pad = pi.padding[0]+pi.padding[2];
                        unsigned int dsx = w * sz + pad * (sz - 1) + pi.padding[2] + margin;
                        if(usingScrollBar)
                            dsx += 36;
                        sx += (screenFrame.Width() - dsx);
                    }
                    else sx += pi.padding[0];
                }
            }

            unsigned int sy = drawY + pi.padding[1];

            for(unsigned int i = 0 ; i< sz ; i++)
            {
                //draw an image at sx sy
                csRef<iPawsImage> drawable;
                if(picsInfo[i].srcString.StartsWith("/"))
                {
                    drawable.AttachNew(new pawsImageDrawable(picsInfo[indices[i]].srcString.GetData(), picsInfo[indices[i]].srcString.GetData()));
                    PawsManager::GetSingleton().GetTextureManager()->Remove(picsInfo[i].srcString.GetData());
                    PawsManager::GetSingleton().GetTextureManager()->AddPawsImage(drawable);
                }
                else
                {
                    //a resource
                    drawable =  PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(picsInfo[indices[i]].srcString.GetData());
                }
                // only draw if the image was found
                if(drawable)
                {
                    drawable->Draw(sx, sy, pi.width, pi.height);
                }
                sx += pi.padding[0] + pi.padding[2] + pi.width;
            }
            drawY += pi.height + pi.padding[3];
        }

        for(unsigned i = x+1 ; i < (startLine+canDrawLines) ; i++)
        {
            if(i >= lines.GetSize()) break;
            if(currentStr == lines[i])
                ++x;
            else break;
        }

    }
//reassert default color
    SetColour(fontColor);
}

unsigned int pawsMultiPageDocumentView::ProcessPictureInfo(iDocumentNode* node)
{
    //process a picture node. If the src attribute of the node contains more than one picture location,
    //this function would fill create more than one PictureInfo structure into picsInfo array.
    csString srcs = node->GetAttributeValue("src");

    //all the pictures in current node hold the same format
    unsigned int align = node->GetAttributeValueAsInt("align");
    unsigned int width = node->GetAttributeValueAsInt("width");
    unsigned int height = node->GetAttributeValueAsInt("height");
    csString paddingStr = node->GetAttributeValue("padding");
    paddingStr = paddingStr.Trim();
    unsigned int padding[4] = {0, 0, 0, 0};
    unsigned int index = 0;

    for(unsigned int i = 0 ; i < paddingStr.Length() ; i++)
    {
        if(paddingStr[i] != ' ')
        {
            //none numbers of padding parameters are not checked here
            padding[index] *= 10;
            padding[index] += paddingStr[i]- '0';
        }
        else ++index;
    }

    csString temp = srcs;
    size_t pos;
    size_t sz = 0;

    //process the src attribute and push corresponding PictureInfo into picsInfo array
    while((pos = temp.FindFirst(';')) != SIZET_NOT_FOUND)
    {
        csString sub;
        temp.SubString(sub,0,pos);
        PictureInfo pi;
        pi.align = align;
        pi.height = height;
        pi.padding[0] = padding[0];
        pi.padding[1] = padding[1];
        pi.padding[2] = padding[2];
        pi.padding[3] = padding[3];
        pi.srcString = sub;
        pi.width = width;
        picsInfo.Push(pi);
        temp.SubString(sub, pos+1);
        temp = sub;
        sz++;
    }
    return sz;
}
void pawsMultiPageDocumentView::OrganizeContent(const char* newtext)
{
    csString str(newtext);
    csRef<iDocumentNode> root = ParseStringGetNode(str, "Contents", false);

    if(root == 0)
        return;

    csRef<iDocumentNodeIterator> childIter = root->GetNodes();
    picsInfo.Empty();

    int offSet = margin*2;
    if(usingScrollBar)
        offSet += 36;

    while(childIter->HasNext())
    {
        csRef<iDocumentNode> childNode = childIter->Next();
        csString type(childNode->GetAttributeValue("type"));

        if(type == "text")
        {
            //a text node, processed by the based method
            csString textpart = childNode->GetContentsValue();
            OrganizeText(textpart.GetData());
        }
        else if(type == "pic") //a picture node
        {
            //process the picture node,return how many pictures in the row
            unsigned int sz = ProcessPictureInfo(childNode);
            if(sz == 0)
                continue;

            unsigned int picIndex = picsInfo.GetSize() - 1;
            unsigned int width = picsInfo[picIndex].width;
            unsigned int height = picsInfo[picIndex].height;
            unsigned int* padding = picsInfo[picIndex].padding;
            unsigned int widthPerPic = padding[0] + padding[2] + width;

            unsigned int needLength = widthPerPic*sz;
            unsigned int canDrawLength = screenFrame.Width() - offSet;
            GetFont()->GetMaxSize(maxWidth, maxHeight);
            if(needLength <= canDrawLength)
            {
                //all pictures could be drawn in a row
                csString infoStr="#pic#";
                for(; sz > 0; sz--)
                {
                    unsigned int index = picIndex - sz + 1;


                    if(sz != 1)
                        infoStr = infoStr.Append(index) + ",";
                    else infoStr.Append(index);
                }
                unsigned int occupied_lines = (padding[1] + padding[3] + height)/maxHeight + 1;
                for(unsigned int i = 0 ; i < occupied_lines ; i++)
                    lines.Push(infoStr);
            }
            else
            {
                //all pictures defined in this node could not be drawn in one row
                unsigned int howmanycandraw = canDrawLength/widthPerPic;
                if(howmanycandraw == 0) //the box could not hold the picture in, then the picture will no show
                    continue;
                canDrawLength = howmanycandraw * widthPerPic;
                unsigned int rows = needLength%canDrawLength ? needLength/canDrawLength + 1 : needLength/canDrawLength;

                for(; rows>0; rows--)
                {
                    csString infoStr="#pic#";
                    unsigned int temp = howmanycandraw;

                    for(; temp>0; temp--)
                    {
                        unsigned int index = picIndex - sz + 1;
                        if(index > picIndex)
                            break;

                        if(temp != 1 && index != picIndex)
                            infoStr = infoStr.Append(index) + ",";
                        else infoStr.Append(index);
                        sz--;
                    }

                    unsigned int occupied_lines = (padding[1] + padding[3] + height)/maxHeight + 1;
                    for(unsigned int i = 0 ; i < occupied_lines ; i++)
                        lines.Push(infoStr);
                }
            }//end else
        }//end pic type
    }
}

void pawsMultiPageDocumentView::Resize()
{
    pawsWidget::Resize();
    SetText(text);
}
void pawsMultiPageDocumentView::SetText(const char* newtext)
{
    lines.Empty();

    psString str(newtext);
    size_t pos = str.FindSubString("\r");
    while(pos != SIZET_NOT_FOUND)
    {
        str = str.Slice(0, pos) + "\n" + str.Slice(pos+2, str.Length()-pos-2);
        pos = str.FindSubString("\r");
    }


    usingScrollBar = false;
    if(scrollBar) scrollBar->Hide();

    OrganizeContent(str.GetData());

    if(canDrawLines >= lines.GetSize())
    {
        canDrawLines = lines.GetSize();
        if(scrollBar) //if there is a scrollbar we must setup it correctly else we will scroll in the void
        {
            scrollBar->SetMaxValue(0);
            scrollBar->SetCurrentValue(0);
        }
    }
    else
    {
        usingScrollBar = true;
        lines.Empty();
        OrganizeContent(str.GetData());//the scroll bar will show, we should organize the content again
        if(scrollBar)
        {
            scrollBar->ShowBehind();
            scrollBar->SetMaxValue(lines.GetSize() - canDrawLines);
            scrollBar->SetCurrentValue(0);
        }
    }
    startLine = 0;
    text.Replace(str.GetData());
    if(canDrawLines)
        numPages = lines.GetSize() / canDrawLines;
}

