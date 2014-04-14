/*
 * pawsactivemagicwindow.cpp
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

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/strutil.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsactivemagicwindow.h"
#include "pawsconfigactivemagic.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigpopup.h"

#define BUFF_CATEGORY_PREFIX    "+"
#define DEBUFF_CATEGORY_PREFIX  "-"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsActiveMagicWindow::pawsActiveMagicWindow() :
    useImages(true),
    autoResize(true),
    showEffects(false),
    show(true),
    buffList(NULL),
    lastIndex(0),
    configPopup(NULL)
{
    vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());

    OnResize(); //get orientation set correctly
}

void pawsActiveMagicWindow::OnResize()
{
    if(GetScreenFrame().Width() > GetScreenFrame().Height())
    {
        Orientation = ScrollMenuOptionHORIZONTAL;
    }
    else
    {
        Orientation = ScrollMenuOptionVERTICAL;
    }
    if(buffList!=NULL)
    {
        buffList->SetOrientation(Orientation);
    }
}

bool pawsActiveMagicWindow::PostSetup()
{
    pawsWidget::PostSetup();

    buffList  = (pawsScrollMenu*)FindWidget("BuffBar");
    if(!buffList)
        return false;
    buffList->SetEditLock(ScrollMenuOptionDISABLED);
    if(autoResize)
    {
        buffList->SetLeftScroll(ScrollMenuOptionDISABLED);
        buffList->SetRightScroll(ScrollMenuOptionDISABLED);
    }
    else
    {
        buffList->SetLeftScroll(ScrollMenuOptionDYNAMIC);
        buffList->SetRightScroll(ScrollMenuOptionDYNAMIC);
    }

    showWindow              = (pawsCheckBox*)FindWidget("ShowActiveMagicWindow");
    if(!showWindow)
        return false;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ACTIVEMAGIC);

    // If no active magic, hide the window.
    if(buffList->GetSize() < 1 && showWindow->GetState() == false)
    {
        show=false;
        showWindow->Hide();
    }
    else
    {
        show=true;
        showWindow->Show();
    }

    LoadSetting();

    csString blankSpell;
    blankSpell="";
    psSpellCastMessage msg(blankSpell, psengine->GetKFactor()); //request the current Active Mgic list
    msg.SendMessage();

    return true;
}

bool pawsActiveMagicWindow::Setup(iDocumentNode* node)
{

    if(node->GetAttributeValue("name") && strcmp("ActiveMagicWindow", node->GetAttributeValue("name"))==0)
    {
        csRef<iDocumentAttributeIterator> attiter = node->GetAttributes();
        csRef<iDocumentAttribute> subnode;

        while(attiter->HasNext())
        {

            subnode = attiter->Next();
            if(strcmp("useImages", subnode->GetName())==0)
            {
                if(strcmp("false", subnode->GetValue())==0)
                {
                    useImages=false;
                }
            }
            else if(strcmp("autoResize", subnode->GetName())==0)
            {
                if(strcmp("false", subnode->GetValue())==0)
                {
                    autoResize=false;
                }
            }
            else if(strcmp("showEffects", subnode->GetName())==0)
            {
                if(strcmp("true", subnode->GetValue())==0)
                {
                    showEffects=true;
                }
            }
        }
    }

    return true;
}


void pawsActiveMagicWindow::HandleMessage(MsgEntry* me)
{
    if(!configPopup)
        configPopup = (pawsConfigPopup*)PawsManager::GetSingleton().FindWidget("ConfigPopup");

    psGUIActiveMagicMessage incoming(me);
    if( !incoming.valid )
        return;

    // Use signed comparison to handle sequence number wrap around.
    if( (int)incoming.index - (int)lastIndex < 0 )
    {
        return;
    }
    csList<csString> rowEntry;
    show = showWindow->GetState() ? false : true;
    if(!IsVisible() && psengine->loadstate == psEngine::LS_DONE && show)
        ShowBehind();

    size_t    numSpells=incoming.name.GetSize();

    buffList->Clear();

        for( size_t i=0; i<numSpells; i++ )
        {
            if(incoming.duration[i]==0 && showEffects==false)
            {
    	        continue;
            }
    	    rowEntry.PushBack(incoming.name[i]);
    
	    if( incoming.image[i] == "none" )
	    {
	        //do not show any icon or text in activemagic window - this suppresses display of things like potions or etc.
	        continue;
	    }
	    
            if(useImages)
            {
                csRef<iPawsImage> image;
                if(incoming.image[i].Length() >0)
                {
                    image = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(incoming.image[i]);
                }
                if(image)
                {
                    buffList->LoadSingle(incoming.name[i], incoming.image[i], incoming.name[i], csString(""), 0, this, false);
                }
                else
                {
                    if( incoming.type[i]==BUFF )
                    {
                        buffList->LoadSingle(incoming.name[i], csString("/planeshift/materials/crystal_ball_icon.dds"), incoming.name[i], csString(""), 0, this, false);
                    }
                    else
                    {
                        buffList->LoadSingle(incoming.name[i], csString("danger_01"), incoming.name[i], csString(""), -1, this, false);
                    }
                }
            }
            else
            {
                buffList->LoadSingle(incoming.name[i], csString(""), incoming.name[i], csString(""), 0, this, false);
            }
        }
        if(autoResize)
        {
            AutoResize();
        }
        else
        {
            buffList->Resize();
        }
}

void pawsActiveMagicWindow::AutoResize()
{
    if(!buffList)
    {
        return;
    }

    //what is the starting position ?
    int leftEdge   = GetScreenFrame().xmin,
        rightEdge  = GetScreenFrame().xmax,
        topEdge    = GetScreenFrame().ymin,
        bottomEdge = GetScreenFrame().ymax;

    int newLeftEdge   = 0,
        newRightEdge  = 0,
        newTopEdge    = 0,
        newBottomEdge = 0,
        newHeight     = 0,
        newWidth      = 0,
        tOrientation  = Orientation;


    if(Orientation == ScrollMenuOptionHORIZONTAL)
    {
        int rows = int( buffList->GetTotalButtonWidth()/(psengine->GetG2D()->GetWidth()-16) )+1; //how many rows will we need to show all the entries
	
        if( leftEdge<=0 ) //left anchored, takes precedence over right anchored.
        {
            newLeftEdge=0;
            if( rows==1 )
            {
                newRightEdge=(newLeftEdge+buffList->GetTotalButtonWidth()+16);
            }
            else //rows>1
            {
                newRightEdge=psengine->GetG2D()->GetWidth();
            }
        }
        else if( rightEdge>=psengine->GetG2D()->GetWidth() ) //right anchored
        {
            newRightEdge=psengine->GetG2D()->GetWidth();
            if( rows==1 )
            {
                newLeftEdge=rightEdge-buffList->GetTotalButtonWidth()-16;
            }
            else //rows>1
            {
                newLeftEdge=0;
            }
        }
        else //not anchored at all, expand and contract on right
        {
            if( leftEdge+buffList->GetTotalButtonWidth()+16 < psengine->GetG2D()->GetWidth() )
            {
                newLeftEdge  =leftEdge;
                newRightEdge =leftEdge+buffList->GetTotalButtonWidth()+16;
                if( newRightEdge>psengine->GetG2D()->GetWidth() )
                {
                    newRightEdge=psengine->GetG2D()->GetWidth();
                }
            }
            else //this will push it over the right edge of the screen, adjust
            {
                newRightEdge=psengine->GetG2D()->GetWidth();
                newLeftEdge=newRightEdge-buffList->GetTotalButtonWidth()-16;
                if( newLeftEdge<0 )
                {
                    newLeftEdge=0;
                }
            }
        }

        if( topEdge<=0 ) //anchored at top, takes precendence over bottom anchor
        {
            newTopEdge=0;
            newBottomEdge=(rows*buffList->GetButtonHeight())+16;
        }
        else if( bottomEdge>=psengine->GetG2D()->GetHeight() )  //anchored to bottom
        {
            newTopEdge=psengine->GetG2D()->GetHeight()-(rows*buffList->GetButtonHeight());
            newBottomEdge=bottomEdge;
        }
        else //not anchored
        {
            newTopEdge    =topEdge;
            newBottomEdge =topEdge+(rows*buffList->GetButtonHeight())+16;
        }
    }
    else
    {
        int cols  = int( (buffList->GetSize()*buffList->GetButtonHeight())/(psengine->GetG2D()->GetHeight()-16) )+1, //how many columns will we need to sho all the entries
            tSize = 0;

        tSize = buffList->GetSize()<1?1:buffList->GetSize();

        if( topEdge<=0 )  //top anchored, takes precedence over bottom anchor
        {
            newTopEdge = 0;
            if( cols==1 )
            {
                newBottomEdge = (tSize*buffList->GetButtonHeight())+16;
            }
            else 
            {
                newBottomEdge=psengine->GetG2D()->GetHeight();
            }
        }
        else if( bottomEdge>=psengine->GetG2D()->GetHeight() )
        {
            newBottomEdge=psengine->GetG2D()->GetHeight();
            if( cols==1 )
            {
                newTopEdge=newBottomEdge-(tSize*buffList->GetButtonHeight())-16;
            }
            else
            {
                newTopEdge=0;
            }
        }
        else //no anchor, expand and contract on bottom
        {
           if( topEdge+(tSize*buffList->GetButtonHeight())+16 < psengine->GetG2D()->GetHeight() )
           {
               newTopEdge=topEdge;
               newBottomEdge=topEdge+(tSize*buffList->GetButtonHeight())+16;
           }
           else
           {
               newBottomEdge=psengine->GetG2D()->GetHeight();
               newTopEdge=newBottomEdge-(tSize*buffList->GetButtonHeight())-16;
           }
        }

        if( leftEdge<=0 ) //anchored at left, takes precendence over right anchor
        {
            newLeftEdge=0;
            newRightEdge=((cols*buffList->GetWidestWidth())<buffList->GetButtonHeight()?buffList->GetButtonHeight():cols*buffList->GetWidestWidth())+16;
        }
        else if( rightEdge>=psengine->GetG2D()->GetWidth() )  //anchored to right
        {
            newLeftEdge=psengine->GetG2D()->GetWidth()-((cols*buffList->GetWidestWidth())<buffList->GetButtonHeight()?buffList->GetButtonHeight():cols*buffList->GetWidestWidth())-16;
            newRightEdge=psengine->GetG2D()->GetWidth();
        }
        else //not anchored
        {
            newLeftEdge    =leftEdge;
            newRightEdge =leftEdge+((cols*buffList->GetWidestWidth())<buffList->GetButtonHeight()?buffList->GetButtonHeight():cols*buffList->GetWidestWidth() )+16;
        }
    }

    if( newRightEdge-newLeftEdge < buffList->GetButtonWidth() )
    {
        newWidth=buffList->GetButtonWidth()+BUTTON_PADDING;
    }
    else
    {
        newWidth=newRightEdge-newLeftEdge;
    }
    if( newBottomEdge-newTopEdge < buffList->GetButtonWidth() )
    {
        newHeight = buffList->GetButtonWidth()+BUTTON_PADDING;
    }
    else
    {
        newHeight = newBottomEdge-newTopEdge;
    }
    SetRelativeFrame( newLeftEdge, newTopEdge, newWidth, newHeight ); 

    //SetRelativeFrame calls OnResize which will recalculate Orientation, but we don't want
    //autoResize to change orientation. Restore it here.
    Orientation=tOrientation; 
    buffList->SetOrientation(Orientation);

    buffList->SetRelativeFrame( 0, 0, newRightEdge-newLeftEdge-16, newBottomEdge-newTopEdge-16 ); 

}

void pawsActiveMagicWindow::Close()
{
    Hide();
}

void pawsActiveMagicWindow::Show()
{
    pawsConfigActiveMagic* configWindow;

    configWindow = (pawsConfigActiveMagic*)PawsManager::GetSingleton().FindWidget("ConfigActiveMagic");
    if( configWindow ) {
        configWindow->SetMainWindowVisible( true );
    }

    pawsControlledWindow::Show();
}

void pawsActiveMagicWindow::Hide()
{
    pawsConfigActiveMagic* configWindow;

    configWindow = (pawsConfigActiveMagic*)PawsManager::GetSingleton().FindWidget("ConfigActiveMagic");
    if( configWindow ) {
        configWindow->SetMainWindowVisible( false );
    }

    pawsControlledWindow::Hide();
}

bool pawsActiveMagicWindow::LoadSetting()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root,
                         mainNode,
                         optionNode,
                         optionNode2;
    csString fileName;
    fileName = "/planeshift/userdata/options/configactivemagic.xml";

    if(!vfs->Exists(fileName))
    {
       return true; //no saved config to load. Use default values.
    }

    doc = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(), fileName);
    if(doc == NULL)
    {
        Error2("pawsActiveMagicWindow::LoadUserPrefs Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if(root == NULL)
    {
        Error2("pawsActiveMagicWindow::LoadUserPrefs : %s has no XML root",fileName.GetData());
        return false;
    }
    mainNode = root->GetNode("activemagic");
    if(mainNode == NULL)
    {
        Error2("pawsActiveMagicWindow::LoadUserPrefs %s has no <activemagic> tag",fileName.GetData());
        return false;
    }

    optionNode = mainNode->GetNode("useImages");
    if(optionNode != NULL)
    {
        if( strcasecmp( "yes",  optionNode->GetAttributeValue("on") )==0 )
        {
            SetUseImages( true );
        }
        else
        {
            SetUseImages( false );
        }
    }

    optionNode = mainNode->GetNode("autoResize");
    if(optionNode != NULL)
    {
        if( strcasecmp( "yes",  optionNode->GetAttributeValue("on") )==0 )
        {
            SetAutoResize( true );
        }
        else
        {
            SetAutoResize( false );
        }
    }

    optionNode = mainNode->GetNode("showEffects");
    if(optionNode != NULL)
    {
        if( strcasecmp( "itemAndSpell",  optionNode->GetAttributeValue("active") )==0 )
        {
            SetShowEffects( true );
        }
        else
        {
            SetShowEffects( false );
        }
    }

    optionNode = mainNode->GetNode("showWindow");
    if(optionNode != NULL)
    {
        if( strcasecmp( "yes",  optionNode->GetAttributeValue("on") )==0 )
        {
            SetShowWindow( true );
        }
        else
        {
            SetShowWindow( false );
        }
    }

    optionNode = mainNode->GetNode("buttonHeight");
    if(optionNode != NULL)
        buffList->SetButtonHeight( optionNode->GetAttributeValueAsInt("value", true));
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve buttonHeight node");
    }

    optionNode = mainNode->GetNode("buttonWidthMode");
    if(optionNode != NULL)
    {
        if( strcasecmp( "buttonWidthAutomatic", optionNode->GetAttributeValue("active") )==0 )
        {
            buffList->SetButtonWidth( 0 );
        }
        else
        {
            optionNode = mainNode->GetNode("buttonWidth");
            if(optionNode != NULL)
                buffList->SetButtonWidth( optionNode->GetAttributeValueAsInt("value", true));
            else
            {
                Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve buttonWidth");
                return false;
            }
        }
    }
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve buttonWidthMode");
    }

    optionNode = mainNode->GetNode("leftScroll");
    if(optionNode != NULL)
    {
        if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            buffList->SetLeftScroll( ScrollMenuOptionENABLED );
        else if( strcasecmp( "buttonScrollAuto",  optionNode->GetAttributeValue("active") )==0 )
            buffList->SetLeftScroll( ScrollMenuOptionDYNAMIC );
        else if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            buffList->SetLeftScroll( ScrollMenuOptionDISABLED );
    }
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve leftScroll node");
    }

    optionNode = mainNode->GetNode("rightScroll");
    if(optionNode != NULL)
    {
        if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            buffList->SetRightScroll( ScrollMenuOptionENABLED );
        else if( strcasecmp( "buttonScrollAuto",  optionNode->GetAttributeValue("active") )==0 )
            buffList->SetRightScroll( ScrollMenuOptionDYNAMIC );
        else if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            buffList->SetRightScroll( ScrollMenuOptionDISABLED );
    }
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve rightScroll node");
    }

    optionNode = mainNode->GetNode("textSize");
    if(optionNode == NULL)
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve textSize node");
    }

    optionNode2 = mainNode->GetNode("textFont");
    if(optionNode2 == NULL)
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve textFont node");
    }

    if( optionNode != NULL && optionNode2 != NULL )
    {
        fontName = csString( optionNode2->GetAttributeValue("value") );

        csString    fontPath( "/planeshift/data/ttf/");
        fontPath += fontName.GetData();
        fontPath += ".ttf"; 
        SetFont( fontPath, optionNode->GetAttributeValueAsInt("value", true) );
        buffList->SetFont( fontPath, optionNode->GetAttributeValueAsInt("value", true) );
    }
    //else use default.

    optionNode = mainNode->GetNode("textSpacing");
    if(optionNode != NULL)
        buffList->SetButtonPaddingWidth( optionNode->GetAttributeValueAsInt("value", true));
    else
    {
        Error1("pawsActiveMagicWindow::LoadUserPrefs unable to retrieve textSpacing node");
    }


    return true;
}

void pawsActiveMagicWindow::SaveSetting()
{
}

void pawsActiveMagicWindow::SetShowEffects( bool setting ) 
{
    showEffects=setting;
}
bool pawsActiveMagicWindow::GetShowEffects()
{
    return showEffects;
}

void pawsActiveMagicWindow::SetUseImages( bool setting ) 
{
    useImages=setting;
}
bool pawsActiveMagicWindow::GetUseImages()
{
    return useImages;
}


void pawsActiveMagicWindow::SetAutoResize( bool setting ) 
{
    autoResize=setting;
}
bool pawsActiveMagicWindow::GetAutoResize()
{
    return autoResize;
}

void pawsActiveMagicWindow::SetShowWindow( bool setting ) 
{
    show=setting;
    showWindow->SetState(setting);
}
bool pawsActiveMagicWindow::GetShowWindow()
{
    return show;
}

