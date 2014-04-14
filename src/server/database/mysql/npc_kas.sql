# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'npc_knowledge_areas'
#

DROP TABLE IF EXISTS `npc_knowledge_areas`;
CREATE TABLE npc_knowledge_areas (
  player_id int(10) unsigned NOT NULL DEFAULT '0' ,
  area varchar(30) NOT NULL DEFAULT '0' ,
  priority tinyint(1) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY (player_id,area),
  KEY player_id (player_id)
);


#
# Dumping data for table 'npc_knowledge_areas'
#

INSERT INTO npc_knowledge_areas VALUES("5","Smith","1");
INSERT INTO npc_knowledge_areas VALUES("5","general","10");
INSERT INTO npc_knowledge_areas VALUES("8","Merchant","1");
INSERT INTO npc_knowledge_areas VALUES("8","general","10");
INSERT INTO npc_knowledge_areas VALUES("4","MaleEnki","1");
INSERT INTO npc_knowledge_areas VALUES("4","general","10");
INSERT INTO npc_knowledge_areas VALUES("12","QuestMaster1","1");
INSERT INTO npc_knowledge_areas VALUES("13","QuestMaster2","1");
INSERT INTO npc_knowledge_areas VALUES("14","DictMaster1","1");
INSERT INTO npc_knowledge_areas VALUES("14","general","10");
INSERT INTO npc_knowledge_areas VALUES("15","DictMaster2","1");
INSERT INTO npc_knowledge_areas VALUES("15","general","10");
INSERT INTO npc_knowledge_areas VALUES("40","WinchMover1","1");
INSERT INTO npc_knowledge_areas VALUES("40","WinchMovers","2");
INSERT INTO npc_knowledge_areas VALUES("59","NPCroom1","1");
INSERT INTO npc_knowledge_areas VALUES("60","NPCroom2","1");
