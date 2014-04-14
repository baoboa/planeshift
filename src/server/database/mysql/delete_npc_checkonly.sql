
# You can use this to check if there are any dependencies on other tables before deleting a monster

set @npcid=9581169;
select count(*) from character_traits where character_id=@npcid;
select count(*) from item_instances where char_id_owner=@npcid;
select count(*) from character_skills where character_id=@npcid;
select count(*) from npc_knowledge_areas where player_id=@npcid;
select count(*) from merchant_item_categories where player_id=@npcid;
select count(*) from trainer_skills WHERE player_id=@npcid;
select count(*) from characters WHERE player_id=@npcid;
select count(*) from sc_npc_definitions WHERE char_id=@npcid;

#if part of a tribe
select count(*) from tribe_members where member_id=@npcid;

# Others tables you may be interested in checking for sentient NPCs are:

# npc_triggers where area=@npc_name;
# npc_bad_text where npc=@npc_name;
# npc_knowledge_areas where player_id=@npcid;
# npc_responses where id>=@min_resp and id<=@max_resp;
