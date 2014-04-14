# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'merchant_item_categories'
#

DROP TABLE IF EXISTS `trainer_skills`;
CREATE TABLE trainer_skills (
  player_id int(10) unsigned NOT NULL DEFAULT '0' ,
  skill_id int(10) unsigned NOT NULL DEFAULT '0' ,
  min_rank int(8) unsigned NOT NULL DEFAULT '0' ,
  max_rank int(8) unsigned NOT NULL DEFAULT '0' ,
  min_faction float(10,2) NOT NULL DEFAULT '0.0',
  PRIMARY KEY (player_id,skill_id,min_rank)
);


#
# Dumping data for table 'merchant_item_categories'
#

INSERT INTO trainer_skills VALUES("5","0","0","50","-200.0"); # Smith trains weapons
INSERT INTO trainer_skills VALUES("5","1","0","50","-200.0"); # Smith trains weapons
INSERT INTO trainer_skills VALUES("5","2","0","10","0.0"); 
INSERT INTO trainer_skills VALUES("5","3","0","10","0.0"); 
INSERT INTO trainer_skills VALUES("5","3","10","400","50.0"); 
INSERT INTO trainer_skills VALUES("5","4","0","10","0.0"); 
INSERT INTO trainer_skills VALUES("5","5","0","300","0.0"); 
INSERT INTO trainer_skills VALUES("5","6","0","20","0.0"); 
INSERT INTO trainer_skills VALUES("5","7","0","150","0.0"); 
INSERT INTO trainer_skills VALUES("5","8","0","150","0.0"); 
INSERT INTO trainer_skills VALUES("5","9","0","150","0.0"); 
INSERT INTO trainer_skills VALUES("5","10","0","150","0.0"); 
INSERT INTO trainer_skills VALUES("5","11","0","50","0.0"); 
INSERT INTO trainer_skills VALUES("5","46","10","400","0.0"); # And stats
INSERT INTO trainer_skills VALUES("5","47","10","400","0.0"); 
INSERT INTO trainer_skills VALUES("5","48","10","400","0.0"); 
INSERT INTO trainer_skills VALUES("5","49","10","400","0.0"); 
INSERT INTO trainer_skills VALUES("5","50","10","400","0.0"); 
INSERT INTO trainer_skills VALUES("5","51","10","400","0.0"); 
INSERT INTO trainer_skills VALUES("8","12","0","250","0.0"); # Merchant trains magic
INSERT INTO trainer_skills VALUES("8","13","0","50","0.0");
INSERT INTO trainer_skills VALUES("8","14","0","50","0.0");
INSERT INTO trainer_skills VALUES("8","15","0","250","0.0");
INSERT INTO trainer_skills VALUES("8","16","0","50","-200.0");
INSERT INTO trainer_skills VALUES("8","17","0","50","-200.0");


