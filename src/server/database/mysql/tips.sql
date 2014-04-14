
# Host: localhost    Database: planeshift
# ------------------------------------------------------
# Server version 5.0.19-nt

#
# Table structure for table tips
#

DROP TABLE IF EXISTS `tips`;
CREATE TABLE `tips` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `tip` text NOT NULL,
  PRIMARY KEY  (`id`)
);

#
# Dumping data for table tips
#

# Tips used in loading screens
INSERT INTO `tips` VALUES (1,'By default, press M to change the camera view');
INSERT INTO `tips` VALUES (3,'This game will always be free');
INSERT INTO `tips` VALUES (4,'This is PlaneShift Arcane Chrysalis, Welcome!');
INSERT INTO `tips` VALUES (5,'By default, press R to /tell');
INSERT INTO `tips` VALUES (6,'Press the big fat O to open the options screen');

# Tips used in tutorial and other quests
INSERT INTO `tips` VALUES (1001,'Welcome to the world of Yliakum, a huge underground cave with many strange and wonderful sights.  Use your arrow keys to walk around and explore.');
INSERT INTO `tips` VALUES (1002,'You can move using the arrow keys, or strafe using WASD keys.  Holding Shift makes you run.  Movement takes your stamina, which means you get tired faster running than walking.  Next, find the merchant named Thorian and right-click him.');
INSERT INTO `tips` VALUES (1003,'When you right-click another player or npc in the game, that is called \"targeting.\"  You always have to target what you are focused on so that PlaneShift knows your intent when attacking, chatting or inspecting.');
INSERT INTO `tips` VALUES (1004,'To talk to any NPC, target him/her and talk normally in the chat window.  Find MaleEnki, target him and say \'hello\' to him now and see if he responds in your chat window.');
INSERT INTO `tips` VALUES (1005,'Congratulations, you should now have the Gold Ore quest assigned!  To help find rats, try targeting the sign behind MaleEnki to get directions.');
INSERT INTO `tips` VALUES (1006,'You just got damaged by the animal attacking you.  Your health is in the red bar in the Info window.  When you hit 0%, you die.');
INSERT INTO `tips` VALUES (1007,'You just got damaged by falling too far.  Be careful how much you fall or you could die just from that.');
INSERT INTO `tips` VALUES (1008,'You have just died.  In a few seconds, you will be transported to the Death Realm and you will have to escape it to come back to Yliakum.');

INSERT INTO `tips` VALUES (1100,'This is an example tip which can be called by a script.');
