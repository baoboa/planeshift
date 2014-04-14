/*
* pawsnpcdialog.cpp - Author: Christian Svensson
*
* Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iutil/objreg.h>

#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"

#include "../globals.h"
#include "paws/pawsborder.h"
#include "gui/pawscontrolwindow.h"
#include "chatbubbles.h"
#include "pscelclient.h"
#include "pscamera.h"

#include "pawsnpcdialog.h"
#include "psclientchar.h"


/**
 * TODO:
 * -add support for numkey/enter action on the list version
 */


pawsNpcDialogWindow::pawsNpcDialogWindow() : targetEID(0)
{
    responseList = NULL;
    speechBubble = NULL;
    useBubbles = false;
    ticks = 0;
    cameraMode = 0;
    loadOnce = 0;
    questIDFree = -1;
    enabledChatBubbles = true;
    clickedOnResponseBubble = false;
    gotNewMenu = false;
    timeDelay = 3000; // initial minimum time of 3 seconds
}

bool pawsNpcDialogWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_DIALOG_MENU);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHAT);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_REMOVE_OBJECT);

    responseList = dynamic_cast<pawsListBox*>(FindWidget("ResponseList"));
    speechBubble = FindWidget("SpeechBubble");
    textBox = dynamic_cast<pawsEditTextBox*>(FindWidget("InputText"));
    closeBubble = dynamic_cast<pawsButton*>(FindWidget("CloseBubble"));

    if(!responseList || !FindWidget("Lists") || !speechBubble || !FindWidget("Bubbles") || !closeBubble)
    {
        return false;
    }

    //loads the options regarding this window
    if(!LoadSetting())
    {
        //setup the window with defaults.
        SetupWindowWidgets();
        CleanBubbles();
    }
    return true;
}

void pawsNpcDialogWindow::Draw()
{
    //let this dialog invisible for the time calculated to read the NPC say text
    //if we got a new psDialogMenuMessage we don't need to ask for a new menu (gotNewMenu)
    if(useBubbles && ticks != 0 && csGetTicks()-ticks > timeDelay && !gotNewMenu)
    {
        Debug1(LOG_PAWS,0,"Hiding NPC speech and asking for another set of possible questions.");
        speechBubble->Hide();
        FindWidget("FreeBubble")->Show();
        psengine->GetCmdHandler()->Execute("/npcmenu");
        ticks = 0;
    }
    //printf("gotNewMenu: %d, ticks: %d timeDelay: %d\n",gotNewMenu?1:0, (csGetTicks()-ticks), timeDelay);
    pawsWidget::Draw();
}

void pawsNpcDialogWindow::DrawBackground()
{
    if(!useBubbles)
    {
        pawsWidget::DrawBackground();
    }

}

