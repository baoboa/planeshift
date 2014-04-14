--
-- Table structure for table `wc_cmdlog`
--

DROP TABLE IF EXISTS `wc_cmdlog`;
CREATE TABLE `wc_cmdlog` (
  `id` int(10) NOT NULL auto_increment,
  `username` varchar(100) NOT NULL,
  `query` text ,
  `date` datetime ,
  UNIQUE KEY `id` (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

