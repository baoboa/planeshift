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
-- Definition of table `npc_spawn_rules`
--

DROP TABLE IF EXISTS `npc_spawn_rules`;
CREATE TABLE `npc_spawn_rules` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `min_spawn_time` int(10) unsigned NOT NULL default '0',
  `max_spawn_time` int(10) unsigned NOT NULL default '0',
  `substitute_spawn_odds` float(6,4) default '0.0000',
  `substitute_player` int(10) unsigned default '0',
  `fixed_spawn_x` float(10,2) default NULL,
  `fixed_spawn_y` float(10,2) default NULL,
  `fixed_spawn_z` float(10,2) default NULL,
  `fixed_spawn_rot` float(6,4) default '0.0000',
  `fixed_spawn_sector` varchar(40) default 'room',
  `fixed_spawn_instance` int(10) unsigned NOT NULL default '0',
  `loot_category_id` int(10) unsigned NOT NULL default '0',
  `dead_remain_time` int(10) unsigned NOT NULL default '0',
  `min_spawn_spacing_dist` float(10,2) default '0.0',
  `name` varchar(40) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=1000 DEFAULT CHARSET=latin1;

--
-- Dumping data for table `npc_spawn_rules`
--

/*!40000 ALTER TABLE `npc_spawn_rules` DISABLE KEYS */;
INSERT INTO npc_spawn_rules VALUES (1,10000,20000,0.0000,0,0.00,0.00,0.00,0.0000,'startlocation',0,0,30000,'0.00','Respawn at Orig Location');
INSERT INTO npc_spawn_rules VALUES (100,4000,7000,0.0000,0,0.00,0.00,0.00,0.0000,'',0,0,2000,'2.00','Circle between trees');
INSERT INTO npc_spawn_rules VALUES (101,3000,6000,0.0000,0,0.00,0.00,0.00,0.0000,'',0,0,3000,'3.00','Area between tree, box, and wall');
INSERT INTO npc_spawn_rules VALUES (102,2000,5000,0.0000,0,0.00,0.00,0.00,0.0000,'',0,0,4000,'3.00','Line between tree and end of box');
INSERT INTO npc_spawn_rules VALUES (103,2000,4000,0.0000,0,0.00,0.00,0.00,0.0000,'',0,0,2000,'5.00','Combination of all areas');
/*!40000 ALTER TABLE `npc_spawn_rules` ENABLE KEYS */;




/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
