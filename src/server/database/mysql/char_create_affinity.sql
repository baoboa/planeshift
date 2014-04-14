# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'char_create_affinity'
#

DROP TABLE IF EXISTS `char_create_affinity`;
CREATE TABLE char_create_affinity (
  attribute varchar(20) NOT NULL,
  category varchar(20) NOT NULL,
  PRIMARY KEY (category)
);


#
# Dumping data for table 'char_create_affinity'
#

INSERT INTO char_create_affinity VALUES ('Type','arachnid');
INSERT INTO char_create_affinity VALUES ('Type','crustaceans');
INSERT INTO char_create_affinity VALUES ('Type','amphibians');
INSERT INTO char_create_affinity VALUES ('Type','reptiles');
INSERT INTO char_create_affinity VALUES ('Type','feline');
INSERT INTO char_create_affinity VALUES ('Type','birds');
INSERT INTO char_create_affinity VALUES ('Type','rodents');
INSERT INTO char_create_affinity VALUES ('Lifecycle','daylight');
INSERT INTO char_create_affinity VALUES ('Lifecycle','night-darkness');
INSERT INTO char_create_affinity VALUES ('Attack','slashing');
INSERT INTO char_create_affinity VALUES ('Attack','bludgeoning');
INSERT INTO char_create_affinity VALUES ('Attack','piercing');
INSERT INTO char_create_affinity VALUES ('Attack','acid');
INSERT INTO char_create_affinity VALUES ('Attack','fire');
INSERT INTO char_create_affinity VALUES ('Attack','ice');
INSERT INTO char_create_affinity VALUES ('Attack','earth');
INSERT INTO char_create_affinity VALUES ('Attack','water');
