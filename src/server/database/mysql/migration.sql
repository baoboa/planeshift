# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'migration'
#

DROP TABLE IF EXISTS `migration`;
CREATE TABLE migration (
  id int(8) unsigned NOT NULL auto_increment,
  username varchar(50) NOT NULL DEFAULT '0' ,
  script text ,
  email varchar(64) ,
  time  int(12),
  done  char(1) default 'N',
  PRIMARY KEY (id),
  KEY id_2 (id),
  KEY username_2 (username),
  UNIQUE username (username),
  UNIQUE id (id)
);

