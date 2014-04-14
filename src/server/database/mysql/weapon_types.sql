-- ----------------------------
-- Table structure for `weapon_types`
-- ----------------------------
DROP TABLE IF EXISTS `weapon_types`;
CREATE TABLE `weapon_types` (
  `id` INT NOT NULL AUTO_INCREMENT COMMENT ' holds the types unique id number' ,
  `name` VARCHAR(40) NOT NULL  COMMENT 'is the types name, each name must be unique.' ,
  `skill` int(11) NOT NULL COMMENT 'The skill related to this type',
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name`)
) COMMENT='Holds different types of weapons and their associations';

INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (1, 'SWORD', 0);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (2, 'KNIVES', 1);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (3, 'AXE', 2);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (4, 'HAMMER', 3);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (5, 'POLEARM', 5);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (6, 'MARTIALARTS', 4);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (7, 'BOW', 6);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (8, 'CROSS BOW', 6);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (9, 'SABRE', 0);
INSERT INTO `weapon_types` (`id`, `name`, `skill`) VALUES (10, 'CLAYMORE', 0);
