# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'math_scripts'
#

DROP TABLE IF EXISTS `math_scripts`;
CREATE TABLE math_scripts (
  name varchar(40) NOT NULL DEFAULT '' ,
  math_script text NOT NULL,
  PRIMARY KEY (name)
);


#
# Dumping data for table 'math_scripts'
#

INSERT INTO math_scripts VALUES( "Calculate Damage",
"
	AttackRoll = rnd(1);
        DefenseRoll = rnd(1);

        WeaponSkill = Attacker:GetAverageSkillValue(AttackWeapon:Skill1,AttackWeapon:Skill2,AttackWeapon:Skill3);
        TargetWeaponSkill = Target:GetAverageSkillValue(TargetWeapon:Skill1,TargetWeapon:Skill2,TargetWeapon:Skill3);

        PI = 3.14159265358979323846;
        Dist = DiffX*DiffX + DiffY*DiffY + DiffZ*DiffZ;

        BadRange = pow(AttackWeapon:Range * 1.1 + 1,2) - Dist;
        exit = if(0>BadRange,1,0);

        Angle = atan2(-DiffX,-DiffZ);
        AngleAtt = Angle;
        Angle = Attacker:loc_yrot - Angle;
        Angle = Angle % (2*PI);
        Angle = if(Angle > PI, Angle-2*PI, Angle);
        BadAngle = PI * if(Dist>1.5, 0.3, 0.4) - abs(Angle);
        exit = if(0>BadAngle,1,0);

        Missed = min(AttackRoll-.25,0.1);
        exit = if(0>Missed,1,0);

        Dodged = min(AttackRoll-.5,.01);
        if(Dodged < 0)
        {
          if(Attacker:IsNPC)
          {
            Target:PracticeSkillID(AttackLocationItem:Skill1,1);
            Target:PracticeSkillID(AttackLocationItem:Skill2,1);
            Target:PracticeSkillID(AttackLocationItem:Skill3,1);
          }

          exit = 1;
        }

        Blocked = AttackRoll - DefenseRoll;
        if(Blocked < 0)
        {
          if(Attacker:IsNPC)
          {
            if(TargetWeapon:IsShield)
            {
              Target:PracticeSkillID(TargetWeapon:Skill1,1);
              Target:PracticeSkillID(TargetWeapon:Skill2,1);
              Target:PracticeSkillID(TargetWeapon:Skill3,1);
            }

            Target:PracticeSkillID(AttackLocationItem:Skill1,1);
            Target:PracticeSkillID(AttackLocationItem:Skill2,1);
            Target:PracticeSkillID(AttackLocationItem:Skill3,1);
          }

          exit = 1;
        }

        RequiredInputVars = Target:AttackerTargeted+Attacker:GetSkillValue(AttackWeapon:Skill1)+AttackLocationItem:Hardness;

        AttackerStance = Attacker:CombatStance;
        TargetStance = Target:CombatStance;

        AttackValue = WeaponSkill;
        TargetAttackValue = TargetWeaponSkill;
        DV = Attacker:GetSkillValue(46);
        TargetDV = 0;

        AVStance = (AttackValue*Attacker:attack_damage_mod)+(DV*Attacker:defense_absorb_mod);

        TargetAVStance = (TargetAttackValue*Target:attack_damage_mod)+(TargetDV*Target:defense_absorb_mod);

        FinalDamage = 10*(AVStance-TargetDV);
        if(Target:IsNPC)
        {
          Attacker:PracticeSkillID(AttackWeapon:Skill1,1);
          Attacker:PracticeSkillID(AttackWeapon:Skill2,1);
          Attacker:PracticeSkillID(AttackWeapon:Skill3,1);
        }
");

INSERT INTO math_scripts VALUES( "Calculate Decay",
"
    WeaponDecay = if(Blocked, 1.25, 1 - ArmorVsWeapon) * Weapon:DecayRate * (1 - Weapon:DecayResistance);
    if(Blocked)
    {
      BlockingDecay = 0.75 * BlockingWeapon:DecayRate * (1 - BlockingWeapon:DecayResistance);
    }
    else
    {
      ArmorDecay = ArmorVsWeapon * Armor:DecayRate * (1 - Armor:DecayResistance);
    }
");

INSERT INTO math_scripts VALUES( "Lockpicking Time", "Time = ((LockQuality / 10) * 3)*1000;");

INSERT INTO math_scripts VALUES( "Calculate Fall Damage",
"
	exit = (rnd(1) > 0.5);
        Damage = if(FallHeight>1,pow(FallHeight, 1.8) * 0.8, 0);
");

INSERT INTO math_scripts VALUES( "CalculateManaCost", "ManaCost = Realm*4*(1+(KFactor*KFactor/10000)); StaminaCost = ManaCost;");

INSERT INTO math_scripts VALUES( "CalculateChanceOfCastSuccess", "ChanceOfSuccess = (50-KFactor) + WaySkill/25 + (RelatedStat)/25;");

INSERT INTO math_scripts VALUES( "CalculateChanceOfResearchSuccess", "ChanceOfSuccess = WaySkill/10 + ( 10 / Spell:Realm ) * 7;");

INSERT INTO math_scripts VALUES( "CalculatePowerLevel", "MentalStatBonus = (RelatedStat-40)/5.4; PowerLevel = 1.0 + (WaySkill/10)*(1+(1+(200-WaySkill)/100)*(KFactor*KFactor/10000)) + (MentalStatBonus/10);");

INSERT INTO math_scripts VALUES( "SpellPractice", "PracticePoints = floor(10/(1 + MaxRealm - Realm))");

INSERT INTO math_scripts VALUES( "CalculateMaxCarryWeight", "MaxCarry =  ( Actor:GetStatValue(0) );");

INSERT INTO math_scripts VALUES( "CalculateMaxCarryAmount", "MaxAmount =  750;");

INSERT INTO math_scripts VALUES( "MaxRealm", "MaxRealm = 1 + floor(WaySkill / 20);");

INSERT INTO math_scripts VALUES( "CalculateConsumeQuality", "QualityLevel = Quality/300;");

INSERT INTO math_scripts VALUES( "CalculateSkillCosts",
"
        NextRankCost = BaseCost +(1*(SkillRank/200));
        ZCost = (PracticeFactor/100.00)*NextRankCost;
        YCost = ((100.00-PracticeFactor)/100.00)*NextRankCost;
");

INSERT INTO math_scripts VALUES( "CalculateStatCosts",
"
        YCost = SkillRank / 10 + 10;
        ZCost = 0;
");

INSERT INTO math_scripts VALUES( "StaminaMove",
"
        Drain = (Speed/3) * (Weight/MaxWeight);
");

INSERT INTO math_scripts VALUES( "StaminaCombat",
"
        PhyDrain = 4 + rnd(3);
        MntDrain = 2 + rnd(5);
");

INSERT INTO math_scripts VALUES( "StaminaBase",
"
        BasePhy = (Actor:GetSkillValue(50) + Actor:GetSkillValue(48) + Actor:GetSkillValue(46)) / 3;
        BaseMen = (Actor:GetSkillValue(49) + Actor:GetSkillValue(51) + Actor:GetSkillValue(47)) / 3;
");

INSERT INTO math_scripts VALUES( "StaminaRatioWalk", 
"PStaminaRate = Actor:MaxPStamina/100*BaseRegenPhysical;
MStaminaRate = Actor:MaxMStamina/100*BaseRegenMental;");

INSERT INTO math_scripts VALUES( "StaminaRatioStill", 
"PStaminaRate = Actor:MaxPStamina/100*BaseRegenPhysical;
MStaminaRate = Actor:MaxMStamina/100*BaseRegenMental;");

INSERT INTO math_scripts VALUES( "StaminaRatioSit", 
"PStaminaRate = Actor:MaxPStamina*0.015*BaseRegenPhysical;
MStaminaRate = Actor:MaxMStamina*0.015*BaseRegenMental;");

INSERT INTO math_scripts VALUES( "StaminaRatioWork", 
"PStaminaRate = BaseRegenPhysical-6.0*(100-SkillMentalFactor)/100;
MStaminaRate = BaseRegenMental-6.0*(100-SkillMentalFactor)/100;");

INSERT INTO math_scripts VALUES( "CalculateMaxHP", "MaxHP = Actor:GetSkillValue(51) + Actor:GetSkillValue(46) + Actor:GetSkillValue(50);");

INSERT INTO math_scripts VALUES( "CalculateMaxMana", "MaxMana = Actor:GetSkillValue(51) + Actor:GetSkillValue(49);");

INSERT INTO math_scripts VALUES( "LootModifierCostCap", "ModCap = MaxHP*10;");

INSERT INTO math_scripts VALUES( "Calculate Repair Rank","Result = if(Object:SalePrice > 300,Object:SalePrice/150,0);");

INSERT INTO math_scripts VALUES( "Calculate Repair Time",
"
        Result = Object:SalePrice/100;
        Factor = Worker:GetSkillValue(Object:RequiredRepairSkill) / (Object:SalePrice/20);
        Result = Result / Factor;
        Result = if(Result < 20, 20, Result);
");

INSERT INTO math_scripts VALUES( "Calculate Repair Result",
"
        Factor = Worker:GetSkillValue(Object:RequiredRepairSkill) / (Object:SalePrice/20);
        Result = ((Object:SalePrice/25) * Factor) * (rnd(1)+0.5);
");

INSERT INTO math_scripts VALUES( "Calculate Repair Quality",
"
ResultQ = if(Object:Quality+RepairAmount > Object:MaxQuality, Object:MaxQuality, Object:Quality+RepairAmount);
ResultMaxQ = Object:MaxQuality-(ResultQ-Object:Quality)*0.2;
ResultMaxQ = if(ResultMaxQ < 0, 0, ResultMaxQ);
ResultQ = if(ResultQ > ResultMaxQ, ResultMaxQ, ResultQ);
");

INSERT INTO math_scripts VALUES( "Calculate Repair Experience",
"
ResultPractice = 1;
ResultModifier = RepairAmount;
");

INSERT INTO math_scripts VALUES( "Calculate Lockpicking Experience",
"
ResultPractice = 1;
ResultModifier = 1;
");


INSERT INTO math_scripts VALUES( "Calculate Mining Experience",
"
ResultPractice = if(Success, 1, 0);
ResultModifier = if(Success, 25, 2);
");


INSERT INTO math_scripts VALUES( "Calculate Skill Experience",
"
Exp = PracticePoints*Modifier;
");

INSERT INTO math_scripts VALUES( "CalculateFamiliarAffinity", "Affinity = Type + Lifecycle + AttackTool + AttackType;");

INSERT INTO math_scripts VALUES( "CalculatePetDepletedLockoutTime", "DepletedLockoutTime = 10.0 * if(Skill,Skill,1);");
INSERT INTO math_scripts VALUES( "CalculatePetDismissLockoutTime", "DismissLockoutTime = 10.0 * if(Skill,Skill,1);");
INSERT INTO math_scripts VALUES( "CalculateMaxPetTime", "MaxTime = 5 * 60 * if(Skill,Skill,1);");
INSERT INTO math_scripts VALUES( "CalculatePetDeathLockoutTime", "DeathLockoutTime = 10.0 * if(Skill,Skill,1);");
INSERT INTO math_scripts VALUES( "CalculatePetTrainingLockoutTime", "TrainingLockoutTime = 10.0 * if(Skill,Skill,1);");
INSERT INTO math_scripts VALUES( "CalculateMaxPetRange", "MaxRange = 10 + Skill*10;" );
INSERT INTO math_scripts VALUES( "CalculatePetReact"," React = if(1+Skill>=Level,1,0);");

INSERT INTO math_scripts VALUES( "Calc Player Sketch Limits",
"
	IconScore = Actor:getSkillValue(64) + 5;
	PrimCount = Actor:getSkillValue(64) + 20;
");

INSERT INTO math_scripts VALUES( "Calc Item Price", "FinalPrice = Price*(Quality/MaxQuality);");

INSERT INTO math_scripts VALUES( "Calc Item Sell Price", "FinalPrice = Price * 0.8;");

INSERT INTO math_scripts VALUES( "Calc Guild Account Level", "AccountLevel = 1 + log(log(TotalTrias));");

INSERT INTO math_scripts VALUES( "Calc Char Account Level", "AccountLevel = log(log(TotalTrias));");

INSERT INTO math_scripts VALUES( "Calc Bank Fee", "BankFee = 5.25 - (AccountLevel * 0.25);");

INSERT INTO math_scripts VALUES( "Calculate Mining Odds", "
Skill = PlayerSkill / RequiredSkill;
Quality = ToolQuality / RequiredToolQuality;

Total = Distance * Probability * Quality * Skill + 0.3;

ResultQuality = ToolQuality * Skill;
ResultQuality = ResultQuality*300;

");

INSERT INTO math_scripts VALUES( "Calc Item Merchant Price Buy", "Result = ItemPrice - CharData:GetSkillValue(47)/10;");

INSERT INTO math_scripts VALUES( "Calc Item Merchant Price Sell", "Result = ItemPrice + CharData:GetSkillValue(47)/10;");

INSERT INTO math_scripts VALUES( "Calculate Dynamic Experience", "Exp = 0;");

INSERT INTO math_scripts VALUES( "Calculate Transformation Apply Skill",
"
// just return for processless transforms
if(!Process:IsValid())
{
    if(Secure)
    {
        Worker:SendSystemInfo('Processless transforms gives no quality change.',0);
    }
    exit = 1;
}

PriSkill = Process:PrimarySkillId;
PriPoints = Process:PrimarySkillPracticePoints;

// add here quality logic

Quality = 100;

if(Secure)
{
    Worker:SendSystemInfo('Event took %f seconds.', 1, CalculatedTime);
}

" );

INSERT INTO math_scripts VALUES( "Calculate Transformation Time",
"if(Transform:ItemQuantity & Transform:ItemID != 0 & Transform:ResultItemID != 0)
{
    Time = Transform:TransformPoints * (0.9 + Object:StackCount * 0.1);
}
else
{
    Time = Transform:TransformPoints;
}");

INSERT INTO math_scripts VALUES( "Calculate Transformation Experience",
"
Exp = if(StartQuality < CurrentQuality, 2*(CurrentQuality-StartQuality), 0);
");

INSERT INTO math_scripts VALUES( "Calculate Transformation Practice",
"
    PriPoints = Process:PrimarySkillPracticePoints;
    PriPoints = PriPoints*CalculatedTime/4;

    SecSkill = Process:SecondarySkillId;
    if(SecSkill > 0) {
      SecPoints = Process:SecondarySkillPracticePoints;
      SecPoints = SecPoints*CalculatedTime/4;
    } else {
      SecPoints = 0;
    }
");


INSERT INTO math_scripts VALUES( "CalculateDodgeValue" , "
	LightRes = (LightPoints * Actor:GetSkillValue(7));
	MediumRes = (MediumPoints * Actor:GetSkillValue(8));
	HeavyRes = (HeavyPoints * Actor:GetSkillValue(9));

	Result = LightRes + MediumRes + HeavyRes;
	 
	if(Result = 0)
	{
		 Result = 0.2;
	}
");

INSERT INTO math_scripts VALUES( "PracticeArmorSkills" , "
	if(LightPoints > 0)
	{
		Actor:PracticeSkillID(7,LightPoints);
	}
	if(MediumPoints > 0)
	{
		Actor:PracticeSkillID(8,MediumPoints);
	}
	if(HeavyPoints > 0)
	{
		Actor:PracticeSkillID(9,HeavyPoints);
	}
");

INSERT INTO math_scripts VALUES( "SetBaseSkills" , "
	if((AGI > 0) & (Actor:GetSkillValue(46) < AGI))
	{
		Actor:SetSkillValue(46,AGI);
	}
	if((CHA > 0) & (Actor:GetSkillValue(47) < CHA))
	{
		Actor:SetSkillValue(47,CHA);
	}
	if((END > 0) & (Actor:GetSkillValue(48) < END))
	{
		Actor:SetSkillValue(48,END);
	}
	if((INT > 0) & (Actor:GetSkillValue(49) < INT))
	{
		Actor:SetSkillValue(49,INT);
	}
	if((STR > 0) & (Actor:GetSkillValue(50) < STR))
	{
		Actor:SetSkillValue(50,STR);
	}
	if((WILL > 0) & (Actor:GetSkillValue(51) < WILL))
	{
		Actor:SetSkillValue(51,WILL);
	}
");

INSERT INTO math_scripts VALUES( "GetCharLevel" , "
    if(Physical > 0)
    {
        Result = (Actor:GetSkillValue(50)  +
                Actor:GetSkillValue(48)  +
                Actor:GetSkillValue(46)) / 3;
    }
    else
    {
        Result = (Actor:GetSkillValue(49)  +
                Actor:GetSkillValue(51) +
                Actor:GetSkillValue(47)) / 3;
    }
");

INSERT INTO math_scripts VALUES( "GetSkillValues" , "
		AGI = Actor:GetSkillValue(46);
		CHA = Actor:GetSkillValue(47);
		END = Actor:GetSkillValue(48);
		INT = Actor:GetSkillValue(49);
		STR = Actor:GetSkillValue(50);
		WIL = Actor:GetSkillValue(51);
");

INSERT INTO math_scripts VALUES( "GetSkillBaseValues" , "
		AGI = Actor:GetSkillBaseValue(46);
		CHA = Actor:GetSkillBaseValue(47);
		END = Actor:GetSkillBaseValue(48);
		INT = Actor:GetSkillBaseValue(49);
		STR = Actor:GetSkillBaseValue(50);
		WIL = Actor:GetSkillBaseValue(51);
");

INSERT INTO math_scripts VALUES( "Calculate Song Parameters" , "

    // Initializing variables
    CanPlay = 1;
    InstrSkill = 52;
    InstrSkillRank = Character:GetSkillValue(InstrSkill);

    // The minimum duration of the note the character can play gets half every 50 ranks
    CharMinimumDuration = 1000 / exp2(InstrSkillRank / 50);  // in milliseconds

    // The character unlocks the beat type 8 (16) at rank 40 (80)
    if((BeatType = 8 & InstrSkillRank < 40) | (BeatType = 16 & InstrSkillRank < 80))
    {
        CanPlay = 0;
        Character:SendSystemError('You cannot play musical scores with this meter!', 0);
    }

    // The character cannot play scores with notes too quick
    if(MinimumDuration < CharMinimumDuration)
    {
        CanPlay = 0;
        TargetTempo = floor(Tempo * MinimumDuration / CharMinimumDuration);
        Character:SendSystemError('You cannot play this song so fast! Play it at least at tempo %.0f.', 1, TargetTempo);
    }

    if(CanPlay = 0 | NNotes = 0)
    {
        ScoreRank = -1;
    }
    else
    {
        // Determining which altered notes are actually played (character learn a new note every 20 ranks)
        NAlteredNotes = 0;
        if(InstrSkillRank >= 20)
        {
            NAlteredNotes = NAlteredNotes + NBb;
        }
        else
        {
            NNotes = NNotes - NBb;
        }

        if(InstrSkillRank >= 40)
        {
            NAlteredNotes = NAlteredNotes + NGb;
        }
        else
        {
            NNotes = NNotes - NGb;
        }

        if(InstrSkillRank >= 60)
        {
            NAlteredNotes = NAlteredNotes + NEb;
        }
        else
        {
            NNotes = NNotes - NEb;
        }

        if(InstrSkillRank >= 80)
        {
            NAlteredNotes = NAlteredNotes + NDb;
        }
        else
        {
            NNotes = NNotes - NDb;
        }

        if(InstrSkillRank >= 100)
        {
            NAlteredNotes = NAlteredNotes + NAb;
        }
        else
        {
            NNotes = NNotes - NAb;
        }

        // Checking if the character can play at least one note
        if(NNotes = 0)
        {
            ScoreRank = -1;
        }
        else
        {
            // Determining difficulty due to beat type
            if(BeatType < 8)
            {
                BeatTypeDifficulty = 0;
            }
            else
            {
                BeatTypeDifficulty = 40 * BeatType / 8;
            }

            // Difficulty due to average duration is the inverted function for CharMinimumDuration
            SpeedDifficulty = log2(1000 / AverageDuration) * 50;

            // ScoreRank is in the interval (0,InstrSkillRank+50) if the score can be played
            ScoreRank = 10 * NAlteredNotes / NNotes + BeatTypeDifficulty + SpeedDifficulty;
            if(ScoreRank < 0)
            {
                ScoreRank = 0;
            }
        }
    }
");

INSERT INTO math_scripts VALUES( "Calculate Song Experience" , "
    InstrSkill = 52; // needed by the server
    SecondsPerPracticePoint = 10;

    if(ScoreRank < 0)  // the score is empty or has only rests
    {
        PracticePoints = 0;
        Modifier = 1;
        TimeLeft = 0;
    }
    else
    {
        InstrSkillRank = Character:GetSkillValue(InstrSkill);

        // No practice points if the score's rank differ from the player's skill by more than 50
        RelativeDifficulty = 1 - abs(InstrSkillRank - ScoreRank) / 50;
        if(RelativeDifficulty < 0)
        {
            RelativeDifficulty = 0;
        }

        PracticePoints = SongTime / SecondsPerPracticePoint * RelativeDifficulty;
        TimeLeft = SongTime % SecondsPerPracticePoint * RelativeDifficulty;
        Modifier = 1;
    }
");

INSERT INTO math_scripts VALUES( "DoDamageScript", "
if(Damage > Actor:MaxHP*0.3)
{
    Actor:InterruptSpellCasting()
}" );

INSERT INTO math_scripts VALUES( "Apply Post Trade Process",
"NewItem:SetItemModifier(0, OldItem:GetItemModifier(0));
NewItem:SetItemModifier(1, OldItem:GetItemModifier(1));
NewItem:SetItemModifier(2, OldItem:GetItemModifier(2));
" );

INSERT INTO math_scripts VALUES( "DiurnalNight",
"if (NPCClient:gameHour > 22 | NPCClient:gameHour < 6)
{
   Result = 1.0;
} else
{
   Result = 0.0;
}" );

INSERT INTO math_scripts VALUES( "NocturnalNight",
"if (NPCClient:gameHour > 8 & NPCClient:gameHour < 18)
{
   Result = 1.0;
} else
{
   Result = 0.0;
}" );

INSERT INTO math_scripts VALUES( "trade_enchant_gem",
"
CurrentGlyph = Worker:GetItem(0);
CurrentGlyphItem = CurrentGlyph:GetBaseItem();
CurrentGlyphId = CurrentGlyphItem:Id;
WorkItem=OldItem:GetBaseItem();
WorkItemId=WorkItem:Id;
Executed=0;
Worker:SendSystemInfo('GLYPH ID %f. Object ID: %f', 2, CurrentGlyphId, WorkItemId);

// arrow glyph
if (CurrentGlyphId=13) {
  NewItem:SetItemModifier(0,12);
  NewItem:SetItemModifier(1,14);
  NewItem:SetItemModifier(2,13);
  Worker:SendSystemInfo('Found transform with GLYPH 13', 0);
  Executed=1;
}

// cannot enchant an already enchanted gem
if (OldItem:GetItemModifier(0)) { Executed=0; }
if (OldItem:GetItemModifier(1)) { Executed=0; }
if (OldItem:GetItemModifier(2)) { Executed=0; }

// do not change quality or modifiers if nothing happened
if (Executed=0) {
  Worker:SendSystemInfo('Setting quality BACK to original %f / %f', 2, OldItem:Quality, OldItem:MaxQuality);
  NewItem:SetQuality(OldItem:Quality);
  NewItem:SetMaxQuality(OldItem:MaxQuality);
  NewItem:SetItemModifier(0, OldItem:GetItemModifier(0));
  NewItem:SetItemModifier(1, OldItem:GetItemModifier(1));
  NewItem:SetItemModifier(2, OldItem:GetItemModifier(2));
}

" );

INSERT INTO math_scripts VALUES( "trade_delete_item",
"
Worker:DeleteItem(0, 1); //
" );

INSERT INTO math_scripts VALUES( "TestAdaptivVel",
"if (Distance > 5.0 & AdaptivVelScale < 4.0)
{
   AdaptivVelScale=AdaptivVelScale + 0.1;
} else if (Distance < 1.0 & AdaptivVelScale > 0.5)
{
   AdaptivVelScale=AdaptivVelScale - 0.1;
}" );

INSERT INTO math_scripts VALUES( "CombineQuality",
"
//MaxQuality -> highest quality item (defined by the code)
//MinQuality -> lowest quality item (defined by the code)
//ItemNumber -> Total number of items
//TotalQuality -> the sum of all qualities (defined by the code)
//average quality of all items 
AverageQuality = TotalQuality/ItemNumber;
//average quality of the 
AvgExtQuality = (MaxQuality + MinQuality)/2;
Result = AverageQuality * log(1+(AvgExtQuality/AverageQuality));
Result =  min(Result, 300);
" );

INSERT INTO math_scripts VALUES( "AttackDefaultDelay", "Result = Latency * 1000;" );
INSERT INTO math_scripts VALUES( "AttackRangeDelay", "Result = Latency * 1000 + Distance * 100;" );
