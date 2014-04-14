#
# How to delete a char
# 
# The tables populated by a char are:
#
# characters
# character_glyphs
# character_quests
# character_skills
# character_traits
# item_instances
# player_spells
# 
# Use this to find all chars of all players
# select c.id,c.name,a.username from characters c, accounts a where c.account_id=a.id order by a.username;
#
# Use this to find all chars of a certain player
# select c.id,c.name,a.username from characters c, accounts a where c.account_id=a.id and a.username="pancallo@netscape.net" order by a.username;

# set here the id of the player you want to delete


# perform deletion for a range of IDS
set @player_id_min=9000074;
set @player_id_max=9000077;
delete from player_spells where player_id>=@player_id_min and player_id<=@player_id_max;
delete from item_instances where char_id_owner>=@player_id_min and char_id_owner<=@player_id_max;
delete from character_traits where character_id>=@player_id_min and character_id<=@player_id_max;
delete from character_skills where character_id>=@player_id_min and character_id<=@player_id_max;
delete from character_quests where player_id>=@player_id_min and player_id<=@player_id_max;
delete from character_glyphs where player_id>=@player_id_min and player_id<=@player_id_max;
delete from characters where id>=@player_id_min and id<=@player_id_max;



# perform deletion for one char only
set @player_id=9242522;
delete from player_spells where player_id=@player_id;
delete from item_instances where char_id_owner=@player_id;
delete from character_traits where character_id=@player_id;
delete from character_skills where character_id=@player_id;
delete from character_quests where player_id=@player_id;
delete from character_glyphs where player_id=@player_id;
delete from characters where id=@player_id;
