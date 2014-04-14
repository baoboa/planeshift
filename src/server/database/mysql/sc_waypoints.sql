# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'sc_waypoints'
#

DROP TABLE IF EXISTS `sc_waypoints`;
CREATE TABLE `sc_waypoints` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `name` varchar(30) NOT NULL DEFAULT '' ,
  `wp_group` varchar(30) NOT NULL DEFAULT '' ,
  `x` double(10,2) NOT NULL default '0.00',
  `y` double(10,2) NOT NULL default '0.00',
  `z` double(10,2) NOT NULL default '0.00',
  `radius` int(10) NOT NULL default '0',
  `flags` varchar(100) DEFAULT '' ,
  `loc_sector_id` int(8) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY  (`id`),
  UNIQUE name (name)
);

#
# Dumping data for table 'waypoints'
#
INSERT INTO `sc_waypoints` VALUES (1, 'p1', '', '-45.00', '0.0', '-159.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (2, 'p2', '', '-49.90', '0.0', '-152.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (3, 'p3', '', '-57.30', '0.0', '-142.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (4, 'p4', '', '-66.50', '0.0', '-149.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (5, 'p5', '', '-61.00', '0.0', '-158.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (6, 'p6', '', '-54.00', '0.0', '-163.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (7, 'p7', '', '-75.00', '0.0', '-190.00', 5, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (8, 'p8', '', '-75.00', '0.0', '-170.00', 5, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (9, 'p9', '', '-80.00', '0.0', '-220.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (10, 'p10', '', '-45.00', '0.0', '-210.00', 2, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (11, 'p11', '', '-45.00', '0.0', '-135.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (12, 'p12', '', '30.00', '0.0', '-215.00', 1, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (13, 'tower_1', '', '-45.00', '0.0', '-185.00', 2, '', 3);
INSERT INTO `sc_waypoints` VALUES (14, 'tower_2', '', '-30.00', '0.0', '-185.00', 1, '', 3);
INSERT INTO `sc_waypoints` VALUES (15, 'corr_ent1', '', '-58.00', '0.0', '-127.00', 5, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (16, 'corr_1', '', '-54.00', '0.0', '-93.00', 2, 'ALLOW_RETURN', 4);
INSERT INTO `sc_waypoints` VALUES (17, 'corr_2', '', '-22.00', '0.0', '-88.00', 2, '', 5);
INSERT INTO `sc_waypoints` VALUES (18, 'p13', '', '-20.00', '0.0', '-55.00', 5, '', 6);
INSERT INTO `sc_waypoints` VALUES (19, 'smith_1', '', '-57.56', '0.0', '-156.72', 0, '', 3);
INSERT INTO `sc_waypoints` VALUES (20, 'smith_2', '', '-53.56', '0.0', '-155.46', 0, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (21, 'ug_entrance_1', '', '-11.00', '0.0', '-153.00', 0, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (22, 'ug_entrance_2', '', '-11.00', '0.0', '-170.00', 0, 'ALLOW_RETURN', 3);
INSERT INTO `sc_waypoints` VALUES (23, 'ug_1', '', '0.00', '0.0', '-153.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (24, 'ug_2', '', '0.00', '0.0', '-170.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (25, 'ug_3', '', '15.00', '0.0', '-170.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (26, 'ug_4', '', '15.00', '0.0', '-153.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (27, 'ug_5', '', '30.00', '0.0', '-170.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (28, 'ug_6', '', '30.00', '0.0', '-153.00', 0, 'ALLOW_RETURN, UNDERGROUND', 3);
INSERT INTO `sc_waypoints` VALUES (29, 'MoveMaster5A_home', '', '-56.0', '0.0', '-147.5', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (30, 'MoveMaster5A_work', '', '-61.5', '0.0', '-217.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (31, 'p31', '', '-43.00', '0.0', '-192.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (32, 'tower_3', '', '-40.0', '0.0', '-170.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (33, 'MoveMaster5C_home', '',   '4.5', '0.0', '-239.5', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (34, 'MoveMaster5C_work', '', '-23.0', '0.0', '-125.0', 1, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (35, 'p35', '', '-52.0', '0.0', '-146.5', 1, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (36, 'tower_watch1', 'tower_watch', '-30.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (37, 'tower_watch2', 'tower_watch', '-32.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (38, 'tower_watch3', 'tower_watch', '-34.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (39, 'tower_watch4', 'tower_watch', '-36.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (40, 'tower_watch5', 'tower_watch', '-38.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (41, 'tower_watch6', 'tower_watch', '-40.0', '0.0', '-178.0', 1, 'ALLOW_RETURN, PUBLIC, CITY', 3);
INSERT INTO `sc_waypoints` VALUES (42,'npcr3_000','',-54.15,0.00,-53.54,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (43,'npcr3_001','',-79.49,0.00,-44.22,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (44,'npcr3_002','',-100.87,0.00,-24.17,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (45,'npcr3_003','',-131.01,0.01,-22.90,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (46,'npcrw_000','',-262.57,0.03,-134.25,2,'ALLOW_RETURN, PUBLIC, UNDERGROUND',7);
INSERT INTO `sc_waypoints` VALUES (47,'npcrw_001','',-273.36,0.03,-140.05,2,'ALLOW_RETURN, PUBLIC, UNDERGROUND',7);
INSERT INTO `sc_waypoints` VALUES (48,'npcrw_002','',-285.16,0.03,-133.44,2,'ALLOW_RETURN, PUBLIC, UNDERGROUND',7);
INSERT INTO `sc_waypoints` VALUES (49,'npcr3_004','',-191.43,0.03,32.70,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (50,'npcr3_005','',-179.25,0.03,49.27,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (51,'npcr3_006','',-133.75,0.01,48.38,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (52,'npcr3_007','',-114.41,0.00,48.51,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (53,'npcr3_008','',-108.33,0.00,16.48,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (54,'npcr3_009','',-69.54,0.00,-13.60,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (55,'npcr3_010','',-43.16,0.00,-0.59,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (56,'npcr3_011','',-43.27,0.00,15.31,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (57,'npcr3_012','',-8.75,7.23,10.45,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (58,'npcr3_013','',-10.45,8.23,-5.24,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (59,'npcr3_014','',-4.48,8.23,-18.47,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (60,'npcr3_015','',-2.02,7.22,-36.83,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (61,'npcr3_016','',-28.37,4.21,-36.28,2,'ALLOW_RETURN, PUBLIC',6);
INSERT INTO `sc_waypoints` VALUES (62,'npcr1_013','',6.45,0.01,-230.46, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (63,'npcr1_014','',-6.83,0.01,-230.10, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (64,'npcr1_015','',-7.50,0.01,-239.00, 1, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (65,'npcr1_016','',-7.50,0.01,-244.70, 1, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (66,'npcr1_017','',-8.00,0.01,-250.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (67,'npcr1_018','',-15.00,0.00,-230.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (68,'npcr1_019','',-30.00,0.00,-230.00, 2, 'ALLOW_RETURN, PRIVATE', 3);
INSERT INTO `sc_waypoints` VALUES (69,'npcr1_020','',-45.00,0.00,-230.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (70,'npcr1_021','',-60.00,0.00,-230.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (71,'npcr1_022','',-30.00,0.00,-235.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (72,'p36','',-16.00,0.00,-196.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (73,'p37','',4.00,0.00,-196.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (74,'npcr1_p74','',-16.00,0.00,-250.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (75,'npcr1_p75','',-24.00,0.00,-250.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (76,'npcr1_p76','',-32.00,0.00,-250.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (77,'npcr1_p77','',-40.00,0.00,-250.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (78,'npcr1_p78','',-48.00,0.00,-250.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (79,'npcr1_p79','',10.00,0.00,-260.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
INSERT INTO `sc_waypoints` VALUES (80,'npcr1_p80','',-40.00,0.00,-266.00, 2, 'ALLOW_RETURN, PUBLIC', 3);
