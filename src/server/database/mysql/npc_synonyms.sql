# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 4.0.18-max-nt
#
# Table structure for table 'npc_synonyms'
#

DROP TABLE IF EXISTS `npc_synonyms`;
CREATE TABLE npc_synonyms (
  word varchar(30) NOT NULL DEFAULT '' ,
  synonym_of varchar(30) ,
  more_general varchar(30) ,
  PRIMARY KEY (word)
);


#
# Dumping data for table 'npc_synonyms'
#

INSERT INTO npc_synonyms VALUES("hi","greetings","");
INSERT INTO npc_synonyms VALUES("hello","greetings","");
INSERT INTO npc_synonyms VALUES("hail","greetings","");
INSERT INTO npc_synonyms VALUES("ciao","greetings","");
INSERT INTO npc_synonyms VALUES("salut","greetings","");
INSERT INTO npc_synonyms VALUES("bonjour","greetings","");
INSERT INTO npc_synonyms VALUES("howdy","greetings","");
INSERT INTO npc_synonyms VALUES("good day","greetings","");
INSERT INTO npc_synonyms VALUES("good afternoon","greetings","");
INSERT INTO npc_synonyms VALUES("good evening","greetings","");
INSERT INTO npc_synonyms VALUES("buenos dias","greetings","");
INSERT INTO npc_synonyms VALUES("bye","goodbye","");
INSERT INTO npc_synonyms VALUES("farewell","goodbye","");
INSERT INTO npc_synonyms VALUES("ye","you","");
INSERT INTO npc_synonyms VALUES("thee","you","");
INSERT INTO npc_synonyms VALUES("yourself","you","");
INSERT INTO npc_synonyms VALUES("what's up","how are you","");
INSERT INTO npc_synonyms VALUES("whatsup","how are you","");
INSERT INTO npc_synonyms VALUES("whatssup","how are you","");
INSERT INTO npc_synonyms VALUES("how is your day","how are you","");
INSERT INTO npc_synonyms VALUES("yeah","yes","");
INSERT INTO npc_synonyms VALUES("yep","yes","");
INSERT INTO npc_synonyms VALUES("yeap","yes","");
INSERT INTO npc_synonyms VALUES("sure","yes","");
INSERT INTO npc_synonyms VALUES("oui","yes","");
INSERT INTO npc_synonyms VALUES("sounds fair","yes","");
INSERT INTO npc_synonyms VALUES("of course","yes","");
INSERT INTO npc_synonyms VALUES("i'm interested","yes","");
INSERT INTO npc_synonyms VALUES("i am interested","yes","");
INSERT INTO npc_synonyms VALUES("i agree","yes","");
INSERT INTO npc_synonyms VALUES("i'll do it","yes","");
INSERT INTO npc_synonyms VALUES("ok","yes","");
INSERT INTO npc_synonyms VALUES("okay","yes","");
INSERT INTO npc_synonyms VALUES("nope","no","");
INSERT INTO npc_synonyms VALUES("nah","no","");
INSERT INTO npc_synonyms VALUES("not fair","no","");
INSERT INTO npc_synonyms VALUES("forget it","no","");
INSERT INTO npc_synonyms VALUES("not yet","no","");
INSERT INTO npc_synonyms VALUES("not today","no","");
INSERT INTO npc_synonyms VALUES("i don't agre","no","");
INSERT INTO npc_synonyms VALUES("i'm not interested","no","");
INSERT INTO npc_synonyms VALUES("i am not interested","no","");
INSERT INTO npc_synonyms VALUES("maybe next time","no","");
INSERT INTO npc_synonyms VALUES("don't","not","");
INSERT INTO npc_synonyms VALUES("may i have","give me","");
INSERT INTO npc_synonyms VALUES("do you have","give me","");
INSERT INTO npc_synonyms VALUES("can i have","give me","");
INSERT INTO npc_synonyms VALUES("could i have","give me","");
INSERT INTO npc_synonyms VALUES("i would like to","i want to","");
INSERT INTO npc_synonyms VALUES("i would like","give me","");
INSERT INTO npc_synonyms VALUES("i'd like to","i want to","");
INSERT INTO npc_synonyms VALUES("i'd like","give me","");
INSERT INTO npc_synonyms VALUES("i wanna","give me","");
INSERT INTO npc_synonyms VALUES("i need some","give me","");
INSERT INTO npc_synonyms VALUES("fix","repair","");
INSERT INTO npc_synonyms VALUES("which","what","");
INSERT INTO npc_synonyms VALUES("who are you","about you","");
INSERT INTO npc_synonyms VALUES("what about you","about you","");
INSERT INTO npc_synonyms VALUES("tell me something about","about","");
INSERT INTO npc_synonyms VALUES("tell me about","about","");
INSERT INTO npc_synonyms VALUES("i want to train","train","");
INSERT INTO npc_synonyms VALUES("you train","train","");
INSERT INTO npc_synonyms VALUES("apple","","fruit");
INSERT INTO npc_synonyms VALUES("fruit","","thing");
INSERT INTO npc_synonyms VALUES("peach","","fruit");
