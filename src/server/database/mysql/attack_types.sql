-- ----------------------------
-- Table structure for `attack_types`
-- ----------------------------
DROP TABLE IF EXISTS `attack_types`;
CREATE TABLE `attack_types` (
  `id` INT NOT NULL AUTO_INCREMENT COMMENT 'holds the attack type unique id number' ,
  `name` VARCHAR(40) NOT NULL COMMENT 'is the attack type name, each name must be unique.' ,
  `weaponName` VARCHAR(40) DEFAULT NULL COMMENT 'filled in for a very specific weapon with special attack, either this or weapon type should always be filled in', 
  `weaponType` VARCHAR(40) DEFAULT NULL COMMENT 'more than one required weapon type may be listed, seperate each weapon type id with a space',
  `onehand` BOOLEAN NOT NULL COMMENT 'attack designed for a 1 hand weapon (true) or 2 handed (false)',
  `stat` int(11) NOT NULL COMMENT 'The skill related to this type',
  PRIMARY KEY (`id`) ,
  UNIQUE INDEX `name_UNIQUE` (`name`)
) COMMENT='Holds attack types to be used in the updated combat system';

INSERT INTO `attack_types` VALUES (1, 'Assassination', NULL, 'SWORD KNIVES', 1, 1);
INSERT INTO `attack_types` VALUES (2, 'Archery', NULL, 'BOW CROSSBOW', 1, 6);
INSERT INTO `attack_types` VALUES (3, 'Barbaric', NULL, 'SWORD CLAYMORE POLEARM HAMMER AXE', 0, 3);
INSERT INTO `attack_types` VALUES (4, 'Long Bow Special', 'Long Bow', NULL, 1, 6);
