#
# Table structure for table sc_tribe_assets
#

DROP TABLE IF EXISTS `sc_tribe_assets`;
CREATE TABLE `sc_tribe_assets`
(
  `id` int(10) NOT NULL auto_increment COMMENT 'IDs for assets',
  `tribe_id` int(10) NOT NULL default '0',
  `name` varchar(30) NOT NULL default '',
  `type` int(2) NOT NULL default '0',
  `coordX` float(5) NOT NULL default '0',
  `coordY` float(5) NOT NULL default '0',
  `coordZ` float(5) NOT NULL default '0',
  `sector_id` int(10) NOT NULL default '3',
  `itemID` int(10) NOT NULL default '0',
  `quantity` int(10) NOT NULL default '0' COMMENT '-1 if its a building',
  `status` int(1) NOT NULL default '0',
  PRIMARY KEY (`id`)
);

#
# Dumping data for table sc_tribe_assets
# 
