/*
 * test_bit.c — Unit tests for bit.c flag manipulation functions.
 *
 * Covers:
 *   is_stat      — identifies stat vs. toggle tables
 *   flag_lookup  — find bit value by name
 *   flag_string  — convert bit mask to name string
 *   flag_value   — parse name(s) to bit mask (toggle and stat modes)
 *
 * Test tables are defined locally so these tests have no dependency on
 * const.c's large game tables.  The only global tables referenced are
 * area_flags and sex_flags (from stubs.c), used to exercise is_stat()
 * via bit.c's internal flag_stat_table[].
 *
 * Dependencies satisfied by stubs.c:
 *   str_cmp, one_argument, all flag-type tables in flag_stat_table
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <form.h>
#include "merc.h"
#include "test_framework.h"

/*
 * Forward declarations for bit.c functions — not declared in merc.h.
 * Defined in bit.c; resolved at link time via bit_t.o.
 */
bool    is_stat     ( const struct flag_type *flag_table );
int     flag_lookup ( const char *name, const struct flag_type *flag_table );
int     flag_value  ( const struct flag_type *flag_table, char *argument );
char   *flag_string ( const struct flag_type *flag_table, int bits );

/* ------------------------------------------------------------------ */
/*  Local test flag tables                                              */
/* ------------------------------------------------------------------ */

/*
 * A small toggle (non-stat) flag table — NOT in bit.c's flag_stat_table,
 * so is_stat() returns FALSE for it.  Used to test flag_lookup,
 * flag_string, and flag_value in multi-flag (toggle) mode.
 */
static const struct flag_type fire_flags[] =
{
    { "fire",    A, TRUE  },   /* A = 1 */
    { "cold",    B, TRUE  },   /* B = 2 */
    { "acid",    C, TRUE  },   /* C = 4 */
    { "hidden",  D, FALSE },   /* D = 8, not settable */
    { "",        0, FALSE }    /* sentinel */
};

/*
 * A stat table — also NOT in flag_stat_table, so is_stat() returns FALSE
 * for it too.  Flag_value stat-mode is tested via sex_flags (which IS in
 * flag_stat_table with stat = TRUE).
 */
static const struct flag_type size_test_flags[] =
{
    { "small",   1, TRUE  },
    { "medium",  2, TRUE  },
    { "large",   3, TRUE  },
    { "",        0, FALSE }
};

/* ------------------------------------------------------------------ */
/*  Externs pointing to stubs defined in stubs.c                       */
/*  These are the SAME pointers stored in bit.c's flag_stat_table[],  */
/*  so is_stat() can recognise them.                                   */
/* ------------------------------------------------------------------ */
extern const struct flag_type area_flags[];   /* stat = FALSE */
extern const struct flag_type sex_flags[];    /* stat = TRUE  */

/* ------------------------------------------------------------------ */
/*  is_stat                                                             */
/* ------------------------------------------------------------------ */

static void test_is_stat_toggle_table(void)
{
    /* area_flags is in flag_stat_table with stat = FALSE. */
    ASSERT_INT_EQ(is_stat(area_flags), FALSE);
}

static void test_is_stat_stat_table(void)
{
    /* sex_flags is in flag_stat_table with stat = TRUE. */
    ASSERT_INT_EQ(is_stat(sex_flags), TRUE);
}

static void test_is_stat_unknown_table(void)
{
    /* fire_flags is not in flag_stat_table at all → FALSE. */
    ASSERT_INT_EQ(is_stat(fire_flags), FALSE);
}

/* ------------------------------------------------------------------ */
/*  flag_lookup                                                         */
/* ------------------------------------------------------------------ */

static void test_flag_lookup_found_first(void)
{
    ASSERT_INT_EQ(flag_lookup("fire", fire_flags), A);
}

static void test_flag_lookup_found_middle(void)
{
    ASSERT_INT_EQ(flag_lookup("acid", fire_flags), C);
}

static void test_flag_lookup_not_found(void)
{
    ASSERT_INT_EQ(flag_lookup("lightning", fire_flags), NO_FLAG);
}

static void test_flag_lookup_not_settable_returns_no_flag(void)
{
    /* "hidden" exists but settable = FALSE, so lookup returns NO_FLAG. */
    ASSERT_INT_EQ(flag_lookup("hidden", fire_flags), NO_FLAG);
}

static void test_flag_lookup_case_insensitive(void)
{
    /* str_cmp is case-insensitive; "FIRE" should match "fire". */
    ASSERT_INT_EQ(flag_lookup("FIRE", fire_flags), A);
    ASSERT_INT_EQ(flag_lookup("Cold", fire_flags), B);
}

static void test_flag_lookup_empty_name(void)
{
    ASSERT_INT_EQ(flag_lookup("", fire_flags), NO_FLAG);
}

/* ------------------------------------------------------------------ */
/*  flag_string                                                         */
/* ------------------------------------------------------------------ */

static void test_flag_string_single_bit(void)
{
    /* For a toggle table, flag_string returns space-separated names. */
    ASSERT_STR_EQ(flag_string(fire_flags, A), "fire");
}

