
--
-- Table structure for table `sc_path_points`
--

DROP TABLE IF EXISTS `sc_path_points`;
CREATE TABLE `sc_path_points` (
  `id` int(8) unsigned NOT NULL auto_increment,
  `path_id` int(8) unsigned NOT NULL default '0',
  `prev_point` int(8) unsigned NOT NULL default '0',
  `x` double(10,2) NOT NULL default '0.00',
  `y` double(10,2) NOT NULL default '0.00',
  `z` double(10,2) NOT NULL default '0.00',
  `loc_sector_id` int(8) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `sc_path_points`
--
INSERT INTO `sc_path_points` VALUES (7,23,0,-26.79,0.00,-183.13,3);
INSERT INTO `sc_path_points` VALUES (8,23,7,-26.64,0.00,-174.72,3);
INSERT INTO `sc_path_points` VALUES (9,23,8,-24.98,0.00,-165.16,3);
INSERT INTO `sc_path_points` VALUES (10,23,9,-16.96,0.00,-164.90,3);
INSERT INTO `sc_path_points` VALUES (11,23,10,-13.49,0.00,-177.05,3);
INSERT INTO `sc_path_points` VALUES (12,23,11,-8.90,0.00,-180.01,3);
INSERT INTO `sc_path_points` VALUES (13,23,12,-8.26,0.38,-180.22,3);
INSERT INTO `sc_path_points` VALUES (14,23,13,-1.61,4.36,-181.57,3);
INSERT INTO `sc_path_points` VALUES (15,23,14,-1.49,4.43,-186.58,3);
INSERT INTO `sc_path_points` VALUES (16,23,15,-8.36,0.31,-188.72,3);
INSERT INTO `sc_path_points` VALUES (17,23,16,-9.39,0.00,-188.86,3);
INSERT INTO `sc_path_points` VALUES (18,23,17,-13.48,0.00,-191.37,3);
INSERT INTO `sc_path_points` VALUES (19,23,18,-25.41,0.00,-189.27,3);
INSERT INTO `sc_path_points` VALUES (20,1,0,-33.00,0.00,-185.13,3);
INSERT INTO `sc_path_points` VALUES (21,1,20,-39.00,0.00,-184.96,3);
INSERT INTO `sc_path_points` VALUES (22,15,0,-54.28,0.00,-168.00,3);
INSERT INTO `sc_path_points` VALUES (23,15,22,-52.76,0.00,-174.08,3);
INSERT INTO `sc_path_points` VALUES (24,15,23,-48.38,0.00,-180.00,3);
INSERT INTO `sc_path_points` VALUES (26,16,0,-47.00,0.00,-193.00,3);
INSERT INTO `sc_path_points` VALUES (27,16,26,-45.00,0.00,-196.44,3);
INSERT INTO `sc_path_points` VALUES (28,16,27,-41.00,0.00,-200.92,3);
INSERT INTO `sc_path_points` VALUES (29,16,28,-41.00,0.00,-205.76,3);
INSERT INTO `sc_path_points` VALUES (30,57,0,-276.10,0.03,-140.15,7);
INSERT INTO `sc_path_points` VALUES (31,57,30,-279.79,0.03,-139.19,7);
INSERT INTO `sc_path_points` VALUES (32,57,31,-283.09,0.03,-136.28,7);
INSERT INTO `sc_path_points` VALUES (33,59,0,-191.31,0.03,40.68,6);
INSERT INTO `sc_path_points` VALUES (34,59,33,-189.29,0.03,45.23,6);
INSERT INTO `sc_path_points` VALUES (35,59,34,-185.19,0.03,47.67,6);
INSERT INTO `sc_path_points` VALUES (36,67,0,-39.99,1.10,19.07,6);
INSERT INTO `sc_path_points` VALUES (37,67,36,-37.02,2.09,19.73,6);
INSERT INTO `sc_path_points` VALUES (38,67,37,-34.91,2.48,18.37,6);
INSERT INTO `sc_path_points` VALUES (39,67,38,-35.25,2.29,13.66,6);
INSERT INTO `sc_path_points` VALUES (40,67,39,-32.84,3.12,9.24,6);
INSERT INTO `sc_path_points` VALUES (41,67,40,-28.81,4.13,9.45,6);
INSERT INTO `sc_path_points` VALUES (42,67,41,-24.71,5.16,11.03,6);
INSERT INTO `sc_path_points` VALUES (43,67,42,-19.36,6.22,13.09,6);
INSERT INTO `sc_path_points` VALUES (44,67,43,-15.84,6.31,11.96,6);
INSERT INTO `sc_path_points` VALUES (45,67,44,-11.81,7.22,11.10,6);
INSERT INTO `sc_path_points` VALUES (46,68,0,-7.20,7.22,8.11,6);
INSERT INTO `sc_path_points` VALUES (47,68,46,-7.10,7.23,5.75,6);
INSERT INTO `sc_path_points` VALUES (48,68,47,-9.00,7.51,4.04,6);
INSERT INTO `sc_path_points` VALUES (49,68,48,-11.44,8.22,1.85,6);
INSERT INTO `sc_path_points` VALUES (50,68,49,-11.32,8.23,-1.63,6);
INSERT INTO `sc_path_points` VALUES (51,71,0,-11.85,5.22,-38.84,6);
INSERT INTO `sc_path_points` VALUES (52,71,51,-18.66,4.20,-39.73,6);
INSERT INTO `sc_path_points` VALUES (53,71,52,-20.29,4.21,-37.38,6);
INSERT INTO `sc_path_points` VALUES (54,71,53,-23.52,4.21,-35.64,6);
INSERT INTO `sc_path_points` VALUES (55,72,0,-30.58,4.20,-36.02,6);
INSERT INTO `sc_path_points` VALUES (56,72,55,-32.46,3.22,-39.72,6);
INSERT INTO `sc_path_points` VALUES (57,72,56,-35.83,3.20,-39.43,6);
INSERT INTO `sc_path_points` VALUES (58,72,57,-37.45,3.20,-36.42,6);
INSERT INTO `sc_path_points` VALUES (59,72,58,-42.62,2.42,-36.65,6);
INSERT INTO `sc_path_points` VALUES (60,72,59,-48.16,1.21,-38.63,6);
INSERT INTO `sc_path_points` VALUES (61,72,60,-52.34,1.18,-36.76,6);
INSERT INTO `sc_path_points` VALUES (62,72,61,-59.24,0.20,-37.82,6);
INSERT INTO `sc_path_points` VALUES (63,72,62,-65.73,0.18,-40.13,6);
