///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2022 by Daniel Anderson
////  YAML area file support added 2026
///////////////////////////////////////////////////////////////////////

/*
 * YAML-based area file loading and saving using libyaml.
 *
 * Area files can be in either the legacy .are format or the new .yaml
 * format.  When both exist, .yaml takes precedence.
 *
 * Loading mirrors the two-pass boot sequence in db.c:
 *   Pass 1 (load_yaml_area_file):       area header, mobiles, objects,
 *                                        rooms, helps, specials, clans
 *   Pass 2 (load_yaml_area_resets_shops): resets, shops
 *
 * Saving (save_yaml_area) writes a .yaml file derived from the area's
 * existing filename (replacing ".are" with ".yaml").
 *
 * YAML area file top-level keys:
 *   area, mobiles, objects, rooms, resets, shops, specials, helps
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <yaml.h>

#include "merc.h"
#include "db.h"
#include "yaml_area.h"

/* ------------------------------------------------------------------ */
/* Externs from db.c                                                   */
/* ------------------------------------------------------------------ */

extern AREA_DATA        *area_first;
extern AREA_DATA        *area_last;
extern ROOM_INDEX_DATA  *room_index_hash[MAX_KEY_HASH];
extern char              strArea[MAX_INPUT_LENGTH];

extern int  top_affect;
extern int  top_area;
extern int  top_ed;
extern int  top_exit;
extern int  top_help;
extern int  top_mob_index;
extern int  top_obj_index;
extern int  top_reset;
extern int  top_room;
extern int  top_shop;
extern int  top_vnum_room;
extern int  top_vnum_mob;
extern int  top_vnum_obj;

extern HELP_DATA  *help_first;
extern HELP_DATA  *help_last;
extern SHOP_DATA  *shop_first;
extern SHOP_DATA  *shop_last;
extern char       *help_greeting;

extern AFFECT_DATA *affect_free;

/* Functions from db.c needed here */
extern void assign_area_vnum( int vnum );
extern void new_reset( ROOM_INDEX_DATA *pR, RESET_DATA *pReset );

/* ------------------------------------------------------------------ */
/* Internal: libyaml document-tree navigation helpers                  */
/* ------------------------------------------------------------------ */

static const char *
doc_scalar( yaml_document_t *doc, int node_id )
{
    yaml_node_t *node;
    if ( node_id <= 0 )
        return "";
    node = yaml_document_get_node( doc, node_id );
    if ( node && node->type == YAML_SCALAR_NODE )
        return (const char *)node->data.scalar.value;
    return "";
}

/*
 * Find the value node-id for a key in a YAML mapping.
 * Returns 0 if not found or if map_id is not a mapping.
 */
static int
doc_find( yaml_document_t *doc, int map_id, const char *key )
{
    yaml_node_t      *node;
    yaml_node_pair_t *pair;

    if ( map_id <= 0 )
        return 0;
    node = yaml_document_get_node( doc, map_id );
    if ( !node || node->type != YAML_MAPPING_NODE )
        return 0;
    for ( pair = node->data.mapping.pairs.start;
          pair < node->data.mapping.pairs.top;
          pair++ )
    {
        if ( strcmp( doc_scalar( doc, pair->key ), key ) == 0 )
            return pair->value;
    }
    return 0;
}

static int
doc_int( yaml_document_t *doc, int map_id, const char *key, int def )
{
    int         valnode = doc_find( doc, map_id, key );
    const char *s;
    if ( !valnode )
        return def;
    s = doc_scalar( doc, valnode );
    if ( !s || s[0] == '\0' )
        return def;
    return atoi( s );
}

static long
doc_long( yaml_document_t *doc, int map_id, const char *key, long def )
{
    int         valnode = doc_find( doc, map_id, key );
    const char *s;
    if ( !valnode )
        return def;
    s = doc_scalar( doc, valnode );
    if ( !s || s[0] == '\0' )
        return def;
    return atol( s );
}

static long
doc_flag( yaml_document_t *doc, int map_id, const char *key, long def )
{
    int         valnode = doc_find( doc, map_id, key );
    const char *s;
    long        total = 0;
    if ( !valnode )
        return def;
    s = doc_scalar( doc, valnode );
    if ( !s || s[0] == '\0' )
        return 0;
    /* plain decimal integer (including '0') */
    if ( s[0] == '-' || isdigit( (unsigned char)s[0] ) )
        return atol( s );
    /* flag letter string: each character is one bit via flag_convert() */
    while ( *s )
        total += flag_convert( *s++ );
    return total;
}

static const char *
doc_str( yaml_document_t *doc, int map_id, const char *key )
{
    int valnode = doc_find( doc, map_id, key );
    if ( !valnode )
        return "";
    return doc_scalar( doc, valnode );
}

static int
doc_seq_len( yaml_document_t *doc, int seq_id )
{
    yaml_node_t *node;
    if ( seq_id <= 0 )
        return 0;
    node = yaml_document_get_node( doc, seq_id );
    if ( !node || node->type != YAML_SEQUENCE_NODE )
        return 0;
    return (int)( node->data.sequence.items.top
                - node->data.sequence.items.start );
}

static int
doc_seq_item( yaml_document_t *doc, int seq_id, int n )
{
    yaml_node_t      *node;
    yaml_node_item_t *item;
    if ( seq_id <= 0 )
        return 0;
    node = yaml_document_get_node( doc, seq_id );
    if ( !node || node->type != YAML_SEQUENCE_NODE )
        return 0;
    item = node->data.sequence.items.start + n;
    if ( item >= node->data.sequence.items.top )
        return 0;
    return (int)*item;
}

/* ------------------------------------------------------------------ */
/* Internal: section loaders (pass 1)                                  */
/* ------------------------------------------------------------------ */

static void
load_yaml_area_section( yaml_document_t *doc, int node_id )
{
    AREA_DATA *pArea;

    pArea               = alloc_perm( sizeof(*pArea) );
    pArea->age          = 15;
    pArea->nplayer      = 0;
    pArea->filename     = str_dup( strArea );
    pArea->vnum         = top_area;
    pArea->name         = str_dup( doc_str( doc, node_id, "name" ) );
    pArea->builders     = str_dup( doc_str( doc, node_id, "builders" ) );
    pArea->security     = doc_int( doc, node_id, "security", 9 );
    pArea->area_flags   = doc_int( doc, node_id, "area_flags", 0 );

    /* vnums: [low, high] sequence */
    {
        int vnums_id = doc_find( doc, node_id, "vnums" );
        pArea->lvnum = 0;
        pArea->uvnum = 0;
        if ( vnums_id )
        {
            int n0 = doc_seq_item( doc, vnums_id, 0 );
            int n1 = doc_seq_item( doc, vnums_id, 1 );
            if ( n0 ) pArea->lvnum = atoi( doc_scalar( doc, n0 ) );
            if ( n1 ) pArea->uvnum = atoi( doc_scalar( doc, n1 ) );
        }
    }

    if ( pArea->name[0] == '\0' )
    {
        free_string( pArea->name );
        pArea->name = str_dup( "New Area" );
    }

    if ( area_first == NULL )
        area_first = pArea;
    if ( area_last  != NULL )
        area_last->next = pArea;
    area_last   = pArea;
    pArea->next = NULL;
    top_area++;
}

