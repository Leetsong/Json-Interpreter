#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

/* print by color */

#define ORANGE "\033[1;33;40m"
#define GREEN "\033[1;32;40m"
#define RED "\033[1;31;40m"
#define NONE "\033[0m"

#define fprintf_color(color, output, ...) \
    do { \
        fprintf(output, color); \
        fprintf(output, __VA_ARGS__); \
        fprintf(output, NONE); \
    } while(0)

#define fprintf_warn(output, ...) fprintf_color(ORANGE, output, __VA_ARGS__)
#define fprintf_info(output, ...) fprintf_color(GREEN, output, __VA_ARGS__)
#define fprintf_error(output, ...) fprintf_color(RED, output, __VA_ARGS__)

/* test base */

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, RED "%s:%d:\n", __FILE__, __LINE__); \
            fprintf(stderr, "    expect: " format " actual: " format "\n" NONE, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_TEST(expect, actual, str) EXPECT_EQ_BASE((expect) == (actual), str[expect], str[actual], "%s")
#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, len) EXPECT_EQ_BASE(strncmp((expect), (actual), (len)) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) == 1, 1, actual, "%d");
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, 0, actual, "%d");

/* test macro */

#define TEST_NUMBER(expect, json) \
    do { \
        lept_value v; \
        lept_init(&v); \
        int ret_parse, ret_type; \
        double ret_number; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(LEPT_NUMBER, ret_type, lept_type_string); \
        ret_number = lept_get_number(&v); \
        EXPECT_EQ_DOUBLE(expect, ret_number); \
        lept_free(&v); \
    } while(0)

#define TEST_STRING(expect, json) \
    do { \
        lept_value v; \
        lept_init(&v); \
        int ret_parse, ret_type; \
        const char* ret_string; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(LEPT_STRING, ret_type, lept_type_string); \
        ret_string = lept_get_string(&v); \
        EXPECT_EQ_STRING(expect, ret_string, lept_get_string_length(&v)); \
        lept_free(&v); \
    } while(0)

#define TEST_LITERAL(expect, json) \
    do{ \
        lept_value v; \
        lept_init(&v); \
        int ret_parse, ret_type; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(expect, ret_type, lept_type_string); \
        lept_free(&v); \
    } while(0)

// #define TEST_ERROR(error, json) \
//     do { \
//         lept_value v; \
//         lept_init(&v); \
//         int ret_parse; \
//         ret_parse = lept_parse(&v, json); \
//         EXPECT_EQ_TEST(error, ret_parse, lept_parse_xxx_string); \
//         lept_free(&v); \
//     } while(0)

#define TEST_EXPECT_VALUE(json) \
    do { \
        lept_value v; \
        lept_init(&v); \
        int ret_parse, ret_type; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_EXPECT_VALUE, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(LEPT_NULL, ret_type, lept_type_string); \
        lept_free(&v); \
    } while(0)

#define TEST_INVALID_VALUE(json) \
    do { \
        lept_value v; \
        lept_init(&v); \
        int ret_parse, ret_type; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_INVALID_VALUE, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(LEPT_NULL, ret_type, lept_type_string); \
        lept_free(&v); \
    } while(0)

#define TEST_NUMBER_TOO_BIG(json) \
    do { \
        lept_value v; \
        lept_init(&v); \
        int ret_parse, ret_type; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_NUMBER_TOO_BIG, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(LEPT_NULL, ret_type, lept_type_string); \
        lept_free(&v); \
    } while(0)

#define TEST_ROOT_NOT_SINGULAR(expect, json) \
    do { \
        lept_value v; \
        lept_init(&v); \
        int ret_parse, ret_type; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_ROOT_NOT_SINGULAR, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(expect, ret_type, lept_type_string); \
        lept_free(&v); \
    } while(0)

