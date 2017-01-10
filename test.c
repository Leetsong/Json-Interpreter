#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

/* helper - print by color */

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

// static void _print_json_array(lept_value* v, int depth, FILE* fp);
// static void _print_json_object(lept_value* v, int depth, FILE* fp);
//
// static void _print_json_array(lept_value* v, int depth, FILE* fp) {
//     for(int i = 0; i < depth; i ++) {
//         fprintf(fp, "\t");
//     }
//     fprintf(fp, "[\n");
//
//     for(size_t i = 0; i < lept_get_array_size(v); i ++) {
//         for(int i = 0; i < depth; i ++) {
//             fprintf(fp, "\t");
//         }
//         fprintf(fp, "  ");
//         switch(lept_get_type(lept_get_array_element(v, i))) {
//         case LEPT_NULL: fprintf(fp, "null,\n"); break;
//         case LEPT_TRUE: fprintf(fp, "true,\n"); break;
//         case LEPT_FALSE: fprintf(fp, "false,\n"); break;
//         case LEPT_NUMBER: fprintf(fp, "%lf,\n", lept_get_number(lept_get_array_element(v, i))); break;
//         case LEPT_STRING: fprintf(fp, "\"%s\",\n", lept_get_string(lept_get_array_element(v, i))); break;
//         case LEPT_ARRAY:
//             fprintf(fp, "\n");
//             _print_json_array(lept_get_array_element(v, i), depth + 1, fp);
//             break;
//         case LEPT_OBJECT:
//             fprintf(fp, "\n");
//             _print_json_object(lept_get_array_element(v, i), depth + 1, fp);
//             break;
//         default: assert(0);
//         }
//     }
//
//     for(int i = 0; i < depth; i ++) {
//         fprintf(fp, "\t");
//     }
//     fprintf(fp, "],\n");
//
// }
//
// static void _print_json_object(lept_value* v, int depth, FILE* fp) {
//     for(int i = 0; i < depth; i ++) {
//         fprintf(fp, "\t");
//     }
//     fprintf(fp, "{\n");
//
//     for(size_t i = 0; i < lept_get_object_size(v); i ++) {
//         for(int i = 0; i < depth; i ++) {
//             fprintf(fp, "\t");
//         }
//         fprintf(fp, "  \"%s\": ", lept_get_object_key(v, i));
//         switch(lept_get_type(lept_get_object_value(v, i))) {
//         case LEPT_NULL: fprintf(fp, "null,\n"); break;
//         case LEPT_TRUE: fprintf(fp, "true,\n"); break;
//         case LEPT_FALSE: fprintf(fp, "false,\n"); break;
//         case LEPT_NUMBER: fprintf(fp, "%lf,\n", lept_get_number(lept_get_object_value(v, i))); break;
//         case LEPT_STRING: fprintf(fp, "\"%s\",\n", lept_get_string(lept_get_object_value(v, i))); break;
//         case LEPT_ARRAY:
//             fprintf(fp, "\n");
//             _print_json_array(lept_get_object_value(v, i), depth + 1, fp);
//             break;
//         case LEPT_OBJECT:
//             fprintf(fp, "\n");
//             _print_json_object(lept_get_object_value(v, i), depth + 1, fp);
//             break;
//         default: assert(0);
//         }
//     }
//
//
//     for(int i = 0; i < depth; i ++) {
//         fprintf(fp, "\t");
//     }
//     fprintf(fp, "},\n");
//
// }
//
// static void print_json_object(const char* r, lept_value* v) {
//     FILE* fp = fopen("test.json", "a");
//     if(fp == NULL) {
//         fprintf_error(stderr, "!!!!!!!!!!!! >> %s: FAILED TO OPEN THE FILE", __func__);
//     }
//     fprintf(fp, ">>>>>>> %s\n", r);
//     _print_json_object(v, 0, fp);
//     fprintf(fp, "\n");
//     fclose(fp);
// }

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
#ifdef _MSC_VER
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu");
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu");
#endif

/* test macro */

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

#define TEST_ERROR(lept_error, json) \
    do { \
        lept_value v; \
        lept_init(&v); \
        int ret_parse; \
        ret_parse = lept_parse(&v, json); \
        EXPECT_EQ_TEST(lept_error, ret_parse, lept_parse_xxx_string); \
        lept_free(&v); \
    } while(0)

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

#define TEST_ARRAY(expect, json) \
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


