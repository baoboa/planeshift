# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'trade_autocontainers'
#

DROP TABLE IF EXISTS `trade_autocontainers`;
CREATE TABLE trade_autocontainers (
  id int(10) unsigned NOT NULL auto_increment,
  item_instance_id int(10) unsigned DEFAULT '0' ,
  garbage_item_id int(10) unsigned DEFAULT '0' ,
  owner_guild_id int(10) unsigned DEFAULT '0' ,
  trans_points int(10) unsigned DEFAULT '0' ,
  animation varchar(30) ,
  description varchar(255) NOT NULL DEFAULT '' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'trade_autocontainers'
#

INSERT INTO trade_autocontainers VALUES(1,74,91,0,75,'','Free standing oven near spawn point.');
INSERT INTO trade_autocontainers VALUES(2,105,73,0,75,'','Smelter.');
INSERT INTO trade_autocontainers VALUES(3,108,0,0,0,'','Anvil.');
INSERT INTO trade_autocontainers VALUES(4,117,0,0,0,'','Potters Wheel.');
