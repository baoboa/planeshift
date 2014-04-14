#
# Table for GM Events
#
DROP TABLE IF EXISTS `gm_events`;
CREATE TABLE `gm_events` (
  `id` int(10) NOT NULL default '0',
  `date_created` TIMESTAMP default current_timestamp,
  `name` varchar(40) NOT NULL default '',
  `description` blob NOT NULL,
  `status` int NOT NULL default '0',
  `gm_id` int(10) NOT NULL default '0',
  PRIMARY KEY  (`id`)
);
