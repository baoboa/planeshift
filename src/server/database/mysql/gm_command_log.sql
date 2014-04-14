#
# Table structure for table 'gm_command_log'
#

DROP TABLE IF EXISTS `gm_command_log`;
CREATE TABLE gm_command_log (
  id int(10) unsigned NOT NULL auto_increment,
  account_id INT(10) UNSIGNED NOT NULL DEFAULT '0',
  gm int(10) NOT NULL DEFAULT '0', 
  command varchar(200) NOT NULL DEFAULT '/none',
  player int(10) NOT NULL DEFAULT '0',
  ex_time datetime ,
  PRIMARY KEY (id),
  UNIQUE id (id)
);


#
# Dumping data for table 'gm_command_log'
#
INSERT INTO gm_command_log VALUES("1", "1", "1", "/test", "1", "2003-01-01 12:12:12");
