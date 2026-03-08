/*
 * test_handler.c — Unit tests for selected handler.c utility functions.
 *
 * Covers:
 *   get_date        — format a time_t as "mm/dd/yy"
 *   get_time        — format a time_t as "hh:mm"
 *   material_lookup — find material bit value by name
 *   material_name   — find material name by bit value
 *   race_lookup     — find race index by name prefix
 *   class_lookup    — find class index by name prefix
 *   weapon_lookup   — find weapon-type index by name prefix
 *   wiznet_lookup   — find wiznet-table index by name prefix
 *   check_immune    — immunity / resistance / vulnerability logic
 *
 * Dependencies provided by stubs.c:
 *   material_type[], race_table[], class_table[], weapon_table[],
 *   wiznet_table[], str_cmp, str_prefix
 *
 * NOTE: get_date/get_time both write to a SHARED static buffer inside
 * handler.c ("thedate" / "thetime").  Each test must complete its
 * assert before the next call overwrites the buffer.
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <form.h>
#include "merc.h"
#include "test_framework.h"

/* ------------------------------------------------------------------ */
/*  Helpers                                                             */
/* ------------------------------------------------------------------ */

/*
 * Build a UTC time_t for a known date/time so the tests are deterministic.
 * Using UTC avoids timezone-dependent localtime() shifts.
 *
 * Note: handler.c's get_date / get_time use localtime().  On most systems
 * the CI/test environment will be set to UTC, making the round-trip exact.
 * If your local timezone differs, the hour/date assertions may be off.
 * The struct fields are: year - 1900, month (0-based), mday, hour, min.
 */
static time_t make_time(int year, int mon, int mday, int hour, int min)
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    t.tm_year  = year - 1900;
    t.tm_mon   = mon - 1;   /* 0-based */
    t.tm_mday  = mday;
    t.tm_hour  = hour;
    t.tm_min   = min;
    t.tm_isdst = -1;
    return mktime(&t);
}

/* ------------------------------------------------------------------ */
/*  get_date                                                            */
/* ------------------------------------------------------------------ */

static void test_get_date_format(void)
{
    /*
     * get_date formats as "mm/dd/yy" where yy = tm_year (years since 1900).
     * For 2024-06-15: mon=6, mday=15, year offset=124  → "06/15/124"
     *
     * This is the existing behaviour (intentional two-digit shorthand that
     * actually shows years-since-1900 not two-digit year).
     */
    time_t t = make_time(2024, 6, 15, 12, 0);
    char *result = get_date(t);
    ASSERT_NOTNULL(result);
    ASSERT_STR_EQ(result, "06/15/124");
}

static void test_get_date_zero_padded_month(void)
{
    /* January should be zero-padded: "01/…" */
    time_t t = make_time(2025, 1, 3, 0, 0);
    char *result = get_date(t);
    ASSERT(result[0] == '0');
    ASSERT(result[1] == '1');
}

static void test_get_date_december(void)
{
    time_t t = make_time(2025, 12, 31, 23, 59);
    char *result = get_date(t);
    ASSERT(result[0] == '1');
    ASSERT(result[1] == '2');
}

/* ------------------------------------------------------------------ */
/*  get_time                                                            */
/* ------------------------------------------------------------------ */

static void test_get_time_format(void)
{
    /* get_time formats as "hh:mm" in 24-hour time. */
    time_t t = make_time(2025, 3, 8, 14, 30);
    char *result = get_time(t);
    ASSERT_NOTNULL(result);
    ASSERT_INT_EQ((int)strlen(result), 5);  /* "14:30" = 5 chars */
    ASSERT(result[2] == ':');
}

static void test_get_time_midnight(void)
{
    time_t t = make_time(2025, 3, 8, 0, 0);
    char *result = get_time(t);
    ASSERT_STR_EQ(result, "00:00");
}

static void test_get_time_noon(void)
{
    time_t t = make_time(2025, 3, 8, 12, 0);
    char *result = get_time(t);
    ASSERT_STR_EQ(result, "12:00");
}

static void test_get_time_zero_padded_hour(void)
{
    time_t t = make_time(2025, 3, 8, 9, 5);
    char *result = get_time(t);
    ASSERT(result[0] == '0');  /* hour < 10 is zero-padded */
    ASSERT(result[1] == '9');
    ASSERT(result[3] == '0');  /* minute < 10 is zero-padded */
    ASSERT(result[4] == '5');
}

