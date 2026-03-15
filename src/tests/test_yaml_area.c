/*
 * test_yaml_area.c — Unit tests for YAML area file loading.
 *
 * Covers two bugs fixed in this codebase:
 *
 *   1. Mob dice loading — dice were stored as YAML sequences [num, type, bonus]
 *      but the loader was calling doc_int() treating them as mapping keys,
 *      so all dice came back as 0 (causing every mob to have 0 HP).
 *      Fixed by adding doc_seq_int() and loading by sequence index.
 *
 *   2. Area/mob bitmask flag loading — area_flags and mob flag fields
 *      (act, off_flags, etc.) are now written as letter strings ("AB")
 *      by the saver.  The loader must accept both integer ("7") and
 *      letter-string ("AB") formats.
 *
 * Strategy:
 *   1. Write a known minimal YAML area to a temp file.
 *   2. Call load_yaml_area_file() and verify the populated data structures.
 *   3. Use a second YAML file with integer area_flags to verify backward compat.
 *
 * Links: yaml_area_t.o + flags_util_t.o + stubs.o + stubs_area.o + -lyaml
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <form.h>
#include "merc.h"
#include "db.h"
#include "../yaml_area.h"
#include "test_framework.h"

/* Globals from stubs_area.c */
extern AREA_DATA        *area_first;
extern AREA_DATA        *area_last;
extern char              strArea[MAX_INPUT_LENGTH];
extern MOB_INDEX_DATA   *mob_index_hash[MAX_KEY_HASH];
extern OBJ_INDEX_DATA   *obj_index_hash[MAX_KEY_HASH];
extern ROOM_INDEX_DATA  *room_index_hash[MAX_KEY_HASH];
extern int               top_area;
extern int               top_mob_index;

/* Functions from stubs_area.c */
extern MOB_INDEX_DATA  *get_mob_index( int vnum );

/* ------------------------------------------------------------------ */
/*  Test YAML file paths                                                */
/* ------------------------------------------------------------------ */

static const char *AREA_YAML_1 = "/tmp/test_area1.yaml";
static const char *AREA_YAML_2 = "/tmp/test_area2.yaml";

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

static void reset_area_state( void )
{
    int i;
    area_first    = NULL;
    area_last     = NULL;
    top_area      = 0;
    top_mob_index = 0;
    for ( i = 0; i < MAX_KEY_HASH; i++ )
    {
        mob_index_hash[i]  = NULL;
        obj_index_hash[i]  = NULL;
        room_index_hash[i] = NULL;
    }
}

/*
 * Area 1: area_flags as letter string "AB" (= 3), one mob with:
 *   hit:    [3, 8, 15]   — number=3, type=8,  bonus=15
 *   mana:   [2, 6, 5]    — number=2, type=6,  bonus=5
 *   damage: [1, 4, 2]    — number=1, type=4,  bonus=2
 *   off_flags: "A"       — = 1 (letter string)
 */
static void write_area_yaml_1( const char *path )
{
    FILE *fp = fopen( path, "w" );
    if ( !fp ) { fprintf( stderr, "Cannot write %s\n", path ); exit(1); }
    fprintf( fp,
        "area:\n"
        "  name: Test Area\n"
        "  builders: Rahl\n"
        "  security: 9\n"
        "  vnums: [1, 100]\n"
        "  area_flags: \"AB\"\n"
        "\n"
        "mobiles:\n"
        "  - vnum: 1\n"
        "    keywords: test mob\n"
        "    short_desc: a test mob\n"
        "    long_desc: A test mob is here.\n"
        "    description: A test mob.\n"
        "    race: human\n"
        "    act: \"0\"\n"
        "    affected_by: \"0\"\n"
        "    affected2_by: \"0\"\n"
        "    alignment: 0\n"
        "    level: 10\n"
        "    hitroll: 5\n"
        "    hit: [3, 8, 15]\n"
        "    mana: [2, 6, 5]\n"
        "    damage: [1, 4, 2]\n"
        "    dam_type: 0\n"
        "    ac:\n"
        "      pierce: -10\n"
        "      bash: -10\n"
        "      slash: -10\n"
        "      exotic: -5\n"
        "    off_flags: \"A\"\n"
        "    imm_flags: \"0\"\n"
        "    res_flags: \"0\"\n"
        "    vuln_flags: \"0\"\n"
        "    start_pos: 8\n"
        "    default_pos: 8\n"
        "    sex: 0\n"
        "    gold: 0\n"
        "    form: \"0\"\n"
        "    parts: \"0\"\n"
        "    size: M\n"
    );
    fclose( fp );
}

/*
 * Area 2: area_flags as plain integer 7 — backward compat test.
 * No mobs needed; vnums chosen not to collide with area 1.
 */
static void write_area_yaml_2( const char *path )
{
    FILE *fp = fopen( path, "w" );
    if ( !fp ) { fprintf( stderr, "Cannot write %s\n", path ); exit(1); }
    fprintf( fp,
        "area:\n"
        "  name: Test Area 2\n"
        "  builders: Rahl\n"
        "  security: 9\n"
        "  vnums: [200, 300]\n"
        "  area_flags: 7\n"
    );
    fclose( fp );
}

