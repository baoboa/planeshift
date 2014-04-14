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
# Table structure for table server_options
#

DROP TABLE IF EXISTS `server_options`;
CREATE TABLE `server_options` (
  `option_name` varchar(50) NOT NULL default '',
  `option_value` varchar(90) NOT NULL default '',
  PRIMARY KEY  (`option_name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

#
# Dumping data for table server_options
#

INSERT INTO `server_options` VALUES ('db_version','1277');
INSERT INTO `server_options` VALUES ('game_time','15:00');
INSERT INTO `server_options` VALUES ('game_date','100-1-1');
INSERT INTO `server_options` VALUES ('instruments_category','27');
INSERT INTO `server_options` VALUES ('standard_motd','This is the message of the day from server_options table.');
INSERT INTO `server_options` VALUES('tutorial:sectorname', 'tutorial');
INSERT INTO `server_options` VALUES('tutorial:sectorx', '-225.37');
INSERT INTO `server_options` VALUES('tutorial:sectory', '-21.32');
INSERT INTO `server_options` VALUES('tutorial:sectorz', '26.79');
INSERT INTO `server_options` VALUES('tutorial:sectoryrot', '-2.04');
INSERT INTO `server_options` VALUES('death:sectorname', 'DR01');
INSERT INTO `server_options` VALUES('death:sectorx', '-29.2');
INSERT INTO `server_options` VALUES('death:sectorz', '28.2');
INSERT INTO `server_options` VALUES('death:sectory', '-119.0');
INSERT INTO `server_options` VALUES('death:sectoryrot', '0.00');
INSERT INTO `server_options` VALUES('death:avoidtext', '');
INSERT INTO `server_options` VALUES('death:avoidtime', '0');
INSERT INTO `server_options` VALUES('npcmanager:petskill', '31');
INSERT INTO `server_options` VALUES('combat:default_stance', 'normal');
INSERT INTO `server_options` VALUES('progression:requiretraining', '0');
INSERT INTO `server_options` VALUES('progression:maxskillvalue', '200');
INSERT INTO `server_options` VALUES('progression:maxstatvalue', '400');



/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