/* ------------------------------------------------------------------ */
/*  material_lookup / material_name                                     */
/* ------------------------------------------------------------------ */
/*
 * stubs.c defines material_type[] as:
 *   { "none", 0, TRUE }, { "iron", A=1, … }, { "silver", B=2, … },
 *   { "gold",  C=4, … }, { "", 0, FALSE }
 */

static void test_material_lookup_known(void)
{
    /*
     * Real values from bit.c's material_type table (defined in merc.h):
     *   MAT_IRON = A = 1, MAT_SILVER = G = 64, MAT_GOLD = F = 32
     */
    ASSERT_INT_EQ((int)material_lookup("iron"),   MAT_IRON);    /* A = 1  */
    ASSERT_INT_EQ((int)material_lookup("silver"), MAT_SILVER);  /* G = 64 */
    ASSERT_INT_EQ((int)material_lookup("gold"),   MAT_GOLD);    /* F = 32 */
}

static void test_material_lookup_none_entry(void)
{
    /* "none" has bit 0 — should be found and return 0. */
    ASSERT_INT_EQ((int)material_lookup("none"), 0);
}

static void test_material_lookup_unknown(void)
{
    /* Unknown material returns 0 (not-found sentinel). */
    ASSERT_INT_EQ((int)material_lookup("mithril"), 0);
}

static void test_material_name_known(void)
{
    ASSERT_STR_EQ(material_name(MAT_IRON),   "iron");
    ASSERT_STR_EQ(material_name(MAT_SILVER), "silver");
    ASSERT_STR_EQ(material_name(MAT_GOLD),   "gold");
}

static void test_material_name_unknown_bit(void)
{
    /* bit not in table → "unknown" */
    ASSERT_STR_EQ(material_name(999), "unknown");
}

/* ------------------------------------------------------------------ */
/*  race_lookup                                                         */
/* ------------------------------------------------------------------ */
/*
 * stubs.c defines race_table[] with: unique(0), human(1), elf(2), dwarf(3),
 * then { NULL } sentinel.
 */

static void test_race_lookup_exact(void)
{
    ASSERT_INT_EQ(race_lookup("human"), 1);
    ASSERT_INT_EQ(race_lookup("elf"),   2);
    ASSERT_INT_EQ(race_lookup("dwarf"), 3);
}

static void test_race_lookup_prefix(void)
{
    /* race_lookup uses str_prefix — "dw" matches "dwarf". */
    ASSERT_INT_EQ(race_lookup("dw"), 3);
    ASSERT_INT_EQ(race_lookup("el"), 2);
}

static void test_race_lookup_case_insensitive(void)
{
    ASSERT_INT_EQ(race_lookup("Human"), 1);
    ASSERT_INT_EQ(race_lookup("ELF"),   2);
}

static void test_race_lookup_not_found(void)
{
    /* Unknown race returns 0 (index of "unique" — the default). */
    ASSERT_INT_EQ(race_lookup("orc"), 0);
}

/* ------------------------------------------------------------------ */
/*  class_lookup                                                        */
/* ------------------------------------------------------------------ */
/*
 * stubs.c defines class_table[5]:
 *   0=mage, 1=cleric, 2=thief, 3=warrior, 4=paladin
 */

static void test_class_lookup_exact(void)
{
    ASSERT_INT_EQ(class_lookup("mage"),    0);
    ASSERT_INT_EQ(class_lookup("warrior"), 3);
    ASSERT_INT_EQ(class_lookup("paladin"), 4);
}

static void test_class_lookup_prefix(void)
{
    ASSERT_INT_EQ(class_lookup("war"), 3);
    ASSERT_INT_EQ(class_lookup("pal"), 4);
    ASSERT_INT_EQ(class_lookup("cl"),  1);
}

static void test_class_lookup_not_found(void)
{
    /* Returns -1 when no match. */
    ASSERT_INT_EQ(class_lookup("necromancer"), -1);
}

/* ------------------------------------------------------------------ */
/*  weapon_lookup                                                       */
/* ------------------------------------------------------------------ */
/*
 * stubs.c defines weapon_table[]:
 *   0=sword, 1=mace, 2=dagger, then { NULL }
 */

static void test_weapon_lookup_exact(void)
{
    ASSERT_INT_EQ(weapon_lookup("sword"),  0);
    ASSERT_INT_EQ(weapon_lookup("mace"),   1);
    ASSERT_INT_EQ(weapon_lookup("dagger"), 2);
}

