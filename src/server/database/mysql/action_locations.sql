DROP TABLE IF EXISTS `action_locations`;
CREATE TABLE action_locations
(
  id INTEGER UNSIGNED NOT NULL AUTO_INCREMENT,
  master_id INTEGER UNSIGNED,
  name VARCHAR(45) NOT NULL,
  sectorname VARCHAR(30) NOT NULL,
  meshname VARCHAR(250) NOT NULL,
  polygon INTEGER UNSIGNED, 
  pos_x FLOAT,
  pos_y FLOAT,
  pos_z FLOAT,
  pos_instance int(10) unsigned NOT NULL DEFAULT '4294967295' COMMENT 'Indicates the instance where this action location will be accessible from. 0xFFFFFFFF (default) is instance all',
  radius FLOAT,
  triggertype VARCHAR(10),
  responsetype VARCHAR(10),
  response TEXT,
  active_ind  char(1) NOT NULL default 'Y',
  PRIMARY KEY(`id`)
);

#Entrance with lock
INSERT INTO `action_locations` VALUES (1,0,'The Blacksmith Shop','NPCroom','npc1_building',0,-56,0,-148,4294967295,10,'SELECT','EXAMINE','<Examine><Entrance Type=\'ActionID\' LockID=\'95\' X=\'-58\' Y=\'0\' Z=\'-115\' Rot=\'280\' Sector=\'NPCroom1\' /><Return X=\'-56\' Y=\'0\' Z=\'-148\' Rot=\'130\' Sector=\'NPCroom\' /><Description>A lovely residence</Description></Examine>','Y');

#Entrance with quest check
INSERT INTO `action_locations` VALUES (2,0,'A door to a restricted place','NPCroom','npc1_door1', 0,-42,0,-158,4294967295,10,'SELECT','EXAMINE',"<Examine><Entrance Type='Prime' Script='Actor:HasCompletedQuest(\"Rescue the Princess\")' X='-58' Y='0' Z='-115' Rot='280' Sector='NPCroom1'/><Description>A door to a restricted place</Description></Examine>",'Y');

INSERT INTO `action_locations` VALUES (3,0,'IngotsOfFire','NPCroom','smith_ingots',0,0,0,0,4294967295,10,'PROXIMITY','SCRIPT','flame_damage','Y');
INSERT INTO `action_locations` VALUES (4,0,'HealingTree','NPCroom','',0,-21.98,0.00,-169.02,4294967295,5,'PROXIMITY','SCRIPT','healing_tree','Y');
INSERT INTO `action_locations` VALUES (5,0,'woodbox','NPCroom','npc1_woodbox',0,0,0,0,4294967295,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'80\'/>\n<Description>A Box full of books</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (6,0,'Smith Forge','NPCroom','npc1_furnace',0,0,0,0,4294967295,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'115\'/>\n<Description>A smith forge, ready to heat materials</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (7,0,'Anvil','NPCroom','npc1_anvil',0,0,0,0,4294967295,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'116\'/>\n<Description>An anvil used to forge items</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (8,0,'Test Game','NPCroom','npc1_Box10',0,4.2,0.99,-216.94,4294967295,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'Test Game\' /><Description>A minigame board for testing.</Description></Examine>','Y');
INSERT INTO `action_locations` VALUES (10,0,'ownedbox','NPCroom','npc1_woodplank',0,0,0,0,4294967295,5,'SELECT','EXAMINE','<Examine>\n<Container ID=\'82\'/>\n<Description>A Box owned by the merchant</Description>\n</Examine>','Y');
INSERT INTO `action_locations` VALUES (11,0,'Tic Tac Toe','NPCroom','npc1_Box09',0,-1.69,0.89,-217.21,4294967295,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'Tic Tac Toe\' EndGame=\'Yes\' Script=\'minigame_win\' /><Description>Tic Tac Toe Board Game with $$$ prizes $$$.</Description></Examine>','Y');
INSERT INTO `action_locations` VALUES (12,0,'Rainmaker game','NPCroom','npc1_Box08',0,-9.00,0.79,-217.21,4294967295,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'Weather game\' EndGame=\'Yes\' Script=\'rain\' /><Description>Magic puzzle that makes it rain. Make a simple r shape with the game pieces.</Description></Examine>','Y');
INSERT INTO `action_locations` VALUES (13,0,'One Two Three','NPCroom','npc1_Box07',0,-16.17,0.70,-217.06,4294967295,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'One Two Three\' EndGame=\'Yes\' Script=\'mechanism\' Param0=\'npc1_Box08\' Param1=\'0,5,0\' Param2=\'\' /><Description>Tests specific pieces rule. Put tile 1 in 0,0, tile 2 in 1,0 and tile 3 in 2,0 . When solved the next wooden box should move.</Description></Examine>','Y');
INSERT INTO `action_locations` VALUES (14, 0, 'simpleAL', 'NPCroom', 'npc1_trunk00', '0', '0', '0', '0', '4294967295', '0', 'SELECT', 'EXAMINE', 'A nice looking tree', 'Y');
INSERT INTO `action_locations` VALUES (15,0,'Already Placed Pieces','NPCroom','npc1_Box20',0,2.73,0.01,-241.63,4294967295,4,'SELECT','EXAMINE','<Examine><GameBoard Name=\'Already Placed Pieces\' EndGame=\'Yes\' Script=\'minigame_win\' /><Description>Moved to pieces to specific locations.</Description></Examine>','Y');
