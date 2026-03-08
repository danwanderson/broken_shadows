///////////////////////////////////////////////////////////////////////
////  Broken Shadows (c) 1995-2022 by Daniel Anderson
////  YAML player-file support added 2026
///////////////////////////////////////////////////////////////////////

/*
 * YAML-based player file load/save.
 *
 * The YAML player file mirrors all fields written by fwrite_char(),
 * fwrite_obj() and fwrite_pet() in save.c, using descriptive key names.
 *
 * File layout (yaml_path = ../player/<Name>.yaml):
 *
 *   player: { ... all character fields ... }
 *   objects:
 *     - { vnum, nest, wear_loc, ... }
 *   pets:
 *     - { vnum, name, ... }
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <yaml.h>

#include "merc.h"
#include "board.h"
#include "yaml_save.h"

#define MAX_NEST    100

/* ------------------------------------------------------------------ */
/* Externs from save.c / other modules                                 */
/* ------------------------------------------------------------------ */

extern OBJ_DATA    *obj_free;
extern AFFECT_DATA *affect_free;
extern CHAR_DATA   *char_free;
extern PC_DATA     *pcdata_free;
extern EXTRA_DESCR_DATA *extra_descr_free;
extern FILE        *fpReserve;

static OBJ_DATA    *rgObjNest[MAX_NEST];

/* ------------------------------------------------------------------ */
/* Emitter helpers (duplicated locally to keep yaml_save.c standalone) */
/* ------------------------------------------------------------------ */

static void
ys_scalar( yaml_emitter_t *em, const char *value )
{
    yaml_event_t ev;
    yaml_scalar_event_initialize(
        &ev, NULL,
        (yaml_char_t *)"!!str",
        (yaml_char_t *)( value ? value : "" ),
        (int)strlen( value ? value : "" ),
        1, 1, YAML_ANY_SCALAR_STYLE );
    yaml_emitter_emit( em, &ev );
    yaml_event_delete( &ev );
}

static void
ys_int( yaml_emitter_t *em, long v )
{
    char buf[32];
    snprintf( buf, sizeof(buf), "%ld", v );
    ys_scalar( em, buf );
}

static void
ys_key( yaml_emitter_t *em, const char *k )
{
    ys_scalar( em, k );
}

static void
ys_str_kv( yaml_emitter_t *em, const char *k, const char *v )
{
    ys_key( em, k );
    ys_scalar( em, v ? v : "" );
}

static void
ys_int_kv( yaml_emitter_t *em, const char *k, long v )
{
    ys_key( em, k );
    ys_int( em, v );
}

static void
ys_map_start( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_mapping_start_event_initialize(
        &ev, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE );
    yaml_emitter_emit( em, &ev );
    yaml_event_delete( &ev );
}

static void
ys_map_end( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_mapping_end_event_initialize( &ev );
    yaml_emitter_emit( em, &ev );
    yaml_event_delete( &ev );
}

static void
ys_seq_start( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_sequence_start_event_initialize(
        &ev, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE );
    yaml_emitter_emit( em, &ev );
    yaml_event_delete( &ev );
}

static void
ys_seq_end( yaml_emitter_t *em )
{
    yaml_event_t ev;
    yaml_sequence_end_event_initialize( &ev );
    yaml_emitter_emit( em, &ev );
    yaml_event_delete( &ev );
}

/* ------------------------------------------------------------------ */
/* Build the YAML file path from a character name                      */
/* ------------------------------------------------------------------ */

static void
yaml_player_path( const char *name, char *buf, size_t len )
{
    snprintf( buf, len, "%s%s.yaml", PLAYER_DIR, capitalize( (char *)name ) );
}

bool
yaml_player_file_exists( const char *name, char *yaml_path )
{
    struct stat st;
    yaml_player_path( name, yaml_path, MAX_INPUT_LENGTH );
    return ( stat( yaml_path, &st ) == 0 && S_ISREG( st.st_mode ) );
}

/* ------------------------------------------------------------------ */
/* Writing helpers                                                      */
/* ------------------------------------------------------------------ */

static void
write_affect( yaml_emitter_t *em, AFFECT_DATA *paf )
{
    const char *skill_name = "";

    if ( paf->type >= 0 && paf->type < MAX_SKILL
    &&   skill_table[paf->type].name != NULL )
        skill_name = skill_table[paf->type].name;

    ys_map_start( em );
        ys_str_kv( em, "name",      skill_name      );
        ys_int_kv( em, "where",     paf->where       );
        ys_int_kv( em, "level",     paf->level       );
        ys_int_kv( em, "duration",  paf->duration    );
        ys_int_kv( em, "modifier",  paf->modifier    );
        ys_int_kv( em, "location",  paf->location    );
        ys_int_kv( em, "bitvector", paf->bitvector   );
    ys_map_end( em );
}

/*
 * Write a single object (and any contained objects) to the emitter.
 * Objects are written in reverse-next_content order so that they
 * load forwards, matching the fwrite_obj() behaviour.
 */
