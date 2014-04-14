# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'petitions'
#

DROP TABLE IF EXISTS `petitions`;
CREATE TABLE petitions (
  id int(10) unsigned NOT NULL auto_increment,
  player int(10) NOT NULL DEFAULT '0' ,
  petition blob NOT NULL,
  status varchar(20) DEFAULT 'Open' ,
  category varchar(30) DEFAULT 'Not Assigned' ,
  created_date datetime ,
  closed_date datetime DEFAULT '0000-00-00 00:00:00' ,
  assigned_gm int(10) DEFAULT '-1' ,
  resolution blob ,
  escalation_level int(10) unsigned DEFAULT '1' ,
  PRIMARY KEY (id),
  UNIQUE id (id)
)
COMMENT = 'Requests from players to GMs (petition window)';


#
# Dumping data for table 'petitions'
#

INSERT INTO petitions VALUES("1","1","Testing petition command once","Open","Not Assigned","2003-06-30 13:52:09","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("2","4","I lost my favorite item fighting big ugly monsters","Open","Not Assigned","2003-06-30 13:56:23","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("3","1","I'm stuck under the temple...ouch","In Progress","Not Assigned","2003-06-30 15:39:06","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("4","3","I'm testing your petition command","Open","Not Assigned","2003-06-30 15:39:48","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("5","2","Incredible game!","Open","Not Assigned","2003-07-04 07:39:20","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("6","1","A very very very very very very very very very very very very very very very very very very very very LONG petition","Open","Not Assigned","2003-07-04 15:47:52","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("14","1","This is a test of a very very very very very very very very very very very very very LONG petition","Open","Not Assigned","2003-07-04 18:51:40","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("15","1","help me AGHHHH!","Cancelled","Not Assigned","2003-07-04 19:06:13","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("16","1","I said \"HELP ME!\" and he said \"Give me all your stuff\" so I did; what do I do please?","Cancelled","Not Assigned","2003-07-04 19:12:00","0000-00-00 00:00:00","-1","Not Resolved","1");
INSERT INTO petitions VALUES("17","1","I need a vacation!","Cancelled","Not Assigned","2003-07-04 23:36:36","0000-00-00 00:00:00","-1","Not Resolved","1");
