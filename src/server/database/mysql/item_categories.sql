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
# Table structure for table item_categories
#

DROP TABLE IF EXISTS `item_categories`;
CREATE TABLE `item_categories` (
  `category_id` int(8) unsigned NOT NULL auto_increment,
  `name` varchar(20) NOT NULL default '',
  `item_stat_id_repair_tool` int(11) default NULL,
  `is_repair_tool_consumed` char(1) default 'Y',
  `skill_id_repair` int(11) default NULL,
  `repair_difficulty_pct` int(3) unsigned NOT NULL DEFAULT '100' COMMENT 'Difficulty amount when repairing this item category (passed to the repair scripts)',
  `identify_skill_id` int(10),
  `identify_min_skill` int(8) unsigned,
  PRIMARY KEY  (`category_id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

#
# Dumping data for table item_categories
#

INSERT INTO `item_categories` VALUES (1,'Weapons',531,'Y',33,100,40,50);
INSERT INTO `item_categories` VALUES (2,'Armor',462,'Y',30,100,0,0);
INSERT INTO `item_categories` VALUES (3,'Items',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (4,'Gems',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (5,'Glyphs',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (6,'Mining Tools',NULL,'Y',33,100,0,0);
INSERT INTO `item_categories` VALUES (7,'Potions',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (8,'Kitchen Tools',NULL,'Y',33,100,0,0);
INSERT INTO `item_categories` VALUES (9,'Smith Tools',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (10,'Artist Tools',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (11,'Farmer Tools',NULL,'Y',33,100,0,0);
INSERT INTO `item_categories` VALUES (12,'Food',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (13,'Books',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (14,'Food Recipes',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (15,'Crafting Techniques',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (16,'Raw Materials',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (17,'Shield Parts',NULL,'Y',30,100,0,0);
INSERT INTO `item_categories` VALUES (18,'Weapon Parts',NULL,'Y',33,100,0,0);
INSERT INTO `item_categories` VALUES (19,'Armor Parts',NULL,'Y',30,100,0,0);
INSERT INTO `item_categories` VALUES (20,'Jewelry',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (21,'Coins',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (22,'Animal Parts',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (23,'Hides',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (24,'Quest Items',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (25,'Furniture',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (26,'Traps',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (27,'Instruments',NULL,'Y',NULL,100,0,0);
INSERT INTO `item_categories` VALUES (51,'Structures',NULL,'Y',NULL,100,0,0);

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