static void
write_obj( yaml_emitter_t *em, CHAR_DATA *ch, OBJ_DATA *obj, int iNest )
{
    AFFECT_DATA      *paf;
    EXTRA_DESCR_DATA *ed;
    int               j;

    /* recurse siblings first (mirrors fwrite_obj recursion) */
    if ( obj->next_content != NULL )
        write_obj( em, ch, obj->next_content, iNest );

    /* same filter as fwrite_obj */
    if ( ( ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER )
    ||   obj->item_type == ITEM_KEY
    ||   ( obj->item_type == ITEM_MAP && !obj->value[0] ) )
    {
        /* still need to recurse contents even if skipping this obj */
        if ( obj->contains != NULL )
            write_obj( em, ch, obj->contains, iNest + 1 );
        return;
    }

    ys_map_start( em );

    ys_int_kv( em, "vnum",      obj->pIndexData->vnum );
    ys_int_kv( em, "nest",      iNest                 );
    ys_int_kv( em, "wear_loc",  obj->wear_loc         );
    ys_int_kv( em, "enchanted", obj->enchanted ? 1 : 0 );

    /* only emit fields that differ from the prototype */
    if ( obj->name != obj->pIndexData->name )
        ys_str_kv( em, "name",        obj->name          );
    if ( obj->short_descr != obj->pIndexData->short_descr )
        ys_str_kv( em, "short_desc",  obj->short_descr   );
    if ( obj->description != obj->pIndexData->description )
        ys_str_kv( em, "description", obj->description   );
    if ( obj->extra_flags != obj->pIndexData->extra_flags )
        ys_int_kv( em, "extra_flags", obj->extra_flags   );
    if ( obj->wear_flags != obj->pIndexData->wear_flags )
        ys_int_kv( em, "wear_flags",  obj->wear_flags    );
    if ( obj->item_type != obj->pIndexData->item_type )
        ys_int_kv( em, "item_type",   obj->item_type     );
    if ( obj->weight != obj->pIndexData->weight )
        ys_int_kv( em, "weight",      obj->weight        );

    ys_int_kv( em, "level",  obj->level  );
    ys_int_kv( em, "timer",  obj->timer  );
    ys_int_kv( em, "cost",   obj->cost   );

    if ( obj->value[0] != obj->pIndexData->value[0]
    ||   obj->value[1] != obj->pIndexData->value[1]
    ||   obj->value[2] != obj->pIndexData->value[2]
    ||   obj->value[3] != obj->pIndexData->value[3]
    ||   obj->value[4] != obj->pIndexData->value[4] )
    {
        ys_key( em, "values" );
        ys_seq_start( em );
        for ( j = 0; j < 5; j++ )
            ys_int( em, obj->value[j] );
        ys_seq_end( em );
    }

    /* spells stored in values for potions/wands/etc */
    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
        if ( obj->value[1] > 0 && obj->value[1] < MAX_SKILL
        &&   skill_table[obj->value[1]].name )
            ys_str_kv( em, "spell_1", skill_table[obj->value[1]].name );
        if ( obj->value[2] > 0 && obj->value[2] < MAX_SKILL
        &&   skill_table[obj->value[2]].name )
            ys_str_kv( em, "spell_2", skill_table[obj->value[2]].name );
        if ( obj->value[3] > 0 && obj->value[3] < MAX_SKILL
        &&   skill_table[obj->value[3]].name )
            ys_str_kv( em, "spell_3", skill_table[obj->value[3]].name );
        break;
    case ITEM_PILL:
    case ITEM_STAFF:
    case ITEM_WAND:
        if ( obj->value[3] > 0 && obj->value[3] < MAX_SKILL
        &&   skill_table[obj->value[3]].name )
            ys_str_kv( em, "spell_3", skill_table[obj->value[3]].name );
        break;
    default:
        break;
    }

    ys_key( em, "affects" );
    ys_seq_start( em );
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->type >= 0 && paf->type < MAX_SKILL )
            write_affect( em, paf );
    }
    ys_seq_end( em );

    ys_key( em, "extra_descs" );
    ys_seq_start( em );
    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
        ys_map_start( em );
            ys_str_kv( em, "keywords",    ed->keyword     );
            ys_str_kv( em, "description", ed->description );
        ys_map_end( em );
    }
    ys_seq_end( em );

    ys_map_end( em );   /* end this object mapping */

    /* recurse contained objects at deeper nest */
    if ( obj->contains != NULL )
        write_obj( em, ch, obj->contains, iNest + 1 );
}

/* ------------------------------------------------------------------ */
/* Public: save_char_obj_yaml                                          */
/* ------------------------------------------------------------------ */

