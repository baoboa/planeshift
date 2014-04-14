# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'item_animations'
#

DROP TABLE IF EXISTS `item_animations`;
CREATE TABLE `item_animations` (
  `id` int(10) unsigned NOT NULL DEFAULT '0',
  `cstr_animation` varchar(200) NOT NULL DEFAULT '',
  `min_use_level` int(5) unsigned NOT NULL DEFAULT '0',
  `type_flags` smallint(5) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`,`cstr_animation`)
);

#
# Dumping data for table 'item_animations'
#

INSERT INTO item_animations VALUES("1","attack","0","0");
INSERT INTO item_animations VALUES("2","greet","0","0");