static void
load_yaml_mobiles_section( yaml_document_t *doc, int seq_id )
{
    int i, n;
    n = doc_seq_len( doc, seq_id );

    if ( area_last == NULL )
    {
        bug( "load_yaml_mobiles_section: no area loaded yet.", 0 );
        return;
    }

    for ( i = 0; i < n; i++ )
    {
        MOB_INDEX_DATA *pMobIndex;
        int             mob_id = doc_seq_item( doc, seq_id, i );
        int             vnum;
        int             iHash;
        const char     *size_str;
        const char     *spec_str;
        int             ac_id;

        if ( !mob_id )
            continue;

        vnum = doc_int( doc, mob_id, "vnum", 0 );
        if ( vnum == 0 )
            continue;

        fBootDb = FALSE;
        if ( get_mob_index( vnum ) != NULL )
        {
            bug( "load_yaml_mobiles: vnum %d duplicated.", vnum );
            continue;
        }
        fBootDb = TRUE;

        pMobIndex               = alloc_perm( sizeof(*pMobIndex) );
        pMobIndex->vnum         = (sh_int)vnum;
        pMobIndex->area         = area_last;
        pMobIndex->count        = 0;
        pMobIndex->killed       = 0;
        pMobIndex->pShop        = NULL;
        pMobIndex->spec_fun     = NULL;

        pMobIndex->player_name  = str_dup( doc_str( doc, mob_id, "keywords" ) );
        pMobIndex->short_descr  = str_dup( doc_str( doc, mob_id, "short_desc" ) );
        pMobIndex->long_descr   = str_dup( doc_str( doc, mob_id, "long_desc" ) );
        pMobIndex->description  = str_dup( doc_str( doc, mob_id, "description" ) );

        if ( pMobIndex->long_descr[0] != '\0' )
            pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
        if ( pMobIndex->description[0] != '\0' )
            pMobIndex->description[0] = UPPER( pMobIndex->description[0] );

        pMobIndex->race         = race_lookup( doc_str( doc, mob_id, "race" ) );
        if ( pMobIndex->race < 0 )
            pMobIndex->race = 0;

        pMobIndex->act          = doc_flag( doc, mob_id, "act", 0 )
                                | ACT_IS_NPC
                                | race_table[pMobIndex->race].act;
        pMobIndex->affected_by  = doc_flag( doc, mob_id, "affected_by", 0 )
                                | race_table[pMobIndex->race].aff;
        pMobIndex->affected2_by = doc_flag( doc, mob_id, "affected2_by", 0 )
                                | race_table[pMobIndex->race].aff2;
        pMobIndex->alignment    = (sh_int)doc_int( doc, mob_id, "alignment", 0 );

        /* clan: stored as integer; 0 == no clan */
        pMobIndex->clan         = (sh_int)doc_int( doc, mob_id, "clan", 0 );

        pMobIndex->level        = (sh_int)doc_int( doc, mob_id, "level", 1 );
        pMobIndex->hitroll      = (sh_int)doc_int( doc, mob_id, "hitroll", 0 );

        /* hit/mana/damage dice: mappings {number, type, bonus} */
        {
            int dice_id;
            dice_id = doc_find( doc, mob_id, "hit" );
            pMobIndex->hit[DICE_NUMBER] = (sh_int)doc_int( doc, dice_id, "number", 0 );
            pMobIndex->hit[DICE_TYPE]   = (sh_int)doc_int( doc, dice_id, "type",   0 );
            pMobIndex->hit[DICE_BONUS]  = (sh_int)doc_int( doc, dice_id, "bonus",  0 );

            dice_id = doc_find( doc, mob_id, "mana" );
            pMobIndex->mana[DICE_NUMBER] = (sh_int)doc_int( doc, dice_id, "number", 0 );
            pMobIndex->mana[DICE_TYPE]   = (sh_int)doc_int( doc, dice_id, "type",   0 );
            pMobIndex->mana[DICE_BONUS]  = (sh_int)doc_int( doc, dice_id, "bonus",  0 );

            dice_id = doc_find( doc, mob_id, "damage" );
            pMobIndex->damage[DICE_NUMBER] = (sh_int)doc_int( doc, dice_id, "number", 0 );
            pMobIndex->damage[DICE_TYPE]   = (sh_int)doc_int( doc, dice_id, "type",   0 );
            pMobIndex->damage[DICE_BONUS]  = (sh_int)doc_int( doc, dice_id, "bonus",  0 );
        }

        pMobIndex->dam_type     = (sh_int)doc_int( doc, mob_id, "dam_type", 0 );

        /* armor class: map {pierce, bash, slash, exotic} stored /10 */
        ac_id = doc_find( doc, mob_id, "ac" );
        pMobIndex->ac[AC_PIERCE] = (sh_int)(doc_int( doc, ac_id, "pierce", 0 ) * 10);
        pMobIndex->ac[AC_BASH]   = (sh_int)(doc_int( doc, ac_id, "bash",   0 ) * 10);
        pMobIndex->ac[AC_SLASH]  = (sh_int)(doc_int( doc, ac_id, "slash",  0 ) * 10);
        pMobIndex->ac[AC_EXOTIC] = (sh_int)(doc_int( doc, ac_id, "exotic", 0 ) * 10);

        pMobIndex->off_flags    = doc_flag( doc, mob_id, "off_flags", 0 )
                                | race_table[pMobIndex->race].off;
        pMobIndex->imm_flags    = doc_flag( doc, mob_id, "imm_flags", 0 )
                                | race_table[pMobIndex->race].imm;
        pMobIndex->res_flags    = doc_flag( doc, mob_id, "res_flags", 0 )
                                | race_table[pMobIndex->race].res;
        pMobIndex->vuln_flags   = doc_flag( doc, mob_id, "vuln_flags", 0 )
                                | race_table[pMobIndex->race].vuln;

        pMobIndex->start_pos    = (sh_int)doc_int( doc, mob_id, "start_pos",   POS_STANDING );
        pMobIndex->default_pos  = (sh_int)doc_int( doc, mob_id, "default_pos", POS_STANDING );
        pMobIndex->sex          = (sh_int)doc_int( doc, mob_id, "sex", 0 );
        pMobIndex->gold         = doc_long( doc, mob_id, "gold", 0L );

        pMobIndex->form         = doc_flag( doc, mob_id, "form", 0L )
                                | race_table[pMobIndex->race].form;
        pMobIndex->parts        = doc_flag( doc, mob_id, "parts", 0L )
                                | race_table[pMobIndex->race].parts;

        /* size: single letter T/S/M/L/H/G */
        size_str = doc_str( doc, mob_id, "size" );
        switch ( ( size_str && size_str[0] ) ? toupper(size_str[0]) : 'M' )
        {
            case 'T': pMobIndex->size = SIZE_TINY;   break;
            case 'S': pMobIndex->size = SIZE_SMALL;  break;
            case 'L': pMobIndex->size = SIZE_LARGE;  break;
            case 'H': pMobIndex->size = SIZE_HUGE;   break;
            case 'G': pMobIndex->size = SIZE_GIANT;  break;
            default:  pMobIndex->size = SIZE_MEDIUM; break;
        }
        pMobIndex->material = material_lookup( doc_str( doc, mob_id, "material" ) );

        /* optional spec function */
        spec_str = doc_str( doc, mob_id, "spec_fun" );
        if ( spec_str && spec_str[0] != '\0' )
        {
            pMobIndex->spec_fun = spec_lookup( spec_str );
            if ( pMobIndex->spec_fun == 0 )
            {
                char _bugbuf[256];
                snprintf( _bugbuf, sizeof(_bugbuf),
                    "load_yaml_mobiles: unknown spec '%s' on vnum %d.",
                    spec_str, vnum );
                bug( _bugbuf, 0 );
            }
        }

        iHash                   = vnum % MAX_KEY_HASH;
        pMobIndex->next         = mob_index_hash[iHash];
        mob_index_hash[iHash]   = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;
        assign_area_vnum( vnum );
        kill_table[URANGE( 0, pMobIndex->level, MAX_LEVEL - 1 )].number++;
    }
}

