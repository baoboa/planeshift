# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'char_creation'
#

DROP TABLE IF EXISTS `character_creation`;
CREATE TABLE character_creation (
  id                int(8) unsigned NOT NULL auto_increment,
  name              varchar(80) NOT NULL DEFAULT '0' ,
  description       text NOT NULL ,
  cp_cost           int(8) default '0' ,
  scriptname            varchar(255), 
  choice_area       varchar(32),
  PRIMARY KEY (id),
  UNIQUE id (id)
);


INSERT INTO character_creation VALUES("1",
                                 "Mage Assistant",
                                 "Your father was a clerk at the magic shop",
                                 6,
                                 "charcreate_2",                                 
                                 "FATHER_JOB"
                                );
                                
INSERT INTO character_creation VALUES("2",
                                 "Mage Master",
                                 "Your father was a master at the magic shop",
                                 6,
                                 "charcreate_9",                                 
                                 "FATHER_JOB"
                                );
                                
INSERT INTO character_creation VALUES("3",
                                 "Painter",
                                 "Your mother was a painter that does high quality work",
                                 6,
                                 "charcreate_9",                                 
                                 "MOTHER_JOB"
                                );

                                
INSERT INTO character_creation VALUES("4",
                                 "Talad",
                                 "Feel the wrath of TALAD!",
                                 6,
                                 "charcreate_2",                                 
                                 "RELIGION"
                                );                               

INSERT INTO character_creation VALUES("5",
                                 "Full Moon",
                                 "Feel the fur!",
                                 6,
                                 "charcreate_3",                                 
                                 "BIRTH_EVENT"
                                );                               
                                
INSERT INTO character_creation VALUES("6",
                                 "Tag",
                                 "Tag your buddies around the playground",
                                 6,
                                 "charcreate_3",                                 
                                 "CHILD_ACTIVITY"
                                );                               
                                
INSERT INTO character_creation VALUES("7",
                                 "Mansion",
                                 "You live in Hammer's house.",
                                 6,
                                 "charcreate_3",                                 
                                 "CHILD_HOUSE"
                                );       
                                
                                                        
INSERT INTO character_creation VALUES("8",
                                 "One",
                                 "Just the one other child in your family",
                                 6,
                                 "charcreate_3",                                 
                                 "CHILD_SIBLINGS"
                                );       
                               
INSERT INTO character_creation VALUES("9",
                                 "Two",
                                 "Two childs and you in your family",
                                 6,
                                 "charcreate_2",                                 
                                 "CHILD_SIBLINGS"
                                ); 
                                 
INSERT INTO character_creation VALUES("10",
                                 "None",
                                 "All alone",
                                 6,
                                 "charcreate_3",                                 
                                 "CHILD_SIBLINGS"
                                );  

INSERT INTO character_creation VALUES (215,'Unodin','','-1','charcreate_215','ZODIAC');
INSERT INTO character_creation VALUES (216,'Byari','','-2','charcreate_216','ZODIAC');
INSERT INTO character_creation VALUES (217,'Treman','','-3','charcreate_217','ZODIAC');
INSERT INTO character_creation VALUES (218,'Kravaan','','-4','charcreate_218','ZODIAC');
INSERT INTO character_creation VALUES (219,'Quintahl','','-5','charcreate_219','ZODIAC');
INSERT INTO character_creation VALUES (220,'Azhord','','-6','charcreate_220','ZODIAC');
INSERT INTO character_creation VALUES (221,'Ylaaren','','-7','charcreate_221','ZODIAC');
INSERT INTO character_creation VALUES (222,'Dwanden','','-8','charcreate_222','ZODIAC');
INSERT INTO character_creation VALUES (223,'Novari','','-9','charcreate_223','ZODIAC');
INSERT INTO character_creation VALUES (224,'Yndoli','','-10','charcreate_224','ZODIAC');


DROP TABLE IF EXISTS `char_create_life`;
CREATE TABLE char_create_life (
  id                int(8) unsigned NOT NULL auto_increment,
  name              varchar(80) NOT NULL DEFAULT '0' ,
  description       text NOT NULL ,
  cp_cost           int(8) default '0' ,
  scriptname        varchar(255), 
  is_base           char,
  PRIMARY KEY (id),
  UNIQUE id (id)
);

Insert INTO char_create_life values ( "1", "Married a farmer", "Farmer!", "5", "charcreate_6", 'Y' );

Insert INTO char_create_life values ( "2", "Married a miner", "Miner!", "5", "charcreate_6", 'Y' );

Insert INTO char_create_life values ( "3", "Married a lumberjack", "lumberjack", "5", "charcreate_7", 'Y' );

Insert INTO char_create_life values ( "4", "Spouse brought home a diamond", "diamond", "5", "charcreate_7", 'N' );


DROP TABLE IF EXISTS `char_create_life_relations`;
CREATE TABLE char_create_life_relations (
  choice                int(8),
  adds_choice           int(8) default '0' ,
  removes_choice        int(8) default '0'
);

Insert INTO char_create_life_relations values ( "1", NULL, "2"  );
Insert INTO char_create_life_relations values ( "1", NULL, "3"  );
Insert INTO char_create_life_relations values ( "2", "4", NULL  );
Insert INTO char_create_life_relations values ( "2", NULL, "1"  );
Insert INTO char_create_life_relations values ( "2", NULL, "3"  );
Insert INTO char_create_life_relations values ( "3", NULL, "1"  );
Insert INTO char_create_life_relations values ( "3", NULL, "2"  );







