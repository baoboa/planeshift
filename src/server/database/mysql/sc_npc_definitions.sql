# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'sc_npc_definitions'
#

DROP TABLE IF EXISTS `sc_npc_definitions`;
CREATE TABLE sc_npc_definitions (
  char_id int(10) NOT NULL DEFAULT '0' ,
  name varchar(30) ,
  npctype varchar(30) ,
  region varchar(30) ,
  move_vel_override float(10,2) DEFAULT '0.00' ,
  ang_vel_override float(10,2) DEFAULT '0.00' ,
  char_id_owner int(10) unsigned DEFAULT '0' ,
  console_debug char(1) DEFAULT '0' ,
  disabled char(1) DEFAULT 'N' ,
  PRIMARY KEY (char_id)
);


#
# Dumping data for table 'sc_npc_definitions'
#

INSERT INTO sc_npc_definitions VALUES("4","MaleEnki","Wanderer","wander region","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("5","Smith","Smith","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("6","Fighter1","Fighter","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("7","Fighter2","On Sight Fighter","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("8","Merchant","Wanderer","wander region","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("12","QuestMaster1","QuestMaster1","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("13","QuestMaster2","QuestMaster2","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("14","DictMaster1","DictMaster1","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("15","DictMaster2","DictMaster2","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("20","Miner","AbstractTribesman","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("21","MoveMaster1","MoveTest1","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("22","MoveMaster2","MoveTest2","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("23","MoveMaster3","MoveTest3","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("24","MoveMaster4","MoveTest4","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("25","Fighter3","Fighter","NPC Room Fighter region 1","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("26","Fighter4","On Sight Fighter","NPC Room Fighter region 1","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("27","MoveMaster5A","MoveTest5","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("28","MoveMaster5B","MoveTest5","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("29","MoveMaster5C","MoveTest5","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("30","WinchBeast1","WinchBeast","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("40","WinchMover1","WinchMover","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("50","FAMILIAR:Rogue","Minion","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("51","FAMILIAR:Clacker","Minion","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("52","FAMILIAR:Rat","Minion","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("53","MoveUnderground","MoveUnderground","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("62","Hunter","HuntingTribe","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("78","MoveMaster6","MoveTest6","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("79","LocateMaster1","LocateTest1","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("80","Chaser1","ChaseTest1","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("81","Chaser2","ChaseTest2","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("82","SpellMaster1","SpellMaster1","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("83","SpellFighter","SpellFighter","NPC Room Fighter region 1","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("84","Fighter5","Fighter5","NPC Room South Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("85","Fighter6","Fighter6","NPC Room South Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("86","Fighter7","Fighter7","NPC Room South Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("87","Fighter8","Fighter8","NPC Room South Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("88","Fighter9","Fighter9","NPC Room North Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("89","Fighter10","Fighter10","NPC Room North Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("90","Fighter11","Fighter11","NPC Room North Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("91","Fighter12","Fighter12","NPC Room North Meadow","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("92","SpellMaster2","SpellMaster2","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("100","NPC Fighter 1","FightNearestNPC","npc_battlefield","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("101","NPC Fighter 2","FightNearestNPC","npc_battlefield","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("102","NPC Fighter 3","FightNearestNPC","npc_battlefield","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("103","NPC Fighter 4","FightNearestNPC","npc_battlefield","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("104","Small Mover","SizeMove","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("105","Large Mover","SizeMove","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("106","Huge Mover","SizeMove","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("107","Chaser3","ChaseTest3","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("110","Hired Guard","HiredNPC","","0.00","0.00","0","N","N");
INSERT INTO sc_npc_definitions VALUES("111","Hired Merchant","HiredNPC","","0.00","0.00","0","N","N");