void
save_char_obj_yaml( CHAR_DATA *ch )
{
    char          strsave[MAX_INPUT_LENGTH];
    char          strtmp[MAX_INPUT_LENGTH];
    FILE         *fp;
    yaml_emitter_t em;
    yaml_event_t   ev;
    AFFECT_DATA   *paf;
    int            sn, gn, pos, i;

    if ( IS_NPC(ch) )
        return;

    if ( ch->desc != NULL && ch->desc->original != NULL )
        ch = ch->desc->original;

    /* god log (same as save_char_obj) */
#if defined(unix)
    if ( IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL )
    {
        fclose( fpReserve );
        sprintf( strtmp, "%s%s", GOD_DIR, capitalize(ch->name) );
        if ( ( fp = fopen( strtmp, "w" ) ) != NULL )
        {
            fprintf( fp, "Lev %2d Trust %2d  %s%s\n",
                ch->level, get_trust(ch), ch->name, ch->pcdata->title );
            fclose( fp );
        }
        fpReserve = fopen( NULL_FILE, "r" );
    }
#endif

    yaml_player_path( ch->name, strsave, sizeof(strsave) );

    /* Write to a temp file first, then rename */
    snprintf( strtmp, sizeof(strtmp), "%s%s.yaml",
              PLAYER_DIR, "temp_yaml" );

    fclose( fpReserve );
    fp = fopen( strtmp, "w" );
    if ( !fp )
    {
        bug( "save_char_obj_yaml: fopen failed for %s", (int)strtmp );
        fpReserve = fopen( NULL_FILE, "r" );
        return;
    }

    if ( !yaml_emitter_initialize( &em ) )
    {
        bug( "save_char_obj_yaml: yaml_emitter_initialize failed", 0 );
        fclose( fp );
        fpReserve = fopen( NULL_FILE, "r" );
        return;
    }
    yaml_emitter_set_output_file( &em, fp );
    yaml_emitter_set_unicode( &em, 1 );

    yaml_stream_start_event_initialize( &ev, YAML_UTF8_ENCODING );
    yaml_emitter_emit( &em, &ev );
    yaml_event_delete( &ev );

    yaml_document_start_event_initialize( &ev, NULL, NULL, NULL, 1 );
    yaml_emitter_emit( &em, &ev );
    yaml_event_delete( &ev );

    ys_map_start( &em );     /* root */

    /* ---- player section ---- */
    ys_key( &em, "player" );
    ys_map_start( &em );

    ys_str_kv( &em, "name",         ch->name                                );
    ys_int_kv( &em, "version",      4                                        );
    if ( ch->short_descr && ch->short_descr[0] )
        ys_str_kv( &em, "short_desc",   ch->short_descr                     );
    if ( ch->long_descr && ch->long_descr[0] )
        ys_str_kv( &em, "long_desc",    ch->long_descr                      );
    if ( ch->description && ch->description[0] )
        ys_str_kv( &em, "description",  ch->description                     );
    ys_str_kv( &em, "race",         pc_race_table[ch->race].name            );
    ys_int_kv( &em, "sex",          ch->sex                                  );
    ys_int_kv( &em, "class",        ch->ch_class                             );
    ys_int_kv( &em, "level",        ch->level                                );
    if ( ch->trust )
        ys_int_kv( &em, "trust",    ch->trust                                );
    ys_int_kv( &em, "security",     ch->pcdata->security                     );
    ys_int_kv( &em, "logon",        (long)ch->logon                          );
    ys_int_kv( &em, "played",
        (long)( ch->played + (int)(current_time - ch->logon) )               );
    ys_int_kv( &em, "last_note",    (long)ch->last_note                      );
    ys_int_kv( &em, "lines",        ch->lines                                );
    ys_int_kv( &em, "room",
        ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
          && ch->was_in_room != NULL )
            ? ch->was_in_room->vnum
            : ch->in_room == NULL ? 3001 : ch->in_room->vnum                 );

    /* hit / mana / move */
    ys_key( &em, "hmv" );
    ys_map_start( &em );
        ys_int_kv( &em, "hit",      ch->hit      );
        ys_int_kv( &em, "max_hit",  ch->max_hit  );
        ys_int_kv( &em, "mana",     ch->mana     );
        ys_int_kv( &em, "max_mana", ch->max_mana );
        ys_int_kv( &em, "move",     ch->move     );
        ys_int_kv( &em, "max_move", ch->max_move );
    ys_map_end( &em );

    ys_int_kv( &em, "gold",         ch->gold                                 );
    ys_int_kv( &em, "bank",         ch->bank                                 );
    ys_int_kv( &em, "bounty",       ch->pcdata->bounty                       );
    ys_int_kv( &em, "pkills",       ch->pcdata->pkills                       );
    ys_int_kv( &em, "pkilled",      ch->pcdata->pkilled                      );
    ys_int_kv( &em, "killed",       ch->pcdata->killed                       );
    if ( ch->pcdata->spouse )
        ys_str_kv( &em, "spouse",   ch->pcdata->spouse                       );
    ys_int_kv( &em, "exp",          ch->exp                                  );
    ys_int_kv( &em, "act",          ch->act                                  );
    ys_int_kv( &em, "affected_by",  ch->affected_by                          );
    ys_int_kv( &em, "affected2_by", ch->affected2_by                         );
    ys_int_kv( &em, "comm",         ch->comm                                 );
    ys_int_kv( &em, "wiznet",       ch->wiznet                               );
    ys_int_kv( &em, "invis_level",  ch->invis_level                          );
    ys_int_kv( &em, "incog_level",  ch->incog_level                          );
    ys_int_kv( &em, "position",
        ch->position == POS_FIGHTING ? POS_STANDING : ch->position           );
    ys_int_kv( &em, "practice",     ch->practice                             );
    ys_int_kv( &em, "train",        ch->train                                );
    ys_int_kv( &em, "saving_throw", ch->saving_throw                         );
    ys_int_kv( &em, "alignment",    ch->alignment                            );
    ys_int_kv( &em, "hitroll",      ch->hitroll                              );
    ys_int_kv( &em, "damroll",      ch->damroll                              );

    ys_key( &em, "armor" );
    ys_map_start( &em );
        ys_int_kv( &em, "pierce", ch->armor[AC_PIERCE] );
        ys_int_kv( &em, "bash",   ch->armor[AC_BASH]   );
        ys_int_kv( &em, "slash",  ch->armor[AC_SLASH]  );
        ys_int_kv( &em, "exotic", ch->armor[AC_EXOTIC] );
    ys_map_end( &em );

    ys_int_kv( &em, "wimpy", ch->wimpy );

    ys_key( &em, "perm_stat" );
    ys_map_start( &em );
        ys_int_kv( &em, "str", ch->perm_stat[STAT_STR] );
        ys_int_kv( &em, "int", ch->perm_stat[STAT_INT] );
        ys_int_kv( &em, "wis", ch->perm_stat[STAT_WIS] );
        ys_int_kv( &em, "dex", ch->perm_stat[STAT_DEX] );
        ys_int_kv( &em, "con", ch->perm_stat[STAT_CON] );
    ys_map_end( &em );

    ys_key( &em, "mod_stat" );
    ys_map_start( &em );
        ys_int_kv( &em, "str", ch->mod_stat[STAT_STR] );
        ys_int_kv( &em, "int", ch->mod_stat[STAT_INT] );
        ys_int_kv( &em, "wis", ch->mod_stat[STAT_WIS] );
        ys_int_kv( &em, "dex", ch->mod_stat[STAT_DEX] );
        ys_int_kv( &em, "con", ch->mod_stat[STAT_CON] );
    ys_map_end( &em );

    /* player-only fields */
    ys_str_kv( &em, "password",    ch->pcdata->pwd                           );
    ys_str_kv( &em, "bamfin",      ch->pcdata->bamfin                        );
    ys_str_kv( &em, "bamfout",     ch->pcdata->bamfout                       );
    ys_str_kv( &em, "title",       ch->pcdata->title                         );
    ys_int_kv( &em, "points",      ch->pcdata->points                        );
    ys_int_kv( &em, "true_sex",    ch->pcdata->true_sex                      );
    ys_int_kv( &em, "last_level",  ch->pcdata->last_level                    );

    ys_key( &em, "perm_hmv" );
    ys_map_start( &em );
        ys_int_kv( &em, "hit",  ch->pcdata->perm_hit  );
        ys_int_kv( &em, "mana", ch->pcdata->perm_mana );
        ys_int_kv( &em, "move", ch->pcdata->perm_move );
    ys_map_end( &em );

    ys_key( &em, "condition" );
    ys_map_start( &em );
        ys_int_kv( &em, "drunk",  ch->pcdata->condition[COND_DRUNK]  );
        ys_int_kv( &em, "full",   ch->pcdata->condition[COND_FULL]   );
        ys_int_kv( &em, "thirst", ch->pcdata->condition[COND_THIRST] );
    ys_map_end( &em );

    {
        char safe_prompt[MAX_STRING_LENGTH];
        strncpy( safe_prompt, ch->pcdata->prompt, sizeof(safe_prompt) - 1 );
        safe_prompt[sizeof(safe_prompt) - 1] = '\0';
        ys_str_kv( &em, "prompt", safe_prompt );
    }

    if ( ch->pcdata->clan )
        ys_str_kv( &em, "clan", clan_lookup( ch->pcdata->clan ) );
    if ( ch->pcdata->clan_leader )
        ys_int_kv( &em, "clan_leader", ch->pcdata->clan_leader );

    ys_int_kv( &em, "quest_points", ch->questpoints );
    ys_int_kv( &em, "quest_next",   ch->nextquest   );
    ys_int_kv( &em, "bonus_points", ch->bonusPoints );

    ys_int_kv( &em, "recall_room",
        ch->pcdata->recall_room
            ? ch->pcdata->recall_room->vnum
            : ROOM_VNUM_TEMPLE );

    ys_str_kv( &em, "email",
        ch->pcdata->email ? ch->pcdata->email : "(none)" );
    ys_str_kv( &em, "comment",
        ch->pcdata->comment ? ch->pcdata->comment : "(none)" );
    ys_int_kv( &em, "incarnations", ch->pcdata->incarnations );

    /* aliases */
    ys_key( &em, "aliases" );
    ys_seq_start( &em );
    for ( pos = 0; pos < MAX_ALIAS; pos++ )
    {
        if ( ch->pcdata->alias[pos] == NULL
        ||   ch->pcdata->alias_sub[pos] == NULL )
            break;
        ys_map_start( &em );
            ys_str_kv( &em, "alias",     ch->pcdata->alias[pos]     );
            ys_str_kv( &em, "expansion", ch->pcdata->alias_sub[pos] );
        ys_map_end( &em );
    }
    ys_seq_end( &em );

    /* note boards */
    ys_key( &em, "boards" );
    ys_seq_start( &em );
    for ( i = 0; i < MAX_BOARD; i++ )
    {
        ys_map_start( &em );
            ys_str_kv( &em, "name",      boards[i].short_name          );
            ys_int_kv( &em, "last_note", (long)ch->pcdata->last_note[i] );
        ys_map_end( &em );
    }
    ys_seq_end( &em );

    /* skills */
    ys_key( &em, "skills" );
    ys_seq_start( &em );
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if ( skill_table[sn].name == NULL )
            continue;
        if ( ch->pcdata->learned[sn] <= 0 )
            continue;
        ys_map_start( &em );
            ys_str_kv( &em, "name",  skill_table[sn].name  );
            ys_int_kv( &em, "level", ch->pcdata->learned[sn] );
        ys_map_end( &em );
    }
    ys_seq_end( &em );

    /* groups */
    ys_key( &em, "groups" );
    ys_seq_start( &em );
    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            continue;
        if ( !ch->pcdata->group_known[gn] )
            continue;
        ys_scalar( &em, group_table[gn].name );
    }
    ys_seq_end( &em );

    /* affects */
    ys_key( &em, "affects" );
    ys_seq_start( &em );
    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
        if ( paf->type < 0 || paf->type >= MAX_SKILL )
            continue;
        write_affect( &em, paf );
    }
    ys_seq_end( &em );

    ys_map_end( &em );   /* end player: */

    /* ---- objects section ---- */
    ys_key( &em, "objects" );
    ys_seq_start( &em );
    if ( ch->carrying != NULL )
        write_obj( &em, ch, ch->carrying, 0 );
    ys_seq_end( &em );

    /* ---- pets section ---- */
    ys_key( &em, "pets" );
    ys_seq_start( &em );
    if ( ch->pet != NULL && ch->pet->in_room == ch->in_room )
    {
        CHAR_DATA *pet = ch->pet;
        ys_map_start( &em );
            ys_int_kv( &em, "vnum",       pet->pIndexData->vnum    );
            ys_str_kv( &em, "name",       pet->name                );
            ys_str_kv( &em, "short_desc", pet->short_descr         );
            ys_str_kv( &em, "long_desc",  pet->long_descr          );
            ys_int_kv( &em, "level",      pet->level               );
            ys_int_kv( &em, "sex",        pet->sex                 );

            ys_key( &em, "affects" );
            ys_seq_start( &em );
            for ( paf = pet->affected; paf != NULL; paf = paf->next )
            {
                if ( paf->type >= 0 && paf->type < MAX_SKILL )
                    write_affect( &em, paf );
            }
            ys_seq_end( &em );
        ys_map_end( &em );
    }
    ys_seq_end( &em );

    ys_map_end( &em );   /* end root */

    yaml_document_end_event_initialize( &ev, 1 );
    yaml_emitter_emit( &em, &ev );
    yaml_event_delete( &ev );

    yaml_stream_end_event_initialize( &ev );
    yaml_emitter_emit( &em, &ev );
    yaml_event_delete( &ev );

    yaml_emitter_delete( &em );
    fclose( fp );

    /* atomic rename */
    {
        char cmd[MAX_STRING_LENGTH];
        snprintf( cmd, sizeof(cmd), "mv %s %s", strtmp, strsave );
        system( cmd );
    }

    fpReserve = fopen( NULL_FILE, "r" );
}

