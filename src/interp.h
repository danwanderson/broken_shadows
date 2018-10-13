///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-1999 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/* this is a listing of all the commands and command related data */

/* for command types */
#define MR		MAX_RANK			//10
#define R1      MAX_RANK - 1		//9
#define R2      MAX_RANK - 2		//8
#define R3      MAX_RANK - 3		//7
#define R4      MAX_RANK - 4		//6
#define R5      MAX_RANK - 5		//5
#define R6      MAX_RANK - 6		//4
#define R7      MAX_RANK - 7		//3
#define R8      MAX_RANK - 8		//2
#define R9      MAX_RANK - 9		//1
#define R0      0

/*
 * Structure for a command in the command lookup table.
 */
struct  cmd_type
{
    char * const        name;
    DO_FUN *            do_fun;
    sh_int              position;
    sh_int              level;
    sh_int              log;
    bool              show;
};

/* the command table itself */
extern  const   struct  cmd_type        cmd_table       [];

/*
 * Command functions.
 * Defined in act_*.c (mostly).
 */
void do_promote( CHAR_DATA *ch, char *argument );
void do_afk( CHAR_DATA *ch, char *argument );
void do_alias( CHAR_DATA *ch, char *argument );
void do_allow( CHAR_DATA *ch, char *argument );
void do_areas( CHAR_DATA *ch, char *argument );
void do_at( CHAR_DATA *ch, char *argument );
void do_autoassist( CHAR_DATA *ch, char *argument );
void do_autoexit( CHAR_DATA *ch, char *argument );
void do_autogold( CHAR_DATA *ch, char *argument );
void do_autolist( CHAR_DATA *ch, char *argument );
void do_autoloot( CHAR_DATA *ch, char *argument );
void do_autosac( CHAR_DATA *ch, char *argument );
void do_autosplit( CHAR_DATA *ch, char *argument );
void do_backstab( CHAR_DATA *ch, char *argument );
void do_poofin ( CHAR_DATA *ch, char *argument );
void do_poofout( CHAR_DATA *ch, char *argument );
void do_ban( CHAR_DATA *ch, char *argument );
void do_bash( CHAR_DATA *ch, char *argument );
void do_berserk( CHAR_DATA *ch, char *argument );
void do_brandish( CHAR_DATA *ch, char *argument );
void do_brief( CHAR_DATA *ch, char *argument );
void do_buy( CHAR_DATA *ch, char *argument );
void do_cast( CHAR_DATA *ch, char *argument );
void do_changes( CHAR_DATA *ch, char *argument );
void do_channels( CHAR_DATA *ch, char *argument );
void do_clone( CHAR_DATA *ch, char *argument );
void do_close( CHAR_DATA *ch, char *argument );
void do_commands( CHAR_DATA *ch, char *argument );
void do_combine( CHAR_DATA *ch, char *argument );
void do_compact( CHAR_DATA *ch, char *argument );
void do_compare( CHAR_DATA *ch, char *argument );
void do_consider( CHAR_DATA *ch, char *argument );
void do_credits( CHAR_DATA *ch, char *argument );
void do_delet( CHAR_DATA *ch, char *argument );
void do_delete( CHAR_DATA *ch, char *argument );
void do_deny( CHAR_DATA *ch, char *argument );
void do_description( CHAR_DATA *ch, char *argument );
void do_dirt( CHAR_DATA *ch, char *argument );
void do_disarm( CHAR_DATA *ch, char *argument );
void do_disconnect( CHAR_DATA *ch, char *argument );
void do_down( CHAR_DATA *ch, char *argument );
void do_drink( CHAR_DATA *ch, char *argument );
void do_drop( CHAR_DATA *ch, char *argument );
void do_east( CHAR_DATA *ch, char *argument );
void do_eat( CHAR_DATA *ch, char *argument );
void do_echo( CHAR_DATA *ch, char *argument );
void do_affects( CHAR_DATA *ch, char *argument );
void do_emote( CHAR_DATA *ch, char *argument );
void do_equipment( CHAR_DATA *ch, char *argument );
void do_examine( CHAR_DATA *ch, char *argument );
void do_exits( CHAR_DATA *ch, char *argument );
void do_fill( CHAR_DATA *ch, char *argument );
void do_flee( CHAR_DATA *ch, char *argument );
void do_follow( CHAR_DATA *ch, char *argument );
void do_force( CHAR_DATA *ch, char *argument );
void do_freeze( CHAR_DATA *ch, char *argument );
void do_gain( CHAR_DATA *ch, char *argument );
void do_get( CHAR_DATA *ch, char *argument );
void do_give( CHAR_DATA *ch, char *argument );
void do_gossip( CHAR_DATA *ch, char *argument );
void do_goto( CHAR_DATA *ch, char *argument );
void do_group( CHAR_DATA *ch, char *argument );
void do_groups( CHAR_DATA *ch, char *argument );
void do_gtell( CHAR_DATA *ch, char *argument );
void do_heal( CHAR_DATA *ch, char *argument );
void do_help( CHAR_DATA *ch, char *argument );
void do_hide( CHAR_DATA *ch, char *argument );
void do_holylight( CHAR_DATA *ch, char *argument );
void do_immtalk( CHAR_DATA *ch, char *argument );
void do_imotd( CHAR_DATA *ch, char *argument );
void do_inventory( CHAR_DATA *ch, char *argument );
void do_invis( CHAR_DATA *ch, char *argument );
void do_kick( CHAR_DATA *ch, char *argument );
void do_kill( CHAR_DATA *ch, char *argument );
void do_list( CHAR_DATA *ch, char *argument );
void do_load( CHAR_DATA *ch, char *argument );
void do_lock( CHAR_DATA *ch, char *argument );
void do_log( CHAR_DATA *ch, char *argument );
void do_look( CHAR_DATA *ch, char *argument );
void do_memory( CHAR_DATA *ch, char *argument );
void do_mfind( CHAR_DATA *ch, char *argument );
void do_mload( CHAR_DATA *ch, char *argument );
void do_mset( CHAR_DATA *ch, char *argument );
void do_mstat( CHAR_DATA *ch, char *argument );
void do_mwhere( CHAR_DATA *ch, char *argument );
void do_motd( CHAR_DATA *ch, char *argument );
void do_murde( CHAR_DATA *ch, char *argument );
void do_murder( CHAR_DATA *ch, char *argument );
void do_music( CHAR_DATA *ch, char *argument );
void do_newlock( CHAR_DATA *ch, char *argument );
void do_nochannels( CHAR_DATA *ch, char *argument );
void do_noemote( CHAR_DATA *ch, char *argument );
void do_nofollow( CHAR_DATA *ch, char *argument );
void do_noloot( CHAR_DATA *ch, char *argument );
void do_north( CHAR_DATA *ch, char *argument );
void do_nosummon( CHAR_DATA *ch, char *argument );
void do_nocolor( CHAR_DATA *ch, char *argument );
void do_note( CHAR_DATA *ch, char *argument );
void do_notell( CHAR_DATA *ch, char *argument );
void do_ofind( CHAR_DATA *ch, char *argument );
void do_oload( CHAR_DATA *ch, char *argument );
void do_open( CHAR_DATA *ch, char *argument );
void do_order( CHAR_DATA *ch, char *argument );
void do_oset( CHAR_DATA *ch, char *argument );
void do_ostat( CHAR_DATA *ch, char *argument );
void do_outfit( CHAR_DATA *ch, char *argument );
void do_pardon( CHAR_DATA *ch, char *argument );
void do_password( CHAR_DATA *ch, char *argument );
void do_peace( CHAR_DATA *ch, char *argument );
void do_pecho( CHAR_DATA *ch, char *argument );
void do_pick( CHAR_DATA *ch, char *argument );
void do_pk( CHAR_DATA *ch, char *argument );
void do_pour( CHAR_DATA *ch, char *argument );
void do_practice( CHAR_DATA *ch, char *argument );
void do_prompt( CHAR_DATA *ch, char *argument );
void do_purge( CHAR_DATA *ch, char *argument );
void do_put( CHAR_DATA *ch, char *argument );
void do_quaff( CHAR_DATA *ch, char *argument );
void do_qui( CHAR_DATA *ch, char *argument );
void do_quiet( CHAR_DATA *ch, char *argument );
void do_quit( CHAR_DATA *ch, char *argument );
void do_read( CHAR_DATA *ch, char *argument );
void do_reboo( CHAR_DATA *ch, char *argument );
void do_reboot( CHAR_DATA *ch, char *argument );
void do_recall( CHAR_DATA *ch, char *argument );
void do_recho( CHAR_DATA *ch, char *argument );
void do_recite( CHAR_DATA *ch, char *argument );
void do_remove( CHAR_DATA *ch, char *argument );
void do_reply( CHAR_DATA *ch, char *argument );
void do_repop( CHAR_DATA *ch, char *argument );
void do_report( CHAR_DATA *ch, char *argument );
void do_rescue( CHAR_DATA *ch, char *argument );
void do_rest( CHAR_DATA *ch, char *argument );
void do_restore( CHAR_DATA *ch, char *argument );
void do_return( CHAR_DATA *ch, char *argument );
void do_rset( CHAR_DATA *ch, char *argument );
void do_rstat( CHAR_DATA *ch, char *argument );
void do_rules( CHAR_DATA *ch, char *argument );
void do_sacrifice( CHAR_DATA *ch, char *argument );
void do_save( CHAR_DATA *ch, char *argument );
void do_say( CHAR_DATA *ch, char *argument );
void do_scan( CHAR_DATA *ch, char *argument );
void do_score( CHAR_DATA *ch, char *argument );
void do_scroll( CHAR_DATA *ch, char *argument );
void do_sell( CHAR_DATA *ch, char *argument );
void do_set( CHAR_DATA *ch, char *argument );
void do_shutdow( CHAR_DATA *ch, char *argument );
void do_shutdown( CHAR_DATA *ch, char *argument );
void do_sit( CHAR_DATA *ch, char *argument );
void do_skills( CHAR_DATA *ch, char *argument );
void do_sla( CHAR_DATA *ch, char *argument );
void do_slay( CHAR_DATA *ch, char *argument );
void do_sleep( CHAR_DATA *ch, char *argument );
void do_slookup( CHAR_DATA *ch, char *argument );
void do_sneak( CHAR_DATA *ch, char *argument );
void do_snoop( CHAR_DATA *ch, char *argument );
void do_socials( CHAR_DATA *ch, char *argument );
void do_south( CHAR_DATA *ch, char *argument );
void do_sockets( CHAR_DATA *ch, char *argument );
void do_spells( CHAR_DATA *ch, char *argument );
void do_split( CHAR_DATA *ch, char *argument );
void do_sset( CHAR_DATA *ch, char *argument );
void do_stand( CHAR_DATA *ch, char *argument );
void do_stat( CHAR_DATA *ch, char *argument );
void do_steal( CHAR_DATA *ch, char *argument );
void do_string( CHAR_DATA *ch, char *argument );
void do_switch( CHAR_DATA *ch, char *argument );
void do_tell( CHAR_DATA *ch, char *argument );
void do_time( CHAR_DATA *ch, char *argument );
void do_title( CHAR_DATA *ch, char *argument );
void do_train( CHAR_DATA *ch, char *argument );
void do_transfer( CHAR_DATA *ch, char *argument );
void do_trip( CHAR_DATA *ch, char *argument );
void do_unlock( CHAR_DATA *ch, char *argument );
void do_up( CHAR_DATA *ch, char *argument );
void do_value( CHAR_DATA *ch, char *argument );
void do_visible( CHAR_DATA *ch, char *argument );
void do_vnum( CHAR_DATA *ch, char *argument );
void do_wake( CHAR_DATA *ch, char *argument );
void do_wear( CHAR_DATA *ch, char *argument );
void do_weather( CHAR_DATA *ch, char *argument );
void do_west( CHAR_DATA *ch, char *argument );
void do_where( CHAR_DATA *ch, char *argument );
void do_who( CHAR_DATA *ch, char *argument );
void do_whoname( CHAR_DATA *ch, char *argument );
void do_wimpy( CHAR_DATA *ch, char *argument );
void do_wizhelp( CHAR_DATA *ch, char *argument );
void do_wizlock( CHAR_DATA *ch, char *argument );
void do_wizlist( CHAR_DATA *ch, char *argument );
void do_worth( CHAR_DATA *ch, char *argument );
void do_yell( CHAR_DATA *ch, char *argument );
void do_zap( CHAR_DATA *ch, char *argument );
void do_info( CHAR_DATA *ch, char *argument );
void do_sinfo( CHAR_DATA *ch, char *argument );
void do_search( CHAR_DATA *ch, char *argument );
void do_beacon( CHAR_DATA *ch, char *argument );
void do_chaos( CHAR_DATA *ch, char *argument );
void do_cwho( CHAR_DATA *ch, char *argument );
void do_alias( CHAR_DATA *ch, char *argument );
void do_unalias( CHAR_DATA *ch, char *argument );
void do_brew( CHAR_DATA *ch, char *argument );
void do_scribe( CHAR_DATA *ch, char *argument );
void do_show( CHAR_DATA *ch, char *argument );
void do_clan( CHAR_DATA *ch, char *argument );
void do_finger( CHAR_DATA *ch, char *argument );
void do_rlist( CHAR_DATA *ch, char *argument );
void do_test( CHAR_DATA *ch, char *argument );
void do_new_discon( CHAR_DATA *ch, char *argument );

