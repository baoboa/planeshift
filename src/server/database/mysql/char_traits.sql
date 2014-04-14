# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'character_traits'
#

DROP TABLE IF EXISTS `character_traits`;
CREATE TABLE character_traits (
  character_id int(10) unsigned NOT NULL DEFAULT '0' ,
  trait_id int(10) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY (character_id,trait_id)
);


#
# Dumping data for table 'character_traits'
#

