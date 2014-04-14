/*
* Author: Andrew Mann, Marc Prüßmeier
*
* Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

/*
 PAWS File Navigation widget

 This widget provides file navigation controls similar to the file->open dialog on many applications.

 The widget uses an iVFS interface to navigate through the file system.

 |-----------------------------------------------------|
 |[..] [New Directory]     | [New File]                |
 |-----------------------------------------------------|
 | DIRECTORY            |/\| file1.bmp              |/\|
 | zipfile.zip.dir      |  | file2.gif              |  |
 |                      |  | zipfile.zip            |  |
 |                      |  |                        |  |
 |                      |  |                        |  |
 |                      |  |                        |  |
 |                      |\/|                        |\/|
 |-----------------------------------------------------|
 |Filename:                                     Action |
 |                                              Cancel |
 |-----------------------------------------------------|


 |-----------------------------------------------------|
 |(Button)   (Button)      | (Button)                  |
 |-----------------------------------------------------|
 | (Listbox with        |/\| (Listbox with          |/\|
 | scrollbar)           |  | scrollbar)             |  |
 |                      |  |                        |  |
 |                      |  |                        |  |
 |                      |  |                        |  |
 |                      |  |                        |  |
 |                      |\/|                        |\/|
 |-----------------------------------------------------|
 |Filename: (texteditbox)                      (Button)|
 |                                             (Button)|
 |-----------------------------------------------------|



*/


#include <psconfig.h>
#include <csutil/syspath.h>
#include <iutil/databuff.h>
#include "pawsfilenavigation.h"
#include "iutil/string.h"

#include "paws/pawsmanager.h"
#include "pawsmainwidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawscombo.h"


pawsFileNavigation::pawsFileNavigation()
{
    iObjectRegistry* object_reg = PawsManager::GetSingleton().GetObjectRegistry();
    if(object_reg)
        vfs =  csQueryRegistry<iVFS> (object_reg);

    current_path="/";
    zip_mounts.DeleteAll();
    filelistbox=NULL;
    dirlistbox=NULL;
    fullpathandfilename=NULL;
    action=NULL;
    factory = "pawsFileNavigation";
}

pawsFileNavigation::pawsFileNavigation(const pawsFileNavigation &origin)
    :pawsWidget(origin),
     vfs(origin.vfs),
     current_path(origin.current_path),
     fullpathandfilename(0),
     selection_state(origin.selection_state),
     action(origin.action)
{
    for(unsigned int i = 0 ; i < origin.zip_mounts.GetSize(); i++)
        zip_mounts.Push(origin.zip_mounts[i]);

    dirlistbox = 0;
    filelistbox = 0;

    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.children[i] == origin.dirlistbox)
            dirlistbox = dynamic_cast<pawsListBox*>(children[i]);
        else if(origin.children[i] == origin.filelistbox)
            filelistbox = dynamic_cast<pawsListBox*>(children[i]);

        if(filelistbox != 0 && dirlistbox != 0) break;
    }
}

pawsFileNavigation::~pawsFileNavigation()
{
    delete[] fullpathandfilename;
}

bool pawsFileNavigation::PostSetup()
{
    pawsListBox* dirlistbox=(pawsListBox*)FindWidget("dirlist");
    if(dirlistbox)
    {
        dirlistbox->SetSortingFunc(0,textBoxSortFunc);
        dirlistbox->SetSortedColumn(0);
    }

    pawsListBox* filelistbox=(pawsListBox*)FindWidget("filelist");
    if(filelistbox)
    {
        filelistbox->SetSortingFunc(0,textBoxSortFunc);
        filelistbox->SetSortedColumn(0);
    }

    pawsComboBox* filetypetb=(pawsComboBox*)FindWidget("filetype");
    if(filetypetb)
    {
        filetypetb->SetFilename("test");
    }
    SetAlwaysOnTop(true);
    return true;
}