/* ================================================================== */
/* Loading helpers                                                     */
/* ================================================================== */

static const char *
yl_scalar( yaml_document_t *doc, int node_id )
{
    yaml_node_t *node;
    if ( node_id <= 0 )
        return "";
    node = yaml_document_get_node( doc, node_id );
    if ( node && node->type == YAML_SCALAR_NODE )
        return (const char *)node->data.scalar.value;
    return "";
}

static int
yl_find( yaml_document_t *doc, int map_id, const char *key )
{
    yaml_node_t      *node;
    yaml_node_pair_t *pair;
    if ( map_id <= 0 )
        return 0;
    node = yaml_document_get_node( doc, map_id );
    if ( !node || node->type != YAML_MAPPING_NODE )
        return 0;
    for ( pair = node->data.mapping.pairs.start;
          pair < node->data.mapping.pairs.top; pair++ )
    {
        if ( strcmp( yl_scalar( doc, pair->key ), key ) == 0 )
            return pair->value;
    }
    return 0;
}

static int
yl_int( yaml_document_t *doc, int map_id, const char *key, int def )
{
    int         vn = yl_find( doc, map_id, key );
    const char *s;
    if ( !vn ) return def;
    s = yl_scalar( doc, vn );
    if ( !s || !s[0] ) return def;
    return atoi( s );
}

static long
yl_long( yaml_document_t *doc, int map_id, const char *key, long def )
{
    int         vn = yl_find( doc, map_id, key );
    const char *s;
    if ( !vn ) return def;
    s = yl_scalar( doc, vn );
    if ( !s || !s[0] ) return def;
    return atol( s );
}

static long
yl_flag( yaml_document_t *doc, int map_id, const char *key, long def )
{
    int         vn = yl_find( doc, map_id, key );
    const char *s;
    long        total = 0;
    if ( !vn ) return def;
    s = yl_scalar( doc, vn );
    if ( !s || !s[0] ) return 0;
    /* plain decimal integer (including '0') */
    if ( s[0] == '-' || isdigit( (unsigned char)s[0] ) )
        return atol( s );
    /* flag letter string: each character is one bit via flag_convert() */
    while ( *s )
        total += flag_convert( *s++ );
    return total;
}

static const char *
yl_str( yaml_document_t *doc, int map_id, const char *key )
{
    int vn = yl_find( doc, map_id, key );
    if ( !vn ) return "";
    return yl_scalar( doc, vn );
}

static int
yl_seq_len( yaml_document_t *doc, int seq_id )
{
    yaml_node_t *node;
    if ( seq_id <= 0 ) return 0;
    node = yaml_document_get_node( doc, seq_id );
    if ( !node || node->type != YAML_SEQUENCE_NODE ) return 0;
    return (int)( node->data.sequence.items.top
                - node->data.sequence.items.start );
}

static int
yl_seq_item( yaml_document_t *doc, int seq_id, int n )
{
    yaml_node_t      *node;
    yaml_node_item_t *item;
    if ( seq_id <= 0 ) return 0;
    node = yaml_document_get_node( doc, seq_id );
    if ( !node || node->type != YAML_SEQUENCE_NODE ) return 0;
    item = node->data.sequence.items.start + n;
    if ( item >= node->data.sequence.items.top ) return 0;
    return (int)*item;
}

/* ------------------------------------------------------------------ */
/* Load player section                                                  */
/* ------------------------------------------------------------------ */

