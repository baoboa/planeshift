#
# Table structure for table sc_tribe_knowledge
#

DROP TABLE IF EXISTS `sc_tribe_knowledge`;
CREATE TABLE `sc_tribe_knowledge`
(
  `id` int(10) NOT NULL auto_increment,
  `tribe_id` int(10) NOT NULL,
  `knowledge` varchar(1000) NOT NULL default '',
  PRIMARY KEY (`id`)
);

#
# Dumping data for table sc_tribe_knowledge
#
