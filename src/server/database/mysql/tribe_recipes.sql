#
# Table structure for table tribe_recipes
#

DROP TABLE IF EXISTS `tribe_recipes`;
CREATE TABLE `tribe_recipes`
(
  id int(10) NOT NULL auto_increment,
  name varchar(30) NOT NULL default '',
  requirements varchar(1000) NOT NULL default '',
  algorithm varchar(1000) NOT NULL default '',
  persistent int(1) NOT NULL default '0',
  uniqueness int(1) NOT NULL default '0',
  PRIMARY KEY (`id`)
);

#
# Dumping test data for recipe table
#

#
# Basic Recipes ~ Should not be changed. Data in these recipes is hardcoded in
# the sources. 
#

INSERT INTO `tribe_recipes` VALUES (1, 'Do Nothing', '', 'select(Any,1);setBuffer(selection,Wait_Duration,10.0);percept(selection,tribe:wait);wait(10.0);', 1,1);

INSERT INTO `tribe_recipes` VALUES (10, 'Miner Explore', 'tribesman(Miner,1);', 'select(Miner,1);explore();', 0, 0);
INSERT INTO `tribe_recipes` VALUES (11, 'Miner Test Mine', 'tribesman(Miner,1);', 'select(Miner,1);setBuffer(selection,Resource,$TBUFFER[Resource]);percept(selection,tribe:test_mine);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (12, 'Hunter Explore', 'tribesman(Hunter,1);', 'select(Hunter,1);explore();', 0, 0);

INSERT INTO `tribe_recipes` VALUES (20, 'Miner Dig Resource', 'memory(mine,1,Miner Explore);memory($TBUFFER[Resource],1,Miner Test Mine);tribesman(Miner,1);', 'select(Miner,1);locateResource($TBUFFER[Resource],Miner Test Mine);setBuffer(selection,Resource,$TBUFFER[Resource]);mine();', 0, 0);
INSERT INTO `tribe_recipes` VALUES (21, 'Hunter Hunt Resource', 'memory(hunting_ground,1,Hunter Explore);tribesman(Hunter,1);', 'select(Hunter,1);locateResource(hunting_ground,Hunter Explore);setBuffer(selection,Resource,$TBUFFER[Resource]);percept(selection,tribe:hunt);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (22, 'Miner Buy Skin', 'tribesman(Miner,1);', 'select(Miner,1);setBuffer(selection,Trade,Skin);percept(selection,tribe:buy);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (23, 'Hunter Buy Coal', 'tribesman(Hunter,1);', 'select(Hunter,1);setBuffer(selection,Trade,Coal);percept(selection,tribe:buy);', 0, 0);



INSERT INTO `tribe_recipes` VALUES (30, 'Miner Mate', 'resource($REPRODUCTION_RESOURCE,$REPRODUCTION_COST,Miner Dig Resource,Resource);tribesman(any,1);', 'select(any,1);setBuffer(selection,Reproduce_Type,$member_type);mate();alterResource($REPRODUCTION_RESOURCE, -$REPRODUCTION_COST);', 0 , 0);
INSERT INTO `tribe_recipes` VALUES (31, 'Hunter Mate', 'resource($REPRODUCTION_RESOURCE,$REPRODUCTION_COST,Hunter Hunt Resource,Resource);tribesman(any,1);', 'select(any,1);setBuffer(selection,Reproduce_Type,$member_type);mate();alterResource($REPRODUCTION_RESOURCE, -$REPRODUCTION_COST);', 0 , 0);

# Spot Recipe
INSERT INTO `tribe_recipes` VALUES (40, 'Hunter Tribe Spots', '', 'reserveSpot(0,0,0,Campfire);reserveSpot(-4,0,6,Small Tent);reserveSpot(6,0,4,Small Tent);reserveSpot(4,0,-6,Small Tent);reserveSpot(-8,0,-2,Small Tent);', 0, 0);

INSERT INTO `tribe_recipes` VALUES (41, 'Miner Tribe Spots', '', 'reserveSpot(0,0,0,Campfire);reserveSpot(-4,0,6,Small Tent);reserveSpot(6,0,4,Small Tent);reserveSpot(4,0,-6,Small Tent);reserveSpot(-8,0,-2,Small Tent);reserveSpot(-4,0,-4,Campfire);reserveSpot(-8,0,4,Small Tent);reserveSpot(8,0,8,Small Tent);reserveSpot(2,0,4,Small Tent);reserveSpot(-6,0,-8,Small Tent);', 0, 0);


# Upkeep
INSERT INTO `tribe_recipes` VALUES (50, 'Mining Upkeep', 'resource($REPRODUCTION_RESOURCE,10,Miner Dig Resource,Resource);', 'alterResource($REPRODUCTION_RESOURCE,-10);', 1, 1);
INSERT INTO `tribe_recipes` VALUES (51, 'Hunting Upkeep', 'resource($REPRODUCTION_RESOURCE,10,Hunter Hunt Resource,Resource);', 'alterResource($REPRODUCTION_RESOURCE,-10);', 1, 1);

# 
INSERT INTO `tribe_recipes` VALUES (60, 'Miner Build Campfire', 'tribesman(Miner,1);resource(Coal,10,Miner Dig Resource,Resource);', 'select(Miner,1);locateBuildingSpot(Campfire);percept(selection,tribe:build);alterResource(Coal,-10);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (61, 'Miner Build Tent', 'tribesman(Miner,1);resource(Skin,15,Miner Buy Skin,Resource);', 'select(Miner,1);locateBuildingSpot(Small Tent);percept(selection,tribe:build);alterResource(Skin,-15);', 0, 0);

INSERT INTO `tribe_recipes` VALUES (70, 'Hunter Build Campfire', 'tribesman(Hunter,1);resource(Coal,10,Hunter Buy Coal,Resource);', 'select(Hunter,1);locateBuildingSpot(Campfire);percept(selection,tribe:build);alterResource(Coal,-10);', 0, 0);
INSERT INTO `tribe_recipes` VALUES (71, 'Hunter Build Tent', 'tribesman(Hunter,1);resource(Skin,15,Hunter Hunt Resource,Resource);', 'select(Hunter,1);locateBuildingSpot(Small Tent);percept(selection,tribe:build);alterResource(Skin,-15);', 0, 0);


# Targets ~ Missions
INSERT INTO `tribe_recipes` VALUES (90, 'Miner Evolve Tribe', 
'tribesman(number,16,Miner Mate);
resource(Coal,150,Miner Dig Resource,Resource);
resource(Gold Ore,200,Miner Dig Resource,Resource);
building(Campfire,2,Miner Build Campfire);
building(Small Tent,8,Miner Build Tent);',
'wait(10.0);', 1, 0);

INSERT INTO `tribe_recipes` VALUES (91, 'Hunter Evolve Tribe',
'tribesman(number,4,Hunter Mate);
resource(Skin,150,Hunter Hunt Resource,Resource);
resource(Meat,200,Hunter Hunt Resource,Resource);
building(Campfire,1,Hunter Build Campfire);
building(Small Tent,2,Hunter Build Tent);',
'wait(10.0);', 1, 0);

# Tribal Recipes

INSERT INTO `tribe_recipes` VALUES (100, 'Mining Tribe',
'# No requirements', 
'# The Mining tribe
brain(MiningTribe);aggressivity(peaceful);growth(conservatory);unity(organised);sleepPeriod(diurnal);

# Load recipes
loadRecipe(Do Nothing);
loadRecipe(Miner Tribe Spots);
loadRecipe(Miner Evolve Tribe,distributed);
', 1, 1);

INSERT INTO `tribe_recipes` VALUES (101, 'Hunting Tribe',
'# No requirements',
'# The Hunter tribe
brain(HuntingTribe);aggressivity(neutral);growth(conservatory);unity(organised);sleepPeriod(diurnal);

# Load Recipes
loadRecipe(Do Nothing);
loadRecipe(Hunter Tribe Spots);
loadRecipe(Hunter Evolve Tribe,distributed);
', 1, 1);
