# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'character_skills'
#

DROP TABLE IF EXISTS `character_skills`;
CREATE TABLE character_skills (
  character_id int(10) unsigned NOT NULL DEFAULT '0' ,      # Key into characters table
  skill_id int(10) unsigned NOT NULL DEFAULT '0' ,          # Key into skills table
  skill_Z int(10) unsigned NOT NULL DEFAULT '0' ,           # Practice Level Of skill
  skill_Y int(10) unsigned NOT NULL DEFAULT '0' ,           # Knowledge Level of skills
  skill_Rank int(10) unsigned NOT NULL DEFAULT '0' ,        # Current Skill Rank
  PRIMARY KEY (character_id,skill_id)
);


# Acraig char
INSERT INTO character_skills VALUES("3","0","4","4","3");

# vengeance char
INSERT INTO character_skills VALUES("2","0","0","0","2");
INSERT INTO character_skills VALUES("2","11","0","0","3");
INSERT INTO character_skills VALUES("2","37","0","0","2");
INSERT INTO character_skills VALUES("2","15","0","0","10");
INSERT INTO character_skills VALUES("2","38","0","0","5");
INSERT INTO character_skills VALUES("2","39","0","0","5");
INSERT INTO character_skills VALUES("2","40","0","0","20");

# ??? this points to nothing
INSERT INTO character_skills VALUES("19","51","0","0","60");
INSERT INTO character_skills VALUES("19","50","0","0","60");
INSERT INTO character_skills VALUES("19","49","0","0","60");
INSERT INTO character_skills VALUES("19","48","0","0","60");
INSERT INTO character_skills VALUES("19","47","0","0","60");
INSERT INTO character_skills VALUES("19","46","0","0","60");

# Fighter1 skills on axe (Fighter2 inherits from Fighter1)
INSERT INTO character_skills VALUES("6","2","0","0","100"); # Axe

# Tribe skills on mining
INSERT INTO character_skills VALUES("20","2","0","0","165"); # Axe
INSERT INTO character_skills VALUES("20","4","0","0","115"); # Melee
INSERT INTO character_skills VALUES("20","37","0","0","200"); # Mining
INSERT INTO character_skills VALUES("20","46","0","0","75"); # Agility
INSERT INTO character_skills VALUES("20","47","0","0","85"); # Charisma
INSERT INTO character_skills VALUES("20","48","0","0","175"); # Endurance
INSERT INTO character_skills VALUES("20","49","0","0","65"); # Intelligence
INSERT INTO character_skills VALUES("20","50","0","0","175"); # Strength
INSERT INTO character_skills VALUES("20","51","0","0","95"); # Will

# Tribe skills on hunting
INSERT INTO character_skills VALUES("62","0","0","0","65"); # Sword
INSERT INTO character_skills VALUES("62","1","0","0","95"); # Knives & Daggers
INSERT INTO character_skills VALUES("62","4","0","0","115"); # Melee
INSERT INTO character_skills VALUES("62","6","0","0","195"); # Ranged
INSERT INTO character_skills VALUES("62","46","0","0","175"); # Agility
INSERT INTO character_skills VALUES("62","47","0","0","95"); # Charisma
INSERT INTO character_skills VALUES("62","48","0","0","175"); # Endurance
INSERT INTO character_skills VALUES("62","49","0","0","75"); # Intelligence
INSERT INTO character_skills VALUES("62","50","0","0","95"); # Strength
INSERT INTO character_skills VALUES("62","51","0","0","85"); # Will

# Strong fighter8 to test loot cost cap
INSERT INTO character_skills VALUES("87","46","0","0","400");
INSERT INTO character_skills VALUES("87","47","0","0","400");
INSERT INTO character_skills VALUES("87","48","0","0","400");
INSERT INTO character_skills VALUES("87","49","0","0","400");
INSERT INTO character_skills VALUES("87","50","0","0","400");
INSERT INTO character_skills VALUES("87","51","0","0","400");

# SpellMaster1, make spells a 100% probability
INSERT INTO character_skills VALUES("82","47","0","0","400"); # Charisma
INSERT INTO character_skills VALUES("82","51","0","0","400"); # Will
INSERT INTO character_skills VALUES("82","11","0","0","400"); # Crystal
INSERT INTO character_skills VALUES("82","12","0","0","400"); # Azure

# SpellMaster1, make spell a 100% probability
INSERT INTO character_skills VALUES("92","47","0","0","400"); # Charisma
INSERT INTO character_skills VALUES("92","51","0","0","400"); # Will
INSERT INTO character_skills VALUES("92","11","0","0","400"); # Crystal
INSERT INTO character_skills VALUES("92","12","0","0","400"); # Azure
