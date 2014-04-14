# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'security_levels'
#

DROP TABLE IF EXISTS `security_levels`;
CREATE TABLE security_levels (
  level tinyint(3) unsigned NOT NULL DEFAULT '0' ,
  title varchar(30) NOT NULL DEFAULT '0' ,
  PRIMARY KEY (level)
);


#
# Dumping data for table 'security_levels'
#

INSERT INTO security_levels VALUES("0","Player");
INSERT INTO security_levels VALUES("1","Guide");
INSERT INTO security_levels VALUES("2","GameMaster");
INSERT INTO security_levels VALUES("3","WorldBuilder");
INSERT INTO security_levels VALUES("4","Admin");