static void test_weapon_lookup_prefix(void)
{
    ASSERT_INT_EQ(weapon_lookup("sw"),  0);
    ASSERT_INT_EQ(weapon_lookup("dag"), 2);
}

static void test_weapon_lookup_not_found(void)
{
    /* Returns -1 when no match. */
    ASSERT_INT_EQ(weapon_lookup("polearm"), -1);
}

/* ------------------------------------------------------------------ */
/*  wiznet_lookup                                                       */
/* ------------------------------------------------------------------ */
/*
 * stubs.c defines wiznet_table[]:
 *   0=on, 1=logins, 2=ticks, then { NULL }
 */

static void test_wiznet_lookup_exact(void)
{
    ASSERT_INT_EQ((int)wiznet_lookup("on"),     0);
    ASSERT_INT_EQ((int)wiznet_lookup("logins"), 1);
    ASSERT_INT_EQ((int)wiznet_lookup("ticks"),  2);
}

static void test_wiznet_lookup_prefix(void)
{
    ASSERT_INT_EQ((int)wiznet_lookup("log"), 1);
    ASSERT_INT_EQ((int)wiznet_lookup("ti"),  2);
}

static void test_wiznet_lookup_not_found(void)
{
    ASSERT_INT_EQ((int)wiznet_lookup("newbies"), -1);
}

/* ------------------------------------------------------------------ */
/*  check_immune                                                        */
/* ------------------------------------------------------------------ */
/*
 * check_immune(ch, dam_type):
 *   DAM_NONE (0)          → always IS_NORMAL
 *   DAM_BASH/PIERCE/SLASH → physical path (IMM/RES/VULN_WEAPON general,
 *                           then IMM/RES/VULN_BASH etc. specific)
 *   DAM_FIRE and others   → magic path   (IMM/RES/VULN_MAGIC general,
 *                           then specific fire/cold/etc.)
 *   Specific flag overrides the general flag (applied last).
 *   DAM_OTHER / unknown   → early return with whatever general set.
 */

/* Convenience: build a zeroed CHAR_DATA then set specific flag fields. */
static CHAR_DATA make_char_imm(long imm, long res, long vuln)
{
    CHAR_DATA ch;
    memset(&ch, 0, sizeof(ch));
    ch.imm_flags  = imm;
    ch.res_flags  = res;
    ch.vuln_flags = vuln;
    return ch;
}

static void test_check_immune_dam_none(void)
{
    CHAR_DATA ch = make_char_imm(0, 0, 0);
    ASSERT_INT_EQ(check_immune(&ch, DAM_NONE), IS_NORMAL);
}

static void test_check_immune_no_flags_fire(void)
{
    CHAR_DATA ch = make_char_imm(0, 0, 0);
    ASSERT_INT_EQ(check_immune(&ch, DAM_FIRE), IS_NORMAL);
}

static void test_check_immune_specific_imm_fire(void)
{
    CHAR_DATA ch = make_char_imm(IMM_FIRE, 0, 0);
    ASSERT_INT_EQ(check_immune(&ch, DAM_FIRE), IS_IMMUNE);
}

static void test_check_immune_specific_res_fire(void)
{
    CHAR_DATA ch = make_char_imm(0, RES_FIRE, 0);
    ASSERT_INT_EQ(check_immune(&ch, DAM_FIRE), IS_RESISTANT);
}

static void test_check_immune_specific_vuln_fire(void)
{
    CHAR_DATA ch = make_char_imm(0, 0, VULN_FIRE);
    ASSERT_INT_EQ(check_immune(&ch, DAM_FIRE), IS_VULNERABLE);
}

static void test_check_immune_global_magic_no_specific(void)
{
    /* IMM_MAGIC applies to all magical attacks unless overridden. */
    CHAR_DATA ch = make_char_imm(IMM_MAGIC, 0, 0);
    ASSERT_INT_EQ(check_immune(&ch, DAM_FIRE),      IS_IMMUNE);
    ASSERT_INT_EQ(check_immune(&ch, DAM_COLD),      IS_IMMUNE);
    ASSERT_INT_EQ(check_immune(&ch, DAM_LIGHTNING),  IS_IMMUNE);
}

