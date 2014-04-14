#
# Table to register players to GM events
#

DROP TABLE IF EXISTS `character_events`;
CREATE TABLE `character_events` (
  `player_id` int(10) NOT NULL default '0',
  `event_id` int(10) NOT NULL default '0',
  `vote` TINYINT(1) NULL DEFAULT NULL COMMENT 'The vote expressed by the player on the specific event (range 1-10 where 0 is equal to 1/10)' ,
  `comments` TEXT NULL DEFAULT NULL COMMENT 'A comment left by the player on the event',
  PRIMARY KEY  (`player_id`,`event_id`)
)
COMMENT = 'List of players who participate into a GM event';
