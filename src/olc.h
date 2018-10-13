///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2018 by Daniel Anderson
////  
////  Permission to use this code is given under the conditions set
////  forth in ../doc/shadows.license
////
////  For a complete list of credits, please see ../doc/shadows.credits
///////////////////////////////////////////////////////////////////////


/***************************************************************************
 *  File: olc.h                                                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/
/*
 * This is a header file for all the OLC files.  Feel free to copy it into
 * merc.h if you wish.  Many of these routines may be handy elsewhere in
 * the code.  -Jason Dinkel
 */


/*
 * The version info.  Please use this info when reporting bugs.
 * It is displayed in the game by typing 'version' while editing.
 * Do not remove these from the code - by request of Jason Dinkel
 */
#define VERSION "ILAB Online Creation [Beta 1.0, ROM 2.3 modified]"
#define AUTHOR  "     By Jason(jdinkel@mines.colorado.edu)\n\r" \
                "     Modified for use with ROM 2.3\n\r"        \
                "     By Hans Birkeland (hansbi@ifi.uio.no)"
#define DATE    "     (Apr. 7, 1995 - ROM mod, Apr 16, 1995)"
#define CREDITS "     Original by Surreality(cxw197@psu.edu) and Locke(locke@lm.com)"



/*
 * New typedefs.
 */
typedef bool OLC_FUN            args( ( CHAR_DATA *ch, char *argument ) );


/*
 * Connected states for editor.
 */
#define ED_AREA 1
#define ED_ROOM 2
#define ED_OBJECT 3
#define ED_MOBILE 4

/* 
 * Interpreter Prototypes
 */
void aedit( CHAR_DATA *ch, char *argument );
void redit( CHAR_DATA *ch, char *argument );
void medit( CHAR_DATA *ch, char *argument );
void oedit( CHAR_DATA *ch, char *argument );

/*
 * OLC Constants
 */
#define MAX_MOB 1               /* Default maximum number for resetting mobs */



/*
 * Structure for an OLC editor command.
 */
struct olc_cmd_type
{
    char * const        name;
    OLC_FUN *           olc_fun;
};



/*
 * Structure for an OLC editor startup command.
 */
struct  editor_cmd_type
{
    char * const        name;
    DO_FUN *            do_fun;
};



/*
 * Utils.
 */
AREA_DATA *get_vnum_area        ( int vnum );
AREA_DATA *get_area_data        ( int vnum );
int flag_value                  ( const struct flag_type *flag_table,
                                  char *argument );
char *flag_string               ( const struct flag_type *flag_table,
                                         int bits );
void add_reset                  ( ROOM_INDEX_DATA *room, 
                                         RESET_DATA *pReset, int index );



/*
 * Interpreter Table Prototypes
 */
extern const struct olc_cmd_type        aedit_table[];
extern const struct olc_cmd_type        redit_table[];
extern const struct olc_cmd_type        oedit_table[];
extern const struct olc_cmd_type        medit_table[];


/*
 * Editor Commands.
 */
void do_aedit( CHAR_DATA *ch, char *argument );
void do_redit( CHAR_DATA *ch, char *argument );
void do_oedit( CHAR_DATA *ch, char *argument );
void do_medit( CHAR_DATA *ch, char *argument );



/*
 * General Functions
 */
bool show_commands              ( CHAR_DATA *ch, char *argument );
bool show_help                  ( CHAR_DATA *ch, char *argument );
bool edit_done                  ( CHAR_DATA *ch );
bool show_version               ( CHAR_DATA *ch, char *argument );



/*
 * Area Editor Prototypes
 */
bool aedit_show( CHAR_DATA *ch, char *argument );
bool aedit_create( CHAR_DATA *ch, char *argument );
bool aedit_name( CHAR_DATA *ch, char *argument );
bool aedit_file( CHAR_DATA *ch, char *argument );
bool aedit_age( CHAR_DATA *ch, char *argument );
/* bool aedit_recall( CHAR_DATA *ch, char *argument ); */
bool aedit_reset( CHAR_DATA *ch, char *argument );
bool aedit_security( CHAR_DATA *ch, char *argument );
bool aedit_builder( CHAR_DATA *ch, char *argument );
bool aedit_vnum( CHAR_DATA *ch, char *argument );
bool aedit_lvnum( CHAR_DATA *ch, char *argument );
bool aedit_uvnum( CHAR_DATA *ch, char *argument );
bool aedit_rlist( CHAR_DATA *ch, char *argument );



/*
 * Room Editor Prototypes
 */
bool redit_show( CHAR_DATA *ch, char *argument );
bool redit_create( CHAR_DATA *ch, char *argument );
bool redit_name( CHAR_DATA *ch, char *argument );
bool redit_desc( CHAR_DATA *ch, char *argument );
bool redit_ed( CHAR_DATA *ch, char *argument );
bool redit_format( CHAR_DATA *ch, char *argument );
bool redit_north( CHAR_DATA *ch, char *argument );
bool redit_south( CHAR_DATA *ch, char *argument );
bool redit_east( CHAR_DATA *ch, char *argument );
bool redit_west( CHAR_DATA *ch, char *argument );
bool redit_up( CHAR_DATA *ch, char *argument );
bool redit_down( CHAR_DATA *ch, char *argument );
bool redit_mreset( CHAR_DATA *ch, char *argument );
bool redit_oreset( CHAR_DATA *ch, char *argument );
bool redit_mlist( CHAR_DATA *ch, char *argument );
bool redit_olist( CHAR_DATA *ch, char *argument );
bool redit_mshow( CHAR_DATA *ch, char *argument );
bool redit_oshow( CHAR_DATA *ch, char *argument );



/*
 * Object Editor Prototypes
 */