static void
load_yaml_objects_section( yaml_document_t *doc, int seq_id )
{
    int i, n;
    n = doc_seq_len( doc, seq_id );

    if ( area_last == NULL )
    {
        bug( "load_yaml_objects_section: no area loaded yet.", 0 );
        return;
    }

    for ( i = 0; i < n; i++ )
    {
        OBJ_INDEX_DATA *pObjIndex;
        int             obj_id = doc_seq_item( doc, seq_id, i );
        int             vnum;
        int             iHash;
        int             vals_id;
        int             aff_seq_id;
        int             ed_seq_id;
        int             j;

        if ( !obj_id )
            continue;

        vnum = doc_int( doc, obj_id, "vnum", 0 );
        if ( vnum == 0 )
            continue;

        fBootDb = FALSE;
        if ( get_obj_index( vnum ) != NULL )
        {
            bug( "load_yaml_objects: vnum %d duplicated.", vnum );
            continue;
        }
        fBootDb = TRUE;

        pObjIndex               = alloc_perm( sizeof(*pObjIndex) );
        pObjIndex->vnum         = (sh_int)vnum;
        pObjIndex->area         = area_last;
        pObjIndex->reset_num    = 0;
        pObjIndex->extra_descr  = NULL;
        pObjIndex->affected     = NULL;

        pObjIndex->name         = str_dup( doc_str( doc, obj_id, "name" ) );
        pObjIndex->short_descr  = str_dup( doc_str( doc, obj_id, "short_desc" ) );
        pObjIndex->description  = str_dup( doc_str( doc, obj_id, "description" ) );
        pObjIndex->material     = material_lookup( doc_str( doc, obj_id, "material" ) );

        pObjIndex->item_type    = (sh_int)doc_int(  doc, obj_id, "item_type",  0 );
        pObjIndex->extra_flags  = doc_flag( doc, obj_id, "extra_flags", 0 );
        pObjIndex->wear_flags   = (sh_int)doc_flag( doc, obj_id, "wear_flags", 0 );
        pObjIndex->level        = (sh_int)doc_int( doc, obj_id, "level",      0 );
        pObjIndex->weight       = (sh_int)doc_int( doc, obj_id, "weight",     0 );
        pObjIndex->cost         = doc_int( doc, obj_id, "cost",       0 );

        /* values: sequence of 5 integers */
        vals_id = doc_find( doc, obj_id, "values" );
        for ( j = 0; j < 5; j++ )
        {
            int item = doc_seq_item( doc, vals_id, j );
            pObjIndex->value[j] = item ? atoi( doc_scalar( doc, item ) ) : 0;
        }

        /* affects */
        aff_seq_id = doc_find( doc, obj_id, "affects" );
        for ( j = 0; j < doc_seq_len( doc, aff_seq_id ); j++ )
        {
            AFFECT_DATA *paf;
            int          aff_id = doc_seq_item( doc, aff_seq_id, j );
            const char  *where_str;

            if ( !aff_id )
                continue;
            paf             = alloc_perm( sizeof(*paf) );
            paf->type       = -1;
            paf->level      = pObjIndex->level;
            paf->duration   = -1;
            paf->location   = (sh_int)doc_int( doc, aff_id, "location", 0 );
            paf->modifier   = (sh_int)doc_int( doc, aff_id, "modifier", 0 );
            paf->bitvector  = doc_flag( doc, aff_id, "bitvector", 0 );

            where_str = doc_str( doc, aff_id, "where" );
            if      ( !str_cmp( where_str, "A" ) || !str_cmp( where_str, "affects" ) )
                paf->where = TO_AFFECTS;
            else if ( !str_cmp( where_str, "I" ) || !str_cmp( where_str, "immune" ) )
                paf->where = TO_IMMUNE;
            else if ( !str_cmp( where_str, "R" ) || !str_cmp( where_str, "resist" ) )
                paf->where = TO_RESIST;
            else if ( !str_cmp( where_str, "V" ) || !str_cmp( where_str, "vuln" ) )
                paf->where = TO_VULN;
            else if ( !str_cmp( where_str, "Z" ) || !str_cmp( where_str, "affects2" ) )
                paf->where = TO_AFFECTS2;
            else
                paf->where = TO_OBJECT;

            paf->next           = pObjIndex->affected;
            pObjIndex->affected = paf;
            top_affect++;
        }

        /* extra descriptions */
        ed_seq_id = doc_find( doc, obj_id, "extra_descs" );
        for ( j = 0; j < doc_seq_len( doc, ed_seq_id ); j++ )
        {
            EXTRA_DESCR_DATA *ed;
            int               ed_id = doc_seq_item( doc, ed_seq_id, j );
            if ( !ed_id )
                continue;
            ed              = alloc_perm( sizeof(*ed) );
            ed->keyword     = str_dup( doc_str( doc, ed_id, "keywords" ) );
            ed->description = str_dup( doc_str( doc, ed_id, "description" ) );
            ed->next        = pObjIndex->extra_descr;
            pObjIndex->extra_descr = ed;
            top_ed++;
        }

        /* translate spell slots to skill numbers for pill/potion/scroll/wand/staff */
        switch ( pObjIndex->item_type )
        {
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
            pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
            pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
            break;
        }

        iHash                   = vnum % MAX_KEY_HASH;
        pObjIndex->next         = obj_index_hash[iHash];
        obj_index_hash[iHash]   = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;
        assign_area_vnum( vnum );
    }
}

