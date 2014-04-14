# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'trade_transformations'
#

DROP TABLE IF EXISTS `trade_transformations`;
CREATE TABLE trade_transformations (
  id int(10) unsigned NOT NULL auto_increment,
  pattern_id int(10) unsigned NOT NULL                  COMMENT 'pattern for transformation',
  process_id int(10) unsigned NOT NULL                  COMMENT 'process for transformation',
  result_id int(10) unsigned NOT NULL                   COMMENT 'resulting item',
  result_qty int(8) unsigned NOT NULL                   COMMENT 'resulting item quantity',
  item_id int(10) unsigned NOT NULL                     COMMENT 'item to be transformed',
  item_qty int(8) unsigned NOT NULL                     COMMENT 'required quantity for transformation',
  trans_points varchar(255) NOT NULL DEFAULT '0'        COMMENT 'ammount of time to complete transformation',
  penalty_pct float(10,6) NOT NULL DEFAULT '1.000000'   COMMENT 'percent of quality for resulting item',
  description varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (id)
);


#
# Dumping data for table 'trade_transformations'
#

INSERT INTO trade_transformations VALUES(1,1,1,69,1,76,1,10,0.400000,'You mix waybread slop into batter in mixing bowl.');
INSERT INTO trade_transformations VALUES(2,1,2,66,1,77,1,10,0.600000,'You mix waybread nutty slop into nutty batter in mixing bowl.');
INSERT INTO trade_transformations VALUES(3,1,2,67,1,78,1,10,0.800000,'You mix waybread sweet slop into sweet batter in mixing bowl.');
INSERT INTO trade_transformations VALUES(4,1,2,68,1,79,1,10,1.000000,'You mix waybread superb slop into superb batter in mixing bowl.');
INSERT INTO trade_transformations VALUES(5,1,3,69,1,76,1,10,0.400000,'You mix waybread slop into batter in mixing bowl with mixer.');
INSERT INTO trade_transformations VALUES(6,2,5,134,1,69,1,20,0.400000,'You kneed waybread batter into dough on table.');
INSERT INTO trade_transformations VALUES(7,2,5,135,1,66,1,20,0.600000,'You kneed nutty waybread batter into nutty dough on table.');
INSERT INTO trade_transformations VALUES(8,2,5,136,1,67,1,20,0.800000,'You kneed sweet waybread batter into sweet dough on table.');
INSERT INTO trade_transformations VALUES(9,2,5,137,1,68,1,20,1.000000,'You kneed superb waybread batter into superb dough on table.');
INSERT INTO trade_transformations VALUES(10,1,4,80,1,134,1,20,0.400000,'You bake waybread dough into waybread in oven.');
INSERT INTO trade_transformations VALUES(11,1,4,81,1,135,1,20,0.600000,'You bake nutty waybread dough into nutty waybread in oven.');
INSERT INTO trade_transformations VALUES(12,1,4,82,1,136,1,20,0.800000,'You bake sweet waybread dough into sweet waybread in oven.');
INSERT INTO trade_transformations VALUES(13,1,4,83,1,137,1,20,1.000000,'You bake superb waybread dough into superb waybread in oven.');
INSERT INTO trade_transformations VALUES(14,0,6,72,1,80,1,40,0.100000,'You bake waybread into burnt waybread in oven.');
INSERT INTO trade_transformations VALUES(15,0,6,72,1,81,1,40,0.100000,'You bake nutty waybread into burnt waybread in oven.');
INSERT INTO trade_transformations VALUES(16,0,6,72,1,82,1,40,0.100000,'You bake sweet waybread into burnt waybread in oven.');
INSERT INTO trade_transformations VALUES(17,0,6,72,1,83,1,40,0.100000,'You bake superb waybread into burnt waybread in oven.');
INSERT INTO trade_transformations VALUES(18,0,6,73,1,72,1,40,0.010000,'You bake burnt waybread into dust in oven.');
INSERT INTO trade_transformations VALUES(19,0,6,73,1,0,0,60,1.000000,'You bake anything and it turns into dust in oven.');
INSERT INTO trade_transformations VALUES(20,0,6,0,0,73,1,60,1.000000,'You bake dust until it disappears in oven.');
INSERT INTO trade_transformations VALUES(21,0,7,106,0,104,0,20,1.000000,'You blast coke into hot coke.');
INSERT INTO trade_transformations VALUES(22,0,7,107,0,102,0,20,1.000000,'You blast iron ore into pig iron.');
INSERT INTO trade_transformations VALUES(23,3,8,110,1,107,1,20,1.000000,'You hammer pig iron into a wrought iron ingot.');
INSERT INTO trade_transformations VALUES(24,3,8,111,1,110,1,20,1.000000,'You hammer one wrought iron ingots into a short iron bar.');
INSERT INTO trade_transformations VALUES(25,3,8,112,1,111,2,20,1.000000,'You hammer two short iron bars into an iron bar.');
INSERT INTO trade_transformations VALUES(26,3,8,113,1,112,2,20,1.000000,'You hammer two iron bars into a long iron bar.');
INSERT INTO trade_transformations VALUES(27,3,9,121,1,113,1,20,1.000000,'You cast a fishing reel from long iron bar.');
INSERT INTO trade_transformations VALUES(28,6,10,69,1,76,1,1,0.400000,'You mix wayoutbread slop into batter in mixing bowl.');
INSERT INTO trade_transformations VALUES(29,6,11,134,1,69,1,1,0.400000,'You kneed wayoutbread batter into dough.');
INSERT INTO trade_transformations VALUES(30,6,11,80,1,134,1,1,0.400000,'You bake wayoutbread dough into wayoutbread.');
INSERT INTO trade_transformations VALUES(31,7,12,73,1,0,0,1,1.000000,'You bake anything and it turns into dust.');

-- Advanced crafting
INSERT INTO `trade_transformations` VALUES (9739,458,652,673,1,673,1,10,1,'Enchant Ruby Crystal');

-- Item getting deleted in the process
INSERT INTO `trade_transformations` VALUES (9962,458,812,417,1,416,1,10,1,'gem socket');