static void
load_yaml_char( CHAR_DATA *ch, yaml_document_t *doc, int player_id )
{
    AFFECT_DATA *paf;
    int          aliases_id;
    int          boards_id;
    int          skills_id;
    int          groups_id;
    int          affects_id;
    int          hmv_id;
    int          armor_id;
    int          perm_id;
    int          mod_id;
    int          cond_id;
    int          phmv_id;
    int          i, j;
    const char  *clan_name;

    /* basic identity */
    {
        const char *race_name = yl_str( doc, player_id, "race" );
        ch->race = race_lookup( race_name[0] ? race_name : "human" );
        if ( ch->race < 0 ) ch->race = 0;
    }
    ch->version     = yl_int(  doc, player_id, "version",    3 );
    ch->sex         = (sh_int)yl_int( doc, player_id, "sex",     0 );
    ch->ch_class    = (sh_int)yl_int( doc, player_id, "class",   0 );
    ch->level       = (sh_int)yl_int( doc, player_id, "level",   1 );
    ch->trust       = (sh_int)yl_int( doc, player_id, "trust",   0 );
    ch->pcdata->security = yl_int( doc, player_id, "security", 0 );
    ch->logon       = (time_t)yl_long( doc, player_id, "logon",   (long)current_time );
    ch->played      = yl_int(  doc, player_id, "played",    0 );
    ch->last_note   = (time_t)yl_long( doc, player_id, "last_note", 0 );
    ch->lines       = yl_int(  doc, player_id, "lines",     22 );

    /* short/long/description */
    {
        const char *s;
        s = yl_str( doc, player_id, "short_desc" );
        if ( s[0] ) { free_string(ch->short_descr); ch->short_descr = str_dup(s); }
        s = yl_str( doc, player_id, "long_desc" );
        if ( s[0] ) { free_string(ch->long_descr); ch->long_descr = str_dup(s); }
        s = yl_str( doc, player_id, "description" );
        if ( s[0] ) { free_string(ch->description); ch->description = str_dup(s); }
    }

    /* room – resolved after load via fix */
    ch->in_room = get_room_index( yl_int( doc, player_id, "room", ROOM_VNUM_TEMPLE ) );
    if ( !ch->in_room )
        ch->in_room = get_room_index( ROOM_VNUM_TEMPLE );

    /* hit / mana / move */
    hmv_id = yl_find( doc, player_id, "hmv" );
    if ( hmv_id )
    {
        ch->hit      = (sh_int)yl_int( doc, hmv_id, "hit",      0 );
        ch->max_hit  = (sh_int)yl_int( doc, hmv_id, "max_hit",  0 );
        ch->mana     = (sh_int)yl_int( doc, hmv_id, "mana",     0 );
        ch->max_mana = (sh_int)yl_int( doc, hmv_id, "max_mana", 0 );
        ch->move     = (sh_int)yl_int( doc, hmv_id, "move",     0 );
        ch->max_move = (sh_int)yl_int( doc, hmv_id, "max_move", 0 );
    }

    ch->gold            = yl_long ( doc, player_id, "gold",         0L );
    ch->bank            = yl_long ( doc, player_id, "bank",         0L );
    ch->pcdata->bounty  = yl_long ( doc, player_id, "bounty",       0L );
    ch->pcdata->pkills  = yl_int  ( doc, player_id, "pkills",       0 );
    ch->pcdata->pkilled = yl_int  ( doc, player_id, "pkilled",      0 );
    ch->pcdata->killed  = yl_int  ( doc, player_id, "killed",       0 );
    ch->exp             = yl_long ( doc, player_id, "exp",          0L );
    ch->act             = yl_flag ( doc, player_id, "act",          ch->act );
    ch->affected_by     = (int)yl_flag ( doc, player_id, "affected_by",  0 );
    ch->affected2_by    = yl_flag ( doc, player_id, "affected2_by", 0L );
    ch->comm            = yl_flag ( doc, player_id, "comm",         ch->comm );
    ch->wiznet          = yl_flag ( doc, player_id, "wiznet",       0L );
    ch->invis_level     = (sh_int)yl_int( doc, player_id, "invis_level", 0 );
    ch->incog_level     = (sh_int)yl_int( doc, player_id, "incog_level", 0 );
    ch->position        = (sh_int)yl_int( doc, player_id, "position",    POS_STANDING );
    ch->practice        = (sh_int)yl_int( doc, player_id, "practice",    0 );
    ch->train           = (sh_int)yl_int( doc, player_id, "train",       0 );
    ch->saving_throw    = (sh_int)yl_int( doc, player_id, "saving_throw",0 );
    ch->alignment       = (sh_int)yl_int( doc, player_id, "alignment",   0 );
    ch->hitroll         = (sh_int)yl_int( doc, player_id, "hitroll",     0 );
    ch->damroll         = (sh_int)yl_int( doc, player_id, "damroll",     0 );
    ch->wimpy           = (sh_int)yl_int( doc, player_id, "wimpy",       0 );

    armor_id = yl_find( doc, player_id, "armor" );
    if ( armor_id )
    {
        ch->armor[AC_PIERCE] = (sh_int)yl_int( doc, armor_id, "pierce", 0 );
        ch->armor[AC_BASH]   = (sh_int)yl_int( doc, armor_id, "bash",   0 );
        ch->armor[AC_SLASH]  = (sh_int)yl_int( doc, armor_id, "slash",  0 );
        ch->armor[AC_EXOTIC] = (sh_int)yl_int( doc, armor_id, "exotic", 0 );
    }

    perm_id = yl_find( doc, player_id, "perm_stat" );
    if ( perm_id )
    {
        ch->perm_stat[STAT_STR] = (sh_int)yl_int( doc, perm_id, "str", 0 );
        ch->perm_stat[STAT_INT] = (sh_int)yl_int( doc, perm_id, "int", 0 );
        ch->perm_stat[STAT_WIS] = (sh_int)yl_int( doc, perm_id, "wis", 0 );
        ch->perm_stat[STAT_DEX] = (sh_int)yl_int( doc, perm_id, "dex", 0 );
        ch->perm_stat[STAT_CON] = (sh_int)yl_int( doc, perm_id, "con", 0 );
    }

    mod_id = yl_find( doc, player_id, "mod_stat" );
    if ( mod_id )
    {
        ch->mod_stat[STAT_STR] = (sh_int)yl_int( doc, mod_id, "str", 0 );
        ch->mod_stat[STAT_INT] = (sh_int)yl_int( doc, mod_id, "int", 0 );
        ch->mod_stat[STAT_WIS] = (sh_int)yl_int( doc, mod_id, "wis", 0 );
        ch->mod_stat[STAT_DEX] = (sh_int)yl_int( doc, mod_id, "dex", 0 );
        ch->mod_stat[STAT_CON] = (sh_int)yl_int( doc, mod_id, "con", 0 );
    }

    /* spouse */
    {
        const char *sp = yl_str( doc, player_id, "spouse" );
        if ( sp[0] )
        {
            if ( ch->pcdata->spouse ) free_string( ch->pcdata->spouse );
            ch->pcdata->spouse = str_dup( sp );
        }
    }

    /* player-only fields */
    {
        const char *s;
        #define LOAD_STR(field, key) \
            s = yl_str(doc, player_id, key); \
            if (s[0]) { free_string(ch->pcdata->field); ch->pcdata->field = str_dup(s); }

        LOAD_STR( pwd,    "password" );
        LOAD_STR( bamfin,  "bamfin"  );
        LOAD_STR( bamfout, "bamfout" );
        LOAD_STR( title,   "title"   );
        LOAD_STR( prompt,  "prompt"  );
        LOAD_STR( email,   "email"   );
        LOAD_STR( comment, "comment" );
        #undef LOAD_STR
    }

    ch->pcdata->points      = yl_int( doc, player_id, "points",       0 );
    ch->pcdata->true_sex    = (sh_int)yl_int( doc, player_id, "true_sex",    0 );
    ch->pcdata->last_level  = yl_int( doc, player_id, "last_level",   0 );
    ch->pcdata->incarnations= yl_int( doc, player_id, "incarnations", 1 );
    ch->questpoints         = yl_int( doc, player_id, "quest_points", 0 );
    ch->nextquest           = (sh_int)yl_int( doc, player_id, "quest_next",   0 );
    ch->bonusPoints         = yl_int( doc, player_id, "bonus_points", 0 );

    phmv_id = yl_find( doc, player_id, "perm_hmv" );
    if ( phmv_id )
    {
        ch->pcdata->perm_hit  = (sh_int)yl_int( doc, phmv_id, "hit",  0 );
        ch->pcdata->perm_mana = (sh_int)yl_int( doc, phmv_id, "mana", 0 );
        ch->pcdata->perm_move = (sh_int)yl_int( doc, phmv_id, "move", 0 );
    }

    cond_id = yl_find( doc, player_id, "condition" );
    if ( cond_id )
    {
        ch->pcdata->condition[COND_DRUNK]  = (sh_int)yl_int( doc, cond_id, "drunk",  0 );
        ch->pcdata->condition[COND_FULL]   = (sh_int)yl_int( doc, cond_id, "full",   0 );
        ch->pcdata->condition[COND_THIRST] = (sh_int)yl_int( doc, cond_id, "thirst", 0 );
    }

    /* clan */
    clan_name = yl_str( doc, player_id, "clan" );
    if ( clan_name[0] )
    {
        char clan_buf[MAX_INPUT_LENGTH];
        strncpy( clan_buf, clan_name, sizeof(clan_buf) - 1 );
        clan_buf[sizeof(clan_buf) - 1] = '\0';
        ch->pcdata->clan = (sh_int)get_clan( clan_buf );
    }
    ch->pcdata->clan_leader = yl_int( doc, player_id, "clan_leader", 0 );

    /* recall room */
    {
        int rr = yl_int( doc, player_id, "recall_room", ROOM_VNUM_TEMPLE );
        ch->pcdata->recall_room = get_room_index( rr );
        if ( !ch->pcdata->recall_room )
            ch->pcdata->recall_room = get_room_index( ROOM_VNUM_TEMPLE );
    }

    /* aliases */
    aliases_id = yl_find( doc, player_id, "aliases" );
    j = 0;
    for ( i = 0; i < yl_seq_len( doc, aliases_id ) && j < MAX_ALIAS; i++ )
    {
        int    a_id = yl_seq_item( doc, aliases_id, i );
        const char *al  = yl_str( doc, a_id, "alias" );
        const char *exp = yl_str( doc, a_id, "expansion" );
        if ( !al[0] ) continue;
        ch->pcdata->alias[j]     = str_dup( al  );
        ch->pcdata->alias_sub[j] = str_dup( exp );
        j++;
    }

    /* boards */
    boards_id = yl_find( doc, player_id, "boards" );
    for ( i = 0; i < yl_seq_len( doc, boards_id ); i++ )
    {
        int         b_id = yl_seq_item( doc, boards_id, i );
        const char *bname = yl_str( doc, b_id, "name" );
        long        btime = yl_long( doc, b_id, "last_note", 0L );
        int         k;
        for ( k = 0; k < MAX_BOARD; k++ )
        {
            if ( !str_cmp( boards[k].short_name, bname ) )
            {
                ch->pcdata->last_note[k] = (time_t)btime;
                break;
            }
        }
    }

    /* skills */
    skills_id = yl_find( doc, player_id, "skills" );
    for ( i = 0; i < yl_seq_len( doc, skills_id ); i++ )
    {
        int sk_id = yl_seq_item( doc, skills_id, i );
        const char *sname = yl_str( doc, sk_id, "name" );
        int         lev   = yl_int( doc, sk_id, "level", 0 );
        int         sn;
        if ( !sname[0] || !lev ) continue;
        sn = skill_lookup( sname );
        if ( sn >= 0 )
            ch->pcdata->learned[sn] = (sh_int)lev;
    }

    /* groups */
    groups_id = yl_find( doc, player_id, "groups" );
    for ( i = 0; i < yl_seq_len( doc, groups_id ); i++ )
    {
        int         g_id = yl_seq_item( doc, groups_id, i );
        const char *gname = yl_scalar( doc, g_id );
        if ( gname[0] )
            group_add( ch, gname, FALSE );
    }

    /* affects */
    affects_id = yl_find( doc, player_id, "affects" );
    for ( i = 0; i < yl_seq_len( doc, affects_id ); i++ )
    {
        int aff_id = yl_seq_item( doc, affects_id, i );
        const char *sname;
        int sn;

        if ( affect_free == NULL )
            paf = alloc_perm( sizeof(*paf) );
        else
        {
            paf         = affect_free;
            affect_free = affect_free->next;
        }

        sname = yl_str( doc, aff_id, "name" );
        sn    = skill_lookup( sname );
        if ( sn < 0 )
        {
            bug( "load_yaml_char: unknown affect skill '%s'", (int)sname );
            sn = 0;
        }
        paf->type      = (sh_int)sn;
        paf->where     = (sh_int)yl_int( doc, aff_id, "where",     0 );
        paf->level     = (sh_int)yl_int( doc, aff_id, "level",     ch->level );
        paf->duration  = (sh_int)yl_int( doc, aff_id, "duration",  -1 );
        paf->modifier  = (sh_int)yl_int( doc, aff_id, "modifier",  0 );
        paf->location  = (sh_int)yl_int( doc, aff_id, "location",  0 );
        paf->bitvector = yl_flag( doc, aff_id, "bitvector", 0L );
        paf->next      = ch->affected;
        ch->affected   = paf;
    }
}

