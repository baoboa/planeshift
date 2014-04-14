# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'character_quests'
#

DROP TABLE IF EXISTS `character_quests`;
CREATE TABLE `character_quests` (
  `player_id` int(10) unsigned NOT NULL default '0',
  `quest_id` int(10) unsigned NOT NULL default '0',
  `status` char(1) NOT NULL default 'O',
  `assigner_id` int(10) unsigned default '0',
  `remaininglockout` int(10) default '0',
  `last_response` int(10) default '-1',
  `last_response_npc_id` INT(10) unsigned DEFAULT NULL COMMENT 'This field stores the npc PID who gave the last response. Required for menu filtering.',
  `completionOrder` INT(10) unsigned NOT NULL DEFAULT '0' COMMENT 'This field stores the ordering in which this quest was completed from 0 to n.',
  PRIMARY KEY  (`player_id`,`quest_id`)
);



#
# Dumping data for table 'character_quests'
#
INSERT INTO `character_quests` VALUES (2,1,'C',6,0,-1,0,0);	# Somebody must rescue the Princess
