#
# How to delete an NPC
# 
# 
# use this command to know which NPCs are currently loaded
# select name from players where alive_ind='Y';

# set here the name of the NPC you want to delete
set @npc_name='Aleena Arlavin';

# get ID of NPC
select @npc_id:=id from characters where name=@npc_name;

# get range of values of NPC (THIS ASSUME IT WAS LOADED SERIALIZED WITH OTHER NPCs)
select @min_resp:=min(response_id) from npc_triggers where area=@npc_name;
select @max_resp:=max(response_id) from npc_triggers where area=@npc_name;

# perform deletion
delete from npc_bad_text where npc=@npc_name;
delete from npc_knowledge_areas where player_id=@npc_id;

# replace min and max with values obtained above
delete from npc_responses where id>=@min_resp and id<=@max_resp;

delete from npc_triggers where area=@npc_name;
delete from item_instances where char_id_owner=@npc_id;
delete from character_skills where character_id=@npc_id;
delete from merchant_item_categories where player_id=@npc_id;
delete from trainer_skills where player_id=@npc_id;

#delete npcclient entries
delete from sc_npc_definitions WHERE char_id=@npc_id;

delete from characters where id=@npc_id;
