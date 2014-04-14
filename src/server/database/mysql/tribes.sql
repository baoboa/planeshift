#
# Table structure for table tribes
#

#
# Use max_size = -1 for unlimited tribe size.
#

DROP TABLE IF EXISTS `tribes`;
CREATE TABLE `tribes` 
(
  id int(10) NOT NULL auto_increment COMMENT 'The ID of the tribe.',
  name varchar(30) NOT NULL default '',
  home_sector_id int(10) unsigned NOT NULL default '0',
  home_x float(10,2) DEFAULT '0.00' ,
  home_y float(10,2) DEFAULT '0.00' ,
  home_z float(10,2) DEFAULT '0.00' ,
  home_radius float(10,2) DEFAULT '0.00' ,
  max_size int(10) signed DEFAULT '0',
  wealth_resource_name varchar(30) NOT NULL default '',
  wealth_resource_nick varchar(30) NOT NULL default '',
  wealth_resource_area varchar(30) NOT NULL default '',
  wealth_gather_need varchar(30) NOT NULL default '' COMMENT 'The need used to gather resources',
  wealth_resource_growth float(10,2) NOT NULL default '0.00',
  wealth_resource_growth_active float(10,2) NOT NULL default '0.00',
  wealth_resource_growth_active_limit int(10) NOT NULL default '0',
  reproduction_cost int(10) signed DEFAULT '0',
  npc_idle_behavior varchar(30) NOT NULL default 'do nothing',
  tribal_recipe int(5) NOT NULL,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table characters
#

INSERT INTO `tribes` VALUES (1,'Mining tribe',3,-70,0,-175,20,100,'Gold Ore','gold','mine','Dig',1.00,0.0,0,10,'DoNothing', 100);
INSERT INTO `tribes` VALUES (2,'Hunting tribe',3,-75,0,-230,5,5,'Meat','meat','hunting_ground','Hunt',1.00,0.50,100,60,'DoNothing', 101);
