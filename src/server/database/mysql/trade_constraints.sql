# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'trade_constraints'
#

DROP TABLE IF EXISTS `trade_constraints`;
CREATE TABLE trade_constraints (
  id int(10) unsigned NOT NULL auto_increment,
  constrnt varchar(64) NOT NULL DEFAULT '',
  message varchar(128) NOT NULL DEFAULT '',
  description varchar(255) NOT NULL DEFAULT '' ,
  PRIMARY KEY (id)
);


#
# Dumping data for table 'trade_constraints'
#

INSERT INTO trade_constraints VALUES(1,'TIME','You can not complete the work at this time of the day!','Time of day. Parameter: hh:mm:ss; where h is hour,m is minute, and s is second. Example: TIME(12:00:00) is noon.');
INSERT INTO trade_constraints VALUES(2,'FRIENDS','You need more people for this work!','People in area.  Parameter: n,r; where n is number of people and r is the range. Example: FRIENDS(6,4) is six people within 4.');
INSERT INTO trade_constraints VALUES(3,'LOCATION','You can not finish this work here!','Location of player.  Parameter: x,y,z,r; where x is x-coord, y is y-coord, z is z-coord, and r is rotation.  Example: LOCATION(-10.53,176.36,,) is at [-10.53,176.36] any hight and any direction.');
INSERT INTO trade_constraints VALUES(4,'MODE','You are not doing the right things to complete this work!','Player mode.  Parameter: m; where m is client mode number.  Example: MODE(4) is player needs to be dancing.');
