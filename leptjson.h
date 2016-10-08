#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stddef.h> /* size_t */

struct lept_value;
typedef struct lept_value lept_value;

typedef enum {
	LEPT_NULL, LEPT_FALSE, LEPT_TRUE,
	LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY,
	LEPT_OBJECT
} lept_type;

struct lept_value {

	union {
		struct {
			lept_value* e;
			size_t size;
		}; /* array */

		struct {
			size_t len;
			char* s;
		}; /* string */

		double n; /* number */
	};

    lept_type type;

};

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,
	LEPT_PARSE_INVALID_ESCAPE,
	LEPT_PARSE_INVALID_STRING_CHAR,
	LEPT_PARSE_INVALID_UNICODE_SURROGATE,
	LEPT_PARSE_INVALID_UNICODE_HEX
};

/* helper - strings */

static const char* lept_type_string[] = {
	"LEPT_NULL",
	"LEPT_FALSE",
	"LEPT_TRUE",
	"LEPT_NUMBER",
	"LEPT_STRING",
	"LEPT_ARRAY",
	"LEPT_OBJECT"
};

static const char* lept_parse_xxx_string[] = {
	"LEPT_PARSE_OK",
	"LEPT_PARSE_EXPECT_VALUE",
	"LEPT_PARSE_INVALID_VALUE",
	"LEPT_PARSE_ROOT_NOT_SINGULAR",
	"LEPT_PARSE_NUMBER_TOO_BIG",
	"LEPT_PARSE_MISS_QUOTATION_MARK",
	"LEPT_PARSE_INVALID_ESCAPE",
	"LEPT_PARSE_INVALID_STRING_CHAR",
	"LEPT_PARSE_INVALID_UNICODE_SURROGATE",
	"LEPT_PARSE_INVALID_UNICODE_HEX"
};

void lept_init(lept_value* v);
void lept_free(lept_value* v);

int lept_parse(lept_value* v, const char* json);
lept_type lept_get_type(const lept_value* v);

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double d);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

#define lept_set_null(v) lept_free(v)

#endif /* LEPTJSON_H__ */
