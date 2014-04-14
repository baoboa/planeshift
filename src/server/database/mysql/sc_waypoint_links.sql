# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'sc_waypoint_links'
#

DROP TABLE IF EXISTS `sc_waypoint_links`;
CREATE TABLE `sc_waypoint_links` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `name` varchar(30) NOT NULL DEFAULT '' ,
  `type` varchar(30) NOT NULL DEFAULT '' ,
  `wp1` int(8) unsigned NOT NULL default '0',
  `wp2` int(8) unsigned NOT NULL default '0',
  `flags` varchar(30) DEFAULT '' ,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table 'sc_waypoint_lists'
#
INSERT INTO `sc_waypoint_links` VALUES (1, '', 'LINEAR' ,14, 13, '');
INSERT INTO `sc_waypoint_links` VALUES (2, '', 'LINEAR' ,1, 2, '');
INSERT INTO `sc_waypoint_links` VALUES (3, '', 'LINEAR' ,1, 6, '');
INSERT INTO `sc_waypoint_links` VALUES (4, '', 'LINEAR' ,2, 35, '');
INSERT INTO `sc_waypoint_links` VALUES (5, '', 'LINEAR' ,20, 19, 'ONEWAY');
INSERT INTO `sc_waypoint_links` VALUES (6, 'Behind Blacksmith', 'LINEAR' ,3, 4, '');
INSERT INTO `sc_waypoint_links` VALUES (7, '', 'LINEAR' ,3, 11, '');
INSERT INTO `sc_waypoint_links` VALUES (8, '', 'LINEAR' ,4, 5, '');
INSERT INTO `sc_waypoint_links` VALUES (9, '', 'LINEAR' ,6, 5, 'ONEWAY');
INSERT INTO `sc_waypoint_links` VALUES (10, '', 'LINEAR' ,8, 5, '');
INSERT INTO `sc_waypoint_links` VALUES (11, '', 'LINEAR' ,8, 7, '');
INSERT INTO `sc_waypoint_links` VALUES (12, '', 'LINEAR' ,9, 7, '');
INSERT INTO `sc_waypoint_links` VALUES (13, '', 'LINEAR' ,10, 7, '');
INSERT INTO `sc_waypoint_links` VALUES (14, '', 'LINEAR' ,12, 73, '');
INSERT INTO `sc_waypoint_links` VALUES (15, '', 'LINEAR' ,6, 13, '');
INSERT INTO `sc_waypoint_links` VALUES (16, '', 'LINEAR' ,13, 10, '');
INSERT INTO `sc_waypoint_links` VALUES (17, '', 'LINEAR' ,3, 15, '');
INSERT INTO `sc_waypoint_links` VALUES (18, '', 'LINEAR' ,15, 16, 'NO_WANDER');
INSERT INTO `sc_waypoint_links` VALUES (19, '', 'LINEAR' ,16, 17, '');
INSERT INTO `sc_waypoint_links` VALUES (20, '', 'LINEAR' ,18, 17, '');
INSERT INTO `sc_waypoint_links` VALUES (21, '', 'LINEAR' ,19, 5, '');
INSERT INTO `sc_waypoint_links` VALUES (22, '', 'LINEAR' ,2, 20, '');
INSERT INTO `sc_waypoint_links` VALUES (23, 'Test Path 1', 'LINEAR' ,14, 14, 'ONEWAY');
INSERT INTO `sc_waypoint_links` VALUES (24, '', 'LINEAR' ,1, 21, '');
INSERT INTO `sc_waypoint_links` VALUES (25, '', 'LINEAR' ,21, 22, '');
INSERT INTO `sc_waypoint_links` VALUES (26, '', 'LINEAR' ,21, 23, '');
INSERT INTO `sc_waypoint_links` VALUES (27, '', 'LINEAR' ,22, 24, '');
INSERT INTO `sc_waypoint_links` VALUES (28, '', 'LINEAR' ,23, 24, '');
INSERT INTO `sc_waypoint_links` VALUES (29, '', 'LINEAR' ,24, 25, '');
INSERT INTO `sc_waypoint_links` VALUES (30, '', 'LINEAR' ,25, 26, '');
INSERT INTO `sc_waypoint_links` VALUES (31, '', 'LINEAR' ,26, 23, '');
INSERT INTO `sc_waypoint_links` VALUES (32, '', 'LINEAR' ,26, 28, '');
INSERT INTO `sc_waypoint_links` VALUES (33, '', 'LINEAR' ,28, 27, '');
INSERT INTO `sc_waypoint_links` VALUES (34, '', 'LINEAR' ,25, 27, '');
INSERT INTO `sc_waypoint_links` VALUES (35, '', 'LINEAR' ,13, 32, '');
INSERT INTO `sc_waypoint_links` VALUES (36, '', 'LINEAR' ,11, 34, '');
INSERT INTO `sc_waypoint_links` VALUES (37, '', 'LINEAR' ,13, 31, '');
INSERT INTO `sc_waypoint_links` VALUES (38, '', 'LINEAR' ,9, 30, '');
INSERT INTO `sc_waypoint_links` VALUES (39, '', 'LINEAR' ,62, 33, '');
INSERT INTO `sc_waypoint_links` VALUES (40, '', 'LINEAR' ,29, 35, '');
INSERT INTO `sc_waypoint_links` VALUES (41, '', 'LINEAR' ,35, 3, '');
INSERT INTO `sc_waypoint_links` VALUES (42, '', 'LINEAR' ,11, 15, '');
INSERT INTO `sc_waypoint_links` VALUES (43, '', 'LINEAR' ,11, 21, '');
INSERT INTO `sc_waypoint_links` VALUES (44, '', 'LINEAR' ,14, 36, '');
INSERT INTO `sc_waypoint_links` VALUES (45, '', 'LINEAR' ,36, 37, '');
INSERT INTO `sc_waypoint_links` VALUES (46, '', 'LINEAR' ,37, 38, '');
INSERT INTO `sc_waypoint_links` VALUES (47, '', 'LINEAR' ,38, 39, '');
INSERT INTO `sc_waypoint_links` VALUES (48, '', 'LINEAR' ,39, 40, '');
INSERT INTO `sc_waypoint_links` VALUES (49, '', 'LINEAR' ,40, 41, '');
INSERT INTO `sc_waypoint_links` VALUES (50, '', 'LINEAR' ,41, 13, '');
INSERT INTO `sc_waypoint_links` VALUES (51,'','LINEAR',18,42,'');
INSERT INTO `sc_waypoint_links` VALUES (52,'','LINEAR',42,43,'');
INSERT INTO `sc_waypoint_links` VALUES (53,'','LINEAR',43,44,'');
INSERT INTO `sc_waypoint_links` VALUES (54,'','LINEAR',44,45,'');
INSERT INTO `sc_waypoint_links` VALUES (55,'','LINEAR',45,46,'');
INSERT INTO `sc_waypoint_links` VALUES (56,'','LINEAR',46,47,'');
INSERT INTO `sc_waypoint_links` VALUES (57,'','LINEAR',47,48,'');
INSERT INTO `sc_waypoint_links` VALUES (58,'','LINEAR',48,49,'');
INSERT INTO `sc_waypoint_links` VALUES (59,'','LINEAR',49,50,'');
INSERT INTO `sc_waypoint_links` VALUES (60,'','LINEAR',50,51,'');
INSERT INTO `sc_waypoint_links` VALUES (61,'','LINEAR',51,52,'');
INSERT INTO `sc_waypoint_links` VALUES (62,'','LINEAR',52,53,'');
INSERT INTO `sc_waypoint_links` VALUES (63,'','LINEAR',53,44,'');
INSERT INTO `sc_waypoint_links` VALUES (64,'','LINEAR',44,54,'');
INSERT INTO `sc_waypoint_links` VALUES (65,'','LINEAR',54,55,'');
INSERT INTO `sc_waypoint_links` VALUES (66,'','LINEAR',55,56,'');
INSERT INTO `sc_waypoint_links` VALUES (67,'','LINEAR',56,57,'');
INSERT INTO `sc_waypoint_links` VALUES (68,'','LINEAR',57,58,'');
INSERT INTO `sc_waypoint_links` VALUES (69,'','LINEAR',58,59,'');
INSERT INTO `sc_waypoint_links` VALUES (70,'','LINEAR',59,60,'');
INSERT INTO `sc_waypoint_links` VALUES (71,'','LINEAR',60,61,'');
INSERT INTO `sc_waypoint_links` VALUES (72,'','LINEAR',61,43,'');
INSERT INTO `sc_waypoint_links` VALUES (73,'','LINEAR',12,62,'');
INSERT INTO `sc_waypoint_links` VALUES (74,'','LINEAR',62,63,'');
INSERT INTO `sc_waypoint_links` VALUES (75,'','LINEAR',63,64,'');
INSERT INTO `sc_waypoint_links` VALUES (76,'','LINEAR',64,65,'TELEPORT');
INSERT INTO `sc_waypoint_links` VALUES (77,'','LINEAR',65,66,'');

