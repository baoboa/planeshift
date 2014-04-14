/*
 * pawsbutton.cpp - Author: Andrew Craig
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
// pawsbutton.cpp: implementation of the pawsButton class.
//
//////////////////////////////////////////////////////////////////////

#include <psconfig.h>
#include <ivideo/fontserv.h>
#include <iutil/evdefs.h>
#include <iutil/plugin.h>

#include <isoundmngr.h>

#include "pawsmainwidget.h"
#include "pawsmanager.h"
#include "pawsbutton.h"
#include "pawstexturemanager.h"
#include "pawsprefmanager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsButton::pawsButton()
    : enabled(true), upTextOffsetX(0), upTextOffsetY(0), downTextOffsetX(0), downTextOffsetY(0)
{
    down = false;
    notify = NULL;
    toggle = false;
    flash = 0;
    flashtype = FLASH_REGULAR;
    keybinding = 0;
    changeOnMouseOver = false;
    originalFontColour = -1;
    factory = "pawsButton";
}

pawsButton::pawsButton(const pawsButton &pb) :
    pawsWidget((const pawsWidget &)pb),
    down(pb.down),
    pressedImage(pb.pressedImage),
    releasedImage(pb.releasedImage),
    greyUpImage(pb.greyUpImage),
    greyDownImage(pb.greyDownImage),
    specialFlashImage(pb.specialFlashImage),
    toggle(pb.toggle),
    buttonLabel(pb.buttonLabel),
    keybinding(pb.keybinding),
    notify(pb.notify),
    style(pb.style),
    enabled(pb.enabled),
    flash(pb.flash),
    flashtype(pb.flashtype),
    sound_click(pb.sound_click),
    upTextOffsetX(pb.upTextOffsetX),
    upTextOffsetY(pb.upTextOffsetY),
    downTextOffsetX(pb.downTextOffsetX),
    downTextOffsetY(pb.downTextOffsetY),
    originalFontColour(pb.originalFontColour),
    changeOnMouseOver(pb.changeOnMouseOver)
{
    factory = "pawsButton";
}
bool pawsButton::Setup(iDocumentNode* node)
{
    // Check for toggle
    toggle = node->GetAttributeValueAsBool("toggle", true);

    // Check for keyboard shortcut for this button
    const char* key = node->GetAttributeValue("key");
    if(key)
    {
        if(!strcasecmp(key,"Enter"))
            keybinding = 10;
        else
            keybinding = *key;
    }
    // Check for sound to be associated to Buttondown
    csRef<iDocumentAttribute> soundAttribute = node->GetAttribute("sound");
    if(soundAttribute)
    {
        csString soundName = node->GetAttributeValue("sound");
        SetSound(soundName);
    }
    else
    {
        csString name2;

        csRef<iDocumentNode> buttonLabelNode = node->GetNode("label");
        if(buttonLabelNode)
            name2 = buttonLabelNode->GetAttributeValue("text");

        name2.Downcase();

        if(name2 == "ok")
            SetSound("gui.ok");
        else if(name2 == "quit")
            SetSound("gui.quit");
        else if(name2 == "cancel")
            SetSound("gui.cancel");
        else
            SetSound("sound.standardButtonClick");
    }

    // Check for notify widget
    csRef<iDocumentAttribute> notifyAttribute = node->GetAttribute("notify");
    if(notifyAttribute)
        notify = PawsManager::GetSingleton().FindWidget(notifyAttribute->GetValue());

    // Check for mouse over
    changeOnMouseOver = node->GetAttributeValueAsBool("changeonmouseover", false);

    // Get the down button image name.
    csRef<iDocumentNode> buttonDownImage = node->GetNode("buttondown");
    if(buttonDownImage)
    {
        csString downImageName = buttonDownImage->GetAttributeValue("resource");
        SetDownImage(downImageName);
        downTextOffsetX = buttonDownImage->GetAttributeValueAsInt("textoffsetx");
        downTextOffsetY = buttonDownImage->GetAttributeValueAsInt("textoffsety");
    }

    // Get the up button image name.
    csRef<iDocumentNode> buttonUpImage = node->GetNode("buttonup");
    if(buttonUpImage)
    {
        csString upImageName = buttonUpImage->GetAttributeValue("resource");
        SetUpImage(upImageName);
        upTextOffsetX = buttonUpImage->GetAttributeValueAsInt("textoffsetx");
        upTextOffsetY = buttonUpImage->GetAttributeValueAsInt("textoffsety");
    }

    // Get the down button image name.
    csRef<iDocumentNode> buttonGreyDownImage = node->GetNode("buttongraydown");
    if(buttonGreyDownImage)
    {
        csString greyDownImageName = buttonGreyDownImage->GetAttributeValue("resource");
        SetGreyUpImage(greyDownImageName);
    }

    // Get the up button image name.
    csRef<iDocumentNode> buttonGreyUpImage = node->GetNode("buttongrayup");
    if(buttonGreyUpImage)
    {
        csString greyUpImageName = buttonGreyDownImage->GetAttributeValue("resource");
        SetGreyUpImage(greyUpImageName);
    }

    // Get the "on char name flash" button image name.
    csRef<iDocumentNode> buttonSpecialImage = node->GetNode("buttonspecial");
    if(buttonSpecialImage)
    {
        csString onSpecialImageName = buttonSpecialImage->GetAttributeValue("resource");
        SetOnSpecialImage(onSpecialImageName);
    }


    // Get the button label
    csRef<iDocumentNode> buttonLabelNode = node->GetNode("label");
    if(buttonLabelNode)
    {
        buttonLabel = PawsManager::GetSingleton().Translate(buttonLabelNode->GetAttributeValue("text"));
    }

    originalFontColour = GetFontColour();

    return true;
}

bool pawsButton::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        SetText(node->GetAttributeValue("text"));
    }

    if(node->GetAttributeValue("down"))
    {
        SetState(strcmp(node->GetAttributeValue("down"),"true")==0);
    }

    return true;
}


void pawsButton::SetDownImage(const csString &image)
{
    pressedImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(image);
}

void pawsButton::SetUpImage(const csString &image)
{
    releasedImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(image);
}

void pawsButton::SetGreyUpImage(const csString &greyUpImage)
{
    this->greyUpImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(greyUpImage);
}

void pawsButton::SetGreyDownImage(const csString &greyDownImage)
{
    this->greyDownImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(greyDownImage);
}

void pawsButton::SetOnSpecialImage(const csString &image)
{
    specialFlashImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(image);
}


void pawsButton::SetSound(const char* soundName)
{
    sound_click = soundName;
}

void pawsButton::SetText(const char* text)
{
    buttonLabel = text;

    if(buttonLabel == "ok")
        SetSound("gui.ok");
    else if(buttonLabel == "quit")
        SetSound("gui.quit");
    else if(buttonLabel == "cancel")
        SetSound("gui.cancel");
    else
        SetSound("sound.standardButtonClick");
}


pawsButton::~pawsButton()
{
}

void pawsButton::Draw()
{
    pawsWidget::Draw();
    int drawAlpha = -1;
    if(parent && parent->GetMaxAlpha() >= 0)
    {
        fadeVal = parent->GetFadeVal();
        alpha = parent->GetMaxAlpha();
        alphaMin = parent->GetMinAlpha();
        drawAlpha = (int)(alphaMin + (alpha-alphaMin) * fadeVal * 0.010);
    }
    if(down)
    {
        if(!enabled && greyDownImage)
            greyDownImage->Draw(screenFrame, drawAlpha);
        else if(pressedImage)
            pressedImage->Draw(screenFrame, drawAlpha);
    }
    else if(flash==0)
    {
        if(!enabled && greyUpImage)
            greyUpImage->Draw(screenFrame, drawAlpha);
        else if(releasedImage)
            releasedImage->Draw(screenFrame, drawAlpha);
    }
    else // Flash the button if it's not depressed.
    {
        if(flashtype == FLASH_HIGHLIGHT)
        {
            SetColour(graphics2D->FindRGB(255,0,0));
            if(releasedImage)
                releasedImage->Draw(screenFrame, drawAlpha);
        }
        else
        {
            if(flash <= 10)
            {
                flash++;
                switch(flashtype)
                {
                    case FLASH_REGULAR:
                        if(pressedImage)
                            pressedImage->Draw(screenFrame);
                        break;
                    case FLASH_SPECIAL:
                        if(specialFlashImage)
                            specialFlashImage->Draw(screenFrame);
                        break;
                    default:
                        // Unexpected flash
                        Error1("Unknown flash type!");
                }
            }
            else
            {
                if(flash == 30)
                    flash = 1;
                else flash++;
                if(releasedImage) releasedImage->Draw(screenFrame, drawAlpha);
            }
        }
    }
    if(!(buttonLabel.IsEmpty()))
    {
        int drawX=0;
        int drawY=0;
        int width=0;
        int height=0;

        GetFont()->GetDimensions(buttonLabel , width, height);

        int midX = screenFrame.Width() / 2;
        int midY = screenFrame.Height() / 2;

        drawX = screenFrame.xmin + midX - width/2;
        drawY = screenFrame.ymin + midY - height/2;
        drawY -= 2; // correction

        if(down)
            DrawWidgetText(buttonLabel, drawX + downTextOffsetX, drawY + downTextOffsetY);
        else
            DrawWidgetText(buttonLabel, drawX + upTextOffsetX, drawY + upTextOffsetY);
    }
}

bool pawsButton::OnMouseEnter()
{
    if(changeOnMouseOver)
    {
        SetState(true, false);
    }

    return pawsWidget::OnMouseEnter();
}

bool pawsButton::OnMouseExit()
{
    if(changeOnMouseOver)
    {
        SetState(false, false);
    }

    return pawsWidget::OnMouseExit();
}

bool pawsButton::OnMouseDown(int button, int modifiers, int x, int y)
{

    if(!enabled)
    {
        return true;
    }
    else if(button == csmbWheelUp || button == csmbWheelDown || button == csmbHWheelLeft || button == csmbHWheelRight)
    {
        if(parent)
        {
            return parent->OnMouseDown(button, modifiers, x, y);
        }
        else
        {
            return false;
        }
    }

    // Play a sound
    PawsManager::GetSingleton().GetSoundManager()->PlaySound(sound_click, false, iSoundManager::GUI_SNDCTRL);

    if(toggle)
    {
        SetState(!IsDown());
    }
    else if(changeOnMouseOver)
    {
        SetState(false, false);
    }
    else
    {
        SetState(true, false);
    }

    if(notify != NULL)
    {
        return notify->CheckButtonPressed(button, modifiers, this);
    }
    else if(parent)
    {
        return parent->CheckButtonPressed(button, modifiers, this);
    }

    return false;
}

bool pawsButton::CheckKeyHandled(int keyCode)
{
    if(keybinding && keyCode == keybinding)
    {
        OnMouseUp(-1,0,0,0);
        return true;
    }
    return false;
}

bool pawsButton::OnMouseUp(int button, int modifiers, int x, int y)
{
    if(!enabled)
    {
        return false;
    }

    if(!toggle)
        SetState(false, false);

    if(button != -1)   // triggered by keyboard
    {
        // Check to make sure mouse is still in this button
        if(!Contains(x,y))
        {
            Debug1(LOG_PAWS, 0, "Not still in button so not pressed.");
            return true;
        }
    }

    if(notify != NULL)
    {
        notify->CheckButtonReleased(button, modifiers, this);
    }
    else if(parent)
    {
        return parent->CheckButtonReleased(button, modifiers, this);
    }

    return false;
}

bool pawsButton::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers)
{
    /* This would be supposed to send a mouse click to the button in case enter is used
    * but in reality as we don't have tab (or similar) focus switching for things in the gui it requires
    * the player to click on the button with the mouse first in order to use this. So in reality it gives only
    * unwanted results so for now it will stay commented out.
    * TODO: make a way to select the focused widget with the keyboard (like with tab) and check for autorepetion
    * (make it lower or disable it?)
    */

    /*if (enabled && key == CSKEY_ENTER)
    {
    OnMouseDown(csmbLeft,modifiers,screenFrame.xmin,screenFrame.ymin);
    return true;
    }*/
    return pawsWidget::OnKeyDown(keyCode, key, modifiers);
}

void pawsButton::SetNotify(pawsWidget* widget)
{
    notify = widget;
}

void pawsButton::SetEnabled(bool enabled)
{
    this->enabled = enabled;
}

bool pawsButton::IsEnabled() const
{
    return enabled;
}

void pawsButton::SetState(bool isDown, bool publish)
{
    down = isDown;

    if(flash && down)
    {
        flash = 0;
        if(flashtype == FLASH_HIGHLIGHT)
            SetColour(originalFontColour);
    }

    if(!toggle)
        return;

    if(notify)
        notify->RunScriptEvent(PW_SCRIPT_EVENT_VALUECHANGED);
    else
        RunScriptEvent(PW_SCRIPT_EVENT_VALUECHANGED);

    if(!publish)
        return;

    for(size_t a=0; a<publishList.GetSize(); ++a)
        PawsManager::GetSingleton().Publish(publishList[a], isDown);
}