/* new commands by Rahl */
void do_ctell( CHAR_DATA *ch, char *argument );
void do_status( CHAR_DATA *ch, char *argument );
void do_push( CHAR_DATA *ch, char *argument );
void do_wizslap( CHAR_DATA *ch, char *argument );
void do_butcher( CHAR_DATA *ch, char *argument );
void do_copyover( CHAR_DATA *ch, char *argument );
void do_quest( CHAR_DATA *ch, char *argument );
void do_circle( CHAR_DATA *ch, char *argument );
void do_bonus( CHAR_DATA *ch, char *argument );
void do_lore( CHAR_DATA *ch, char *argument );
void do_wiznet( CHAR_DATA *ch, char *argument );
void do_grip( CHAR_DATA *ch, char *argument );
void do_envenom( CHAR_DATA *ch, char *argument );
void do_enter( CHAR_DATA *ch, char *argument );
void do_incognito( CHAR_DATA *ch, char *argument );
void do_clan_recall( CHAR_DATA *ch, char *argument );
void do_page( CHAR_DATA *ch, char *argument );
void do_balance( CHAR_DATA *ch, char *argument );
void do_deposit( CHAR_DATA *ch, char *argument );
void do_withdraw( CHAR_DATA *ch, char *argument );
void do_accept( CHAR_DATA *ch, char *argument );
void do_disown( CHAR_DATA *ch, char *argument );
void do_rename( CHAR_DATA *ch, char *argument );
void do_bounty( CHAR_DATA *ch, char *argument );
void do_renam( CHAR_DATA *ch, char *argument );
void do_board( CHAR_DATA *ch, char *argument );
void do_email( CHAR_DATA *ch, char *argument );
void do_message( CHAR_DATA *ch, char *argument );
void do_comment( CHAR_DATA *ch, char *argument );
void do_whirlwind( CHAR_DATA *ch, char *argument );
void do_petition( CHAR_DATA *ch, char *argument );
void do_auction( CHAR_DATA *ch, char *argument );
void do_disable( CHAR_DATA *ch, char *argument );
void do_new_changes( CHAR_DATA *ch, char *argument );
void do_addapply( CHAR_DATA *ch, char *argument );
void do_permban( CHAR_DATA *ch, char *argument );
void do_play( CHAR_DATA *ch, char *argument );
void do_game( CHAR_DATA *ch, char *argument );
void do_grab( CHAR_DATA *ch, char *argument );
void do_owhere( CHAR_DATA *ch, char *argument );
void do_jail( CHAR_DATA *ch, char *argument );
void do_autodamage( CHAR_DATA *ch, char *argument );
void do_purchase( CHAR_DATA *ch, char *argument );
void do_nobounty( CHAR_DATA *ch, char *argument );
void do_gocial( CHAR_DATA *ch, char *argument );
void do_engrave( CHAR_DATA *ch, char *argument );
void do_marry( CHAR_DATA *ch, char *argument );
void do_divorce( CHAR_DATA *ch, char *argument );
void do_spousetalk( CHAR_DATA *ch, char *argument );
void do_consent( CHAR_DATA *ch, char *argument );
void do_states( CHAR_DATA *ch, char *argument );
void do_areport( CHAR_DATA *ch, char *argument );
void do_protect( CHAR_DATA *ch, char *argument );
void do_violate( CHAR_DATA *ch, char *argument );
void do_gmote( CHAR_DATA *ch, char *argument );
void do_smote( CHAR_DATA *ch, char *argument );
void do_pmote( CHAR_DATA *ch, char *argument );
void do_zecho( CHAR_DATA *ch, char *argument );
void do_prefi( CHAR_DATA *ch, char *argument );
void do_prefix( CHAR_DATA *ch, char *argument );
void do_permit( CHAR_DATA *ch, char *argument );
void do_permba( CHAR_DATA *ch, char *argument );
void do_debug( CHAR_DATA *ch, char *argument );
void do_sendinfo( CHAR_DATA *ch, char *argument );
void do_asave( CHAR_DATA *ch, char *argument );
void do_olc( CHAR_DATA *ch, char *argument );
void do_resets( CHAR_DATA *ch, char *argument );
void do_alist( CHAR_DATA *ch, char *argument );
void do_quote_channel( CHAR_DATA *ch, char *argument );
