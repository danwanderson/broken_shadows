/*
 * test_framework.h — Minimal unit test framework for Broken Shadows MUD.
 *
 * Usage:
 *   1. Include this header in a test .c file.
 *   2. Write tests using ASSERT_*, ASSERT_INT_EQ, ASSERT_STR_EQ, ASSERT_NOTNULL.
 *   3. End main() with TEST_SUITE_END().
 *
 * Each test binary is compiled and linked independently; each binary exits
 * with 0 on full pass, 1 on any failure.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Per-binary counters — one copy per translation unit. */
static int _tests_run    = 0;
static int _tests_failed = 0;

/* ------------------------------------------------------------------ */
/*  Core assertion macros                                               */
/* ------------------------------------------------------------------ */

#define ASSERT(cond)                                                        \
    do {                                                                    \
        _tests_run++;                                                       \
        if (!(cond)) {                                                      \
            fprintf(stderr, "FAIL  [%s:%d] %s\n",                          \
                    __FILE__, __LINE__, #cond);                             \
            _tests_failed++;                                                \
        } else {                                                            \
            printf("pass  [%s:%d] %s\n", __FILE__, __LINE__, #cond);       \
        }                                                                   \
    } while (0)

#define ASSERT_INT_EQ(a, b)                                                 \
    do {                                                                    \
        _tests_run++;                                                       \
        int _a = (int)(a), _b = (int)(b);                                  \
        if (_a != _b) {                                                     \
            fprintf(stderr, "FAIL  [%s:%d] %s == %s  (got %d, want %d)\n", \
                    __FILE__, __LINE__, #a, #b, _a, _b);                   \
            _tests_failed++;                                                \
        } else {                                                            \
            printf("pass  [%s:%d] %s == %d\n",                             \
                   __FILE__, __LINE__, #a, _a);                             \
        }                                                                   \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                                 \
    do {                                                                    \
        _tests_run++;                                                       \
        const char *_a = (a), *_b = (b);                                   \
        if (!_a || !_b || strcmp(_a, _b) != 0) {                           \
            fprintf(stderr, "FAIL  [%s:%d] \"%s\" != \"%s\"\n",            \
                    __FILE__, __LINE__,                                     \
                    _a ? _a : "(null)", _b ? _b : "(null)");                \
            _tests_failed++;                                                \
        } else {                                                            \
            printf("pass  [%s:%d] \"%s\"\n", __FILE__, __LINE__, _a);      \
        }                                                                   \
    } while (0)

#define ASSERT_NOTNULL(p)                                                   \
    do {                                                                    \
        _tests_run++;                                                       \
        if ((void *)(p) == NULL) {                                          \
            fprintf(stderr, "FAIL  [%s:%d] %s is NULL\n",                  \
                    __FILE__, __LINE__, #p);                                \
            _tests_failed++;                                                \
        } else {                                                            \
            printf("pass  [%s:%d] %s != NULL\n", __FILE__, __LINE__, #p);  \
        }                                                                   \
    } while (0)

/* ------------------------------------------------------------------ */
/*  End a test binary's main():  print summary and return exit code.   */
/* ------------------------------------------------------------------ */

#define TEST_SUITE_END()                                                    \
    do {                                                                    \
        int _passed = _tests_run - _tests_failed;                          \
        printf("\n--- %s: %d/%d passed",                                   \
               __FILE__, _passed, _tests_run);                             \
        if (_tests_failed > 0)                                             \
            printf(", %d FAILED", _tests_failed);                          \
        printf(" ---\n");                                                   \
        return (_tests_failed > 0) ? 1 : 0;                                \
    } while (0)

#endif /* TEST_FRAMEWORK_H */
