# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'sc_location_type'
#

DROP TABLE IF EXISTS `sc_location_type`;
CREATE TABLE `sc_location_type` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `name` varchar(30) NOT NULL DEFAULT '' ,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table 'locations'
#
INSERT INTO `sc_location_type` VALUES (1, 'field');
INSERT INTO `sc_location_type` VALUES (2, 'mine');
INSERT INTO `sc_location_type` VALUES (3, 'market');
INSERT INTO `sc_location_type` VALUES (4, 'pvp_region');
INSERT INTO `sc_location_type` VALUES (5, 'wander region');
INSERT INTO `sc_location_type` VALUES (6, 'winch');
INSERT INTO `sc_location_type` VALUES (7, 'NPC Room Fighter region 1');
INSERT INTO `sc_location_type` VALUES (8, 'home');
INSERT INTO `sc_location_type` VALUES (9, 'work');
INSERT INTO `sc_location_type` VALUES (10,'tribe_home');
INSERT INTO `sc_location_type` VALUES (11,'hunting_ground');
INSERT INTO `sc_location_type` VALUES (12,'sleeping_area');
INSERT INTO `sc_location_type` VALUES (13,'NPC Room South Meadow');
INSERT INTO `sc_location_type` VALUES (14,'NPC Room North Meadow');
INSERT INTO `sc_location_type` VALUES (15,'npc_battlefield');

