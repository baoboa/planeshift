#
# Table structure for table tribe_members
#

DROP TABLE IF EXISTS `tribe_members`;
CREATE TABLE `tribe_members` 
(
  `tribe_id` int(10) NOT NULL,
  `member_id` int(10) unsigned NOT NULL,
  `member_type` varchar(30) NOT NULL default '',
  `flags` varchar(200) default '' COMMENT 'Various flags like: DYNAMIC, ...',
  PRIMARY KEY  (`tribe_id`,`member_id`)
);

#
# Dumping data for table characters
#

INSERT INTO `tribe_members` VALUES (1,20,"Miner","STATIC");
INSERT INTO `tribe_members` VALUES (2,62,"Hunter","STATIC");