bool pawsFileNavigation::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(!widget)
        return false;

    if(widget == FindWidget("uppath"))
    {
        if(UpOnePath())
            FillFileList();
    }
    if(widget == FindWidget("windowclose") || widget == FindWidget("cancel"))
    {
        if(action)
        {
            delete action;
            action = NULL;
        }
        parent->DeleteChild(this);       // destructs itself
        return true;
    }

    if(widget == FindWidget("perform"))
    {
        if(action)
        {
            action->Execute(GetFullPathFilename());
            delete action;
            action = NULL;
        }
        parent->DeleteChild(this);       // destructs itself
        return true;
    }
    return true;
}

bool pawsFileNavigation::OnChange(pawsWidget* /*widget*/)
{
    return true;
}

bool pawsFileNavigation::FillFileList()
{
    csRef<iStringArray> filelist;
    csString currpathname = GetCurrentPath();
    csString filetype;
    size_t i;

    pawsComboBox* filetypetb=(pawsComboBox*)FindWidget("filetype");
    if(filetypetb)
        filetype=filetypetb->GetSelectedRowString();

    if(!dirlistbox)
        dirlistbox=(pawsListBox*)FindWidget("dirlist");
    if(!dirlistbox)
        return false;

    if(!filelistbox)
        filelistbox=(pawsListBox*)FindWidget("filelist");
    if(!filelistbox)
        return false;

    if(!vfs)
        return false;

    csString ZipTestPath = currpathname;
    while(ZipTestPath.Length()>1)
    {
        csString ZipTestValue = ZipTestPath;
        csString ZipFileType;
        ZipTestPath.Truncate(ZipTestPath.FindLast('/'));
        ZipTestPath.SubString(ZipTestValue,ZipTestPath.FindLast('/')+1,ZipTestPath.Length()-ZipTestPath.FindLast('/'));
        ZipTestValue.SubString(ZipFileType,ZipTestValue.FindLast('.'),ZipTestValue.Length()-ZipTestValue.FindLast('/'));
        if(ZipFileType.Downcase()==".zip")
        {
            vfs->Unmount(ZipTestPath+"/",NULL);
            csRef<iDataBuffer> xrpath = vfs->GetRealPath(ZipTestPath);
            char* xpath=xrpath->GetData();
            vfs->Mount(ZipTestPath+"/",xpath);
        }
        ZipTestPath.Truncate(ZipTestPath.FindLast('/')+1);
    }

    size_t j=zip_mounts.GetSize();
    for(i=0; i<j; i++)
    {
        csString checkpath = zip_mounts.Get(i);
        csString zipfile = checkpath;
        checkpath.Truncate(checkpath.FindLast('/')+1);
        if(!currpathname.StartsWith(checkpath,false))
        {
            vfs->Unmount(zipfile,NULL);
            zip_mounts.DeleteIndex(i);
            i--;
            j--;
        }
    }

    // Fill in the path text
    pawsComboBox* pathcombobox=(pawsComboBox*)FindWidget("pathtext");
    if(pathcombobox)
    {
        if(pathcombobox->GetRowCount()>10)
        {
        }

        if(current_path.Length() > 60)
        {
            csString pathtext;
            pathtext="...";
            pathtext.Append(current_path.GetData() + current_path.Length()-27);
            pathcombobox->NewOption(pathtext);
        }
        else
            pathcombobox->NewOption(current_path);
    }


    dirlistbox->Clear();
    filelistbox->Clear();

    csString current_path_filter = current_path;

    for(int z=0; z<2; z++)
    {
        if(z) current_path_filter += "/" + filetype;

        filelist=vfs->FindFiles(current_path_filter);

        if(!z && current_path!="/" && current_path!="\\" && current_path.Length()>0)
        {
            if(strcmp(current_path.GetData(),"\\")>0)
            {
                pawsListBoxRow* lbrow=dirlistbox->NewRow();
                if(lbrow)
                {
                    pawsTextBox* filetb=(pawsTextBox*)lbrow->GetColumn(0);
                    if(filetb)
                        filetb->SetText("\\..");
                }
            }
            if(strcmp(current_path.GetData(),"/")>0)
            {
                pawsListBoxRow* lbrow=dirlistbox->NewRow();
                if(lbrow)
                {
                    pawsTextBox* filetb=(pawsTextBox*)lbrow->GetColumn(0);
                    if(filetb)
                        filetb->SetText("/..");
                }
            }
        }


        for(size_t i=0; i<filelist->GetSize(); i++)
        {
            const char* filename=filelist->Get(i);
            if(filename)
            {
                // Remove leading path from the filename, as VFS will add
                if(strlen(filename) >= current_path.Length())
                {
                    if(!strncmp(filename,current_path.GetData(),current_path.Length()))
                    {
                        filename+=current_path.Length();
                        if(filename[0]==0x00)
                            continue;
                    }
                }

                if(RelativeIsFile(filename))
                {
                    if((strrchr(filename,'.')>filename)&&
                            !strcmp((char*)strrchr(filename,'.'),".zip"))
                    {
                        // ZIP File
                        csString fullfilename = filelist->Get(i);
                        csString fulldirname = fullfilename + "/";

                        if(!strstr(currpathname,".zip"))
                            if(!vfs->Exists(fulldirname))
                            {
                                pawsListBoxRow* lbrow=dirlistbox->NewRow();
                                if(lbrow)
                                {
                                    csString corrdirname = filename;
                                    corrdirname+="/";
                                    pawsTextBox* filetb=(pawsTextBox*)lbrow->GetColumn(0);
                                    if(filetb)
                                        filetb->SetText(corrdirname);
                                    csRef<iDataBuffer> xrpath = vfs->GetRealPath(fullfilename);
                                    char* xpath=xrpath->GetData();
                                    vfs->Unmount(fulldirname,NULL);
                                    vfs->Mount(fulldirname,xpath);
                                    zip_mounts.Insert(0,fullfilename);
                                }
                            }
                    }

                    // FILE
                    if(z)
                    {
                        pawsListBoxRow* lbrow=filelistbox->NewRow();
                        if(lbrow)
                        {
                            pawsTextBox* filetb=(pawsTextBox*)lbrow->GetColumn(0);
                            if(filetb)
                                filetb->SetText(filename);
                        }
                    }
                }
                else   // DIR
                {
                    if(!z)
                    {
                        pawsListBoxRow* lbrow=dirlistbox->NewRow();
                        if(lbrow)
                        {
                            pawsTextBox* filetb=(pawsTextBox*)lbrow->GetColumn(0);
                            if(filetb)
                                filetb->SetText(filename);
                        }
                    }
                }
            }
        }
    }

    filelistbox->SortRows();
    dirlistbox->SortRows();

    return true;
}

