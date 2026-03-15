/*
 * stubs.c — Minimal stub implementations of external dependencies needed
 * to link the individual test binaries without the full game server.
 *
 * Each test binary links against only the specific modules being tested
 * (e.g. buffer.o, bit.o, handler.o) plus this file.  Stubs here satisfy
 * the linker for every symbol those modules reference that is NOT provided
 * by the standard library.
 *
 * The global flag-type tables (area_flags, sex_flags …) must exactly match
 * the symbols declared as extern in merc.h and pointed to by bit.c's
 * internal flag_stat_table[].  Stub tables that appear in flag_stat_table
 * only need correct pointer identity; their contents are irrelevant unless
 * a specific test exercises a named entry.
 *
 * Thread-safety: not a concern — tests are single-threaded.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <form.h>
#include "merc.h"

/* ================================================================== */
/*  db.c globals                                                        */
/* ================================================================== */

/*
 * Memory allocation size classes — must match the values in db.c so that
 * buffer.c's find_mem_size() and the EMEM_SIZE sentinel both work correctly.
 */
const int rgSizeList[MAX_MEM_LIST] = {
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32704
};

/* Other db/recycle globals referenced transitively. */
CHAR_DATA  *char_list      = NULL;
int         top_area       = 0;
int         top_room       = 0;
int         top_exit       = 0;
int         top_reset      = 0;
int         top_ed         = 0;
char        str_empty[1]   = { '\0' };  /* declared extern in recycle.h */
int         mobile_count   = 0;         /* declared extern in recycle.h */

/* Clan globals — needed by yaml_area.c's load_clans_yaml / save_clans_yaml. */
CLAN_DATA  *clan_first     = NULL;
CLAN_DATA  *clan_last      = NULL;
int         top_clan       = 0;

/*
 * alloc_perm — permanent (never-freed) allocator used throughout db.c.
 * In tests we just use malloc; memory is reclaimed when the process exits.
 */
void *alloc_perm( int size )
{
    void *p = malloc( (size_t)size );
    if ( p )
        memset( p, 0, (size_t)size );
    return p;
}

/* ================================================================== */
/*  comm.c / handler.c stubs                                            */
/*                                                                      */
/*  bugf and log_stringf are defined in handler.c, not comm.c, in this  */
/*  codebase.  The definitions here are marked __attribute__((weak)) so  */
/*  that handler_t.o's strong definitions take precedence when           */
/*  test_handler links against it, while test_buffer and test_bit still  */
/*  get a working implementation.                                        */
/* ================================================================== */

