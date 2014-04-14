# MySQL-Front Dump 1.16 beta
#
# Host: localhost Database: planeshift
#--------------------------------------------------------
# Server version 3.23.52-max-nt
#
# Table structure for table 'traits'
#
# next_trait is used for traits that is composed of multiple traits. Eg. skin have to have traits for hands, feets,...
# race_id describe the race and the gender this trait apply to. Gender is taken from the race_info table
# Location describes what this trait is for.  Each following field can mean different things for different
# traits.
#
# HAIR_STYLE
# BEARD_STYLE
# FACE     :   applied to the face of the character.
#                 - Material is the texture that will be the face.
#                 - Texture is used to created the material if the material does not exists.
#                 - Mesh is the mesh that the material is assigned to.  
#                 - Submesh not used.
#               
# HAIR_COLOR    : Hair color is composed of two traits. 1. is used to the beard. 2. is used for the hair.
#                 - Material is the texture that will be the hair/beard.
#                 - Texture is used to create the material if the material does not exist.
#                 - Mesh is the mesh that the material is assigned to.  
#                 - Submesh not used.
#
# EYE_COLOR     : Eye color is a single trait.
#                 - Material is the texture that will be the eyes.
#                 - Texture is used to create the material if the material does not exist.
#                 - Mesh is the mesh that the material is assigned to. 
#                 - Submesh not used.
#
# SKIN_TONE     : Skin tone is composed of traits. Each trait is applayed to the given mesh.
#
# ITEM          : Traits that are to be applied when a item is equiped.
#
# The setting team will be responsible for building these correctly, from the server console.
#

DROP TABLE IF EXISTS `traits`;
CREATE TABLE traits (
  id int(10) unsigned NOT NULL auto_increment,  
  next_trait int(10) NOT NULL DEFAULT '-1',
  race_id int(30),
  only_npc int(1),
  location varchar(30),
  name varchar(30),
  `cstr_mesh` varchar(200) NOT NULL DEFAULT '',
  `cstr_material` varchar(200) NOT NULL DEFAULT '',
  `cstr_texture` varchar(200) NOT NULL DEFAULT '',
  shader varchar(32),
  PRIMARY KEY (id),
  UNIQUE id (id)
);

INSERT INTO `traits` VALUES (1,-1,0,0,'FACE','Face 1','Head','$F_head','/planeshift/models/$F/$F_head.dds','');
INSERT INTO `traits` VALUES (2,-1,14,0,'FACE','Face 1','Head','$F_head','/planeshift/models/$F/$F_head.dds','');
INSERT INTO `traits` VALUES (3,-1,2,0,'FACE','Face 1','Head','$F_head','/planeshift/models/$F/$F_head.dds','');
INSERT INTO `traits` VALUES (4,-1,2,0,'FACE','Face 2','Head','$F_head_2','/planeshift/models/$F/$F_head_2.dds','');


