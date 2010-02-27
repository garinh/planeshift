drop database if exists planeshift;
create database planeshift;
use planeshift;

source accessrules.sql;
source accounts.sql;
source action_locations.sql;
source alliances.sql;
source armorvsweapon.sql;
source bad_names.sql;
source bans.sql;
source character_relationships.sql;
source characters.sql;
source character_limitations.sql;
source char_creation.sql;
source character_quests.sql;
source char_skills.sql;
source char_traits.sql;
source command_access.sql;
source factions.sql;
source familiar_types.sql;
source gameboards.sql;
source gm_command_log.sql;
source guildlevels.sql;
source guilds.sql;
source hunt_locations.sql;
source item_categories.sql;
source item_instances.sql;
source item_stats.sql;
source item_animations.sql;
source loot_modifiers.sql;
source loot_rules.sql;
source loot_rule_details.sql;
source math_scripts.sql;
source merchant_item_cats.sql;
source migration.sql;
source money_events.sql;
source movement.sql;
source natural_resources.sql;
source npc_bad_text.sql;
source npc_disallowed_words.sql;
source npc_kas.sql;
source npc_responses.sql;
source npc_spawn_ranges.sql;
source npc_spawn_rules.sql;
source npc_synonyms.sql;
source npc_trigger_groups.sql;
source npc_triggers.sql;
source petitions.sql;
source player_spells.sql;
source progress_events.sql;
source quest_scripts.sql;
source quests.sql;
source race_info.sql;
source race_spawns.sql;
source sc_locations.sql;
source sc_location_type.sql;
source sc_npc_definitions.sql;
source sc_tribe_memories.sql;
source sc_tribe_resources.sql;
source sc_waypoints.sql;
source sc_waypoint_aliases.sql;
source sc_waypoint_links.sql;
source sc_path_points.sql;
source sectors.sql;
source security_levels.sql;
source server_options.sql;
source skills.sql;
source spells.sql;
source spell_glyphs.sql;
source stances.sql;
source trainer_skills.sql;
source traits.sql;
source tribes.sql;
source tribe_members.sql;
source unique_content.sql;
source ways.sql;
source trade_combinations.sql;
source trade_patterns.sql;
source trade_processes.sql;
source trade_transformations.sql;
source trade_autocontainers.sql;
source trade_constraints.sql;
source tips.sql;
source char_create_affinity.sql;
source gm_events.sql;
source character_events.sql;
source introductions.sql;
source warnings.sql;
source guild_wars.sql;
source wc_accessrules.sql;
source wc_cmdlog.sql;
source wc_statistics.sql;



source create_indexes.sql;