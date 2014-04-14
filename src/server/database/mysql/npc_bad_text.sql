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
# Table structure for table npc_bad_text
#

DROP TABLE IF EXISTS `npc_bad_text`;
CREATE TABLE `npc_bad_text` (
  `id` int(10) NOT NULL auto_increment,
  `badtext` varchar(255) NOT NULL default '0',
  `triggertext` varchar(255) default NULL,
  `player` varchar(50) default '0',
  `npc` varchar(50) default '0',
  `occurred` datetime default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

#
# Dumping data for table npc_bad_text
#

INSERT INTO `npc_bad_text` VALUES (12,'whatever',NULL,'guest','Orc Pawn','2002-09-15 03:05:43');
INSERT INTO `npc_bad_text` VALUES (13,'hello',NULL,'Vengeance','MaleEnki','2003-07-10 13:11:20');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
