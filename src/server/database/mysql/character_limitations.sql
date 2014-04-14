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
# Table structure for table character_limitations
#

DROP TABLE IF EXISTS `character_limitations`;
CREATE TABLE `character_limitations` (
  `id` int(11) NOT NULL auto_increment,
  `limit_type` char(6) NOT NULL default '' COMMENT '3 letter abbrev of type of limit, such as "sketch"',
  `player_score` int(11) NOT NULL default '0' COMMENT 'Compared to mathscript score for player to decide in or out',
  `value` varchar(30) NOT NULL default '' COMMENT 'String needed by engine to enable the feature',
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='Allows subsetting of strings based on type and rp score';

#
# Dumping data for table character_limitations
#

INSERT INTO `character_limitations` VALUES (1,'SKETCH',1,'city_01');
INSERT INTO `character_limitations` VALUES (2,'SKETCH',20,'city_02');
INSERT INTO `character_limitations` VALUES (3,'SKETCH',1,'con_01');
INSERT INTO `character_limitations` VALUES (4,'SKETCH',5,'con_02');
INSERT INTO `character_limitations` VALUES (5,'SKETCH',10,'con_03');
INSERT INTO `character_limitations` VALUES (6,'SKETCH',15,'con_04');
INSERT INTO `character_limitations` VALUES (7,'SKETCH',20,'con_05');
INSERT INTO `character_limitations` VALUES (8,'SKETCH',25,'con_06');
INSERT INTO `character_limitations` VALUES (9,'SKETCH',1,'cultivated_01');
INSERT INTO `character_limitations` VALUES (10,'SKETCH',1,'danger_01');
INSERT INTO `character_limitations` VALUES (11,'SKETCH',1,'dec_01');
INSERT INTO `character_limitations` VALUES (12,'SKETCH',5,'dec_02');
INSERT INTO `character_limitations` VALUES (13,'SKETCH',10,'dec_03');
INSERT INTO `character_limitations` VALUES (14,'SKETCH',15,'dec_04');
INSERT INTO `character_limitations` VALUES (15,'SKETCH',20,'dec_05');
INSERT INTO `character_limitations` VALUES (16,'SKETCH',25,'dec_06');
INSERT INTO `character_limitations` VALUES (17,'SKETCH',30,'dec_07');
INSERT INTO `character_limitations` VALUES (18,'SKETCH',0,'grass_01');
INSERT INTO `character_limitations` VALUES (19,'SKETCH',0,'hills_01');
INSERT INTO `character_limitations` VALUES (20,'SKETCH',0,'house_01');
INSERT INTO `character_limitations` VALUES (21,'SKETCH',5,'house_02');
INSERT INTO `character_limitations` VALUES (22,'SKETCH',5,'magic_01');
INSERT INTO `character_limitations` VALUES (23,'SKETCH',10,'magic_02');
INSERT INTO `character_limitations` VALUES (24,'SKETCH',0,'monster_01');
INSERT INTO `character_limitations` VALUES (25,'SKETCH',5,'monster_02');
INSERT INTO `character_limitations` VALUES (26,'SKETCH',10,'monster_03');
INSERT INTO `character_limitations` VALUES (27,'SKETCH',0,'mountains_01');
INSERT INTO `character_limitations` VALUES (28,'SKETCH',0,'palm_01');
INSERT INTO `character_limitations` VALUES (29,'SKETCH',5,'palm_02');
INSERT INTO `character_limitations` VALUES (30,'SKETCH',10,'palm_03');
INSERT INTO `character_limitations` VALUES (31,'SKETCH',15,'palm_04');
INSERT INTO `character_limitations` VALUES (32,'SKETCH',20,'palm_05');
INSERT INTO `character_limitations` VALUES (33,'SKETCH',0,'rolling_01');
INSERT INTO `character_limitations` VALUES (34,'SKETCH',0,'sand_01');
INSERT INTO `character_limitations` VALUES (35,'SKETCH',10,'swamp_01');
INSERT INTO `character_limitations` VALUES (36,'SKETCH',10,'tower_01');
INSERT INTO `character_limitations` VALUES (37,'SKETCH',0,'treasure_01');
INSERT INTO `character_limitations` VALUES (38,'SKETCH',0,'village_01');
INSERT INTO `character_limitations` VALUES (39,'SKETCH',10,'village_02');
INSERT INTO `character_limitations` VALUES (40,'SKETCH',15,'wetland_01');
INSERT INTO `character_limitations` VALUES (41,'SKETCH',0,'x_01');
INSERT INTO `character_limitations` VALUES (42,'SKETCH',5,'x_02');
INSERT INTO `character_limitations` VALUES (43,'SKETCH',10,'x_03');
INSERT INTO `character_limitations` VALUES (44,'SKETCH',15,'x_04');
INSERT INTO `character_limitations` VALUES (45,'SKETCH',0,'lair_01');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