/* ------------------------------------------------------------------ */
/* Load a single object entry from the YAML objects sequence           */
/* ------------------------------------------------------------------ */

static void
load_yaml_obj( CHAR_DATA *ch, yaml_document_t *doc, int obj_id )
{
    OBJ_DATA *obj;
    int        vnum  = yl_int( doc, obj_id, "vnum",  0 );
    int        iNest = yl_int( doc, obj_id, "nest",  0 );
    OBJ_INDEX_DATA *pIdx;

    if ( vnum == 0 )
        return;

    pIdx = get_obj_index( vnum );
    if ( pIdx == NULL )
    {
        bug( "load_yaml_obj: bad vnum %d.", vnum );
        return;
    }

    obj = create_object( pIdx, -1 );

    /* apply any overrides */
    {
        const char *s;
        s = yl_str( doc, obj_id, "name"        ); if (s[0]) { free_string(obj->name);        obj->name        = str_dup(s); }
        s = yl_str( doc, obj_id, "short_desc"  ); if (s[0]) { free_string(obj->short_descr); obj->short_descr = str_dup(s); }
        s = yl_str( doc, obj_id, "description" ); if (s[0]) { free_string(obj->description); obj->description = str_dup(s); }
    }
    {
        int vn;
        obj->extra_flags = yl_flag(doc, obj_id, "extra_flags", (long)obj->extra_flags);
        obj->wear_flags  = yl_flag(doc, obj_id, "wear_flags",  (long)obj->wear_flags);
        vn = yl_find( doc, obj_id, "item_type"   ); if (vn) obj->item_type   = atoi(yl_scalar(doc,vn));
        vn = yl_find( doc, obj_id, "weight"      ); if (vn) obj->weight      = atoi(yl_scalar(doc,vn));
    }
    obj->enchanted  = yl_int( doc, obj_id, "enchanted", 0 ) ? TRUE : FALSE;
    obj->wear_loc   = (sh_int)yl_int( doc, obj_id, "wear_loc", WEAR_NONE );
    obj->level      = (sh_int)yl_int( doc, obj_id, "level",  0 );
    obj->timer      = (sh_int)yl_int( doc, obj_id, "timer",  0 );
    obj->cost       = yl_int( doc, obj_id, "cost",   obj->cost );

    /* values */
    {
        int vals_id = yl_find( doc, obj_id, "values" );
        if ( vals_id )
        {
            int j;
            for ( j = 0; j < 5; j++ )
            {
                int item = yl_seq_item( doc, vals_id, j );
                if ( item )
                    obj->value[j] = atoi( yl_scalar( doc, item ) );
            }
        }
    }

    /* spell names in values for potions/wands/etc */
    {
        const char *sp;
        sp = yl_str( doc, obj_id, "spell_1" );
        if (sp[0]) { int sn = skill_lookup(sp); if (sn >= 0) obj->value[1] = sn; }
        sp = yl_str( doc, obj_id, "spell_2" );
        if (sp[0]) { int sn = skill_lookup(sp); if (sn >= 0) obj->value[2] = sn; }
        sp = yl_str( doc, obj_id, "spell_3" );
        if (sp[0]) { int sn = skill_lookup(sp); if (sn >= 0) obj->value[3] = sn; }
    }

    /* affects */
    {
        int aff_seq = yl_find( doc, obj_id, "affects" );
        int k;
        for ( k = 0; k < yl_seq_len( doc, aff_seq ); k++ )
        {
            AFFECT_DATA *paf;
            int          a_id = yl_seq_item( doc, aff_seq, k );
            const char  *sname;
            int          sn;

            if ( affect_free == NULL )
                paf = alloc_perm( sizeof(*paf) );
            else
            {
                paf         = affect_free;
                affect_free = affect_free->next;
            }

            sname = yl_str( doc, a_id, "name" );
            sn    = skill_lookup( sname );
            paf->type      = (sh_int)( sn >= 0 ? sn : -1 );
            paf->where     = (sh_int)yl_int( doc, a_id, "where",     TO_OBJECT );
            paf->level     = (sh_int)yl_int( doc, a_id, "level",     obj->level );
            paf->duration  = (sh_int)yl_int( doc, a_id, "duration",  -1 );
            paf->modifier  = (sh_int)yl_int( doc, a_id, "modifier",  0 );
            paf->location  = (sh_int)yl_int( doc, a_id, "location",  0 );
            paf->bitvector = yl_flag( doc, a_id, "bitvector", 0L );
            paf->next      = obj->affected;
            obj->affected  = paf;
        }
    }

    /* extra descriptions */
    {
        int ed_seq = yl_find( doc, obj_id, "extra_descs" );
        int k;
        for ( k = 0; k < yl_seq_len( doc, ed_seq ); k++ )
        {
            EXTRA_DESCR_DATA *ed;
            int               e_id = yl_seq_item( doc, ed_seq, k );
            if ( extra_descr_free == NULL )
                ed = alloc_perm( sizeof(*ed) );
            else
            {
                ed               = extra_descr_free;
                extra_descr_free = extra_descr_free->next;
            }
            ed->keyword     = str_dup( yl_str( doc, e_id, "keywords"    ) );
            ed->description = str_dup( yl_str( doc, e_id, "description" ) );
            ed->next        = obj->extra_descr;
            obj->extra_descr = ed;
        }
    }

    /* Place object using the nest stack */
    if ( iNest == 0 || rgObjNest[iNest] == NULL )
        obj_to_char( obj, ch );
    else
        obj_to_obj( obj, rgObjNest[iNest] );

    rgObjNest[iNest] = obj;
}