/* test cases */

static void test_parse_number() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

	TEST_NUMBER(55.123, "55.123");
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
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_STRING("12SDFE3", "\"12SDFE3\"");
    TEST_STRING("12ASDF\"AS3", "\"12ASDF\\\"AS3\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("\x00\x01\x12", "\"\\u0000\\u0123\\u1234\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_array() {
	fprintf_warn(stdout, " => %s starts...\n", __func__);

	lept_value v;
	int ret_parse;

	lept_init(&v);
	ret_parse = lept_parse(&v, "[55.123, 122.1, 3.12, 4]");
	EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
	EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
	EXPECT_EQ_DOUBLE(55.123, lept_get_array_element(&v, 0)->number.v);
	EXPECT_EQ_DOUBLE(122.1, lept_get_array_element(&v, 1)->number.v);
	EXPECT_EQ_DOUBLE(3.12, lept_get_array_element(&v, 2)->number.v);
	EXPECT_EQ_DOUBLE(4.0, lept_get_array_element(&v, 3)->number.v);
	lept_free(&v);

	lept_init(&v);
	ret_parse = lept_parse(&v, "[       1,true, null,4    ]");
	EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
	EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
	EXPECT_EQ_DOUBLE(1.0f, lept_get_array_element(&v, 0)->number.v);
	EXPECT_EQ_INT(1, lept_get_boolean(lept_get_array_element(&v, 1)));
	EXPECT_EQ_TEST(LEPT_NULL, lept_get_type(lept_get_array_element(&v, 2)), lept_type_string);
	EXPECT_EQ_DOUBLE(4.0f, lept_get_array_element(&v, 3)->number.v);
	lept_free(&v);

	lept_init(&v);
	ret_parse = lept_parse(&v, "[\"13fas\", \"\\uD834\\uDD1E\", 3, 4]");
	EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
	EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
	EXPECT_EQ_STRING("13fas", lept_get_string(lept_get_array_element(&v, 0)), 7);
	EXPECT_EQ_STRING("\xF0\x9D\x84\x9E", lept_get_string(lept_get_array_element(&v, 1)), 7);
	EXPECT_EQ_DOUBLE(3.0f, lept_get_array_element(&v, 2)->number.v);
	EXPECT_EQ_DOUBLE(4.0f, lept_get_array_element(&v, 3)->number.v);
	lept_free(&v);

	lept_init(&v);
	ret_parse = lept_parse(&v, "[\"13fas\", [ 1, 55.123], 3, 4]");
	EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
	EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
	EXPECT_EQ_STRING("13fas", lept_get_string(lept_get_array_element(&v, 0)), 7);
	lept_value* v2 = lept_get_array_element(&v, 1);
	EXPECT_EQ_DOUBLE(1.0, lept_get_array_element(v2, 0)->number.v);
	EXPECT_EQ_DOUBLE(55.123, lept_get_array_element(v2, 1)->number.v);
	EXPECT_EQ_DOUBLE(3.0, lept_get_array_element(&v, 2)->number.v);
	EXPECT_EQ_DOUBLE(4.0, lept_get_array_element(&v, 3)->number.v);
	lept_free(&v);
}

