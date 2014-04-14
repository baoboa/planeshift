/*
 * localization.cpp - Author: Ondrej Hurt
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


// CS INCLUDES
#include <psconfig.h>
#include <iutil/document.h>
#include <iutil/vfs.h>

// COMMON INCLUDES
#include "util/log.h"
#include "paws/pawsmanager.h"

// CLIENT INCLUDES
#include "localization.h"

#define DEFAULT_FILE_PATH "/this/"

psLocalization::psLocalization()
{
    object_reg = NULL;
}

psLocalization::~psLocalization()
{
    ClearStringTable();
}

void psLocalization::Initialize(iObjectRegistry* _object_reg)
{
    object_reg = _object_reg;
    dirty = false;
}

void psLocalization::SetLanguage(const csString & _lang)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, tblRoot, authorRoot, item;
    csRef<iDocumentNodeIterator> itemIter;
    psStringTableItem * itemData;

    assert(object_reg != NULL);

    ClearStringTable();
    language = _lang;

    filename = "/this/lang/" + language + "/stringtable.xml";

    // If the file is not available at all, we will just silently quit because it is not required.
    // But any other errors will be reported.
    if ( ! FileExists(filename) )
    {
        filename = "";
        return;
    }

    doc = ParseFile(object_reg, filename);
    if (doc == NULL)
    {
        Error2("Parsing of string table file %s failed", filename.GetData());
        return;
    }
    root = doc->GetRoot();
    if (root == NULL)    
    {
        Error2("String table file %s has no root", filename.GetData());
        return;
    }
    
    authorRoot = root->GetNode("Author");
    if (authorRoot != NULL)
        authors = authorRoot->GetAttributeValue("text");
    else
        authors = "";

    tblRoot = root->GetNode("StringTable");
    if (tblRoot == NULL)
    {
        Error2("String table file %s must have <StringTable> tag", filename.GetData());
        return;
    }

    int i=0;
    itemIter = tblRoot->GetNodes("item");
    while (itemIter->HasNext())
    {
        item = itemIter->Next();

        itemData = new psStringTableItem;
        itemData->original   =  item->GetAttributeValue("orig");
        itemData->translated =  item->GetAttributeValue("trans");
        stringTbl.Put(itemData->original.GetData(), itemData);
        i++;
    }
    printf("--------------------------------\nLoaded %d translation entries for %s.\n--------------------------------\n", i, language.GetData());
}


csString psLocalization::FindLocalizedFile(const csString & shortPath)
{
    csString fullPath;

    PawsManager &paws = PawsManager::GetSingleton();
    fullPath = paws.getVFSPathToSkin() + shortPath;
    if (FileExists(fullPath))
        return fullPath;

    fullPath.Format("/this/lang/%s/%s",language.GetDataSafe(),shortPath.GetDataSafe());
    if (FileExists(fullPath))
        return fullPath;

    fullPath = "/this/data/gui/" + shortPath;
    if (FileExists(fullPath))
        return fullPath;
    else
    {    
        fullPath = DEFAULT_FILE_PATH + shortPath;
        return fullPath;
    }
}

const csString& psLocalization::Translate(const csString & orig)
{
    if (orig.IsEmpty() || filename.IsEmpty() )
        return orig;

    psStringTableItem * item;
    psStringTableHash::Iterator iter = stringTbl.GetIterator(orig.GetData());

    while (iter.HasNext())
    {
        item = (psStringTableItem*)iter.Next();
        if (item->original == orig && item->translated.Length() > 0)  // length 0 check is for "not found" ones added dynamically
            return item->translated;
        else if (item->translated.Length() == 0)
            return orig;
    }

    // orig not found, so store it
    printf("Added '%s' to stringtable.\n", orig.GetDataSafe() );

    item = new psStringTableItem;
    item->original    =  orig;
    item->translated  =  "";
    stringTbl.Put(item->original.GetData(), item);
    dirty = true;

    return orig;
}

bool psLocalization::FileExists(const csString & fileName)
{
    csRef<iVFS> vfs;

    vfs =  csQueryRegistry<iVFS > ( object_reg);
    return vfs->Exists(fileName);
}


void psLocalization::ClearStringTable()
{
    if (dirty)
        WriteStringTable();

    psStringTableItem * item;
    psStringTableHash::GlobalIterator iter = stringTbl.GetIterator();

    while (iter.HasNext())
    {
        item = (psStringTableItem*)iter.Next();
        delete item;
    }
    stringTbl.DeleteAll();
    dirty = false;
}

int CompareStringTableItem(psStringTableItem* const &first,  psStringTableItem* const &second)
{
    return csComparator<csString,csString>::Compare(first->original,second->original);
}


void psLocalization::WriteStringTable()
{
    if (filename.IsEmpty())
        return;

    csRef<iVFS> vfs;

    vfs =  csQueryRegistry<iVFS > ( object_reg);
    csRef<iFile> file = vfs->Open(filename,VFS_FILE_WRITE);

    if (!file)
    {
        Error2("Could not write stringtable file '%s'.", filename.GetDataSafe() );
        return;
    }

    psStringTableItem * item;
    psStringTableHash::GlobalIterator iter = stringTbl.GetIterator();

    csString AuthorString = "<Author text=\"" + EscpXML(authors.GetDataSafe()) + "\" />\n";
    
    file->Write(AuthorString, AuthorString.Length());
    file->Write("<StringTable>\n", strlen("<stringtable>\n") );

    // Sort the list before writing.
    csArray<psStringTableItem*> array;
    while (iter.HasNext())
    {
        item = (psStringTableItem*)iter.Next();
        array.InsertSorted(item,CompareStringTableItem);
    }

    // Write the sorted list to file.
    csArray<psStringTableItem*>::Iterator iterSorted = array.GetIterator();
    int count=0;
    while (iterSorted.HasNext())
    {
        count++;
        item = (psStringTableItem*)iterSorted.Next();
        csString line;
        line.Format("  <item orig=\"%s\" trans=\"%s\" />\n", EscpXML(item->original).GetDataSafe(), EscpXML(item->translated).GetDataSafe() );
        file->Write(line, line.Length() );
    }

    file->Write("</StringTable>\n",strlen("</stringtable>\n") );

    printf("-----------------------------------\nSaved %d translation entries.\n-----------------------------------\n", count);

}