static void test_check_immune_specific_overrides_general(void)
{
    /*
     * IMM_MAGIC grants immunity, but VULN_FIRE is applied after and
     * overrides, leaving the character vulnerable to fire specifically.
     */
    CHAR_DATA ch = make_char_imm(IMM_MAGIC, 0, VULN_FIRE);
    ASSERT_INT_EQ(check_immune(&ch, DAM_FIRE), IS_VULNERABLE);
    /* Other magical damage still hits the general immunity. */
    ASSERT_INT_EQ(check_immune(&ch, DAM_COLD), IS_IMMUNE);
}

static void test_check_immune_physical_weapon_immunity(void)
{
    /* IMM_WEAPON covers all physical (bash/pierce/slash) damage. */
    CHAR_DATA ch = make_char_imm(IMM_WEAPON, 0, 0);
    ASSERT_INT_EQ(check_immune(&ch, DAM_BASH),   IS_IMMUNE);
    ASSERT_INT_EQ(check_immune(&ch, DAM_PIERCE),  IS_IMMUNE);
    ASSERT_INT_EQ(check_immune(&ch, DAM_SLASH),   IS_IMMUNE);
    /* But weapon immunity does NOT affect magical damage. */
    ASSERT_INT_EQ(check_immune(&ch, DAM_FIRE), IS_NORMAL);
}

static void test_check_immune_res_weapon_physical(void)
{
    CHAR_DATA ch = make_char_imm(0, RES_WEAPON, 0);
    ASSERT_INT_EQ(check_immune(&ch, DAM_BASH),  IS_RESISTANT);
    ASSERT_INT_EQ(check_immune(&ch, DAM_SLASH),  IS_RESISTANT);
}

static void test_check_immune_specific_bash_overrides_weapon(void)
{
    /*
     * IMM_WEAPON grants immunity to weapons, but IMM_BASH is also set.
     * Since the specific flag is applied last it keeps IS_IMMUNE — no
     * visible change here — the important case is when specific downgrades:
     */
    CHAR_DATA ch = make_char_imm(IMM_WEAPON, 0, VULN_BASH);
    /* General: IS_IMMUNE (weapon). Specific VULN_BASH overrides → VULN. */
    ASSERT_INT_EQ(check_immune(&ch, DAM_BASH), IS_VULNERABLE);
    /* PIERCE has no specific override — stays immune from weapon. */
    ASSERT_INT_EQ(check_immune(&ch, DAM_PIERCE), IS_IMMUNE);
}

static void test_check_immune_unknown_dam_type_uses_general(void)
{
    /*
     * DAM_OTHER (16) hits the switch default and returns early, preserving
     * whatever the general magic check set.
     */
    CHAR_DATA ch_imm  = make_char_imm(IMM_MAGIC,  0, 0);
    CHAR_DATA ch_none = make_char_imm(0, 0, 0);

    ASSERT_INT_EQ(check_immune(&ch_imm,  DAM_OTHER), IS_IMMUNE);
    ASSERT_INT_EQ(check_immune(&ch_none, DAM_OTHER), IS_NORMAL);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* get_date */
    test_get_date_format();
    test_get_date_zero_padded_month();
    test_get_date_december();

    /* get_time */
    test_get_time_format();
    test_get_time_midnight();
    test_get_time_noon();
    test_get_time_zero_padded_hour();

    /* material_lookup / material_name */
    test_material_lookup_known();
    test_material_lookup_none_entry();
    test_material_lookup_unknown();
    test_material_name_known();
    test_material_name_unknown_bit();

    /* race_lookup */
    test_race_lookup_exact();
    test_race_lookup_prefix();
    test_race_lookup_case_insensitive();
    test_race_lookup_not_found();

    /* class_lookup */
    test_class_lookup_exact();
    test_class_lookup_prefix();
    test_class_lookup_not_found();

    /* weapon_lookup */
    test_weapon_lookup_exact();
    test_weapon_lookup_prefix();
    test_weapon_lookup_not_found();

    /* wiznet_lookup */
    test_wiznet_lookup_exact();
    test_wiznet_lookup_prefix();
    test_wiznet_lookup_not_found();

    /* check_immune */
    test_check_immune_dam_none();
    test_check_immune_no_flags_fire();
    test_check_immune_specific_imm_fire();
    test_check_immune_specific_res_fire();
    test_check_immune_specific_vuln_fire();
    test_check_immune_global_magic_no_specific();
    test_check_immune_specific_overrides_general();
    test_check_immune_physical_weapon_immunity();
    test_check_immune_res_weapon_physical();
    test_check_immune_specific_bash_overrides_weapon();
    test_check_immune_unknown_dam_type_uses_general();

    TEST_SUITE_END();
}
