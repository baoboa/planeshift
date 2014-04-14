# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'progression_events'
#

DROP TABLE IF EXISTS `progression_events`;
CREATE TABLE progression_events (
  name varchar(40) NOT NULL DEFAULT '' ,
  event_script text NOT NULL,
  PRIMARY KEY (name)
);


#
# Dumping data for table 'progression_events'
#

INSERT INTO progression_events VALUES("respawn",'<script><teleport aim="Target" location="spawn"/></script>');
INSERT INTO progression_events VALUES("go to tic tac toe",'<script><teleport aim="Target" sector="NPCroom" x="-1.69" y="2.89" z="-217.21"/></script>');
INSERT INTO progression_events VALUES("run to the hills",'<script><teleport aim="Target" sector="npcroom2"/></script>');
INSERT INTO progression_events VALUES("give_exp",'<script><exp aim="Target" value="Exp"/><msg aim="Target" text="You gained ${Exp} experience."/></script>');
INSERT INTO progression_events VALUES("SpellFailure",
    '<script>
       <fx target="Caster" name="spell_failure"/>
       <mstamina aim="Caster" value="-Spell:StaminaCost(Caster,KFactor)*0.1"/>
       <mana aim="Caster" value="-Spell:ManaCost(Caster,KFactor)*0.1"/>
    </script>');
INSERT INTO progression_events VALUES("cast Life Infusion",
    '<script>
       <hp aim="Target" value="10*Power"/>
       <msg aim="Caster" text="You transfer healing energy by your touch."/>
       <msg aim="Target" text="${Caster} healed your wounds a bit."/>
       <fx source="Caster" target="Target" name="clear" type="attached"/>
     </script>');
INSERT INTO progression_events VALUES("cast Summon Missile",
    '<script>
       <let vars="Roll = rnd(100);">
         <if t="Roll &lt; 60">
           <then>
             <hp attacker="Caster" aim="Target" value="-6*Power"/>
             <mstamina aim="Target" value="-8"/>
             <msg aim="Caster" text="You hit ${Target} with your magic arrow."/>
             <fx source="Caster" target="Target" type="unattached" name="arrow success"/>
           </then>
           <else>
             <msg aim="Caster" text="You miss ${Target} with your magic arrow."/>
             <fx source="Caster" target="Target" type="unattached" name="arrow miss"/>
          </else>
         </if>
       </let>
     </script>');
INSERT INTO progression_events VALUES("cast Swiftness",
    '<script>
       <if t="Target:ActiveSpellCount(\'Swiftness\') &lt; 1">
         <then>
           <apply aim="Target" type="buff" name="Swiftness" duration="30000">
             <agi value="10+2*Power"/>
             <msg text="You gain great agility!" undo="You return to your normal speed."/>
             <fx name="puff"/>
           </apply>
         </then>
         <else>
           <msg aim="Caster" text="${Target} is already swift!"/>
         </else>
       </if>
     </script>');
INSERT INTO progression_events VALUES("cast Hyper Shot",
'
<script>
             <hp attacker="Caster" aim="Target" value="-6*Power"/>
             <msg aim="Caster" text="You hit ${Target} with your Hyper arrow."/>
             <fx source="Caster" target="Target" type="unattached" name="arrow success"/>

</script>
');
INSERT INTO progression_events VALUES("cast LB Shot",
'
<script>
       <if t="Target = OrigTarget">
         <then>
           <fx source="Caster" target="Target" name="summon_missile" type="unattached"/>
           <hp aim="Target" value="-10*Power"/>
           <msg aim="Target" text="${Actor} hits you with a divine arrow!"/>
         </then>
         <else>
           <fx source="OrigTarget" target="Target" name="flame_burst" type="unattached"/>
           <hp aim="Target" value="-2*Power"/>
           <msg aim="Target" text="${Actor}s Divine Arrow Sends waves of energy to damage you"/>
         </else>
       </if>
     </script>
');

INSERT INTO progression_events VALUES("cast Hammer Smash",
'
<script>
       <if t="Target = OrigTarget">
         <then>
           <fx source="Caster" target="Target" name="flame_burst" type="unattached"/>
           <hp aim="Target" value="-Power/2"/>
           <msg aim="Target" text="${Actor} Pounds you with hammer smash!"/>
         </then>
         <else>
           <fx source="Caster" target="Target" name="flame_burst" type="unattached"/>
           <hp aim="Target" value="-Power/3"/>
           <msg aim="Target" text="${Actor} hammer smash hits you, flames sprout from your body!"/>
         </else>
       </if>
</script>
');
INSERT INTO progression_events VALUES("cast Defensive Wind",
    '<script>
       <apply aim="Target" type="buff" name="Defensive Wind" duration="30000">
         <def value="1+0.05*Power"/>
         <msg text="You summon wind around your body that defends you from attacks." undo="The defensive wind stops blowing."/>
         <fx name="puff"/>
       </apply>
     </script>');
INSERT INTO progression_events VALUES("cast Dispel",
    '<script>
       <msg aim="Caster" text="You successfully cast Dispel on ${Target}."/>
       <cancel aim="Target" type="all">
         <spell type="buff" name="Swiftness"/>
         <spell type="buff" name="Defensive Wind"/>
         <spell type="buff" name="Drain"/>
       </cancel>
     </script>');
INSERT INTO progression_events VALUES("cast Gust of Wind",
    '<script>
       <hp attacker="Caster" aim="Target" value="-10"/>
       <msg aim="Caster" text="You hit ${Target} with a strong gust of wind."/>
     </script>');
INSERT INTO progression_events VALUES("cast Flame Spire",
     '<script>
        <apply aim="Target" type="buff" name="Flame Spire" duration="30000">
          <msg text="A spire of flames surrounds you." undo="The spire of flames flickers out."/>
          <on type="defense">
            <let vars="Dmg = 10+@{Power};">
              <hp attacker="Defender" aim="Attacker" value="-Dmg"/>
              <msg aim="Defender" text="Your flame spire burns ${Attacker} for ${Dmg} damage!"/>
              <msg aim="Attacker" text="${Defender}&apos;s flame spire burns you for ${Dmg} damage!"/>
            </let>
          </on>
        </apply>
      </script>');
INSERT INTO progression_events VALUES("cast Magetank",
     '<script>
        <apply aim="Target" type="buff" name="Magetank" duration="30000">
          <msg text="You gain a lot of HP!" undo="Your HP returns to normal."/>
          <hp-max value="200"/>
          <hp-rate value="0.6"/>
        </apply>
      </script>');
INSERT INTO progression_events VALUES("cast Drain",
    '<script>
       <apply-linked name="Drain" duration="30000">
         <buff aim="Caster">
           <str value="10"/>
           <hp-rate value="0.3"/>
           <msg text="You drain the life of ${Target}." undo="You can no longer drain the life of ${Target}."/>
         </buff>
         <debuff aim="Target">
           <str value="-10"/>
           <hp-rate attacker="Caster" value="-0.5"/>
           <msg text="${Caster} drains your life!" undo="${Caster} can no longer drain your life."/>
         </debuff>
       </apply-linked>
     </script>');
INSERT INTO progression_events VALUES("cast Chain Lightning",
    '<script>
       <if t="Target = OrigTarget">
         <then>
           <fx source="Caster" target="Target" name="summon_missile" type="unattached"/>
           <hp aim="Target" value="-10"/>
           <msg aim="Target" text="${Caster} fries you with chain lightning!"/>
         </then>
         <else>
           <fx source="OrigTarget" target="Target" name="flame_burst" type="unattached"/>
           <hp aim="Target" value="-5"/>
           <msg aim="Target" text="${Caster} shoots chain lightning at ${OrigTarget}, which arcs and hits you!"/>
         </else>
       </if>
     </script>');

INSERT INTO progression_events VALUES("cast Identify",
    '<script>
       <let vars="Identify=Target:GetFlag(\'Identifiable\')">
       <if t="Identify = 1">
         <then>
           <msg aim="Caster" text="${Target} has to be identified!"/>
           <let vars="Identified=Target:SetFlag(\'Identifiable\',0)" />
           <msg aim="Caster" text="${Target} is now identified!"/>
         </then>
         <else>
           <msg aim="Caster" text="${Target} does NOT need identify!"/>
         </else>
       </if>
       </let>
     </script>');

INSERT INTO progression_events VALUES("cast Fear",
    '<script>
       <msg aim="Caster" text="You create an horrible dome of fear around ${Target}."/>
       <apply aim="Target" name="Fear" type="debuff" duration="3000*Power">
         <atk value="0.7"/>
         <def value="0.7"/>
         <wil value="-Power"/>
       </apply>
       <if t="Target:IsNPC = 1">
         <then>
           <npccmd aim="Target" cmd="npccmd:self:flee"/>
         </then>
       </if>
     </script>');

INSERT INTO progression_events VALUES("cast Charme",
    '<script>
     <if t="Caster:ActiveSpellCount(\'Charme\') &lt; 1">
      <then>
       <let vars="Duration=24;">
        <if t="Target:IsNPC">
          <then>
            <apply-linked name="Charme" duration="1000*Duration">
              <buff aim="Caster">
                <msg aim="Caster" text="You charmed ${Target}&apos;s mind." undo="You can no longer affect ${Target}&apos;s mind." />
              </buff>
              <debuff aim="Target">
                <msg text="${Caster} bound your mind." undo="${Caster} can no longer bind your mind."/>
                <!-- Send a command to change the target brain so it can be undone later. -->
                <npccmd aim="Target" cmd="npccmd:self:charme" undo="npccmd:self:reload" />
                <!-- Add the target to the list of pets for "duration" time -->
                <add_pet owner="Caster" />
              </debuff>
            </apply-linked>
          </then>
          <else>
            <msg aim="Caster" text="You cannot affect ${Target}&apos;s mind with this spell."/>
          </else>
        </if>
      </let>
     </then>
     <else>
      <msg aim="Caster" text="${Target} is already affected by Charme or you&apos;re already bound to another target."/>
     </else>
    </if>
   </script>');

# Char creation events - god
INSERT INTO progression_events VALUES ('charcreate_2','<script><skill aim="Actor" name="Crystal Way" value="2"/><int  aim="Actor" value="3"/><animal-affinity aim="Actor" name="daylight" value="2"/></script>');

# Char creation events - life
INSERT INTO progression_events VALUES ('charcreate_3','<script><str aim="Actor" value="3"/></script>');
INSERT INTO progression_events VALUES ('charcreate_6','<script><wil aim="Actor" value="4"/><cha aim="Actor" value="-4"/></script>');
INSERT INTO progression_events VALUES ('charcreate_7','<script><wil aim="Actor" value="-1"/><cha aim="Actor" value="1"/></script>');

# Char creation events - parents
INSERT INTO progression_events VALUES ('charcreate_9','<script><agi aim="Actor" value="4*ParentStatus"/><skill aim="Actor" name="Alchemy" value="4*ParentStatus"/><animal-affinity aim="Actor" name="acid" value="8"/></script>');

# Char creation events - zodiacs
INSERT INTO progression_events VALUES ('charcreate_215','<script><wil aim="Actor" value="10"/></script>');
INSERT INTO progression_events VALUES ('charcreate_216','<script><agi aim="Actor" value="20"/></script>');
INSERT INTO progression_events VALUES ('charcreate_217','<script><str aim="Actor" value="-12"/></script>');
INSERT INTO progression_events VALUES ('charcreate_218','<script><end aim="Actor" value="10"/></script>');
INSERT INTO progression_events VALUES ('charcreate_219','<script><int aim="Actor" value="10"/></script>');
INSERT INTO progression_events VALUES ('charcreate_220','<script><cha aim="Actor" value="10"/></script>');
INSERT INTO progression_events VALUES ('charcreate_221','<script><str aim="Actor" value="10"/></script>');
INSERT INTO progression_events VALUES ('charcreate_222','<script><end aim="Actor" value="10"/></script>');
INSERT INTO progression_events VALUES ('charcreate_223','<script><int aim="Actor" value="10"/></script>');
INSERT INTO progression_events VALUES ('charcreate_224','<script><cha aim="Actor" value="10"/></script>');



INSERT INTO progression_events VALUES("drink_potion",'<script><mstamina aim="Actor" value="10+(Quality/20)"/><apply aim="Actor" type="buff" name="potion Buff" duration="240000"><end value="-6"/><skill name="Brown Way" value="2+(Quality/20)"/><msg text="You drink the potion and feel weird."/></apply></script>');
INSERT INTO progression_events VALUES("drink_poison",'<script><hp aim="Actor" value="-20"/><msg aim="Actor" text="You drink some poison and feel bad."/></script>');
INSERT INTO progression_events VALUES("BuffPotionWILL",'<script><apply aim="Actor" type="buff" name="Will Potion" duration="120000"><wil value="5"/><msg text="You drink a will potion." undo="The will potion wears off."/></apply></script>');
INSERT INTO progression_events VALUES('drink_speed_potion','<script><msg aim="Actor" text="drink_speed_potion not implemented. I know, it makes me sad too."/></script>');
INSERT INTO progression_events VALUES('drink_strange_potion','<script><msg aim="Actor" text="drink_strange_potion not implemented."/></script>');
INSERT INTO progression_events VALUES('create_familiar','<script><create-familiar aim="Target"/></script>');
INSERT INTO progression_events VALUES('fire_damage','<script><hp aim="Actor" value="-6"/><msg aim="Actor" text="You touch the ingots and your hand is burned!"/></script>');
INSERT INTO progression_events VALUES('healing_tree','<script><hp aim="Actor" value="6"/><msg aim="Actor" text="Walking near the tree you feel refreshed!"/></script>');
INSERT INTO progression_events VALUES('morph_ulbernaut','<script><apply aim="Actor" type="buff" name="Ulberform" duration="60000"><race value="ulbernaut" sex="n"/><msg text="You turn into a lumbering giant!" undo="Your ulbernaut form wears off."/></apply></script>');
INSERT INTO progression_events VALUES('usevariables','<script><let vars="Dmg = Actor:GetVariableValueInt(\'Myvariable\')"><hp aim="Actor" value="Dmg"/><msg aim="Actor" text="You have been healed ${Dmg} points as the variable says. If zero, equip a sword of variables!"/></let></script>');
INSERT INTO progression_events VALUES('rain','<script><msg aim="Actor" text="rain not implemented."/></script>');
-- I don't think we can do the original Nitroglycerin effect anymore. :(
-- We'd need to have consume scripts list an AOE spell rather than a script directly.
INSERT INTO progression_events VALUES("explosion",'<script><msg aim="Actor" text="The bottle explodes and kills you!"/><hp aim="Actor" value="-Actor:HP"/></script>');
INSERT INTO progression_events VALUES("food",'<script><msg aim="Actor" text="You eat and you feel better!"/><hp aim="Actor" value="20"/></script>');
INSERT INTO progression_events VALUES('ResearchSpellSuccess','<script><msg aim="Actor" text="You have discovered a new spell!"/></script>');
INSERT INTO progression_events VALUES('ResearchSpellFailure','<script><msg aim="Actor" text="You fail to discover a new spell."/></script>');
INSERT INTO progression_events VALUES('drop_marker','<script><fx target="Target" type="attached" name="shadow"/><msg aim="Actor" text="Spot Marked!"/></script>');
INSERT INTO progression_events VALUES('death_penalty',
    '<script>
       <apply aim="Actor" type="debuff" name="-Death Realm Curse" duration="100000">
         <agi value="-0.5*Actor:GetSkillValue(46)"/>
         <end value="-0.5*Actor:GetSkillValue(48)"/>
         <str value="-0.5*Actor:GetSkillValue(50)"/>
         <cha value="-0.5*Actor:GetSkillValue(47)"/>
         <int value="-0.5*Actor:GetSkillValue(49)"/>
         <wil value="-0.5*Actor:GetSkillValue(51)"/>
         <msg text="You fell the curse of the death realm upon you." undo="You feel the curse of death lift."/>
       </apply>
     </script>');

INSERT INTO progression_events VALUES('PATH_Street Warrior',
    '<script>
       <str aim="Actor" value="0.35*CharPoints"/>
       <end aim="Actor" value="0.25*CharPoints"/>
       <agi aim="Actor" value="0.25*CharPoints"/>
       <int aim="Actor" value="0.05*CharPoints"/>
       <wil aim="Actor" value="0.05*CharPoints"/>
       <cha aim="Actor" value="0.05*CharPoints"/>
       <skill aim="Actor" name="Mace &amp; Hammer" value="1"/>
       <skill aim="Actor" name="Medium Armor"      value="1"/>
       <skill aim="Actor" name="Pickpockets"       value="1"/>
       <skill aim="Actor" name="Find Traps"        value="1"/>
       <skill aim="Actor" name="Body Development"  value="2"/>
     </script>');

INSERT INTO progression_events VALUES('minigame_win', '<script><item aim="Target" name="Tria" count="50"/><exp aim="Target" value="100"/><msg aim="Target" text="You won 50 tria and 100 experience points!"/></script>');

INSERT INTO progression_events VALUES('explore_area',
    '<script>
       <if t="Target:IsWithin(Range, NPC:loc_x, NPC:loc_y, NPC:loc_z, NPC:sector) = 1">
         <then>
           <if t="Target:HasExploredArea(NPC:PID) = 0">
             <then>
               <msg aim="Target" text="You have discovered ${Area} and gained ${Exp} experience!"/>
               <exp aim="Target" value="Exp"/>
             </then>
           </if>
         </then>
       </if>
     </script>');

INSERT INTO progression_events VALUES("vegeta",
    '<script>
       <if t="Target:IsEnemy(NPC:owner) = 1">
         <then>
           <if t="Target:IsWithin(5, NPC:loc_x, NPC:loc_y, NPC:loc_z, NPC:sector) = 1">
             <then>
               <hp aim="Target" value="-9001"/>
               <msg aim="Target" type="error" text="You have stepped on a trap! IT\'S OVER NINE THOUSAAAAANDD!!"/>
               <destroy aim="NPC"/>
             </then>
           </if>
         </then>
       </if>
     </script>');

INSERT INTO progression_events VALUES("send_tutorial_msg",
    '<script><let vars="TipNum=Param0"><tutorialmsg aim="Target" num="TipNum"/></let></script>');

INSERT INTO progression_events VALUES("mechanism",
    '<script><let vars="Mesh=Param0;Move=Param1;Rot=Param2"><mechanism aim="Target" mesh="Mesh" move="Move" rot="Rot" /></let></script>');