bool oedit_show( CHAR_DATA *ch, char *argument );
bool oedit_create( CHAR_DATA *ch, char *argument );
bool oedit_name( CHAR_DATA *ch, char *argument );
bool oedit_short( CHAR_DATA *ch, char *argument );
bool oedit_long( CHAR_DATA *ch, char *argument );
bool oedit_addaffect( CHAR_DATA *ch, char *argument );
bool oedit_delaffect( CHAR_DATA *ch, char *argument );
bool oedit_value0( CHAR_DATA *ch, char *argument );
bool oedit_value1( CHAR_DATA *ch, char *argument );
bool oedit_value2( CHAR_DATA *ch, char *argument );
bool oedit_value3( CHAR_DATA *ch, char *argument );
bool oedit_value4( CHAR_DATA *ch, char *argument );
bool oedit_weight( CHAR_DATA *ch, char *argument );
bool oedit_cost( CHAR_DATA *ch, char *argument );
bool oedit_ed( CHAR_DATA *ch, char *argument );

bool oedit_extra( CHAR_DATA *ch, char *argument );
bool oedit_wear( CHAR_DATA *ch, char *argument );
bool oedit_type( CHAR_DATA *ch, char *argument );
bool oedit_affect( CHAR_DATA *ch, char *argument );
bool oedit_material( CHAR_DATA *ch, char *argument );
bool oedit_level( CHAR_DATA *ch, char *argument );



/*
 * Mobile Editor Prototypes
 */
bool medit_show( CHAR_DATA *ch, char *argument );
bool medit_create( CHAR_DATA *ch, char *argument );
bool medit_name( CHAR_DATA *ch, char *argument );
bool medit_short( CHAR_DATA *ch, char *argument );
bool medit_long( CHAR_DATA *ch, char *argument );
bool medit_shop( CHAR_DATA *ch, char *argument );
bool medit_desc( CHAR_DATA *ch, char *argument );
bool medit_level( CHAR_DATA *ch, char *argument );
bool medit_align( CHAR_DATA *ch, char *argument );
bool medit_spec( CHAR_DATA *ch, char *argument );

bool medit_sex( CHAR_DATA *ch, char *argument );
bool medit_ac( CHAR_DATA *ch, char *argument );
bool medit_act( CHAR_DATA *ch, char *argument );
bool medit_affect( CHAR_DATA *ch, char *argument );
bool medit_affect2( CHAR_DATA *ch, char *argument );
bool medit_form( CHAR_DATA *ch, char *argument );
bool medit_part( CHAR_DATA *ch, char *argument );
bool medit_imm( CHAR_DATA *ch, char *argument );
bool medit_res( CHAR_DATA *ch, char *argument );
bool medit_vuln( CHAR_DATA *ch, char *argument );
bool medit_material( CHAR_DATA *ch, char *argument );
bool medit_off( CHAR_DATA *ch, char *argument );
bool medit_size( CHAR_DATA *ch, char *argument );
bool medit_hitdice( CHAR_DATA *ch, char *argument );
bool medit_manadice( CHAR_DATA *ch, char *argument );
bool medit_damdice( CHAR_DATA *ch, char *argument );
bool medit_damtype( CHAR_DATA *ch, char *argument );
bool medit_race( CHAR_DATA *ch, char *argument );
bool medit_position( CHAR_DATA *ch, char *argument );
bool medit_gold( CHAR_DATA *ch, char *argument );
bool medit_hitroll( CHAR_DATA *ch, char *argument );


/*
 * Macros
 */

#define IS_SWITCHED( ch )       ( ch->desc->original )    /* ROM OLC */

#define IS_BUILDER(ch, Area)    ( ( ch->pcdata->security >= Area->security  \
                                || strstr( Area->builders, capitalize( ch->name ) )       \
                                || strstr( Area->builders, "All" ) )        \
                                && !IS_SWITCHED( ch ) )

#define TOGGLE_BIT(var, bit)    ((var) ^= (bit))

/* Return pointers to what is being edited. */
#define EDIT_MOB(Ch, Mob)       ( Mob = (MOB_INDEX_DATA *)Ch->desc->pEdit )
#define EDIT_OBJ(Ch, Obj)       ( Obj = (OBJ_INDEX_DATA *)Ch->desc->pEdit )
#define EDIT_ROOM(Ch, Room)     ( Room = (ROOM_INDEX_DATA *)Ch->in_room )
#define EDIT_AREA(Ch, Area)     ( Area = (AREA_DATA *)Ch->desc->pEdit )






/*
 * Prototypes
 */
/* mem.c - memory prototypes. */
#define ED      EXTRA_DESCR_DATA
RESET_DATA      *new_reset_data         ( void );
void            free_reset_data         ( RESET_DATA *pReset );
AREA_DATA       *new_area               ( void );
void            free_area               ( AREA_DATA *pArea );
EXIT_DATA       *new_exit               ( void );
void            free_exit               ( EXIT_DATA *pExit );
ED              *new_extra_descr        ( void );
void            free_extra_descr        ( ED *pExtra );
ROOM_INDEX_DATA *new_room_index         ( void );
void            free_room_index         ( ROOM_INDEX_DATA *pRoom );
AFFECT_DATA     *new_affect             ( void );
void            free_affect             ( AFFECT_DATA* pAf );
SHOP_DATA       *new_shop               ( void );
void            free_shop               ( SHOP_DATA *pShop );
OBJ_INDEX_DATA  *new_obj_index          ( void );
void            free_obj_index          ( OBJ_INDEX_DATA *pObj );
MOB_INDEX_DATA  *new_mob_index          ( void );
void            free_mob_index          ( MOB_INDEX_DATA *pMob );
#undef  ED
