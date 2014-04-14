/*
* shortcutwindow.cpp - Author: Andrew Dai, Joe Lyon
*
* Copyright (C) 2003, 2014 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//=============================================================================
// Application Includes
//=============================================================================
#include "shortcutwindow.h"
#include "chatwindow.h"


//=============================================================================
// Defines
//=============================================================================
#define COMMAND_FILE            "/planeshift/userdata/options/shortcutcommands.xml"
#define DEFAULT_COMMAND_FILE    "/planeshift/data/options/shortcutcommands_def.xml"

#define BUTTON_PADDING          4
#define BUTTON_SPACING          8
#define WINDOW_PADDING          5
#define SCROLLBAR_SIZE          12

#define DONE_BUTTON             1100
#define CANCEL_BUTTON           1101
#define SETKEY_BUTTON           1102
#define CLEAR_BUTTON            1103
#define CLEAR_ICON_BUTTON       1104

#define UP_BUTTON               1105
#define DOWN_BUTTON             1106


#define SHORTCUT_BUTTON_OFFSET  2000
#define PALETTE_BUTTON_OFFSET  10000

//=============================================================================
// Classes
//=============================================================================

pawsShortcutWindow::pawsShortcutWindow() :
    textBox(NULL),
    labelBox(NULL),
    shortcutText(NULL),
    title(NULL),
    subWidget(NULL),
    iconPalette(NULL),
    iconDisplay(NULL),
    iconDisplayID(0),
    editedButton(NULL),
    MenuBar(NULL),
    UpButton(NULL),
    DownButton(NULL),
    position(0),
    buttonWidth(0),                     //added 20130726 - ticket 6087
    textSpacing(0),
    scrollSize(0),                      //added 20130726 - ticket 6087
    EditMode(0)                     // 0 = edit lock prevents drag, 1 = edit lock prevent all editing
{
    vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());

    //LoadCommandsFile();
    //psengine->GetCharControl()->LoadKeyFile();

    //cmdsource = psengine->GetCmdHandler();
    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
}


pawsShortcutWindow::~pawsShortcutWindow()
{
    SaveCommands();
}

void pawsShortcutWindow::LoadCommandsFile()
{

// if there's a new style character-specific file then load it
   csString CommandFileName,
             CharName( psengine->GetMainPlayerName() );
    size_t   spPos = CharName.FindFirst( ' ' );

    if( spPos != (size_t) -1 )
    { //there is a space in the name
        CommandFileName = CharName.Slice(0,spPos );
    }
    else
    {
        CommandFileName = CharName;
    }

    CommandFileName.Insert( 0, "/planeshift/userdata/options/shortcutcommands_" );
    CommandFileName.Append( ".xml" );
    if( vfs->Exists( CommandFileName.GetData() ))
    {
        LoadCommands(CommandFileName);
    }
    else
    {
        //if there's an old-style custom shortcuts file, load or append it
        if (vfs->Exists("/planeshift/userdata/options/shortcutcommands.xml"))
        {
            LoadCommands( "/planeshift/userdata/options/shortcutcommands.xml" );
        }
        else // if there's no customization then load the defaults
        {
            LoadCommands( "/planeshift/data/options/shortcutcommands_def.xml" );
        }
        fileName = CommandFileName; //save the new name to write to.
        SaveCommands();
    }
}

void pawsShortcutWindow::HandleMessage( MsgEntry* me )
{
}

bool pawsShortcutWindow::Setup(iDocumentNode *node)
{
    csRef<iDocumentNode> tempnode;
    csRef<iDocumentNodeIterator> nodeIter = node->GetNodes();
    while( nodeIter->HasNext() )
    {
        tempnode = nodeIter->Next();
        if( tempnode->GetAttributeValue("name") && strcasecmp( "MenuBar", tempnode->GetAttributeValue("name"))==0 )
        {
            csRef<iDocumentAttributeIterator> attiter = tempnode->GetAttributes();
            csRef<iDocumentAttribute> subnode;

            while ( attiter->HasNext() )
            {
                subnode = attiter->Next();
                if( strcasecmp( "buttonWidth", subnode->GetName() )==0 )
                {
                    if( strcasecmp( "auto", subnode->GetValue() )==0 )
                    {
                        buttonWidth=0;
                    }
                    else
                    {
                        buttonWidth=subnode->GetValueAsInt();
                    }
                }
                else if( strcasecmp( "scrollSize", subnode->GetName() )==0 )
                {
                    scrollSize=subnode->GetValueAsFloat();
                }
                else if( strcasecmp( "editMode", subnode->GetName() )==0 )
                {
                    if( strcasecmp( "all", subnode->GetValue() )==0 )
                    {
                        EditMode=1;
                    }
                }
                else if( strcasecmp( "textSpacing", subnode->GetName() )==0 )
                {
                    textSpacing=subnode->GetValueAsInt();
                }
            }
            break;
        }
    }

    return true;
}

int pawsShortcutWindow::GetMonitorState()
{
    if( main_hp == NULL && main_mana == NULL &&  phys_stamina == NULL &&  ment_stamina == NULL  )
    { //if ALL of them are undefined, then disable the config control
        return -1;
    }
    //if ANY of them exists, then show/hide should be avail
    return int(GetProperty( NULL, "visible" ));
}

void pawsShortcutWindow::StartMonitors()
{
    if( main_hp != NULL )
    {
        main_hp->Show();
    }
    if( main_mana != NULL )
    {
        main_mana->Show();
    }
    if( phys_stamina != NULL )
    {
        phys_stamina->Show();
    }
    if( ment_stamina != NULL )
    {
        ment_stamina->Show();
    }
}

void pawsShortcutWindow::StopMonitors()
{
    if( main_hp != NULL )
    {
        main_hp->Hide();
    }
    if( main_mana != NULL )
    {
        main_mana->Hide();
    }
    if( phys_stamina != NULL )
    {
        phys_stamina->Hide();
    }
    if( ment_stamina != NULL )
    {
        ment_stamina->Hide();
    }
}


bool pawsShortcutWindow::PostSetup()
{
    size_t i;
    csArray<csString>  n;

    pawsControlledWindow::PostSetup();

    LoadCommandsFile();
    psengine->GetCharControl()->LoadKeyFile();
    cmdsource = psengine->GetCmdHandler();

    main_hp      = (pawsProgressBar*)FindWidget( "My HP" );
    main_mana    = (pawsProgressBar*)FindWidget( "My Mana" );
    phys_stamina = (pawsProgressBar*)FindWidget( "My PysStamina" );
    ment_stamina = (pawsProgressBar*)FindWidget( "My MenStamina" );

    if ( main_hp )
    {
        main_hp->SetTotalValue(1);
    }
    if ( main_mana )
    {
        main_mana->SetTotalValue(1);
    }
    if ( phys_stamina )
    {
        phys_stamina->SetTotalValue(1);
    }
    if ( ment_stamina )
    {
        ment_stamina->SetTotalValue(1);
    }
    if( main_hp || main_mana || phys_stamina || ment_stamina )
    {
        psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_MODE);
    }

    MenuBar      = (pawsScrollMenu*)FindWidget( "MenuBar" );
    if( MenuBar==NULL )
    {
        psSystemMessage msg(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Missing MenuBar widget, probably due to obsolete skin"));
        return false;
    }

    UpButton       = (pawsButton*)FindWidget( "Up Arrow" );
    if( UpButton!=NULL )
    {
        MenuBar->SetLeftScroll(ScrollMenuOptionDISABLED );
    }
    DownButton     = (pawsButton*)FindWidget( "Down Arrow" );
    if( DownButton!=NULL )
    {
        MenuBar->SetRightScroll(ScrollMenuOptionDISABLED );
    }
    iconScrollBar     = (pawsScrollBar*)FindWidget( "iconScroll" );
    if( iconScrollBar!=NULL )
    {
        iconScrollBar->SetMaxValue( NUM_SHORTCUTS-1 );
        iconScrollBar->SetMinValue( 0 );
        MenuBar->SetScrollWidget( iconScrollBar );
    }

    MenuBar->SetButtonWidth( buttonWidth );
    MenuBar->SetEditMode( EditMode );
    if( scrollSize>1 )
    {
        MenuBar->SetScrollIncrement( (int)scrollSize );
    }
    else
    {
        MenuBar->SetScrollProportion( scrollSize );
    }
    
    MenuBar->SetButtonPaddingWidth( textSpacing );


    n =  names;
    for( i=0; i<names.GetSize(); i++ )
    {
        csString t = GetTriggerText( i );
        if( t.Length()>0 )
        {
            n[i].Insert( 0, " - " );
            n[i].Insert( 0, t );
        }
    }
    MenuBar->LoadArrays( names, icon, n, cmds, 2000, this);

    position = 0;

    LoadUserPrefs();


    return true;
}

void pawsShortcutWindow::OnResize()
{
}


bool pawsShortcutWindow::OnMouseDown( int button, int modifiers, int x, int y )
{
    if ( button == csmbWheelUp )
    {
        MenuBar->OnMouseDown( button, modifiers, x, y );
	return true;
    }
    else if ( button == csmbWheelDown )
    {
        MenuBar->OnMouseDown( button, modifiers, x, y );
	return true;
    }
    else
    {
	return pawsControlledWindow::OnMouseDown(button, modifiers, x, y);
    }
}


bool pawsShortcutWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    // Play a sound
    PawsManager::GetSingleton().GetSoundManager()->PlaySound("gui.shortcut", false, iSoundManager::GUI_SNDCTRL);
    return true;
}

bool pawsShortcutWindow::OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget )
{
    pawsDnDButton* dndb = dynamic_cast <pawsDnDButton*> (widget);
    if( dndb && dndb->IsDragDropInProgress() )
    {
        SaveCommands();
        ((pawsDnDButton*)widget)->SetDragDropInProgress( 0 );
        dndb->SetDragDropInProgress( 0 );
	return true;
    }
    if( psengine->GetSlotManager()->IsDragging() )
    {
        return true;
    }

    if (!subWidget)
        subWidget = PawsManager::GetSingleton().FindWidget("ShortcutEdit");
    if (!subWidget )
    {
        Error1( "pawsShortcutWindow::OnButtonReleased unable to read ShortcutEdit widget!!\n");
        return false;
    }

    if (!labelBox)
        labelBox = dynamic_cast <pawsEditTextBox*> (subWidget->FindWidget("LabelBox"));
    if (!labelBox )
    {
        Error1( "pawsShortcutWindow::OnButtonReleased unable to read labelBox widget!!\n");
        return false;
    }

    if (!textBox)
         textBox = dynamic_cast <pawsMultilineEditTextBox*> (subWidget->FindWidget("CommandBox"));
    if (!textBox )
    {
        Error1( "pawsShortcutWindow::OnButtonReleased unable to read textBox widget!!\n");
        return false;
    }

    if (!shortcutText)
         shortcutText = dynamic_cast <pawsTextBox*> (subWidget->FindWidget("ShortcutText"));
    if (!shortcutText )
    {
        Error1( "pawsShortcutWindow::OnButtonReleased unable to read shortcutText widget!!\n");
        return false;
    }

    if (!iconDisplay)
         iconDisplay = dynamic_cast <pawsDnDButton*> (subWidget->FindWidget("IconDisplay"));
    if (!iconDisplay )
    {
        Error1( "pawsShortcutWindow::OnButtonReleased unable to read iconDisplay widget!!\n");
        return false;
    }

    if (!iconPalette)
    {
        iconPalette = dynamic_cast <pawsScrollMenu*> (subWidget->FindWidget("iconPalette"));
        if (iconPalette)
        {
            //get a ptr to the texture manager so we can look at the elementList, which stores the icon names.
            pawsTextureManager *tm = PawsManager::GetSingleton().GetTextureManager();
        
            //build an array of the icon names
            csHash<csRef<iPawsImage>, csString>::GlobalIterator Iter(tm->elementList.GetIterator());
            Iter.Reset();
            //build a sorted list of all PS icons
            while(Iter.HasNext())
            {
                csString curr = csString(Iter.Next()->GetName());
//fprintf(stderr, "pawsShortcutWindow::OnButtonReleased building icon palette, item %s\n", curr.GetData() );
                size_t i;
                if( allIcons.IsEmpty() )
                {
                    allIcons.Push( curr.GetData() );
                    continue;
                }
                for( i=0; i<allIcons.GetSize(); i++) //insertion sort
                {
//fprintf(stderr, "......comparing to item %i - %s\n", i, allIcons[i].GetData() );
                    if( allIcons[i] > curr )
                    {
//fprintf(stderr, "......inserting\n" );
                       allIcons.Insert(i, curr.GetData()); 
                       break;
                    }
                }
                if( i>=allIcons.GetSize() )
                {
                    allIcons.Push( curr.GetData() );
                }
            }

            //pass the array of icon names to LoadArrays as both the icon and the tooltip, so we can see then names when we hover over one
            iconPalette->LoadArrays( stubArray, allIcons, allIcons, stubArray, PALETTE_BUTTON_OFFSET, this );
            iconPalette->SetEditLock( ScrollMenuOptionDISABLED );
            iconPalette->OnResize();
    
            iconDisplayID=-1;
        }
        else
        {
            Error1( "pawsShortcutWindow::OnButtonReleased unable to load iconPalette widget!!\n");
            return false;
        }
    }

    // These should not be NULL
    CS_ASSERT(subWidget); CS_ASSERT(labelBox); CS_ASSERT(textBox); CS_ASSERT(shortcutText); CS_ASSERT(iconDisplay);

    switch ( widget->GetID() )
    {
        case DONE_BUTTON:
        {
            if (!labelBox->GetText() || *(labelBox->GetText()) == '\0' )
            {
                if (textBox->GetText() && *(textBox->GetText()) != '\0')
                {
                    //no name but a command was specified.  
                    psSystemMessage msg(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please specify a name when creating a shortcut."));
                    msg.FireEvent();
                    return true;
                }
                else //shortcut is empty and will be removed. Also remove possible key binding.
                {
                    //remove key bindings
                    csString editedCmd;
                    editedCmd.Format("Shortcut %zd",edit+1);
                    psengine->GetCharControl()->RemapTrigger(editedCmd,psControl::NONE,0,0);

                    ((pawsDnDButton *)editedButton)->Clear();
                }
            }
            else  //labelBox (ie name) is set
            {
                if( edit < names.GetSize() )
                {
                    names[edit] = csString(labelBox->GetText());
                    cmds[edit] = csString(textBox->GetText());
                    ((pawsDnDButton *)editedButton)->SetAction( textBox->GetText() );
                    if( iconDisplay->GetMaskingImage()!=NULL )
                    {
                        icon[edit] = csString( iconDisplay->GetMaskingImage()->GetName());
                    }
                    else
                    {
                        icon[edit].Clear();
                    }
                }
    	
                editedButton->SetMaskingImage( icon[edit] );
                if( iconDisplay->GetMaskingImage()!=NULL ) //there is already a masking image
                {
                    ((pawsDnDButton *)editedButton)->SetText( csString("") );
                }
                else  //there's no masking image so far
                {
                    ((pawsDnDButton *)editedButton)->SetText( names[edit] );
                }
                iconDisplayID = -1;
                csString t = GetTriggerText( edit );
                if( t.Length()>0 && edit < names.GetSize() )
                {
                    csString n = names[ edit ];
                    n.Insert( 0, " - " );
                    n.Insert( 0, t );
                    editedButton->SetToolTip( n );
                }
                else
                {
                    editedButton->SetToolTip( names[edit] );
                }

            } 
            SaveCommands();

            PawsManager::GetSingleton().SetModalWidget(NULL);
            PawsManager::GetSingleton().SetCurrentFocusedWidget(this);
            subWidget->Hide();

            Resize();
            ResetEditWindow();

            return true;
        }
        case CLEAR_BUTTON:
        {
            ResetEditWindow();

            return true;
        }
        case CLEAR_ICON_BUTTON:
        {
            iconDisplay->ClearMaskingImage();
            return true;
        }
        case CANCEL_BUTTON:
        {
            subWidget->Hide();
            ResetEditWindow();

            return true;
        }
        case SETKEY_BUTTON:
        {
            pawsWidget * fingWndAsWidget;
        
            fingWndAsWidget = PawsManager::GetSingleton().FindWidget("FingeringWindow");
            if (fingWndAsWidget == NULL)
            {
                Error1("Could not find widget FingeringWindow");
                return false;
            }
            pawsFingeringWindow * fingWnd = dynamic_cast<pawsFingeringWindow *>(fingWndAsWidget);
            if (fingWnd == NULL)
            {
                Error1("FingeringWindow is not pawsFingeringWindow");
                return false;
            }
            fingWnd->ShowDialog(this, labelBox->GetText());
    
            return true;
        }
        case UP_BUTTON:
        {
            if( UpButton!=NULL )
            {
                MenuBar->ScrollUp();
            }
            return true;
        }
        case DOWN_BUTTON:
        {
            if( DownButton!=NULL )
            {
                MenuBar->ScrollDown();
            }
            return true;
        }
    }            // switch( ... )
    if ( mouseButton == csmbLeft && !(keyModifier & CSMASK_CTRL))    //if left mouse button clicked
    {
        if( widget->GetID()>=PALETTE_BUTTON_OFFSET )        //if the clicked widget's offset is within the PALETTE range
        {
            iconDisplayID = widget->GetID();
            iconDisplay->SetMaskingImage(allIcons[iconDisplayID-PALETTE_BUTTON_OFFSET]);
        }
        else
        {
            if( cmds.GetSize()>0 && !MenuBar->IsEditable() )
            {
                if( (cmds[widget->GetID() - SHORTCUT_BUTTON_OFFSET + position ]) )
                {
                    ExecuteCommand( widget->GetID() - SHORTCUT_BUTTON_OFFSET + position );
                }
            }
        }
    }
    else if ( mouseButton == csmbRight || (mouseButton == csmbLeft && (keyModifier & CSMASK_CTRL)) )
    {
        if( widget->GetID() >= PALETTE_BUTTON_OFFSET ) //ignore right-click on icon palette
        {
            return true;
        }

        if( !(MenuBar->IsEditable()) && MenuBar->GetEditMode()==1 )
        {
            return false;
        }
        
        edit = widget->GetID() - SHORTCUT_BUTTON_OFFSET;
        editedButton = widget;

        if ( edit < 0 )
            return false;

        if (!subWidget || !labelBox || !textBox || !shortcutText)
            return false;

        if ( names[edit] && names[edit].Length() )
            labelBox->SetText( names[edit].GetData() );
        else
            labelBox->Clear();

        if ( cmds[edit] && cmds[edit].Length() )
        {
            textBox->SetText( cmds[edit].GetData() );
            shortcutText->SetText( GetTriggerText(edit) );
        }
        else
        {
            textBox->Clear();
            shortcutText->SetText("");
        }

        if( icon[edit] )
        {
            iconDisplay->SetMaskingImage(icon[edit]);
            iconDisplayID=edit+PALETTE_BUTTON_OFFSET;
        }
        else
        {
            iconDisplay->ClearMaskingImage();
        }

        subWidget->Show();
        PawsManager::GetSingleton().SetCurrentFocusedWidget(textBox);
    }
    else
    {
        return false;
    }
    return true;
}

bool pawsShortcutWindow::OnScroll(int direction, pawsScrollBar* widget)
{
    if( widget )
    {
        float pos = widget->GetCurrentValue();
        if( pos>NUM_SHORTCUTS-1 )
        {
            pos=NUM_SHORTCUTS-1;
            widget->SetCurrentValue( NUM_SHORTCUTS-1 );
        }
        else if( pos<0 )
        {
            pos=0.0;
            widget->SetCurrentValue( 0.0 );
        }
        MenuBar->ScrollToPosition( pos );
    }
 return true;
}

void pawsShortcutWindow::ResetEditWindow()
{
    iconDisplayID = -1;
    shortcutText->SetText( "" );
    iconDisplay->ClearMaskingImage();
    labelBox->Clear();
    textBox->Clear();
}


csString pawsShortcutWindow::GetTriggerText(int shortcutNum)
{
    psCharController* manager = psengine->GetCharControl();
    csString str;
    str.Format("Shortcut %d",shortcutNum+1);

    const psControl* ctrl = manager->GetTrigger( str );
    if (!ctrl)
    {
        printf("Unimplemented action '%s'!\n",str.GetData());
        return "";
    }

    return ctrl->ToString();
}


void pawsShortcutWindow::LoadDefaultCommands()
{
    LoadCommands(DEFAULT_COMMAND_FILE);
}


void pawsShortcutWindow::LoadCommands(const char * FN)
{
    int number;
    // Read button commands
    csRef<iDocument> doc = psengine->GetXMLParser()->CreateDocument();

    csRef<iDataBuffer> buf (vfs->ReadFile (FN));
    if (!buf || !buf->GetSize ())
    {
        return ;
    }
    const char* error = doc->Parse( buf );
    if ( error )
    {
        Error2("Error loading shortcut window commands: %s\n", error);
        return ;
    }

    fileName=FN;

    icon.Empty();
    names.Empty();
    cmds.Empty();
    if( MenuBar )
        MenuBar->Clear();

    bool zerobased = false;

    csRef<iDocumentNodeIterator> iter = doc->GetRoot()->GetNode("shortcuts")->GetNodes();
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> child = iter->Next();

        if ( child->GetType() != CS_NODE_ELEMENT )
            continue;
        sscanf(child->GetValue(), "shortcut%d", &number);
        if(number == 0)
        {
            zerobased = true;
        }
        if(!zerobased)
        {
            number--;
        }
        //if (number < 0 || number >= NUM_SHORTCUTS)
        if (number < 0 )
            continue;
        icon.Put(number, child->GetAttributeValue("buttonimage") );
        names.Put(number, child->GetAttributeValue("name") );
        cmds.Put(number, child->GetContentsValue() );
    }
    //set minimum sizes of command arrays
    if( names.GetSize()<NUM_SHORTCUTS )
    {
        names.SetSize( NUM_SHORTCUTS, csString( "") );
    }
    if( icon.GetSize()<NUM_SHORTCUTS )
    {
        icon.SetSize( NUM_SHORTCUTS, csString("") );
    }
    if( cmds.GetSize()<NUM_SHORTCUTS )
    {
        cmds.SetSize( NUM_SHORTCUTS, csString("") );
    }

    csArray<csString>  n;
    n =  names;
    for( size_t i=0; i<names.GetSize(); i++ )
    {
        csString t = GetTriggerText( i );
        if( t.Length()>0 )
        {
            n[i].Insert( 0, " - " );
            n[i].Insert( 0, t );
        }
    }
    if( MenuBar )
    {
        MenuBar->LoadArrays( names, icon, n, cmds, 2000, this);
        MenuBar->Resize();
    }
}

void pawsShortcutWindow::SaveCommands(const char *FN)
{
    fileName = FN;
    SaveCommands();
}

void pawsShortcutWindow::SaveCommands()
{
    bool found = false;
    size_t i;
    for (i = 0;i < cmds.GetSize();i++)
    {
        if (cmds[i].IsEmpty())
            continue;

        found = true;
        break;
    }
    if (!found) // Don't save if no commands have been defined
        return ;

    // Save the commands with their labels
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot ();
    csRef<iDocumentNode> parentMain = root->CreateNodeBefore(CS_NODE_ELEMENT);
    parentMain->SetValue("shortcuts");
    csRef<iDocumentNode> parent;

    csRef<iDocumentNode> text;
    csString temp;
    for (i = 0;i < cmds.GetSize();i++)
    {
        if (cmds[i].IsEmpty())
            continue;
        parent = parentMain->CreateNodeBefore (CS_NODE_ELEMENT);
        temp.Format("shortcut%zd", i + 1);
        parent->SetValue(temp);

        if (names[i].IsEmpty())
        {
            temp.Format("%zd", i);
            parent->SetAttribute("name", temp);
        }
        else
        {
            parent->SetAttribute("name", names[i].GetData());
        }

        if (!icon[i].IsEmpty() )
        {
            parent->SetAttribute("buttonimage", icon[i].GetData());
        }
        text = parent->CreateNodeBefore(CS_NODE_TEXT);
        text->SetValue(cmds[i].GetData());
    }
    doc->Write(vfs, fileName.GetData() );
}

void pawsShortcutWindow::ExecuteCommand(int shortcutNum )
{

    //if (shortcutNum < 0 || shortcutNum >= NUM_SHORTCUTS)
    if (shortcutNum < 0 )
    {
        return;
    }
    
    const char* current = cmds[shortcutNum].GetData();
    if (current)
    {
        const char* pos;
        const char* next = current - 1;
        csString command;
        
        while (next && *(next + 1))
        {
            // Move command pointer to start of next command
            pos = next + 1;
            command = pos;
            // Execute next command delimited by a line feed
            if (*pos)
            {
                // Find location of next command
                next = strchr(pos, '\n');
                if (next)
                    command.Truncate(next - pos);
            }

            if(command.GetAt(0) == '#') //it's a comment skip it
            {
                continue;
            }
 
            if(command.FindFirst("$target") != command.Length() - 1)
            {
                GEMClientObject *object= psengine->GetCharManager()->GetTarget();
                if(object)
                {
                    csString name = object->GetName(); //grab name of target
                    size_t space = name.FindFirst(" ");
                    name = name.Slice(0,space);
                    command.ReplaceAll("$target",name); // actually replace target
                }
            }
            if(command.FindFirst("$guild") != command.Length() - 1)
            {
                GEMClientActor *object= dynamic_cast<GEMClientActor*>(psengine->GetCharManager()->GetTarget());
                if(object)
                {
                    csString name = object->GetGuildName(); //grab guild name of target
                    command.ReplaceAll("$guild",name); // actually replace target
                }
            }
            if(command.FindFirst("$race") != command.Length() - 1)
            {
                GEMClientActor *object= dynamic_cast<GEMClientActor*>(psengine->GetCharManager()->GetTarget());
                if(object)
                {
                    csString name = object->race; //grab race name of target
                    command.ReplaceAll("$race",name); // actually replace target
                }
            }
            if(command.FindFirst("$sir") != command.Length() - 1)
            {
                GEMClientActor *object= dynamic_cast<GEMClientActor*>(psengine->GetCharManager()->GetTarget());
                if(object)
                {
                    csString name = "Dear";
                    switch (object->gender)
                    {
                    case PSCHARACTER_GENDER_NONE:
                        name = "Gemma";
                        break;
                    case PSCHARACTER_GENDER_FEMALE:
                        name = "Lady";
                        break;
                    case PSCHARACTER_GENDER_MALE:
                        name = "Sir";
                        break;
                    }
                    command.ReplaceAll("$sir",name); // actually replace target
                }
            }
            const char* errorMessage = cmdsource->Publish( command );
            if ( errorMessage )
                chatWindow->ChatOutput( errorMessage );
        }
    }
}

const csString& pawsShortcutWindow::GetCommandName(int shortcutNum )
{
    if (shortcutNum < 0 || shortcutNum >= NUM_SHORTCUTS)
    //if (shortcutNum < 0 )
    {
        static csString error("Out of range");
        return error;
    }

    if (!names[shortcutNum])
    {
        static csString unused("Unused");
        return unused;
    }
    
    return names[shortcutNum];
}

bool pawsShortcutWindow::OnFingering(csString string, psControl::Device device, uint button, uint32 mods)
{
    pawsFingeringWindow* fingWnd = dynamic_cast<pawsFingeringWindow*>(PawsManager::GetSingleton().FindWidget("FingeringWindow"));
    if (fingWnd == NULL || !fingWnd->IsVisible())
        return true;

    csString editedCmd;
    editedCmd.Format("Shortcut %zd",edit+1);

    bool changed = false;

    if (string == NO_BIND)  // Removing trigger
    {
        psengine->GetCharControl()->RemapTrigger(editedCmd,psControl::NONE,0,0);
        changed = true;
    }
    else  // Changing trigger
    {
        changed = psengine->GetCharControl()->RemapTrigger(editedCmd,device,button,mods);
    }

    if (changed)
    {
        shortcutText->SetText(GetTriggerText(edit));
        return true;  // Hide fingering window
    }
    else  // Map already exists
    {
        const psControl* other = psengine->GetCharControl()->GetMappedTrigger(device,button,mods);
        CS_ASSERT(other);

        csString name = GetDisplayName(other->name);
        if (name.IsEmpty())     //then not cleaned up properly from deleting a shortcut
        {
            name = other->name; //use its numeric name
        }
        else
        {
            name.AppendFmt(" (%s)", other->name.GetDataSafe());
        }

        fingWnd->SetCollisionInfo(name);

        return false;
    }
}

void pawsShortcutWindow::Show()
{
    psStatDRMessage msg;
    msg.SendMessage();
    pawsControlledWindow::Show();
}

bool pawsShortcutWindow::LoadUserPrefs()
{
    csRef<iDocument>     doc;
    csRef<iDocumentNode> root,
                         mainNode,
                         optionNode,
                         optionNode2;

    csString fileName;
    fileName = "/planeshift/userdata/options/configshortcut.xml";

    if(!vfs->Exists(fileName))
    {
       return true; //no saved config to load.
    }

    doc = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(), fileName);
    if(doc == NULL)
    {
        Error2("pawsShortcutWindow::LoadUserPrefs Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if(root == NULL)
    {
        Error2("pawsShortcutWindow::LoadUserPrefs : %s has no XML root",fileName.GetData());
        return false;
    }
    mainNode = root->GetNode("shortcut");
    if(mainNode == NULL)
    {
        Error2("pawsShortcutWindow::LoadUserPrefs %s has no <shortcut> tag",fileName.GetData());
        return false;
    }

    optionNode = mainNode->GetNode("buttonHeight");
    if(optionNode != NULL)
        MenuBar->SetButtonHeight( optionNode->GetAttributeValueAsInt("value", true));
    else
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve buttonHeight node");
    }

    optionNode = mainNode->GetNode("buttonWidthMode");
    if(optionNode != NULL)
    {
        if( strcasecmp( "buttonWidthAutomatic", optionNode->GetAttributeValue("active") )==0 )
        {
            MenuBar->SetButtonWidth( 0 );
        }
        else
        {
            optionNode = mainNode->GetNode("buttonWidth");
            if(optionNode != NULL)
                MenuBar->SetButtonWidth( optionNode->GetAttributeValueAsInt("value", true));
            else
            {
                Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve buttonWidth");
                return false;
            }
        }
    }
    else
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve buttonWidthMode");
    }

    optionNode = mainNode->GetNode("leftScroll");
    if(optionNode != NULL)
    {
        if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            MenuBar->SetLeftScroll( ScrollMenuOptionENABLED );
        else if( strcasecmp( "buttonScrollAuto",  optionNode->GetAttributeValue("active") )==0 )
            MenuBar->SetLeftScroll( ScrollMenuOptionDYNAMIC );
        else if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            MenuBar->SetLeftScroll( ScrollMenuOptionDISABLED );
    }
    else
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve leftScroll node");
    }

    optionNode = mainNode->GetNode("rightScroll");
    if(optionNode != NULL)
    {
        if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            MenuBar->SetRightScroll( ScrollMenuOptionENABLED );
        else if( strcasecmp( "buttonScrollAuto",  optionNode->GetAttributeValue("active") )==0 )
            MenuBar->SetRightScroll( ScrollMenuOptionDYNAMIC );
        else if( strcasecmp( "buttonScrollOn",  optionNode->GetAttributeValue("active") )==0 )
            MenuBar->SetRightScroll( ScrollMenuOptionDISABLED );
    }
    else
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve rightScroll node");
    }

    optionNode = mainNode->GetNode("editLockMode");
    if(optionNode != NULL)
    {
        if( strcasecmp( "editLockDND",  optionNode->GetAttributeValue("active") )==0 )
        {
            MenuBar->SetEditMode( 0 ); //allow rt-click editing but prevent dnd
        }
        else //default to lock all
        {
            MenuBar->SetEditMode( 1 );//prevent all editing
        }
    }


    optionNode = mainNode->GetNode("healthAndMana");
    if(optionNode != NULL)
    {
        if( strcasecmp( "no",  optionNode->GetAttributeValue("on") )==0 )
            StopMonitors();
        else
            StartMonitors();
    }
    else
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve healthAndMana node");
    }

    optionNode = mainNode->GetNode("buttonBackground");
    if(optionNode != NULL)
    {
        if( strcasecmp( "no", optionNode->GetAttributeValue("on") )==0 )
        {
            MenuBar->EnableButtonBackground(false);
        }
        else  //default to button background showing
        {
            MenuBar->EnableButtonBackground(true);
        }
    }
    else
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve buttonBackground node");
    }

    optionNode = mainNode->GetNode("textSize");
    if(optionNode == NULL)
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve textSize node");
    }

    optionNode2 = mainNode->GetNode("textFont");
    if(optionNode2 == NULL)
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve textFont node");
    }

    if( optionNode != NULL && optionNode2 != NULL )
    {
        csString    fontPath( "/planeshift/data/ttf/");
        fontPath += optionNode2->GetAttributeValue("value");
        fontPath += ".ttf";
        SetFont( fontPath, optionNode->GetAttributeValueAsInt("value", true) );
        MenuBar->SetFont( fontPath, optionNode->GetAttributeValueAsInt("value", true) );
    }
    //else use default.


    optionNode = mainNode->GetNode("textSpacing");
    if(optionNode != NULL)
        MenuBar->SetButtonPaddingWidth( optionNode->GetAttributeValueAsInt("value", true));
    else
    {
        Error1("pawsShortcutWindow::LoadUserPrefs unable to retrieve textSpacing node");
    }

    return true;
}
