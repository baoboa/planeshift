/*
 *  testrpgrules.h - Author: Keith Fulton
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

class psMoney;

class TestRPGRules
{
public:
    TestRPGRules(iObjectRegistry* object_reg);
    ~TestRPGRules();

    int Run();

private:
    void PrintHelp();
    void PrintOutput(const char* string, ...);

	bool TestMoney(iDocumentNode *testnode);
	bool InitMoney(iDocumentNode *command, psMoney& money);
	bool CheckMoneyString(iDocumentNode *command, psMoney& money);
	bool CheckMoneyTotal(iDocumentNode *command, psMoney& money);
	bool CheckMoneyTrias(iDocumentNode *command, psMoney& money);
	bool CheckMoneyHexas(iDocumentNode *command, psMoney& money);
	bool CheckMoneyOctas(iDocumentNode *command, psMoney& money);
	bool CheckMoneyCircles(iDocumentNode *command, psMoney& money);
	bool AdjustTrias(iDocumentNode *command, psMoney& money);
	bool AdjustHexas(iDocumentNode *command, psMoney& money);
	bool AdjustOctas(iDocumentNode *command, psMoney& money);
	bool AdjustCircles(iDocumentNode *command, psMoney& money);
	bool AdjustMoney(iDocumentNode *command, psMoney& money);
	bool CheckUserString(iDocumentNode *command, psMoney& money);
	bool NormalizeMoney(iDocumentNode *command, psMoney& money);
	bool CheckGreaterThan(iDocumentNode *command, psMoney& money);
	bool SubtractMoney(iDocumentNode *command, psMoney& money);
	bool NegateMoney(iDocumentNode *command, psMoney& money);
	bool MultiplyMoney(iDocumentNode *command, psMoney& money);

	csRef<iObjectRegistry> object_reg;
	csRef<iVFS> vfs;
	csRef<iFile> log;
	csRef<iDocumentSystem> docsys;

};
