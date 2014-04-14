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
# Table structure for table skills
#

DROP TABLE IF EXISTS `skills`;
CREATE TABLE `skills` (
  `skill_id` int(10) unsigned NOT NULL default '0',
  `name` varchar(35) NOT NULL default '',
  `description` text,
  `practice_factor` int(8) NOT NULL default '100',
  `mental_factor` tinyint(4) default NULL,
  `price` int(8) NOT NULL default '0',
  `base_rank_cost` int(2) NOT NULL default '0',
  `category` varchar(255) NOT NULL default 'VARIOUS',
  `cost_script` varchar(255) NOT NULL DEFAULT 'CalculateSkillCosts' COMMENT 'Stores the script to be run when calculating the costs of this skill.',
  PRIMARY KEY  (`skill_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

#
# Dumping data for table skills
#

INSERT INTO `skills` VALUES (0,'Sword','',80,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (1,'Knives & Daggers','',80,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (2,'Axe','',80,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (3,'Mace & Hammer','',80,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (4,'Melee','',70,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (5,'Polearm & Spear','',80,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (6,'Ranged','',50,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (7,'Light Armor','',60,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (8,'Medium Armor','',60,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (9,'Heavy Armor','',60,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (10,'Shield Handling','',60,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (11,'Crystal Way','',30,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (12,'Azure Way','',30,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (13,'Blue Way','',30,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (14,'Red Way','',30,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (15,'Brown Way','',30,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (16,'Dark Way','',30,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (17,'Assassin Weapon','',60,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (18,'Backstab','',60,80,42,20,'COMBAT', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (19,'Climb','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (20,'Find Traps','',20,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (21,'Hide in Shadows','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (22,'Lockpicking','',30,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (23,'Pickpockets','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (24,'Set Traps','',10,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (25,'Body Development','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (26,'Riding','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (27,'Swimming','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (28,'Alchemy','',60,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (29,'Anti-Magic','',20,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (30,'Repair Armors','',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (31,'Empathy','',60,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (32,'Herbal','',20,80,42,20,'MAGIC', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (33,'Repair Weapons','',20,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (34,'Argan','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (35,'Esteria','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (36,'Lah\'ar','',60,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (37,'Mining','Digging valuable things out of the ground.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (38,'Baking','Baking food items in an oven.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (39,'Cooking','The preparation of food for general comsuption.',50,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (40,'Blacksmith','Working steel and iron into weapons and armor.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (41,'Knife Making','The making of various types of knives.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (42,'Sword Making','The making of various types of swords.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (43,'Axe Making','The making of various types of axes.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (44,'Mace Making','The making of various types of maces.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (45,'Shield Making','The making of various types of shields.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (46,'Agility','',0,80,42,20,'STATS', 'CalculateStatCosts');
INSERT INTO `skills` VALUES (47,'Charisma','',0,80,42,20,'STATS', 'CalculateStatCosts');
INSERT INTO `skills` VALUES (48,'Endurance','',0,80,42,20,'STATS', 'CalculateStatCosts');
INSERT INTO `skills` VALUES (49,'Intelligence','',0,80,42,20,'STATS', 'CalculateStatCosts');
INSERT INTO `skills` VALUES (50,'Strength','',0,80,42,20,'STATS', 'CalculateStatCosts');
INSERT INTO `skills` VALUES (51,'Will','',0,80,42,20,'STATS', 'CalculateStatCosts');
INSERT INTO `skills` VALUES (52,'skill52','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (53,'skill53','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (54,'skill54','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (55,'skill55','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (56,'skill56','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (57,'skill57','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (58,'skill58','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (59,'skill59','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (60,'skill60','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (61,'skill61','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (62,'skill62','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (63,'Metallurgy','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (64,'Armor Making','The making of various types of armor.',60,80,42,20,'JOBS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (65,'Illustration','Drawing and editing sketches, scrolls, maps, signs and poems.',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (66,'skill66','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (67,'skill67','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');
INSERT INTO `skills` VALUES (68,'skill68','',0,80,42,20,'VARIOUS', 'CalculateSkillCosts');

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
