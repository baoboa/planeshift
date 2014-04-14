# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'loot_rule_details'
#

DROP TABLE IF EXISTS `loot_rule_details`;
CREATE TABLE  `planeshift`.`loot_rule_details` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `loot_rule_id` int(10) unsigned NOT NULL DEFAULT '0',
  `item_stat_id` int(10) unsigned NOT NULL DEFAULT '0',
  `min_item` int(10) unsigned NOT NULL DEFAULT '1' COMMENT 'The minimum amount of these items which could be created',
  `max_item` int(10) unsigned NOT NULL DEFAULT '1' COMMENT 'The maximum amount of these items which could be created',
  `probability` float(5,4) NOT NULL DEFAULT '0.0000',
  `min_money` int(10) unsigned NOT NULL DEFAULT '0',
  `max_money` int(10) unsigned NOT NULL DEFAULT '0',
  `randomize` tinyint(1) NOT NULL DEFAULT '0',
  `randomize_probability` float(5,4) NOT NULL DEFAULT '1.0000' COMMENT 'The probability this items will be randomized if choosen',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Dumping data for table 'loot_rule_details'
#

INSERT INTO loot_rule_details VALUES (1,1,8,1,1,0.5,0,0,0,1);
INSERT INTO loot_rule_details VALUES (2,1,7,1,1,0.35,0,0,1,1);
INSERT INTO loot_rule_details VALUES (3,1,6,1,1,0.15,10,35,0,1);
INSERT INTO loot_rule_details VALUES (4,2,3,1,1,1,0,0,1,1);
