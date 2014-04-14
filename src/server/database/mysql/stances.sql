DROP TABLE IF EXISTS `stances`;
CREATE TABLE stances(
  id int(8) unsigned NOT NULL auto_increment,
  name varchar(30) DEFAULT '0' ,
  stamina_drain_P float(3,2) UNSIGNED DEFAULT '0.0' ,
  stamina_drain_M float(3,2) UNSIGNED DEFAULT '0.0' ,
  attack_speed_mod float(3,2) UNSIGNED DEFAULT '0.0' ,
  attack_damage_mod float(3,2) UNSIGNED DEFAULT '0.0' ,
  defense_avoid_mod float(3,2) UNSIGNED DEFAULT '0.0' ,
  defense_absorb_mod float(3,2) UNSIGNED DEFAULT '0.0' ,
  PRIMARY KEY (id),
  UNIQUE name (name)
);

INSERT INTO stances VALUES("1", "none", "0", "0", "0", "0", "0", "0");
INSERT INTO stances VALUES("2", "bloody", "2", "1", "1.1", "1.2", "0.9", "0.8");
INSERT INTO stances VALUES("3", "aggressive", "1.5", "0.5", "1", "1.1", "1", "0.9");
INSERT INTO stances VALUES("4", "normal", "1", "0", "1", "1", "1", "1");
INSERT INTO stances VALUES("5", "defensive", "1.5", "0.5", "1", "0.9", "1", "1.1");
INSERT INTO stances VALUES("6", "fullydefensive", "2", "1", "0", "0", "1.2", "1.2");

