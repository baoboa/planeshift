# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'character_relationships'
#

DROP TABLE IF EXISTS `character_relationships`;
CREATE TABLE `character_relationships` 
( 
	character_id int(10) unsigned NOT NULL default '0' 			COMMENT 'character id from the characters table' ,
	related_id int(10) unsigned NOT NULL default '0'             COMMENT 'character id of the related character' ,
	relationship_type varchar(15) NOT NULL default ''   COMMENT 'one of three values currently `familiar`, `buddy`, `spouse` or `exploration` ' ,
	spousename  varchar(30) default NULL                COMMENT 'used for marriages, historical data in case of deletion' ,
	
	PRIMARY KEY (`character_id`, `relationship_type`, `related_id` ), 
	KEY ( `character_id` ),
	KEY (`related_id`, `relationship_type`) 
)
COMMENT = 'relations of char to another char, ex. buddy'; 


INSERT INTO character_relationships VALUES( 2,   1, "buddy" , "" ); 
INSERT INTO character_relationships VALUES( 9,   2, "buddy" , "" ); 
INSERT INTO character_relationships VALUES( 1,  11, "familiar" , "" ); 
INSERT INTO character_relationships VALUES( 1,  61, "familiar" , "" );
