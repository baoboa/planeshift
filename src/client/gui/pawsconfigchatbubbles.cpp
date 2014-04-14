/*
 * pawsconfigchatbubbles.cpp - Author: Steven Patrick
 *
 * Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//CS INCLUDES
#include <psconfig.h>

//PAWS INCLUDES
#include "pawsconfigchatbubbles.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "paws/pawstextbox.h"
#include "paws/pawscrollbar.h"
#include "paws/pawscombo.h"

//CLIENT INCLUDES
#include "../globals.h"
#include "chatbubbles.h"
#include "net/messages.h"

// Used to set default config files if the userdata ones are not found.
#define SAVE_USER     true
#define DEFAULT_FILE  "/planeshift/data/options/chatbubbles_def.xml"
#define USER_FILE     "/planeshift/userdata/options/chatbubbles.xml"

#define LINES_IN_FRAME	13
#define NO_CHAT_TYPES	15
#define	SCROLLBAR_SIZE	20
#define SCROLLBAR_LINES NO_CHAT_TYPES * 4 + 6 - LINES_IN_FRAME
#define Y_START_POS		10
#define HEIGHT			25
#define X_ENABLED_POS		293
#define X_TEXT_POS		15
#define X_R_POS			182
#define X_G_POS			222
#define X_B_POS			262

pawsConfigChatBubbles::pawsConfigChatBubbles()
{
    allEnabled = NULL;
}

bool pawsConfigChatBubbles::PostSetup()
{
    chatBubbles = (psChatBubbles*) psengine->GetChatBubbles();
    if(!chatBubbles)
    {
        Error1("Couldn't find ChatBubbles!");
        return false;
    }

    allEnabled = (pawsCheckBox*)FindWidget("allEnabled");
    if (!allEnabled) {
        Error1("Could not locate allEnabled widget!");
        return false;
    }

    for (int c = 0, lineNo = 1; c < NO_CHAT_TYPES; ++c, lineNo += 4) {
        pawsBubbleChatType temp;

        temp.startLineNo = lineNo;

        switch (c) {
            case 0:
                temp.type = "say";
                temp.chatType = CHAT_SAY;
                break;
            case 1:
                temp.type = "tell";
                temp.chatType = CHAT_TELL;
                break;
            case 2:
                temp.type = "group";
                temp.chatType = CHAT_GROUP;
                break;
            case 3:
                temp.type = "guild";
                temp.chatType = CHAT_GUILD;
                break;
            case 4:
                temp.type = "alliance";
                temp.chatType = CHAT_ALLIANCE;
                break;
            case 5:
                temp.type = "shout";
                temp.chatType = CHAT_SHOUT;
                break;
            case 6:
                temp.type = "npc";
                temp.chatType = CHAT_NPC;
                break;
            case 7:
                temp.type = "auction";
                temp.chatType = CHAT_AUCTION;
                break;
            case 8:
                temp.type = "me";
                temp.chatType = CHATBUBBLE_ME;
                break;
            case 9:
                temp.type = "tellself";
                temp.chatType = CHAT_TELLSELF;
                break;
            case 10:
                temp.type = "my";
                temp.chatType = CHATBUBBLE_MY;
                break;
            case 11:
                temp.type = "npc_me";
                temp.chatType = CHAT_NPC_ME;
                break;
            case 12:
                temp.type = "npc_my";
                temp.chatType = CHAT_NPC_MY;
                break;
            case 13:
                temp.type = "npc_narrate";
                temp.chatType = CHAT_NPC_NARRATE;
                break;
            case 14:
                temp.type = "npcinternal";
                temp.chatType = CHAT_NPCINTERNAL;
                break;
            default:
                break;
        }

        csString widgetName = temp.type + "Enabled";
        temp.enabled = (pawsCheckBox*)FindWidget(widgetName);
        if (!temp.enabled) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "Text";
        temp.Text = (pawsTextBox*)FindWidget(widgetName);
        if (!temp.Text) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "Shadow";
        temp.Shadow = (pawsTextBox*)FindWidget(widgetName);
        if (!temp.Shadow) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "Outline";
        temp.Outline = (pawsTextBox*)FindWidget(widgetName);
        if (!temp.Outline) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "AlignText";
        temp.AlignText = (pawsTextBox*)FindWidget(widgetName);
        if (!temp.AlignText) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "TextR";
        temp.TextR = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.TextR) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "TextG";
        temp.TextG = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.TextG) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "TextB";
        temp.TextB = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.TextB) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "ShadowR";
        temp.ShadowR = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.ShadowR) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "ShadowG";
        temp.ShadowG = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.ShadowG) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "ShadowB";
        temp.ShadowB = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.ShadowB) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "OutlineR";
        temp.OutlineR = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.OutlineR) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "OutlineG";
        temp.OutlineG = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.OutlineG) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "OutlineB";
        temp.OutlineB = (pawsEditTextBox*)FindWidget(widgetName);
        if (!temp.OutlineB) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }

        widgetName = temp.type + "Align";
        temp.Align = (pawsComboBox*)FindWidget(widgetName);
        if (!temp.Align) {
            Error2("Could not locate %s widget!", widgetName.GetData());
            return false;
        }
        temp.Align->NewOption("Left");
        temp.Align->NewOption("Center");
        temp.Align->NewOption("Right");

        pawsBubbleChatTypes.Push(temp);
    }

    maxLineLenText = (pawsTextBox *)FindWidget("maxLineLenText");
    if (!maxLineLenText) {
        Error1("Could not locate maxLineLenText widget!");
        return false;
    }

    maxLineLen = (pawsEditTextBox *)FindWidget("maxLineLen");
    if (!maxLineLen) {
        Error1("Could not locate maxLineLen widget!");
        return false;
    }

    shortPhraseCharCountText = (pawsTextBox *)FindWidget("shortPhraseCharCountText");
    if (!shortPhraseCharCountText) {
        Error1("Could not locate shortPhraseCharCountText widget!");
        return false;
    }

    shortPhraseCharCount = (pawsEditTextBox *)FindWidget("shortPhraseCharCount");
    if (!shortPhraseCharCount) {
        Error1("Could not locate shortPhraseCharCount widget!");
        return false;
    }

    longPhraseLineCountText = (pawsTextBox *)FindWidget("longPhraseLineCountText");
    if (!longPhraseLineCountText) {
        Error1("Could not locate longPhraseLineCountText widget!");
        return false;
    }

    longPhraseLineCount = (pawsEditTextBox *)FindWidget("longPhraseLineCount");
    if (!longPhraseLineCount) {
        Error1("Could not locate longPhraseLineCount widget!");
        return false;
    }

    mixActionColours = (pawsCheckBox*)FindWidget("mixActionColours");
    if (!mixActionColours) {
        Error1("Could not locate mixActionColours widget!");
        return false;
    }	

    //Create scrollBar
    scrollBar = new pawsScrollBar;
    AddChild(scrollBar);
    scrollBar->SetHorizontal(false);
    int attach = ATTACH_BOTTOM | ATTACH_RIGHT | ATTACH_TOP;
    scrollBar->SetAttachFlags( attach );
    scrollBar->SetRelativeFrame( defaultFrame.Width()-SCROLLBAR_SIZE, 0, SCROLLBAR_SIZE, defaultFrame.Height()-8);
    scrollBar->PostSetup();
    scrollBar->Resize();
    scrollBar->SetTickValue( 1.0 );
    scrollBar->SetMaxValue(SCROLLBAR_LINES);

    drawFrame();

    return true;
}

void pawsConfigChatBubbles::drawFrame()
{
    //Clear the frame by hiding all widgets
    allEnabled->Hide();
    size_t len = pawsBubbleChatTypes.GetSize();
    for (size_t c = 0; c < len; ++c) {
        pawsBubbleChatType *chatType = &pawsBubbleChatTypes[c];
        chatType->enabled->Hide();

        chatType->Text->Hide();
        chatType->TextR->Hide();
        chatType->TextG->Hide();
        chatType->TextB->Hide();

        chatType->Shadow->Hide();
        chatType->ShadowR->Hide();
        chatType->ShadowG->Hide();
        chatType->ShadowB->Hide();		

        chatType->Outline->Hide();
        chatType->OutlineR->Hide();
        chatType->OutlineG->Hide();
        chatType->OutlineB->Hide();

        chatType->AlignText->Hide();
        chatType->Align->Hide();
    }
    maxLineLenText->Hide();
    maxLineLen->Hide();
    shortPhraseCharCountText->Hide();
    shortPhraseCharCount->Hide();
    longPhraseLineCountText->Hide();
    longPhraseLineCount->Hide();
    mixActionColours->Hide();

    int startLineNo = (int) scrollBar->GetCurrentValue();

    if (startLineNo == 0) {
        //Draw allEnable
        allEnabled->Show();
    }

    int endLineNo = startLineNo + LINES_IN_FRAME;

    //Draw other widgets as neccessary
    for (size_t c = 0; c < len; ++c) {
        pawsBubbleChatType *chatType = &pawsBubbleChatTypes[c];
        if (chatType->startLineNo >= startLineNo && chatType->startLineNo <= endLineNo) {
            int lineNo = chatType->startLineNo - startLineNo;
            int yPos = Y_START_POS + HEIGHT * lineNo;

            chatType->enabled->SetRelativeFramePos(X_ENABLED_POS, yPos);
            chatType->enabled->Show();

            chatType->Text->SetRelativeFramePos(X_TEXT_POS, yPos);
            chatType->Text->Show();
            chatType->TextR->SetRelativeFramePos(X_R_POS, yPos);			
            chatType->TextR->Show();
            chatType->TextG->SetRelativeFramePos(X_G_POS, yPos);			
            chatType->TextG->Show();
            chatType->TextB->SetRelativeFramePos(X_B_POS, yPos);			
            chatType->TextB->Show();
        }
        if ((chatType->startLineNo + 1) >= startLineNo && (chatType->startLineNo + 1) <= endLineNo) {
            int lineNo = (chatType->startLineNo + 1) - startLineNo;
            int yPos = Y_START_POS + HEIGHT * lineNo;

            chatType->Shadow->SetRelativeFramePos(X_TEXT_POS, yPos);
            chatType->Shadow->Show();
            chatType->ShadowR->SetRelativeFramePos(X_R_POS, yPos);			
            chatType->ShadowR->Show();
            chatType->ShadowG->SetRelativeFramePos(X_G_POS, yPos);			
            chatType->ShadowG->Show();
            chatType->ShadowB->SetRelativeFramePos(X_B_POS, yPos);			
            chatType->ShadowB->Show();
        }
        if ((chatType->startLineNo + 2) >= startLineNo && (chatType->startLineNo + 2) <= endLineNo) {
            int lineNo = (chatType->startLineNo + 2) - startLineNo;
            int yPos = Y_START_POS + HEIGHT * lineNo;

            chatType->Outline->SetRelativeFramePos(X_TEXT_POS, yPos);
            chatType->Outline->Show();
            chatType->OutlineR->SetRelativeFramePos(X_R_POS, yPos);			
            chatType->OutlineR->Show();
            chatType->OutlineG->SetRelativeFramePos(X_G_POS, yPos);			
            chatType->OutlineG->Show();
            chatType->OutlineB->SetRelativeFramePos(X_B_POS, yPos);			
            chatType->OutlineB->Show();
        }
        if ((chatType->startLineNo + 3) >= startLineNo && (chatType->startLineNo + 3) <= endLineNo) {
            int lineNo = (chatType->startLineNo + 3) - startLineNo;
            int yPos = Y_START_POS + HEIGHT * lineNo;

            chatType->AlignText->SetRelativeFramePos(X_TEXT_POS, yPos);
            chatType->AlignText->Show();
            chatType->Align->SetRelativeFramePos(X_R_POS, yPos - 4);
            chatType->Align->Show();
        }
    }	

    //Other widgets
    int lineNo = NO_CHAT_TYPES * 4 + 1;
    if (lineNo >= startLineNo && lineNo <= endLineNo) {
        int yPos = Y_START_POS + HEIGHT * (lineNo - startLineNo);
        maxLineLenText->SetRelativeFramePos(X_TEXT_POS, yPos);
        maxLineLenText->Show();
        maxLineLen->SetRelativeFramePos(X_B_POS, yPos);
        maxLineLen->Show();
    }
    ++lineNo;

    if (lineNo >= startLineNo && lineNo <= endLineNo) {
        int yPos = Y_START_POS + HEIGHT * (lineNo - startLineNo);
        shortPhraseCharCountText->SetRelativeFramePos(X_TEXT_POS, yPos);
        shortPhraseCharCountText->Show();
        shortPhraseCharCount->SetRelativeFramePos(X_B_POS, yPos);
        shortPhraseCharCount->Show();
    }
    ++lineNo;

    if (lineNo >= startLineNo && lineNo <= endLineNo) {
        int yPos = Y_START_POS + HEIGHT * (lineNo - startLineNo);
        longPhraseLineCountText->SetRelativeFramePos(X_TEXT_POS, yPos);
        longPhraseLineCountText->Show();
        longPhraseLineCount->SetRelativeFramePos(X_B_POS, yPos);
        longPhraseLineCount->Show();
    }

    ++lineNo;

    if (lineNo >= startLineNo && lineNo <= endLineNo) {
        int yPos = Y_START_POS + HEIGHT * (lineNo - startLineNo);
        mixActionColours->SetRelativeFramePos(X_TEXT_POS, yPos);
        mixActionColours->Show();
    }
}

bool pawsConfigChatBubbles::Initialize()
{
    if ( ! LoadFromFile("configchatbubbles.xml"))
        return false;

    return true;
}

bool pawsConfigChatBubbles::LoadConfig()
{
    csArray<BubbleChatType> chatTypes;

    chatTypes = chatBubbles->GetBubbleChatTypes();

    allEnabled->SetState(chatBubbles->isEnabled());

    csString temp;
    temp.Format("%d", (int) chatBubbles->GetBubbleMaxLineLen());
    maxLineLen->SetText(temp);
    temp.Format("%d", (int) chatBubbles->GetBubbleShortPhraseCharCount());
    shortPhraseCharCount->SetText(temp);
    temp.Format("%d", (int) chatBubbles->GetBubbleLongPhraseLineCount());
    longPhraseLineCount->SetText(temp);

    size_t lenChatTypes = chatTypes.GetSize();
    size_t lenPawsBubbleChatTypes = pawsBubbleChatTypes.GetSize();
    for (size_t c = 0; c < lenChatTypes; ++c) {
        for (size_t d = 0; d < lenPawsBubbleChatTypes; ++d) {
            if (chatTypes[c].chatType == pawsBubbleChatTypes[d].chatType) {
                pawsBubbleChatTypes[d].enabled->SetState(chatTypes[c].enabled);

                int R, G, B;
                psengine->GetG2D()->GetRGB(chatTypes[c].textSettings.colour, R, G, B);

                csString r, g, b;
                r.Format("%d", R);            
                g.Format("%d", G);            
                b.Format("%d", B);            

                pawsBubbleChatTypes[d].TextR->SetText(r);
                pawsBubbleChatTypes[d].TextG->SetText(g);
                pawsBubbleChatTypes[d].TextB->SetText(b);

                psengine->GetG2D()->GetRGB(chatTypes[c].textSettings.shadowColour, R, G, B);

                r.Format("%d", R);            
                g.Format("%d", G);            
                b.Format("%d", B);            

                pawsBubbleChatTypes[d].ShadowR->SetText(r);
                pawsBubbleChatTypes[d].ShadowG->SetText(g);
                pawsBubbleChatTypes[d].ShadowB->SetText(b);

                psengine->GetG2D()->GetRGB(chatTypes[c].textSettings.outlineColour, R, G, B);

                r.Format("%d", R);            
                g.Format("%d", G);            
                b.Format("%d", B);            

                pawsBubbleChatTypes[d].OutlineR->SetText(r);
                pawsBubbleChatTypes[d].OutlineG->SetText(g);
                pawsBubbleChatTypes[d].OutlineB->SetText(b);

                if (chatTypes[c].textSettings.align == ETA_RIGHT)
                    pawsBubbleChatTypes[d].Align->Select(2);
                else if (chatTypes[c].textSettings.align == ETA_CENTER)
                    pawsBubbleChatTypes[d].Align->Select(1);
                else
                    pawsBubbleChatTypes[d].Align->Select(0);

                break;
            }
        }
    }

    mixActionColours->SetState(chatBubbles->isMixingActionColours());

    dirty = false;

    return true;
}

bool pawsConfigChatBubbles::SaveConfig()
{
    //Save to file, then tell psChatBubbles to load it

    csString xml = "<?xml version=\"1.0\"?>\n";
    // xml += "<!-- Supported attributes for <chat>: type, color[R,G,B], shadow[R,G,B], outline[R,G,B], align, effectPrefix -->\n";
    // xml += "<!-- Supported Chat Types: say, tell, group, guild, alliance, auction, shout, me, tellself, my, npc, npc_me, npc_my, npc_narrate -->\n";
    // xml += "<!-- Supported Align Types: left, center, right -->\n\n";
    xml.AppendFmt("<chat_bubbles maxLineLen=\"%s\" shortPhraseCharCount=\"%s\" longPhraseLineCount=\"%s\" mixActionColours=\"%s\" enabled=\"%s\">\n",
        maxLineLen->GetText(), shortPhraseCharCount->GetText(), longPhraseLineCount->GetText(), mixActionColours->GetState() ? "yes" : "no", allEnabled->GetState() ? "yes" : "no");

    size_t lenPawsBubbleChatTypes = pawsBubbleChatTypes.GetSize();
    for (size_t c = 0; c < lenPawsBubbleChatTypes; ++c) {
        xml.AppendFmt("    <chat type=\"%s\" ", pawsBubbleChatTypes[c].type.GetData());

        xml.AppendFmt("enabled=\"%s\" ", pawsBubbleChatTypes[c].enabled->GetState() ? "yes" : "no");

        xml.AppendFmt("colourR=\"%s\" ", pawsBubbleChatTypes[c].TextR->GetText());
        xml.AppendFmt("colourG=\"%s\" ", pawsBubbleChatTypes[c].TextG->GetText());
        xml.AppendFmt("colourB=\"%s\" ", pawsBubbleChatTypes[c].TextB->GetText());

        xml.AppendFmt("shadowR=\"%s\" ", pawsBubbleChatTypes[c].ShadowR->GetText());
        xml.AppendFmt("shadowG=\"%s\" ", pawsBubbleChatTypes[c].ShadowG->GetText());
        xml.AppendFmt("shadowB=\"%s\" ", pawsBubbleChatTypes[c].ShadowB->GetText());

        xml.AppendFmt("outlineR=\"%s\" ", pawsBubbleChatTypes[c].OutlineR->GetText());
        xml.AppendFmt("outlineG=\"%s\" ", pawsBubbleChatTypes[c].OutlineG->GetText());
        xml.AppendFmt("outlineB=\"%s\" ", pawsBubbleChatTypes[c].OutlineB->GetText());

        csString align;
        if (pawsBubbleChatTypes[c].Align->GetSelectedRowNum() == 2)
            align = "right";
        else if (pawsBubbleChatTypes[c].Align->GetSelectedRowNum() == 1)
            align = "center";
        else
            align = "left";
        xml.AppendFmt("align=\"%s\" ", align.GetData());

        xml += "effectPrefix=\"chatbubble_\" />\n";
    }

    xml += "</chat_bubbles>";

    dirty = false;

    if (psengine->GetVFS()->WriteFile(USER_FILE, xml, xml.Length()))
        return chatBubbles->Load(USER_FILE);
    else 
        return false;
}

void pawsConfigChatBubbles::SetDefault()
{
    chatBubbles->Load(DEFAULT_FILE, SAVE_USER);
    LoadConfig();
}

void pawsConfigChatBubbles::Show()
{
    pawsWidget::Show();
}

bool pawsConfigChatBubbles::OnMouseDown(int button, int /*modifiers*/, int /*x*/, int /*y*/)
{
    if (button) {
        if (button == csmbWheelUp)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() - 1);
        else if (button == csmbWheelDown)
            scrollBar->SetCurrentValue(scrollBar->GetCurrentValue() + 1);
    }

    return true;
}

bool pawsConfigChatBubbles::OnScroll(int /*direction*/, pawsScrollBar* /*widget*/)
{
    drawFrame();
    return true;
}

void pawsConfigChatBubbles::OnListAction(pawsListBox* /*selected*/, int /*status*/)
{
    dirty = true;
}

