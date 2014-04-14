#
# Table structure for table sc_tribe_resources
#

DROP TABLE IF EXISTS `sc_tribe_resources`;
CREATE TABLE `sc_tribe_resources` 
(
  `id` int(10) NOT NULL auto_increment,
  `tribe_id` int(10) NOT NULL,
  `name` varchar(30) NOT NULL default '',
  `nick` varchar(30) NOT NULL default '',
  `amount` int(10) DEFAULT '0' ,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table sc_tribe_memories
#
