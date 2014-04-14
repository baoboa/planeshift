create unique index indx_sector_name on sectors (name);
create index indx_AL_sectorname on action_locations (sectorname);
create index indx_AL_master_id on action_locations (master_id);
create index indx_char_name on characters (name);
create index indx_char_lastname on characters (lastname);
create index indx_item_instance_flags on item_instances (flags);
create index indx_petition_status on petitions (status);
create index indx_petition_escalation_level on petitions (escalation_level);
create index indx_petition_player on petitions (player);
create unique index indx_guild_name on guilds (name);

create index indx_item_instance_char_id_owner on item_instances (char_id_owner);
create index indx_item_instance_location_in_parent on item_instances (location_in_parent);
#Duplicates
#create unique index indx_accounts_username on accounts (username);
#create index indx_accounts_ip on accounts (last_login_ip);
