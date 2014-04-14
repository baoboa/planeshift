# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'merchant_item_categories'
#

DROP TABLE IF EXISTS `merchant_item_categories`;
CREATE TABLE merchant_item_categories (
  player_id int(8) unsigned NOT NULL DEFAULT '0' ,
  category_id int(8) unsigned NOT NULL DEFAULT '0' ,
  PRIMARY KEY (player_id,category_id)
);


#
# Dumping data for table 'merchant_item_categories'
#

INSERT INTO merchant_item_categories VALUES("5","1"); # Smith buy/sell weapons
INSERT INTO merchant_item_categories VALUES("8","1"); # Merchant buy/sell everyting
INSERT INTO merchant_item_categories VALUES("8","2");
INSERT INTO merchant_item_categories VALUES("8","3");
INSERT INTO merchant_item_categories VALUES("8","4");
INSERT INTO merchant_item_categories VALUES("8","5");


