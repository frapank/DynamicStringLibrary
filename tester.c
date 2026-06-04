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

    RUN_TEST(test_creation_and_properties);
    RUN_TEST(test_capacity_hint);
    RUN_TEST(test_concatenation);
    RUN_TEST(test_push_char);
    RUN_TEST(test_clear_and_zero);
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
