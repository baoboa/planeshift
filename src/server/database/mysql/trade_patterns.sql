# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'trade_patterns'
#

DROP TABLE IF EXISTS `trade_patterns`;
CREATE TABLE trade_patterns (
  id int(8) unsigned NOT NULL auto_increment,
  pattern_name varchar(40) NOT NULL,
  group_id int(8) unsigned NOT NULL DEFAULT '0',
  designitem_id int(10) unsigned ,
  k_factor float(10,2) NOT NULL DEFAULT '0.00' ,
  description varchar(255) NOT NULL DEFAULT '' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'trade_patterns'
#

INSERT INTO trade_patterns VALUES(1,'waybread',2,71,8.00,'Baking Waybread');
INSERT INTO trade_patterns VALUES(2,'BAKING',0,0,8.00,'Baking Group Stuff');
INSERT INTO trade_patterns VALUES(3,'fishing reel',0,120,4.00,'Casting a Fishing Reel');
INSERT INTO trade_patterns VALUES(4,'hammer head',0,125,4.00,'Casting a Hammer Head');
INSERT INTO trade_patterns VALUES(5,'avil',0,127,4.00,'Casting an Anvil');
INSERT INTO trade_patterns VALUES(6,'wayoutbread',0,0,8.00,'Baking Wayoutbread');
INSERT INTO trade_patterns VALUES(7,'flame weapon',0,0,8.00,'Flaming weapon');

-- Advanced crafting
INSERT INTO `trade_patterns` VALUES (458,'Enchant Gem',99999,3034,4,'Enchant Gem Pattern');
