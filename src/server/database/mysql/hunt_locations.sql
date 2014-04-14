# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'hunt_locations'
#

DROP TABLE IF EXISTS `hunt_locations`;
CREATE TABLE `hunt_locations` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `x` double(10,2) NOT NULL default '0.00',
  `y` double(10,2) NOT NULL default '0.00',
  `z` double(10,2) default '0.00',
  `itemid` int(10) NOT NULL default '0',
  `interval` int(11) NOT NULL default '0',
  `max_random` int(11) NOT NULL default '0',
  `sector` int(10) unsigned NOT NULL default '0',
  `amount` int(10) unsigned NOT NULL default '1',
  `range` double(10,2) unsigned NOT NULL default '0.00',
  `lock_str` int(5) NOT NULL DEFAULT '0' COMMENT 'The lock strength of the generated item.',
  `lock_skill` int(2) NOT NULL DEFAULT '-1' COMMENT 'The lock skill used to open the item.',
  `flags` VARCHAR( 200 ) NOT NULL DEFAULT '' COMMENT 'The flags to apply to the item.',
  PRIMARY KEY  (`id`)
)
COMMENT = 'Areas to spawn items, like apples or mushrooms';

#
# Dumping data for table 'hunt_locations'
#
INSERT INTO `hunt_locations` VALUES (1, '27.60', '0.01', '-190', 91, 4000, 0, 3, 2, '5.00', 0, -1, '');
INSERT INTO `hunt_locations` VALUES (2, '-75', '0.01', '-186', 59, 4000, 0, 3, 1, '3.00', 100, 22, 'LOCKABLE LOCKED');