static void test_parse_object() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);


	lept_value v;
	int ret_parse;
    int i;

	lept_init(&v);

	ret_parse = lept_parse(&v, "{\"key1\":1}");
    // print_json_object("{\"key1\":1}", &v);
	EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
	EXPECT_EQ_SIZE_T(1, lept_get_object_size(&v));

    EXPECT_EQ_SIZE_T(4, lept_get_object_key_length(&v, 0));
	EXPECT_EQ_STRING("key1", lept_get_object_key(&v, 0), 4);
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_object_value(&v, 0)->type);
	EXPECT_EQ_DOUBLE(1.0f, lept_get_object_value(&v, 0)->number.v);

    lept_free(&v);


    lept_init(&v);

    ret_parse = lept_parse(&v, "{\"key1\":1, \"key22\":  true}");
    // print_json_object("{\"key1\":1, \"key22\":  true}", &v);
    EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
    EXPECT_EQ_SIZE_T(2, lept_get_object_size(&v));

    EXPECT_EQ_SIZE_T(4, lept_get_object_key_length(&v, 0));
    EXPECT_EQ_STRING("key1", lept_get_object_key(&v, 0), 4);
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_object_value(&v, 0)->type);
    EXPECT_EQ_DOUBLE(1.0f, lept_get_object_value(&v, 0)->number.v);

    EXPECT_EQ_SIZE_T(5, lept_get_object_key_length(&v, 1));
    EXPECT_EQ_STRING("key22", lept_get_object_key(&v, 1), 5);
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_object_value(&v, 1)->type);
    EXPECT_EQ_INT(1, lept_get_boolean(lept_get_object_value(&v, 1)));

    lept_free(&v);


    lept_init(&v);

    ret_parse = lept_parse(&v, "{\"key1\":1, \"key22\":  true, \"key123\": [1, 2,3]}");
    // print_json_object("{\"key1\":1, \"key22\":  true, \"key123\": [1, 2,3]}", &v);
    EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
    EXPECT_EQ_SIZE_T(3, lept_get_object_size(&v));

    EXPECT_EQ_SIZE_T(4, lept_get_object_key_length(&v, 0));
    EXPECT_EQ_STRING("key1", lept_get_object_key(&v, 0), 4);
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_object_value(&v, 0)->type);
    EXPECT_EQ_DOUBLE(1.0f, lept_get_object_value(&v, 0)->number.v);

    EXPECT_EQ_SIZE_T(5, lept_get_object_key_length(&v, 1));
    EXPECT_EQ_STRING("key22", lept_get_object_key(&v, 1), 5);
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_object_value(&v, 1)->type);
    EXPECT_EQ_INT(1, lept_get_boolean(lept_get_object_value(&v, 1)));

    EXPECT_EQ_SIZE_T(6, lept_get_object_key_length(&v, 2));
    EXPECT_EQ_STRING("key123", lept_get_object_key(&v, 2), 6);
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_object_value(&v, 2)->type);

    lept_free(&v);


    lept_init(&v);

    ret_parse = lept_parse(&v, "{\"key1\":1, \"key22\":  true, \"key123\": [1, 2,3],         \"key1234\"   : { \"kkk\": \"123\"}}");
    // print_json_object("{\"key1\":1, \"key22\":  true, \"key123\": [1, 2,3],         \"key1234\"   : { \"kkk\": \"123\"}}", &v);
    EXPECT_EQ_TEST(LEPT_PARSE_OK, ret_parse, lept_parse_xxx_string);
    EXPECT_EQ_SIZE_T(4, lept_get_object_size(&v));

    EXPECT_EQ_SIZE_T(4, lept_get_object_key_length(&v, 0));
    EXPECT_EQ_STRING("key1", lept_get_object_key(&v, 0), 4);
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_object_value(&v, 0)->type);
    EXPECT_EQ_DOUBLE(1.0f, lept_get_object_value(&v, 0)->number.v);

    EXPECT_EQ_SIZE_T(5, lept_get_object_key_length(&v, 1));
    EXPECT_EQ_STRING("key22", lept_get_object_key(&v, 1), 5);
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_object_value(&v, 1)->type);
    EXPECT_EQ_INT(1, lept_get_boolean(lept_get_object_value(&v, 1)));

    EXPECT_EQ_SIZE_T(6, lept_get_object_key_length(&v, 2));
    EXPECT_EQ_STRING("key123", lept_get_object_key(&v, 2), 6);
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_object_value(&v, 2)->type);

    EXPECT_EQ_SIZE_T(7, lept_get_object_key_length(&v, 3));
    EXPECT_EQ_STRING("key1234", lept_get_object_key(&v, 3), 7);
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_object_value(&v, 3)->type);
    EXPECT_EQ_SIZE_T(1, lept_get_object_size(lept_get_object_value(&v, 3)));
    EXPECT_EQ_SIZE_T(3, lept_get_object_key_length(lept_get_object_value(&v, 3), 0));
    EXPECT_EQ_STRING("kkk", lept_get_object_key(lept_get_object_value(&v, 3), 0), 3);

    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(7, lept_get_object_size(&v));
    EXPECT_EQ_STRING("n", lept_get_object_key(&v, 0), lept_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(LEPT_NULL,   lept_get_type(lept_get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1), lept_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(LEPT_FALSE,  lept_get_type(lept_get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2), lept_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(LEPT_TRUE,   lept_get_type(lept_get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3), lept_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4), lept_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(lept_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_object_value(&v, 4)), lept_get_string_length(lept_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5), lept_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, lept_get_array_size(lept_get_object_value(&v, 5)));
    for(i = 0; i < 3; i++) {
        lept_value* e = lept_get_array_element(lept_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(e));
    }
    EXPECT_EQ_STRING("o", lept_get_object_key(&v, 6), lept_get_object_key_length(&v, 6));
    {
        lept_value* o = lept_get_object_value(&v, 6);
        EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(o));
        for(i = 0; i < 3; i++) {
            lept_value* ov = lept_get_object_value(o, i);
            EXPECT_TRUE('1' + i == lept_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, lept_get_object_key_length(o, i));
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(ov));
        }
    }
    lept_free(&v);

}

