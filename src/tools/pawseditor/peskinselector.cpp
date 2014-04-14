/*
* peskinselector.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings [Borrillis] <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 6/23/2005
* Description : new widget to select skin
*
*/


#include <psconfig.h>
#include <csutil/databuf.h>
#include <iutil/cfgmgr.h>
#include <iutil/stringarray.h>

// PAWS INCLUDES
#include "peskinselector.h"
#include "paws/pawsmanager.h"
#include "paws/pawstexturemanager.h"
#include "paws/pawslistbox.h"
#include "paws/pawscombo.h"
#include "paws/pawsbutton.h"
#include "paws/pawscheckbox.h"
#include "paws/pawsimagedrawable.h"
#include "util/psxmlparser.h"
#include "pawseditorglobals.h"


peSkinSelector::~peSkinSelector()
{
    vfs->Unmount("/skin",mountedPath);
    mountedPath.Empty();
}

bool peSkinSelector::PostSetup()
{
    vfs =  csQueryRegistry<iVFS> (PawsManager::GetSingleton().GetObjectRegistry());
    config = editApp->GetConfigManager();


    skins = (pawsComboBox*)FindWidget("SkinList");
    desc = (pawsMultiLineTextBox*)FindWidget("SkinDesc");
    preview = (pawsWidget*)FindWidget("SkinPreview");
    previewBtn = (pawsButton*)FindWidget("PreviewButton");
    previewBox = (pawsCheckBox*)FindWidget("PreviewBox");

    // Fill the skins
    csString root = config->GetStr("Planeshift.GUI.Skin.Dir","/planeshift/art/skins/");
    csRef<iStringArray> files = vfs->FindFiles(root);
    for(size_t i = 0; i < files->GetSize(); i++)
    {
        csString file = files->Get(i);
        file = file.Slice(root.Length(),file.Length()-root.Length());

        csString slash(CS_PATH_SEPARATOR);
        csString ext; 
        if ( file.Slice( file.Length() - 1, 1 ) == "/" ) // Trailing Slash == directory
        {
            if ( file.Slice(file.Length()-4,3) == "zip" )
                skins->NewOption(file.Slice(0,file.Length()-5));
        }
        else //no Trailing slash
        {
            if ( file.Slice(file.Length()-3,3) == "zip" )
                skins->NewOption(file.Slice(0,file.Length()-4));
        }
    }

    // Load the current skin
    csString skin = config->GetStr("Planeshift.GUI.Skin","default");
    if( !strcmp( skin, "" ) )
        return true; // Stop here, but it's ok

    PreviewSkin(skin);

    return true;       
}

void peSkinSelector::OnListAction( pawsListBox* widget, int status )
{
    PreviewSkin( skins->GetSelectedRowString().GetData() );
}

