# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'familiar_types'
#

DROP TABLE IF EXISTS `familiar_types`;
CREATE TABLE `familiar_types`
(
	`id` INT UNSIGNED NOT NULL ,
	`name` VARCHAR(30) NOT NULL,
	`type` VARCHAR(30) NOT NULL,
	`lifecycle` VARCHAR(30) NOT NULL,
	`attacktool` VARCHAR(30) NOT NULL,
	`attacktype` VARCHAR(30) NOT NULL,
	`magicalaffinity` VARCHAR(30) NOT NULL,
	`vision` INT NULL,   /* 1 = enhanced, 0 = normal, -1 = limited */
	`speed` INT NULL,    /* 1 = enhanced, 0 = normal, -1 = limited */
	`hearing` INT NULL,  /* 1 = enhanced, 0 = normal, -1 = limited */
	PRIMARY KEY (`id`)
) ENGINE=MyISAM ;

#
# Dumping data for table 'familiar_types'
#

INSERT INTO `familiar_types` VALUES ( 50, 'Bungie', 'animal',    'daylight', 'teeth',   'piercing', 'none', 0, 0, 0 );
INSERT INTO `familiar_types` VALUES ( 51, 'Pyreal', 'mineral',   'daylight', 'osmosis', 'poison',   'none', 0, 0, 0 );
INSERT INTO `familiar_types` VALUES ( 52, 'Bloom',  'elemental', 'daylight', 'touch',   'acid',     'none', 0, 0, 0 );
