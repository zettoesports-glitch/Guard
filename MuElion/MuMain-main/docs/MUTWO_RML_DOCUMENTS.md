# MuTwo RmlUi document inventory

This inventory was recovered from string data in `Main-x64-Debug.exe`. It
records document names only; no private/public RML contents were copied.

## Fully qualified paths observed

- Data/UI/PC/Character/character_frame.rml
- Data/UI/PC/Character/pet_frame.rml
- Data/UI/PC/Character/pet_info.rml
- Data/UI/PC/Chat/chat.rml
- Data/UI/PC/Command/command_window.rml
- Data/UI/PC/Common/common.rml
- Data/UI/PC/Common/message_box.rml
- Data/UI/PC/Common/scrollbar.rml
- Data/UI/PC/Common/tooltip.rml
- Data/UI/PC/Friend/friend.rml
- Data/UI/PC/HUD/main_frame.rml
- Data/UI/PC/HUD/master_tree.rml
- Data/UI/PC/HUD/move_command.rml
- Data/UI/PC/HUD/top_menu.rml
- Data/UI/PC/Inventory/inventory.rml
- Data/UI/PC/Inventory/inventory_extension.rml
- Data/UI/PC/Inventory/private_store.rml
- Data/UI/PC/Login/login.rml
- Data/UI/PC/Login/login_bottom.rml
- Data/UI/PC/MuHelper/mu_helper.rml
- Data/UI/PC/Option/option.rml
- Data/UI/PC/Party/party_frame.rml
- Data/UI/PC/ServerMessage/server_message.rml
- Data/UI/PC/SystemMenu/system_menu.rml
- Data/UI/PC/World/store_label.rml
- Data/UI/PC/ui_assets.rcss

## Additional RML basenames observed

- blood_castle_entry.rml
- buff_list.rml
- chaos_menu.rml
- character_create.rml
- character_frame.rml
- chat.rml
- command_window.rml
- duel_confirm.rml
- duel_result.rml
- duel_watch.rml
- elpis_menu.rml
- empire_guardian_entry.rml
- friend.rml
- fruit_choice.rml
- gatekeeper.rml
- gem_attach.rml
- gem_detach.rml
- gem_menu.rml
- gem_unit.rml
- gens_ranking.rml
- gold_archer.rml
- guild_create.rml
- guild_info.rml
- guild_position.rml
- help.rml
- hud_map_mask.rml
- hud_map_npc_label.rml
- interaction_progress.rml
- inventory.rml
- inventory_extension.rml
- item_confirmation.rml
- item_durability.rml
- item_explanation.rml
- kanturu_entry.rml
- kanturu_info.rml
- login.rml
- login_bottom.rml
- long_notice.rml
- lucky_item.rml
- lucky_menu.rml
- main_frame.rml
- map_name.rml
- master_tree.rml
- message_box.rml
- minimap.rml
- mix.rml
- monster_info.rml
- move_command.rml
- mu_helper.rml
- npc_dialogue.rml
- npc_job_change.rml
- npc_quest_progress.rml
- npc_shop.rml
- option.rml
- party_frame.rml
- pet_frame.rml
- pet_info.rml
- private_store.rml
- quest_journal.rml
- quick_command.rml
- seed_investigator_menu.rml
- seed_master_menu.rml
- server_message.rml
- skill_list_icons.rml
- storage.rml
- storage_extension.rml
- storage_keypad.rml
- storage_password.rml
- store_label.rml
- system_menu.rml
- temple_info.rml
- temple_result.rml
- temple_score.rml
- tooltip.rml
- top_menu.rml
- trade.rml
- trade_confirm.rml
- trainer_menu.rml
- trainer_recover.rml
- window_menu.rml

## Reconstruction policy

The public VDraven/MuClient repository does not currently publish a license
file. Therefore this branch does not copy its RML/RCSS contents. UI documents
are reconstructed independently from the executable's observable behavior,
the existing MuMain UI logic, and newly authored RML/RCSS.

The initial `HUD/main_frame.rml` in this branch is a neutral bootstrap. It
loads the reconstructed stylesheet and instantiates the recovered custom
`map-viewport` element without replacing the legacy HUD yet.


## Materialized reconstruction tree

The `mordenize` branch now contains independently-authored neutral RML
skeletons for every fully-qualified path listed above. The runtime loads them
lazily by path; only `HUD/main_frame.rml` is shown during bootstrap.

These skeletons intentionally contain no copied MuClient markup or styling.
They establish the recovered file/layout contract so each screen can be
reconstructed incrementally without guessing directory names or replacing the
legacy UI before parity is validated.


### Additional public PC document contract

- `Data/UI/PC/ServerSelect/server_select.rml`


- `Data/UI/PC/Character/character_create.rml`