void peSkinSelector::PreviewSkin(const char* name)
{
    csString zip;
    zip += config->GetStr("Planeshift.GUI.Skin.Dir","/planeshift/art/skins/");;
    zip += name;
    zip += ".zip";

    csString slash(CS_PATH_SEPARATOR);
    if ( vfs->Exists(zip + slash) )
        zip += slash;

    //const char *zip = "/this/art/skins/cvs.zip";
    if( !vfs->Exists( zip ) )
    {
        printf("Current skin doesn't exist, skipping..\n");
        return;
    }

    // Make sure the skin is selected
    if(skins->GetSelectedRowString() != name)
    {
        skins->Select(name);
        return; // To prevent recursion
    }

    // Get the path
    csRef<iDataBuffer> real = vfs->GetRealPath(zip);
    
    // Mount the skin
    vfs->Unmount("/skin",mountedPath);
    vfs->Mount("/skin",real->GetData());
    mountedPath = real->GetData();

    // Parse XML
    csRef<iDocument> xml = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(),"/skin/skin.xml");
    if(!xml)
    {
        Error1("Parse error in /skin/skin.xml");
        return;
    }
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error1("No XML root in /skin/skin.xml");
        return;
    }
    root = root->GetNode("xml");
    if(!root)
    {
        Error1("No <xml> tag in /skin/skin.xml");
        return;
    }
    csRef<iDocumentNode> skinInfo = root->GetNode("skin_information");
    if(!skinInfo)
    {
        Error1("No <skin_information> in /skin/skin.xml");
        return;
    }

    // Read XML
    csRef<iDocumentNode> nameNode           = skinInfo->GetNode("name");
    
    csRef<iDocumentNode> authorNode         = skinInfo->GetNode("author");
    
    csRef<iDocumentNode> descriptionNode    = skinInfo->GetNode("description");
    
    csRef<iDocumentNode> mountNode          = root->GetNode("mount_path");

    bool success = true;

    if(!mountNode)
    {
        printf("skin.xml is missing mount_path!\n");
        success = false;
    }

    if(!authorNode)
    {
        printf("skin.xml is missing author!\n");
        success = false;
    }

    if(!descriptionNode)
    {
        printf("skin.xml is missing description!\n");
        success = false;
    }

    if(!nameNode)
    {
        printf("skin.xml is missing name!\n");
        success = false;
    }

    if(!success)
        return;


    // Move data to variables
    csString skinname,author,description;
    skinname = nameNode->GetContentsValue();
    author = authorNode->GetContentsValue();
    description = descriptionNode->GetContentsValue();

    currentSkin = name;

    // Print the info
    csString info;
    info.Format("%s\nAuthor: %s\n%s",skinname.GetData(),author.GetData(),description.GetData());
    desc->SetText(info);

    // Reset the backgrounds
    preview->RemoveTitle();
    preview->SetBackground("Blue Background");
    previewBtn->SetBackground("Blue Background");
    previewBox->SetImages("radiooff","radioon");

    // Load the skin
    success = success && PreviewResource( "Examine Background", "skintest_bg", mountNode->GetContentsValue() );
    success = success && PreviewResource( "Standard Button", "skintest_btn", mountNode->GetContentsValue() );
    success = success && PreviewResource( "Blue Title", "skintest_title", mountNode->GetContentsValue() );
    success = success && PreviewResource( "radiooff", "skintest_roff", mountNode->GetContentsValue() );
    success = success && PreviewResource( "radioon", "skintest_ron", mountNode->GetContentsValue() );
    success = success && PreviewResource( "quit", "quit",mountNode->GetContentsValue() );

    if( !success )
    {
        PawsManager::GetSingleton().CreateWarningBox("Couldn't load skin! Check the console for detailed output");
        return;
    }

    preview->SetTitle("Skin preview","skintest_title","center","true");
    preview->SetMaxAlpha(1);
    preview->SetBackground("skintest_bg");

    previewBtn->SetMaxAlpha(1);
    previewBtn->SetBackground("skintest_btn");

    previewBox->SetImages("skintest_roff","skintest_ron");
}

bool peSkinSelector::PreviewResource( const char* resource, const char* resname, const char* mountPath)
{
    // Remove the resource if it exists
    PawsManager::GetSingleton().GetTextureManager()->Remove(resname);

    // Open the image list
    csRef<iDocument> xml = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(),"/skin/imagelist.xml");
    if(!xml)
    {
        Error1("Parse error in /skin/imagelist.xml");
        return false;
    }
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error1("No XML root in /skin/imagelist.xml");
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode("image_list");
    if(!topNode)
    {
        Error1("No <image_list> in /skin/imagelist.xml");
        return false;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

   // Find the resource
    csString filename;
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        if ( strcmp( node->GetValue(), "image" ) == 0 )
        {
            if(!strcmp(node->GetAttributeValue( "resource" ),resource))
            {
                filename = node->GetAttributeValue( "file" );
                break;
            }
        }
    }

    if(!filename.Length())
    {
        printf("Couldn't locate resource %s in the current skin!\n",resource);
        return false;
    }


    // Skin uses /paws/skin which we can't use since the app is already using that
    // So we need to replace that with /skin which we can and do use
    filename.DeleteAt(0,strlen(mountPath));
    filename.Insert(0,"/skin/");

    if(!vfs->Exists(filename))
    {
        printf("Skin is missing the '%s' resource at file '%s'!\n",resource,filename.GetData());
        return false;
    }

    csRef<iPawsImage> img;
    img.AttachNew(new pawsImageDrawable(filename.GetData(), resname, false, csRect(), 0, 0, 0, 0));
    PawsManager::GetSingleton().GetTextureManager()->AddPawsImage(img);

    return true;
}

bool peSkinSelector::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if(!widget->GetName())
        return false;

    if(!strcmp(widget->GetName(),"Close"))
    {
        this->Hide();
        return true;
    }

    if(!strcmp(widget->GetName(),"Apply"))
    {
        editApp->LoadSkin( currentSkin );
        config->SetStr( "Planeshift.GUI.Skin", currentSkin );
        config->Save();
        editApp->ReloadWidget();
        this->Hide();
        return true;
    }

    return false;
}

