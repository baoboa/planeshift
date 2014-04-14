DROP TABLE IF EXISTS `character_variables`;
CREATE TABLE `character_variables` (
  `character_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'The character this variable is assigned to',
  `name` varchar(255) NOT NULL COMMENT 'The name of the variable',
  `value` varchar(255) NOT NULL COMMENT 'The value of the variable',
  PRIMARY KEY (`character_id`,`name`),
  KEY `character_id` (`character_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='Used to store variables for a character';


INSERT INTO character_variables VALUES("104","scale","0.5"); # Small mover
INSERT INTO character_variables VALUES("105","scale","1.5"); # Large mover
INSERT INTO character_variables VALUES("106","scale","2.0"); # Huge mover
