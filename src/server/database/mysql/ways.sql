# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'ways'
#

DROP TABLE IF EXISTS `ways`;
CREATE TABLE ways (
  id int(8) unsigned NOT NULL auto_increment,
  `name` varchar(20) NOT NULL DEFAULT '' COMMENT 'The name of the way',
  `skill` int(11) NOT NULL COMMENT 'The skill related to this way',
  `related_stat` int(11) NOT NULL COMMENT 'The stat skill which has influence on this way',
  PRIMARY KEY (id)
);


#
# Dumping data for table 'ways'
#

INSERT INTO ways VALUES("1","Crystal",11,47);	# Stat skill Charisma
INSERT INTO ways VALUES("2","Azure",12,51);		# Stat skill Will
INSERT INTO ways VALUES("3","Red",14,51);		# Stat skill Will
INSERT INTO ways VALUES("4","Dark",16,47);		# Stat skill Charisma
INSERT INTO ways VALUES("5","Brown",15,49);		# Stat skill Inteligence
INSERT INTO ways VALUES("6","Blue",13,49);		# Stat skill Inteligence
