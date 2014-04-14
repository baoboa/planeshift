DROP TABLE IF EXISTS `money_events`;
CREATE TABLE money_events(
  id int(8) unsigned NOT NULL,
  taxid int(8) unsigned NOT NULL,
  guild bool,
  nextEvent mediumint(6) unsigned NOT NULL,
  inter int(3) NOT NULL,
  totaltrias int(10),
  prg_evt_paid text,
  prg_evt_npaid text,
  prg_evn_fnpaid text,
  latePayment bool,
  lateBy int(3) NOT NULL,
  instance int(11) default '0',
  PRIMARY KEY (id),
  UNIQUE id (id)
);

INSERT INTO money_events VALUES("1", "1", "1", "107203", "7", "1", "", "", "", "0", "0", "0");
