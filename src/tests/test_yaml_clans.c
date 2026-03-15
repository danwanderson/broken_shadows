/*
 * test_yaml_clans.c — Unit tests for YAML clan loading and saving.
 *
 * Covers:
 *   load_clans_yaml  — parses clans.yaml, populates clan linked list
 *   save_clans_yaml  — serialises clan list to clans.yaml
 *   doc_bool         — boolean "true"/"false" parsing (exercised via load)
 *   round-trip       — save then reload, verify data survives intact
 *
 * Strategy:
 *   1. Write a known YAML to a temp file.
 *   2. Call load_clans_yaml() and verify the in-memory CLAN_DATA structs.
 *   3. Call save_clans_yaml() and verify the output file exists and is
 *      non-empty.
 *   4. Reload the saved file and verify the data round-tripped correctly.
 *
 * Links: yaml_area_t.o + flags_util_t.o + stubs.o + -lyaml
 * All area/mob/room functions in yaml_area.c are unreachable from main()
 * and are pruned by --gc-sections, so no stubs are needed for them.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <form.h>
#include "merc.h"
#include "../yaml_area.h"
#include "test_framework.h"

/* Clan globals defined in stubs.c */
extern CLAN_DATA *clan_first;
extern CLAN_DATA *clan_last;
extern int        top_clan;

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

static void reset_clans(void)
{
    /* Clear the linked list between test passes.
     * alloc_perm memory is not freed (permanent allocator), but for tests
     * that's fine — the process is short-lived. */
    clan_first = NULL;
    clan_last  = NULL;
    top_clan   = 0;
}

static const char *TEST_YAML_PATH  = "/tmp/test_clans_input.yaml";
static const char *SAVE_YAML_PATH  = "clans.yaml";   /* save_clans_yaml CWD */

static void write_test_yaml(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp) { fprintf(stderr, "Cannot write %s\n", path); exit(1); }
    fprintf(fp,
        "version: 1\n"
        "clans:\n"
        "  - number: 1\n"
        "    name: rose\n"
        "    visible: \"`KBlack Rose`w\"\n"
        "    god: Rahl\n"
        "    max_members: 30\n"
        "    min_level: 10\n"
        "    num_members: 13\n"
        "    req_flags: \"0\"\n"
        "    cost_gold: 0\n"
        "    cost_xp: 0\n"
        "    auto_accept: true\n"
        "    classes:\n"
        "      mage: true\n"
        "      cleric: true\n"
        "      thief: false\n"
        "      warrior: true\n"
        "      paladin: false\n"
        "    races:\n"
        "      null_race: true\n"
        "      dwarf: true\n"
        "      elf: false\n"
        "      giant: true\n"
        "      hobbit: false\n"
        "      human: true\n"
        "      troll: false\n"
        "      drow: true\n"
        "      gnome: false\n"
        "      half_elf: true\n"
        "      wyvern: false\n"
        "      vampire: true\n"
        "\n"
        "  - number: 2\n"
        "    name: nightfall\n"
        "    visible: \"`BNightfall`w\"\n"
        "    god: none\n"
        "    max_members: 30\n"
        "    min_level: 20\n"
        "    num_members: 6\n"
        "    req_flags: \"a\"\n"
        "    cost_gold: 100\n"
        "    cost_xp: 500\n"
        "    auto_accept: false\n"
        "    classes:\n"
        "      mage: true\n"
        "      cleric: true\n"
        "      thief: true\n"
        "      warrior: true\n"
        "      paladin: true\n"
        "    races:\n"
        "      null_race: true\n"
        "      dwarf: true\n"
        "      elf: true\n"
        "      giant: true\n"
        "      hobbit: false\n"
        "      human: true\n"
        "      troll: false\n"
        "      drow: true\n"
        "      gnome: true\n"
        "      half_elf: true\n"
        "      wyvern: true\n"
        "      vampire: true\n"
    );
    fclose(fp);
}

/* ------------------------------------------------------------------ */
/*  load tests                                                          */
/* ------------------------------------------------------------------ */

static void test_load_returns_true(void)
{
    reset_clans();
    ASSERT(load_clans_yaml(TEST_YAML_PATH) == TRUE);
}

static void test_load_count(void)
{
    ASSERT_INT_EQ(top_clan, 2);
}

static void test_load_clan1_basic(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    ASSERT_INT_EQ(c->number, 1);
    ASSERT_STR_EQ(c->name, "rose");
    ASSERT_STR_EQ(c->god,  "Rahl");
    ASSERT_INT_EQ(c->max_members, 30);
    ASSERT_INT_EQ(c->min_level,   10);
    ASSERT_INT_EQ(c->num_members, 13);
}

static void test_load_clan1_req_flags_zero(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    ASSERT_INT_EQ((int)c->req_flags, 0);
}

static void test_load_clan1_auto_accept_true(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    ASSERT(c->auto_accept == TRUE);
}

static void test_load_clan1_classes(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    /* mage=true, cleric=true, thief=false, warrior=true, paladin=false */
    ASSERT(c->classes[0] == TRUE);   /* mage    */
    ASSERT(c->classes[1] == TRUE);   /* cleric  */
    ASSERT(c->classes[2] == FALSE);  /* thief   */
    ASSERT(c->classes[3] == TRUE);   /* warrior */
    ASSERT(c->classes[4] == FALSE);  /* paladin */
}

