# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'guildlevels'
#

DROP TABLE IF EXISTS `guildlevels`;
CREATE TABLE guildlevels (
  guild_id int(11) unsigned NOT NULL DEFAULT '0' ,
  level smallint(3) unsigned NOT NULL DEFAULT '0' ,
  level_name varchar(25) NOT NULL DEFAULT '0' ,
  rights smallint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (guild_id,level)
);


#
# Dumping data for table 'guildlevels'
#

INSERT INTO guildlevels VALUES("444","9","Maximus",65535);
INSERT INTO guildlevels VALUES("444","8","Mogul",3);
INSERT INTO guildlevels VALUES("444","7","Grand Master",3);
INSERT INTO guildlevels VALUES("444","6","High Master",3);
INSERT INTO guildlevels VALUES("444","5","Master",3);
INSERT INTO guildlevels VALUES("444","4","Peon",3);
INSERT INTO guildlevels VALUES("444","3","Adept",3);
INSERT INTO guildlevels VALUES("444","2","Novice",3);
INSERT INTO guildlevels VALUES("444","1","Apprentice",3);
INSERT INTO guildlevels VALUES("11","9","Maximus",65535);
INSERT INTO guildlevels VALUES("11","8","Mogul",3);
INSERT INTO guildlevels VALUES("11","7","Grand Master",3);
INSERT INTO guildlevels VALUES("11","6","High Master",3);
INSERT INTO guildlevels VALUES("11","5","Master",3);
INSERT INTO guildlevels VALUES("11","4","Senior",3);
INSERT INTO guildlevels VALUES("11","3","Adept",3);
INSERT INTO guildlevels VALUES("11","2","Novice",3);
INSERT INTO guildlevels VALUES("11","1","Apprentice",3);