bool pawsNpcDialogWindow::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers)
{
    //manage when the window has bubbles enabled
    if(useBubbles)
    {
        //check if the text entry box has focus else just let the basic
        //pawswidget implementation handle it
        if(!textBox->HasFocus())
        {
            return pawsWidget::OnKeyDown(keyCode, key, modifiers);
        }
        switch(key)
        {
            case CSKEY_ENTER:
            {
                csString text = textBox->GetText();
                csString answer = "";
                //
                for(size_t i = 0 ; i < questInfo.GetSize(); i++)
                {
                    csString tmp = questInfo[i].text;
                    tmp.DeleteAt(0,2);
                    tmp = tmp.Trim();
                    if(text == tmp)
                    {
                        answer = questInfo[i].trig;
                        break;
                    }
                }

                //try checking if there is free text instead
                if(answer == "" && text != "")
                {
                    if(questIDFree != -1) answer.Format("{%d} ", questIDFree);
                    answer += text;
                    questIDFree = -1;
                }

                if(answer != "")
                {
                    csString cmd;
                    if(answer.GetAt(0) == '=')  // prompt window signal
                    {
                        // cut the questID and convert it to a number as it's needed by the prompt window management
                        size_t pos = answer.FindFirst("{");
                        size_t endPos = answer.FindFirst("}");
                        int questID = -1;
                        if(pos != SIZET_NOT_FOUND)
                        {
                            unsigned long value = strtoul(answer.GetData() + pos + 1, NULL, 0);
                            questID = value;
                            // check for overflows
	                		if(questID < -1) 
                			{
                                questID = -1;
                            }

                            endPos += 1;
                        }
                        else
                        {
                            endPos = 1;
                        }

                        pawsStringPromptWindow::Create(csString(answer.GetData()+endPos),
                                                       csString(""),
                                                       false, 320, 30, this, answer.GetData()+endPos, questID);
                    }
                    else
                    {
                        if(answer.GetAt(0) != '<')
                        {
                            cmd.Format("/tellnpc %s", answer.GetData());
                            psengine->GetCmdHandler()->Publish(cmd);
                        }
                        else
                        {
                            psSimpleStringMessage gift(0,MSGTYPE_EXCHANGE_AUTOGIVE,answer);
                            gift.SendMessage();
                        }
                        ticks = csGetTicks(); // reset time , so we wait for the next response
                        gotNewMenu = false;
                        DisplayTextInChat(text.GetData());
                    }
                    textBox->Clear();
                }

                break;
            }
        }
    }
    else
    {
        return pawsWidget::OnKeyDown(keyCode, key, modifiers);
    }
    return true;
}

bool pawsNpcDialogWindow::OnButtonPressed(int button, int keyModifier, pawsWidget* widget)
{
    if(useBubbles)
    {
        if(widget)
        {
            csString name = widget->GetName();
            // process clicking on bubbles, but allow click-through on empty space
            if(name.StartsWith("Bubble"))
            {
                //get the trigger which was selected. we should never get out of bounds
                //if the system works well
                csString trigger = questInfo.Get(displayIndex+widget->GetID()-100).trig;
                csString text = questInfo.Get(displayIndex+widget->GetID()-100).text;
                // we clicked on a free text question, leave only free text box
                if(trigger.GetAt(0) == '=')
                {
                    // cut the questID and convert it to a number as it's needed by the prompt window management
                    size_t pos = trigger.FindFirst("{");
                    int questID = -1;
                    if(pos != SIZET_NOT_FOUND)
                    {
                        unsigned long value = strtoul(trigger.GetData() + pos + 1, NULL, 0);
                        questID = value;
                        // check for overflows
                		if(questID < -1) 
            			{
                            questID = -1;
                        }
                    }

                    questIDFree = questID;
                    ShowOnlyFreeText();
                    PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
                    gotNewMenu = true;
                    return true;
                }
                else if(trigger.GetAt(0) != '<')
                {
                    csString cmd;
                    cmd.Format("/tellnpc %s", trigger.GetData());
                    psengine->GetCmdHandler()->Publish(cmd);
                }
                else
                {
                    psSimpleStringMessage gift(0, MSGTYPE_EXCHANGE_AUTOGIVE, trigger);
                    gift.SendMessage();
                }
                DisplayTextInChat(text.GetData());
                CleanBubbles();
                PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
                ticks = csGetTicks(); // reset time, so we can wait for the next server response
                gotNewMenu = false;
            }
            else if(name == "CloseBubble")
            {
                gotNewMenu = false;
                Hide();
            }
            else if(name == "SpeechBubble")
            {
                clickedOnResponseBubble = true;
            }
            // process the left/right arrow clicking event
            else
            {
                if(name == "LeftArrow")
                {
                    if(displayIndex >= 3) displayIndex -= 3;
                    else displayIndex = 0;
                }
                else if(name == "RightArrow")
                {
                    if(displayIndex < questInfo.GetSize() - 3) displayIndex += 3;
                    else displayIndex = questInfo.GetSize() - 3;
                }

                DisplayQuestBubbles(displayIndex);

                pawsWidget* pw1 = FindWidget("LeftArrow");
                pawsWidget* pw2 = FindWidget("RightArrow");

                if(displayIndex < 3)
                    pw1->Hide();
                else
                    pw1->Show();

                if(displayIndex >= questInfo.GetSize() - 3)
                    pw2->Hide();
                else
                    pw2->Show();

                PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
            }
        }

        return true;

    }
    return pawsWidget::OnButtonPressed(button, keyModifier, widget);
}

