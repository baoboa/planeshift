/*
 * pawsSketchWindow.cpp - Author: Keith Fulton
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// PAWS INCLUDES
#include "pawsilluminationwindow.h"
#include "paws/pawsprefmanager.h" // For FONT_STYLE_DROPSHADOW - DrawColorWidgetText (maybe move DrawColorWidgetText to pawswidget?)
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/log.h"
#include "util/psxmlparser.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsSketchWindow::pawsSketchWindow()
{
    selectedIndex = SIZET_NOT_FOUND;
    dirty     = false;
    editMode  = false;
    mouseDown = false;
    colorPending = false;
    mouseDownX = 0;
    mouseDownY = 0;
    frame = 0;
}
pawsSketchWindow::pawsSketchWindow(const pawsSketchWindow& origin)
{

}
pawsSketchWindow::~pawsSketchWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_VIEW_SKETCH);
}

bool pawsSketchWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_VIEW_SKETCH);
    //setup an easy way to access our widgets
    FeatherTool = FindWidget("FeatherTool");
    TextTool = FindWidget("TextTool");
    LineTool = FindWidget("LineTool");
    BezierTool = FindWidget("BezierTool");
    PlusTool = FindWidget("PlusTool");
    LeftArrowTool = FindWidget("LeftArrowTool");
    RightArrowTool = FindWidget("RightArrowTool");
    DeleteTool = FindWidget("DeleteTool");
    NameTool = FindWidget("NameTool");
    SaveButton = FindWidget("SaveButton");
    LoadButton = FindWidget("LoadButton");
    ColorTool = FindWidget("ColorTool");
    return true;
}

iGraphics2D *pawsSketchWindow::GetG2D()
{
    csRef<iGraphics2D> gfx = PawsManager::GetSingleton().GetGraphics2D();
    return gfx;
}

void pawsSketchWindow::SetToolbarButtons()
{
    //hide useless widgets in the window so the sketch is more clean
    //to whoever looks at it while not being the author
    if(readOnly)
    {
        if(FeatherTool)
            FeatherTool->Hide();
        if(SaveButton)
            SaveButton->Hide();
    }
    else
    {
         if(FeatherTool)
            FeatherTool->Show();
        if(SaveButton)
            SaveButton->Show();
    }

    if(!editMode) //we aren't in edit mode, let's hide the unusable tools
    {
        if(TextTool)
            TextTool->Hide();
        if(LineTool)
            LineTool->Hide();
        if(BezierTool)
            BezierTool->Hide();
        if(PlusTool)
            PlusTool->Hide();
        if(LeftArrowTool)
            LeftArrowTool->Hide();
        if(RightArrowTool)
            RightArrowTool->Hide();
        if(DeleteTool)
            DeleteTool->Hide();
        if(NameTool)
            NameTool->Hide();
        if(LoadButton)
            LoadButton->Hide();
        if(ColorTool)
            ColorTool->Hide();
    }
    else //we are in edit mode so let's show buttons to do some editing
    {
        if(TextTool)
            TextTool->Show();
        if(LineTool)
            LineTool->Show();
        if(BezierTool)
            BezierTool->Show();
        if(PlusTool)
            PlusTool->Show();
        if(LeftArrowTool)
            LeftArrowTool->Show();
        if(RightArrowTool)
            RightArrowTool->Show();
        if(DeleteTool)
            DeleteTool->Show();
        if(NameTool)
            NameTool->Show();
        if(SaveButton)
            SaveButton->Show();
        if(LoadButton)
            LoadButton->Show();
        if(ColorTool)
            ColorTool->Show();
    }
}

void pawsSketchWindow::HandleMessage( MsgEntry* me )
{
    editMode      = false;
    isResizable = false;
    stringPending = false;
    colorPending = false;
    psSketchMessage msg( me );

    currentItemID = msg.ItemID;
    Notify4(LOG_PAWS,"Got Sketch for item %u: %s\nLimits:%s\n", msg.ItemID, msg.Sketch.GetData(), msg.limits.GetDataSafe());
    objlist.Empty();

    dirty = false;
    ParseLimits(msg.limits);
    if(!ParseSketch(msg.Sketch))
    {// Don't show a sketch which can't be parsed.
        Hide();
        return;
    }

    // if char does not have the right to edit (ie not the originator-artist)...
    if (!msg.rightToEdit)
        readOnly = true;

    SetToolbarButtons();

    sketchName = msg.name;
    SetTitle(sketchName);
    
    //set the background image for the map
    sketchBgImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(msg.backgroundImg);

    if (!blackBox)
        blackBox = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("blackbox");
    Show();
}

csString pawsSketchWindow::toXML(void )
{
    //generates the xml document from the objects currently displayed in the sketch
    csString xml;

    xml = "<pages><page ";
    xml.AppendFmt("l=\"%d\" t=\"%d\" w=\"%d\" h=\"%d\">", GetLogicalWidth(
            screenFrame.xmin), GetLogicalHeight(screenFrame.ymin),
            GetLogicalWidth(screenFrame.xmax - screenFrame.xmin),
            GetLogicalHeight(screenFrame.ymax - screenFrame.ymin));

    // Add background and size stuff here
    for (size_t i = 0; i < objlist.GetSize(); i++)
        if (objlist[i]->x <= screenFrame.xmax - screenFrame.xmin
                && objlist[i]->y <= screenFrame.ymax - screenFrame.ymin)
            objlist[i]->WriteXml(xml);

    xml += "</page></pages>";
    return xml;
}

void pawsSketchWindow::Hide()
{
    if (dirty)
    {
        csString xml = toXML();
        Debug2(LOG_PAWS, 0, "Saving sketch as: %s", xml.GetDataSafe());
        psSketchMessage sketch(0, currentItemID,0,"", xml, true, sketchName,"");
        sketch.SendMessage();
    }
    pawsWidget::Hide();
}


void pawsSketchWindow::Draw()
{
    pawsWidget::DrawWindow();

    //draw background
    if(sketchBgImage)
        sketchBgImage->Draw(screenFrame, 0);
        
    pawsWidget::DrawForeground();

    // The close button X overrides the clip region so we have to set it back here
    ClipToParent(false);

    // if (editMode && !mouseDown) update "new selection" each nth frame
    if (editMode && !mouseDown && !colorPending)
    {
        frame++;
        if(frame > 9)
        {
            frame = 0;
            // Error1("Frame>9");
            psPoint pos = PawsManager::GetSingleton().GetMouse()->GetPosition();
            static int prevMouseX = 0;
            static int prevMouseY = 0;

            if( (abs(prevMouseX - pos.x)>1) || (abs(prevMouseY - pos.y)>1) ) // only update if the mouse moved (otherwise keyboard editing will be impossible
            {
                // Error1("prevMouseX != pos.x || prevMouseY != pos.y");
                prevMouseX = pos.x;
                prevMouseY = pos.y;

                for (size_t i=0; i<objlist.GetSize(); i++)
                {
                    if (objlist[i]->IsHit(pos.x,pos.y))
                    {
                        // we have it selected already
                        if(i == selectedIndex)
                        {
                            break;
                        }

                        // Unselect old object
                        if (selectedIndex != SIZET_NOT_FOUND)
                        {
                            objlist[selectedIndex]->Select(false);
                        }

                        // Select new object
                        selectedIndex = i;
                        objlist[i]->Select(true);
                        break;
                    }
                }
            }
        }
    }

    DrawSketch();
}
void pawsSketchWindow::DrawSketch()
{
    if (selectedIndex != SIZET_NOT_FOUND && IsMouseDown())
    {
        psPoint pos = PawsManager::GetSingleton().GetMouse()->GetPosition();
        /* UpdatePosition requires the RELATIVE position now. Much easier, much cleaner.
        objlist[selectedIndex]->UpdatePosition(GetLogicalWidth(pos.x
                - ScreenFrame().xmin), GetLogicalHeight(pos.y
                - ScreenFrame().ymin));*/
        objlist[selectedIndex]->UpdatePosition(pos.x - mouseDownX, pos.y - mouseDownY);
        mouseDownX = pos.x; // remember new position for the next relative update.
        mouseDownY = pos.y;
    }
    for (size_t i = 0; i < objlist.GetSize(); i++)
        objlist[i]->Draw();
}

