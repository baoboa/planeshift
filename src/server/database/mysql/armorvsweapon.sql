# MySQL-Front 3.2  (Build 13.0)

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES */;

/*!40101 SET NAMES latin1 */;
/*!40103 SET TIME_ZONE='SYSTEM' */;

# Host: localhost    Database: planeshift
# ------------------------------------------------------
# Server version 5.0.19-nt

#
# Table structure for table armor_vs_weapon
#

DROP TABLE IF EXISTS `armor_vs_weapon`;
CREATE TABLE `armor_vs_weapon` (
  `id` int(5) NOT NULL auto_increment,
  `1a` float NOT NULL default '0',
  `1b` float NOT NULL default '0',
  `1c` float NOT NULL default '0',
  `1d` float NOT NULL default '0',
  `2a` float NOT NULL default '0',
  `2b` float NOT NULL default '0',
  `2c` float NOT NULL default '0',
  `3a` float NOT NULL default '0',
  `3b` float NOT NULL default '0',
  `3c` float NOT NULL default '0',
  `weapon_type` varchar(20) NOT NULL default '',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Dumping data for table armor_vs_weapon
#

INSERT INTO `armor_vs_weapon` VALUES (1,1,1,1,1,0.9,0.9,0.9,0.5,0.5,0.5,'Dagger');
INSERT INTO `armor_vs_weapon` VALUES (2,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,'Sabre');
INSERT INTO `armor_vs_weapon` VALUES (3,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.95,'Claymore');
INSERT INTO `armor_vs_weapon` VALUES (4,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.95,'Axe');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