bool pawsFileNavigation::UpOnePath()
{
    int position;
    position=(int)current_path.Length()-1;

    if(position<0)
        return false;
    if(current_path.GetAt(position) == '/' || current_path.GetAt(position) == '\\')
        position--;

    if(position<=0)
        return false;

    while(position>0 && current_path.GetAt(position) != '/' &&  current_path.GetAt(position) != '\\')
        position--;
    // Don't trim off the trailing slash
    current_path.Truncate(position+1);
    return true;
}

bool pawsFileNavigation::DownOnePath(const char* pathstring,int pathlen)
{
    if(pathlen<=0)
        return false;
    size_t current_len;

    // Append a delimiter if needed
    current_len=current_path.Length();
    if(current_len==0 || (current_path.GetAt(current_len-1) != '/' &&
                          current_path.GetAt(current_len-1) != '\\'))
        current_path.Append("/");

    current_path.Append(pathstring,pathlen);
    return true;
}


bool pawsFileNavigation::SmartAppendPath(const char* append)
{
    csString previous_path=current_path;

    while(append[0] != 0x00)
    {

        // Remove preceding slashes
        while(append[0] == '/' || append[0] == '\\')
            append++;

        // Handle ../ in path
        if(!strncmp(append,"..",2))
        {
            if(append[2] == 0x00 || append[2] == '/' || append[2] == '\\')
            {
                if(!UpOnePath())
                {
                    current_path=previous_path;
                    return false;
                }
                if(append[2] == 0x00)
                    break;
                append+=3;
                continue;
            }
        }

        // Handle ./ in path
        if(append[0] == '.' && (append[1] == '/' || append[1] == '\\'))
        {
            append+=2;
            continue;
        }

        // Find the next segment
        int segment_length=0;
        while(append[segment_length] != 0x00 && append[segment_length] != '/' && append[segment_length] != '\\')
            segment_length++;
        if(!DownOnePath(append,segment_length))
        {
            current_path=previous_path;
            return false;
        }
        append+=segment_length;
        if(append[0] != 0x00)
            append++;
    }

    // Add a trailing slash if necessary
    if(current_path.Length() == 0 || (
                current_path.GetAt(current_path.Length()-1) != '/' &&
                current_path.GetAt(current_path.Length()-1) != '\\'))
    {
        current_path.Append("/");
    }

    if(!vfs->Exists(current_path))
    {
        current_path=previous_path;
        return false;
    }

    return true;
}

