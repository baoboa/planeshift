# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'factions'
#

DROP TABLE IF EXISTS `factions`;
CREATE TABLE factions (
  id int(8) unsigned NOT NULL auto_increment,
  faction_name varchar(40) NOT NULL DEFAULT '' ,
  `faction_description` text,
  `faction_character` TEXT  NOT NULL,
  faction_weight float(6,3) NOT NULL DEFAULT '1.000' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'factions'
#

INSERT INTO factions VALUES("1","orcs","faction orcs will garantie you are hated by every one except orcs",
"20 Orcs loved you.
10 Orcs simpatized with you.
-20 Orcs hated you.
-10 Orcs where annoyed by you.
","1.000");
INSERT INTO factions VALUES("2","merchants","faction merchants will offer you a nice deal from time to time",
"20 Merchants find you invaluable to find and fix bugs.
10 Merchants are starting to like you.
-20 Merchants have hired weltall to kill you because of your bugs.
-10 Merchants don't feel good when you are around.
","1.000");
INSERT INTO factions VALUES("3","liberals","faction liberals will make you popular by all those who value freedom and such stuff",
"20 Liberals think you are open minded and worth talking to.
10 Liberals view you as someone they can occasionally count on.
-20 Liberals see you as the opposition.
-10 Liberals think you are too sympathetic to their enemies to be trusted.
","2.000");
INSERT INTO factions VALUES("4","conservatives","faction conservatives are for those who value tradition and steady life style",
"20 Conservatives put their faith in you.
10 Conservatives look to you with hope you'll one day be dependable to them.
-20 Conservatives see you as an enemy of their ideology.
-10 Conservatives think you are too easily influenced by their enemies to be counted on.
","2.000");
