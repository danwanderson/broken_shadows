/*
 * test_buffer.c — Unit tests for buffer.c
 *
 * Covers:
 *   find_mem_size   — size-class lookup
 *   buffer_new      — allocation
 *   buffer_strcat   — append with expansion
 *   buffer_clear    — reset without free
 *   buffer_free     — deallocation
 *   bprintf         — formatted print (replaces buffer contents)
 *
 * Dependencies satisfied by stubs.c:
 *   rgSizeList, bugf
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <form.h>
#include "merc.h"
#include "test_framework.h"

/* ------------------------------------------------------------------ */
/*  find_mem_size                                                       */
/* ------------------------------------------------------------------ */

static void test_find_mem_size_exact_first(void)
{
    /* 16 is the smallest size class; any size <= 16 maps to 16. */
    ASSERT_INT_EQ(find_mem_size(1),  16);
    ASSERT_INT_EQ(find_mem_size(16), 16);
}

static void test_find_mem_size_next_class(void)
{
    /* 17 must round up to 32 (the next size class). */
    ASSERT_INT_EQ(find_mem_size(17), 32);
}

static void test_find_mem_size_exact_256(void)
{
    ASSERT_INT_EQ(find_mem_size(256), 256);
}

static void test_find_mem_size_gap(void)
{
    /*
     * There is no 512 class; 257 must skip to 1024.
     * rgSizeList: 16 32 64 128 256 1024 …
     */
    ASSERT_INT_EQ(find_mem_size(257), 1024);
}

static void test_find_mem_size_largest(void)
{
    /* Last entry is 32704 (32768-64). */
    ASSERT_INT_EQ(find_mem_size(32704), 32704);
}

static void test_find_mem_size_too_large(void)
{
    /* Requests larger than the biggest class return EMEM_SIZE (-1). */
    ASSERT_INT_EQ(find_mem_size(32705), -1);
}

/* ------------------------------------------------------------------ */
/*  buffer_new / buffer_free                                            */
/* ------------------------------------------------------------------ */

static void test_buffer_new_returns_valid_buffer(void)
{
    BUFFER *b = buffer_new(64);
    ASSERT_NOTNULL(b);
    ASSERT_NOTNULL(b->data);
    ASSERT_INT_EQ(b->len, 0);
    ASSERT(b->size >= 64);
    ASSERT_INT_EQ(b->overflowed, FALSE);
    buffer_free(b);
}

static void test_buffer_new_minimum_size(void)
{
    /* Even requesting 1 byte should succeed (maps to 16). */
    BUFFER *b = buffer_new(1);
    ASSERT_NOTNULL(b);
    ASSERT(b->size >= 1);
    buffer_free(b);
}

/* ------------------------------------------------------------------ */
/*  buffer_strcat                                                       */
/* ------------------------------------------------------------------ */

static void test_buffer_strcat_basic(void)
{
    BUFFER *b = buffer_new(64);
    buffer_strcat(b, "hello");
    ASSERT_INT_EQ(b->len, 5);
    ASSERT_STR_EQ(b->data, "hello");
    buffer_free(b);
}

static void test_buffer_strcat_append_twice(void)
{
    BUFFER *b = buffer_new(64);
    buffer_strcat(b, "foo");
    buffer_strcat(b, "bar");
    ASSERT_INT_EQ(b->len, 6);
    ASSERT_STR_EQ(b->data, "foobar");
    buffer_free(b);
}

static void test_buffer_strcat_null_ignored(void)
{
    /* buffer_strcat silently ignores NULL text. */
    BUFFER *b = buffer_new(64);
    buffer_strcat(b, "hi");
    buffer_strcat(b, NULL);
    ASSERT_INT_EQ(b->len, 2);
    ASSERT_STR_EQ(b->data, "hi");
    buffer_free(b);
}

static void test_buffer_strcat_empty_ignored(void)
{
    /* buffer_strcat silently ignores empty-string text. */
    BUFFER *b = buffer_new(64);
    buffer_strcat(b, "x");
    buffer_strcat(b, "");
    ASSERT_INT_EQ(b->len, 1);
    buffer_free(b);
}

static void test_buffer_strcat_triggers_expansion(void)
{
    /*
     * Start with a small buffer and append enough data to force at least
     * one reallocation.  The buffer should still hold all data correctly.
     */
    const char *chunk = "0123456789abcdef"; /* 16 chars */
    BUFFER *b = buffer_new(16);             /* initial capacity == 16 */
    int i;

    for (i = 0; i < 5; i++)               /* append 5 * 16 = 80 bytes */
        buffer_strcat(b, chunk);

    ASSERT_INT_EQ(b->len, 80);
    ASSERT_INT_EQ(b->overflowed, FALSE);
    buffer_free(b);
}

/* ------------------------------------------------------------------ */
/*  buffer_clear                                                        */
/* ------------------------------------------------------------------ */

static void test_buffer_clear_resets_length(void)
{
    BUFFER *b = buffer_new(64);
    buffer_strcat(b, "some data");
    ASSERT_INT_EQ(b->len, 9);

    buffer_clear(b);
    ASSERT_INT_EQ(b->len, 0);
    ASSERT_INT_EQ(b->overflowed, FALSE);
    /* Data pointer and size unchanged — no reallocation. */
    ASSERT_NOTNULL(b->data);
    buffer_free(b);
}

static void test_buffer_clear_allows_reuse(void)
{
    BUFFER *b = buffer_new(64);
    buffer_strcat(b, "first");
    buffer_clear(b);
    buffer_strcat(b, "second");
    ASSERT_INT_EQ(b->len, 6);
    ASSERT_STR_EQ(b->data, "second");
    buffer_free(b);
}

/* ------------------------------------------------------------------ */
/*  bprintf                                                             */
/* ------------------------------------------------------------------ */

static void test_bprintf_basic(void)
{
    BUFFER *b = buffer_new(256);
    int ret = bprintf(b, "hello %d", 42);
    ASSERT_STR_EQ(b->data, "hello 42");
    ASSERT_INT_EQ(ret, 8); /* vsnprintf return: chars that would be written */
    buffer_free(b);
}

static void test_bprintf_replaces_previous_content(void)
{
    /* bprintf calls buffer_clear internally, so it REPLACES content. */
    BUFFER *b = buffer_new(256);
    bprintf(b, "first");
    bprintf(b, "second");
    ASSERT_STR_EQ(b->data, "second");
    buffer_free(b);
}

static void test_bprintf_string_format(void)
{
    BUFFER *b = buffer_new(256);
    bprintf(b, "name=%s age=%d", "Alice", 30);
    ASSERT_STR_EQ(b->data, "name=Alice age=30");
    buffer_free(b);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* find_mem_size */
    test_find_mem_size_exact_first();
    test_find_mem_size_next_class();
    test_find_mem_size_exact_256();
    test_find_mem_size_gap();
    test_find_mem_size_largest();
    test_find_mem_size_too_large();

    /* buffer_new / buffer_free */
    test_buffer_new_returns_valid_buffer();
    test_buffer_new_minimum_size();

    /* buffer_strcat */
    test_buffer_strcat_basic();
    test_buffer_strcat_append_twice();
    test_buffer_strcat_null_ignored();
    test_buffer_strcat_empty_ignored();
    test_buffer_strcat_triggers_expansion();

    /* buffer_clear */
    test_buffer_clear_resets_length();
    test_buffer_clear_allows_reuse();

    /* bprintf */
    test_bprintf_basic();
    test_bprintf_replaces_previous_content();
    test_bprintf_string_format();

    TEST_SUITE_END();
}
