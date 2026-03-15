/*
 * stubs_area.c — Additional stubs needed to link test_yaml_area.
 *
 * test_yaml_area calls load_yaml_area_file(), which reaches all section
 * loaders (mobiles, objects, rooms, helps, specials).  The symbols defined
 * here satisfy the linker for every external reference those loaders make
 * that is not already provided by stubs.c or the standard library.
 *
 * These are kept separate from stubs.c to avoid polluting the simpler
 * test binaries that don't exercise the area loader.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <form.h>
#include "merc.h"
#include "db.h"

/* ================================================================== */
/*  Area globals                                                        */
/* ================================================================== */

AREA_DATA           *area_first                     = NULL;
AREA_DATA           *area_last                      = NULL;
char                 strArea[MAX_INPUT_LENGTH]       = "";

/* Hash tables for mob/obj/room lookups */
MOB_INDEX_DATA      *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA      *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA     *room_index_hash[MAX_KEY_HASH];

/* Boot flag — yaml_area.c checks this to allow duplicate vnums at boot */
bool                 fBootDb                        = FALSE;

/* top_* counters not already defined in stubs.c */
int                  top_affect                     = 0;
int                  top_help                       = 0;
int                  top_mob_index                  = 0;
int                  top_obj_index                  = 0;
int                  top_shop                       = 0;
int                  top_vnum_room                  = 0;
int                  top_vnum_mob                   = 0;
int                  top_vnum_obj                   = 0;

/* Help / shop list heads */
HELP_DATA           *help_first                     = NULL;
HELP_DATA           *help_last                      = NULL;
SHOP_DATA           *shop_first                     = NULL;
SHOP_DATA           *shop_last                      = NULL;
char                *help_greeting                  = NULL;

/* Affect free list */
AFFECT_DATA         *affect_free                    = NULL;

/* Kill table — indexed by mob level */
KILL_DATA            kill_table[MAX_LEVEL];

/* db2.c global */
int                  social_count                   = 0;

/* ================================================================== */
/*  Lookup stubs                                                        */
/* ================================================================== */

MOB_INDEX_DATA *get_mob_index( int vnum )
{
    MOB_INDEX_DATA *p;
    for ( p = mob_index_hash[vnum % MAX_KEY_HASH]; p; p = p->next )
        if ( p->vnum == vnum )
            return p;
    return NULL;
}

OBJ_INDEX_DATA *get_obj_index( int vnum )
{
    OBJ_INDEX_DATA *p;
    for ( p = obj_index_hash[vnum % MAX_KEY_HASH]; p; p = p->next )
        if ( p->vnum == vnum )
            return p;
    return NULL;
}

ROOM_INDEX_DATA *get_room_index( int vnum )
{
    ROOM_INDEX_DATA *p;
    for ( p = room_index_hash[vnum % MAX_KEY_HASH]; p; p = p->next )
        if ( p->vnum == vnum )
            return p;
    return NULL;
}

/* ================================================================== */
/*  No-op and minimal stubs                                             */
/* ================================================================== */

void assign_area_vnum( int vnum )           { (void)vnum; }

void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset )
{
    (void)pR; (void)pReset;
}

SPEC_FUN *spec_lookup( const char *name )   { (void)name; return NULL; }
char     *spec_string( SPEC_FUN *fun )      { (void)fun;  return ""; }
long      material_lookup( const char *name ) { (void)name; return 0; }

/*
 * race_lookup — returns 0 (the "unique" race index) for any name.
 * The stubs.c race_table has "unique" at index 0 with all-zero flags,
 * so OR-ing mob flags with race_table[0].act etc. is a no-op.
 */
int race_lookup( const char *name )         { (void)name; return 0; }
