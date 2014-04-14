--
-- Table structure for table `wc_statistics`
--

DROP TABLE IF EXISTS `wc_statistics`;
CREATE TABLE `wc_statistics` (
  `id` int(10) NOT NULL auto_increment,
  `groupid` tinyint(3) NOT NULL,
  `periodname` varchar(50) NOT NULL,
  `result` INT(10) UNSIGNED DEFAULT NULL,
  `param1` INT(10) UNSIGNED DEFAULT NULL,
  UNIQUE KEY `id` (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `wc_statistics`
--


/*!40000 ALTER TABLE `wc_statistics` DISABLE KEYS */;
LOCK TABLES `wc_statistics` WRITE;
INSERT INTO `wc_statistics` VALUES (1,1,'2007 Q1',10,0);
INSERT INTO `wc_statistics` VALUES (2,1,'2007 Q2',20,0);
UNLOCK TABLES;
