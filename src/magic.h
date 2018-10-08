///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/* spells used in Merc */

/*
 * Spell functions.
 * Defined in magic.c.
 */
void spell_null            ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_armor           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_bless           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_blindness       ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_call_lightning  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_calm            ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_cancellation    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_change_sex      ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_chain_lightning ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_charm_person    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_chill_touch     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_continual_light ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_control_weather ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_create_food     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_create_spring   ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_create_water    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_cure_blindness  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_cure_disease    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_cure_poison     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_curse           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_demonfire       ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_detect_evil     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_detect_hidden   ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_detect_invis    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_detect_magic    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_detect_poison   ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_dispel_evil     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_dispel_magic    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_earthquake      ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_enchant_armor   ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_enchant_weapon  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_energy_drain    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_faerie_fire     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_faerie_fog      ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_fireball        ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_fly             ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_frenzy          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_gate            ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_giant_strength  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_harm            ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_haste           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_heal            ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_holy_word       ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_identify        ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_infravision     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_invis           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_know_alignment  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_lightning_bolt  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_locate_object   ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_magic_missile   ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_mass_healing    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_mass_invis      ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_pass_door       ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_plague          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_poison          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_protection_evil ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_refresh         ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_remove_curse    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_sanctuary       ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_shield          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_sleep           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_stone_skin      ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_summon          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_teleport        ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_ventriloquate   ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_weaken          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_word_of_recall  ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_disintegrate    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_vision          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_regeneration    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_web             ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_imprint         ( int sn, int level, CHAR_DATA *ch, void *vo, int target );

/* new spells by Rahl */
void spell_flame_sword     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_protection_good ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_create_rose     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_detect_good     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_dispel_good     ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_fireproof       ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_slow            ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_heat_metal      ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_ray_of_truth    ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_recharge        ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_portal          ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_nexus           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_resurrect       ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_fear            ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
void spell_blink           ( int sn, int level, CHAR_DATA *ch, void *vo, int target );
