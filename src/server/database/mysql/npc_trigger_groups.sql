# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'npc_trigger_groups'
#

DROP TABLE IF EXISTS `npc_trigger_groups`;
CREATE TABLE npc_trigger_groups (
  id int(10) unsigned NOT NULL auto_increment,
  trigger_text varchar(40) NOT NULL DEFAULT '' ,
  equivalent_to_id int(10) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY (id),
  UNIQUE trigger_text (trigger_text)
);


#
# Dumping data for table 'npc_trigger_groups'
#

INSERT INTO npc_trigger_groups VALUES("1","give me quest","0");
INSERT INTO npc_trigger_groups VALUES("2","do you have quest","1");
INSERT INTO npc_trigger_groups VALUES("3","can I help you","1");
INSERT INTO npc_trigger_groups VALUES("4","what can I do","1");
INSERT INTO npc_trigger_groups VALUES("5","price has doubled","0");
INSERT INTO npc_trigger_groups VALUES("6","it costs double","5");
