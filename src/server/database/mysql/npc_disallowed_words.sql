# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'npc_disallowed_words'
#

DROP TABLE IF EXISTS `npc_disallowed_words`;
CREATE TABLE npc_disallowed_words (
  word varchar(30) NOT NULL DEFAULT '' ,
  PRIMARY KEY (word)
);


#
# Dumping data for table 'npc_disallowed_words'
#

INSERT INTO npc_disallowed_words VALUES("a");
INSERT INTO npc_disallowed_words VALUES("an");
INSERT INTO npc_disallowed_words VALUES("and");
INSERT INTO npc_disallowed_words VALUES("anything");
INSERT INTO npc_disallowed_words VALUES("i");
INSERT INTO npc_disallowed_words VALUES("is");
INSERT INTO npc_disallowed_words VALUES("isn't");
INSERT INTO npc_disallowed_words VALUES("please");
INSERT INTO npc_disallowed_words VALUES("the");
