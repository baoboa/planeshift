/*
 * mathscript_unittest.cpp by Kenny Graunke <kenny@whitecape.org>
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

//=============================================================================
// Project Includes
//=============================================================================
#include "util/mathscript.h"

//=============================================================================
// Library Includes
//=============================================================================
#include <gtest/gtest.h>

// Ugh, so lame...but somehow it thinks NULL is an integer otherwise, and fails
#undef NULL
#define NULL ((void*)0)

/*
class MathScriptTest : public testing::Test {
protected:
    MathScriptTest() { }
    ~MathScriptTest() { }
};
*/

TEST(MathScriptTest, BasicExpression)
{
    MathExpression *exp = MathExpression::Create("X < 7");
    ASSERT_NE(exp, NULL);
    MathEnvironment env;
    env.Define("X", 5);
    ASSERT_NE(env.Lookup("X"), NULL);
    EXPECT_NE(exp->Evaluate(&env), 0.0);
}

TEST(MathScriptTest, BasicScript)
{
    // Basic conversion from Fahrenheit to Celsius and Kelvin.
    MathScript *script = MathScript::Create("TemperatureConv", "C = (F - 32) * 5/9; K = C + 273.15");
    ASSERT_NE(script, NULL);
    MathEnvironment env;
    env.Define("F", 212.0);
    script->Evaluate(&env);
    MathVar *F = env.Lookup("F");
    MathVar *C = env.Lookup("C");
    MathVar *K = env.Lookup("K");
    EXPECT_NE(F, NULL);
    EXPECT_NE(C, NULL);
    EXPECT_NE(K, NULL);
    EXPECT_EQ(212.0,  F->GetValue());
    EXPECT_EQ(100.0,  C->GetValue());
    EXPECT_EQ(373.15, K->GetValue());
    EXPECT_STREQ("373.15", K->ToString());
    MathScript::Destroy(script);
}

class Foo : public iScriptableVar
{
public:
    virtual double GetProperty(const char *prop)
    {
        if (strcmp(prop, "TheAnswer") == 0)
            return 42;

        return 0.0;
    }
    virtual double CalcFunction(const char *function, const double *params)
    {
        if (strcmp(function, "Multiply") == 0)
            return params[0]*params[1];

        if (strcmp(function, "GetSkillRank") == 0)
        {
            csString skill(MathScriptEngine::GetString(params[0]));
            if (skill == "Lah'ar")
                return 77;
            if (skill == "Sword")
                return 33;
            return 0;
        }

        if (strcmp(function, "GetSeven") == 0)
        {
            return 7;
        }

        return 0.0;
    }
    const char* ToString()
    {
        return "Foo";
    }
};

TEST(MathScriptTest, BasicProperty)
{
    Foo foo;
    MathExpression *exp = MathExpression::Create("Quux:TheAnswer");
    MathEnvironment env;
    ASSERT_NE(exp, NULL);
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    EXPECT_EQ(42, exp->Evaluate(&env));
}

TEST(MathScriptTest, BasicMethod)
{
    Foo foo;
    MathExpression *exp = MathExpression::Create("Quux:Multiply(6,7)");
    MathEnvironment env;
    ASSERT_NE(exp, NULL);
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    EXPECT_EQ(42, exp->Evaluate(&env));
}

TEST(MathScriptTest, MethodNoArgs)
{
    Foo foo;
    MathExpression *exp = MathExpression::Create("Quux:GetSeven()");
    MathEnvironment env;
    ASSERT_NE(exp, NULL);
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    EXPECT_EQ(7, exp->Evaluate(&env));
}

TEST(MathScriptTest, PropertyAndFunction)
{
    Foo foo;
    MathExpression *exp = MathExpression::Create("Quux:TheAnswer = Quux:Multiply(6,7)");
    MathEnvironment env;
    ASSERT_NE(exp, NULL);
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    EXPECT_NE(0.0, exp->Evaluate(&env));
}

