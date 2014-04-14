# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'trade_processes'
#

DROP TABLE IF EXISTS `trade_processes`;
CREATE TABLE trade_processes (
  process_id int(10) unsigned NOT NULL,
  subprocess_number int(4) unsigned NOT NULL                      COMMENT 'subprocess number',
  name varchar(40) default NULL                                   COMMENT 'process name',
  animation varchar(30)                                           COMMENT 'transformation animation',
  render_effect CHAR(32) default NULL                             COMMENT 'transformation render effect',
  workitem_id int(10) unsigned NOT NULL                           COMMENT 'target item to complete transformation',
  equipment_id int(10) unsigned                                   COMMENT 'required equipted item',
  constraints varchar(64) NOT NULL DEFAULT ''                     COMMENT 'constraints that apply to transformation',
  garbage_id int(10) unsigned                                     COMMENT 'garbage item for flubbed transformations',
  garbage_qty int(8) unsigned                                     COMMENT 'garbage quantity for flubbed transformations',
  primary_skill_id int(10)                                        COMMENT 'primary skill for transformation',
  primary_min_skill int(8) unsigned                               COMMENT 'minimum primary skill level required to perform transformation, 0 is no minimum',
  primary_max_skill int(8) unsigned                               COMMENT 'maximum primary skill level at which practice and quality are effected, 0 is no maximum',
  primary_practice_points int(4)                                  COMMENT 'number of practice primary skill points gained for performing transformation',
  primary_quality_factor int(3) unsigned DEFAULT '0'              COMMENT 'percentage of the primary skill range that applies to quality',
  secondary_skill_id int(10)                                      COMMENT 'secondary skill foriegn key',
  secondary_min_skill int(8) unsigned                             COMMENT 'minimum secondary skill level required to perform transformation, 0 is no minimum',
  secondary_max_skill int(8) unsigned                             COMMENT 'maximum secondary skill level at which practice and quality are effected, 0 is no maximum',
  secondary_practice_points int(4)                                COMMENT 'number of practice secondary skill points gained for performing transformation',
  secondary_quality_factor int(3) unsigned                        COMMENT 'percentage of the secondary skill range that applies to quality',
  script varchar(255) NOT NULL DEFAULT 'Apply Post Trade Process' COMMENT 'A script to run after a process has completed. In order to apply custom effects to the result.',
  description varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (process_id,subprocess_number)
);


#
# Dumping data for table 'trade_processes'
#

INSERT INTO trade_processes VALUES(1,0,"Mix","greet","",70,65,'',75,1,39,0,10,4,15,38,0,20,2,30,'Apply Post Trade Process','Mix in mixing bowl.');
INSERT INTO trade_processes VALUES(1,1,"Mix","greet","",70,90,'',75,1,39,0,10,4,15,38,0,20,2,30,'Apply Post Trade Process','Mix in mixing bowl.');
INSERT INTO trade_processes VALUES(2,0,"Mix at 1AM","greet","",70,65,"TIME(13:00:00)",75,1,39,0,10,4,15,38,0,20,2,30,'Apply Post Trade Process','Mix in mixing bowl at 13:00.');
INSERT INTO trade_processes VALUES(3,0,"Mix with mixer","greet","",70,90,'',75,1,39,10,60,4,15,38,10,15,2,30,'Apply Post Trade Process','Mix in mixing bowl with mixer.');
INSERT INTO trade_processes VALUES(4,0,"Bake with skill","greet","",74,0,'',75,1,39,5,20,6,50,38,10,40,3,50,'Apply Post Trade Process','Bake in oven with skill.');
INSERT INTO trade_processes VALUES(5,0,"Kneed","greet","",138,0,'',75,1,0,0,0,0,0,0,0,0,0,0,'Apply Post Trade Process','Kneed on table.');
INSERT INTO trade_processes VALUES(6,0,"Bake","greet","",74,0,'',75,1,0,0,0,0,0,0,0,0,0,0,'Apply Post Trade Process','Bake in oven.');
INSERT INTO trade_processes VALUES(7,0,"Blast","greet","",4815,0,'',0,0,0,0,0,0,0,0,0,0,0,0,'Apply Post Trade Process','Blast in forge.');
INSERT INTO trade_processes VALUES(8,0,"Hammer","greet","",4811,4816,'',0,0,0,0,0,0,0,0,0,0,0,0,'Apply Post Trade Process','Hammer on anvil.');
INSERT INTO trade_processes VALUES(9,0,"Mold cast","greet","",119,0,'',0,0,0,0,0,0,0,0,0,0,0,0,'Apply Post Trade Process','Use a casting mold.');
INSERT INTO trade_processes VALUES(10,0,"Wayout Mix","greet","",70,65,'',75,1,39,0,10,4,15,38,0,20,2,30,'Apply Post Trade Process','Do wayout mixing in mixing bowl.');
INSERT INTO trade_processes VALUES(11,0,"Wayout Baking","","",0,0,'',75,1,39,0,10,4,15,38,0,20,2,30,'Apply Post Trade Process','Do wayout baking.');
INSERT INTO trade_processes VALUES(12,0,"Flaming weapon","","",0,0,'',75,1,0,0,0,0,0,0,0,0,0,0,'Apply Post Trade Process','Flame on.');

-- Advanced crafting
INSERT INTO `trade_processes` VALUES (652,0,'Enchant','cast','',4928,0,'',0,1,11,0,100,1,50,-1,0,0,0,0,'trade_enchant_gem','Enchant gems');

-- Item getting deleted in the process
INSERT INTO `trade_processes` VALUES (812,0,'Set1','craft','',4928,673,'',0,1,40,0,30,1,50,-1,0,0,0,0,'trade_delete_item','Delete item');