bool pawsFileNavigation::SmartSetPath(const char* path)
{
    if(path[0] == '/' || path[0] == '\\')
    {
        if(!strcmp(path,"/")||!strcmp(path,"\\"))
        {
            current_path=path;
            return true;
        }

        csString previous_path=current_path;
        current_path="";
        if(!SmartAppendPath(path+1))
        {
            current_path=previous_path;
            return false;
        }
        return true;
    }
    return SmartAppendPath(path);
}


bool pawsFileNavigation::RelativeIsFile(const char* filename)
{
    size_t filename_len;
    if(!filename)
        return false;

    filename_len=strlen(filename);
    if(filename[filename_len-1] == '/' ||
            filename[filename_len-1] == '\\')
        return false;
    return true;
}


void pawsFileNavigation::OnListAction(pawsListBox* selected, int status)
{
    if(!filelistbox)
        filelistbox=(pawsListBox*)FindWidget("filelist");

    if(!dirlistbox)
        dirlistbox=(pawsListBox*)FindWidget("dirlist");

    if(selected==FindWidget("filetype")->FindWidget("ComboListBox"))
    {
        FillFileList();
        return;
    }

    if(selected==FindWidget("pathtext")->FindWidget("ComboListBox"))
    {
        pawsComboBox* boxtb=(pawsComboBox*)FindWidget("pathtext");
        csString pathtext = boxtb->GetSelectedRowString();
        current_path=pathtext;
        FillFileList();
        return;
    }

    if(!dirlistbox || !filelistbox || (selected != dirlistbox && selected != filelistbox))
        return;

    if(status == LISTBOX_SELECTED)
    {
        pawsListBoxRow* lbrow=selected->GetSelectedRow();
        if(lbrow)
        {
            pawsTextBox* filetb=(pawsTextBox*)lbrow->GetColumn(0);
            if(filetb)
            {
                const char* filename=filetb->GetText();
                if(filename)
                {
                    if(!strcmp(filename,"\\..") || !strcmp(filename,"/.."))
                    {
                        UpOnePath();
                        FillFileList();
                        return;
                    }

                    if(RelativeIsFile(filename))
                    {
                        // Handle selected a file
                        SetSelectedFilename(filename);
                    }
                    else
                    {
                        if(SmartAppendPath(filename))
                            FillFileList();
                    }
                }
            }
        }
    }
}

bool pawsFileNavigation::SetSelectedFilename(const char* filename)
{
    pawsEditTextBox* filenametb=(pawsEditTextBox*)FindWidget("fileedit");

    if(!filenametb)
        return false;

    filenametb->SetText(filename);

    return true;
}

bool pawsFileNavigation::SetFileFilters(const char* filetype)
{
    pawsComboBox* filetypetb=(pawsComboBox*)FindWidget("filetype");

    if(!filetypetb)
        return false;

    filetypetb->Clear();

    csString Filter = filetype;
    csString FilterValue;

    if(Filter.FindFirst('|') == (size_t)-1)
    {
        SetFilterForSelection(Filter);
        pawsComboBox* filetypetb=(pawsComboBox*)FindWidget("filetype");
        filetypetb->Select(Filter);
    }
    else
    {
        while(Filter.Length() > 0)
        {
            Filter.SubString(FilterValue,Filter.FindLast('|')+1,Filter.Length()-Filter.FindLast('|'));
            SetFilterForSelection(FilterValue.Downcase());
            Filter.Truncate(Filter.FindLast('|'));
        }
    }

    return true;
}

