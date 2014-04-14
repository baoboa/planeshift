# MySQL-Front 3.2  (Build 13.0)

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES */;

/*!40101 SET NAMES latin1 */;
/*!40103 SET TIME_ZONE='SYSTEM' */;

# Host: localhost    Database: planeshift
# ------------------------------------------------------
# Server version 5.0.19-nt

#
# Table structure for table npc_triggers
#

DROP TABLE IF EXISTS `npc_triggers`;
CREATE TABLE `npc_triggers` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `trigger_text` varchar(255) default NULL,
  `prior_response_required` int(10) unsigned default '0',
  `area` varchar(30) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB;

#
# Dumping data for table npc_triggers
#

INSERT INTO `npc_triggers` VALUES (1,'greetings',0,'general');
INSERT INTO `npc_triggers` VALUES (2,'how you',0,'general');
INSERT INTO `npc_triggers` VALUES (3,'goodbye',0,'general');
INSERT INTO `npc_triggers` VALUES (4,'about you',0,'general');
INSERT INTO `npc_triggers` VALUES (5,'about you',0,'Smith');
INSERT INTO `npc_triggers` VALUES (6,'train',0,'Smith');
INSERT INTO `npc_triggers` VALUES (8,'train',0,'Merchant');
INSERT INTO `npc_triggers` VALUES (9,'<l money=\"0,0,0,11\"></l>',23,'Merchant');
INSERT INTO `npc_triggers` VALUES (10,'repeat just now',0,'general');
INSERT INTO `npc_triggers` VALUES (11,'repeat recently',0,'general');
INSERT INTO `npc_triggers` VALUES (12,'repeat already',0,'general');
INSERT INTO `npc_triggers` VALUES (13,'give me thing',0,'general');
INSERT INTO `npc_triggers` VALUES (14,'givemeexp',0,'Smith');
INSERT INTO `npc_triggers` VALUES (15,'about you',0,'Merchant');
INSERT INTO `npc_triggers` VALUES (16,'error',0,'general');
INSERT INTO `npc_triggers` VALUES (17,'error',0,'QuestMaster1');
INSERT INTO `npc_triggers` VALUES (18,'error',0,'QuestMaster2');
INSERT INTO `npc_triggers` VALUES (19,'greetings',0,'QuestMaster1');
INSERT INTO `npc_triggers` VALUES (20,'greetings',0,'QuestMaster2');
INSERT INTO `npc_triggers` VALUES (21,'give me fruit',0,'DictMaster1');
INSERT INTO `npc_triggers` VALUES (22,'give me apple',0,'DictMaster1');
INSERT INTO `npc_triggers` VALUES (23,'give me orange',0,'DictMaster1');
INSERT INTO `npc_triggers` VALUES (24,'greetings',0,'DictMaster2');
INSERT INTO `npc_triggers` VALUES (25,'give me fruit',0,'DictMaster2');
INSERT INTO `npc_triggers` VALUES (26,'give me pear',0,'DictMaster2');
INSERT INTO `npc_triggers` VALUES (27,'give me plum',0,'DictMaster2');
INSERT INTO `npc_triggers` VALUES (28,'about you',31,'DictMaster2');
INSERT INTO `npc_triggers` VALUES (29,'about you',0,'DictMaster2');
INSERT INTO `npc_triggers` VALUES (30,'!veryshortrange',0,'general');
INSERT INTO `npc_triggers` VALUES (31,'!shortrange',0,'general');
INSERT INTO `npc_triggers` VALUES (32,'!longrange',0,'general');
INSERT INTO `npc_triggers` VALUES (33,'winch up',0,'WinchMovers');
INSERT INTO `npc_triggers` VALUES (34,'winch down',0,'WinchMovers');
INSERT INTO `npc_triggers` VALUES (35,'unknown',0,'general');
INSERT INTO `npc_triggers` VALUES (36,'!anyrange',0,'NPCroom1');
INSERT INTO `npc_triggers` VALUES (37,'!anyrange',0,'NPCroom2');
INSERT INTO `npc_triggers` VALUES (38,'bring me up',0,'WinchMovers');
INSERT INTO `npc_triggers` VALUES (39,'grettings',0,'WinchMovers');
INSERT INTO `npc_triggers` VALUES (40,'enkidukai',0,'MaleEnki');
INSERT INTO `npc_triggers` VALUES (41,'goldore',0,'MaleEnki');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
