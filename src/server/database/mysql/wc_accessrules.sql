--
-- Table structure for table `wc_accessrules`
--

DROP TABLE IF EXISTS `wc_accessrules`;
CREATE TABLE `wc_accessrules` (
  `id` int(10) NOT NULL auto_increment,
  `security_level` tinyint(3) NOT NULL,
  `objecttype` varchar(50) NOT NULL,
  `access` tinyint(3) default NULL,
  UNIQUE KEY `id` (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `wc_accessrules`
--

-- security_level: music and pr<30
-- 30=can see all except natural resources, crafting, spells, quests (generic or setting profile I)
-- 31=as above plus can see natural resources (rules profile I)
-- 32=as above plus can see crafting (rules profile II)
-- 33=as above plus can see spells (rules profile III)
-- 34=as *31* plus can see quests (setting profile II)

-- access: 1=read, 2=edit, 3=create, 4=delete

LOCK TABLES `wc_accessrules` WRITE;
/*!40000 ALTER TABLE `wc_accessrules` DISABLE KEYS */;
-- READ
INSERT INTO wc_accessrules values (100,30,'npcs',1),(101,30,'quests',0),(102,30,'items',1),(103,30,'als',1),(104,30,'natres',0),(105,30,'crafting',0),(106,30,'spells',0),(107,30,'other',1),(108,30,'statistics',1),(109,30,'assets',1),(110,30,'admin',1);
INSERT INTO wc_accessrules values (120,31,'npcs',1),(121,31,'quests',0),(122,31,'items',1),(123,31,'als',1),(124,31,'natres',1),(125,31,'crafting',0),(126,31,'spells',0),(127,31,'other',1),(128,31,'statistics',1),(129,31,'assets',1),(130,31,'admin',1);
INSERT INTO wc_accessrules values (140,32,'npcs',1),(141,32,'quests',0),(142,32,'items',1),(143,32,'als',1),(144,32,'natres',1),(145,32,'crafting',1),(146,32,'spells',0),(147,32,'other',1),(148,32,'statistics',1),(149,32,'assets',1),(150,32,'admin',1);
INSERT INTO wc_accessrules values (160,33,'npcs',1),(161,33,'quests',0),(162,33,'items',1),(163,33,'als',1),(164,33,'natres',1),(165,33,'crafting',1),(166,33,'spells',1),(167,33,'other',1),(168,33,'statistics',1),(169,33,'assets',1),(170,33,'admin',1);
INSERT INTO wc_accessrules values (180,34,'npcs',1),(181,34,'quests',1),(182,34,'items',1),(183,34,'als',1),(184,34,'natres',1),(185,34,'crafting',0),(186,34,'spells',0),(187,34,'other',1),(188,34,'statistics',1),(189,34,'assets',1),(190,34,'admin',1);
-- EDIT
INSERT INTO wc_accessrules values (200,35,'npcs',2),(201,35,'quests',0),(202,35,'items',2),(203,35,'als',2),(204,35,'natres',0),(205,35,'crafting',0),(206,35,'spells',0),(207,35,'other',2),(208,35,'statistics',2),(209,35,'assets',2),(210,35,'admin',2);
INSERT INTO wc_accessrules values (220,36,'npcs',2),(221,36,'quests',0),(222,36,'items',2),(223,36,'als',2),(224,36,'natres',2),(225,36,'crafting',0),(226,36,'spells',0),(227,36,'other',2),(228,36,'statistics',2),(229,36,'assets',2),(230,36,'admin',2);
INSERT INTO wc_accessrules values (240,37,'npcs',2),(241,37,'quests',0),(242,37,'items',2),(243,37,'als',2),(244,37,'natres',2),(245,37,'crafting',2),(246,37,'spells',0),(247,37,'other',2),(248,37,'statistics',2),(249,37,'assets',2),(250,37,'admin',2);
INSERT INTO wc_accessrules values (260,38,'npcs',2),(261,38,'quests',0),(262,38,'items',2),(263,38,'als',2),(264,38,'natres',2),(265,38,'crafting',2),(266,38,'spells',2),(267,38,'other',2),(268,38,'statistics',2),(269,38,'assets',2),(270,38,'admin',2);
INSERT INTO wc_accessrules values (280,39,'npcs',2),(281,39,'quests',2),(282,39,'items',2),(283,39,'als',2),(284,39,'natres',2),(285,39,'crafting',0),(286,39,'spells',0),(287,39,'other',2),(288,39,'statistics',2),(289,39,'assets',2),(290,39,'admin',2);
-- CREATE
INSERT INTO wc_accessrules values (300,40,'npcs',3),(301,40,'quests',0),(302,40,'items',3),(303,40,'als',3),(304,40,'natres',0),(305,40,'crafting',0),(306,40,'spells',0),(307,40,'other',3),(308,40,'statistics',3),(309,40,'assets',3),(310,40,'admin',3);
INSERT INTO wc_accessrules values (320,41,'npcs',3),(321,41,'quests',0),(322,41,'items',3),(323,41,'als',3),(324,41,'natres',3),(325,41,'crafting',0),(326,41,'spells',0),(327,41,'other',3),(328,41,'statistics',3),(329,41,'assets',3),(330,41,'admin',3);
INSERT INTO wc_accessrules values (340,42,'npcs',3),(341,42,'quests',0),(342,42,'items',3),(343,42,'als',3),(344,42,'natres',3),(345,42,'crafting',3),(346,42,'spells',0),(347,42,'other',3),(348,42,'statistics',3),(349,42,'assets',3),(350,42,'admin',3);
INSERT INTO wc_accessrules values (360,43,'npcs',3),(361,43,'quests',0),(362,43,'items',3),(363,43,'als',3),(364,43,'natres',3),(365,43,'crafting',3),(366,43,'spells',3),(367,43,'other',3),(368,43,'statistics',3),(369,43,'assets',3),(370,43,'admin',3);
INSERT INTO wc_accessrules values (380,44,'npcs',3),(381,44,'quests',3),(382,44,'items',3),(383,44,'als',3),(384,44,'natres',3),(385,44,'crafting',0),(386,44,'spells',0),(387,44,'other',3),(388,44,'statistics',3),(389,44,'assets',3),(390,44,'admin',3);

/*!40000 ALTER TABLE `wc_accessrules` ENABLE KEYS */;

-- Obsolete data
-- INSERT INTO wc_accessrules values (100,30,'npcs',1),(101,30,'quests',1),(102,30,'items',1),(103,30,'als',1),(104,30,'natres',1),(105,30,'crafting',1),(106,30,'spells',1),(107,30,'other',1),(108,30,'statistics',1),(109,30,'assets',1),(110,30,'admin',1);
-- INSERT INTO wc_accessrules values (200,31,'npcs',),(201,31,'quests',),(202,31,'items',),(203,31,'als',),(204,31,'natres',),(205,31,'crafting',),(206,31,'spells',),(207,31,'other',),(208,31,'statistics',),(209,31,'assets',),(210,31,'admin',);
-- INSERT INTO wc_accessrules values (300,31,'npcs',),(301,31,'quests',),(302,31,'items',),(303,31,'als',),(304,31,'natres',),(305,31,'crafting',),(306,31,'spells',),(307,31,'other',),(308,31,'statistics',),(309,31,'assets',),(310,31,'admin',);

UNLOCK TABLES;

