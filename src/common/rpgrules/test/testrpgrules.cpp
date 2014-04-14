/*
 *  testrpgrules.cpp - Author: Keith Fulton
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cstool/initapp.h>
#include <csutil/cmdhelp.h>
#include <csutil/stringarray.h>

#include <iutil/cmdline.h>
#include <iutil/document.h>
#include <iutil/stringarray.h>

#include "util/pscssetup.h"
#include "testrpgrules.h"
#include "../psmoney.h"



CS_IMPLEMENT_APPLICATION

TestRPGRules::TestRPGRules(iObjectRegistry* object_reg) : object_reg(object_reg)
{
	docsys = csQueryRegistry<iDocumentSystem>(object_reg);
	vfs    = csQueryRegistry<iVFS>(object_reg);
    log    = vfs->Open("/this/testrpgrules.log", VFS_FILE_WRITE);
}

TestRPGRules::~TestRPGRules()
{
}

void TestRPGRules::PrintHelp()
{
    printf("This application runs an xml script to test the psMoney class.\n\n");

//    printf("Options:\n");
 //   printf("-path The vfs path to directory to search in. Defaults to /this/ccheck/\n\n");
  //  printf("Usage: ccheck(.exe) -path /this/path/to/directory/\n");
}

int TestRPGRules::Run()
{
    PrintOutput("RPG Rules Library Unit Test Application.\n\n");

	csRef<iDataBuffer> buf = vfs->ReadFile("/this/data/tests/rpgrules.xml");
	if (!buf)
	{
		PrintOutput("Cannot load file /this/data/tests/rpgrules.xml\n");
		return 1;
	}
	csRef<iDocument> doc = docsys->CreateDocument();

	doc->Parse(buf, true);

	if(!doc->GetRoot().IsValid())
	{
		PrintOutput("XML in /this/data/tests/rpgrules.xml is not valid.\n");
		return 2;
	}

	csRef<iDocumentNode> root = doc->GetRoot()->GetNode("rpgtestsuite");
	if(!root.IsValid())
	{
		PrintOutput("No <rpgtestsuite> node in xml test file.\n");
		return 3;
	}

	csRef<iDocumentNodeIterator> itr = root->GetNodes("testmoney");
	while(itr->HasNext())
	{
		csRef<iDocumentNode> node = itr->Next();
		bool b = TestMoney(node);
		if (!b)
			return 4;
	}

	PrintOutput("All tests passed successfully.\n");

	return 0;
}

bool TestRPGRules::TestMoney(iDocumentNode *testnode)
{
	PrintOutput("Running test: %-30s ", testnode->GetAttributeValue("name"));

	bool result = true;
	psMoney money;

	csRef<iDocumentNodeIterator> itr = testnode->GetNodes();
	while(result && itr->HasNext())
	{
		csRef<iDocumentNode> node = itr->Next();
		if (!strcmp(node->GetValue(),"init"))
			result = InitMoney(node,money);
		else if (!strcmp(node->GetValue(),"checkstr"))
			result = CheckMoneyString(node, money);
		else if (!strcmp(node->GetValue(),"checktotal"))
			result = CheckMoneyTotal(node, money);
		else if (!strcmp(node->GetValue(),"checktrias"))
			result = CheckMoneyTrias(node, money);
		else if (!strcmp(node->GetValue(),"checkhexas"))
			result = CheckMoneyHexas(node, money);
		else if (!strcmp(node->GetValue(),"checkoctas"))
			result = CheckMoneyOctas(node, money);
		else if (!strcmp(node->GetValue(),"checkcircles"))
			result = CheckMoneyCircles(node, money);
		else if (!strcmp(node->GetValue(),"adjusttrias"))
			result = AdjustTrias(node, money);
		else if (!strcmp(node->GetValue(),"adjusthexas"))
			result = AdjustHexas(node, money);
		else if (!strcmp(node->GetValue(),"adjustoctas"))
			result = AdjustOctas(node, money);
		else if (!strcmp(node->GetValue(),"adjustcircles"))
			result = AdjustCircles(node, money);
		else if (!strcmp(node->GetValue(),"adjustmoney"))
			result = AdjustMoney(node, money);
		else if (!strcmp(node->GetValue(),"checkuserstring"))
			result = CheckUserString(node, money);
		else if (!strcmp(node->GetValue(),"normalize"))
			result = NormalizeMoney(node, money);
		else if (!strcmp(node->GetValue(),"subtract"))
			result = SubtractMoney(node, money);
		else if (!strcmp(node->GetValue(),"negate"))
			result = NegateMoney(node, money);
		else if (!strcmp(node->GetValue(),"multiply"))
			result = MultiplyMoney(node, money);
	}

	if (result)
		PrintOutput("Ok\n");
	
	return result;
}

bool TestRPGRules::InitMoney(iDocumentNode *command, psMoney& money)
{
	printf("Init money\n");
	money.Set(command->GetAttributeValue("value"));
	return true;
}

bool TestRPGRules::CheckMoneyString(iDocumentNode *command, psMoney& money)
{
	printf("Check string conversion of money\n");
	csString val = money.ToString();
	return (val == command->GetAttributeValue("value"));
}

bool TestRPGRules::CheckMoneyTotal(iDocumentNode *command, psMoney& money)
{
	printf("Check total money\n");
	int val = money.GetTotal();
	return (val == command->GetAttributeValueAsInt("value"));
}

bool TestRPGRules::CheckMoneyTrias(iDocumentNode *command, psMoney& money)
{
	printf("Check # of trias\n");
	int val = money.GetTrias();
	return (val == command->GetAttributeValueAsInt("value"));
}

bool TestRPGRules::CheckMoneyHexas(iDocumentNode *command, psMoney& money)
{
	printf("Check # of hexas\n");
	int val = money.GetHexas();
	return (val == command->GetAttributeValueAsInt("value"));
}

bool TestRPGRules::CheckMoneyOctas(iDocumentNode *command, psMoney& money)
{
	printf("Check # of octas\n");
	int val = money.GetOctas();
	return (val == command->GetAttributeValueAsInt("value"));
}

bool TestRPGRules::CheckMoneyCircles(iDocumentNode *command, psMoney& money)
{
	printf("Check # of circles\n");
	int val = money.GetCircles();
	return (val == command->GetAttributeValueAsInt("value"));
}

bool TestRPGRules::AdjustTrias(iDocumentNode *command, psMoney& money)
{
	printf("Adjust # of trias\n");
	money.AdjustTrias(command->GetAttributeValueAsInt("value"));
	return true;
}

bool TestRPGRules::AdjustHexas(iDocumentNode *command, psMoney& money)
{
	printf("Adjust # of hexas\n");
	money.AdjustHexas(command->GetAttributeValueAsInt("value"));
	return true;
}

bool TestRPGRules::AdjustOctas(iDocumentNode *command, psMoney& money)
{
	printf("Adjust # of octas\n");
	money.AdjustOctas(command->GetAttributeValueAsInt("value"));
	return true;
}

bool TestRPGRules::AdjustCircles(iDocumentNode *command, psMoney& money)
{
	printf("Adjust # of circles\n");
	money.AdjustCircles(command->GetAttributeValueAsInt("value"));
	return true;
}

bool TestRPGRules::AdjustMoney(iDocumentNode *command, psMoney& money)
{
	printf("Adjust money with csv string\n");
	psMoney other(command->GetAttributeValue("value"));
	money += other;
	return true;
}

bool TestRPGRules::CheckUserString(iDocumentNode *command, psMoney& money)
{
	printf("Check user string\n");
	return (money.ToUserString() == command->GetAttributeValue("value"));
}

bool TestRPGRules::NormalizeMoney(iDocumentNode *command, psMoney& money)
{
	printf("Check user string\n");
	money = money.Normalized();
	return true;
}

bool TestRPGRules::CheckGreaterThan(iDocumentNode *command, psMoney& money)
{
	printf("Check greater than\n");
	psMoney other(command->GetAttributeValue("value"));
	return money > other;
}

bool TestRPGRules::SubtractMoney(iDocumentNode *command, psMoney& money)
{
	printf("Check subtract\n");
	psMoney other(command->GetAttributeValue("value"));
	money = money - other;
	return true;
}

bool TestRPGRules::NegateMoney(iDocumentNode *command, psMoney& money)
{
	printf("Check negate\n");
	money = -money;
	return true;
}

bool TestRPGRules::MultiplyMoney(iDocumentNode *command, psMoney& money)
{
	printf("Check multiply operator\n");
	money = money * command->GetAttributeValueAsInt("value");
	return true;
}


void TestRPGRules::PrintOutput(const char* string, ...)
{
    csString outputString;
    va_list args;
    va_start (args, string);
    outputString.FormatV (string, args);
    va_end (args);

    printf("%s", outputString.GetData());

    if(log.IsValid())
    {
        log->Write(outputString.GetData(), outputString.Length());
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

    TestRPGRules* test = new TestRPGRules(object_reg);
    int ret = test->Run();
    delete test;

	PS_PAUSEEXIT(ret);

    return 0;
}
