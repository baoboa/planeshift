#
# Table structure for table 'bans'
#

DROP TABLE IF EXISTS `bans`;
CREATE TABLE bans (
  account int(10) unsigned NOT NULL DEFAULT '0',
  ip_range CHAR(15) default NULL,
  start int(20) unsigned default '0',
  end int(20) unsigned default '0',
  reason text default NULL,
  ban_ip BOOLEAN NOT NULL DEFAULT 0 COMMENT 'Account banned by IP as well',
  PRIMARY KEY  (account)
);

INSERT INTO `bans` VALUES (3,'80.11.111.',1229325764,1249325764,'Dummy ban example',1);