static void test_flag_string_multiple_bits(void)
{
    const char *result = flag_string(fire_flags, A | B);
    /* Both "fire" and "cold" should appear (order matches table order). */
    ASSERT(strstr(result, "fire") != NULL);
    ASSERT(strstr(result, "cold") != NULL);
}

static void test_flag_string_no_bits_set(void)
{
    /* No matching bits → returns "none". */
    ASSERT_STR_EQ(flag_string(fire_flags, 0), "none");
}

static void test_flag_string_unknown_bits(void)
{
    /* Bits not in the table → returns "none". */
    ASSERT_STR_EQ(flag_string(fire_flags, 256), "none");
}

static void test_flag_string_stat_table_single_value(void)
{
    /*
     * For a stat table (sex_flags, stat = TRUE in flag_stat_table),
     * flag_string looks for an exact bit match rather than OR-ing flags.
     * bit.c defines: male = SEX_MALE = 1, female = SEX_FEMALE = 2.
     */
    ASSERT_STR_EQ(flag_string(sex_flags, SEX_MALE),   "male");
    ASSERT_STR_EQ(flag_string(sex_flags, SEX_FEMALE), "female");
}

/* ------------------------------------------------------------------ */
/*  flag_value                                                          */
/* ------------------------------------------------------------------ */

static void test_flag_value_single_toggle(void)
{
    /* Toggle table, single name → its bit value. */
    char arg[] = "fire";
    ASSERT_INT_EQ(flag_value(fire_flags, arg), A);
}

static void test_flag_value_multiple_toggles(void)
{
    /* Multiple space-separated names → OR of their bits. */
    char arg[] = "fire cold";
    ASSERT_INT_EQ(flag_value(fire_flags, arg), A | B);
}

static void test_flag_value_triple_toggles(void)
{
    char arg[] = "fire cold acid";
    ASSERT_INT_EQ(flag_value(fire_flags, arg), A | B | C);
}

static void test_flag_value_not_found_toggle(void)
{
    char arg[] = "unknown";
    ASSERT_INT_EQ(flag_value(fire_flags, arg), NO_FLAG);
}

static void test_flag_value_stat_single(void)
{
    /*
     * sex_flags is in flag_stat_table with stat = TRUE.
     * In stat mode flag_value accepts only one word and returns its bit.
     * bit.c defines: male = SEX_MALE = 1.
     */
    char arg[] = "male";
    ASSERT_INT_EQ(flag_value(sex_flags, arg), SEX_MALE);  /* SEX_MALE = 1 */
}

static void test_flag_value_stat_not_found(void)
{
    char arg[] = "dragon";
    ASSERT_INT_EQ(flag_value(sex_flags, arg), NO_FLAG);
}

static void test_flag_value_not_settable_returns_no_flag(void)
{
    /* "hidden" has settable = FALSE; flag_lookup returns NO_FLAG for it. */
    char arg[] = "hidden";
    ASSERT_INT_EQ(flag_value(fire_flags, arg), NO_FLAG);
}

/* ------------------------------------------------------------------ */
/*  Edge cases                                                          */
/* ------------------------------------------------------------------ */

static void test_flag_string_all_three_flags(void)
{
    const char *result = flag_string(fire_flags, A | B | C);
    ASSERT(strstr(result, "fire") != NULL);
    ASSERT(strstr(result, "cold") != NULL);
    ASSERT(strstr(result, "acid") != NULL);
    /* "hidden" is not settable but IS in the table; flag_string should
     * still include it when its bit is set (flag_string has no settable
     * filter — that filter only applies in flag_lookup). */
}

static void test_flag_lookup_size_test_table(void)
{
    /* Verify locally-defined table works independently. */
    ASSERT_INT_EQ(flag_lookup("small",  size_test_flags), 1);
    ASSERT_INT_EQ(flag_lookup("medium", size_test_flags), 2);
    ASSERT_INT_EQ(flag_lookup("large",  size_test_flags), 3);
    ASSERT_INT_EQ(flag_lookup("huge",   size_test_flags), NO_FLAG);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* is_stat */
    test_is_stat_toggle_table();
    test_is_stat_stat_table();
    test_is_stat_unknown_table();

    /* flag_lookup */
    test_flag_lookup_found_first();
    test_flag_lookup_found_middle();
    test_flag_lookup_not_found();
    test_flag_lookup_not_settable_returns_no_flag();
    test_flag_lookup_case_insensitive();
    test_flag_lookup_empty_name();

    /* flag_string */
    test_flag_string_single_bit();
    test_flag_string_multiple_bits();
    test_flag_string_no_bits_set();
    test_flag_string_unknown_bits();
    test_flag_string_stat_table_single_value();

    /* flag_value */
    test_flag_value_single_toggle();
    test_flag_value_multiple_toggles();
    test_flag_value_triple_toggles();
    test_flag_value_not_found_toggle();
    test_flag_value_stat_single();
    test_flag_value_stat_not_found();
    test_flag_value_not_settable_returns_no_flag();

    /* edge cases */
    test_flag_string_all_three_flags();
    test_flag_lookup_size_test_table();

    TEST_SUITE_END();
}