static void
load_yaml_rooms_section( yaml_document_t *doc, int seq_id )
{
    int i, n;
    n = doc_seq_len( doc, seq_id );

    if ( area_last == NULL )
    {
        bug( "load_yaml_rooms_section: no area loaded yet.", 0 );
        return;
    }

    for ( i = 0; i < n; i++ )
    {
        ROOM_INDEX_DATA *pRoomIndex;
        int              room_id = doc_seq_item( doc, seq_id, i );
        int              vnum;
        int              iHash;
        int              exit_seq_id;
        int              ed_seq_id;
        int              j;

        if ( !room_id )
            continue;

        vnum = doc_int( doc, room_id, "vnum", 0 );
        if ( vnum == 0 )
            continue;

        fBootDb = FALSE;
        if ( get_room_index( vnum ) != NULL )
        {
            bug( "load_yaml_rooms: vnum %d duplicated.", vnum );
            continue;
        }
        fBootDb = TRUE;

        pRoomIndex              = alloc_perm( sizeof(*pRoomIndex) );
        pRoomIndex->people      = NULL;
        pRoomIndex->contents    = NULL;
        pRoomIndex->extra_descr = NULL;
        pRoomIndex->area        = area_last;
        pRoomIndex->vnum        = (sh_int)vnum;
        pRoomIndex->light       = 0;
        pRoomIndex->status      = 0;
        pRoomIndex->timer       = 0;
        for ( j = 0; j <= 5; j++ )
            pRoomIndex->exit[j] = NULL;

        pRoomIndex->name        = str_dup( doc_str( doc, room_id, "name" ) );
        pRoomIndex->description = str_dup( doc_str( doc, room_id, "description" ) );
        pRoomIndex->room_flags  = doc_flag( doc, room_id, "room_flags", 0 );
        pRoomIndex->sector_type = (sh_int)doc_int( doc, room_id, "sector", SECT_INSIDE );
        pRoomIndex->owner       = str_dup( doc_str( doc, room_id, "owner" ) );

        /* horrible ROM hack: midgaard law rooms */
        if ( 3000 <= vnum && vnum < 3400 )
            SET_BIT( pRoomIndex->room_flags, ROOM_LAW );

        /* exits */
        exit_seq_id = doc_find( doc, room_id, "exits" );
        for ( j = 0; j < doc_seq_len( doc, exit_seq_id ); j++ )
        {
            EXIT_DATA *pexit;
            int        ex_id = doc_seq_item( doc, exit_seq_id, j );
            int        door;
            int        flags;

            if ( !ex_id )
                continue;

            door = doc_int( doc, ex_id, "dir", -1 );
            if ( door < 0 || door > 5 )
                continue;

            pexit               = alloc_perm( sizeof(*pexit) );
            pexit->description  = str_dup( doc_str( doc, ex_id, "desc" ) );
            pexit->keyword      = str_dup( doc_str( doc, ex_id, "keywords" ) );
            pexit->exit_info    = 0;
            pexit->rs_flags     = (int)doc_flag( doc, ex_id, "exit_flags", 0 );
            pexit->key          = (sh_int)doc_int( doc, ex_id, "key", -1 );
            pexit->u1.vnum      = (sh_int)doc_int( doc, ex_id, "to_vnum", -1 );
            pexit->orig_door    = door;

            /* legacy lock-level compat (optional) */
            flags = doc_int( doc, ex_id, "lock", 0 );
            if ( flags != 0 && pexit->rs_flags == 0 )
            {
                switch ( flags )
                {
                    case 1: pexit->rs_flags = EX_ISDOOR;                                                break;
                    case 2: pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF;                                break;
                    case 3: pexit->rs_flags = EX_ISDOOR | EX_PASSPROOF | EX_HIDDEN;                    break;
                    case 4: pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF | EX_PASSPROOF | EX_HIDDEN;     break;
                    case 5: pexit->rs_flags = EX_ISDOOR | EX_PASSPROOF;                                break;
                    case 6: pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF | EX_PASSPROOF;                 break;
                    case 7: pexit->rs_flags = EX_ISDOOR | EX_HIDDEN;                                   break;
                    case 8: pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF | EX_HIDDEN;                    break;
                }
            }

            pRoomIndex->exit[door] = pexit;
            top_exit++;
        }

        /* extra descriptions */
        ed_seq_id = doc_find( doc, room_id, "extra_descs" );
        for ( j = 0; j < doc_seq_len( doc, ed_seq_id ); j++ )
        {
            EXTRA_DESCR_DATA *ed;
            int               ed_id = doc_seq_item( doc, ed_seq_id, j );
            if ( !ed_id )
                continue;
            ed              = alloc_perm( sizeof(*ed) );
            ed->keyword     = str_dup( doc_str( doc, ed_id, "keywords" ) );
            ed->description = str_dup( doc_str( doc, ed_id, "description" ) );
            ed->next        = pRoomIndex->extra_descr;
            pRoomIndex->extra_descr = ed;
            top_ed++;
        }

        iHash                   = vnum % MAX_KEY_HASH;
        pRoomIndex->next        = room_index_hash[iHash];
        room_index_hash[iHash]  = pRoomIndex;
        top_room++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;
        assign_area_vnum( vnum );
    }
}

static void
load_yaml_helps_section( yaml_document_t *doc, int seq_id )
{
    int i, n;
    n = doc_seq_len( doc, seq_id );

    for ( i = 0; i < n; i++ )
    {
        HELP_DATA *pHelp;
        int        help_id = doc_seq_item( doc, seq_id, i );

        if ( !help_id )
            continue;

        pHelp          = alloc_perm( sizeof(*pHelp) );
        pHelp->level   = (sh_int)doc_int( doc, help_id, "level", 0 );
        pHelp->keyword = str_dup( doc_str( doc, help_id, "keyword" ) );
        pHelp->text    = str_dup( doc_str( doc, help_id, "text" ) );

        if ( !str_cmp( pHelp->keyword, "greeting" ) )
            help_greeting = pHelp->text;

        if ( help_first == NULL )
            help_first = pHelp;
        if ( help_last  != NULL )
            help_last->next = pHelp;
        help_last   = pHelp;
        pHelp->next = NULL;
        top_help++;
    }
}

static void
load_yaml_specials_section( yaml_document_t *doc, int seq_id )
{
    int i, n;
    n = doc_seq_len( doc, seq_id );

    for ( i = 0; i < n; i++ )
    {
        MOB_INDEX_DATA *pMobIndex;
        int             spec_id = doc_seq_item( doc, seq_id, i );
        int             vnum;
        const char     *spec_str;

        if ( !spec_id )
            continue;

        vnum     = doc_int( doc, spec_id, "vnum", 0 );
        spec_str = doc_str( doc, spec_id, "spec_fun" );

        if ( vnum == 0 || !spec_str || spec_str[0] == '\0' )
            continue;

        pMobIndex = get_mob_index( vnum );
        if ( !pMobIndex )
        {
            bug( "load_yaml_specials: vnum %d not found.", vnum );
            continue;
        }
        pMobIndex->spec_fun = spec_lookup( spec_str );
        if ( pMobIndex->spec_fun == NULL )
        {
            char _bugbuf[256];
            snprintf( _bugbuf, sizeof(_bugbuf),
                "load_yaml_specials: unknown spec '%s' on vnum %d.",
                spec_str, vnum );
            bug( _bugbuf, 0 );
        }
    }
}

/* ------------------------------------------------------------------ */
/* Internal: section loaders (pass 2 – resets & shops)                */
/* ------------------------------------------------------------------ */

