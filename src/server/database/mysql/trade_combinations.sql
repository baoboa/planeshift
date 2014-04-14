# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'trade_combinations'
#

DROP TABLE IF EXISTS `trade_combinations`;
CREATE TABLE trade_combinations (
  id int(10) unsigned NOT NULL auto_increment,
  pattern_id int(10) unsigned NOT NULL ,
  result_id int(10) unsigned NOT NULL ,
  result_qty int(10) unsigned DEFAULT '0' ,
  item_id int(10) unsigned NOT NULL ,
  min_qty int(10) unsigned DEFAULT '0' ,
  max_qty int(10) unsigned DEFAULT '0' ,
  description varchar(255) NOT NULL DEFAULT '' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'trade_combinations'
#

INSERT INTO trade_combinations VALUES(1,1,76,1,60,1,2,'Egg in waybread batter');
INSERT INTO trade_combinations VALUES(2,1,76,1,61,2,5,'Milk in waybread batter');
INSERT INTO trade_combinations VALUES(3,1,76,1,62,7,12,'Flour in waybread batter');
INSERT INTO trade_combinations VALUES(4,1,77,1,60,1,2,'Egg in waybread nutty batter');
INSERT INTO trade_combinations VALUES(5,1,77,1,61,2,5,'Milk in waybread nutty batter');
INSERT INTO trade_combinations VALUES(6,1,77,1,62,7,12,'Flour in waybread nutty batter');
INSERT INTO trade_combinations VALUES(7,1,77,1,64,1,2,'Nuts in waybread nutty batter');
INSERT INTO trade_combinations VALUES(8,1,78,1,60,1,2,'Egg in waybread sweet batter');
INSERT INTO trade_combinations VALUES(9,1,78,1,61,2,5,'Milk in waybread sweet batter');
INSERT INTO trade_combinations VALUES(10,1,78,1,62,7,12,'Flour in waybread sweet batter');
INSERT INTO trade_combinations VALUES(11,1,78,1,63,1,1,'Honey in waybread sweet batter');
INSERT INTO trade_combinations VALUES(12,1,79,1,60,1,2,'Egg in waybread superb batter');
INSERT INTO trade_combinations VALUES(13,1,79,1,61,2,5,'Milk in waybread superb batter');
INSERT INTO trade_combinations VALUES(14,1,79,1,62,7,12,'Flower in waybread superb batter');
INSERT INTO trade_combinations VALUES(15,1,79,1,64,1,2,'Nuts in waybread superb batter');
INSERT INTO trade_combinations VALUES(16,1,79,1,63,1,1,'Honey in waybread superb batter');
INSERT INTO trade_combinations VALUES(17,1,77,1,80,1,1,'Nuts and waybread');
INSERT INTO trade_combinations VALUES(18,1,77,1,64,1,1,'Nuts and waybread');
INSERT INTO trade_combinations VALUES(19,6,76,1,60,1,2,'Egg in waybread batter');
INSERT INTO trade_combinations VALUES(20,6,76,1,61,2,5,'Milk in waybread batter');
INSERT INTO trade_combinations VALUES(21,6,76,1,62,7,12,'Flour in waybread batter');
