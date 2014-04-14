/*
 *  transtool.cpp - Author: Stefano Angeleri
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

#include <cssysdef.h>

#include <cstool/initapp.h>
#include <csutil/cmdhelp.h>
#include <csutil/documenthelper.h>
#include <csutil/xmltiny.h>

#include <iutil/cmdline.h>
#include <iutil/document.h>

#include "transtool.h"

CS_IMPLEMENT_APPLICATION

TransTool::TransTool(iObjectRegistry* object_reg) : object_reg(object_reg)
{
    docsys = csQueryRegistry<iDocumentSystem>(object_reg);
    vfs = csQueryRegistry<iVFS>(object_reg);
}

TransTool::~TransTool()
{
}

void TransTool::PrintHelp()
{
    printf("This application converts planeshift translation files to L10N translation xml files and viceversa.\n\n");

    printf("Options:\n");
    printf("-in The file to convert. Defaults to /this/stringtable.xml\n");
    printf("-out the destination file of the conversion. Defaults to /this/strings.xml\n");
    printf("-atp converts from L10N to planeshift format else the conversion will be from planeshift format to L10N\n");
    printf("-orig uses the orig also for the translation part\n");
    printf("-clean removes empty entries\n");
    printf("-author sets the author of the translation file. Has meaning only with -atp\n");
    printf("Usage: transtool -in /this/stringtable.xml -out /this/strings.xml");
}

void TransTool::Run()
{
    printf("Translation converter.\n\n");

    csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser>(object_reg);
    if (csCommandLineHelper::CheckHelp (object_reg))
    {
        PrintHelp();
        return;
    }

    //get the required options from the command line

    csString infile = cmdline->GetOption("in");
    if(infile.IsEmpty())
    {
        infile = "/this/stringtable.xml";
    }

    csString outfile = cmdline->GetOption("out");
    if(outfile.IsEmpty())
    {
        outfile = "/this/strings.xml";
    }

    bool atp = cmdline->GetBoolOption("atp");
    bool useOrig = cmdline->GetBoolOption("orig");
    bool cleanEmpty = cmdline->GetBoolOption("clean");

    csString author = cmdline->GetOption("author");

    printf("converting %s to %s\n", infile.GetData(), outfile.GetData());

    //initialize

    //if we require an L10N to planeshift format we will run this segment
    if(atp)
    {
        //prepare our destination document
        csRef<iDocument> doc = tinydoc.CreateDocument();
        csRef<iDocumentNode> transout = doc->CreateRoot();
        transout = transout->CreateNodeBefore(CS_NODE_ELEMENT);
        //add the author entry
        transout->SetValue("Author");
        transout->SetAttribute("text", author);
        //add the main stringtable entry
        transout = transout->CreateNodeBefore(CS_NODE_ELEMENT);
        transout->SetValue("StringTable");

        //open the input file for conversion
        csRef<iDataBuffer> buf = vfs->ReadFile(infile);
        if(!buf.IsValid())
        {
            printf("unable to open input file\n");
            return;
        }

        //parse the input file and get to the node we are interested in
        //in L10N resources is the equivalent of our stringtable
        csRef<iDocument> indoc = docsys->CreateDocument();
        indoc->Parse(buf, true);
        csRef<iDocumentNode> transIn = indoc->GetRoot();
        csRef<iDocumentNodeIterator> transInItera = transIn->GetNodes();
        transIn = transIn->GetNode("resources");

        //iterate for all string entries
        csRef<iDocumentNodeIterator> transInIter = transIn->GetNodes("string");
        while(transInIter->HasNext())
        {
            csRef<iDocumentNode> node = transInIter->Next();
            //create an item element to fill our data
            csRef<iDocumentNode> transString = transout->CreateNodeBefore(CS_NODE_ELEMENT);
            transString->SetValue("item");
            //set the name of the resource as the original text
            csString tmpName = node->GetAttributeValue("name");
            //sanitize the name by putting back new lines
            tmpName.ReplaceAll("&#10;","\n");
            transString->SetAttribute("orig", tmpName);
            //set the translated text in trans="", in case it's missing just put an empty entry
            transString->SetAttribute("trans", node->GetContentsValue() ? node->GetContentsValue() : "" );
        }

        //write out the result
        doc->Write(vfs, outfile);
    }
    else
    {
        //in this case we conver a planeshift translation file to L10N format
        csRef<iDocument> doc = tinydoc.CreateDocument();
        csRef<iDocumentNode> transout = doc->CreateRoot();
        transout = transout->CreateNodeBefore(CS_NODE_ELEMENT);
        //create a resources node which is the root node containing translations
        transout->SetValue("resources");

        //read the input file
        csRef<iDataBuffer> buf = vfs->ReadFile(infile);
        if(!buf.IsValid())
        {
            printf("unable to open input file\n");
            return;
        }

        //parse it and find the string table node which is the one we need as it contains our translations
        csRef<iDocument> indoc = docsys->CreateDocument();
        indoc->Parse(buf, true);
        csRef<iDocumentNode> transIn = indoc->GetRoot();
        transIn = transIn->GetNode("StringTable");

        //for each item (which is a translation entry) add one in the destination file
        csRef<iDocumentNodeIterator> transInIter = transIn->GetNodes("item");
        while(transInIter->HasNext())
        {
            csRef<iDocumentNode> node = transInIter->Next();
            //check if we want to skip empty entries (useful when loading translated files)
            if(cleanEmpty && csString(node->GetAttributeValue(useOrig? "orig" : "trans")).Length() == 0)
                continue;

            //create a node with the string
            csRef<iDocumentNode> transString = transout->CreateNodeBefore(CS_NODE_ELEMENT);
            transString->SetValue("string");

            //set the name of the string as the orig
            transString->SetAttribute("name", node->GetAttributeValue("orig"));
            csRef<iDocumentNode> transStringInner = transString->CreateNodeBefore(CS_NODE_TEXT);
            //set the translation of the string from trans or orig depending on the command line option
            //-orig
            //when uploading the base file it's better to use the orig for the translated string
            transStringInner->SetValue(node->GetAttributeValue(useOrig? "orig" : "trans"));
        }

        //write out the result
        doc->Write(vfs, outfile);
    }
}

int main(int argc, char** argv)
{
    iObjectRegistry* object_reg = csInitializer::CreateEnvironment(argc, argv);
    if(!object_reg)
    {
        printf("Object Reg failed to Init!\n");
        return 1;
    }

    csInitializer::RequestPlugins (object_reg, CS_REQUEST_VFS,
    CS_REQUEST_PLUGIN("crystalspace.documentsystem.multiplexer", iDocumentSystem),
    CS_REQUEST_END);

    TransTool* transtool = new TransTool(object_reg);
    transtool->Run();
    delete transtool;
    CS_STATIC_VARIABLE_CLEANUP
    return 0;
}
