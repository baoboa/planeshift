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
# Table structure for table npc_responses
#

DROP TABLE IF EXISTS `npc_responses`;
CREATE TABLE `npc_responses` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `trigger_id` int(10) unsigned default '0',
  `response1` text,
  `response2` text,
  `response3` text,
  `response4` text,
  `response5` text,
  `pronoun_him` varchar(30) default '',
  `pronoun_her` varchar(30) default '',
  `pronoun_it` varchar(30) default '',
  `pronoun_them` varchar(30) default '',
  `script` blob,
  `prerequisite` blob,
  `quest_id` int(10) unsigned default NULL,
  `audio_path1` varchar(100) default NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response1.',
  `audio_path2` varchar(100) default NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response2.',
  `audio_path3` varchar(100) default NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response3.',
  `audio_path4` varchar(100) default NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response4.',
  `audio_path5` varchar(100) default NULL COMMENT 'This holds an optional VFS path to a speech file to be sent to the client and played on demand for response5.',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB;

#
# Dumping data for table 'npc_responses'
#

INSERT INTO `npc_responses` VALUES (1,1,'Hello $playername.','Hello friend.','Hi.  What can I do for you?','Go away!','Whatever dude...','0','0','0','0','<response><respondpublic/><action anim=\"greet\"/></response>','',0,'/planeshift/data/voice/merchant/hello_n.wav','/planeshift/data/voice/merchant/hello_n.wav','/planeshift/data/voice/merchant/hello_n.wav','/planeshift/data/voice/merchant/hello_n.wav','/planeshift/data/voice/merchant/hello_n.wav');
INSERT INTO `npc_responses` VALUES (2,2,'I\'m fine and you?','Terrible... what a day...','Comme ci, comme ca... et toi?','Va bene, e tu?','','0','0','0','0','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (3,3,'Later.','May the Crystal shine brightly on you.','Goodbye $playername.','','','0','0','0','0','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (11,4,'I\'m just a simple peasant.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (12,5,'I\'m the great creator of many weapons.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (15,6,'I will train you','','','','','','','','','<response><respond/><train skill=\"Sword\"/></response>','<pre><faction name=\"orcs\" value=\"0\"/></pre>',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (16,6,'No way I train you','','','','','','','','','','<pre><not><faction name=\"orcs\" value=\"0\"/></not></pre>',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (17,9,'Thanks, friend.','What a lovely gesture.','','','','','','','','<response><respond/><action anim=\"greet\"/></response>','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (18,10,'Umm, didn\'t you just say that?','I feel like I\'m repeating myself here','Don\'t be annoying.','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (19,11,'Oh weird, it\'s like deja vu!','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (20,12,'I have already responded to that and I will not do so again, $sir!','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (21,13,'I\'m not giving you anything.  What have you given me?','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (22,14,'Ok, I will give you some exp.','','','','','','','','','<response><run script="give_exp" with="Exp = 200;"/></response>','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (23,15,'Hi $sir!, I will say thank you for 11 trias.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (24,16,'You are speaking strangely, please rephrase.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (25,17,'this is my error response.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (26,19,'I QuestMaster1 test Quest sub step control[Type \'step2\', verify error response. Type \'step1\', verify quest started and follow instructions], and Quest lockouts[Type \'quest2\', veriyf quest started and follow instructions.]','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (27,20,'I QuestMaster2 test Quest prerequisites[Type \'quest2\', verify error response. Type \'quest1\', verify quest assigned and follow instructions]','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (28,21,'I have the following fruits: apple, orange','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (29,22,'You asked for apple','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (30,23,'You asked for orange','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (31,24,'A greeting from DictMaster2','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (32,25,'I have the following fruits: pear, plum','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (33,26,'You asked for pear','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (34,27,'You asked for plum','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (35,29,'How rude! You asked for about me, without saying hello.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (36,28,'You asked for about me','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (37,20,'Welcome to npcroom, $playername.  Do you need help?',NULL,NULL,NULL,NULL,'','','','',NULL,'',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (38,31,'Be thee friend or foe, stranger?',NULL,NULL,NULL,NULL,'','','','',NULL,'',0,'/data/voice/merchant/betheefriend.spx',NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (39,32,'Hark!  Who goes there?',NULL,NULL,NULL,NULL,'','','','',NULL,'',0,'/data/voice/merchant/hark.spx',NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (40,33,'Ok, I will winch send the \"winch up\" command to the beast.','','','','','','','','','<response><respond/><npccmd cmd=\"winch_up\" /></response>','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (41,34,'Ok, I will winch send the \"winch down\" command to the beast.','','','','','','','','','<response><respond/><npccmd cmd=\"winch_down\" /></response>','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (42,8,'I will train you','','','','','','','','','<response><respond/><train skill=\"Sword\"/></response>','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (43,18,'this is my error response.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (44,35,'You just asked a question that I do not know the answer to.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (45,36,NULL,NULL,NULL,NULL,NULL,'','','','','<response><run script="explore_area" with="Area = \'NPCroom1\'; Range = 100; Exp = 100;"/></response>','',NULL,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (46,37,NULL,NULL,NULL,NULL,NULL,'','','','','<response><run script="explore_area" with="Area = \'NPCroom2\'; Range = 100; Exp = 1000;"/></response>','',NULL,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (47,38,'Ok, I will bring you up by send the \"bring up\" command to the beast.','','','','','','','','','<response><respond/><npccmd cmd=\"bring_up\" /></response>','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (48,39,'Welcome to the Winch. Tell me to "winch up" to have the winch beast winch up, "winch down" to have the winch beast winch down, or "bring me up" to have the winch beast bring you up.','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);

INSERT INTO `npc_responses` VALUES (49,40,'Enkidukai are a feline race! Meowwwww','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);
INSERT INTO `npc_responses` VALUES (50,41,'This is the answer from MaleEnki trigger KA','','','','','','','','','','',0,NULL,NULL,NULL,NULL,NULL);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