bool pawsNpcDialogWindow::OnMouseDown(int button, int modifiers, int x , int y)
{
    //let pawsWidget handle the event
    bool result = pawsWidget::OnMouseDown(button, modifiers, x, y);

    if(useBubbles)
        PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);

    return result;
}

void pawsNpcDialogWindow::DisplayQuestBubbles(unsigned int index)
{
    unsigned int c = 1;
    csString bname = "";

    //set text and visible bubbles
    for(unsigned int i = index; i < questInfo.GetSize() && c <= 3; i++, c++)
    {
        bname = "Bubble";
        bname.Append(c);
        pawsButton* pb = dynamic_cast<pawsButton*>(FindWidget(bname));
        pawsTextBox* qn = dynamic_cast<pawsTextBox*>(pb->FindWidget("QuestName"));
        pawsMultiLineTextBox* qt = dynamic_cast<pawsMultiLineTextBox*>(pb->FindWidget("QuestText"));

        qn->SetText(questInfo[i].title);
        qt->SetText(questInfo[i].text);
        pb->Show();
    }

    //set remaining bubbles as invisible
    for(; c <= 3 ; c++)
    {
        bname = "Bubble";
        bname.Append(c);
        pawsButton* pb = dynamic_cast<pawsButton*>(FindWidget(bname));
        pb->Hide();
    }

    PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
}