static void test_load_clan1_races(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    /* null_race=T dwarf=T elf=F giant=T hobbit=F human=T
       troll=F drow=T gnome=F half_elf=T wyvern=F vampire=T */
    ASSERT(c->races[0]  == TRUE);    /* null_race */
    ASSERT(c->races[1]  == TRUE);    /* dwarf     */
    ASSERT(c->races[2]  == FALSE);   /* elf       */
    ASSERT(c->races[3]  == TRUE);    /* giant     */
    ASSERT(c->races[4]  == FALSE);   /* hobbit    */
    ASSERT(c->races[5]  == TRUE);    /* human     */
    ASSERT(c->races[6]  == FALSE);   /* troll     */
    ASSERT(c->races[7]  == TRUE);    /* drow      */
    ASSERT(c->races[8]  == FALSE);   /* gnome     */
    ASSERT(c->races[9]  == TRUE);    /* half_elf  */
    ASSERT(c->races[10] == FALSE);   /* wyvern    */
    ASSERT(c->races[11] == TRUE);    /* vampire   */
}

static void test_load_clan2_basic(void)
{
    CLAN_DATA *c = clan_first ? clan_first->next : NULL;
    ASSERT_NOTNULL(c);
    ASSERT_INT_EQ(c->number, 2);
    ASSERT_STR_EQ(c->name, "nightfall");
    ASSERT_INT_EQ(c->min_level, 20);
    ASSERT_INT_EQ((int)c->cost_gold, 100);
    ASSERT_INT_EQ((int)c->cost_xp,   500);
}

static void test_load_clan2_req_flags_letter_a(void)
{
    /* "a" = flag_convert('a') = 2^26 = 67108864 */
    CLAN_DATA *c = clan_first ? clan_first->next : NULL;
    ASSERT_NOTNULL(c);
    ASSERT_INT_EQ((int)c->req_flags, 67108864);
}

static void test_load_clan2_auto_accept_false(void)
{
    CLAN_DATA *c = clan_first ? clan_first->next : NULL;
    ASSERT_NOTNULL(c);
    ASSERT(c->auto_accept == FALSE);
}

static void test_load_clan2_linked_list_end(void)
{
    CLAN_DATA *c = clan_first ? clan_first->next : NULL;
    ASSERT_NOTNULL(c);
    ASSERT(c->next == NULL);
    ASSERT(clan_last == c);
}

/* ------------------------------------------------------------------ */
/*  doc_bool — exercised through loading                               */
/* ------------------------------------------------------------------ */

static void test_doc_bool_true_string(void)
{
    /* auto_accept: true → TRUE */
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    ASSERT_INT_EQ(c->auto_accept, TRUE);
}

static void test_doc_bool_false_string(void)
{
    /* auto_accept: false → FALSE */
    CLAN_DATA *c = clan_first ? clan_first->next : NULL;
    ASSERT_NOTNULL(c);
    ASSERT_INT_EQ(c->auto_accept, FALSE);
}

/* ------------------------------------------------------------------ */
/*  save tests                                                          */
/* ------------------------------------------------------------------ */

static void test_save_returns_true(void)
{
    unlink(SAVE_YAML_PATH);  /* start clean */
    ASSERT(save_clans_yaml() == TRUE);
}

static void test_save_file_exists(void)
{
    struct stat st;
    ASSERT(stat(SAVE_YAML_PATH, &st) == 0);
    ASSERT(st.st_size > 0);
}

/* ------------------------------------------------------------------ */
/*  round-trip: save then reload                                       */
/* ------------------------------------------------------------------ */

static void test_roundtrip_reload(void)
{
    /* Load from the file that save_clans_yaml() just wrote. */
    reset_clans();
    ASSERT(load_clans_yaml(SAVE_YAML_PATH) == TRUE);
    ASSERT_INT_EQ(top_clan, 2);
}

static void test_roundtrip_clan1_name(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    ASSERT_STR_EQ(c->name, "rose");
}

static void test_roundtrip_clan2_req_flags(void)
{
    CLAN_DATA *c = clan_first ? clan_first->next : NULL;
    ASSERT_NOTNULL(c);
    ASSERT_INT_EQ((int)c->req_flags, 67108864);
}

static void test_roundtrip_clan1_classes(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    ASSERT(c->classes[0] == TRUE);
    ASSERT(c->classes[2] == FALSE);
}

static void test_roundtrip_clan1_races(void)
{
    CLAN_DATA *c = clan_first;
    ASSERT_NOTNULL(c);
    ASSERT(c->races[2]  == FALSE);  /* elf   */
    ASSERT(c->races[5]  == TRUE);   /* human */
    ASSERT(c->races[11] == TRUE);   /* vampire */
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* Prepare the input file once for all load tests */
    write_test_yaml(TEST_YAML_PATH);

    /* load */
    test_load_returns_true();
    test_load_count();
    test_load_clan1_basic();
    test_load_clan1_req_flags_zero();
    test_load_clan1_auto_accept_true();
    test_load_clan1_classes();
    test_load_clan1_races();
    test_load_clan2_basic();
    test_load_clan2_req_flags_letter_a();
    test_load_clan2_auto_accept_false();
    test_load_clan2_linked_list_end();

    /* doc_bool */
    test_doc_bool_true_string();
    test_doc_bool_false_string();

    /* save (uses the clans loaded above) */
    test_save_returns_true();
    test_save_file_exists();

    /* round-trip */
    test_roundtrip_reload();
    test_roundtrip_clan1_name();
    test_roundtrip_clan2_req_flags();
    test_roundtrip_clan1_classes();
    test_roundtrip_clan1_races();

    /* cleanup */
    unlink(TEST_YAML_PATH);
    unlink(SAVE_YAML_PATH);

    TEST_SUITE_END();
}
