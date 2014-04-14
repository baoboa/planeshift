# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'player_spells'
#

DROP TABLE IF EXISTS `player_spells`;
CREATE TABLE player_spells (
  player_id int(10) unsigned NOT NULL DEFAULT '0' ,
  spell_id int(8) unsigned NOT NULL DEFAULT '0' ,
  spell_slot tinyint(3) unsigned DEFAULT '0' ,
  PRIMARY KEY (player_id,spell_id)
);


#
# Dumping data for table 'player_spells'
#

#INSERT INTO player_spells VALUES("1","1","0");
#INSERT INTO player_spells VALUES("1","2","1");
#INSERT INTO player_spells VALUES("1","3","2");
#INSERT INTO player_spells VALUES("1","4","3");
#INSERT INTO player_spells VALUES("1","5","4");

# Vengeance spells
INSERT INTO player_spells VALUES("2","1","0");
INSERT INTO player_spells VALUES("2","2","1");
INSERT INTO player_spells VALUES("2","3","2");
INSERT INTO player_spells VALUES("2","4","3");
INSERT INTO player_spells VALUES("2","5","4");
INSERT INTO player_spells VALUES("2","6","5");
INSERT INTO player_spells VALUES("2","7","6");
INSERT INTO player_spells VALUES("2","8","7");
INSERT INTO player_spells VALUES("2","10","8");


# NPC SpellMaster1 spells
INSERT INTO player_spells VALUES("82","1","0");
INSERT INTO player_spells VALUES("82","2","1");
INSERT INTO player_spells VALUES("82","3","2");
INSERT INTO player_spells VALUES("82","4","3");
INSERT INTO player_spells VALUES("82","5","4");
INSERT INTO player_spells VALUES("82","6","5");
INSERT INTO player_spells VALUES("82","7","6");
INSERT INTO player_spells VALUES("82","8","7");

# NPC SpellFighter spells
INSERT INTO player_spells VALUES("83","1","0");
INSERT INTO player_spells VALUES("83","2","1");
INSERT INTO player_spells VALUES("83","3","2");
INSERT INTO player_spells VALUES("83","4","3");
INSERT INTO player_spells VALUES("83","5","4");
INSERT INTO player_spells VALUES("83","6","5");
INSERT INTO player_spells VALUES("83","7","6");
INSERT INTO player_spells VALUES("83","8","7");

# NPC SpellMaster2 spells
INSERT INTO player_spells VALUES("92","1","0");
INSERT INTO player_spells VALUES("92","2","1");
INSERT INTO player_spells VALUES("92","3","2");
INSERT INTO player_spells VALUES("92","4","3");
INSERT INTO player_spells VALUES("92","5","4");
INSERT INTO player_spells VALUES("92","6","5");
INSERT INTO player_spells VALUES("92","7","6");
INSERT INTO player_spells VALUES("92","8","7");
