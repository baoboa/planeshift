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
-- Definition of table `race_spawns`
--

DROP TABLE IF EXISTS `race_spawns`;
CREATE TABLE `race_spawns` (
  `raceid` int(11) NOT NULL default '0',
  `x` float NOT NULL default '0',
  `y` float NOT NULL default '0',
  `z` float NOT NULL default '0',
  `yrot` float NOT NULL default '0',
  `range` float unsigned NOT NULL DEFAULT '0',
  `sector_id` int(11) NOT NULL default '3',
  PRIMARY KEY  USING BTREE (`raceid`,`x`,`y`,`z`,`yrot`,`range`,`sector_id`)
);

--
-- Dumping data for table `race_spawns`
--

/*!40000 ALTER TABLE `race_spawns` DISABLE KEYS */;
INSERT INTO `race_spawns` (`raceid`,`x`,`y`,`z`,`yrot`,`range`,`sector_id`) VALUES 
 (0,-20,4,-150,0,0,3),
 (0,0,0,-150,0,0,3),
 (1,2,0,-150,0,0,3),
 (2,4,0,-150,0,0,3),
 (3,-2,0,-150,0,0,3),
 (4,0,0,-150,0,0,3),
 (5,0,0,-150,0,0,3),
 (6,0,0,-150,0,0,3),
 (7,0,0,-150,0,0,3),
 (8,0,0,-150,0,0,3),
 (9,-4,0,-150,0,0,3),
 (10,0,0,-150,0,0,3),
 (11,0,0,-150,0,0,3),
 (12,0,0,-150,0,0,3),
 (13,-20,4,-150,0,0,3),
 (14,-20,4,-150,0,0,3),
 (15,-20,4,-150,0,0,3),
 (16,-20,4,-150,0,0,3),
 (17,-20,4,-150,0,0,3),
 (18,-20,4,-150,0,0,3),
 (19,-20,4,-150,0,0,3),
 (20,-20,4,-150,0,0,3),
 (21,-20,4,-150,0,0,3),
 (9999,-20,4,-150,0,0,3);
/*!40000 ALTER TABLE `race_spawns` ENABLE KEYS */;




/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