void pawsSketchWindow::DrawBlackBox(int x, int y)
{
    // Black box only appears in edit mode, but SketchObjects don't need to know that
    if (editMode)
        blackBox->Draw(x,y);
}

void pawsSketchWindow::DrawColorWidgetText(const char *text, int x, int y, int color)
{
    csRef<iFont> font = GetFont();
    int style = GetFontStyle();

    if (style & FONT_STYLE_DROPSHADOW)
        graphics2D->Write( font, x+2, y+2, GetFontShadowColour(), -1, text );

    graphics2D->Write( font, x, y, color, -1, text);
}

double pawsSketchWindow::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    if (!strcasecmp(functionName,"ClickTextTool"))
    {
        OnKeyDown(0,'t',0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickEditMode"))
    {
        OnKeyDown(CSKEY_F2,0,0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickLineTool"))
    {
        OnKeyDown(0,'\\',0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickBezierTool"))
    {
        OnKeyDown(0,'b',0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickIconTool"))
    {
        OnKeyDown(0,'+',0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickPrevIcon"))
    {
        OnKeyDown(CSKEY_PGUP,0,0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickNextIcon"))
    {
        OnKeyDown(CSKEY_PGDN,0,0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickDeleteTool"))
    {
        OnKeyDown(CSKEY_DEL,0,0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickNameTool"))
    {
        OnKeyDown(0,'n',0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickSaveButton"))
    {
        OnKeyDown(0,'s',0);
        return 0.0;
    }
    else if (!strcasecmp(functionName,"ClickLoadButton"))
    {
        OnKeyDown(0,'l',0);
        return 0.0;
    }
    else if(!strcasecmp(functionName,"ClickColorPicker"))
    {
        OnKeyDown(0, 'c', 0);
        return 0.0;
    }

    // else call parent version to inherit other functions
    return pawsWidget::CalcFunction(env, functionName,params);
}

bool pawsSketchWindow::OnKeyDown( utf32_char keyCode, utf32_char key, int modifiers )
{
    // printf("Keycode: %d, Key: %d\n", keyCode, key);

    if (keyCode == CSKEY_F2)
    {
        if (!readOnly)
        {
            if (selectedIndex != SIZET_NOT_FOUND && editMode)
            {
                objlist[selectedIndex]->Select(false); // turn off flashing
                selectedIndex = SIZET_NOT_FOUND;
            }

            editMode = editMode ? false : true; // toggle edit mode
            isResizable = editMode; // only resizable in edit mode
            dirty=true; // always set this for now to make sure resizes get saved.

            // notify the user that we are entering/leaving edit mode
            if (editMode)
            {
                psSystemMessage sysMsg( 0, MSG_ACK, PawsManager::GetSingleton().Translate("You have entered edit mode.") );
                sysMsg.FireEvent();
            }
            else
            {
                psSystemMessage sysMsg( 0, MSG_ACK, PawsManager::GetSingleton().Translate("Edit mode has been disabled.") );
                sysMsg.FireEvent();
            }
            SetToolbarButtons();
        }
        else
        {
            psSystemMessage sysMsg( 0, MSG_ERROR, PawsManager::GetSingleton().Translate("You will only hurt the illustration if you try to edit it.") );
            sysMsg.FireEvent();
            editMode = false; // just to be safe
        }
        return true;
    }
    else if (key == 's' )//&& !readOnly) //allow saving also when not in edit mode but check if the player saving the sketch
    {                                 //has editing rights over the sketch it's being saved
            SaveSketch();
            return true;
    }
    else if (editMode && !colorPending)
    {
        if (key == '+') // keycode is the kbd scan code, key is the ASCII
        {
            AddSketchIcon();
            return true;
        }
        else if (key == '\\')
        {
            AddSketchLine();
            return true;
        }
        else if (key == 'b')
        {
            AddSketchBezier();
            return true;
        }
        else if (key == 't')
        {
            AddSketchText();
            return true;
        }
        else if (key == 'n')
        {
            ChangeSketchName();
            return true;
        }
        else if (key == 'l') //allow loading only in editing mode
        {
            LoadSketch();
            return true;
        }
        else if (key == 'c')
        {
            if (selectedIndex != SIZET_NOT_FOUND)
            {
                int color = objlist[selectedIndex]->color;
                if(color == -1)
                {
                    color = 0;
                }
                colorPending = true;
                pawsColorPromptWindow::Create("Select Colour", color,0,120, this, "SetColor", 1);
                /* // Used for testing.
                iGraphics2D *g2d = GetG2D();
                int color = objlist[selectedIndex]->color;

                if(color == -1)
                {
                    color = 0;
                }

                int r, g, b;
                g2d->GetRGB(color, r, g, b);

                g=b;
                b=g;
                r+=0xf;
                if(r>=0x80)
                    r=0;

                color = g2d->FindRGB(r, g, b);

                objlist[selectedIndex]->SetColor(color);*/
            }
            return true;
        }

        switch (keyCode)
        {
            case CSKEY_DEL:     RemoveSelected();
                                return true;
            case CSKEY_PGDN:    NextPrevIcon(1); // +1 is next
                                return true;
            case CSKEY_PGUP:    NextPrevIcon(-1); // +1 is next
                                return true;
            case CSKEY_DOWN:    MoveObject(0,1);
                                return true;
            case CSKEY_UP:      MoveObject(0,-1);
                                return true;
            case CSKEY_LEFT:    MoveObject(-1,0);
                                return true;
            case CSKEY_RIGHT:   MoveObject(1,0);
                                return true;
            default:
                break;
        }
    }

    return pawsWidget::OnKeyDown(keyCode,key,modifiers);
}

void pawsSketchWindow::AddSketchText()
{
    if ((int)objlist.GetSize() >= primCount)
    {
        psSystemMessage sysMsg( 0, MSG_ERROR, PawsManager::GetSingleton().Translate("Your sketch is too complex for you to add more.") );
        sysMsg.FireEvent();
        return;
    }

    if (!stringPending)
    {
        stringPending = true;

        // This window calls OnStringEntered when Ok is pressed.
        pawsStringPromptWindow::Create("New Text", csString(""),
            false, 220, 20, this, "AddText");
    }
}

void pawsSketchWindow::OnStringEntered(const char* name, int /*param*/, const char* value)
{
    stringPending = false;

    if (!value || !strlen(value))
        return;

    if (!strcasecmp(name,"AddText"))
    {
        // deselect the previous object, if any
        if (selectedIndex != SIZET_NOT_FOUND)
        {
            objlist[selectedIndex]->Select(false);
        }

        int x = (GetScreenFrame().xmax - GetScreenFrame().xmin)/2;
        int y = (GetScreenFrame().ymax - GetScreenFrame().ymin)/2;
        SketchText *text = new SketchText(x,y,value,this);
        selectedIndex = objlist.Push(text);
        objlist[selectedIndex]->Select(true);
    }
    else if (!strcasecmp(name,"ChangeName"))
    {
        sketchName = value;
        SetTitle(sketchName);
    }
    else if (!strcasecmp(name,"FileNameBox"))
    {
        //we got to load the sketch we were asked for
        iVFS* vfs = psengine->GetVFS();
        csString tempFileName;
        tempFileName.Format("/planeshift/userdata/sketches/%s", value);
        if (!vfs->Exists(tempFileName))
        {
            psSystemMessage msg(0, MSG_ERROR, "File not found!");
            msg.FireEvent();
            return;
        }
        csRef<iDataBuffer> buff = vfs->ReadFile(tempFileName); //reads the file
        csString sketchxml =  buff->operator*(); //converts what we have read in something our ParseSketch function can
                                              //understand
        objlist.Empty();                      //clears the objlist to start loading our new sketch from clean
        ParseSketch(sketchxml,true);          //loads our sketch, and check that it's under our limits
    }
}

void pawsSketchWindow::OnColorEntered(const char* /*name*/, int param, int color)
{
    colorPending = false;

    if (param == 0)
    {//cancel
        return;
    }

    if (selectedIndex != SIZET_NOT_FOUND)
    {
        if(color == 0)
        {
            color = -1;
        }
        objlist[selectedIndex]->color = color;
    }
}

void pawsSketchWindow::AddSketchLine()
{
    if ((int)objlist.GetSize() >= primCount)
    {
        psSystemMessage sysMsg( 0, MSG_ERROR, PawsManager::GetSingleton().Translate("Your sketch is too complex for you to add more.") );
        sysMsg.FireEvent();
        return;
    }

    int x = (GetScreenFrame().xmax - GetScreenFrame().xmin)/2;
    int y = (GetScreenFrame().ymax - GetScreenFrame().ymin)/2;
    int x2 = x + 50;
    int y2 = y + 50;

    SketchLine *line = new SketchLine(x,y,x2,y2,this);
    objlist.Push(line);
}

void pawsSketchWindow::AddSketchBezier()
{
    if ((int)objlist.GetSize() >= primCount)
    {
        psSystemMessage sysMsg( 0, MSG_ERROR, PawsManager::GetSingleton().Translate("Your sketch is too complex for you to add more.") );
        sysMsg.FireEvent();
        return;
    }
    int x = (GetScreenFrame().xmax - GetScreenFrame().xmin)/2 -50;
    int y = (GetScreenFrame().ymax - GetScreenFrame().ymin)/2 -50;

    // deselect the previous object, if any
    if (selectedIndex != SIZET_NOT_FOUND)
    {
        objlist[selectedIndex]->Select(false);
    }

    SketchBezier *bezier = new SketchBezier(x,y, x, y+50, x+50,y+50 , x+50, y,this);
    selectedIndex = objlist.Push(bezier);
    objlist[selectedIndex]->Select(true);
}

void pawsSketchWindow::AddSketchIcon()
{
    if (iconList.GetSize() == 0)
        return;

    if ((int)objlist.GetSize() >= primCount)
    {
        psSystemMessage sysMsg( 0, MSG_ERROR, PawsManager::GetSingleton().Translate("Your sketch is too complex for you to add more.") );
        sysMsg.FireEvent();
        return;
    }

    // printf("Adding sketch icon now.\n");

    int x = (GetScreenFrame().xmax - GetScreenFrame().xmin)/2;
    int y = (GetScreenFrame().ymax - GetScreenFrame().ymin)/2;

    // deselect the previous object, if any
    if (selectedIndex != SIZET_NOT_FOUND)
    {
        objlist[selectedIndex]->Select(false);
    }

    SketchIcon *icon = new SketchIcon(x,y,iconList[0],this);
    selectedIndex = objlist.Push(icon);
    objlist[selectedIndex]->Select(true);
}

void pawsSketchWindow::NextPrevIcon(int delta)
{
    if (selectedIndex == SIZET_NOT_FOUND)
        return;

    csString iconName = objlist[selectedIndex]->GetStr();

    // Find which icon is currently selected
    for (size_t whichIcon=0; whichIcon < iconList.GetSize(); whichIcon++)
    {
        if (iconName == iconList[whichIcon]) // found
        {
            whichIcon = (whichIcon + delta) % iconList.GetSize();
            objlist[selectedIndex]->SetStr(iconList[whichIcon]);
            break;
        }
    }
}

void pawsSketchWindow::MoveObject(int dx, int dy)
{
    if (selectedIndex == SIZET_NOT_FOUND)
        return;

    /*objlist[selectedIndex]->UpdatePosition(objlist[selectedIndex]->x + dx,
                                           objlist[selectedIndex]->y + dy);*/
    objlist[selectedIndex]->UpdatePosition(dx, dy);
}

void pawsSketchWindow::RemoveSelected()
{
    if (selectedIndex != SIZET_NOT_FOUND)
    {
        objlist.DeleteIndex(selectedIndex);
        selectedIndex = SIZET_NOT_FOUND;
        // We don't set the mouse pointer anymore. It flickered on setting/unsetting and made problems with the offset. (icon jumped on click)
        //if (IsMouseDown()) //if the mouse is being used we need to restore it's shape
        //    PawsManager::GetSingleton().GetMouse()->ChangeImage("Skins Normal Mouse Pointer");
    }
}

void pawsSketchWindow::ChangeSketchName()
{
    if (!stringPending)
    {
        stringPending = true;

        // This window calls OnStringEntered when Ok is pressed.
        pawsStringPromptWindow::Create("Sketch Name", sketchName,
            false, 220, 20, this, "ChangeName");
    }
}

void pawsSketchWindow::SaveSketch()
{
    csString xml = toXML();
    iVFS* vfs = psengine->GetVFS();
    unsigned int tempNumber = 0;
    //searchs for a free sketch slot in the user directory and write in it
    csString tempFileNameTemplate = "/planeshift/userdata/sketches/%s.xml", tempFileName;
    if (filenameSafe(sketchName).Length())
    {
        tempFileName.Format(tempFileNameTemplate, filenameSafe(sketchName).GetData());
    }
    else
    {
        tempFileNameTemplate = "/planeshift/userdata/sketches/sketch%d.xml";
        do
        {
            tempFileName.Format(tempFileNameTemplate, tempNumber);
            tempNumber++;
        } while (vfs->Exists(tempFileName));
    }

    vfs->WriteFile(tempFileName, xml, xml.Length());
    psSystemMessage msg(0, MSG_ACK, "Sketch saved to %s", tempFileName.GetData()+30 );
    msg.FireEvent();
}

void pawsSketchWindow::LoadSketch()
{
    if (!stringPending)
    {
        stringPending = true;

        //This window calls OnStringEntered when Ok is pressed.
        //Asks to the user the file name of the sketch inside the sketches directory under their user directory
        pawsStringPromptWindow::Create("Enter local filename", "",
            false, 220, 20, this, "FileNameBox",0,true);
    }
}

bool pawsSketchWindow::OnMouseDown( int button, int modifiers, int x, int y )
{
    if (!editMode || colorPending)
        return pawsWidget::OnMouseDown(button, modifiers,x,y);

    mouseDown = true;
    mouseDownX = x;
    mouseDownY = y;
    for (size_t i=0; i<objlist.GetSize(); i++)
    {
        if (objlist[i]->IsHit(x,y))
        {
            // Unselect old object
            if (selectedIndex != SIZET_NOT_FOUND)
            {
                // printf("Deselect object %d.\n", selectedIndex);
                objlist[selectedIndex]->Select(false);
            }

            // Select new object
            selectedIndex = i;
            objlist[i]->Select(true);
            // printf("Select object %d.\n", selectedIndex);

            // flickers and makes the icon jump
            //PawsManager::GetSingleton().GetMouse()->ChangeImage( objlist[i]->GetStr() );
            return true;
        }
    }

    if (selectedIndex != SIZET_NOT_FOUND)
    {
        // User clicked on nothing, so deselect whatever was selected
        objlist[selectedIndex]->Select(false);
        selectedIndex = SIZET_NOT_FOUND;
    }
    // MouseUp doesn't get fired here IF MouseDown was handled by pawsWidget.
    mouseDown = false; // thus we need to prevent having a wrong "mouseDown" state.
    return pawsWidget::OnMouseDown(button, modifiers,x,y);
}

bool pawsSketchWindow::OnMouseUp( int button, int modifiers, int x, int y )
{
    if (!editMode)
        return pawsWidget::OnMouseUp(button, modifiers,x,y);

    mouseDown = false;

    if (selectedIndex != SIZET_NOT_FOUND)
    {
        // flickers and makes the icon jump
        //PawsManager::GetSingleton().GetMouse()->ChangeImage("Skins Normal Mouse Pointer");
        dirty = true;
        return true;
    }
    return pawsWidget::OnMouseUp(button, modifiers,x,y);
}

bool pawsSketchWindow::ParseLimits(const char *xmlstr)
{
    csRef<iDocumentSystem>  xml;
    xml =  csQueryRegistry<iDocumentSystem> ( psengine->GetObjectRegistry() );
    if (!xml)
        xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( xmlstr );
    if ( error )
    {
        Error2("Error parsing Sketch string: %s", error);
        return false;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Sketch xml");
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode("limits");
    if(!topNode)
    {
        Error1("No <pages> tag in Sketch xml");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    iconList.Empty();
    primCount = 0;
    readOnly  = false;

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        csString type = node->GetValue();
        if (type=="ic")
        {
            iconList.Push(node->GetContentsValue());
            Notify2(LOG_ANY,"Got icon %s\n", node->GetContentsValue());
        }
        else if (type=="count")
        {
            primCount = node->GetContentsValueAsInt();
            Notify2(LOG_ANY,"Got max primitives count of %d.\n", primCount);
        }
        else if (type=="rdonly")
        {
            readOnly = true;
            Notify1(LOG_ANY,"Map is read-only for this player.\n");
        }
    }
    return true;
}

bool pawsSketchWindow::ParseSketch(const char *xmlstr, bool checklimits)
{
    csRef<iDocumentSystem>  xml;
    xml =  csQueryRegistry<iDocumentSystem> ( psengine->GetObjectRegistry() );
    if (!xml)
        xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( xmlstr );
    if ( error )
    {
        Error2( "Error parsing Sketch string: %s", error );
        return false;
    }
     csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Sketch xml");
        return false;
    }
   csRef<iDocumentNode> topNode = root->GetNode("pages");
    if(!topNode)
    {
        Error1("No <pages> tag in Sketch xml");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes("page");

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();
        MoveTo(GetActualWidth(node->GetAttributeValueAsInt("l")),GetActualHeight(node->GetAttributeValueAsInt("t")));
        screenFrame.SetSize(GetActualWidth(node->GetAttributeValueAsInt("w")),GetActualHeight(node->GetAttributeValueAsInt("h")));

        defaultFrame.xmin = GetActualWidth(node->GetAttributeValueAsInt("l"));
        defaultFrame.ymin = GetActualHeight(node->GetAttributeValueAsInt("y"));
        int width  = GetActualWidth(node->GetAttributeValueAsInt("w"));
        int height = GetActualHeight(node->GetAttributeValueAsInt("h"));
        defaultFrame.SetSize(width, height);

        csRef<iDocumentNodeIterator> pagenodes = node->GetNodes();
        while (pagenodes->HasNext() )
        {
            SketchObject *obj=NULL;
            csRef<iDocumentNode> tmp = pagenodes->Next();
            csString type = tmp->GetValue();
            if(checklimits && (int)objlist.GetSize() >= primCount) //are we under the limitations?...
                break; //...it seems we aren't so break before going on, at least one part of sketch is available
            // Determine what type to create, and create it
            if (type == "ic") // icon
                obj = new SketchIcon;
            else if (type == "tx") // text
                obj = new SketchText;
            else if (type == "back") // background specifier
                ; // do nothing
            else if (type == "ln")
                obj = new SketchLine;
            else if (type == "bc") // bezier curve
                obj = new SketchBezier;
            else
            {
                Error2("Illegal sketch op <%s>.  Sketch cannot display.", type.GetDataSafe() );
                // The user has to be aware of that without looking into the "Error log".
                psSystemMessage sysMsg( 0, MSG_ERROR, PawsManager::GetSingleton().Translate("Illegal sketch op. Sketch cannot display.") );
                sysMsg.FireEvent();
                return false;
            }

            if (obj)
            {
                // Now load it
                if (!obj->Load(tmp,this))
                {
                    delete obj;
                    return false;
                }
                objlist.Push(obj);
            }
        }
        /// TODO: Support multiple pages here.
    }


    OnResize();

    return true;
}

pawsSketchWindow::SketchIcon::SketchIcon(int _x, int _y, const char *icon, pawsSketchWindow *parent)
{
    Init(_x,_y,icon,parent);
}

bool pawsSketchWindow::SketchIcon::Init(int _x, int _y, const char *icon, pawsSketchWindow *parent)
{
    x = _x;
    y = _y;
    str = icon;

    iconImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(str);
    // printf("Icon %s at (%d,%d)\n",str.GetDataSafe(),x,y);

    if (parent)
        this->parent = parent;

    return true;
}

void pawsSketchWindow::SketchIcon::SetStr(const char *s)
{
    Init(x,y,s,NULL);
    // printf("Changed icon to %s.\n",s);
}

bool pawsSketchWindow::SketchIcon::Load(iDocumentNode *node, pawsSketchWindow *parent)
{
    if (!node->GetAttributeValue("i"))
        return false;

    return Init(node->GetAttributeValueAsInt("x"),
                node->GetAttributeValueAsInt("y"),
                node->GetAttributeValue("i"), // TODO: Convert to common_strings
                parent);
}

void pawsSketchWindow::SketchIcon::WriteXml(csString& xml)
{
    csString add;

    add.Format("<ic x=\"%d\" y=\"%d\" i=\"%s\"/>",x,y,str.GetDataSafe());
    xml += add;
}

void pawsSketchWindow::SketchIcon::Draw()
{
    if (!selected || parent->IsMouseDown() || frame > 15) // blink icon only if it isn't dragged.
    {
        // replacing the mouse cursor with the icon flickers and makes the icon jump.
        //if (!parent->IsMouseDown() || !selected)
            iconImage->Draw(parent->GetActualWidth(x) + parent->GetScreenFrame().xmin,
                            parent->GetActualHeight(y) + parent->GetScreenFrame().ymin,
                            parent->GetActualWidth(iconImage->GetWidth()),
                            parent->GetActualHeight(iconImage->GetHeight()));
    }
    if (selected)
        frame = (frame > 29) ? 0 : frame+1;

}

bool pawsSketchWindow::SketchIcon::IsHit(int mouseX, int mouseY)
{
    csRect rect = parent->GetScreenFrame();
    rect.xmin += parent->GetActualWidth(x);
    rect.ymin += parent->GetActualHeight(y);
    rect.xmax = rect.xmin + iconImage->GetWidth();
    rect.ymax = rect.ymin + iconImage->GetHeight();

    return rect.Contains(mouseX,mouseY);
}

bool pawsSketchWindow::SketchText::Load(iDocumentNode *node, pawsSketchWindow *parent)
{
    x = node->GetAttributeValueAsInt("x");
    y = node->GetAttributeValueAsInt("y");
    if (!node->GetAttributeValue("t"))
        return false;
    str = node->GetAttributeValue("t"); // TODO: Convert to common_strings
    // printf("Text %s at (%d,%d)\n",str.GetDataSafe(),x,y);

    psString col = node->GetAttributeValue("col");
    col.Upcase();
    const char* colc = col.GetDataSafe();
    int i = 0;
    int val = 0;
    int tot = 0;
    while(colc[i] != 0)
    {
        val = colc[i] - '0';
        if (9 < val)
            val -= 7;
        tot <<= 4;
        tot |= (val & 0x0f);
        i++;
    }
    color = tot;

    this->parent = parent;
    return true;
}


void pawsSketchWindow::SketchText::WriteXml(csString& xml)
{
    csString txt = EscpXML(str);

    if(color == -1)
    {
        xml.AppendFmt("<tx x=\"%d\" y=\"%d\" t=\"%s\"/>", x, y, txt.GetData());
    }
    else
    {
        xml.AppendFmt("<tx x=\"%d\" y=\"%d\" t=\"%s\" col=\"%X\"/>", x, y, txt.GetData(), color);
    }
}


void pawsSketchWindow::SketchText::Draw()
{
    if (parent->IsMouseDown() || !selected || frame > 15)
    {
        if(color == -1)
        {
            parent->DrawWidgetText(str,
                               parent->GetActualWidth(x)+parent->GetScreenFrame().xmin,
                               parent->GetActualHeight(y)+parent->GetScreenFrame().ymin);
        }
        else
        {
            parent->DrawColorWidgetText(str,
                               parent->GetActualWidth(x)+parent->GetScreenFrame().xmin,
                               parent->GetActualHeight(y)+parent->GetScreenFrame().ymin,
                               color);

        }
    }
    if (selected)
        frame = (frame > 29) ? 0 : frame+1;
}

bool pawsSketchWindow::SketchText::IsHit(int mouseX, int mouseY)
{
    csRect rect = parent->GetScreenFrame();
    rect.xmin += parent->GetActualWidth(x);
    rect.ymin += parent->GetActualHeight(y);

    rect = parent->GetWidgetTextRect(str,rect.xmin, rect.ymin);
    return rect.Contains(mouseX,mouseY);
}


bool pawsSketchWindow::SketchLine::Load(iDocumentNode *node, pawsSketchWindow *parent)
{
    psString pts = node->GetAttributeValue("pts");
    psString num;

    pts.GetWordNumber(1,num);
    x   = atoi(num.GetDataSafe());
    pts.GetWordNumber(2,num);
    y   = atoi(num.GetDataSafe());
    pts.GetWordNumber(3,num);
    x2  = atoi(num.GetDataSafe());
    pts.GetWordNumber(4,num);
    y2  = atoi(num.GetDataSafe());

    psString col = node->GetAttributeValue("col");
    col.Upcase();
    const char* colc = col.GetDataSafe();
    int i = 0;
    int val = 0;
    int tot = 0;
    while(colc[i] != 0)
    {
        val = colc[i] - '0';
        if (9 < val)
            val -= 7;
        tot <<= 4;
        tot |= (val & 0x0f);
        i++;
    }
    color = tot;
    //printf("Line %d,%d to %d, %d\n",x,y,x2,y2);

    this->parent = parent;
    return true;
}

void pawsSketchWindow::SketchLine::WriteXml(csString& xml)
{
    csString add;

    if(color == -1)
    {
        add.Format("<ln pts=\"%d %d %d %d\"/>",x,y,x2,y2);
    }
    else
    {
        add.Format("<ln pts=\"%d %d %d %d\" col=\"%X\"/>",x,y,x2,y2,color);
    }
    xml += add;
}

void pawsSketchWindow::SketchLine::Draw()
{
    iGraphics2D *graphics2D = parent->GetG2D();

    int black = graphics2D->FindRGB( 0,0,0 );
    int _color;

    bool showBoxes, showFirstBox, showSecondBox;
    if(selected && !colorSet)
    {
        _color = graphics2D->FindRGB( 150,0,0 );
    }
    else
    {
        _color = (color==-1)? black : color;
    }

    showBoxes = (!selected || (frame > 15 && !parent->mouseDown));
    showFirstBox = (showBoxes || dragMode == 2);
    showSecondBox = (showBoxes || dragMode == 1);


    graphics2D->DrawLine( parent->GetActualWidth(x)+parent->GetScreenFrame().xmin,
                          parent->GetActualHeight(y)+parent->GetScreenFrame().ymin,
                          parent->GetActualWidth(x2)+parent->GetScreenFrame().xmin,
                          parent->GetActualHeight(y2)+parent->GetScreenFrame().ymin,
                          _color );

    //if (parent->IsMouseDown() || !selected || frame > 15)
    //{
    if(showFirstBox)
    {
        parent->DrawBlackBox(parent->GetActualWidth(x)+parent->GetScreenFrame().xmin-3,
                             parent->GetActualHeight(y)+parent->GetScreenFrame().ymin-3);
    }
    if(showSecondBox)
    {
        parent->DrawBlackBox(parent->GetActualWidth(x2)+parent->GetScreenFrame().xmin-3,
                             parent->GetActualHeight(y2)+parent->GetScreenFrame().ymin-3);
    }
    //}
    if (selected)
        frame = (frame > 29) ? 0 : frame+1;
}

bool pawsSketchWindow::SketchLine::IsHit(int mouseX, int mouseY)
{
    csRect rect = parent->GetScreenFrame();
    mouseX -= rect.xmin;
    mouseY -= rect.ymin;

    mouseX = parent->GetLogicalWidth(mouseX);
    mouseY = parent->GetLogicalHeight(mouseY);

    dragMode = 0;

    if (abs(mouseX-x) < 3 &&
        abs(mouseY-y) < 3)
    {
        dragMode = 1;
        offsetX = mouseX - x;
        offsetY = mouseY - y;
        return true;
    }
    if (abs(mouseX-x2) < 3 &&
        abs(mouseY-y2) < 3)
    {
        dragMode = 2;
        offsetX = mouseX - x2;
        offsetY = mouseY - y2;
        return true;
    }

    rect.xmax = x2;
    rect.ymax = y2;
    rect.xmin = x;
    rect.ymin = y;

    rect.Normalize();

    int xDiff, yDiff;
    xDiff = rect.xmax - rect.xmin;
    yDiff = rect.ymax - rect.ymin;

    // give a chance to select horizontal Lines
    if(xDiff < 3)
    {
        rect.xmax += 3 -xDiff; //correct would be 3-xDiff/2 (but we an expensive division isn't really required here. a diff of 6 or 5 doesn't really make a difference.
        rect.xmin -= 3 -xDiff;
    }

    // give a chance to select vertical Lines
    if(yDiff < 3)
    {
        rect.ymax += 3 -yDiff;
        rect.ymin -= 3 -yDiff;
    }

    if (!rect.Contains(mouseX,mouseY))
        return false;

    // calculate the normal distance of the point (mouse pos) to the line:
    //        A (vector)
    //     .-'| normalDist
    //  .-'   |
    // B-------------------B1 (vector B, origin at x, y of this line)
    csVector2 b1(x, y);
    csVector2 a(mouseX, mouseY);
    a-=b1;
    csVector2 b(x2, y2);
    b-=b1;

    csVector2 c;
    float dotab = ( a.x * b.x ) + ( a.y * b.y );
    float dotb = ( b.x * b.x ) + ( b.y * b.y );
    float dotabDivb = dotab / dotb;
    c.x = b.x * dotabDivb;
    c.y = b.y * dotabDivb;

    csVector2 e(a - c);
    float distFromLine = e.Norm();
    if(!(distFromLine <= 2.5 && distFromLine >= -2.5))
    {
        return false;
    }

    offsetX = mouseX - x;
    offsetY = mouseY - y;
    return true;
}

bool pawsSketchWindow::SketchBezier::Load(iDocumentNode *node, pawsSketchWindow *parent)
{
    psString pts = node->GetAttributeValue("pts");
    psString num;

    int x0, y0, x1, y1, x2, y2, x3, y3;
    pts.GetWordNumber(1,num);
    x0 = atoi(num.GetDataSafe());
    pts.GetWordNumber(2,num);
    y0 = atoi(num.GetDataSafe());
    pts.GetWordNumber(3,num);
    x1 = atoi(num.GetDataSafe());
    pts.GetWordNumber(4,num);
    y1 = atoi(num.GetDataSafe());
    pts.GetWordNumber(5,num);
    x2 = atoi(num.GetDataSafe());
    pts.GetWordNumber(6,num);
    y2 = atoi(num.GetDataSafe());
    pts.GetWordNumber(7,num);
    x3 = atoi(num.GetDataSafe());
    pts.GetWordNumber(8,num);
    y3 = atoi(num.GetDataSafe());

    this->parent = parent;
    p0p1.parent = parent;
    p0p1.x = x0; // point 0 (also start point)
    p0p1.y = y0;
    p0p1.x2 = x1; // point 1 (control point 1)
    p0p1.y2 = y1;

    p0p3.parent = parent;
    p0p3.x = x0; // point 0
    p0p3.y = y0;
    p0p3.x2 = x3; // point 3 (also end point)
    p0p3.y2 = y3;

    p3p2.parent = parent;
    p3p2.x2 = x2; // point 3 (also end point)
    p3p2.y2 = y2;
    p3p2.x = x3; // point 2 (control point 2)
    p3p2.y = y3;

    psString col = node->GetAttributeValue("col");
    col.Upcase();
    const char* colc = col.GetDataSafe();
    int i = 0;
    int val = 0;
    int tot = 0;
    while(colc[i] != 0)
    {
        val = colc[i] - '0';
        if (9 < val)
            val -= 7;
        tot <<= 4;
        tot |= (val & 0x0f);
        i++;
    }
    color = tot;

    return true;
}

void pawsSketchWindow::SketchBezier::WriteXml(csString& xml)
{
    csString add;

    if(color == -1)
    {
                            //x0 y0 x1 y1 x2 y2 x3 y3
        add.Format("<bc pts=\"%d %d %d %d %d %d %d %d\"/>", p0p1.x, p0p1.y, p0p1.x2, p0p1.y2,
                                                            p3p2.x2, p3p2.y2, p3p2.x, p3p2.y);
    }
    else
    {
                            //x0 y0 x1 y1 x2 y2 x3 y3
        add.Format("<bc pts=\"%d %d %d %d %d %d %d %d\" col=\"%X\"/>", p0p1.x, p0p1.y, p0p1.x2, p0p1.y2,
                                                                       p3p2.x2, p3p2.y2, p3p2.x, p3p2.y, color);
    }
    xml += add;
}

void pawsSketchWindow::SketchBezier::Draw()
{
    iGraphics2D *graphics2D = parent->GetG2D();
    int black = graphics2D->FindRGB( 0,0,0 );
    int _color = (color==-1)? black : color;

    if(parent->editMode)
    {
        if(!this->selected)
        { // update selection of lines in case this got delelected;
            p0p3.Select(false);
            p0p1.Select(false);
            p3p2.Select(false);
        }
        else // we are selected, make sure that the "base control line" is selected, if none other is
        {
            if(!p0p1.selected && !p0p3.selected && !p3p2.selected)
            {
                p0p3.Select(true);
            }
        }
        // use nice colours (for easier understanding) for the control lines.
        int blue = graphics2D->FindRGB( 0,0,150 );
        int green = graphics2D->FindRGB( 0,150,0 );
        // draws lines and boxes and makes them blink dependant on their selection state
        p0p3.Draw();
        p0p1.Draw();
        p3p2.Draw();

        // draw those "handle lines" which aren't selected in another colour
        // main line: blue
        if(!p0p3.selected)
        {
             graphics2D->DrawLine( parent->GetActualWidth(p0p3.x)+parent->GetScreenFrame().xmin,
                              parent->GetActualHeight(p0p3.y)+parent->GetScreenFrame().ymin,
                              parent->GetActualWidth(p0p3.x2)+parent->GetScreenFrame().xmin,
                              parent->GetActualHeight(p0p3.y2)+parent->GetScreenFrame().ymin,
                              blue );
        }

        // Handle 1: green
        if(!p0p1.selected)
        {
             graphics2D->DrawLine( parent->GetActualWidth(p0p1.x)+parent->GetScreenFrame().xmin,
                              parent->GetActualHeight(p0p1.y)+parent->GetScreenFrame().ymin,
                              parent->GetActualWidth(p0p1.x2)+parent->GetScreenFrame().xmin,
                              parent->GetActualHeight(p0p1.y2)+parent->GetScreenFrame().ymin,
                              green );
        }

        // Handle 2: green
        if(!p3p2.selected)
        {
             graphics2D->DrawLine( parent->GetActualWidth(p3p2.x)+parent->GetScreenFrame().xmin,
                              parent->GetActualHeight(p3p2.y)+parent->GetScreenFrame().ymin,
                              parent->GetActualWidth(p3p2.x2)+parent->GetScreenFrame().xmin,
                              parent->GetActualHeight(p3p2.y2)+parent->GetScreenFrame().ymin,
                              green );
        }
    }

    // The actual bezier line drawing
    int segX, segY, segX1=0, segY1=0;

    // no need to compute the first point.
    segX = p0p1.x;
    segY = p0p1.y;
    // skip 0,1,2,3    we use the total array offset, thus *4; +4
    for(int i = 4; i < SKETCHBEZIER_SEGMENTS*4 /*resolution*/; i+=4)
    {
        // This is how the unoptimized code would look (alike - if we have Math.Pow)
        // P(t) = (1-t)^3P0 + 3(1-t)^2tP1 + 3(1-t)t^2P2 + t^3P3 with t running from 0 to 1.
        //float t = ((float)i/(float)20);
        /*float fsegX = Math.Pow((1-t), 3) *p0p1.x
            + 3* t * Math.Pow((1-t), 2) *p0p1.x2
            + 3* Math.Pow(t, 2)*(1-t)*p3p2.x2
            + Math.Pow(t, 3)*p3p2.x;
        float fsegY = Math.Pow((1-t), 3) *p0p1.y
            + 3* t * Math.Pow((1-t), 2) *p0p1.y2
            + 3* Math.Pow(t, 2)*(1-t)*p3p2.y2
            + Math.Pow(t, 3)*p3p2.y;*/

        float fsegX = parent->bezierWeights.Get(i) * p0p1.x +
                      parent->bezierWeights.Get(i+1) * p0p1.x2 +
                      parent->bezierWeights.Get(i+2) * p3p2.x2 +
                      parent->bezierWeights.Get(i+3) * p3p2.x;

        float fsegY = parent->bezierWeights.Get(i) * p0p1.y +
                      parent->bezierWeights.Get(i+1) * p0p1.y2 +
                      parent->bezierWeights.Get(i+2) * p3p2.y2 +
                      parent->bezierWeights.Get(i+3) * p3p2.y;

        // this makes sure that we always draw a line from the "previous" calculated point to the newly calculated point
        if(i&4)
        {
            segX1 = (int)(fsegX + 0.5);
            segY1 = (int)(fsegY + 0.5);
        }
        else
        {
            segX = (int)(fsegX + 0.5);
            segY = (int)(fsegY + 0.5);
        }

        graphics2D->DrawLine( parent->GetActualWidth(segX)+parent->GetScreenFrame().xmin,
                          parent->GetActualHeight(segY)+parent->GetScreenFrame().ymin,
                          parent->GetActualWidth(segX1)+parent->GetScreenFrame().xmin,
                          parent->GetActualHeight(segY1)+parent->GetScreenFrame().ymin,
                          _color );
    }

    // no need to calculate the last point either
    if(SKETCHBEZIER_SEGMENTS&1)
    {
        segX1 = p3p2.x;
        segY1 = p3p2.y;
    }
    else
    {
        segX = p3p2.x;
        segY = p3p2.y;
    }
    graphics2D->DrawLine( parent->GetActualWidth(segX)+parent->GetScreenFrame().xmin,
                          parent->GetActualHeight(segY)+parent->GetScreenFrame().ymin,
                          parent->GetActualWidth(segX1)+parent->GetScreenFrame().xmin,
                          parent->GetActualHeight(segY1)+parent->GetScreenFrame().ymin,
                          _color );
}

bool pawsSketchWindow::SketchBezier::IsHit(int mouseX, int mouseY)
{
    p0p3.Select(false);
    p0p1.Select(false);
    p3p2.Select(false);

    if(p0p3.IsHit(mouseX, mouseY))
    {
        p0p3.Select(true);
        //p0p3.selected = true;
        // this is needed to have the boxes blink. We MUST keep the right order in Draw and UpdatePosition!!
        //p0p1.selected = true;
        //p0p1.dragMode = 1;
        //p3p2.selected = true;
        //p3p2.dragMode = 1;
        return true;
    }
    if(p0p1.IsHit(mouseX, mouseY))
    {
        p0p1.Select(true);
        //p0p1.selected = true;
        // for blinking box
        //p0p3.selected = true;
        //p0p3.dragMode = 1;
        return true;
    }
    if(p3p2.IsHit(mouseX, mouseY))
    {
        p3p2.Select(true);
        //p3p2.selected = true;
        // for blinking box
        //p0p3.selected = true;
        //p0p3.dragMode = 2;
        return true;
    }
    return false;
}

void pawsSketchWindow::SketchLine::UpdatePosition(int _x, int _y)
{
    int dx, dy, newX, newY, newX2, newY2;
    csRect rect = parent->GetScreenFrame();

    rect.xmax -= rect.xmin;
    rect.ymax -= rect.ymin;
    rect.xmin = 0;
    rect.ymin = 0;

    rect.Normalize();

    // _x, _y are already expected to be RELATIVE!
    dx = _x;
    dy = _y;

    switch (dragMode)
    {
        case 0:
            //drag Line
            newX = x + dx;
            newY = y + dy;
            newX2 = x2 + dx;
            newY2 = y2 + dy;
            if(rect.ClipLineSafe(newX,newY,newX2,newY2))
            {
                x = newX;
                y = newY;
                x2 = newX2;
                y2 = newY2;
            }
            break;
        case 1:
            //move the first point x,y
            newX = x + dx;
            newY = y + dy;
            newX2 = x2;
            newY2 = y2;
            if(rect.ClipLineSafe(newX,newY,newX2,newY2))
            {
                x = newX;
                y = newY;
                x2 = newX2;
                y2 = newY2;
            }
            break;
        case 2:
            //move the second point x2,y2
            newX = x;
            newY = y;
            newX2 = x2 + dx;
            newY2 = y2 + dy;
            if(rect.ClipLineSafe(newX,newY,newX2,newY2))
            {
                x = newX;
                y = newY;
                x2 = newX2;
                y2 = newY2;
            }
            break;
    }
}

void pawsSketchWindow::SketchBezier::UpdatePosition(int _x, int _y)
{
    int xDiff;
    int yDiff;
    // Error3("SB Update: %d, %d", _x, _y);
    if(p0p3.selected)
    {
        xDiff = p0p3.x;
        yDiff = p0p3.y;
        p0p3.UpdatePosition(_x, _y);
        xDiff -= p0p3.x;
        yDiff -= p0p3.y;
        p0p1.x = p0p3.x;
        p0p1.y = p0p3.y;
        p3p2.x = p0p3.x2;
        p3p2.y = p0p3.y2;
        if(p0p3.dragMode == 0)
        {
            //p3p2.x2 += xDiff; // interesting rot
            //p3p2.y2 += yDiff;
            //p0p1.x2 += xDiff;
            //p0p1.y2 += yDiff;

            p3p2.x2 -= xDiff;
            p3p2.y2 -= yDiff;
            p0p1.x2 -= xDiff;
            p0p1.y2 -= yDiff;
        }
    }
    else if(p0p1.selected)
    {
        p0p1.UpdatePosition(_x, _y);
        p0p3.x = p0p1.x;
        p0p3.y = p0p1.y;
    }
    else if(p3p2.selected)
    {
        p3p2.UpdatePosition( _x, _y);
        p0p3.x2 = p3p2.x;
        p0p3.y2 = p3p2.y;
    }
}

void pawsSketchWindow::SketchIcon::UpdatePosition (int _x, int _y){
    int dx, dy, newX, newY, newX2, newY2, x2Updated, y2Updated;
    csRect rect = parent->GetScreenFrame();
    rect.xmax -= rect.xmin;
    rect.ymax -= rect.ymin;
    rect.xmin = 0;
    rect.ymin = 0;

    rect.Normalize();

    // _x, _y are expected to be RELATIVE!
    dx = _x;
    dy = _y;

    x2Updated = x + iconImage->GetWidth() + dx;
    y2Updated = y + iconImage->GetHeight() + dy;

    newX = x + dx;
    newY = y + dy;
    newX2 = x2Updated;
    newY2 = y2Updated;

    //test for one diagonal only if the point up move the obj
    //are on one corner
    if ( rect.ClipLineSafe(newX,newY,newX2,newY2) )
    {
        if ( newX == ( x + dx ) && newY == ( y + dy ))
        {
            x = newX - ( x2Updated - newX2 );
            y = newY - ( y2Updated - newY2 );
        }
    }
}

void pawsSketchWindow::SketchText::UpdatePosition (int _x, int _y){

    int dx, dy, newX, newY, newX2, newY2, x2Updated, y2Updated;
    csRect rect = parent->GetScreenFrame();
    csRect rectText = parent->GetScreenFrame();

    rect.xmax -= rect.xmin;
    rect.ymax -= rect.ymin;
    rect.xmin = 0;
    rect.ymin = 0;

    rectText = parent->GetWidgetTextRect(str, x, y);

    // _x, _y are expected to be RELATIVE!
    dx = _x;
    dy = _y;

    x2Updated = rectText.xmax + dx;
    y2Updated = rectText.ymax + dy;

    newX = rectText.xmin + dx;
    newY = rectText.ymin + dy;
    newX2 = x2Updated;
    newY2 = y2Updated;

    //test for one diagonal only if the point up move the obj
    //are on one corner
    if ( rect.ClipLineSafe(newX,newY,newX2,newY2) )
    {
        if ( newX == ( x + dx ) && newY == ( y + dy ))
        {
            x = newX - ( x2Updated - newX2 );
            y = newY - ( y2Updated - newY2 );
        }
    }
}

bool pawsSketchWindow::isBadChar(char c)
{
    csString badChars = "/\\?%*:|\"<>";
    if (badChars.FindFirst(c) == (size_t) -1)
        return false;
    else
        return true;
}

csString pawsSketchWindow::filenameSafe(const csString &original)
{
    csString safe;
    size_t len = original.Length();
    for (size_t c = 0; c < len; ++c) {
        if (!isBadChar(original[c]))
            safe += original[c];
    }
    return safe;
}


pawsSketchWindow::BezierWeights::BezierWeights()
{
    // using a single dimensional array to save memory and accelerate the access
    precomputedBezierWeights = new float[(SKETCHBEZIER_SEGMENTS+1) * 4];
    for(int i = 0; i <= (SKETCHBEZIER_SEGMENTS * 4); i+=4)
    {
        // t is a number from 0.0 to 1.0
        // t is used to calculate any given point of a cubic bezier curve, 0.0 gives the start point, 1.0 the end point
        // to calculate a point of a bezier curve, you have to use following formula
        // Pt is the point of position t, P0 - P3 are the 4 control points.
        // Pt = (1-t)^3*P0 + 3*t*((1-t)^2)*P1 + 3*(t^2)*(t-1)*P2 + (t^3)*P3
        // here we calculate everything (but the multiplication with the point) in advance for a given "Bezier Curve" resolution.
        // 0 will be the start point, SKETCHBEZIER_SEGMENTS+1 is the end point. (+1 because a line consists of two points. 21 Points = 20 Line segments)

        // t = i / numofSegments
        float t = ((float)(i/4) * (float)(1.0f/SKETCHBEZIER_SEGMENTS));

        precomputedBezierWeights[i] = (1-t)*(1-t)*(1-t);     //Math.Pow((1-t), 3);
        precomputedBezierWeights[i+1] = 3 * t * ((1-t)*(1-t)); //3* t * Math.Pow((1-t), 2);
        precomputedBezierWeights[i+2] = 3 * (t*t) * (1-t);     //3* Math.Pow(t, 2)*(1-t);
        precomputedBezierWeights[i+3] = t*t*t;                 //Math.Pow(t, 3);
    }
}

pawsSketchWindow::BezierWeights::~BezierWeights()
{
    delete [] precomputedBezierWeights;
}

