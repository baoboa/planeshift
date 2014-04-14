# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'spell_glyphs'
#

DROP TABLE IF EXISTS `spell_glyphs`;
CREATE TABLE spell_glyphs (
  spell_id int(8) unsigned NOT NULL DEFAULT '0' ,
  item_id int(10) unsigned NOT NULL DEFAULT '0' ,
  position tinyint(3) unsigned DEFAULT '0' ,
  PRIMARY KEY (spell_id,item_id),
  UNIQUE (spell_id, position)
);


#
# Dumping data for table 'spell_glyphs'
#

INSERT INTO spell_glyphs VALUES("1","13","0"); #Arrow
INSERT INTO spell_glyphs VALUES("2","15","0"); #Energy
INSERT INTO spell_glyphs VALUES("3","14","0"); #Door
INSERT INTO spell_glyphs VALUES("4","19","0"); #Air
INSERT INTO spell_glyphs VALUES("5","47","0"); #Summon
INSERT INTO spell_glyphs VALUES("5","44","1"); #Creature
INSERT INTO spell_glyphs VALUES("6","20","0"); #Bond
INSERT INTO spell_glyphs VALUES("6","29","1"); #Fire
INSERT INTO spell_glyphs VALUES("7","17","0"); #Light
INSERT INTO spell_glyphs VALUES("8","15","0"); #Energy
INSERT INTO spell_glyphs VALUES("8","14","1"); #Door
INSERT INTO spell_glyphs VALUES("9","57","0"); #Sphere
INSERT INTO spell_glyphs VALUES("10","150","0"); #Fear
INSERT INTO spell_glyphs VALUES("11","20","0"); #Bond
INSERT INTO spell_glyphs VALUES("11","44","1"); #Creature
