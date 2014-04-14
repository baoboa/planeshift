-- MySQL dump 10.11
--
-- Host: localhost    Database: planeshift
-- ------------------------------------------------------
-- Server version	5.0.37-community-nt

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `natural_resources`
--

DROP TABLE IF EXISTS `natural_resources`;
CREATE TABLE `natural_resources` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `loc_sector_id` int(10) unsigned NOT NULL default '0',
  `loc_x` float(10,2) default '0.00' COMMENT 'x coord of center',
  `loc_y` float(10,2) default '0.00' COMMENT 'y coord of center',
  `loc_z` float(10,2) default '0.00' COMMENT 'z coord of center',
  `radius` float(10,2) default '0.00' COMMENT 'radius of the resource',
  `visible_radius` float(10,2) default '0.00'  COMMENT 'allow harvesting but with no result',
  `probability` float(10,6) default '0.000000' COMMENT 'chance of finding an item by harvesting',
  `skill` smallint(4) unsigned default '0' COMMENT 'skill ID used to harvest',
  `skill_level` int(3) unsigned default '0' COMMENT 'skill rank required to harvest',
  `item_cat_id` int(8) unsigned default '0' COMMENT 'item category required for harvesting',
  `item_quality` float(10,6) default '0.000000' COMMENT '???',
  `animation` varchar(30) default NULL COMMENT 'animation played while harvesting',
  `anim_duration_seconds` int(10) unsigned default '0' COMMENT 'animation duration',
  `item_id_reward` int(10) unsigned default '0' COMMENT 'item given to player when successful',
  `reward_nickname` varchar(30) default '0' COMMENT 'name of the item given as used in commands and maps',
  `action` varchar(45) NOT NULL COMMENT 'command needed to harvest: dig/harvest/fish',  `amount` int(10) unsigned default NULL COMMENT 'if not null, items will be spawned as hunt_location',
  `interval` int(11) NOT NULL default '0' COMMENT 'if amount not null, msec interval for spawning item when picked up',
  `max_random` int(11) NOT NULL default '0' COMMENT 'Maximum random interval modifier in msecs',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=8 DEFAULT CHARSET=latin1;

--
-- Dumping data for table `natural_resources`
--

LOCK TABLES `natural_resources` WRITE;
/*!40000 ALTER TABLE `natural_resources` DISABLE KEYS */;
INSERT INTO `natural_resources` VALUES (1,3,-80.00,0.00,-145.00,40.00,50.00,0.990000,37,50,6,0.500000,'greet',10,85,'gold','dig',NULL,0,0);
INSERT INTO `natural_resources` VALUES (2,3,-93.00,0.00,-212.00,40.00,50.00,0.990000,37,50,6,0.500000,'greet',10,103,'coal','dig',NULL,0,0);
INSERT INTO `natural_resources` VALUES (3,3,-93.00,0.00,-186.00,40.00,50.00,0.990000,37,50,6,0.500000,'greet',10,9002,'silver','dig',NULL,0,0);
INSERT INTO `natural_resources` VALUES (4,3,-37.00,0.00,-212.00,20.00,50.00,0.990000,37,50,6,0.500000,'greet',10,9003,'tin','dig',NULL,0,0);
INSERT INTO `natural_resources` VALUES (5,3,-28.00,0.00,-170.00,20.00,50.00,0.990000,37,50,6,0.500000,'greet',10,9004,'platinum','dig',NULL,0,0);

-- Natural resource with hunt location spawn
INSERT INTO `natural_resources` VALUES (6,3,28.00,0.00,-165.00,10.00,12.00,0.990000,37,1,6,0.500000,'greet',10,85,'gold','dig',3,4000,0);

/*!40000 ALTER TABLE `natural_resources` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2007-09-27 10:47:41
