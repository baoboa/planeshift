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
-- Definition of table `sectors`
--

DROP TABLE IF EXISTS `sectors`;
CREATE TABLE  `sectors` (
  `id` smallint(3) unsigned NOT NULL AUTO_INCREMENT,
  `name` varchar(30) NOT NULL COMMENT 'The name of the sector (as defined in map files)',
  `rain_enabled` char(1) NOT NULL DEFAULT 'N' COMMENT 'Sets if automatic rain should trigger. Put Y to enable it, N to disable it.',
  `snow_enabled` char(1) NOT NULL DEFAULT 'N' COMMENT 'Sets if automatic snow should trigger. Put Y to enable it, N to disable it.',
  `fog_enabled` char(1) NOT NULL DEFAULT 'N' COMMENT 'Sets if automatic fog should trigger. Put Y to enable it, N to disable it.',
  `rain_min_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time between two automatically triggered rain events.',
  `rain_max_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time between two automatically triggered rain events.',
  `rain_min_duration` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum duration of an automatically triggered rain event.',
  `rain_max_duration` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum duration of an automatically triggered snow event.',
  `rain_min_drops` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum amount of drops in an automatically triggered rain events.',
  `rain_max_drops` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum amount of drops in an automatically triggered rain events.',
  `rain_min_fade_in` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time the automatically triggered rain event will take to reach the drops amount.',
  `rain_max_fade_in` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time the automatically triggered rain event will take to reach the drops amount.',
  `rain_min_fade_out` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time the automatically triggered event will take to disappear at its end.',
  `rain_max_fade_out` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time the automatically triggered event will take to disappear at its end.',
  `lightning_min_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time between two automatic lightnings during a rain event.',
  `lightning_max_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time between two automatic lightnings during a rain event.',
  `snow_min_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time between two automatically triggered snow events.',
  `snow_max_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time between two automatically triggered snow events.',
  `snow_min_duration` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum duration of an automatically triggered snow event.',
  `snow_max_duration` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum duration of an automatically triggered snow event.',
  `snow_min_flakes` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum amount of flakes in an automatically triggered snow event.',
  `snow_max_flakes` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum amount of flakes in an automatically triggered snow event.',
  `snow_min_fade_in` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time the automatically triggered snow event will take to reach the flakes amount.',
  `snow_max_fade_in` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time the automatically triggered snow event will take to reach the flakes amount.',
  `snow_min_fade_out` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time the automatically triggered snow event will take to disappear at its end.',
  `snow_max_fade_out` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time that the automatically triggered snow event will take to disappear at its end.',
  `fog_min_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time between two automatically triggered fog events.',
  `fog_max_gap` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time between two automatically triggered fog events.',
  `fog_min_duration` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum duration of an automatically triggered fog event.',
  `fog_max_duration` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum duration of an automatically triggered fog event.',
  `fog_min_density` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum density of the fog in an automatically triggered fog event.',
  `fog_max_density` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum density of the fog in an automatically triggered fog event.',
  `fog_min_fade_in` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time that the automatically triggered fog event will take to reach its density.',
  `fog_max_fade_in` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time that the automatically triggered fog event will take to reach its density.',
  `fog_min_fade_out` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the minimum time that the automatically triggered fog event will take to disappear at its end.',
  `fog_max_fade_out` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Sets the maximum time that the automatically triggered fog event will take to disappear at its end.',
  `collide_objects` tinyint(1) NOT NULL DEFAULT '0' COMMENT 'Determines if the items dropped in this sector should have collision detection always enabled.',
  `non_transient_objects` tinyint(4) NOT NULL DEFAULT '0' COMMENT 'Determines if the items dropped in this sector should be transient.',
  `say_range` float(5,2) unsigned NOT NULL DEFAULT '0.00' COMMENT 'Determines the range of say in the specific sector. Set 0.0 to use default.',
  `TeleportingSector` varchar(30) NOT NULL DEFAULT '' COMMENT 'Sector where the player will be teleported automatically when entering this sector',
  `TeleportingCords` varchar(30) NOT NULL DEFAULT '' COMMENT 'Cordinates where the player will be teleported automatically when entering this sector',
  `DeathSector` varchar(30) NOT NULL DEFAULT '' COMMENT 'Sector where the player will be teleported automatically when dieing in this sector',
  `DeathCords` varchar(30) NOT NULL DEFAULT '' COMMENT 'Cordinates where the player will be teleported automatically when dieing in this sector',
  `TeleportingSectorEnable` char(1) NOT NULL DEFAULT 'N' COMMENT 'When not N the sector will teleport when accessing it according to teleportingsector and teleportingcords',
  `TeleportingPenaltyEnable` char(1) NOT NULL DEFAULT 'N' COMMENT 'When not N when teleported when entering this sector it will apply the death penalty',
  `DeathRestoreMana` char(1) NOT NULL DEFAULT 'Y' COMMENT 'When not N the sector will restore mana when the player dies in it, else nothing is done.',
  `DeathRestoreHP` char(1) NOT NULL DEFAULT 'Y' COMMENT 'When not N the sector will restore HP when the player dies in it, else it just sets the minimum to keep him alive and not death loop.',
  `god_name` varchar(45) NOT NULL DEFAULT 'Laanx' COMMENT 'Sets a name of an entity associated to the sector as overseer.',
  PRIMARY KEY (`id`),
  UNIQUE KEY `name` (`name`)
  -- KEY `indx_sector_name` (`name`)
) ENGINE=MyISAM AUTO_INCREMENT=84 DEFAULT CHARSET=latin1 COMMENT='Stores data about sectors available in game.';


--
-- Dumping data for table `sectors`
--

/*!40000 ALTER TABLE `sectors` DISABLE KEYS */;
INSERT INTO `sectors` (`id`,`name`,`rain_enabled`,`snow_enabled`,`fog_enabled`,`rain_min_gap`,`rain_max_gap`,`rain_min_duration`,`rain_max_duration`,`rain_min_drops`,`rain_max_drops`,`rain_min_fade_in`,`rain_max_fade_in`,`rain_min_fade_out`,`rain_max_fade_out`,`lightning_min_gap`,`lightning_max_gap`,`snow_min_gap`,`snow_max_gap`,`snow_min_duration`,`snow_max_duration`,`snow_min_flakes`,`snow_max_flakes`,`snow_min_fade_in`,`snow_max_fade_in`,`snow_min_fade_out`,`snow_max_fade_out`,`fog_min_gap`,`fog_max_gap`,`fog_min_duration`,`fog_max_duration`,`fog_min_density`,`fog_max_density`,`fog_min_fade_in`,`fog_max_fade_in`,`fog_min_fade_out`,`fog_max_fade_out`,`collide_objects`, `non_transient_objects`, `say_range`,`TeleportingSector`, `TeleportingCords`, `DeathSector`, `DeathCords`, `TeleportingSectorEnable`, `TeleportingPenaltyEnable`, `DeathRestoreMana`, `DeathRestoreHP`, `god_name`) VALUES 
 (1,'room','N','N','N',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 10.0, '','','','','N','N', 'Y','Y', 'Laanx'),
 (2,'temple','N','N','N',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 10.0, '','','','','N','N', 'Y','Y', 'Laanx'),
 (3,'NPCroom','N','N','N',15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,4000,4000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,0, 0, 10.0, '','', 'NPCroom','-20.0,1.0,-180.0,0.0','N','N', 'Y','Y', 'Laanx'),
 (4,'NPCroom1','N','N','N',15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,4000,4000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,0, 0, 10.0, '','', 'NPCroom','-20.0,1.0,-180.0,0.0','N','N', 'Y','Y', 'Laanx'),
 (5,'NPCroom2','N','N','N',15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,4000,4000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,0, 0, 10.0, '','', 'NPCroom','-20.0,1.0,-180.0,0.0','N','N', 'Y','Y', 'Laanx'),
 (6,'NPCroom3','N','N','N',15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,4000,4000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,1, 1, 10.0, '','', 'NPCroom','-20.0,1.0,-180.0,0.0','N','N', 'Y','Y', 'Laanx'),
 (7,'NPCroomwarp','N','N','N',15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,4000,4000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,15000,15000,10000,10000,8000,8000,5000,5000,5000,5000,0, 0, 10.0, '','', 'NPCroom','-20.0,1.0,-180.0,0.0','N','N', 'Y','Y', 'Laanx');
/*!40000 ALTER TABLE `sectors` ENABLE KEYS */;




/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