static void test_parse_ok() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_LITERAL(LEPT_NULL, "null");
    TEST_LITERAL(LEPT_FALSE, "false");
    TEST_LITERAL(LEPT_TRUE, "true");

    test_parse_number();
    test_parse_string();
	test_parse_array();
    test_parse_object();
}

static void test_parse_expect_value() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "  ");
}

static void test_parse_invalid_value() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "0000");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[,]");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[2, 2, ]");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[1}");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[1 2");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[[]");
}

static void test_parse_root_not_singular() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "false null");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "123e3 ASD");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "[123, 12] AS");
}

static void test_parse_number_too_big() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "123E123123122");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-123E123123122");
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "[-123E123123122, 12]");
}

static void test_parse_miss_quotation_mark() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"ABC");
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "[123, \"ABC]");
}

static void test_parse_invalid_escape() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_INVALID_ESCAPE, "\"\\v\"");
    TEST_ERROR(LEPT_PARSE_INVALID_ESCAPE, "\"\\x\"");
    TEST_ERROR(LEPT_PARSE_INVALID_ESCAPE, "\"\\0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_ESCAPE, "[123, \"\\0\"]");
}

static void test_parse_invalid_string_char() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "[123, [2, 2], \"\x1F\"]");
}

static void test_parse_invalid_unicode_surrogate() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "[1212, \"\\uD800\\uE000\", [21]]");
}

static void test_parse_invalid_unicode_hex() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
	TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "[\"\\u000G\"]");
}

static void test_parse_miss_key() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_access_string() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

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
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "A", 1);
    lept_set_boolean(&v, 0);
    EXPECT_FALSE(lept_get_boolean(&v));
    lept_free(&v);
}

static void test_access_number() {
    fprintf_warn(stdout, " => %s starts...\n", __func__);

    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_number(&v, 123.1);
    EXPECT_EQ_DOUBLE(123.1, lept_get_number(&v));
    lept_free(&v);
}

static void test_access_array() {
	fprintf_warn(stdout, " => %s starts...\n", __func__);

	lept_value v;

	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ ]"));
	EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));
	lept_free(&v);
}

static void test_access_object() {
	fprintf_warn(stdout, " => %s starts...\n", __func__);

	lept_value v;

	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "{}"));
	EXPECT_EQ_SIZE_T(0, lept_get_object_size(&v));
	lept_free(&v);
}

static void test_parse() {
    fprintf_color(GREEN, stdout,  "== %s starts...\n", __func__);

    test_parse_ok();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_miss_quotation_mark();
    test_parse_invalid_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_surrogate();
    test_parse_invalid_unicode_hex();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();

    test_access_boolean();
    test_access_string();
    test_access_number();
	test_access_array();
    test_access_object();
}

#define TEST_ROUNDTRIP(json, len) \
    do { \
        char* r_json; \
        size_t r_len; \
        int ret; \
        lept_value v; \
        lept_init(&v); \
        ret = lept_parse(&v, json); \
        EXPECT_EQ_INT(LEPT_PARSE_OK, ret); \
        ret = lept_stringify(&v, &r_json, &r_len); \
        EXPECT_EQ_INT(LEPT_STRINGIFY_OK, ret); \
        EXPECT_EQ_SIZE_T(len, r_len); \
        EXPECT_EQ_STRING(json, r_json, r_len); \
    } while(0)

static void test_stringify() {
    fprintf_color(GREEN, stdout,  "== %s starts...\n", __func__);

    TEST_ROUNDTRIP("null", 4);
    TEST_ROUNDTRIP("true", 4);
    TEST_ROUNDTRIP("false", 5);
}

/* main */

int main() {
    test_parse();
    test_stringify();
    fprintf_info(stdout, "%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);

    return main_ret;
}