/* ------------------------------------------------------------------ */
/*  Load tests — area 1                                                 */
/* ------------------------------------------------------------------ */

static void test_load_returns_true( void )
{
    strncpy( strArea, AREA_YAML_1, MAX_INPUT_LENGTH - 1 );
    ASSERT( load_yaml_area_file( AREA_YAML_1 ) == TRUE );
}

static void test_load_area_count( void )
{
    ASSERT_INT_EQ( top_area, 1 );
}

static void test_load_area_name( void )
{
    ASSERT_NOTNULL( area_first );
    ASSERT_STR_EQ( area_first->name, "Test Area" );
}

/* ------------------------------------------------------------------ */
/*  area_flags: letter-string format                                    */
/* ------------------------------------------------------------------ */

static void test_area_flags_letter_AB( void )
{
    /* "AB" → flag_convert('A') + flag_convert('B') = 1 + 2 = 3 */
    ASSERT_NOTNULL( area_first );
    ASSERT_INT_EQ( area_first->area_flags, 3 );
}

/* ------------------------------------------------------------------ */
/*  Mob dice loading                                                    */
/* ------------------------------------------------------------------ */

static void test_mob_loaded( void )
{
    ASSERT_INT_EQ( top_mob_index, 1 );
    ASSERT_NOTNULL( get_mob_index( 1 ) );
}

static void test_mob_hit_dice_number( void )
{
    MOB_INDEX_DATA *m = get_mob_index( 1 );
    ASSERT_NOTNULL( m );
    ASSERT_INT_EQ( m->hit[DICE_NUMBER], 3 );
}

static void test_mob_hit_dice_type( void )
{
    MOB_INDEX_DATA *m = get_mob_index( 1 );
    ASSERT_NOTNULL( m );
    ASSERT_INT_EQ( m->hit[DICE_TYPE], 8 );
}

static void test_mob_hit_dice_bonus( void )
{
    MOB_INDEX_DATA *m = get_mob_index( 1 );
    ASSERT_NOTNULL( m );
    ASSERT_INT_EQ( m->hit[DICE_BONUS], 15 );
}

static void test_mob_mana_dice( void )
{
    MOB_INDEX_DATA *m = get_mob_index( 1 );
    ASSERT_NOTNULL( m );
    ASSERT_INT_EQ( m->mana[DICE_NUMBER], 2 );
    ASSERT_INT_EQ( m->mana[DICE_TYPE],   6 );
    ASSERT_INT_EQ( m->mana[DICE_BONUS],  5 );
}

static void test_mob_damage_dice( void )
{
    MOB_INDEX_DATA *m = get_mob_index( 1 );
    ASSERT_NOTNULL( m );
    ASSERT_INT_EQ( m->damage[DICE_NUMBER], 1 );
    ASSERT_INT_EQ( m->damage[DICE_TYPE],   4 );
    ASSERT_INT_EQ( m->damage[DICE_BONUS],  2 );
}

/* ------------------------------------------------------------------ */
/*  Mob bitmask flag: letter-string format                              */
/* ------------------------------------------------------------------ */

static void test_mob_off_flags_letter_A( void )
{
    /* off_flags: "A" → flag_convert('A') = 1, OR'd with race_table[0].off = 0 */
    MOB_INDEX_DATA *m = get_mob_index( 1 );
    ASSERT_NOTNULL( m );
    ASSERT_INT_EQ( (int)m->off_flags, 1 );
}

static void test_mob_is_npc( void )
{
    /* act field must always have ACT_IS_NPC set regardless of YAML value */
    MOB_INDEX_DATA *m = get_mob_index( 1 );
    ASSERT_NOTNULL( m );
    ASSERT( IS_SET( m->act, ACT_IS_NPC ) );
}

/* ------------------------------------------------------------------ */
/*  area_flags: integer format (backward compat)                       */
/* ------------------------------------------------------------------ */

static void test_area_flags_integer( void )
{
    /* area_flags: 7 (plain integer) → 7 */
    reset_area_state();
    strncpy( strArea, AREA_YAML_2, MAX_INPUT_LENGTH - 1 );
    ASSERT( load_yaml_area_file( AREA_YAML_2 ) == TRUE );
    ASSERT_NOTNULL( area_first );
    ASSERT_INT_EQ( area_first->area_flags, 7 );
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main( void )
{
    write_area_yaml_1( AREA_YAML_1 );
    write_area_yaml_2( AREA_YAML_2 );

    reset_area_state();

    /* load area 1 */
    test_load_returns_true();
    test_load_area_count();
    test_load_area_name();

    /* area_flags letter string */
    test_area_flags_letter_AB();

    /* mob dice */
    test_mob_loaded();
    test_mob_hit_dice_number();
    test_mob_hit_dice_type();
    test_mob_hit_dice_bonus();
    test_mob_mana_dice();
    test_mob_damage_dice();

    /* mob flag letter string */
    test_mob_off_flags_letter_A();
    test_mob_is_npc();

    /* area_flags integer backward compat (resets state internally) */
    test_area_flags_integer();

    /* cleanup */
    unlink( AREA_YAML_1 );
    unlink( AREA_YAML_2 );

    TEST_SUITE_END();
}