static void
load_yaml_resets_section( yaml_document_t *doc, int seq_id )
{
    int i, n;
    int iLastRoom = 0;
    int iLastObj  = 0;

    n = doc_seq_len( doc, seq_id );

    if ( area_last == NULL )
    {
        bug( "load_yaml_resets_section: no area loaded yet.", 0 );
        return;
    }

    for ( i = 0; i < n; i++ )
    {
        RESET_DATA      *pReset;
        ROOM_INDEX_DATA *pRoomIndex;
        EXIT_DATA       *pexit;
        int              reset_id = doc_seq_item( doc, seq_id, i );
        const char      *cmd_str;
        char             letter;

        if ( !reset_id )
            continue;

        cmd_str = doc_str( doc, reset_id, "command" );
        if ( !cmd_str || cmd_str[0] == '\0' )
            continue;
        letter = toupper( cmd_str[0] );

        pReset          = alloc_perm( sizeof(*pReset) );
        pReset->command = letter;
        pReset->arg1    = (sh_int)doc_int( doc, reset_id, "arg1", 0 );
        pReset->arg2    = (sh_int)doc_int( doc, reset_id, "arg2", 0 );
        pReset->arg3    = ( letter == 'G' || letter == 'R' )
                          ? 0
                          : (sh_int)doc_int( doc, reset_id, "arg3", 0 );

        switch ( letter )
        {
        default:
            bug( "load_yaml_resets: bad command '%c'.", letter );
            free( pReset );
            continue;

        case 'M':
            get_mob_index( pReset->arg1 );
            pRoomIndex = get_room_index( pReset->arg3 );
            if ( pRoomIndex )
            {
                new_reset( pRoomIndex, pReset );
                iLastRoom = pReset->arg3;
            }
            break;

        case 'O':
            get_obj_index( pReset->arg1 );
            pRoomIndex = get_room_index( pReset->arg3 );
            if ( pRoomIndex )
            {
                new_reset( pRoomIndex, pReset );
                iLastObj = pReset->arg3;
            }
            break;

        case 'P':
            get_obj_index( pReset->arg1 );
            pRoomIndex = get_room_index( iLastObj );
            if ( pRoomIndex )
                new_reset( pRoomIndex, pReset );
            break;

        case 'G':
        case 'E':
            get_obj_index( pReset->arg1 );
            pRoomIndex = get_room_index( iLastRoom );
            if ( pRoomIndex )
            {
                new_reset( pRoomIndex, pReset );
                iLastObj = iLastRoom;
            }
            break;

        case 'D':
            pRoomIndex = get_room_index( pReset->arg1 );
            if (   pReset->arg2 < 0
                || pReset->arg2 > 5
                || !pRoomIndex
                || !( pexit = pRoomIndex->exit[pReset->arg2] )
                || !IS_SET( pexit->rs_flags, EX_ISDOOR ) )
            {
                bug( "load_yaml_resets: 'D': exit %d not door.", pReset->arg2 );
                free( pReset );
                continue;
            }
            switch ( pReset->arg3 )
            {
                case 0: break;
                case 1: SET_BIT( pexit->rs_flags, EX_CLOSED ); break;
                case 2:
                    SET_BIT( pexit->rs_flags, EX_CLOSED );
                    SET_BIT( pexit->rs_flags, EX_LOCKED );
                    break;
                default:
                    bug( "load_yaml_resets: 'D' bad arg3 %d.", pReset->arg3 );
                    break;
            }
            new_reset( pRoomIndex, pReset );
            break;

        case 'R':
            pRoomIndex = get_room_index( pReset->arg1 );
            if ( pReset->arg2 < 0 || pReset->arg2 > 6 )
            {
                bug( "load_yaml_resets: 'R': bad arg2 %d.", pReset->arg2 );
                free( pReset );
                continue;
            }
            if ( pRoomIndex )
                new_reset( pRoomIndex, pReset );
            break;
        }
    }
}

static void
load_yaml_shops_section( yaml_document_t *doc, int seq_id )
{
    int i, n;
    n = doc_seq_len( doc, seq_id );

    for ( i = 0; i < n; i++ )
    {
        SHOP_DATA      *pShop;
        MOB_INDEX_DATA *pMobIndex;
        int             shop_id = doc_seq_item( doc, seq_id, i );
        int             keeper;
        int             buy_id;
        int             j;

        if ( !shop_id )
            continue;

        keeper = doc_int( doc, shop_id, "keeper", 0 );
        if ( keeper == 0 )
            continue;

        pMobIndex = get_mob_index( keeper );
        if ( !pMobIndex )
        {
            bug( "load_yaml_shops: keeper vnum %d not found.", keeper );
            continue;
        }

        pShop = alloc_perm( sizeof(*pShop) );
        pShop->keeper = (sh_int)keeper;

        buy_id = doc_find( doc, shop_id, "buy_types" );
        for ( j = 0; j < MAX_TRADE; j++ )
        {
            int item = doc_seq_item( doc, buy_id, j );
            pShop->buy_type[j] = item ? (sh_int)atoi( doc_scalar( doc, item ) ) : 0;
        }

        pShop->profit_buy   = (sh_int)doc_int( doc, shop_id, "profit_buy",  100 );
        pShop->profit_sell  = (sh_int)doc_int( doc, shop_id, "profit_sell", 100 );
        pShop->open_hour    = (sh_int)doc_int( doc, shop_id, "open_hour",   0 );
        pShop->close_hour   = (sh_int)doc_int( doc, shop_id, "close_hour",  23 );

        pMobIndex->pShop = pShop;

        if ( shop_first == NULL )
            shop_first = pShop;
        if ( shop_last  != NULL )
            shop_last->next = pShop;
        shop_last   = pShop;
        pShop->next = NULL;
        top_shop++;
    }
}

/* ------------------------------------------------------------------ */
/* Internal: open a YAML file and parse it into a document             */
/* ------------------------------------------------------------------ */

