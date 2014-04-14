#
# Table structure for table sc_tribe_memory
#

DROP TABLE IF EXISTS `sc_tribe_memories`;
CREATE TABLE `sc_tribe_memories` 
(
  `id` int(10) NOT NULL auto_increment,
  `tribe_id` int(10) NOT NULL,
  `name` varchar(30) NOT NULL default '',
  `loc_x` float(10,2) DEFAULT '0.00' ,
  `loc_y` float(10,2) DEFAULT '0.00' ,
  `loc_z` float(10,2) DEFAULT '0.00' ,
  `sector_id` int(10) unsigned NOT NULL default '0',
  `radius` float(10,2) DEFAULT '0.00' ,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table sc_tribe_memories
#