#define TEST_MISS_QUOTATION_MARK(json) \
    do { \
        lept_value v; \
        int ret_parse, ret_type; \
        lept_init(&v); \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_MISS_QUOTATION_MARK, ret_parse, lept_parse_xxx_string); \
        ret_type = lept_get_type(&v); \
        EXPECT_EQ_TEST(LEPT_NULL, ret_type, lept_type_string); \
    } while (0)

#define TEST_INVALID_ESCAPE(json) \
    do { \
        lept_value v; \
        int ret_parse; \
        lept_init(&v); \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_INVALID_ESCAPE, ret_parse, lept_parse_xxx_string); \
        lept_free(&v); \
    } while(0)

#define TEST_INVALID_STRING_CHAR(json) \
    do { \
        lept_value v; \
        int ret_parse; \
        lept_init(&v); \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(LEPT_PARSE_INVALID_STRING_CHAR, ret_parse, lept_parse_xxx_string); \
        lept_free(&v); \
    } while(0)

/* test case */

static void test_parse_number() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_string() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_STRING("12SDFE3", "\"12SDFE3\"");
    TEST_STRING("12ASDF\"AS3", "\"12ASDF\\\"AS3\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}

static void test_parse_ok() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_LITERAL(LEPT_NULL, "null");
    TEST_LITERAL(LEPT_FALSE, "false");
    TEST_LITERAL(LEPT_TRUE, "true");

    test_parse_number();
    test_parse_string();
}

static void test_parse_expect_value() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_EXPECT_VALUE("");
    TEST_EXPECT_VALUE("  ");
}

static void test_parse_invalid_value() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_INVALID_VALUE("nul");
    TEST_INVALID_VALUE("?");
    TEST_INVALID_VALUE("0000");
    TEST_INVALID_VALUE("+0");
    TEST_INVALID_VALUE("+1");
    TEST_INVALID_VALUE(".123");
    TEST_INVALID_VALUE("1.");
    TEST_INVALID_VALUE("inf");
    TEST_INVALID_VALUE("INF");
    TEST_INVALID_VALUE("NAN");
    TEST_INVALID_VALUE("nan");
    TEST_NUMBER_TOO_BIG("123E123123122");
    TEST_NUMBER_TOO_BIG("-123E123123122");
}

static void test_parse_root_not_singular() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_ROOT_NOT_SINGULAR(LEPT_NULL, "null x");
    TEST_ROOT_NOT_SINGULAR(LEPT_FALSE, "false null");
    TEST_ROOT_NOT_SINGULAR(LEPT_NUMBER, "123e3 ASD");
}

static void test_parse_number_too_big() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_NUMBER_TOO_BIG("123E123123122");
    TEST_NUMBER_TOO_BIG("-123E123123122");
}

static void test_parse_miss_quotation_mark() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_MISS_QUOTATION_MARK("\"");
    TEST_MISS_QUOTATION_MARK("\"ABC");
}

static void test_access_string() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v, "Hello World!", 12);
    EXPECT_EQ_STRING("Hello World!", lept_get_string(&v), lept_get_string_length(&v));
    lept_free(&v);
}

static void test_access_boolean() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "A", 1);
    lept_set_boolean(&v, 0);
    EXPECT_FALSE(lept_get_boolean(&v));
    lept_free(&v);
}

static void test_access_number() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_number(&v, 123.1);
    EXPECT_EQ_DOUBLE(123.1, lept_get_number(&v));
    lept_free(&v);
}

static void test_parse_invalid_string_char() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

}

static void test_parse_invalid_escape() {
    fprintf_warn(stdout, "=> %s starts...\n", __func__);

    TEST_INVALID_ESCAPE("\"\\v\"");
    TEST_INVALID_ESCAPE("\"\\x\"");
    TEST_INVALID_ESCAPE("\"\\0\"");
}

static void test_parse() {
    test_parse_ok();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_miss_quotation_mark();
    test_parse_invalid_escape();
    test_parse_invalid_string_char();

    test_access_boolean();
    test_access_string();
    test_access_number();
}

/* main */

int main() {
    test_parse();
    fprintf_info(stdout, "%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);

    return main_ret;
}
