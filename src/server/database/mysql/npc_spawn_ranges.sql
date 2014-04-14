# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'npc_spawn_ranges'
#

DROP TABLE IF EXISTS `npc_spawn_ranges`;
CREATE TABLE npc_spawn_ranges (
  id int(10) unsigned NOT NULL auto_increment,
  npc_spawn_rule_id int(10) unsigned NOT NULL DEFAULT '0' ,
  x1 float(10,2) NOT NULL DEFAULT '0.00' ,
  y1 float(10,2) NOT NULL DEFAULT '0.00' ,
  z1 float(10,2) NOT NULL DEFAULT '0.00' ,
  x2 float(10,2) NOT NULL DEFAULT '0.00' ,
  y2 float(10,2) NOT NULL DEFAULT '0.00' ,
  z2 float(10,2) NOT NULL DEFAULT '0.00' ,
  radius float(10,2) DEFAULT '0.00' ,
  sector_id int(10) NOT NULL DEFAULT '0' ,
  range_type_code char(1) NOT NULL DEFAULT 'A' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'npc_spawn_ranges'
#

INSERT INTO npc_spawn_ranges VALUES("1","100","-65.00","0.00","-415.00","0.00","0.00","0.00", "7.00", "3","C");
INSERT INTO npc_spawn_ranges VALUES("2","101","-36.00","0.00","-446.00","-55.00","0.00","-417.00", "0.00", "3","A");
INSERT INTO npc_spawn_ranges VALUES("3","102","-64.00","0.00","-401.00","-42.00","0.00","-379.00", "3.00", "3","L");
INSERT INTO npc_spawn_ranges VALUES("4","103","-65.00","0.00","-415.00","0.00","0.00","0.00", "7.00", "3","C");
INSERT INTO npc_spawn_ranges VALUES("5","103","-36.00","0.00","-446.00","-55.00","0.00","-417.00", "0.00", "3","A");
INSERT INTO npc_spawn_ranges VALUES("6","103","-64.00","0.00","-401.00","-42.00","0.00","-379.00", "3.00", "3","L");