void __attribute__((weak)) bugf(char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "[BUGF] ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void __attribute__((weak)) log_stringf(char *fmt, ...)
{
    (void)fmt; /* no-op in tests */
}

void bug(const char *str, int param)
{
    fprintf(stderr, "[BUG] ");
    fprintf(stderr, str, param);
    fprintf(stderr, "\n");
}

void log_string(const char *str)
{
    (void)str; /* no-op in tests */
}

void send_to_char(const char *txt, CHAR_DATA *ch)
{
    (void)txt;
    (void)ch;
}

void act(const char *format, CHAR_DATA *ch,
         const void *arg1, const void *arg2, int type)
{
    (void)format; (void)ch; (void)arg1; (void)arg2; (void)type;
}

/* ================================================================== */
/*  string.c stubs — pure string utilities                              */
/* ================================================================== */

/*
 * Case-insensitive string comparison.
 * Returns FALSE (0) if equal, TRUE (non-zero) if different —
 * matching the original semantics in string.c.
 */
bool str_cmp(const char *astr, const char *bstr)
{
    if (!astr || !bstr)
        return TRUE;
    while (*astr && *bstr)
    {
        if (tolower((unsigned char)*astr) != tolower((unsigned char)*bstr))
            return TRUE;
        astr++;
        bstr++;
    }
    return *astr != *bstr;
}

/*
 * Case-insensitive prefix check.
 * Returns FALSE (0) if astr is a prefix of bstr, TRUE otherwise.
 */
bool str_prefix(const char *astr, const char *bstr)
{
    if (!astr || !bstr)
        return TRUE;
    while (*astr)
    {
        if (tolower((unsigned char)*astr) != tolower((unsigned char)*bstr))
            return TRUE;
        astr++;
        bstr++;
    }
    return FALSE;
}

/*
 * Extract one whitespace-delimited word from argument into arg_first.
 * Returns pointer to the remainder of argument.
 * Mirrors the behaviour of one_argument() in string.c.
 */
char *one_argument(char *argument, char *arg_first)
{
    char cEnd = ' ';

    while (*argument == ' ')
        argument++;

    if (*argument == '\'' || *argument == '"')
        cEnd = *argument++;

    while (*argument != '\0')
    {
        if (*argument == cEnd)
        {
            argument++;
            break;
        }
        *arg_first++ = (char)tolower((unsigned char)*argument++);
    }
    *arg_first = '\0';

    while (*argument == ' ')
        argument++;

    return argument;
}

char *str_dup(const char *str)
{
    if (!str || str[0] == '\0')
    {
        char *empty = malloc(1);
        if (empty) empty[0] = '\0';
        return empty;
    }
    return strdup(str);
}

void free_string(char *str)
{
    free(str);
}

/*
 * NOTE: All const struct flag_type tables (area_flags, sex_flags,
 * material_type, imm_flags, etc.) are defined in bit.c — they are not
 * stubbed here.  Test binaries that exercise these tables must link
 * bit_t.o (compiled from bit.c).
 */

/* ================================================================== */
/*  Lookup tables — used by handler.c table-lookup functions           */
/* ================================================================== */

/*
 * Minimal race_table with known entries for race_lookup tests.
 * Designated initialisers zero all unspecified fields.
 * The NULL name sentinel terminates race_lookup's iteration.
 *
 * struct race_type: name, pc_race, act, aff, aff2, off, imm, res,
 *                   vuln, form, parts, remort_race
 */
const struct race_type race_table[] =
{
    { .name = "unique", .pc_race = FALSE },
    { .name = "human",  .pc_race = TRUE  },
    { .name = "elf",    .pc_race = TRUE  },
    { .name = "dwarf",  .pc_race = TRUE  },
    { .name = NULL }
};

/*
 * pc_race_table — only needs to compile; not directly exercised.
 */
const struct pc_race_type pc_race_table[] =
{
    { .name = NULL }
};

/*
 * class_table must have exactly MAX_CLASS (5) entries.
 * class_lookup checks .name[0] and str_prefix; other fields are zeroed.
 *
 * struct class_type: name, who_name[4], attr_prime, weapon, guild[2],
 *                    skill_adept, thac0_00, thac0_32, hp_min, hp_max,
 *                    fMana, base_group, default_group, remort_class
 */
const struct class_type class_table[MAX_CLASS] =
{
    { .name = "mage",    .who_name = "Mag" },
    { .name = "cleric",  .who_name = "Cle" },
    { .name = "thief",   .who_name = "Thi" },
    { .name = "warrior", .who_name = "War" },
    { .name = "paladin", .who_name = "Pal" },
};

/*
 * weapon_table — NULL name sentinel ends weapon_lookup's iteration.
 * gsn pointers are NULL (skill numbers not needed for lookup tests).
 *
 * struct weapon_type: name, vnum (sh_int), type (sh_int), gsn (sh_int*)
 */
const struct weapon_type weapon_table[] =
{
    { .name = "sword",  .type = WEAPON_SWORD  },
    { .name = "mace",   .type = WEAPON_MACE   },
    { .name = "dagger", .type = WEAPON_DAGGER },
    { .name = NULL }
};

/*
 * wiznet_table — NULL name sentinel ends wiznet_lookup's iteration.
 * wiznet_lookup returns the index into this table, not the flag value.
 *
 * struct wiznet_type: name, flag (long), level (int)
 */
const struct wiznet_type wiznet_table[] =
{
    { .name = "on",     .flag = WIZ_ON      },
    { .name = "logins", .flag = WIZ_LOGINS  },
    { .name = "ticks",  .flag = WIZ_TICKS   },
    { .name = NULL }
};
