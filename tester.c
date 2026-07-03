#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dstr.h"

// Global counters for test reporting
static int tests_run = 0;
static int tests_failed = 0;

// Assertion macros with formatted output
#define ASSERT_TRUE(expr) do { \
    tests_run++; \
    if (!(expr)) { \
        printf("  \033[1;31m[FAIL]\033[0m %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #expr); \
        tests_failed++; \
        return; \
    } \
} while (0)

#define ASSERT_STR_EQUAL(expected, actual) do { \
    tests_run++; \
    if (strcmp((expected), (actual)) != 0) { \
        printf("  \033[1;31m[FAIL]\033[0m %s:%d: Strings differ.\n        Expected: \"%s\"\n        Got: \"%s\"\n", __FILE__, __LINE__, expected, actual); \
        tests_failed++; \
        return; \
    } \
} while (0)

#define RUN_TEST(test_func) do { \
    printf("Running %s...\n", #test_func); \
    test_func(); \
} while (0)

// --- TEST SUITE ---

static void test_find_basic(void) {
    dstr s = dstrnew("Hello World");

    ASSERT_TRUE(dstrfind(s, "o W") == 4);
    ASSERT_TRUE(dstrfind(s, "Hello") == 0);
    ASSERT_TRUE(dstrfind(s, "World") == 6);

    dstrfree(s);
}

static void test_find_not_found(void) {
    dstr s = dstrnew("System Integrity");

    ASSERT_TRUE(dstrfind(s, "Hack") == -1);
    ASSERT_TRUE(dstrfind(s, "system") == -1);

    dstrfree(s);
}

static void test_find_edge_cases(void) {
    dstr s = dstrnew("data");

    ASSERT_TRUE(dstrfind(s, "") == -1);
    ASSERT_TRUE(dstrfind(s, "database") == -1);
    ASSERT_TRUE(dstrfind(s, NULL) == -1);
    ASSERT_TRUE(dstrfind(NULL, "data") == -1);

    dstrfree(s);

    dstr empty = dstrnew("");
    ASSERT_TRUE(dstrfind(empty, "a") == -1);
    dstrfree(empty);
}

static void test_find_overlaps_and_repeats(void) {
    dstr s = dstrnew("abacadabra");

    ASSERT_TRUE(dstrfind(s, "a") == 0);
    ASSERT_TRUE(dstrfind(s, "ab") == 0);
    ASSERT_TRUE(dstrfind(s, "cad") == 3);
    ASSERT_TRUE(dstrfind(s, "abra") == 6);

    dstrfree(s);
}

static void test_creation_and_properties(void) {
    dstr s = dstrnew("Hello");
    ASSERT_TRUE(s != NULL);
    ASSERT_STR_EQUAL("Hello", s);
    ASSERT_TRUE(dstrlen(s) == 5);
    ASSERT_TRUE(dstrcap(s) >= 6); // length + \0
    dstrfree(s);
}

static void test_capacity_hint(void) {
    dstr s = dstrnew("Hello", 100);
    ASSERT_TRUE(s != NULL);
    ASSERT_TRUE(dstrlen(s) == 5);
    ASSERT_TRUE(dstrcap(s) == 100);
    dstrfree(s);
}

static void test_concatenation(void) {
    dstr s = dstrnew("Hello");
    s = dstrcat(s, " World");
    ASSERT_TRUE(s != NULL);
    ASSERT_STR_EQUAL("Hello World", s);
    ASSERT_TRUE(dstrlen(s) == 11);
    dstrfree(s);
}

static void test_push_char(void) {
    dstr s = dstrnew("Hello");
    s = dstrpush(s, '!');
    ASSERT_TRUE(s != NULL);
    ASSERT_STR_EQUAL("Hello!", s);
    ASSERT_TRUE(dstrlen(s) == 6);
    dstrfree(s);
}

static void test_clear_and_zero(void) {
    dstr s = dstrnew("Data");
    size_t orig_cap = dstrcap(s);

    dstrclear(s);
    ASSERT_TRUE(dstrlen(s) == 0);
    ASSERT_STR_EQUAL("", s);
    ASSERT_TRUE(dstrcap(s) == orig_cap);

    dstrfree(s);
}

static void test_tolower_basic(void) {
    dstr s = dstrnew("Hello World! 123");
    size_t len = dstrlen(s);
    size_t cap = dstrcap(s);

    dstrtolower(s);

    ASSERT_STR_EQUAL("hello world! 123", s);
    ASSERT_TRUE(dstrlen(s) == len);
    ASSERT_TRUE(dstrcap(s) == cap);

    dstrfree(s);
}

static void test_toupper_basic(void) {
    dstr s = dstrnew("Hello World! 123");
    size_t len = dstrlen(s);
    size_t cap = dstrcap(s);

    dstrtoupper(s);

    ASSERT_STR_EQUAL("HELLO WORLD! 123", s);
    ASSERT_TRUE(dstrlen(s) == len);
    ASSERT_TRUE(dstrcap(s) == cap);

    dstrfree(s);
}

static void test_case_conversion_already_converted(void) {
    dstr lower = dstrnew("already lower");
    dstrtolower(lower);
    ASSERT_STR_EQUAL("already lower", lower);
    dstrfree(lower);

    dstr upper = dstrnew("ALREADY UPPER");
    dstrtoupper(upper);
    ASSERT_STR_EQUAL("ALREADY UPPER", upper);
    dstrfree(upper);
}

static void test_case_conversion_empty(void) {
    dstr s = dstrnew("");

    dstrtolower(s);
    ASSERT_STR_EQUAL("", s);
    ASSERT_TRUE(dstrlen(s) == 0);

    dstrtoupper(s);
    ASSERT_STR_EQUAL("", s);
    ASSERT_TRUE(dstrlen(s) == 0);

    dstrfree(s);
}

static void test_case_conversion_null(void) {
    // Must not crash on NULL input.
    dstrtolower(NULL);
    dstrtoupper(NULL);
    ASSERT_TRUE(true);
}

static void test_case_conversion_non_ascii_untouched(void) {
    // Bytes >= 0x80 (e.g. UTF-8 continuation/lead bytes) must be left
    // untouched, and must not trigger undefined behavior on signed char.
    dstr s = dstrnew("\xC3\x89LLO \xC3\xA9llo");
    size_t len = dstrlen(s);

    dstrtolower(s);
    ASSERT_STR_EQUAL("\xC3\x89llo \xC3\xA9llo", s);
    ASSERT_TRUE(dstrlen(s) == len);

    dstr s2 = dstrnew("\xC3\x89LLO \xC3\xA9llo");
    dstrtoupper(s2);
    ASSERT_STR_EQUAL("\xC3\x89LLO \xC3\xA9LLO", s2);

    dstrfree(s);
    dstrfree(s2);
}

static void test_case_conversion_roundtrip(void) {
    dstr s = dstrnew("MiXeD CaSe 42!");
    dstr original = dstrdup(s);

    dstrtoupper(s);
    dstrtolower(s);

    // Original had lowercase letters already lowercase and uppercase
    // letters that become lowercase too, so compare against a fully
    // lowercased reference instead of the original.
    dstrtolower(original);
    ASSERT_STR_EQUAL(original, s);

    dstrfree(s);
    dstrfree(original);
}

static void test_auto_cleanup(void) {
    // Isolated block to test cleanup attribute
    {
        dstrauto s = dstrnew("AutoFree");
        ASSERT_STR_EQUAL("AutoFree", s);
    }
    // If dstrauto fails or crashes, execution would stop before here.
    ASSERT_TRUE(true);
}

int main(void) {
    printf("\033[1;34m=== STARTING DSTR UNIT TESTS ===\033[0m\n\n");

    RUN_TEST(test_find_basic);
    RUN_TEST(test_find_not_found);
    RUN_TEST(test_find_edge_cases);
    RUN_TEST(test_find_overlaps_and_repeats);
    RUN_TEST(test_creation_and_properties);
    RUN_TEST(test_capacity_hint);
    RUN_TEST(test_concatenation);
    RUN_TEST(test_push_char);
    RUN_TEST(test_clear_and_zero);
    RUN_TEST(test_tolower_basic);
    RUN_TEST(test_toupper_basic);
    RUN_TEST(test_case_conversion_already_converted);
    RUN_TEST(test_case_conversion_empty);
    RUN_TEST(test_case_conversion_null);
    RUN_TEST(test_case_conversion_non_ascii_untouched);
    RUN_TEST(test_case_conversion_roundtrip);
    RUN_TEST(test_auto_cleanup);

    printf("\n\033[1;34m=== FINAL REPORT ===\033[0m\n");
    printf("Total assertions run: %d\n", tests_run);

    if (tests_failed == 0) {
        printf("\033[1;32mPASSED: All tests succeeded!\033[0m\n");
        return EXIT_SUCCESS;
    } else {
        printf("\033[1;31mFAILED: %d assertions failed.\033[0m\n", tests_failed);
        return EXIT_FAILURE;
    }
}
