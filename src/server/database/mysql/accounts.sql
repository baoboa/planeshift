# MySQL-Front 3.2  (Build 13.0)

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES */;

/*!40101 SET NAMES latin1 */;
/*!40103 SET TIME_ZONE='SYSTEM' */;

# Host: localhost    Database: planeshift
# ------------------------------------------------------
# Server version 5.0.19-nt

#
# Table structure for table accounts
#

DROP TABLE IF EXISTS `accounts`;
CREATE TABLE  `accounts` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `username` varchar(50) NOT NULL DEFAULT '0',
  `password` varchar(64) NOT NULL DEFAULT '0',
  `password256` varchar(64) NOT NULL DEFAULT '0',
  `last_login` datetime DEFAULT NULL,
  `created_date` datetime DEFAULT NULL,
  `last_login_ip` varchar(16) DEFAULT NULL,
  `security_level` tinyint(3) NOT NULL DEFAULT '0',
  `verificationid` varchar(32) DEFAULT NULL,
  `country` varchar(64) DEFAULT NULL,
  `gender` varchar(1) DEFAULT 'N',
  `birth` int(4) DEFAULT '0',
  `status` char(1) DEFAULT NULL,
  `spam_points` tinyint(1) DEFAULT '0',
  `advisor_points` int(10) NOT NULL DEFAULT '0',
  `comments` text,
  `realname` varchar(64) DEFAULT NULL,
  `advisor_ban` tinyint(1) DEFAULT '0',
  `operating_system` varchar(32) DEFAULT NULL,
  `graphics_card` varchar(64) DEFAULT NULL,
  `graphics_version` varchar(32) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`),
  UNIQUE KEY `id` (`id`),
  KEY `indx_accounts_ip` (`last_login_ip`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;


#
# Dumping data for table accounts
#

#password is "keith"
INSERT INTO `accounts` VALUES (1,'vengeance','8dd9fa632ca161d0ca1929a4d99cbe77','a4e463d7dec930ea7f5a7d92fc3b798a167c7c206438739d4c82d96d5ebf2cfe','2006-06-24 00:47:50','2001-10-10 00:00:00','127.0.0.1',30,'','US','M',1980,'A',0,0,'none','',0, '', '', '');

#password is "guest"
INSERT INTO `accounts` VALUES (3,'guest','084e0343a0486ff05530df6c705c8bb4','84983c60f7daadc1cb8698621f802c0d9f9a3c3c295c810748fb048115c186ec','2003-07-22 00:31:36','2001-10-10 00:00:00','127.0.0.1',30,'','US','M',1980,'A',0,0,'none','',0, '', '', '');

#password is "andrew"
INSERT INTO `accounts` VALUES (5,'acraig','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2001-11-11 00:00:00','0000-00-00 00:00:00','',0,'','US','M',1980,'A',0,0,'none','',0, '', '', '');

INSERT INTO `accounts` VALUES (9,'superclient','e1047916967c28e68172ef5bfe31ed407b5a6d568def7cb74018704ea25da522','0','2003-07-17 14:46:08','2001-11-11 00:00:00','127.0.0.1',99,'','US','M',1980,'A',0,0,'none','',0, '', '', '');
INSERT INTO `accounts` VALUES (10,'superclient2','5f4dcc3b5aa765d61d8327deb882cf99','0',NULL,NULL,NULL,99,NULL,NULL,'N',0,NULL,0,0,NULL,NULL,0, '', '', '');

#password is "andrew" for all the following accounts
INSERT INTO `accounts` VALUES (11,'dev1','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',30,'','US','M',1980,'A',0,0,'none','',0, '', '', '');
INSERT INTO `accounts` VALUES (12,'dev2','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',30,'','US','M',1980,'A',0,0,'none','',0, '', '', '');

INSERT INTO `accounts` VALUES (13,'gm1','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',22,'','US','M',1980,'A',0,0,'none','',0, '', '', '');
INSERT INTO `accounts` VALUES (14,'gm2','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',26,'','US','M',1980,'A',0,0,'none','',0, '', '', '');

INSERT INTO `accounts` VALUES (15,'player1','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',0,'','US','M',1980,'A',0,0,'none','',0, '', '', '');
INSERT INTO `accounts` VALUES (16,'player2','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',0,'','US','M',1980,'A',0,0,'none','',0, '', '', '');
INSERT INTO `accounts` VALUES (17,'player3','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',0,'','US','M',1980,'A',0,0,'none','',0, '', '', '');
INSERT INTO `accounts` VALUES (18,'player4','d914e3ecf6cc481114a3f534a5faf90b','d979885447a413abb6d606a5d0f45c3b7809e6fde2c83f0df3426f1fc9bfed97','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',0,'','US','M',1980,'A',0,0,'none','',0, '', '', '');

#password is "planeshift" for all the following accounts
INSERT INTO `accounts` VALUES (19,'admin1','b30ee7d55aa2808cc143418fc1b93b46','4ab82ef332d3f38848c71ae8e6e10a40c0c428cfc70644e3d51ee47b89a2e5e7','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',50,'','US','M',1980,'A',0,0,'none','',0, '', '', '');
INSERT INTO `accounts` VALUES (20,'admin2','b30ee7d55aa2808cc143418fc1b93b46','4ab82ef332d3f38848c71ae8e6e10a40c0c428cfc70644e3d51ee47b89a2e5e7','2006-06-24 00:47:50','0000-00-00 00:00:00','127.0.0.1',50,'','US','M',1980,'A',0,0,'none','',0, '', '', '');


/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
