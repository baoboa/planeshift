# Movement database tables.
# Defines all available character states and motions.

# List of character modes and their properties.  normal, run, combat, swim, etc.
DROP TABLE IF EXISTS `movement_modes`;
CREATE TABLE `movement_modes` (
  `id` int(11) NOT NULL,
  `name` varchar(100) NOT NULL default '',
  `move_mod_x` float NOT NULL default '1.0'          COMMENT 'x-direction motion multiplier' ,
  `move_mod_y` float NOT NULL default '1.0'          COMMENT 'y-direction motion multiplier' ,
  `move_mod_z` float NOT NULL default '1.0'          COMMENT 'z-direction motion multiplier' ,
  `rotate_mod_x` float NOT NULL default '1.0'        COMMENT 'x-axis rotation multiplier' ,
  `rotate_mod_y` float NOT NULL default '1.0'        COMMENT 'y-axis rotation multiplier' ,
  `rotate_mod_z` float NOT NULL default '1.0'        COMMENT 'z-axis rotation multiplier' ,
  `idle_animation` varchar(100) NOT NULL default ''  COMMENT 'animation when not moving' ,
  PRIMARY KEY  (`id`)
);

# List of base movement types and their properties.  forward, backwards, jump, etc.
DROP TABLE IF EXISTS `movement_types`;
CREATE TABLE `movement_types` (
  `id` int(11) NOT NULL,
  `name` varchar(100) NOT NULL default '',
  `base_move_x` float NOT NULL default '0'           COMMENT 'x-direction motion' ,
  `base_move_y` float NOT NULL default '0'           COMMENT 'y-direction motion' ,
  `base_move_z` float NOT NULL default '0'           COMMENT 'z-direction motion' ,
  `base_rotate_x` float NOT NULL default '0'         COMMENT 'x-axis rotation' ,
  `base_rotate_y` float NOT NULL default '0'         COMMENT 'y-axis rotation' ,
  `base_rotate_z` float NOT NULL default '0'         COMMENT 'z-axis rotation' ,
  PRIMARY KEY  (`id`)
);

# Relative to the character, +z is backwards, +x is left, and +y is up.
# Both lists must be continuous starting at ID 0.

# Modes  (default must be first)     #    name     x    y    z    xr   yr   zr   animation
INSERT INTO `movement_modes` VALUES (0, 'normal', 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 'stand'        );
INSERT INTO `movement_modes` VALUES (1, 'run',    1.2, 1.8, 2.0, 1.0, 1.5, 1.0, 'stand'        );
INSERT INTO `movement_modes` VALUES (2, 'sneak',  0.4, 0.4, 0.4, 0.8, 0.8, 0.8, 'stand'        );
INSERT INTO `movement_modes` VALUES (3, 'ride',   2.4, 3.6, 4.0, 2.0, 3.0, 2.0, 'stand'        );

# Motions                            #    name            x     y     z     xr    yr    zr
INSERT INTO `movement_types` VALUES (0, 'none',          0.0,  0.0,  0.0,  0.0,  0.0,  0.0);
INSERT INTO `movement_types` VALUES (1, 'forward',       0.0,  0.0, -3.0,  0.0,  0.0,  0.0);
INSERT INTO `movement_types` VALUES (2, 'backward',      0.0,  0.0, +3.0,  0.0,  0.0,  0.0);
INSERT INTO `movement_types` VALUES (3, 'rotate left',   0.0,  0.0,  0.0,  0.0, +1.5,  0.0);
INSERT INTO `movement_types` VALUES (4, 'rotate right',  0.0,  0.0,  0.0,  0.0, -1.5,  0.0);
INSERT INTO `movement_types` VALUES (5, 'strafe left',  +2.0,  0.0,  0.0,  0.0,  0.0,  0.0);
INSERT INTO `movement_types` VALUES (6, 'strafe right', -2.0,  0.0,  0.0,  0.0,  0.0,  0.0);
INSERT INTO `movement_types` VALUES (7, 'jump',          0.0, +3.0,  0.0,  0.0,  0.0,  0.0);
