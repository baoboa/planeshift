
DROP TABLE IF EXISTS `loot_modifiers`;
CREATE TABLE loot_modifiers (
  id INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
  modifier_type VARCHAR(20) NULL,
  name VARCHAR(255) NULL,
  effect TEXT NULL,
  probability FLOAT NULL,
  stat_req_modifier TEXT NULL,
  cost_modifier FLOAT NULL,
  `mesh` varchar(255) DEFAULT NULL COMMENT 'Defines a mesh to be placed as replacement in case this modifier is selected. (priority rule: adjective, suffix, prefix)',
  `icon` varchar(255) DEFAULT NULL COMMENT 'Defines an icon to be placed as replacement in case this modifier is selected. (priority rule: adjective, suffix, prefix)',
  not_usable_with VARCHAR(255) NULL,
  equip_script text NULL,
  PRIMARY KEY(id)
);

-- (id,modifier_type,name,effect,probability,stat_req_modifier,cost_modifier,mesh,not_usable_with) 
-- INSERT INTO `loot_modifiers` ( modifier_type, name, effect, probability, stat_req_modifier, cost_modifier, mesh, not_usable_with ) VALUES 

-- Prefixes
INSERT INTO loot_modifiers VALUES (1,'prefix','Plastic','<ModiferEffect operation="mul" name="item.damage" value="0.90" />',94,'<StatReq name="INT" value="100"/><StatReq name="STR" value="70"/>',0.7,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (2,'prefix','Fabric','<ModiferEffect operation="add" name="item.damage" value="5" />',99,'<StatReq name="STR" value="100"/>',1.2,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (3,'prefix','Plutonium','<ModiferEffect operation="add" name="item.damage" value="4" />',124,'<StatReq name="STR" value="200"/>',1.2,'','','','<str value="10"/><hp-max value="10"/>');

-- Adjectives
INSERT INTO loot_modifiers VALUES (4,'adjective','Torn','<ModiferEffect operation="mul" name="item.damage" value="0.30" />',4,'<StatReq name="STR" value="200"/>',0.2,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (5,'adjective','Spotted','<ModiferEffect operation="mul" name="item.damage" value="0.40" />',8,'<StatReq name="STR" value="200"/>',0.3,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (6,'adjective','Xeroxed','<ModiferEffect operation="mul" name="item.damage" value="0.50" />',12,'<StatReq name="STR" value="200"/>',0.4,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (7,'adjective','Yellowed','<ModiferEffect operation="mul" name="item.damage" value="0.60" />',15,'<StatReq name="STR" value="200"/>',0.4,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (8,'adjective','Brown','<ModiferEffect operation="mul" name="item.damage" value="0.70" />',19,'<StatReq name="STR" value="200"/>',0.5,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (9,'adjective','Scratched','<ModiferEffect operation="mul" name="item.damage" value="0.80" />',24,'<StatReq name="INT" value="200"/>',0.6,'','','','<str value="10"/><hp-max value="10"/>');

-- Suffixes
INSERT INTO loot_modifiers VALUES (10,'suffix','of Purity','<ModiferEffect operation="mul" name="item.damage" value="1.30" />',10,'<StatReq name="STR" value="130" /><StatReq name="END" value="130" />',1.7,'','','','<str value="10"/><hp-max value="10"/>');
INSERT INTO loot_modifiers VALUES (11,'suffix','of Steel','<ModiferEffect operation="mul" name="item.damage" value="1.80" />',45,'<StatReq name="END" value="150" />',2,'','','','<str value="10"/><hp-max value="10"/>');

-- Advanced crafting
INSERT INTO loot_modifiers VALUES (12,'prefix','Greater','<ModiferEffect operation="MUL" name="var.Myvar" value="2" />',0,'',1,'','','','');
INSERT INTO loot_modifiers VALUES (13,'adjective','Luminous','<ModiferEffect operation="VAL" name="var.Myvar" value="1" />',0,'',1,'','','','<str value="10*Myvar"/><hp-max value="10*Myvar"/>');
INSERT INTO loot_modifiers VALUES (14,'suffix','of Raistlin','<ModiferEffect operation="VAL" name="var.Myvar" value="1" />',0,'',1,'','','','<agi value="10*Myvar"/>');