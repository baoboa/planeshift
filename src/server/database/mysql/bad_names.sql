#
# Table structure for table `bad_names`
#

DROP TABLE IF EXISTS `bad_names`;
CREATE TABLE `bad_names` (
  `id` int(8) NOT NULL auto_increment,
  `name` text NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=5 ;

#
# Dumping data for table `bad_names`
#

INSERT INTO `bad_names` VALUES (1, 'shit');
INSERT INTO `bad_names` VALUES (2, 'damn');
INSERT INTO `bad_names` VALUES (3, 'bastard');
INSERT INTO `bad_names` VALUES (4, 'bitch');
