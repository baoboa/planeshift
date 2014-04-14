DROP TABLE IF EXISTS `sc_npctypes`;
CREATE TABLE  `sc_npctypes` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT COMMENT 'An unique id for this row',
  `name` varchar(200) NOT NULL COMMENT 'The name of this npctype',
  `parents` varchar(200) NOT NULL COMMENT 'The parents of this npctype for inheritance',
  `ang_vel` float DEFAULT '999' COMMENT 'Angular speed of the npctype',
  `vel` varchar(200) DEFAULT '999' COMMENT 'Speed of the npctype',
  `collision` varchar(200) DEFAULT NULL COMMENT 'Perception when colliding',
  `out_of_bounds` varchar(200) DEFAULT NULL COMMENT 'Perception when out of bounds',
  `in_bounds` varchar(200) DEFAULT NULL COMMENT 'Perception when in bounds',
  `falling` varchar(200) DEFAULT NULL COMMENT 'Perception when falling',
  `template` int(1) unsigned NOT NULL default '0' COMMENT 'true if this is a base behavior for other behaviors. Used by WC.',
  `script` text NOT NULL COMMENT 'The script of this npctype',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Stores the list of npc types';

#
# Dumping data for table 'sc_npctypes'
#
INSERT INTO sc_npctypes VALUES("1","DoNothing","",0,"","","","","","1",
'<behavior name="DoNothing" decay="0" growth="0" initial="50">
   <wait duration="1" anim="stand" />
</behavior>');

INSERT INTO sc_npctypes VALUES("2","GoHomeOnTeleport","",0,"","","","","","1",
'<behavior name="Teleported" completion_decay="-1" growth="0" initial="0">
   <locate obj="perception" />
   <teleport />                              <!-- Teleport to located position --> 
   <locate obj="waypoint"  static="no" />    <!-- Locate nearest waypoint -->
   <navigate anim="walk" />                  <!-- Local navigation -->
   <locate obj="home:$name" />
   <wander anim="walk" />                    <!-- Navigate using waypoints -->
   <navigate anim="walk" />                  <!-- Local navigation -->
</behavior>

<react event="teleported" behavior="Teleported" />');

INSERT INTO sc_npctypes VALUES("3","InRegion","",0,"","","out of bounds","","","1",
'<behavior name="GoToRegion" completion_decay="-1" resume="yes" initial="0">
   <locate obj="region" destination="Move"/>
   <percept event="move" />      <!-- navigate, wander, navigate -->
</behavior>
<react event="GoToRegion" behavior="GoToRegion" />

<behavior name="MoveBackInRegion" completion_decay="-1" >
   <!-- Move back into the region -->
   <locate obj="region" destination="Move" />
   <percept event="local_move" />      <!-- Local navigation -->
</behavior>

<behavior name="FailedEndpoint" completion_decay="-1" >
   <talk text="I failed to reach $target going away in region" target="false" />
   <percept event="move back in region" />
</behavior>

<behavior name="RandomMoveInRegion" completion_decay="0" loop="yes" resume="yes">
    <wait duration="15" random="5" />
    <locate obj="region" range="20" random="yes" destination="Move" />
    <percept event="local_move" />      <!-- Local navigation -->
</behavior> 

<react event="move back in region" behavior="MoveBackInRegion" do_not_interrupt="GoToRegion,Move" />
<react event="out of bounds"       behavior="MoveBackInRegion" do_not_interrupt="GoToRegion,Move" />
<!-- In case of collision run MoveBackInRegion to select a new random point in the region -->
<react event="collision"           behavior="MoveBackInRegion" />');

INSERT INTO sc_npctypes VALUES("4","Fight","",0,"","","","","","1",
'<behavior name="Fight"   initial="0" growth="0" decay="0" completion_decay="-1" >
   <locate obj="target"  range="20" />
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <melee seek_range="25" melee_range="2" />
</behavior>

<behavior name="Chase" initial="0" growth="0" decay="1" completion_decay="-1" >
   <chase type="target" chase_range="25" offset="0.5" offset_angle="20" anim="run" vel="$run" />
</behavior>

<behavior name="FailedToAttack" completion_decay="-1" resume="yes" >
   <locate obj="point" range="30" destination="Move" />
   <percept event="local_move" />      <!-- Local navigation -->
</behavior> 

<behavior name="FailedEndpoint" completion_decay="-1" resume="yes" >
   <locate obj="point" range="30" destination="Move" />
   <percept event="local_move" />      <!-- Local navigation -->
</behavior> 

<behavior name="Flee" decay="1" completion_decay="-1" resume="yes" >
   <emote cmd="/flee" />
   <loop iterations="3">
      <locate obj="point" range="4" destination="Move" />
      <percept event="local_move" />   <!-- Local navigation to random point -->
   </loop>
</behavior>

<!-- when Charme spell affects the NPC change its brain to Minion -->
<behavior name="Charme" completion_decay="-1" >
   <change_brain brain="Minion" />
</behavior>

<react event="fight"               behavior="Fight" />
<react event="attack"              behavior="Fight" weight="1" /> <!-- Add 1 to hate list -->
<react event="damage"              behavior="Fight" delta="20" weight="2" /> <!-- Add double damage to hate list -->
<react event="target out of range" behavior="Chase" />
<react event="failed to attack"    behavior="FailedToAttack" />

<react event="failed endpoint"     behavior="FailedEndpoint" /> <!-- When move do not reach endpoint. -->
<react event="npccmd:self:charme"  behavior="Charme" /> <!-- change behaviour when Charme spell is cast. -->
<react event="npccmd:self:flee"    behavior="Flee" /> <!-- flee when Fear spell is cast. -->
<react event="flee"                behavior="Flee" /> <!-- flee when flee is percepted. -->

<!-- Stop chas if target is out of chase range -->
<react event="target out of chase" behavior="Chase" absolute="0" only_interrupt="Chase"/>

<react event="death"               behavior="Fight" absolute="0" />');

INSERT INTO sc_npctypes VALUES("5","TurnToSensedPlayer","",0,"","","","","","0",
'<behavior name="TurnToFacePlayer" completion_decay="-1" growth="0" initial="0">
   <locate obj="perception" />
   <rotate type="locatedest" anim="walk" ang_vel="60" />
</behavior>

<react event="player sensed"       behavior="TurnToFacePlayer" delta="100" inactive_only="yes" />');

INSERT INTO sc_npctypes VALUES("6","Fighter","DoNothing,InRegion,Fight",0,"","","","","","0",
'<behavior name="Init" completion_decay="-1" initial="1000" >
  <percept event="GoToRegion" />
</behavior>');

INSERT INTO sc_npctypes VALUES("7","Wanderer","Fighter",0,"","","","","","0",
'<behavior name="Walk" decay="0" growth="0" initial="75">
   <loop>
      <locate obj="region" static="no" />                    <!-- Locate random point within region -->
      <rotate type="locatedest" anim="walk" ang_vel="10" />  <!-- Rotate to fase last located destination -->
      <navigate anim="walk" />                               <!-- Local navigation -->
      <wait duration="5" anim="stand" />
   </loop>
</behavior>');


INSERT INTO sc_npctypes VALUES("8","On Sight Fighter","TurnToSensedPlayer,Fighter",0,"","","","","","1",
'<behavior name="FightDefensive"   initial="0" growth="0" decay="0" completion_decay="-1" >
   <locate obj="target"  range="20" />
   <talk text="$race attacked by $target" target="false" /> 
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <melee seek_range="20" melee_range="3" stance="defensive" />
</behavior>

<!-- Ask server for an assessment. Using target to make sure the assessment match -->
<behavior name="AssessFighter" completion_decay="-1" >
   <assess overall="$target" />
</behavior>

<!-- Handle different assessment in different ways -->
<behavior name="AssessLame" >
   <talk text="$target is a lame target" target="false" />
   <percept event="fight" />
</behavior>
<behavior name="AssessWeaker" >
   <talk text="$target is a weaker target" target="false" />
   <percept event="fight" />
</behavior>
<behavior name="AssessEqual" >
   <talk text="$target is a equal target" target="false" />
   <percept event="fight" />
</behavior>
<behavior name="AssessStronger" >
   <talk text="$target is a stronger target" target="false" />
   <percept event="fight defensive" />
</behavior>
<behavior name="AssessPowerfull" >
   <talk text="$target is a powerfull target" target="false" />
   <percept event="fight defensive" />
</behavior>

<!-- Reacting to players nearby(<10) and adjacent -->
<react event="player nearby"       behavior="AssessFighter" delta="100" inactive_only="yes" faction_diff="-200" oper=">"  />
<react event="player adjacent"     behavior="AssessFighter" delta="100" inactive_only="yes" faction_diff="-200" oper=">"  />

<react event="fight defensive"     behavior="FightDefensive" />

<!-- Reaction to results of assessments -->
<react event="assess extremely weaker"    type="$target" behavior="AssessLame" />
<react event="assess much weaker"         type="$target" behavior="AssessWeaker" />
<react event="assess weaker"              type="$target" behavior="AssessWeaker" />
<react event="assess equal"               type="$target" behavior="AssessEqual" />
<react event="assess stronger"            type="$target" behavior="AssessStronger" />
<react event="assess much stronger"       type="$target" behavior="AssessStronger" />
<react event="assess extremely stronger"  type="$target" behavior="AssessPowerfull" />');


INSERT INTO sc_npctypes VALUES("9","Answerer","DoNothing",0,"","","","","","1",
'<behavior name="turn to face" completion_decay="-1" growth="0" initial="0" >
   <locate obj="perception" />
   <rotate type="locatedest" anim="walk" ang_vel="20" />
   <emote cmd="/greet" />
   <wait duration="10" anim="stand" />
</behavior>

<react event="talk" inactive_only="yes" faction_diff="-100" oper=">" behavior="turn to face" delta="100"  when_invisible="yes" when_invincible="yes" />');

INSERT INTO sc_npctypes VALUES("10","PoliteSitting","",0,"","","","","","1",
'<behavior name="Init" completion_decay="-1" initial="1000" >
  <sit />
</behavior>

<behavior name="Stand" completion_decay="-1" growth="0" initial="0" >
   <standup />
   <wait duration="10" />
   <sit />
</behavior>
<react event="stand" behavior="Stand" />
<react event="player adjacent" inactive_only="yes" behavior="Stand"  when_invisible="yes" when_invincible="yes" />');

INSERT INTO sc_npctypes VALUES("11","Move","",0,"","","","","","1",
'<!-- Fail safe move operation. Target position taken from the "Move" locate.  -->
<!-- Will move to the nearest waypoint. Than to the waypoint next to          -->
<!-- destination. Than local navigation will be used to the destination.      -->
<!-- If enything goes wrong the NPC will be teleported to the destination and -->
<!-- the calling operation will not be terminated, allowing that to complete  -->

<!-- Example usage of this:                                                   -->

<!--      <locate obj="something" destination="Move" />                       -->
<!--      <percept event="move" />                                            -->

<!-- The operation that clean up everyting if something goes wrong -->
<behavior name="MoveFailed" complection_decay="-1" >
   <!-- Something went wrong. Now just make sure we get to the intended location. -->
   <copy_locate source="Move" destination="Active" />
   <talk text="I failed to move, will teleport in 5 sec." target="false" />
   <wait anim="idle" duration="5" />
   <teleport />
   <talk text="I was teleported here, waiting 5 sec." target="false" />
   <wait anim="idle" duration="5" />
</behavior>

<!-- Normal execution of this operation -->
<behavior name="Move" complection_decay="-1" resume="yes" failure="move_failed" >
   <!-- First locate nearest waypoint and go there-->
   <locate obj="waypoint" static="no" failure="move_failed" />
   <navigate anim="walk" failure="move_failed" />
   
   <!-- Now find the final target and move there -->
   <copy_locate source="Move" destination="Active" />
   <wander anim="walk"  failure="move_failed" />
   <navigate anim="walk" failure="move_failed" />   
</behavior>

<!-- Global movement with use of waypoint navigation -->
<behavior name="GlobalMove" complection_decay="-1" resume="yes" failure="move_failed" >
   <!-- Find the target and move there -->
   <copy_locate source="Move" destination="Active" />
   <wander anim="walk" private="false" failure="move_failed" />
</behavior>

<!-- Local movement without the waypoint navigation -->
<behavior name="LocalMove" complection_decay="-1" resume="yes" failure="move_failed" >
   <!-- Find the final target and move there -->
   <copy_locate source="Move" destination="Active" />
   <navigate anim="walk" failure="move_failed" />   
</behavior>

<react event="move"        behavior="Move" />
<react event="local_move"  behavior="LocalMove" />
<react event="global_move" behavior="GlobalMove" />

<react event="move_failed" behavior="MoveFailed" />');


INSERT INTO sc_npctypes VALUES("12","SpokenTo","",0,"","","","","","1",
'<behavior name="SpokenTo" loop="Yes" interrupt="spoken_to_interrupted">
   <wait duration="10" />
 </behavior>

 <!-- Make sure we just keep doing this while SpokenTo -->
 <react event="spoken_to_interrupted" behavior="SpokenTo" />

 <!-- Perception from server to block NPC while spoken to -->
 <react event="spoken_to" type="true" behavior="SpokenTo" />
 <react event="spoken_to" type="false" behavior="SpokenTo" absolute="0" />
');

INSERT INTO sc_npctypes VALUES("13","Sleep","",0,"","","","","","1",
'<!-- Require Idle behavior when composed into a complete behavior -->
<!--  Activate this behavior by <percept event="sleep" />

<!-- Create a behavior that cause the npc to go visible when going from Sleep -->
<behavior name="WokenFromSleep" completion_decay="-1" >
   <visible/>
   <wait duration="1" anim="stand" />
</behavior>

<!-- While this behavior is not interrupted the NPC will keep sleeping. 
     Just start another behavior when done sleeping, and the NPC will
     automatically become visible and perform the task. -->
<behavior name="AtSleep" loop="yes" completion_decay="0" interrupt="woken_from_sleep" >
   <invisible/>
   <loop>
      <wait duration="2.0" />
  </loop>
</behavior>
<react event="woken_from_sleep" behavior="AtSleep" absolute="0" />
<react event="woken_from_sleep" behavior="WokenFromSleep" />

<!-- External reactions. Use <percept event="sleep" to go to sleep. -->
<react event="sleep" behavior="AtSleep" />
');

INSERT INTO sc_npctypes VALUES("14","Diurnal","Sleep,InRegion",0,"$run","","","","","1",
'<!-- Active by day. Sleeping/Hiding during night 22:00-06:00 -->
<!--  Require Idle behavior when composed into a complete behavior -->

<behavior name="GoToSleep" resume="yes" interrupt="wakeup">
   <!-- Locate a location of type sleeping_area and move there  -->
   <locate obj="sleeping_area" range="60" random="yes" static="yes" destination="Move" />
   <percept event="move" />
   <invisible/>
   <wait duration="1" />
   <percept event="sleep" />
</behavior>
<react event="GoToSleep" behavior="GoToSleep" />

<behavior name="Init_Diurnal" resume="yes" completion_decay="-1" initial="1000">
   <visible />
   <wait duration="1" />
   <percept condition="DiurnalNight" event="GoToSleep" failed_event="GoToRegion" />
   <percept event="StartRandomMove" />
</behavior>
<react event="StartRandomMove" behavior="RandomMoveInRegion" absolute="60" />


<react event="time" value="22,0,,," random=",5,,," behavior="GoToSleep" /> 
<react event="time" value="6,0,,," random=",5,,,"  behavior="GoToRegion" /> 
');

INSERT INTO sc_npctypes VALUES("15","Nocturnal","Sleep,InRegion",0,"$walk","","","","","1",
'<!-- Moving about during night. Sleeping/Hiding during day 08:00-18:00 -->
<!--  Require Idle behavior when composed into a complete behavior -->

<behavior name="GoToSleep" resume="yes" interrupt="wakeup">
   <locate obj="sleeping_area" range="60" random="yes" static="yes" destination="Move" />
   <percept event="move" />
   <invisible/>
   <wait duration="1" />
   <percept event="sleep" />
</behavior>
<react event="GoToSleep" behavior="GoToSleep" />

<behavior name="Init_Nocturnal" resume="yes" completion_decay="-1" initial="1000">
   <visible />
   <wait duration="1" />
   <percept condition="NocturnalNight" event="GoToSleep" failed_event="GoToRegion" />
   <percept event="StartRandomMove" />
</behavior>
<react event="StartRandomMove" behavior="RandomMoveInRegion" absolute="60" />


<react event="time" value="8,0,,," random=",5,,," behavior="GoToSleep" /> 
<react event="time" value="18,0,,," random=",5,,,"  behavior="GoToRegion" /> 
');

INSERT INTO sc_npctypes VALUES("16","Follow","",0,"","","","","","1",
'<!-- Follow basic operation -->
<behavior name="follow" completion_decay="100" decay="0" growth="0" initial="0" loop="yes">
   <watch type="owner" range="3.0" />
</behavior>
<behavior name="follow_chase" completion_decay="100" growth="0" initial="0">
   <chase type="owner" anim="walk" collision="follow_collided" offset="1.0+$race_size" />
</behavior>
<behavior name="follow_turn" completion_decay="100" growth="0" initial="0">
   <rotate type="random" min="90" max="270" anim="walk" ang_vel="30" />
   <move vel="2" anim="walk" duration="1.0"/>
</behavior>

<react event="follow_collided"      behavior="follow_turn"  delta=  "100" />
<react event="owner out of range"   behavior="follow_chase"  />
');

INSERT INTO sc_npctypes VALUES("100","Smith","GoHomeOnTeleport,DoNothing",0,"","","","","","0",
'<behavior name="go_climbing1" initial="0" completion_decay="20" loop="no"> 
   <moveto x="-53.6003" y="0.0" z="-155.041" anim="walk" />
   <moveto x="-8.89576" y="0.0" z="-162.498" anim="walk" />
   <moveto x="18.4303" y="21.9941" z="-163.082" anim="walk" />
   <moveto x="-9.01569" y="0.0" z="-159.477" anim="walk" />
   <moveto x="-19.7409" y="0.0" z="-153.539" anim="walk" />
   <moveto x="-42.8985" y="0.0" z="-146.811" anim="walk" />
   <moveto x="-53.6242" y="0.0" z="-155.023" anim="walk" />
   <wait duration="60" anim="stand" />
   <moveto x="-53.6242" y="0.0" z="-155.023" anim="walk" />
   <moveto x="-42.8985" y="0.0" z="-146.811" anim="walk" />
   <moveto x="-19.7409" y="0.0" z="-153.539" anim="walk" />
   <moveto x="-9.01569" y="0.0" z="-159.477" anim="walk" />
   <moveto x="18.4303" y="21.9941" z="-163.082" anim="walk" />
   <moveto x="-8.89576" y="0.0" z="-162.498" anim="walk" />
   <moveto x="-53.6003" y="0.0" z="-155.041" anim="walk" />
</behavior> 
<behavior name="go_obstacles1" initial="0" completion_decay="20" loop="no"> 
   <moveto x="-53.5112" y="0.0" z="-155.135" anim="walk" />
   <moveto x="-44.2481" y="0.0" z="-153.507" anim="walk" />
   <moveto x="-27.3333" y="0.0" z="-163.914" anim="walk" />
   <moveto x="-32.8587" y="0.0" z="-199.55" anim="walk" />
   <moveto x="-45.5557" y="0.0" z="-213.535" anim="walk" />
   <moveto x="-45.5928" y="0.0" z="-225.046" anim="walk" />
   <moveto x="-42.5066" y="0.0" z="-247.016" anim="walk" />
   <moveto x="-35.3197" y="0.0" z="-246.967" anim="walk" />
   <moveto x="-35.1166" y="0.0" z="-237.451" anim="walk" />
   <moveto x="-26.9788" y="0.0" z="-237.333" anim="walk" />
   <moveto x="-26.5143" y="0.0" z="-248.163" anim="walk" />
   <moveto x="-19.3833" y="0.0" z="-247.596" anim="walk" />
   <moveto x="-19.4912" y="0.0" z="-236.566" anim="walk" />
   <moveto x="-10.7207" y="0.0" z="-236.432" anim="walk" />
   <moveto x="-11.0175" y="0.0" z="-249.279" anim="walk" />
   <moveto x="-4.43381" y="0.0" z="-248.536" anim="walk" />
   <moveto x="-4.49285" y="0.0" z="-236.315" anim="walk" />
   <moveto x="1.82961" y="0.0" z="-235.959" anim="walk" />
   <moveto x="1.64485" y="0.0" z="-249.233" anim="walk" />
   <moveto x="8.12992" y="0.0" z="-249.23" anim="walk" />
   <moveto x="8.19972" y="0.0" z="-230.12" anim="walk" />
   <moveto x="-24.0757" y="0.0" z="-222.12" anim="walk" />
   <moveto x="-24.8802" y="0.0" z="-205.212" anim="walk" />
   <moveto x="-42.7254" y="0.0" z="-166.754" anim="walk" />
   <moveto x="-46.2027" y="0.0" z="-152.995" anim="walk" />
   <moveto x="-53.5608" y="0.0" z="-155.176" anim="walk" />
   <wait duration="30" anim="stand" />
   <moveto x="-53.5608" y="0.0" z="-155.176" anim="walk" />
   <moveto x="-46.2027" y="0.0" z="-152.995" anim="walk" />
   <moveto x="-42.7254" y="0.0" z="-166.754" anim="walk" />
   <moveto x="-24.8802" y="0.0" z="-205.212" anim="walk" />
   <moveto x="-24.0757" y="0.0" z="-222.12" anim="walk" />
   <moveto x="8.19972" y="0.0" z="-230.12" anim="walk" />
   <moveto x="8.12992" y="0.0" z="-249.23" anim="walk" />
   <moveto x="1.64485" y="0.0" z="-249.233" anim="walk" />
   <moveto x="1.82961" y="0.0" z="-235.959" anim="walk" />
   <moveto x="-4.49285" y="0.0" z="-236.315" anim="walk" />
   <moveto x="-4.43381" y="0.0" z="-248.536" anim="walk" />
   <moveto x="-11.0175" y="0.0" z="-249.279" anim="walk" />
   <moveto x="-10.7207" y="0.0" z="-236.432" anim="walk" />
   <moveto x="-19.4912" y="0.0" z="-236.566" anim="walk" />
   <moveto x="-19.3833" y="0.0" z="-247.596" anim="walk" />
   <moveto x="-26.5143" y="0.0" z="-248.163" anim="walk" />
   <moveto x="-26.9788" y="0.0" z="-237.333" anim="walk" />
   <moveto x="-35.1166" y="0.0" z="-237.451" anim="walk" />
   <moveto x="-35.3197" y="0.0" z="-246.967" anim="walk" />
   <moveto x="-42.5066" y="0.0" z="-247.016" anim="walk" />
   <moveto x="-45.5928" y="0.0" z="-225.046" anim="walk" />
   <moveto x="-45.5557" y="0.0" z="-213.535" anim="walk" />
   <moveto x="-32.8587" y="0.0" z="-199.55" anim="walk" />
   <moveto x="-27.3333" y="0.0" z="-163.914" anim="walk" />
   <moveto x="-44.2481" y="0.0" z="-153.507" anim="walk" />
   <moveto x="-53.5112" y="0.0" z="-155.135" anim="walk" />
</behavior> 
<behavior name="go_other_sector1" initial="0" completion_decay="20" loop="no"> 
   <moveto x="-53.546" y="0.0" z="-155.243" anim="walk" />
   <moveto x="-42.1452" y="0.0" z="-140.879" anim="walk" />
   <moveto x="-55.2024" y="0.0" z="-126.302" anim="walk" />
   <moveto x="-57.6445" y="0.0" z="-113.753" anim="walk" />
   <wait duration="30" anim="stand" />
   <moveto x="-57.6445" y="0.0" z="-113.753" anim="walk" />
   <moveto x="-55.2024" y="0.0" z="-126.302" anim="walk" />
   <moveto x="-42.1452" y="0.0" z="-140.879" anim="walk" />
   <moveto x="-53.546" y="0.0" z="-155.243" anim="walk" />
</behavior> 
<behavior name="go_sleep1" initial="0" completion_decay="20" loop="no"> 
   <moveto x="-53.6185" y="0.0" z="-155.243" anim="walk" />
   <moveto x="-51.4396" y="0.0" z="-149.481" anim="walk" />
   <moveto x="-54.8432" y="0.0" z="-146.474" anim="walk" />
   <moveto x="-55.6514" y="0.0" z="-147.652" anim="walk" />
   <invisible/>
   <wait duration="30" anim="stand" />
   <visible/>
   <moveto x="-55.6514" y="0.0" z="-147.652" anim="walk" />
   <moveto x="-54.8432" y="0.0" z="-146.474" anim="walk" />
   <moveto x="-51.4396" y="0.0" z="-149.481" anim="walk" />
   <moveto x="-53.6185" y="0.0" z="-155.243" anim="walk" />
</behavior> 

<react event="time" value="8,0,,,"  behavior="go_other_sector1" delta="20" /> 
<react event="time" value="10,0,,,"  behavior="go_sleep1" delta="20" /> 
<react event="time" value="12,0,,,"  behavior="go_obstacles1" delta="20" /> 
<react event="time" value="14,0,,,"  behavior="go_climbing1" delta="20" />');

INSERT INTO sc_npctypes VALUES("101","QuestMaster1","DoNothing,SpokenTo,PoliteSitting",0,"","","","","","0",
'<empty/>');

INSERT INTO sc_npctypes VALUES("102","QuestMaster2","DoNothing,SpokenTo,PoliteSitting",0,"","","","","","0",
'<empty/>');

INSERT INTO sc_npctypes VALUES("103","DictMaster1","DoNothing,Answerer",0,"","","","","","0",
'<behavior name="Sit" >
   <busy/>
   <sit/>
   <wait duration="15" />
   <standup />
   <idle/>
</behavior>
<react event="sit" behavior="Sit" />');

INSERT INTO sc_npctypes VALUES("104","DictMaster2","DoNothing,Answerer",0,"","","","","","0",
'<empty/>');

INSERT INTO sc_npctypes VALUES("106","ChaseTest1","DoNothing",0,"","","","","","0",
'<behavior name="init" initial="1000" completion_decay="-1" >
   <!--debug level="0" /-->
   <nop/>
</behavior>
<behavior name="chase" decay="1" completion_decay="-1" growth="0" initial="0">
   <talk text="Start chaseing actor" target="false" />
   <chase type="nearest_actor" offset="3" offset_angle="90" range="11" chase_range="20" anim="walk" />
   <talk text="Stop chaseing actor" target="false" />
</behavior>

<react event="player nearby" behavior="chase" delta="100" /> <!-- Nearby(<10) -->
<react event="nearest_actor out of chase" behavior="chase" absolute="0" />');

INSERT INTO sc_npctypes VALUES("107","ChaseTest2","DoNothing",0,"","","","","","0",
'<behavior name="init" initial="1000" completion_decay="-1" >
   <!--debug level="0" /-->
   <nop/>
</behavior>
<behavior name="chase" initial="100" >
   <locate obj="entity:pid:22" />  <!-- Locate MoveMaster2 -->
   <chase type="target" side_offset="2" offset_relative_heading="true" anim="walk" />
   <wait anim="stand" duration="1" />  <!-- Could be a watch operation!! -->
</behavior>');

INSERT INTO sc_npctypes VALUES("108","Minion","DoNothing",0,"","","","","","1",
'<!-- Follow, start on /pet follow and stop on /pet stay -->
<behavior name="follow" completion_decay="100" decay="0" growth="0" initial="0" loop="yes">
   <watch type="owner" range="3.0" />
</behavior>
<behavior name="follow_chase" completion_decay="100" growth="0" initial="0">
   <chase type="owner" anim="walk" collision="follow_collided" offset="1.0+$race_size" />
</behavior>
<behavior name="follow_turn" completion_decay="100" growth="0" initial="0">
   <rotate type="random" min="90" max="270" anim="walk" ang_vel="30" />
   <move vel="2" anim="walk" duration="1.0"/>
</behavior>

<!-- Attack, start on /pet attack and stop on /pet stopattack -->
<behavior name="attack" decay="0" growth="0" initial="0">
   <locate obj="target"  range="50"/>
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <melee seek_range="50" melee_range="3" />
</behavior>
<behavior name="attack_chase" completion_decay="100" growth="0" initial="0">
   <chase type="target" anim="run" vel="5" />
</behavior>
<behavior name="attack_turn" completion_decay="100" growth="0" initial="0">
   <rotate type="random" min="90" max="270" anim="walk" ang_vel="30" />
   <move vel="2" anim="walk" duration="1.0"/>
</behavior>

<behavior name="walk" decay="0" growth="0" initial="0">
   <vel_source vel="$WALK" />
   <talk text="Pet slow down" target="false" />
</behavior>
<behavior name="run" decay="0" growth="0" initial="0">
   <vel_source vel="$RUN" />
   <talk text="Pet seams eager" target="false" />
</behavior>


<!-- Reload the standard behaviour (if different) stored in the -->
<!-- sc_npc_definitions table, behaviour triggered by the npccmd interface -->
<!-- when the Charme spell expires -->
<behavior name="Reload" completion_decay="-1" >
   <!-- TODO: the change_brain operation use the same function like the -->
   <!-- /changenpctype command for GMs, unfortunately it doesn\'t suppport -->
   <!-- the "reload" parameter useful to reload the standard brain stored -->
   <!-- in the sc_npc_definitions table which is supported by the GM command. -->
   <change_brain brain="reload" />
</behavior>

<react event="npccmd:self:reload"   behavior="Reload" />
<react event="ownercmd:stay"        behavior="follow,follow_chase,follow_turn"       absolute="0"   only_interrupt="follow,follow_chase,follow_turn" />
<react event="owner out of chase"   behavior="follow,follow_chase,follow_turn"       absolute="0"   only_interrupt="follow,follow_chase,follow_turn" />
<react event="ownercmd:follow"      behavior="follow"       delta=  "100"  inactive_only="true" />
<react event="follow_collided"      behavior="follow_turn"  delta=  "100" />
<react event="owner out of range"   behavior="follow_chase"  />

<react event="ownercmd:stopattack"  behavior="attack,attack_turn,attack_chase"       absolute= "0" />
<react event="ownercmd:attack"      behavior="attack"       delta=  "200"  inactive_only="true" weight="10" />
<react event="ownercmd:walk"        behavior="walk" />
<react event="ownercmd:run"         behavior="run" />
<react event="collision"            behavior="attack_turn"  delta=  "100" />
<react event="damage"               behavior="attack"       delta=  "200"  inactive_only="true" weight="1" />
<react event="target out of range"  behavior="attack_chase"  />');

INSERT INTO sc_npctypes VALUES("109","AbstractTribesman","DoNothing,Move",0,"","","","","","1",
'<!-- Abstract base npc type for tribes -->

<behavior name="InitTribeMember" completion_decay="-1" initial="1000" resume="yes" >
   <talk text="Initialize new tribe member" target="false" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <talk text="Ready for work" target="false" />
</behavior>

<behavior name="peace_meet" completion_decay="500">
   <locate obj="perception" />
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <emote cmd="/greet" />
</behavior>

<behavior name="aggressive_meet" decay="10" completion_decay="150">
   <locate obj="perception" />
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <melee seek_range="20" melee_range="3" />
</behavior>

<behavior name="coward_attacked" decay="10">
   <emote cmd="/flee" />
   <locate obj="waypoint" static="no" />
   <navigate anim="walk" />
   <locate obj="waypoint" static="no" random="yes" range="10" />
   <wander anim="walk" />
</behavior>

<behavior name="normal_attacked" decay="10">
   <locate obj="target" range="20" />
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <melee seek_range="20" melee_range="3" />
</behavior>

<behavior name="united_attacked" decay="10">
   <locate obj="enemy" />
   <percept event="tribesman attacked" target="tribe" />
   <melee seek_range="20" melee_range="3" />
</behavior>

<behavior name="Fight" completion_decay="-1">
   <locate obj="target" range="20" />
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <melee seek_range="20" melee_range="3" />
</behavior>

<behavior name="Chase" initial="0" growth="0" decay="1" complection_decay="-1" >
   <chase type="target" chase_range="20" anim="run" vel="5" />
</behavior>



<behavior name="Explore" completion_decay="100" resume="yes" >
   <talk text="Going Exploring" target="false" />
   <auto_memorize types="all" /> <!-- Turn on auto memorize -->

   <!-- Only use the move to get to the nearest waypoint. Need the -->
   <!-- wander to be part of this behavior in order to memorize -->
   <locate obj="waypoint" static="no" destination="Move" />
   <percept event="move" />

   <locate obj="waypoint" static="no" random="yes" range="80" />
   <wander anim="walk" />

   <!-- We are at the end of this exploration -->
   <wait anim="stand" duration="3" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <auto_memorize types="all" enable="no" /> <!-- Turn off auto memorize -->

   <!-- Share the memories we have discoved during this exploration with the tribe -->
   <share_memories />
</behavior>

<behavior name="HuntResource" completion_decay="100" resume="yes">
   <talk text="Going hunting $NBUFFER[Resource]" target="false" />

   <!-- Go to work -->
   <locate obj="tribe:memory:hunting_ground"  random="yes"  destination="Move" />
   <percept event="move" />

   <!-- Do some Hunting -->
   <talk text="Hunting $NBUFFER[Resource]..." target="false" />
   <wait duration="10" />
   <reward resource="$NBUFFER[Resource]" count="1" />
   <talk text="...done hunting $NBUFFER[Resource]" target="false" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <!-- Give resource to tribe, and inform where it where found -->
   <transfer item="$NBUFFER[Resource]" target="tribe" />
   <share_memories />
</behavior>

<behavior name="MineResource" completion_decay="100" resume="yes">
   <talk text="Going MineResource $NBUFFER[Resource]" target="false" />

   <!-- Go to work -->
   <locate obj="ownbuffer" static="no"  destination="Move" />
   <percept event="move" />

   <equip item="Rock Pick" slot="righthand" />
   <loop iterations="3">
      <percept event="local_move" />

      <work type="dig" resource="$NBUFFER[Resource]" />
      <wait anim="stand" duration="10" />

      <!-- Must move since dig in same place is not alloved -->
      <locate obj="ownbuffer" static="no" destination="Move" />
   </loop>
   <dequip slot="righthand" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <!-- Give resource to tribe, and inform where it where found -->
   <transfer item="$NBUFFER[Resource]" target="tribe" />
   <share_memories />
</behavior>

<!-- Will test a mine to see what resource there are there. -->
<behavior name="TestMineResource" completion_decay="100" resume="yes">
   <talk text="Going TestMineResource" target="false" />

   <!-- Go to work -->
   <locate obj="tribe:memory:mine" random="yes" static="no" destination="Move" />
   <percept event="move" />

   <equip item="Rock Pick" slot="righthand" />
   <loop iterations="3">
      <percept event="local_move" />

      <work type="dig" />
      <wait anim="stand" duration="10" />

      <!-- Must move since dig in same place is not alloved -->
      <locate obj="tribe:memory:mine" static="no" destination="Move" />
   </loop>
   <dequip slot="righthand" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <!-- TODO transfer item="How to find the item!" target="tribe" /-->
   <share_memories />
</behavior>


<!-- Tribes will react to time events triggering GoToSleep and WakeUp -->
<behavior name="GoToSleep" resume="yes" completion_decay="-1" >
   <talk text="Going to sleep" target="false" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <wait anim="idle" duration="5" />
   <sit />
   <percept event="sleep" />

   <wait duration="5" />
   <standup />
</behavior>
<behavior name="Sleep" resume="yes">
   <talk text="zZz" target="false" />

   <wait duration="24:00" /> <!-- 24 hours game time, will be interrupted upon wakeup -->
</behavior>
<behavior name="WakeUp" resume="yes" completion_decay="-1" >
   <talk text="aaahhh" target="false" />
   <wait duration="5" />
   <percept event="wake_up" />
</behavior>
<react event="sleep" behavior="Sleep" />
<react event="wake_up" behavior="Sleep" absolute="0" />


<behavior name="Breed" resume="yes" completion_decay="100">
   <talk text="Going Breed" target="false" />
   <!--debug level="15" /-->

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <!-- Breed -->
   <emote cmd="/divide" />
   <wait duration="10" anim="stand" />
   <locate obj="self" static="no" />
   <reproduce type="$NBUFFER[Reproduce_Type]" />
   <wait duration="5" anim="stand" />


   <!--debug level="0" /-->
</behavior>

<behavior name="Buy" resume="yes" completion_decay="100">
   <!--debug level="15" /-->
   <talk text="Going Buying $NBUFFER[Trade]" target="false" />

   <!-- Go to work -->
   <locate obj="waypoint" static="no"  destination="Move" />
   <percept event="move" />

   <talk text="Trading $NBUFFER[Trade]" target="false" />
   <wait duration="5" anim="stand" />
   <reward resource="$NBUFFER[Trade]" count="1" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <talk text="Done Buying $NBUFFER[Trade]" target="false" />
   <!--debug level="0" /-->
</behavior>

<behavior name="Resurrect" when_dead="yes" completion_decay="200">
   <resurrect />
</behavior>

<behavior name="GoToWork" completion_decay="100" resume="yes" >
   <talk text="Going to work for $NBUFFER[Work_Duration]" target="false" />

   <!-- Go to work -->
   <locate obj="tribe:memory:work" static="no"  destination="Move" />
   <percept event="move" />

   <emote cmd="/work" />
   <wait duration="$NBUFFER[Work_Duration]" anim="stand" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />
</behavior>

<behavior name="GoBuild" completion_decay="100" resume="yes" >
   <!--debug level="15" /-->
   <talk text="Going to build $NBUFFER[Building] for $NBUFFER[Work_Duration]" target="false" />

   <!-- Go to work -->
   <locate obj="building_spot" static="no"  destination="Move" />
   <percept event="move" />

   <wait duration="$NBUFFER[Work_Duration]" anim="stand" />
   <build />
   <talk text="Nice work building this $NBUFFER[Building]" target="false" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <!--debug level="0" /-->
</behavior>

<behavior name="GoUnbuild" completion_decay="100" resume="yes" >
   <!--debug level="15" /-->
   <talk text="Going to unbuild $NBUFFER[Building] for $NBUFFER[Work_Duration]" target="false" />

   <!-- Go to work -->
   <locate obj="building:$NBUFFER[Building]" static="no"  destination="Move" />
   <percept event="move" />

   <wait duration="$NBUFFER[Work_Duration]" anim="stand" />
   <unbuild />
   <talk text="Nice work tearing this $NBUFFER[Building] down" target="false" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <!--debug level="0" /-->
</behavior>

<behavior name="TestGoUnbuild" completion_decay="-1" resume="yes" >
   <set_buffer buffer="Building" value="Small Tent" />
   <set_buffer buffer="Work_Duration" value="5" />
   <percept event="tribe:unbuild" />
</behavior>
<react event="test_go_unbuild" behavior="TestGoUnbuild" />

<behavior name="Guard" resume="yes" >
   <talk text="Going guarding" target="false" />

   <!-- Go to work -->
   <locate obj="ownbuffer" destination="Move" />
   <percept event="local_move" />

   <circle anim="walk" radius="2" />
</behavior>

<behavior name="Wait" resume="yes" >
   <talk text="Going waiting" target="false" />

   <!-- Go home -->
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />

   <sit/>
   <wait duration="$NBUFFER[Wait_Duration]" anim="sit_idle" />
   <standup/>

</behavior>

<behavior name="Turn" completion_decay="500" >
   <rotate type="random" min="0" max="360" anim="walk" />
   <move anim="walk" duration="3" />
</behavior>

<behavior name="TribeHomeActorEntered" resume="yes" >
   <talk text="Someone has entered my tribe." target="false" />
</behavior>

<behavior name="TribeHomeActorLeft" resume="yes" >
   <talk text="Someone has left my tribe." target="false" />
</behavior>

<behavior name="TribeHomeEntering" resume="yes" >
   <talk text="I am entering a tribe." target="false" />
</behavior>

<behavior name="TribeHomeLiving" resume="yes" >
   <talk text="I am living a tribe." target="false" />
</behavior>

<behavior name="LocateNewTarget" completion_decay="-1" >
   <locate obj="friend" static="no" destination="Target" range="20" />
</behavior>

<behavior name="HuntNPC" completion_decay="100" resume="yes">
   <talk text="Going hunting NPCs" target="false" />

   <!-- Go to work -->
   <locate obj="tribe:memory:npc_battlefield"  random="yes"  destination="Move" />
   <percept event="move" />

   <!-- Kill´em all -->
   <talk text="Killing some NPCs. INTO BATTLE!" target="false" /> 
   <loop iterations="1" >
      <locate obj="friend"  range="20" failure="LocateNewTarget" />
      <hate_list delta="1.0" />
      <percept event="fight" />
   </loop>

   <!-- Go home -->
   <!-- 
   <talk text="Going home now." target="false" />
   <locate obj="tribe:home" static="no" destination="Move" />
   <percept event="move" />
   -->
</behavior>

<behavior name="Loot" initial="0" growth="0" completion_decay="-1" >
   <talk text="Looting..." target="false" />
   <locate obj="dead" range="3" />
   <loot type="all" />
</behavior>

<behavior name="GiveItemTribe" initial="0" growth="0" completion_decay="-1" decay="0" >
    <talk text="Giving $perception_type to tribe..." target="false" />
    <transfer item="$perception_type" target="tribe" />
</behavior>

<react event="inventory:added"          behavior="GiveItemTribe"/> <!-- use some delta? -->
<react event="death"                    behavior="Loot" range="5" />
<react event="fight"                    behavior="Fight" />
<react event="collision"                behavior="Turn" />
<react event="tribe:breed"              behavior="Breed" />
<react event="tribe:build"              behavior="GoBuild" />
<react event="tribe:unbuild"            behavior="GoUnbuild" />
<react event="tribe:buy"                behavior="Buy" />
<react event="tribe:explore"            behavior="Explore" />
<react event="tribe:hunt"               behavior="HuntResource"  />
<react event="tribe:huntnpc"            behavior="HuntNPC" />
<react event="tribe:mine"               behavior="MineResource" />
<react event="tribe:resurrect"          behavior="Resurrect" when_dead="yes" />
<react event="tribe:test_mine"          behavior="TestMineResource"  />
<react event="tribe:wait"               behavior="Wait" />
<react event="tribe:work"               behavior="GoToWork" />
<react event="target out of range"      behavior="Chase" />
<react event="target out of chase"      behavior="Chase" absolute="0" only_interrupt="Chase" />
<react event="location sensed" />

<!-- Perceptions that fire when an actor cross the tribe home boundary -->
<react event="tribe_home:actor_entered" behavior="TribeHomeActorEntered" />
<react event="tribe_home:actor_left"    behavior="TribeHomeActorLeft" />
<react event="tribe_home:entering"      behavior="TribeHomeEntering" />
<react event="tribe_home:living"        behavior="TribeHomeLiving" />
');

INSERT INTO sc_npctypes VALUES("111","MiningTribe","AbstractTribesman",0,"","","","","","0",
'<empty/>');

INSERT INTO sc_npctypes VALUES("112","HuntingTribe","AbstractTribesman",0,"","","","","","0",
'<empty/>');

INSERT INTO sc_npctypes VALUES("113","MoveTest1","",0,"$walk","","","","","0",
'<!-- Testing the wander operation. -->

<!-- Locate nearest waypoint -->
<behavior name="Locate waypoint" completion_decay="-1" initial="100">
   <locate obj="waypoint"  static="no" />
   <navigate anim="walk" />        <!-- Local navigation -->
</behavior>

<behavior name="Wander waypoints" decay="0" growth="0" initial="50" loop="yes">
   <!--debug level="0" /-->
   <wander anim="walk" random="yes" />
</behavior>');

INSERT INTO sc_npctypes VALUES("114","MoveTest2","DoNothing",0,"$run","","","","","0",
'<!-- Initial behavior to locate the starting point -->

<behavior name="Initialize" completion_decay="-1" initial="1000">
   <!--debug level="15" /-->
   <locate obj="waypoint"  static="no" />    <!-- Locate nearest waypoint -->
   <navigate anim="walk" />                  <!-- Local navigation -->
</behavior>

<!-- Advanced movement using waypoints -->
<behavior name="Move1" decay="0" growth="0" initial="80" loop="yes">
   <!--debug level="0" /-->
   <locate obj="waypoint:name:p1"  static="no" />
   <wander anim="walk" />        <!-- Navigate using waypoints -->
   <wait anim="stand" duration="5" />
   <rotate type="relative" value="180" ang_vel="45"/>
   <locate obj="waypoint:group:tower_watch"  random="yes" static="no" />
   <wander anim="walk" />        <!-- Navigate using waypoints -->
   <wait anim="stand" duration="5" />
   <locate obj="waypoint:name:tower_2"  static="no" />
   <wander anim="walk" />        <!-- Navigate using waypoints -->
   <movepath path="Test Path 1" direction="REVERSE" />
   <wait anim="stand" duration="5" />
</behavior>');

INSERT INTO sc_npctypes VALUES("115","MoveTest3","DoNothing",0,"","","","","","0",
'<!-- Testing that advanced movement using waypoints -->
<!-- work fine when interruped. If working ok this NPC -->
<!-- should go the same way as MoveTest2 -->

<!-- Advanced movement using waypoints -->
<behavior name="Move1" decay="0" growth="0" initial="80" loop="yes" resume="yes">
    <!--debug level="0" /-->
    <locate obj="waypoint"  static="no" />    <!-- Locate nearest waypoint -->
    <navigate anim="walk" />                  <!-- Local navigation -->
    <locate obj="waypoint:name:p1"  static="no" />
    <wander anim="walk" />        <!-- Navigate using waypoints -->
    <wait anim="stand" duration="5" />
    <rotate type="relative" value="180" ang_vel="45"/>
    <locate obj="waypoint:name:tower_2"  static="no" />
    <wander anim="walk" />        <!-- Navigate using waypoints -->
    <wait anim="stand" duration="5" />
    <rotate type="relative" value="180" ang_vel="45"/>
</behavior>

<behavior name="Interrupt" completion_decay="0" growth="0" initial="0">
   <!-- Test that a Move1 behaviour is interrupted without failing to move -->
   <!-- Use this as a null operation. -->
   <memorize obj="perception" />
</behavior>

<react event="location sensed"    behavior="Interrupt" />');

INSERT INTO sc_npctypes VALUES("116","MoveTest4","DoNothing",0,"","","","","","0",
'<!-- This test moving between sectors -->

<!-- Advanced movement using waypoints -->
<behavior name="Move1" decay="0" growth="0" initial="80" loop="yes">
   <locate obj="waypoint"  static="no" />    <!-- Locate nearest waypoint -->
   <navigate anim="walk" />                  <!-- Local navigation -->

   <locate obj="waypoint:name:p13"  static="no" />
   <wander anim="walk" vel="$walk" />        <!-- Navigate using waypoints -->
   <wait anim="stand" duration="5" />
   <locate obj="waypoint:name:p3"  static="no" />
   <wander anim="walk" vel="$run" />        <!-- Navigate using waypoints -->
   <wait anim="stand" duration="5" />

   <!-- Test the teleport flag on paths -->
   <locate obj="waypoint:name:npcr1_014"  static="no" />
   <wander anim="walk" vel="$run" />        <!-- Navigate using waypoints -->
   <wait anim="stand" duration="5" />
   <talk text="Going for the teleport" target="false" />
   <locate obj="waypoint:name:npcr1_017"  static="no" />
   <talk text="Going to test teleport path" target="false" />
   <wander anim="walk" vel="$run" />        <!-- Navigate using waypoints -->
   <talk text="Did I make it?" target="false" />
   <wait anim="stand" duration="5" />

</behavior>');

INSERT INTO sc_npctypes VALUES("117","MoveTest5","Answerer,Move,Sleep",0,"$walk","","","","","0",
'<!-- Example Citizen behaviour -->

<!-- Go find nearest waypoint at startup -->
<behavior name="Initialize" resume="yes" completion_decay="-1" initial="1000">
   <locate obj="waypoint"  static="no" destination="Move" /> 
   <percept event="local_move" />
</behavior>

<behavior name="GoHome"  resume="yes" completion_decay="-1" >
   <locate obj="waypoint:name:$name_home"  static="no" destination="Move" />
   <percept event="global_move" />

   <!-- Simulate going inside by setting invisible -->
   <wait duration="2" anim="stand" />
   <invisible/>
   <wait duration="1" anim="stand" />
   <rotate type="locaterotation" ang_vel="45"/>
   <!-- Workaround: Need to wait after rotate, before setting invisbile in the Sleep. To prevent infinit rotation. --> 
   <wait duration="1" anim="stand" />

   <percept event="sleep" />
</behavior>
<react event="GoHome" behavior="GoHome" />

<behavior name="GoWork"  resume="yes" completion_decay="-1" >
   <locate obj="waypoint:name:$name_work"  static="no" destination="Move" />
   <percept event="global_move" /> 
   <rotate type="locaterotation" ang_vel="45"/>
</behavior>
<react event="GoWork" behavior="GoWork" />

<react event="time" value="0,0,,," random=",5,,," behavior="GoWork" /> 
<react event="time" value="1,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="2,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="3,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="4,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="5,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="6,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="7,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="8,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="9,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="10,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="11,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="12,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="13,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="14,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="15,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="16,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="17,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="18,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="19,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="20,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="21,0,,," random=",5,,,"  behavior="GoHome" /> 
<react event="time" value="22,0,,," random=",5,,,"  behavior="GoWork" /> 
<react event="time" value="23,0,,," random=",5,,,"  behavior="GoHome" />');

INSERT INTO sc_npctypes VALUES("118","MoveTest6","DoNothing",0,"$run","","","","","0",
'<!-- Initial behavior to locate the starting point -->
<behavior name="Initialize" completion_decay="-1" initial="1000">
   <!--debug level="0" /-->
   <locate obj="waypoint:name:npcr1_014"  static="no" />    <!-- Locate nearest waypoint -->
   <teleport />                  <!-- Teleport to waypoint -->
</behavior>

<!-- Advanced movement using waypoints -->
<behavior name="Move1" decay="0" growth="0" initial="80" loop="yes" failure="Error" >
   <!--debug level="0" /-->
   <talk text="Going to other waypoint not using private" target="false" />
   <locate obj="waypoint:name:npcr1_021"  static="no" />
   <wander anim="walk" private="false" />        <!-- Navigate using waypoints -->
   <rotate type="relative" value="180" ang_vel="45"/>
   <wait anim="stand" duration="5" />

   <talk text="Going to other waypoint" target="false" />
   <locate obj="waypoint:name:npcr1_014"  random="yes" static="no" />
   <wander anim="walk" />        <!-- Navigate using waypoints -->
   <rotate type="relative" value="180" ang_vel="45"/>
   <wait anim="stand" duration="5" />
</behavior>

<react event="Error" behavior="Initialize" />');

INSERT INTO sc_npctypes VALUES("119","MoveUnderground","DoNothing",0,"$walk","","","","","0",
'<!-- Testing the wander operation. -->

<!-- Locate nearest waypoint -->
<behavior name="Initialize" completion_decay="-1" initial="100">
   <!-- debug level="15" /-->
   <locate obj="waypoint"  static="no" />
   <navigate anim="walk" />        <!-- Local navigation -->
   <locate obj="waypoint:name:ug_1" static="no" />
   <wander anim="walk" />          <!-- Navigate using waypoints -->
</behavior>

<behavior name="Wander waypoints" decay="0" growth="0" initial="50" loop="yes">
   <wander anim="walk" random="yes" underground="true" />
</behavior>');

INSERT INTO sc_npctypes VALUES("120","WinchMover","DoNothing,Move",0,"","","","","","0",
'<!-- Initial behavior to locate the starting point -->
<behavior name="Initialize" resume="yes" completion_decay="-1" initial="1000">
   <!--debug level="0" /-->
   <locate obj="winch:Mover pos 1" static="no" destination="Move" />
   <percept event="move" />
   <rotate type="absolute" value="180" ang_vel="20"/>
</behavior>

<behavior name="turn to face" completion_decay="-1" growth="0" initial="0" >
   <locate obj="perception" />
   <rotate type="locatedest" anim="walk" ang_vel="20" />
   <wait duration="10" anim="stand" />
</behavior>

<behavior name="bring_up" resume="yes" completion_decay="-1" >
   <locate obj="player" range="20" destination="Player" />
   <copy_locate source="Player" destination="Active" only="target" />
   <talk text="I will have the winch beast bring you to the winch." target="true" />
   <locate obj="entity:name:WinchBeast1" />
   <talk text="$target please bring $LOCATION[Player.targetName] to the winch." target="false" />
   <percept event="BringPlayer" type="entity:$LOCATION[Player.targetEID]" target="target" />
</behavior>

<react event="talk" inactive_only="yes" faction_diff="-100" oper=">" behavior="turn to face" delta="100"  when_invisible="yes" when_invincible="yes" />
<react event="npccmd:self:bring_up" behavior="bring_up" delta="100" inactive_only="yes"/>
');

INSERT INTO sc_npctypes VALUES("121","WinchBeast","DoNothing,Move",0,"","","","","","0",
'<!-- Test interaction with sequences in the map -->
<!-- and communication from other NPCs using NPCCMD -->

<!-- Initial behavior to locate the starting point -->
<behavior name="Initialize" completion_decay="-1" initial="1000">
   <!--debug level="0" /-->
   <locate obj="waypoint"  static="no" />    <!-- Locate nearest waypoint -->
   <navigate anim="walk" />                  <!-- Local navigation -->
   <locate obj="winch:Beast pos 1" static="no" />
   <wander anim="walk" />        <!-- Navigate using waypoints -->
   <navigate anim="walk" />      <!-- Local navigation -->
   <rotate type="absolute" value="90" ang_vel="20"/>
</behavior>

<!-- Running the wheel -->
<behavior name="winch_up" completion_decay="100">
   <rotate type="absolute" value="180" ang_vel="20"/>
   <sequence name="winch_up" cmd="start" />
   <circle radius="2.5" angle="360" anim="walk" vel="2" />
   <rotate type="relative" value="-90" ang_vel="20"/>
</behavior> 

<behavior name="winch_down" completion_decay="100">
   <rotate type="absolute" value="0" ang_vel="20"/>
   <sequence name="winch_down" cmd="start" />
   <circle radius="2.5" angle="-360" anim="walk" vel="2" />
   <rotate type="relative" value="90" ang_vel="20"/>
</behavior> 

<behavior name="bring_player" resume="yes" completion_decay="-1" >
   <locate obj="$perception_type" static="no" destination="Player" /> <!-- Expected perception type to be "entity:eid:<eid>" -->
   <copy_locate source="Player" destination="Move" />
   <percept event="move" />
   <copy_locate source="Player" destination="Active" only="target" />
   <control />
   <locate obj="winch:Beast pos 1" destination="Move" />
   <percept event="move" />
   <copy_locate source="Player" destination="Active" only="target" />
   <wait duration="5" />
   <release_control />
   <rotate type="absolute" value="90" ang_vel="20"/>
</behavior>
<react event="BringPlayer" behavior="bring_player" />


<react event="npccmd:global:winch_up" behavior="winch_up" delta="100" inactive_only="yes"/>
<react event="npccmd:global:winch_down" behavior="winch_down" delta="100" inactive_only="yes"/>');

INSERT INTO sc_npctypes VALUES("122","LocateTest1","DoNothing",0,"$run","","","","","0",
'<!-- Test some locate fetures -->

<behavior name="LocateNearestWaypointTest" decay="0" completion_decay="-1" growth="0.1" initial="80" >
   <!--debug level="0" /-->
   <talk text="Try to locating nearest waypoint" target="false" />
   <locate obj="waypoint"  static="no" />
   <navigate anim="walk" />                  <!-- Local navigation -->
   <talk text="Found nearest waypoint" target="false" />
   <wait anim="stand" duration="5" />
</behavior>

<behavior name="LocateFailureTest" decay="0" completion_decay="-1" growth="0.1" initial="0" >
   <!--debug level="0" /-->
   <talk text="Try to locate something that will fail" target="false" />
   <locate obj="SomethingThatDoNotExists" failure="LocateFailed" static="no" />
   <talk text="Error, Locate should have failed and operation stopped." target="false" />
   <wait anim="stand" duration="5" />
</behavior>
<behavior name="LocateFailureFailed" >
   <!--debug level="0" /-->
   <talk text="Locate failed, what do I do?" target="false" />
   <wait anim="stand" duration="5" />
</behavior>

<react event="LocateFailed" behavior="LocateFailureTest" only_interrupt="LocateFailureTest" absolute="0" />
<react event="LocateFailed" behavior="LocateFailureFailed" />');


INSERT INTO sc_npctypes VALUES("124","SpellFighter","DoNothing",0,"$run","","","","","0",
'<!--   -->

<behavior name="Prepare" completion_decay="-1">
   <locate obj="self" />
   <cast spell="Swiftness" k="1.0" />
</behavior>

<behavior name="ReturnFire" completion_decay="-1">
   <locate obj="perception" />
   <cast spell="Summon Missile" k="1.0" />
</behavior>

<behavior name="Drain" completion_decay="-1">
   <locate obj="perception" />
   <cast spell="Drain" k="1.0" />
</behavior>

<react event="player nearby" behavior="Prepare" delta="100" /> <!-- Nearby(<10) -->

<!-- If I am hit by a direct damage spell I will return the fire -->
<react event="spell:self"    type="direct damage" behavior="ReturnFire" delta="100" />

<!-- If my target is healed I will drain -->
<react event="spell:target"    type="direct heal" behavior="Drain" delta="100" />
');

INSERT INTO sc_npctypes VALUES("125","Fighter5","DoNothing,Move,Diurnal,Fight",0,"$walk","","","","","0",
'<empty/>
');
INSERT INTO sc_npctypes VALUES("126","Fighter6","DoNothing,Move,Diurnal,Fight",0,"$walk","","","","","0",
'<empty/>
');
INSERT INTO sc_npctypes VALUES("127","Fighter7","DoNothing,Move,Nocturnal,Fight",0,"$walk","","","","","0",
'<empty/>
');
INSERT INTO sc_npctypes VALUES("128","Fighter8","DoNothing,Move,Nocturnal,Fight",0,"$walk","","","","","0",
'<empty/>
');
INSERT INTO sc_npctypes VALUES("129","Fighter9","DoNothing,Move,Diurnal,Fight",0,"$walk","","","","","0",
'<empty/>
');
INSERT INTO sc_npctypes VALUES("130","Fighter10","DoNothing,Move,Diurnal,Fight",0,"$walk","","","","","0",
'<empty/>
');
INSERT INTO sc_npctypes VALUES("131","Fighter11","DoNothing,Move,Nocturnal,Fight",0,"$walk","","","","","0",
'<empty/>
');
INSERT INTO sc_npctypes VALUES("132","Fighter12","DoNothing,Move,Nocturnal,Fight",0,"$walk","","","","","0",
'<empty/>
');

INSERT INTO sc_npctypes VALUES("140","SpellMaster1","DoNothing",0,"$run","","","","","0",
'<!-- Test various spell abilities -->

<behavior name="InitSpellMaster" initial="1000" completion_decay="-1">
   <talk text="$name is initializing" target="false" />
   <wait duration="15" />
   <locate obj="self" />
   <cast spell="Swiftness" k="0.5" />
</behavior>

<!-- Growth of 2 should trigger this behavior approximatly every 20 sec -->
<behavior name="AttackSpellMaster2" initial="10" growth="2.5" completion_decay="-1">
   <locate obj="entity:name:SpellMaster2" />
   <talk text="Casting Summon Missile on $target" target="false" />
   <cast spell="Summon Missile" k="1.0" />
</behavior>

<behavior name="SpellUnknown" completion_decay="-1">
   <locate obj="perception" />
   <talk text="$target casted a spell" target="false" />
</behavior>

<behavior name="SpellSelf" completion_decay="-1">
   <locate obj="perception" />
   <talk text="$target casted a spell on me" target="false" />
</behavior>

<behavior name="SpellTarget" completion_decay="-1">
   <locate obj="perception" />
   <talk text="$target casted a spell on a target" target="false" />
</behavior>

<react event="spell:unknown" behavior="SpellUnknown" />
<react event="spell:self"    behavior="SpellSelf" />
<react event="spell:target"  behavior="SpellTarget" />
');

INSERT INTO sc_npctypes VALUES("141","SpellMaster2","DoNothing",0,"$run","","","","","0",
'<!-- Test various spell abilities -->

<!-- Growth of 2 should trigger this behavior approximatly every 25 sec -->
<behavior name="BoostSpellMaster" initial="1000" growth="2.0" completion_decay="-1">
   <talk text="$name is boosting" target="false" />
   <wait duration="15" />
   <locate obj="self" />
   <cast spell="Swiftness" k="1.0" />
</behavior>

<behavior name="SpellUnknown" completion_decay="-1">
   <locate obj="perception" />
   <talk text="$target casted a spell" target="false" />
</behavior>

<behavior name="SpellSelf" completion_decay="-1">
   <locate obj="perception" />
   <talk text="$target casted a spell on me" target="false" />
</behavior>

<behavior name="SpellTarget" completion_decay="-1">
   <locate obj="perception" />
   <talk text="$target casted a spell on a target" target="false" />
</behavior>

<react event="spell:unknown" behavior="SpellUnknown" />
<react event="spell:self"    behavior="SpellSelf" />
<react event="spell:target"  behavior="SpellTarget" />
');

INSERT INTO sc_npctypes VALUES("142","FightNearestNPC","DoNothing,InRegion,Fight",0,"$run","","","","","0",
'<behavior name="FightNearestNPC"   loop="yes" initial="50" growth="10" decay="0" completion_decay="0" >
   <locate obj="friend"  range="10" />
   <hate_list delta="1.0" />
   <rotate type="locatedest" anim="walk" ang_vel="120" />
   <melee seek_range="20" melee_range="2.5" stance="normal" />
</behavior>

<behavior name="Loot" initial="0" growth="0" completion_decay="-1" >
   <locate obj="dead" range="3" />
   <loot type="all" />
</behavior>

<react event="player nearby"       behavior="FightNearestNPC" delta="100" inactive_only="yes" />
<react event="fight"               behavior="FightNearestNPC" />
<react event="attack"              behavior="FightNearestNPC" weight="1" /> <!-- Add 1 to hate list -->
<react event="damage"              behavior="FightNearestNPC" delta="20" weight="2" /> <!-- Add double damage to hate list -->

<react event="death"               behavior="FightNearestNPC" absolute="0" />
<react event="death"               behavior="Loot" range="5" />');

INSERT INTO sc_npctypes VALUES("143","SizeMove","MoveTest2",0,"$run","","","","","0",
'<empty/>');

INSERT INTO sc_npctypes VALUES("144","ChaseTest3","DoNothing",0,"","","","","","0",
'<behavior name="init" initial="1000" completion_decay="-1" >
   <!--debug level="0" /-->
   <nop/>
</behavior>
<behavior name="chase" initial="100" >
   <locate obj="entity:pid:22" />  <!-- Locate MoveMaster2 -->
   <chase type="target" side_offset="2" offset_relative_heading="true" anim="walk" adaptiv_vel_script="TestAdaptivVel" />
   <wait anim="stand" duration="1" />  <!-- Could be a watch operation!! -->
</behavior>');

INSERT INTO sc_npctypes VALUES("150","HiredNPC","Follow",0,"$walk","","","","","0",
'<!-- Default movement operation for non scripted hired NPC -->

<behavior name="InitHiredNPC" initial="1000" completion_decay="-1" >
   <!--debug level="0" /-->
   <percept event="follow"/>
</behavior>

<react event="follow"               behavior="follow"       delta=  "100"  inactive_only="true" />
');

INSERT INTO sc_npctypes VALUES("151","HiredGuard","MoveTest2",0,"$walk","","","","","0",
'<!-- Behavior for scripted hired NPCs working as guards. -->
<empty/><!-- TODO Create script -->
');

INSERT INTO sc_npctypes VALUES("152","HiredMerchant","MoveTest2",0,"$walk","","","","","0",
'<!-- Behavior for scripted hired NPCs working as merchant. -->
<empty/><!-- TODO Create script -->
');
