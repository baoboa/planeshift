#
# Table structure for table loot_modifiers_restrains
#

DROP TABLE IF EXISTS `loot_modifiers_restrains`;
CREATE TABLE `loot_modifiers_restrains` (
  `loot_modifier_id` INTEGER  NOT NULL COMMENT 'The id of the loot modifier rule',
  `item_id` INTEGER  NOT NULL COMMENT 'The id of the item included in the loot modifier rule',
  `allowed` char(1) NOT NULL DEFAULT 'Y' COMMENT 'Determines if the modifier is allowed or not',
  PRIMARY KEY (`loot_modifier_id`, `item_id`)
)
ENGINE = MyISAM
COMMENT = 'Allows to define some restrain to the loot_modifiers';

