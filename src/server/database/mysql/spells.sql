/*
 * Table structure for table 'spells'
 */

DROP TABLE IF EXISTS `spells`;
CREATE TABLE spells (
  `id` int(8) unsigned NOT NULL auto_increment,
  `name` varchar(30) NOT NULL,
  `way_id` int(8) unsigned DEFAULT '0',
  `realm` tinyint(3) unsigned DEFAULT '0',
  `casting_effect` varchar(255),
  `image_name` varchar(100),
  `spell_description` text,
  `offensive` boolean DEFAULT true,
  `max_power` FLOAT(4,2) DEFAULT '1',
  `target_type` int(4) DEFAULT 0x20,
  `cast_duration` text NOT NULL, /* (power, wayskill, relatedstat) -> seconds */
  `range` text,                  /* (power, wayskill, relatedstat) -> meters  */
  `aoe_radius` text,             /* (power, wayskill, relatedstat) -> meters  */
  `aoe_angle` text,              /* (power, wayskill, relatedstat) -> radians */
  `outcome` text NOT NULL,       /* (power, caster, target) -> side effects   */
  `cstr_npc_spell_category` varchar(200) NOT NULL DEFAULT '',
  `npc_spell_power` float(10,3) DEFAULT '0.000',
  PRIMARY KEY (id),
  UNIQUE name (name)
);


/*
 * Sample/fake spells
 */

INSERT INTO `spells` VALUES (1,'Summon Missile',1,1,'casting','/planeshift/materials/glyph_summon_icon.dds','You summon a wooden arrow and magically shoot it at your target.',1,1,32,'5000-(WaySkill+RelatedStat)/Power','40*Power','0','0','cast Summon Missile','direct damage',1.000);
INSERT INTO `spells` VALUES (2,'Life Infusion',1,1,'casting','/planeshift/materials/glyph_summon_icon.dds','By means of this spell the wizard is able to instill pure energy in a creature. The energy, which has healing effects, is less powerful but similar to the energy of the Great Crystal. It can be cast on the wizard or on another character in touch range.',0,1,24,'1000-(WaySkill+RelatedStat)/Power','3.0','0','0','cast Life Infusion','direct heal',1.000);
INSERT INTO `spells` VALUES (3,'Swiftness',2,1,'casting','/planeshift/materials/glyph_summon_icon.dds','This spell increases the agility of the caster for a time.',0,1,24,'1000-(WaySkill+RelatedStat)/Power','3.0','0','0','cast Swiftness','direct heal',1.000);
INSERT INTO `spells` VALUES (4,'Dispel',1,2,'casting','/planeshift/materials/glyph_summon_icon.dds','This spell cancels various benificial status effects on your target.',0,1,56,'2000-(WaySkill+RelatedStat)/Power','3+Power','0','0','cast Dispel','direct heal',1.000);
INSERT INTO `spells` VALUES (5,'Flame Spire',3,1,'casting','/planeshift/materials/glyph_summon_icon.dds','This spells creates a spire of flames all around the caster.',0,1,8,'2000-(WaySkill+RelatedStat)/Power','3.0','0','0','cast Flame Spire','buff',0.000);
INSERT INTO `spells` VALUES (6,'Magetank',3,1,'casting','/planeshift/materials/glyph_summon_icon.dds','Everybody wants to play one...',0,1,24,'2000-(WaySkill+RelatedStat)/Power','3.0','0','0','cast Magetank','buff',0.000);
INSERT INTO `spells` VALUES (7,'Drain',4,1,'casting','/planeshift/materials/glyph_summon_icon.dds','This spell drains the life of your opponent, over time, and gives some of it back to you.',1,1,32,'2000-(WaySkill+RelatedStat)/Power','15.0','0','0','cast Drain','direct damage',1.000);
INSERT INTO `spells` VALUES (8,'Chain Lightning',3,3,'casting','/planeshift/materials/glyph_summon_icon.dds','You shoot lighting at your target, which then jumps out at those nearby.',1,1,32,'2000-(WaySkill+RelatedStat)/Power','20.0','10','0','cast Chain Lightning','direct damage',1.000);
INSERT INTO `spells` VALUES (9,'Identify',1,1,'casting','/planeshift/materials/glyph_summon_icon.dds','You shoot lighting at your target, which then jumps out at those nearby.',0,1,4,'2000-(WaySkill+RelatedStat)/Power','20.0','0','0','cast Identify','special',1.000);
INSERT INTO `spells` VALUES (10,'Fear',4,1,'casting','/planeshift/materials/glyph_summon_icon.dds','This spell makes your NPC target flee.',1,1,32,'5000-(WaySkill+RelatedStat)/Power','40*Power','0','0','cast Fear','debuff',1.000);
INSERT INTO `spells` VALUES (11,'Charme',4,1,'casting','/planeshift/materials/glyph_summon_icon.dds','This spell charme the NPC target and it becomes your pet.',1,1,32,'5000-(WaySkill+RelatedStat)/Power','40*Power','0','0','cast Charme','debuff',1.000);