INSERT INTO `sc_waypoint_links` VALUES (78,'','LINEAR',63,67,'');
INSERT INTO `sc_waypoint_links` VALUES (79,'','LINEAR',67,68,'');
INSERT INTO `sc_waypoint_links` VALUES (80,'','LINEAR',68,69,'');
INSERT INTO `sc_waypoint_links` VALUES (81,'','LINEAR',69,70,'');
INSERT INTO `sc_waypoint_links` VALUES (82,'','LINEAR',67,71,'');
INSERT INTO `sc_waypoint_links` VALUES (83,'','LINEAR',71,69,'');
INSERT INTO `sc_waypoint_links` VALUES (84,'','LINEAR',72,73,'');
INSERT INTO `sc_waypoint_links` VALUES (85,'','LINEAR',72,10,'');
INSERT INTO `sc_waypoint_links` VALUES (86,'','LINEAR',9,70,'');
INSERT INTO `sc_waypoint_links` VALUES (87,'','LINEAR',66,74,'');
INSERT INTO `sc_waypoint_links` VALUES (88,'','LINEAR',74,75,'');
INSERT INTO `sc_waypoint_links` VALUES (89,'','LINEAR',75,76,'');
INSERT INTO `sc_waypoint_links` VALUES (90,'','LINEAR',76,77,'');
INSERT INTO `sc_waypoint_links` VALUES (91,'','LINEAR',77,78,'');
INSERT INTO `sc_waypoint_links` VALUES (92,'','LINEAR',66,79,'');
INSERT INTO `sc_waypoint_links` VALUES (93,'','LINEAR',76,80,'');


