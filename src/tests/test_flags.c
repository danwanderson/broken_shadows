/*
 * test_flags.c — Unit tests for flag_convert() and print_flags().
 *
 * Covers:
 *   flag_convert  — letter  → bitmask  (A=1, B=2 … Z=2^25, a=2^26 … z=2^51)
 *   print_flags   — bitmask → letter string (inverse; '0' for zero input)
 *   round-trip    — print_flags(x) → re-parse via flag_convert → x
 *
 * No game-server dependencies: links only flags_util_t.o (zero external
 * symbols) and stubs.o (satisfies the linker for merc.h globals).
 */

#include <sys/types.h>
#include <string.h>
#include <form.h>
#include "merc.h"
#include "../flags_util.h"
#include "test_framework.h"

/* ------------------------------------------------------------------ */
/*  flag_convert                                                        */
/* ------------------------------------------------------------------ */

static void test_flag_convert_A(void)
{
    ASSERT_INT_EQ((int)flag_convert('A'), 1);       /* 2^0 */
}

static void test_flag_convert_B(void)
{
    ASSERT_INT_EQ((int)flag_convert('B'), 2);       /* 2^1 */
}

static void test_flag_convert_C(void)
{
    ASSERT_INT_EQ((int)flag_convert('C'), 4);       /* 2^2 */
}

static void test_flag_convert_Z(void)
{
    ASSERT_INT_EQ((int)flag_convert('Z'), 1 << 25); /* 2^25 */
}

static void test_flag_convert_a(void)
{
    /* 'a' is the first lowercase letter = 2^26 = 67108864 */
    ASSERT_INT_EQ((int)flag_convert('a'), 67108864);
}

static void test_flag_convert_b(void)
{
    ASSERT_INT_EQ((int)flag_convert('b'), 67108864 * 2); /* 2^27 */
}

static void test_flag_convert_non_letter(void)
{
    /* Non-letter characters return 0 */
    ASSERT_INT_EQ((int)flag_convert('0'), 0);
    ASSERT_INT_EQ((int)flag_convert(' '), 0);
    ASSERT_INT_EQ((int)flag_convert('!'), 0);
}

/* ------------------------------------------------------------------ */
/*  print_flags                                                         */
/* ------------------------------------------------------------------ */

static void test_print_flags_zero(void)
{
    ASSERT_STR_EQ(print_flags(0), "0");
}

static void test_print_flags_bit0(void)
{
    /* bit 0 → 'A' */
    ASSERT_STR_EQ(print_flags(1), "A");
}

static void test_print_flags_bit1(void)
{
    /* bit 1 → 'B' */
    ASSERT_STR_EQ(print_flags(2), "B");
}

static void test_print_flags_bit25(void)
{
    /* bit 25 → 'Z' */
    ASSERT_STR_EQ(print_flags(1 << 25), "Z");
}

static void test_print_flags_bit26(void)
{
    /* bit 26 → 'a' */
    ASSERT_STR_EQ(print_flags(67108864), "a");
}

static void test_print_flags_multiple_bits(void)
{
    /* bits 0 + 2 → "AC" */
    const char *s = print_flags(1 | 4);
    ASSERT_STR_EQ(s, "AC");
}

static void test_print_flags_three_bits(void)
{
    /* bits 0 + 1 + 3 → "ABD" */
    const char *s = print_flags(1 | 2 | 8);
    ASSERT_STR_EQ(s, "ABD");
}

/* ------------------------------------------------------------------ */
/*  Round-trip: print_flags → flag_convert → original value           */
/* ------------------------------------------------------------------ */

static void test_roundtrip_single_bit(void)
{
    int original = 1 << 3;   /* bit 3 = 'D' */
    const char *s = print_flags(original);
    long reconstructed = 0;
    const char *p;
    for (p = s; *p; p++)
        reconstructed += flag_convert(*p);
    ASSERT_INT_EQ((int)reconstructed, original);
}

static void test_roundtrip_multi_bit(void)
{
    /* bits 0,4,7 = A,E,H */
    int original = (1 << 0) | (1 << 4) | (1 << 7);
    const char *s = print_flags(original);
    long reconstructed = 0;
    const char *p;
    for (p = s; *p; p++)
        reconstructed += flag_convert(*p);
    ASSERT_INT_EQ((int)reconstructed, original);
}

static void test_roundtrip_lowercase_bit(void)
{
    /* bit 26 = 'a' = 67108864 */
    int original = 67108864;
    const char *s = print_flags(original);
    long reconstructed = 0;
    const char *p;
    for (p = s; *p; p++)
        reconstructed += flag_convert(*p);
    ASSERT_INT_EQ((int)reconstructed, original);
}

static void test_roundtrip_mixed_case_bits(void)
{
    /* bits 25 + 26 = 'Z' + 'a' */
    int original = (1 << 25) | (1 << 26);
    const char *s = print_flags(original);
    long reconstructed = 0;
    const char *p;
    for (p = s; *p; p++)
        reconstructed += flag_convert(*p);
    ASSERT_INT_EQ((int)reconstructed, original);
}

static void test_roundtrip_plr_bounty_hunter(void)
{
    /* PLR_BOUNTY_HUNTER = 2^26 = flag_convert('a') — used in clans */
    int original = 67108864;
    const char *s = print_flags(original);
    ASSERT_STR_EQ(s, "a");
    ASSERT_INT_EQ((int)flag_convert(s[0]), original);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* flag_convert */
    test_flag_convert_A();
    test_flag_convert_B();
    test_flag_convert_C();
    test_flag_convert_Z();
    test_flag_convert_a();
    test_flag_convert_b();
    test_flag_convert_non_letter();

    /* print_flags */
    test_print_flags_zero();
    test_print_flags_bit0();
    test_print_flags_bit1();
    test_print_flags_bit25();
    test_print_flags_bit26();
    test_print_flags_multiple_bits();
    test_print_flags_three_bits();

    /* round-trip */
    test_roundtrip_single_bit();
    test_roundtrip_multi_bit();
    test_roundtrip_lowercase_bit();
    test_roundtrip_mixed_case_bits();
    test_roundtrip_plr_bounty_hunter();

    TEST_SUITE_END();
}
