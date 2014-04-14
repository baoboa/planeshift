-- MySQL Administrator dump 1.4
--
-- ------------------------------------------------------
-- Server version	5.0.37-community-nt


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;


--
-- Definition of table `race_info`
--

DROP TABLE IF EXISTS `race_info`;
CREATE TABLE `race_info` (
  `id` int(10) NOT NULL DEFAULT '0',
  `name` varchar(35) NOT NULL DEFAULT '',
  `cstr_mesh` varchar(200) NOT NULL DEFAULT '',
  `sex` char(1) NOT NULL DEFAULT 'M',
  `size_x` float unsigned DEFAULT '0',
  `size_y` float unsigned DEFAULT '0',
  `size_z` float unsigned DEFAULT '0',
/* size_z and size_x should be equal since the collision reaction system does not 
   deal with player rotations. Should we remove both size_z and size_x and just keep a size_radius? - Khaki */
  `cstr_base_texture` varchar(200) NOT NULL DEFAULT '',
  `initial_cp` int(10) DEFAULT '0',
  `start_str` int(5) NOT NULL DEFAULT '50',
  `start_end` int(5) NOT NULL DEFAULT '50',
  `start_agi` int(5) NOT NULL DEFAULT '50',
  `start_int` int(5) NOT NULL DEFAULT '50',
  `start_will` int(5) NOT NULL DEFAULT '50',
  `start_cha` int(5) unsigned NOT NULL DEFAULT '50',
  `base_physical_regen_still` float NOT NULL DEFAULT '0',
  `base_physical_regen_walk` float NOT NULL DEFAULT '0',
  `base_mental_regen_still` float NOT NULL DEFAULT '0',
  `base_mental_regen_walk` float NOT NULL DEFAULT '0',
  `armor_id` int(10) unsigned DEFAULT '0',
  `weapon_id` int(10) DEFAULT '0' COMMENT 'The id of the default weapon used in case no weapon is actually equipped. Should be something like the natural claws of this race or hands.',
  `helm` varchar(20) DEFAULT '',
  `bracer` varchar(20) DEFAULT '' COMMENT 'Stores a bracer group allowing to use the same bracer mesh for more than one race, just like for the helm column',
  `belt` varchar(20) DEFAULT '' COMMENT 'Stores a belt group allowing to use the same belt mesh for more than one race, just like for the helm column',
  `cloak` varchar(20) DEFAULT '' COMMENT 'Stores a cloak group allowing to use the same cloak mesh for more than one race, just like for the helm column',
  `cstr_mounter_animation` varchar(200) NOT NULL DEFAULT '' COMMENT 'Defines the animation the mounter of this race will use when mounting it',
  `race` int(5) unsigned NOT NULL,
  `scale` FLOAT  NOT NULL DEFAULT '0' COMMENT 'Defines the scale of the race. Overrides whathever is defined in the cal3d file.',
  `speed_modifier` float NOT NULL DEFAULT '1' COMMENT 'Used as a multiplier of the velocity',
  PRIMARY KEY (`id`) USING BTREE
);

--
-- Dumping data for table `race_info`
--

/*!40000 ALTER TABLE `race_info` DISABLE KEYS */;
INSERT INTO `race_info` VALUES (0,'StoneBreaker','stonebm','M',0.8,1.2,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',0,0.92,1);
INSERT INTO `race_info` VALUES (1,'Enkidukai','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',1,0.92,1);
INSERT INTO `race_info` VALUES (2,'Ynnwn','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',2,0.92,1);
INSERT INTO `race_info` VALUES (3,'Ylian','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',3,0.92,1);
INSERT INTO `race_info` VALUES (4,'Xacha','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',4,0.92,1);
INSERT INTO `race_info` VALUES (5,'Nolthrir','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',5,0.92,1);
INSERT INTO `race_info` VALUES (6,'Dermorian','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.2,0.8,10,10,0,0,'','','','','',6,0.92,1);
INSERT INTO `race_info` VALUES (7,'Hammerwielder','stonebm','M',0.8,1.2,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',7,0.92,1);
INSERT INTO `race_info` VALUES (9,'Kran','stonebm','N',0.8,1.4,0.6,'',100,50,50,50,50,50,50,2,1.5,10,10,0,0,'','','','','',9,0.92,1);
INSERT INTO `race_info` VALUES (10,'Lemur','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',10,0.92,1);
INSERT INTO `race_info` VALUES (11,'Klyros','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'ylianm','','','','',11,0.92,1);
INSERT INTO `race_info` VALUES (12,'Enkidukai','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',1,0.92,1);
INSERT INTO `race_info` VALUES (13,'StoneBreaker','stonebm','F',0.8,1.2,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'ylianm','','','','',0,0.92,1);
INSERT INTO `race_info` VALUES (14,'Ynnwn','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',2,0.92,1);
INSERT INTO `race_info` VALUES (15,'Ylian','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',3,0.92,1);
INSERT INTO `race_info` VALUES (16,'Xacha','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',4,0.92,1);
INSERT INTO `race_info` VALUES (17,'Nolthrir','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',5,0.92,1);
INSERT INTO `race_info` VALUES (18,'Dermorian','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.2,0.8,10,10,0,0,'','','','','',6,0.92,1);
INSERT INTO `race_info` VALUES (19,'Hammerwielder','stonebm','F',0.8,1.2,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',7,0.92,1);
INSERT INTO `race_info` VALUES (21,'Lemur','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',10,0.92,1);
INSERT INTO `race_info` VALUES (22,'Klyros','stonebm','F',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',11,0.92,1);
INSERT INTO `race_info` VALUES (23,'Rogue','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',12,0.92,1);
INSERT INTO `race_info` VALUES (24,'Clacker','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',13,0.92,1);
INSERT INTO `race_info` VALUES (25,'Rat','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',14,0.92,1);
INSERT INTO `race_info` VALUES (26,'Grendol','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',15,0.92,1);
INSERT INTO `race_info` VALUES (27,'Gobble','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',16,0.92,1);
INSERT INTO `race_info` VALUES (28,'Consumer','consumer','N',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',17,0.1,1);
INSERT INTO `race_info` VALUES (29,'Trepor','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',18,0.92,1);
INSERT INTO `race_info` VALUES (30,'Ulbernaut','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',19,0.92,1);
INSERT INTO `race_info` VALUES (31,'Tefusang','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,2,1.5,10,10,0,0,'','','','','',20,0.92,1);
INSERT INTO `race_info` VALUES (32,'Drifter','stonebm','M',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',21,0.92,2);
INSERT INTO `race_info` VALUES (9999,'Special','nullmesh','N',0.8,1.4,0.6,'',100,50,50,50,50,50,50,1.5,1,10,10,0,0,'','','','','',9999,1,1);
/*!40000 ALTER TABLE `race_info` ENABLE KEYS */;




/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