void pawsNpcDialogWindow::LoadQuest(csString xmlstr)
{
    questInfo.DeleteAll();
    displayIndex = 0;

    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDocument> doc= xml->CreateDocument();
    const char* error = doc->Parse(xmlstr);
    if(error)
    {
        Error2("%s\n", error);
        return;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    csRef<iDocumentNode> topNode = root->GetNode(xmlbinding!=""?xmlbinding:name);

    csRef<iDocumentNode> options = topNode->GetNode("options");

    csRef<iDocumentNodeIterator> it  = options->GetNodes("row");

    QuestInfo qi;

    while(it != 0 && it->HasNext())
    {
        csRef<iDocumentNode> cur = it->Next();
        if(cur->GetAttributeValueAsInt("heading"))
        {
            csRef<iDocumentNode> txt = cur->GetNode("text");
            qi.title = txt->GetContentsValue();
        }
        else
        {
            csRef<iDocumentNode> txt = cur->GetNode("text");
            csRef<iDocumentNode> trg = cur->GetNode("trig");
            qi.text = txt->GetContentsValue();
            qi.trig = trg->GetContentsValue();
            if(!qi.text.StartsWith("?="))  // by purpose this line catches only the questmenu item, not the npcdialogue item
            {
                questInfo.Push(qi);
            }
        }
    }

    pawsWidget* pw1 = FindWidget("LeftArrow");
    pawsWidget* pw2 = FindWidget("RightArrow");

    pw1->Hide();

    if(questInfo.GetSize() <= 3)
        pw2->Hide();
    else
        pw2->Show();
}

void pawsNpcDialogWindow::OnListAction(pawsListBox* widget, int status)
{
    if(status == LISTBOX_HIGHLIGHTED)
    {
        pawsTextBox* fld = dynamic_cast<pawsTextBox*>(widget->GetSelectedRow()->FindWidgetXMLBinding("text"));
        Debug2(LOG_QUESTS, 0, "Pressed: %s\n",fld->GetText());
    }
    else if(status == LISTBOX_SELECTED)
    {
        //if no row is selected
        if(!widget->GetSelectedRow())
            return;

        pawsTextBox* fld  = dynamic_cast<pawsTextBox*>(widget->GetSelectedRow()->FindWidgetXMLBinding("text"));
        Debug2(LOG_QUESTS, 0,"Player chose '%s'.\n", fld->GetText());
        pawsTextBox* trig = dynamic_cast<pawsTextBox*>(widget->GetSelectedRow()->FindWidgetXMLBinding("trig"));
        Debug2(LOG_QUESTS, 0,"Player says '%s'.\n", trig->GetText());

        csString trigger(trig->GetText());

        // Send the server the original trigger
        csString cmd;
        if(trigger.GetAt(0) == '=')  // prompt window signal
        {
            // cut the questID and convert it to a number as it's needed by the prompt window management
            size_t pos = trigger.FindFirst("{");
            size_t endPos = trigger.FindFirst("}");
            int questID = -1;
            if(pos != SIZET_NOT_FOUND)
            {
                unsigned long value = strtoul(trigger.GetData() + pos + 1, NULL, 0);
                questID = value;
                // check for overflows
	    		if(questID < -1) 
    			{
                    questID = -1;
                }

                endPos += 1;
            }
            else
            {
                endPos = 1;
            }

            pawsStringPromptWindow::Create(csString(trigger.GetData()+endPos),
                                           csString(""),
                                           false, 320, 30, this, trigger.GetData()+endPos, questID);
        }
        else
        {
            if(trigger.GetAt(0) != '<')
            {
                cmd.Format("/tellnpc %s", trigger.GetData());
                psengine->GetCmdHandler()->Publish(cmd);
            }
            else
            {
                psSimpleStringMessage gift(0,MSGTYPE_EXCHANGE_AUTOGIVE,trigger);
                gift.SendMessage();
            }
            DisplayTextInChat(fld->GetText());
        }
        Hide();
    }
}

void pawsNpcDialogWindow::DisplayTextInChat(const char* sayWhat)
{
    // Now send the message to the NPC
    csString text(sayWhat);
    size_t dot = text.FindFirst('.'); // Take out the numbering to display
    if(dot != SIZET_NOT_FOUND)
    {
        text.DeleteAt(0,dot+1);
    }
    csString cmd;
    cmd.Format("/tellnpcinternal %s", text.GetData());
    psengine->GetCmdHandler()->Publish(cmd);
    responseList->Clear();
}

void pawsNpcDialogWindow::HandleMessage(MsgEntry* me)
{
    if(me->GetType() == MSGTYPE_DIALOG_MENU)
    {
        psDialogMenuMessage mesg(me);

        Debug2(LOG_QUESTS, 0,"Got psDialogMenuMessage: %s\n", mesg.xml.GetDataSafe());
        responseList->Clear();

        SelfPopulateXML(mesg.xml);

        if(useBubbles)
        {
            speechBubble->Hide(); // hide previous npc say response
            LoadQuest(mesg.xml);
            AdjustForPromptWindow(); // should be done before DisplayQuestBubbles
            DisplayQuestBubbles(displayIndex);
            gotNewMenu = true;
        }
        else
        {
            AdjustForPromptWindow();
        }

        Show();
    }
    else if(me->GetType() == MSGTYPE_CHAT)
    {
        psChatMessage chatMsg(me);
        Debug2(LOG_QUESTS, 0,"Got Chat message from NPC: %s\n", (const char*)chatMsg.sText);

        GEMClientActor* actor = dynamic_cast<GEMClientActor*>(psengine->GetCelClient()->FindObject(chatMsg.actor));
        if(!actor)
            return;

        // handle the basic action types, in the future we could play an animation as well
        csString inText ;
        switch(chatMsg.iChatType)
        {
            case CHAT_NPC_ME:
            {
                inText.Format("(%s %s)", (const char*)chatMsg.sPerson, ((const char*)chatMsg.sText));
                break;
            }

            case CHAT_NPC_MY:
            {
                inText.Format("(%s's %s)", (const char*)chatMsg.sPerson, ((const char*)chatMsg.sText));
                break;
            }

            case CHAT_NPC_NARRATE:
            {
                inText.Format("(%s)", (const char*)chatMsg.sText);
                break;
            }
            default:
                inText = chatMsg.sText;
        }
        NpcSays(inText, actor);

        //checks if the NPC Dialogue is displayed, in this case don't show the normal overhead bubble
        if(IsVisible())
            return;
    }
    else if(me->GetType() == MSGTYPE_REMOVE_OBJECT)
    {
        psRemoveObject mesg(me);
        //if the current target is being removed hide this window
        if(IsVisible() && targetEID.IsValid() && mesg.objectEID.IsValid() && mesg.objectEID == targetEID)
        {
            Hide();
            //In these cases something might have gone wrong with the camera
            //eg: the character is being teleported. So we need to have it reset
            //correctly.
            psengine->GetPSCamera()->ResetCameraPositioning();
        }
    }
}

void pawsNpcDialogWindow::NpcSays(csString &inText,GEMClientActor* actor)
{
    //this is used only when using the chat bubbles interface
    if(!useBubbles)
    {
        return;
    }

    clickedOnResponseBubble = false;

    //display npc response
    if(IsVisible() && actor && psengine->GetCharManager()->GetTarget() == actor)
    {
        dynamic_cast<pawsMultiLineTextBox*>(speechBubble->FindWidget("BubbleText"))->SetText(inText.GetData());
        speechBubble->Show();
        FindWidget("Bubble1")->Hide();
        FindWidget("Bubble2")->Hide();
        FindWidget("Bubble3")->Hide();
        FindWidget("LeftArrow")->Hide();
        FindWidget("RightArrow")->Hide();
        FindWidget("FreeBubble")->Hide();

        ticks = csGetTicks();
        timeDelay = (csTicks)(2000 + 50*strlen(inText.GetData()) + 2000); // add 2 seconds for network delay
        timeDelay =  timeDelay>14000?14000:timeDelay; // clamp to 14000 max

        Show(); //show the npc dialog
    }
}


void pawsNpcDialogWindow::AdjustForPromptWindow()
{
    csString str;

    // This part is used only for menu npc dialogue
    for(size_t i=0; i<responseList->GetRowCount(); i++)
    {
        str = responseList->GetTextCellValue(i,0);
        size_t where = str.Find("?=");
        if(where != SIZET_NOT_FOUND)  // we have a prompt choice
        {
            pawsTextBox* hidden = (pawsTextBox*)responseList->GetRow(i)->GetColumn(1);
            if(hidden)
            {
                csString id = "=";

                // compatibility with old servers.
                if(csString(hidden->GetText()).Find("{") != SIZET_NOT_FOUND)
                {
                    id += hidden->GetText();
                }

                str.DeleteAt(where,2); // take out the ?=
                id += str.GetData();
                // Save the question prompt, starting with the =, in the hidden column
                hidden->SetText(id.GetData());

                // now change the visible menu choice to something better
                pawsTextBox* prompt = (pawsTextBox*)responseList->GetRow(i)->GetColumn(0);

                csString menuPrompt(str);
                menuPrompt.Insert(where,"<Answer ");
                menuPrompt.Append('>');
                prompt->SetText(menuPrompt);
            }
        }
    }

    // this part is used only with bubbles npc dialogue
    for(size_t i = 0 ; i < questInfo.GetSize(); i++)
    {
        QuestInfo &qi = questInfo[i];
        str = qi.text;
        size_t where = str.Find("?=");
        if(where != SIZET_NOT_FOUND)
        {
            csString id = "=";
            // compatibility with old servers.
            if(qi.trig.Find("{") != SIZET_NOT_FOUND)
            {
                id += qi.trig;
            }

            str.DeleteAt(where,2); // take out the ?=
            id += str;
            // Save the question prompt, starting with the =, in the hidden column
            qi.trig = id;
            str.Insert(where,"I know the answer to : ");
            qi.text = str;
        }
    }
}

void pawsNpcDialogWindow::OnStringEntered(const char* name, int param, const char* value)
{
    //The user cancelled the operation. So show again the last window and do nothing else.
    if(value == NULL)
    {
        Show();
        return;
    }

    Debug3(LOG_QUESTS, 0,"Got name=%s, value=%s\n", name, value);

    csString cmd;
    if(param != -1)
    {
        cmd.Format("/tellnpc {%d} %s", param, value);
    }
    else
    {
        cmd.Format("/tellnpc %s", value);
    }

    psengine->GetCmdHandler()->Publish(cmd);
    DisplayTextInChat(value);
    ticks = csGetTicks(); // reset time, so we can wait for the next server response
    gotNewMenu = false;
}

void pawsNpcDialogWindow::SetupWindowWidgets()
{
    pawsWidget* lists = FindWidget("Lists");
    pawsWidget* bubbles = FindWidget("Bubbles");
    if(useBubbles)
    {
        if(border) border->Hide();
        if(close_widget) close_widget->Hide();
        lists->Hide();
        responseList->Hide();
        bubbles->Show();
        SetMovable(false);
        psengine->GetPSCamera()->LockCameraMode(true);
        psengine->GetCharManager()->LockTarget(true);
        defaultFrame = bubbles->GetScreenFrame();
        SetSize(defaultFrame.Width(), defaultFrame.Height());
        CenterTo(graphics2D->GetWidth() / 2, graphics2D->GetHeight() / 2);
        FindWidget("FreeBubble")->Show();
        textBox->Clear();
    }
    else
    {
        if(border) border->Show();
        if(close_widget) close_widget->Show();
        lists->Show();
        responseList->Show();
        bubbles->Hide();
        SetMovable(true);
        psengine->GetPSCamera()->LockCameraMode(false);
        psengine->GetCharManager()->LockTarget(false);
        defaultFrame = lists->GetDefaultFrame();
        SetSize(defaultFrame.Width(), defaultFrame.Height());
        CenterTo(graphics2D->GetWidth() / 2, graphics2D->GetHeight() / 2);
        PawsManager::GetSingleton().SetCurrentFocusedWidget(responseList);
    }
}

void pawsNpcDialogWindow::CleanBubbles()
{
    FindWidget("LeftArrow")->Hide();
    FindWidget("RightArrow")->Hide();
    dynamic_cast<pawsMultiLineTextBox*>(speechBubble->FindWidget("BubbleText"))->SetText("");
    FindWidget("SpeechBubble")->Hide();
    displayIndex = 0;
    questInfo.DeleteAll();
    DisplayQuestBubbles(0);
}

void pawsNpcDialogWindow::ShowOnlyFreeText()
{
    // hide arrows
    FindWidget("LeftArrow")->Hide();
    FindWidget("RightArrow")->Hide();

    // hide bubbles
    csString bname = "";
    for(int c=1; c <= 3 ; c++)
    {
        bname = "Bubble";
        bname.Append(c);
        pawsButton* pb = dynamic_cast<pawsButton*>(FindWidget(bname));
        pb->Hide();
    }

    FindWidget("FreeBubble")->Show();
}

void pawsNpcDialogWindow::ShowIfBubbles()
{
    if(useBubbles)
    {
        CleanBubbles();
        Show();
    }
}

void pawsNpcDialogWindow::Show()
{
    pawsWidget::Show();

    if(loadOnce == 0)
    {
        loadOnce++;

        //split apart just for visiblity ordering in source
        if(useBubbles)
        {
            cameraMode = psengine->GetPSCamera()->GetCameraMode(); //get the camera's current mode

            GEMClientObject* cobj = psengine->GetCharManager()->GetTarget();
            if(cobj)
            {
                //let the camera focus upon the target npc
                csRef<psCelClient> celclient = psengine->GetCelClient();
                GEMClientObject* mobj = celclient->GetMainPlayer();
                csVector3 p1 = cobj->GetPosition();
                csVector3 p2 = mobj->GetPosition();
                csVector3 direction = p1 - p2;
                if(direction.x == 0.0f)
                    direction.x = 0.00001f;
                float nyaw = atan2(-direction.x, -direction.z);

                csVector3 pos;
                float yrot,rot;
                iSector* isect;
                dynamic_cast<GEMClientActor*>(mobj)->GetLastPosition(p1,yrot,isect);
                dynamic_cast<GEMClientActor*>(mobj)->SetPosition(p1,nyaw,isect);
                dynamic_cast<GEMClientActor*>(cobj)->GetLastPosition(p2,rot,isect);
                dynamic_cast<GEMClientActor*>(cobj)->SetPosition(p2,nyaw+3.1415f,isect);
                psengine->GetPSCamera()->SetCameraMode(0); //set the camera to the first person mode
                targetEID = cobj->GetEID();
            }
            enabledChatBubbles = psengine->GetChatBubbles()->isEnabled();
            //psengine->GetChatBubbles()->setEnabled(false);
        }

        SetupWindowWidgets();
    }
    SendToBottom(this);
}

void pawsNpcDialogWindow::Hide()
{
    loadOnce = 0;

    if(useBubbles)
    {
        textBox->Clear();
        CleanBubbles();
        questIDFree = -1;
        psengine->GetPSCamera()->LockCameraMode(false);
        psengine->GetCharManager()->LockTarget(false);
        //psengine->GetChatBubbles()->setEnabled(enabledChatBubbles);
        if(psengine->GetPSCamera()->GetCameraMode() == 0)
        {
            psengine->GetPSCamera()->SetCameraMode(cameraMode); // restore camera mode
        }
    }

    pawsWidget::Hide();
}

bool pawsNpcDialogWindow::LoadSetting()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,npcDialogNode, npcDialogOptionsNode;
    csString option;

    doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_NPCDIALOG_FILE_NAME);
    if(doc == NULL)
    {
        //load the default configuration file in case the user one fails (missing or damaged)
        doc = ParseFile(psengine->GetObjectRegistry(), CONFIG_NPCDIALOG_FILE_NAME_DEF);
        if(doc == NULL)
        {
            Error2("Failed to parse file %s", CONFIG_NPCDIALOG_FILE_NAME_DEF);
            return false;
        }
    }

    root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("npcdialog_def.xml or npcdialog.xml has no XML root");
        return false;
    }

    npcDialogNode = root->GetNode("npcdialog");
    if(npcDialogNode == NULL)
    {
        Error1("npcdialog_def.xml or npcdialog.xml has no <npcdialog> tag");
        return false;
    }

    // Load options for the npc dialog
    npcDialogOptionsNode = npcDialogNode->GetNode("npcdialogoptions");
    if(npcDialogOptionsNode != NULL)
    {
        csRef<iDocumentNodeIterator> oNodes = npcDialogOptionsNode->GetNodes();
        while(oNodes->HasNext())
        {
            csRef<iDocumentNode> option = oNodes->Next();
            csString nodeName(option->GetValue());

            if(nodeName == "usenpcdialog")
            {
                //showWindow->SetState(!option->GetAttributeValueAsBool("value"));
                useBubbles = option->GetAttributeValueAsBool("value");
            }
        }
    }

    return true;
}

void pawsNpcDialogWindow::SaveSetting()
{
    csRef<iFile> file;
    file = psengine->GetVFS()->Open(CONFIG_NPCDIALOG_FILE_NAME,VFS_FILE_WRITE);

    csRef<iDocumentSystem> docsys;
    docsys.AttachNew(new csTinyDocumentSystem);

    csRef<iDocument> doc = docsys->CreateDocument();
    csRef<iDocumentNode> root,defaultRoot, npcDialogNode, npcDialogOptionsNode, useNpcDialogNode;

    root = doc->CreateRoot();

    npcDialogNode = root->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    npcDialogNode->SetValue("npcdialog");

    npcDialogOptionsNode = npcDialogNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    npcDialogOptionsNode->SetValue("npcdialogoptions");

    useNpcDialogNode = npcDialogOptionsNode->CreateNodeBefore(CS_NODE_ELEMENT, 0);
    useNpcDialogNode->SetValue("usenpcdialog");
    useNpcDialogNode->SetAttributeAsInt("value", useBubbles? 1 : 0);

    doc->Write(file);
}
