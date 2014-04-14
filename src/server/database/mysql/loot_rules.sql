# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'loot_rules'
#

DROP TABLE IF EXISTS `loot_rules`;
CREATE TABLE loot_rules (
  id int(10) unsigned NOT NULL auto_increment,
  name varchar(30) ,
  PRIMARY KEY (id) ,
  UNIQUE KEY `name` (`name`)
);


#
# Dumping data for table 'loot_rules'
#

INSERT INTO loot_rules VALUES("1","Drop 1 of Gold/Steel Falchion");
INSERT INTO loot_rules VALUES("2","50% chance of broadsword + 10-");