bool pawsFileNavigation::SetFilterForSelection(const char* filetype)
{
    pawsComboBox* filetypetb=(pawsComboBox*)FindWidget("filetype");

    if(!filetypetb)
        return false;

    filetypetb->NewOption(filetype);

    return true;
}

const char* pawsFileNavigation::GetFilterForSelection()
{
    const char* filetype = NULL;
    pawsComboBox* filetypetb=(pawsComboBox*)FindWidget("filetype");

    if(!filetypetb)
        return NULL;
    filetype=filetypetb->GetSelectedRowString();

    if(!filetype)
        return NULL;

    return filetype;
}

const char* pawsFileNavigation::GetFullPathFilename()
{
    size_t length;
    const char* filename;
    pawsEditTextBox* filenametb=(pawsEditTextBox*)FindWidget("fileedit");

    if(!filenametb)
        return NULL;
    filename=filenametb->GetText();

    if(!filename)
        return NULL;

    length=current_path.Length() + strlen(filename);
    delete[] fullpathandfilename;
    fullpathandfilename = new char[length+1];

    if(fullpathandfilename)
        sprintf(fullpathandfilename,"%s%s",current_path.GetData(),filename);
    return fullpathandfilename;
}

const char* pawsFileNavigation::GetCurrentPath()
{
    return current_path.GetData();
}

void pawsFileNavigation::SetActionButtonText(const char* actiontext)
{
    pawsButton* actionbutton=(pawsButton*)FindWidget("perform");

    if(!actionbutton || !actiontext)
        return;

    actionbutton->SetText(actiontext);
}


int pawsFileNavigation::GetSelectionState()
{
    return selection_state;
}


void pawsFileNavigation::Show()
{
    // Call the parent widget show logic
    pawsWidget::Show();
    FillFileList();
}


bool pawsFileNavigation::OnKeyDown(utf32_char /*keyCode*/, utf32_char key, int /*modifiers*/)
{
    if(key==CSKEY_ENTER)
    {
        const char* filetext;
        pawsEditTextBox* filenametb=(pawsEditTextBox*)FindWidget("fileedit");
        if(!filenametb)
            return false;

        filetext=filenametb->GetText();

        if(!filetext || filetext[0] == 0x00)
            return false;

        // Handle the case where the text field is a path specificiation
        if(SmartSetPath(filetext))
        {
            FillFileList();
            filenametb->SetText("");
            return true;
        }

        // Otherwise interpret this as an action request
        if(action)
        {
            action->Execute(GetFullPathFilename());
            delete action;
        }
        action = NULL;
        parent->DeleteChild(this);       // destructs itself
    }
    return false;
}

pawsFileNavigation* pawsFileNavigation::Create(
    const csString &filename, const csString &filter, iOnFileSelectedAction* action, const char* xmlWidget)
{
    pawsFileNavigation* window_filenav = NULL;
    if(PawsManager::GetSingleton().LoadWidget(xmlWidget))
    {
        window_filenav=(pawsFileNavigation*)PawsManager::GetSingleton().FindWidget("filenavigation");
        if(window_filenav)
        {
            window_filenav->Hide();

            PawsManager::GetSingleton().GetMainWidget()->AddChild(window_filenav);

            window_filenav->SetActionButtonText("Load");
            window_filenav->SetName("Open File");
            window_filenav->MoveTo(200,50);

            window_filenav->Initialize(filename, filter, action);
        }
        else
        {
            //  csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            //    "Warning: Couldn't find filenavigation widget.");
        }
    }
    else
    {
        //  csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
        //    "Warning: Couldn't load filenavigation widget.");
    }
    return window_filenav;
}

void pawsFileNavigation::Initialize(const csString &filename,const csString &filter, iOnFileSelectedAction* action)
{
    SetSelectedFilename(filename);
    SetFileFilters(filter);

    this->action = action;

    Show();
}