TEST(MathScriptTest, NestedCalls)
{
    Foo foo;
    MathExpression *exp = MathExpression::Create("Quux:Multiply(Quux:Multiply(2,3),7)");
    MathEnvironment env;
    ASSERT_NE(exp, NULL);
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    EXPECT_EQ(42, exp->Evaluate(&env));
};

TEST(MathScriptTest, InvalidAssignee)
{
    Foo foo;
    MathScript *script = MathScript::Create("invalid", "foo = 7");
    ASSERT_EQ(NULL, script);
};

TEST(MathScriptTest, StringTest)
{
    Foo foo;
    MathExpression *exp = MathExpression::Create("Quux:GetSkillRank('Lah\\'ar')");
    MathEnvironment env;
    ASSERT_NE(exp, NULL);
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    EXPECT_EQ(77, exp->Evaluate(&env));
};

TEST(MathScriptTest, StringTest2)
{
    Foo foo;
    MathScript *script = MathScript::Create("StringTest2", "\
        HasWeapon = 0;\
        SkillName = if(HasWeapon, 'Sword', \"Lah'ar\");\
        Rank = Quux:GetSkillRank(SkillName);\
    ");
    ASSERT_NE(script, NULL);
    MathEnvironment env;
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    script->Evaluate(&env);
    MathVar *rank = env.Lookup("Rank");
    EXPECT_EQ(77, rank->GetValue());
    MathScript::Destroy(script);
};

TEST(MathScriptTest, StringAssignment)
{
    Foo foo;
    MathScript *script = MathScript::Create("StringAssignment", "Skill = 'Sword'; Rank = Quux:GetSkillRank(Skill);");
    MathEnvironment env;
    ASSERT_NE(script, NULL);
    env.Define("Quux", &foo);
    script->Evaluate(&env);
    MathVar *rank = env.Lookup("Rank");
    EXPECT_EQ(33, rank->GetValue());
    MathScript::Destroy(script);
};

TEST(MathScriptTest, StringAssignment2)
{
    MathScript *script = MathScript::Create("StringAssignment2", "Skill = 'Sword';");
    ASSERT_NE(script, NULL);

    MathEnvironment env;
    script->Evaluate(&env);

    MathVar *rank = env.Lookup("Skill");
    ASSERT_NE(rank, NULL);
    EXPECT_STREQ("Sword", rank->ToString());
    MathScript::Destroy(script);
};


TEST(MathScriptTest, ParserStress)
{
    // Combat makes for a pretty good parser stress test.
    MathScript *script = MathScript::Create("Calculate Damage", "\
        AttackRoll = rnd(1);\
        DefenseRoll = rnd(1);\
\
        WeaponSkill = Attacker:getAverageSkillValue(AttackWeapon:Skill1,AttackWeapon:Skill2,AttackWeapon:Skill3);\
        TargetWeaponSkill = Target:getAverageSkillValue(TargetAttackWeapon:Skill1,TargetAttackWeapon:Skill2,TargetAttackWeapon:Skill3);\
\
        IAH = min(AttackRoll-.25,0.1);\
        exit = if(0>IAH,1,0);\
        AHR = min(AttackRoll-.5,.01);\
        Blocked = AttackRoll - DefenseRoll;\
        QOH = Blocked;\
\
        RequiredInputVars = Target:AttackerTargeted+Attacker:getSkillValue(AttackWeapon:Skill1)+AttackLocationItem:Hardness;\
\
        AttackerStance = Attacker:CombatStance;\
        TargetStance = Target:CombatStance;\
\
        AttackValue = WeaponSkill;\
        TargetAttackValue = TargetWeaponSkill;\
        DV = Attacker:Agility;\
        TargetDV = 0;\
\
        AVStance = if(AttackerStance=1, (AttackValue*2)+(DV*0.8),\
               if(AttackerStance=2, (AttackValue*1.5)+(DV*0.5),\
               if(AttackerStance=3, AttackValue,\
               if(AttackerStance=4, (AttackValue*0.3),\
               0))));\
\
        TargetAVStance = if(TargetStance=1, (TargetAttackValue*2)+(TargetDV*0.8),\
               if(TargetStance=2, (TargetAttackValue*1.5)+(TargetDV*0.5),\
               if(TargetStance=3, TargetAttackValue,\
               if(TargetStance=4, (TargetAttackValue*0.3),\
               0))));\
\
        FinalDamage = 10*(AVStance-TargetDV);\
    ");
    ASSERT_NE(script, NULL);
    // Don't even try to run it...
}