static bool
parse_yaml_file( const char *filename, yaml_parser_t *parser,
                 yaml_document_t *doc )
{
    FILE *fp;

    memset( parser, 0, sizeof(*parser) );
    memset( doc,    0, sizeof(*doc)    );

    fp = fopen( filename, "r" );
    if ( !fp )
    {
        { char _msg[512]; snprintf(_msg, sizeof(_msg), "parse_yaml_file: cannot open '%s'.", filename); bug(_msg, 0); }
        return FALSE;
    }

    if ( !yaml_parser_initialize( parser ) )
    {
        bug( "parse_yaml_file: yaml_parser_initialize failed.", 0 );
        fclose( fp );
        return FALSE;
    }

    yaml_parser_set_input_file( parser, fp );

    if ( !yaml_parser_load( parser, doc ) )
    {
        { char _msg[512]; snprintf(_msg, sizeof(_msg), "parse_yaml_file: YAML parse error in '%s'.", filename); bug(_msg, 0); }
        yaml_parser_delete( parser );
        fclose( fp );
        return FALSE;
    }

    fclose( fp );
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

bool
yaml_area_file_exists( const char *are_filename, char *yaml_filename )
{
    char        buf[MAX_INPUT_LENGTH];
    struct stat st;
    char       *dot;
    size_t      len;

    /* Build .yaml path: replace last ".are" with ".yaml", or append ".yaml" */
    strncpy( buf, are_filename, sizeof(buf) - 6 );
    buf[sizeof(buf) - 6] = '\0';

    dot = strrchr( buf, '.' );
    if ( dot && strcmp( dot, ".are" ) == 0 )
    {
        *dot = '\0';
        strcat( buf, ".yaml" );
    }
    else
    {
        /* no .are extension – just append .yaml */
        len = strlen( buf );
        if ( len + 5 < sizeof(buf) )
            strcat( buf, ".yaml" );
        else
            return FALSE;
    }

    if ( stat( buf, &st ) == 0 && S_ISREG( st.st_mode ) )
    {
        strncpy( yaml_filename, buf, MAX_INPUT_LENGTH - 1 );
        yaml_filename[MAX_INPUT_LENGTH - 1] = '\0';
        return TRUE;
    }
    return FALSE;
}

/*
 * Pass 1: loads area header, mobiles, objects, rooms, helps, specials.
 * (Resets and shops are deferred to pass 2 because they require all
 *  rooms and mobs to be loaded first.)
 */
bool
load_yaml_area_file( const char *filename )
{
    yaml_parser_t   parser;
    yaml_document_t doc;
    yaml_node_t    *root;
    int             area_id;
    int             mobs_id;
    int             objs_id;
    int             rooms_id;
    int             helps_id;
    int             specials_id;

    /* Store filename in the global strArea so that new_load_area
     * equivalents can use it for area->filename */
    strncpy( strArea, filename, sizeof(strArea) - 1 );
    strArea[sizeof(strArea) - 1] = '\0';

    if ( !parse_yaml_file( filename, &parser, &doc ) )
        return FALSE;

    root = yaml_document_get_root_node( &doc );
    if ( !root || root->type != YAML_MAPPING_NODE )
    {
        { char _msg[512]; snprintf(_msg, sizeof(_msg), "load_yaml_area_file: root is not a mapping in '%s'.", filename); bug(_msg, 0); }
        yaml_document_delete( &doc );
        yaml_parser_delete( &parser );
        return FALSE;
    }

    /* Navigate top-level keys */
    area_id     = doc_find( &doc, 1, "area"     );
    mobs_id     = doc_find( &doc, 1, "mobiles"  );
    objs_id     = doc_find( &doc, 1, "objects"  );
    rooms_id    = doc_find( &doc, 1, "rooms"    );
    helps_id    = doc_find( &doc, 1, "helps"    );
    specials_id = doc_find( &doc, 1, "specials" );

    if ( area_id )
        load_yaml_area_section( &doc, area_id );
    else
        { char _msg[512]; snprintf(_msg, sizeof(_msg), "load_yaml_area_file: no 'area:' section in '%s'.", filename); bug(_msg, 0); }

    if ( mobs_id )
        load_yaml_mobiles_section( &doc, mobs_id );

    if ( objs_id )
        load_yaml_objects_section( &doc, objs_id );

    if ( rooms_id )
        load_yaml_rooms_section( &doc, rooms_id );

    if ( helps_id )
        load_yaml_helps_section( &doc, helps_id );

    if ( specials_id )
        load_yaml_specials_section( &doc, specials_id );

    yaml_document_delete( &doc );
    yaml_parser_delete( &parser );
    return TRUE;
}

/*
 * Pass 2: loads resets and shops (requires rooms/mobs to be loaded first).
 */
bool
load_yaml_area_resets_shops( const char *filename )
{
    yaml_parser_t   parser;
    yaml_document_t doc;
    int             resets_id;
    int             shops_id;

    if ( !parse_yaml_file( filename, &parser, &doc ) )
        return FALSE;

    resets_id = doc_find( &doc, 1, "resets" );
    shops_id  = doc_find( &doc, 1, "shops"  );

    /* Re-establish area_last by scanning for the area whose filename
     * matches (needed so load_yaml_resets_section can reference area_last).
     * We do not re-load the #AREADATA block in pass 2 – we only need
     * area_last for new_reset() bookkeeping.  That is already satisfied
     * because boot_db processes areas in the same order in both passes. */

    if ( resets_id )
        load_yaml_resets_section( &doc, resets_id );

    if ( shops_id )
        load_yaml_shops_section( &doc, shops_id );

    yaml_document_delete( &doc );
    yaml_parser_delete( &parser );
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* save_yaml_area: serialise an area to a .yaml file                   */
/* ------------------------------------------------------------------ */

/*
 * Emitter helper macros.
 */
#define YE(call) \
    if ( !(call) ) { bug( "save_yaml_area: emit error.", 0 ); goto cleanup; }

static void
emit_scalar( yaml_emitter_t *em, const char *value )
{
    yaml_event_t ev;
    yaml_scalar_event_initialize(
        &ev,
        NULL,                       /* anchor */
        (yaml_char_t *)"!!str",     /* tag    */
        (yaml_char_t *)( value ? value : "" ),
        (int)strlen( value ? value : "" ),
        1,                          /* plain_implicit */
        1,                          /* quoted_implicit */
        YAML_ANY_SCALAR_STYLE
    );
    yaml_emitter_emit( em, &ev );
}

static void
emit_int( yaml_emitter_t *em, long value )
{
    char buf[32];
    snprintf( buf, sizeof(buf), "%ld", value );
    emit_scalar( em, buf );
}

static void
emit_key( yaml_emitter_t *em, const char *key )
{
    emit_scalar( em, key );
}

static void
emit_str_kv( yaml_emitter_t *em, const char *key, const char *value )
{
    emit_key( em, key );
    emit_scalar( em, value );
}

static void
emit_int_kv( yaml_emitter_t *em, const char *key, long value )
{
    emit_key( em, key );
    emit_int( em, value );
}

static void
emit_mapping_start( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_mapping_start_event_initialize(
        &ev, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE );
    yaml_emitter_emit( em, &ev );
}

static void
emit_mapping_end( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_mapping_end_event_initialize( &ev );
    yaml_emitter_emit( em, &ev );
}

static void
emit_sequence_start( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_sequence_start_event_initialize(
        &ev, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE );
    yaml_emitter_emit( em, &ev );
}

static void
emit_sequence_end( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_sequence_end_event_initialize( &ev );
    yaml_emitter_emit( em, &ev );
}

bool
save_yaml_area( AREA_DATA *pArea )
{
    char             yaml_file[MAX_INPUT_LENGTH];
    char             dot_buf[MAX_INPUT_LENGTH];
    char            *dot;
    FILE            *fp;
    yaml_emitter_t   em;
    yaml_event_t     ev;
    ROOM_INDEX_DATA *pRoom;
    MOB_INDEX_DATA  *pMob;
    OBJ_INDEX_DATA  *pObj;
    RESET_DATA      *pReset;
    SHOP_DATA       *pShop;
    int              i;

    /* Build output filename from pArea->filename */
    strncpy( dot_buf, pArea->filename, sizeof(dot_buf) - 6 );
    dot_buf[sizeof(dot_buf) - 6] = '\0';
    dot = strrchr( dot_buf, '.' );
    if ( dot && strcmp( dot, ".are" ) == 0 )
    {
        *dot = '\0';
        strcat( dot_buf, ".yaml" );
    }
    else
    {
        strncat( dot_buf, ".yaml", sizeof(dot_buf) - strlen(dot_buf) - 1 );
    }
    strncpy( yaml_file, dot_buf, sizeof(yaml_file) - 1 );
    yaml_file[sizeof(yaml_file) - 1] = '\0';

    fp = fopen( yaml_file, "w" );
    if ( !fp )
    {
        { char _msg[512]; snprintf(_msg, sizeof(_msg), "save_yaml_area: cannot open '%s' for write.", yaml_file); bug(_msg, 0); }
        return FALSE;
    }

    if ( !yaml_emitter_initialize( &em ) )
    {
        bug( "save_yaml_area: yaml_emitter_initialize failed.", 0 );
        fclose( fp );
        return FALSE;
    }
    yaml_emitter_set_output_file( &em, fp );
    yaml_emitter_set_unicode( &em, 1 );

    /* Stream / document start */
    yaml_stream_start_event_initialize( &ev, YAML_UTF8_ENCODING );
    yaml_emitter_emit( &em, &ev );

    yaml_document_start_event_initialize( &ev, NULL, NULL, NULL, 1 );
    yaml_emitter_emit( &em, &ev );

    emit_mapping_start( &em );          /* root mapping */

    /* ----- area section ----- */
    emit_key( &em, "area" );
    emit_mapping_start( &em );
        emit_str_kv( &em, "name",      pArea->name     );
        emit_str_kv( &em, "builders",  pArea->builders );
        emit_int_kv( &em, "security",  pArea->security  );
        emit_int_kv( &em, "area_flags", pArea->area_flags );
        emit_key( &em, "vnums" );
        emit_sequence_start( &em );
            emit_int( &em, pArea->lvnum );
            emit_int( &em, pArea->uvnum );
        emit_sequence_end( &em );
    emit_mapping_end( &em );

    /* ----- mobiles section ----- */
    emit_key( &em, "mobiles" );
    emit_sequence_start( &em );
    for ( i = 0; i < MAX_KEY_HASH; i++ )
    {
        for ( pMob = mob_index_hash[i]; pMob != NULL; pMob = pMob->next )
        {
            char buf[32];
            if ( pMob->area != pArea )
                continue;

            emit_mapping_start( &em );
                emit_int_kv(  &em, "vnum",        pMob->vnum           );
                emit_str_kv(  &em, "keywords",    pMob->player_name    );
                emit_str_kv(  &em, "short_desc",  pMob->short_descr    );
                emit_str_kv(  &em, "long_desc",   pMob->long_descr     );
                emit_str_kv(  &em, "description", pMob->description    );
                /* race: look up name */
                emit_str_kv(  &em, "race",        race_table[pMob->race].name );
                emit_int_kv(  &em, "act",         pMob->act
                                                & ~(ACT_IS_NPC
                                                   | race_table[pMob->race].act ) );
                emit_int_kv(  &em, "affected_by", pMob->affected_by
                                                & ~race_table[pMob->race].aff  );
                emit_int_kv(  &em, "affected2_by", pMob->affected2_by
                                                 & ~race_table[pMob->race].aff2 );
                emit_int_kv(  &em, "alignment",   pMob->alignment      );
                emit_int_kv(  &em, "clan",        pMob->clan           );
                emit_int_kv(  &em, "level",       pMob->level          );
                emit_int_kv(  &em, "hitroll",     pMob->hitroll        );

                emit_key( &em, "hit" );
                emit_sequence_start( &em );
                    emit_int( &em, pMob->hit[DICE_NUMBER] );
                    emit_int( &em, pMob->hit[DICE_TYPE]   );
                    emit_int( &em, pMob->hit[DICE_BONUS]  );
                emit_sequence_end( &em );

                emit_key( &em, "mana" );
                emit_sequence_start( &em );
                    emit_int( &em, pMob->mana[DICE_NUMBER] );
                    emit_int( &em, pMob->mana[DICE_TYPE]   );
                    emit_int( &em, pMob->mana[DICE_BONUS]  );
                emit_sequence_end( &em );

                emit_key( &em, "damage" );
                emit_sequence_start( &em );
                    emit_int( &em, pMob->damage[DICE_NUMBER] );
                    emit_int( &em, pMob->damage[DICE_TYPE]   );
                    emit_int( &em, pMob->damage[DICE_BONUS]  );
                emit_sequence_end( &em );

                emit_int_kv( &em, "dam_type", pMob->dam_type );

                emit_key( &em, "ac" );
                emit_mapping_start( &em );
                    emit_int_kv( &em, "pierce", pMob->ac[AC_PIERCE] / 10 );
                    emit_int_kv( &em, "bash",   pMob->ac[AC_BASH]   / 10 );
                    emit_int_kv( &em, "slash",  pMob->ac[AC_SLASH]  / 10 );
                    emit_int_kv( &em, "exotic", pMob->ac[AC_EXOTIC] / 10 );
                emit_mapping_end( &em );

                emit_int_kv( &em, "off_flags",  pMob->off_flags
                                              & ~race_table[pMob->race].off  );
                emit_int_kv( &em, "imm_flags",  pMob->imm_flags
                                              & ~race_table[pMob->race].imm  );
                emit_int_kv( &em, "res_flags",  pMob->res_flags
                                              & ~race_table[pMob->race].res  );
                emit_int_kv( &em, "vuln_flags", pMob->vuln_flags
                                              & ~race_table[pMob->race].vuln );
                emit_int_kv( &em, "start_pos",   pMob->start_pos   );
                emit_int_kv( &em, "default_pos", pMob->default_pos );
                emit_int_kv( &em, "sex",         pMob->sex         );
                emit_int_kv( &em, "gold",        pMob->gold        );
                emit_int_kv( &em, "form",        pMob->form
                                              & ~race_table[pMob->race].form  );
                emit_int_kv( &em, "parts",       pMob->parts
                                              & ~race_table[pMob->race].parts );

                /* size as single letter */
                snprintf( buf, sizeof(buf), "%c",
                    pMob->size == SIZE_TINY   ? 'T' :
                    pMob->size == SIZE_SMALL  ? 'S' :
                    pMob->size == SIZE_LARGE  ? 'L' :
                    pMob->size == SIZE_HUGE   ? 'H' :
                    pMob->size == SIZE_GIANT  ? 'G' : 'M' );
                emit_str_kv( &em, "size", buf );
                emit_str_kv( &em, "material",
                    material_name( pMob->material ) );
                /* spec_fun: look up name */
                emit_str_kv( &em, "spec_fun",
                    pMob->spec_fun ? spec_string( pMob->spec_fun ) : "" );
            emit_mapping_end( &em );
        }
    }
    emit_sequence_end( &em );            /* end mobiles */

    /* ----- objects section ----- */
    emit_key( &em, "objects" );
    emit_sequence_start( &em );
    for ( i = 0; i < MAX_KEY_HASH; i++ )
    {
        for ( pObj = obj_index_hash[i]; pObj != NULL; pObj = pObj->next )
        {
            AFFECT_DATA      *paf;
            EXTRA_DESCR_DATA *ed;
            int               j;

            if ( pObj->area != pArea )
                continue;

            emit_mapping_start( &em );
                emit_int_kv( &em, "vnum",        pObj->vnum        );
                emit_str_kv( &em, "name",        pObj->name        );
                emit_str_kv( &em, "short_desc",  pObj->short_descr );
                emit_str_kv( &em, "description", pObj->description );
                emit_str_kv( &em, "material",    material_name( pObj->material ) );
                emit_int_kv( &em, "item_type",   pObj->item_type   );
                emit_int_kv( &em, "extra_flags", pObj->extra_flags );
                emit_int_kv( &em, "wear_flags",  pObj->wear_flags  );
                emit_int_kv( &em, "level",       pObj->level       );
                emit_int_kv( &em, "weight",      pObj->weight      );
                emit_int_kv( &em, "cost",        pObj->cost        );

                emit_key( &em, "values" );
                emit_sequence_start( &em );
                for ( j = 0; j < 5; j++ )
                    emit_int( &em, pObj->value[j] );
                emit_sequence_end( &em );

                emit_key( &em, "affects" );
                emit_sequence_start( &em );
                for ( paf = pObj->affected; paf != NULL; paf = paf->next )
                {
                    const char *where_str;
                    switch ( paf->where )
                    {
                        case TO_AFFECTS:  where_str = "A"; break;
                        case TO_IMMUNE:   where_str = "I"; break;
                        case TO_RESIST:   where_str = "R"; break;
                        case TO_VULN:     where_str = "V"; break;
                        case TO_AFFECTS2: where_str = "Z"; break;
                        default:          where_str = "O"; break;
                    }
                    emit_mapping_start( &em );
                        emit_str_kv( &em, "where",     where_str     );
                        emit_int_kv( &em, "location",  paf->location );
                        emit_int_kv( &em, "modifier",  paf->modifier );
                        emit_int_kv( &em, "bitvector", paf->bitvector );
                    emit_mapping_end( &em );
                }
                emit_sequence_end( &em );

                emit_key( &em, "extra_descs" );
                emit_sequence_start( &em );
                for ( ed = pObj->extra_descr; ed != NULL; ed = ed->next )
                {
                    emit_mapping_start( &em );
                        emit_str_kv( &em, "keywords",    ed->keyword     );
                        emit_str_kv( &em, "description", ed->description );
                    emit_mapping_end( &em );
                }
                emit_sequence_end( &em );
            emit_mapping_end( &em );
        }
    }
    emit_sequence_end( &em );            /* end objects */

    /* ----- rooms section ----- */
    emit_key( &em, "rooms" );
    emit_sequence_start( &em );
    for ( i = 0; i < MAX_KEY_HASH; i++ )
    {
        for ( pRoom = room_index_hash[i]; pRoom != NULL; pRoom = pRoom->next )
        {
            EXTRA_DESCR_DATA *ed;
            int               d;

            if ( pRoom->area != pArea )
                continue;

            emit_mapping_start( &em );
                emit_int_kv( &em, "vnum",        pRoom->vnum        );
                emit_str_kv( &em, "name",        pRoom->name        );
                emit_str_kv( &em, "description", pRoom->description );
                emit_int_kv( &em, "room_flags",  pRoom->room_flags  );
                emit_int_kv( &em, "sector",      pRoom->sector_type );
                emit_str_kv( &em, "owner",       pRoom->owner       );

                emit_key( &em, "exits" );
                emit_sequence_start( &em );
                for ( d = 0; d <= 5; d++ )
                {
                    EXIT_DATA *pexit = pRoom->exit[d];
                    if ( !pexit )
                        continue;
                    emit_mapping_start( &em );
                        emit_int_kv( &em, "dir",        d                    );
                        emit_str_kv( &em, "desc",       pexit->description   );
                        emit_str_kv( &em, "keywords",   pexit->keyword       );
                        emit_int_kv( &em, "exit_flags", pexit->rs_flags      );
                        emit_int_kv( &em, "key",        pexit->key           );
                        emit_int_kv( &em, "to_vnum",    pexit->u1.to_room
                                          ? pexit->u1.to_room->vnum : -1     );
                    emit_mapping_end( &em );
                }
                emit_sequence_end( &em );

                emit_key( &em, "extra_descs" );
                emit_sequence_start( &em );
                for ( ed = pRoom->extra_descr; ed != NULL; ed = ed->next )
                {
                    emit_mapping_start( &em );
                        emit_str_kv( &em, "keywords",    ed->keyword     );
                        emit_str_kv( &em, "description", ed->description );
                    emit_mapping_end( &em );
                }
                emit_sequence_end( &em );
            emit_mapping_end( &em );
        }
    }
    emit_sequence_end( &em );            /* end rooms */

    /* ----- resets section ----- */
    emit_key( &em, "resets" );
    emit_sequence_start( &em );
    for ( i = 0; i < MAX_KEY_HASH; i++ )
    {
        for ( pRoom = room_index_hash[i]; pRoom != NULL; pRoom = pRoom->next )
        {
            if ( pRoom->area != pArea )
                continue;
            for ( pReset = pRoom->reset_first; pReset != NULL;
                  pReset = pReset->next )
            {
                char cmd[2];
                cmd[0] = pReset->command;
                cmd[1] = '\0';
                emit_mapping_start( &em );
                    emit_str_kv( &em, "command", cmd           );
                    emit_int_kv( &em, "arg1",    pReset->arg1  );
                    emit_int_kv( &em, "arg2",    pReset->arg2  );
                    emit_int_kv( &em, "arg3",    pReset->arg3  );
                emit_mapping_end( &em );
            }
        }
    }
    emit_sequence_end( &em );            /* end resets */

    /* ----- shops section ----- */
    emit_key( &em, "shops" );
    emit_sequence_start( &em );
    for ( i = 0; i < MAX_KEY_HASH; i++ )
    {
        for ( pMob = mob_index_hash[i]; pMob != NULL; pMob = pMob->next )
        {
            int j;
            if ( pMob->area != pArea || !pMob->pShop )
                continue;
            pShop = pMob->pShop;
            emit_mapping_start( &em );
                emit_int_kv( &em, "keeper",       pShop->keeper      );
                emit_key( &em, "buy_types" );
                emit_sequence_start( &em );
                for ( j = 0; j < MAX_TRADE; j++ )
                    emit_int( &em, pShop->buy_type[j] );
                emit_sequence_end( &em );
                emit_int_kv( &em, "profit_buy",   pShop->profit_buy  );
                emit_int_kv( &em, "profit_sell",  pShop->profit_sell );
                emit_int_kv( &em, "open_hour",    pShop->open_hour   );
                emit_int_kv( &em, "close_hour",   pShop->close_hour  );
            emit_mapping_end( &em );
        }
    }
    emit_sequence_end( &em );            /* end shops */

    emit_mapping_end( &em );             /* end root */

    yaml_document_end_event_initialize( &ev, 1 );
    yaml_emitter_emit( &em, &ev );

    yaml_stream_end_event_initialize( &ev );
    yaml_emitter_emit( &em, &ev );

    yaml_emitter_delete( &em );
    fclose( fp );
    return TRUE;

cleanup:
    yaml_emitter_delete( &em );
    fclose( fp );
    return FALSE;
}