/* ------------------------------------------------------------------ */
/* Public: load_char_obj_yaml                                          */
/* ------------------------------------------------------------------ */

bool
load_char_obj_yaml( DESCRIPTOR_DATA *d, const char *name )
{
    static PC_DATA  pcdata_zero;
    char            strsave[MAX_INPUT_LENGTH];
    CHAR_DATA      *ch;
    FILE           *fp;
    yaml_parser_t   parser;
    yaml_document_t doc;
    yaml_node_t    *root;
    int             player_id;
    int             objects_id;
    int             pets_id;
    int             i;
    bool            found = FALSE;
    int             stat;

    /* ------ allocate character (mirrors load_char_obj) ------------ */
    if ( char_free == NULL )
        ch = alloc_perm( sizeof(*ch) );
    else
    {
        ch        = char_free;
        char_free = char_free->next;
    }
    clear_char( ch );

    if ( pcdata_free == NULL )
        ch->pcdata = alloc_perm( sizeof(*ch->pcdata) );
    else
    {
        ch->pcdata  = pcdata_free;
        pcdata_free = pcdata_free->next;
    }
    *ch->pcdata = pcdata_zero;

    d->character = ch;
    ch->desc     = d;
    ch->name     = str_dup( name );
    ch->version  = 0;
    ch->race     = race_lookup( "human" );
    ch->affected2_by |= race_table[ch->race].aff2;
    ch->act  = PLR_NOSUMMON | PLR_AUTOSAC | PLR_AUTOLOOT
             | PLR_AUTOGOLD | PLR_AUTOEXIT;
    ch->comm = COMM_COMBINE | COMM_PROMPT;
    ch->pcdata->board               = &boards[DEFAULT_BOARD];
    ch->invis_level                 = 0;
    ch->incog_level                 = 0;
    ch->practice                    = 0;
    ch->train                       = 0;
    ch->hitroll                     = 0;
    ch->damroll                     = 0;
    ch->trust                       = 0;
    ch->wimpy                       = 0;
    ch->saving_throw                = 0;
    ch->pcdata->points              = 0;
    ch->pcdata->confirm_delete      = FALSE;
    ch->pcdata->pwd                 = str_dup( "" );
    ch->pcdata->bamfin              = str_dup( "" );
    ch->pcdata->bamfout             = str_dup( "" );
    ch->pcdata->title               = str_dup( "" );
    for ( stat = 0; stat < MAX_STATS; stat++ )
        ch->perm_stat[stat] = 13;
    ch->pcdata->perm_hit            = 0;
    ch->pcdata->perm_mana           = 0;
    ch->pcdata->perm_move           = 0;
    ch->pcdata->true_sex            = 0;
    ch->pcdata->last_level          = 0;
    ch->pcdata->condition[COND_THIRST] = 48;
    ch->pcdata->condition[COND_FULL]   = 48;
    ch->pcdata->security            = 0;
    ch->pcdata->prompt              = str_dup( "%i`K/`W%H`w HP %n`K/`W%M`w MP %w`K/`W%V`w MV `K> " );
    ch->pcdata->clan                = 0;
    ch->questpoints                 = 0;
    ch->nextquest                   = 0;
    ch->bank                        = 0;
    ch->pcdata->bounty              = 0;
    ch->pcdata->recall_room         = get_room_index( ROOM_VNUM_TEMPLE );
    ch->pcdata->email               = strdup( "(none)" );
    ch->pcdata->comment             = strdup( "(none)" );
    ch->pcdata->away_message        = NULL;
    ch->pcdata->buffer              = buffer_new( 1000 );
    ch->pcdata->buffer->data[0]     = '\0';
    ch->pcdata->message_ctr         = 0;
    ch->pcdata->kills               = 0;
    ch->pcdata->killed              = 0;
    ch->pcdata->pkills              = 0;
    ch->pcdata->pkilled             = 0;
    ch->pcdata->incarnations        = 1;
    ch->pcdata->clan_leader         = 0;
    ch->bonusPoints                 = 0;

    /* ------ open and parse YAML file ------------------------------ */
    yaml_player_path( name, strsave, sizeof(strsave) );

    fclose( fpReserve );
    fp = fopen( strsave, "r" );
    if ( !fp )
    {
        fpReserve = fopen( NULL_FILE, "r" );
        return FALSE;   /* new character */
    }
    fclose( fp );   /* reopen via yaml_parser_set_input_file */

    memset( &parser, 0, sizeof(parser) );
    memset( &doc,    0, sizeof(doc)    );

    fp = fopen( strsave, "r" );
    if ( !yaml_parser_initialize( &parser )
    ||   !fp )
    {
        bug( "load_char_obj_yaml: parser init failed for %s", (int)strsave );
        if (fp) fclose(fp);
        fpReserve = fopen( NULL_FILE, "r" );
        return FALSE;
    }

    yaml_parser_set_input_file( &parser, fp );
    if ( !yaml_parser_load( &parser, &doc ) )
    {
        bug( "load_char_obj_yaml: YAML parse error in %s", (int)strsave );
        yaml_parser_delete( &parser );
        fclose( fp );
        fpReserve = fopen( NULL_FILE, "r" );
        return FALSE;
    }
    fclose( fp );

    root = yaml_document_get_root_node( &doc );
    if ( !root || root->type != YAML_MAPPING_NODE )
    {
        bug( "load_char_obj_yaml: root not a mapping in %s", (int)strsave );
        yaml_document_delete( &doc );
        yaml_parser_delete( &parser );
        fpReserve = fopen( NULL_FILE, "r" );
        return FALSE;
    }

    player_id  = yl_find( &doc, 1, "player"  );
    objects_id = yl_find( &doc, 1, "objects" );
    pets_id    = yl_find( &doc, 1, "pets"    );

    if ( player_id )
    {
        /* init nest stack */
        {
            int k;
            for ( k = 0; k < MAX_NEST; k++ )
                rgObjNest[k] = NULL;
        }
        load_yaml_char( ch, &doc, player_id );
        found = TRUE;

        /* objects */
        for ( i = 0; i < yl_seq_len( &doc, objects_id ); i++ )
        {
            int obj_id = yl_seq_item( &doc, objects_id, i );
            if ( obj_id )
                load_yaml_obj( ch, &doc, obj_id );
        }

        /* pets */
        for ( i = 0; i < yl_seq_len( &doc, pets_id ); i++ )
        {
            int         pet_node   = yl_seq_item( &doc, pets_id, i );
            int         pet_vnum   = yl_int( &doc, pet_node, "vnum", 0 );
            MOB_INDEX_DATA *pMob;
            CHAR_DATA   *pet;
            AFFECT_DATA *paf;
            int          j;

            if ( !pet_vnum )
                continue;

            pMob = get_mob_index( pet_vnum );
            if ( !pMob )
                continue;

            pet              = create_mobile( pMob );
            SET_BIT( pet->act, ACT_PET );

            {
                const char *s;
                s = yl_str( &doc, pet_node, "name" );
                if (s[0]) { free_string(pet->name); pet->name = str_dup(s); }
                s = yl_str( &doc, pet_node, "short_desc" );
                if (s[0]) { free_string(pet->short_descr); pet->short_descr = str_dup(s); }
                s = yl_str( &doc, pet_node, "long_desc" );
                if (s[0]) { free_string(pet->long_descr); pet->long_descr = str_dup(s); }
            }
            pet->level = (sh_int)yl_int( &doc, pet_node, "level", pet->level );
            pet->sex   = (sh_int)yl_int( &doc, pet_node, "sex",   pet->sex   );

            {
                int aff_seq = yl_find( &doc, pet_node, "affects" );
                for ( j = 0; j < yl_seq_len( &doc, aff_seq ); j++ )
                {
                    int a_id = yl_seq_item( &doc, aff_seq, j );
                    const char *sname;
                    int sn;

                    if ( affect_free == NULL )
                        paf = alloc_perm( sizeof(*paf) );
                    else
                    {
                        paf         = affect_free;
                        affect_free = affect_free->next;
                    }
                    sname = yl_str( &doc, a_id, "name" );
                    sn    = skill_lookup( sname );
                    paf->type      = (sh_int)( sn >= 0 ? sn : -1 );
                    paf->where     = (sh_int)yl_int(&doc, a_id, "where",    0);
                    paf->level     = (sh_int)yl_int(&doc, a_id, "level",    pet->level);
                    paf->duration  = (sh_int)yl_int(&doc, a_id, "duration", -1);
                    paf->modifier  = (sh_int)yl_int(&doc, a_id, "modifier", 0);
                    paf->location  = (sh_int)yl_int(&doc, a_id, "location", 0);
                    paf->bitvector = yl_flag(&doc, a_id, "bitvector", 0L);
                    paf->next      = pet->affected;
                    pet->affected  = paf;
                }
            }

            char_to_room( pet, ch->in_room );
            pet->master = ch;
            pet->leader = ch;
            ch->pet     = pet;
        }
    }

    yaml_document_delete( &doc );
    yaml_parser_delete( &parser );
    fpReserve = fopen( NULL_FILE, "r" );

    /* post-load fixups (same as load_char_obj) */
    if ( found )
    {
        int k;

        if ( ch->race == 0 )
            ch->race = race_lookup( "human" );

        ch->size     = pc_race_table[ch->race].size;
        ch->dam_type = 17; /* punch */

        for ( k = 0; k < 5; k++ )
        {
            if ( pc_race_table[ch->race].skills[k] == NULL )
                break;
            group_add( ch, pc_race_table[ch->race].skills[k], FALSE );
        }
        ch->affected_by |= race_table[ch->race].aff;
        ch->imm_flags   |= race_table[ch->race].imm;
        ch->res_flags   |= race_table[ch->race].res;
        ch->vuln_flags  |= race_table[ch->race].vuln;
        ch->form         = race_table[ch->race].form;
        ch->parts        = race_table[ch->race].parts;

        /* version fixups (same thresholds as load_char_obj) */
        if ( ch->version < 2 )
        {
            group_add( ch, "rom basics", FALSE );
            group_add( ch, class_table[ch->ch_class].base_group,    FALSE );
            group_add( ch, class_table[ch->ch_class].default_group, TRUE  );
            ch->pcdata->learned[gsn_recall] = 50;
        }
        if ( ch->version < 3 && ( ch->level > 35 || ch->trust > 35 ) )
        {
            switch ( ch->level )
            {
                case 40: ch->level = 60; break;
                case 39: ch->level = 58; break;
                case 38: ch->level = 56; break;
                case 37: ch->level = 53; break;
            }
            switch ( ch->trust )
            {
                case 40: ch->trust = 60; break;
                case 39: ch->trust = 58; break;
                case 38: ch->trust = 56; break;
                case 37: ch->trust = 53; break;
                case 36: ch->trust = 51; break;
            }
        }
    }

    return found;
}