TEST(MathScriptTest, InterpolateTest)
{
    csString msg("Xordan hits you for ${Elite} damage...");
    MathEnvironment env;
    env.Define("Elite", 1337);
    env.Define("E", 3.1337);
    env.InterpolateString(msg);
    EXPECT_STREQ("Xordan hits you for 1337 damage...", msg.GetData());

    msg = "${E} times, because he's just that ${Elite}";
    env.InterpolateString(msg);
    EXPECT_STREQ("3.13 times, because he's just that 1337", msg.GetData());

    msg = "${Elite}";
    env.InterpolateString(msg);
    EXPECT_STREQ("1337", msg.GetData());

    msg = "${} ${Elite} ${}";
    env.InterpolateString(msg);
    EXPECT_STREQ("${} 1337 ${}", msg.GetData());
}

TEST(MathScriptTest, MultipleOr)
{
    Foo foo;
    MathExpression *exp = MathExpression::Create("Quux:Multiply(0,8) | Quux:Multiply(2,3) | Quux:Multiply(17,0)");
    ASSERT_NE(exp, NULL);
    MathEnvironment env;
    env.Define("Quux", &foo);
    ASSERT_NE(env.Lookup("Quux"), NULL);
    EXPECT_NE(exp->Evaluate(&env), 0.0);
}

/*
TEST(MathScriptTest, NameMemoryTest)
{
    csString name("Good");
    MathScript *script = MathScript::Create(name, "Skill = Sword;");
    ASSERT_NE(script, NULL);

    name = "BAD";

    MathEnvironment env;
    script->Evaluate(&env);
    // MathExpression's assert should say "Good" here, not "BAD"
}
*/

void randomgentest(int limit)
{
   csString scriptstr;
    if (limit<0)
    {
       scriptstr = "Roll = rnd();";
       limit=1;
    }
    else    
    {
       scriptstr.Format("Roll = rnd(%d);",limit);
    }
    MathScript *script = MathScript::Create("randomgen test", scriptstr.GetData());
    MathEnvironment env;
    ASSERT_NE(script, NULL) << scriptstr.GetData() << " did not create script";
    bool above1=false, abovehalflimit=false;
    for (int i=0; i<100; i++) //try 100 times, since this is random
    {
       script->Evaluate(&env);
       MathVar *roll = env.Lookup("Roll");
       EXPECT_GE(roll->GetValue(), 0); 
       EXPECT_LE(roll->GetValue(), limit);
       if (roll->GetValue()>1)
       {
          above1 = true;
       }
       if (2*roll->GetValue()>limit)
       {
          abovehalflimit = true;
       }
    }
    if (limit>1)
    {
       EXPECT_TRUE(above1) << scriptstr << "never exceeds 1"; 
    }
    if (limit>0)
    {
       EXPECT_TRUE(abovehalflimit) << scriptstr << "never exceeds half of limit"; 
    }
    MathScript::Destroy(script);
}
TEST(MathScriptTest, RandomGenTest)
{
   randomgentest(100);
   randomgentest(50);
   randomgentest(1);
   randomgentest(0);  //should always be 0 :)
   //randomgentest(-1); //this will test rnd(), which should limit at 1, but is NOT IMPLEMENTED
}
