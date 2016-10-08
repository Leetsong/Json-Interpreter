#include "leptjson.h"

#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod(), realloc()... */
#include <string.h>  /* memcpy()... */
#include <ctype.h>   /* isdigit() */
#include <math.h>    /* HUGE_VAL */
#include <errno.h>   /* errno */
#include <stdio.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} lept_context;

/* context */

static void lept_context_init(lept_context* c, const char* json) {
    c->json = json;
    c->stack = NULL;
    c->top = c->size = 0;
}

static void lept_context_free(lept_context* c) {
    assert(c->top == 0);
    free(c->stack);
}

static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if(c->top + size >= c->size) {
        if(c->size == 0) {
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        }
        while(c->top + size >= c->size) {
            c->size += c->size >> 1; /* c.size *= 1.5 */
        }
        c->stack = (char*)realloc(c->stack, c->size * sizeof(char));
    }
    ret = c->stack + c->top; /* old top */
    c->top += size; /* new top */
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

#define LEPT_CONTEXT_PUSH(c, type, v) \
    do { \
        *((type*)(lept_context_push((c), sizeof(type)))) = (v); \
    } while(0)

#define LEPT_CONTEXT_POP(c, type) \
    *((type*)(lept_context_pop((c), sizeof(type))))

#define LEPT_CONTEXT_POP_ALL(c) \
    lept_context_pop((c), c->top) \

/* parse ws */

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/* parse true, null, false */

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
  	size_t i;

  	assert(*c->json == literal[0]);

  	for(i = 0; literal[i]; i ++) {
  		if(c->json[i] != literal[i]) {
  			return LEPT_PARSE_INVALID_VALUE;
  		}
  	}
    c->json += i;
    v->type = type;

    return LEPT_PARSE_OK;
}

/* parse number */

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char* cur_phase = c->json;
    char* next_phase = NULL;

    assert(*cur_phase == '0' || *cur_phase == '1' || *cur_phase == '2' ||
        *cur_phase == '3' || *cur_phase == '4' || *cur_phase == '5' ||
        *cur_phase == '6' || *cur_phase == '7' || *cur_phase == '8' ||
        *cur_phase == '9' || *cur_phase == '-');

    if(*cur_phase == '0') {
    	/* avoid "00.." and hex number(strtod supports "00.." and hex number) */
        if(cur_phase[1] != '\0' && (cur_phase[1] == '0' || cur_phase[1] == 'x')) {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }

    if(*cur_phase != '-') {
        /* has frac */
        if(cur_phase[1] != '\0' && cur_phase[1] == '.') {
        	/* avoid strings like "1.", "2.xas"... */
            if(cur_phase[2] == '\0' ||
              (cur_phase[2] && !isdigit(cur_phase[2]))) {
                return LEPT_PARSE_INVALID_VALUE;
            }
        }
    }

    /** strtod() succeeded,
     *    if and only if the string starts with a (hex or decimal) number
     */
    v->n = strtod(cur_phase, &next_phase);
    if(cur_phase == next_phase) {
        /* not a number */
        return LEPT_PARSE_INVALID_VALUE;
    } else if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL )) {
        /* out of range */
        errno = 0;
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    c->json = next_phase;
    v->type = LEPT_NUMBER;

    return LEPT_PARSE_OK;
}

/* parse string */

#define PUTC(c, ch) do { LEPT_CONTEXT_PUSH(c, char, ch); } while(0)

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static const char* lept_parse_hex4(const char* p, unsigned* u) {
    char temp[5];

    if(!(isxdigit(p[0]) && isxdigit(p[1]) && isxdigit(p[2]) && isxdigit(p[3]))) {
        return NULL;
    }

    for(size_t i = 0; i < 4; i ++) {
        temp[i] = p[i];
    }
    temp[4] = '\0';

    sscanf(temp, "%x", u);

    return p + 4;
}

static const char* lept_surrogate_handling(const char* p, unsigned* u) {
    unsigned u2;

    if(*u < 0xD800 || *u > 0xDBFF) {
        /* dont need surrogate */
        return p;
    }

    if(!(p[0] == '\\' && p[1] == 'u')) {
        /** 0xFFFFFFFF is illegal in UCS, we use it as error:
          * LEPT_PARSE_INVALID_UNICODE_SURROGATE */
        *(int*)u = -1;
        return NULL;
    }

    if((p = lept_parse_hex4(p+2, &u2)) == NULL) {
        /* LEPT_PARSE_INVALID_UNICODE_HEX */
        return NULL;
    }

    if(u2 < 0xDC00 || u2 > 0xDFFF) {
        /** 0xFFFFFFFF is illegal in UCS, we use it as error:
          * LEPT_PARSE_INVALID_UNICODE_SURROGATE */
        *(int*)u = -1;
        return NULL;
    }

    /* save codepoint */
    *u = 0x10000 + (*u - 0xD800) * 0x400 + (u2 - 0xDC00);

    return p;
}

static void lept_encode_utf8(lept_context* c, unsigned u) {
    if(u <= 0x7F) {
        PUTC(c, (unsigned char)u);
        return ;
    }

    if(u <= 0x7FF) {
        PUTC(c, 0xC0 | (u >> 6));
        PUTC(c, 0x80 | (u & 0x3F));
        return ;
    }

    if(u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
        return ;
    }

    if(u <= 0x10FFFF) {
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
        return ;
    }

    /* never reach here */
    assert(0);
}

static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p = c->json;
    unsigned u; /* codepoint */

    assert(*p++ == '\"');

    for(;;) {
        unsigned char ch = *p++;
        switch (ch) {
            case '\"':
                len = c->top - head;
                lept_set_string(v, (const char*)LEPT_CONTEXT_POP_ALL(c), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
                STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
            case '\\':
                ch = *p++;
                switch(ch) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/'); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    case 'u':
                        /* convert to codepoint and save it in u */
                        if((p = lept_parse_hex4(p, &u)) == NULL) {
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        }
                        /* surrogate handling */
                        if((p = lept_surrogate_handling(p, &u)) == NULL) {
                            if((int)u == -1) {
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            } else {
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                            }
                        }
                        /* convert to utf-8 and save it */
                        lept_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(LEPT_PARSE_INVALID_ESCAPE);
                }
                break;
            default:
                if((ch >= 0x20 && ch <= 0x21) || (ch >= 0x23 && ch <= 0x5B) || (ch >=0x5D)) {
                    PUTC(c, ch);
                } else {
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
                }
        }
    }
}

/* parse array */

#define PUTV(c, v) do { LEPT_CONTEXT_PUSH(c, lept_value, v); } while(0)

static int lept_parse_value(lept_context* c, lept_value* v);

static int lept_parse_array(lept_context* c, lept_value* v) {
    const char* p;
    int ret;

    assert(*(c->json)++ == '[');

    lept_parse_whitespace(c);
    if(*(c->json) == ']') {
        v->type = LEPT_ARRAY;
        v->e = NULL;
        v->size = 0;
        c->json ++;
        return LEPT_PARSE_OK;
    }

    while(1) {

        lept_value v2;
        lept_init(&v2);
        ret = lept_parse_value(c, &v2);
        if(ret != LEPT_PARSE_OK) {
            return ret;
        }
        PUTV(c, v2);

        /* handle ',' and ws */
        lept_parse_whitespace(c);
        if(*(p = c->json) == ']') {
            break;
        } else if(*p != ',') {
            return LEPT_PARSE_INVALID_VALUE;
        } else {
            /* continue handling this array */
            c->json ++;
        }
        lept_parse_whitespace(c);

    }

    /* copy to v->e */
    v->type = LEPT_ARRAY;
    v->size = c->top / sizeof(lept_value);
    v->e = (lept_value*)malloc(v->size * sizeof(lept_value));
    memcpy(v->e, LEPT_CONTEXT_POP_ALL(c), v->size * sizeof(lept_value));

    c->json ++;

    return ret;
}

/* parse */

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch(*c->json) {
        case '0': case '1': case '2': case '3': case '4':
        case '6': case '7': case '8': case '9': case '-':
            return lept_parse_number(c, v);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case '\"': return lept_parse_string(c, v);
        case '[':  return lept_parse_array(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return LEPT_PARSE_INVALID_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;

    assert(v != NULL);

    /* initialize */
    lept_init(v);
    lept_context_init(&c, json);
    ret = LEPT_PARSE_INVALID_VALUE;

    /* parse json */
    lept_parse_whitespace(&c);
    ret = lept_parse_value(&c, v);
    lept_parse_whitespace(&c);

    /* check end */
    if(ret == LEPT_PARSE_OK && *(c.json) != '\0') {
        ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
    }

    lept_context_free(&c);

    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

/* free */

void lept_init(lept_value* v) {
    memset(v, 0, sizeof(lept_value));
    v->type = LEPT_NULL;
}

void lept_free(lept_value* v) {
    assert(v != NULL);
    if(v->type == LEPT_STRING && v->len > 0) {
        free(v->s);
        v->s = NULL;
        v->len = 0;
    } else if(v->type == LEPT_ARRAY && v->size > 0) {
        for(size_t i = 0; i < v->size; i ++) {
            lept_free(&v->e[i]);
        }
        free(v->e);
        v->e = NULL;
        v->size = 0;
    }
    /* to avoid double free */
    v->type = LEPT_NULL;
}

/* string */

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len != 0));
    lept_free(v);
    v->s = (char*)malloc((len + 1) * sizeof(char));
    memcpy(v->s, s, len);
    v->s[len] = '\0';
    v->len = len;
    v->type = LEPT_STRING;
}

/* boolean */

int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = (b == 0) ? LEPT_FALSE : LEPT_TRUE;
}

/* number */

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}

void lept_set_number(lept_value* v, double d) {
    lept_free(v);
    v->n = d;
    v->type = LEPT_NUMBER;
}

/* array */

size_t lept_get_array_size(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index) {
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->size);
    return &(v->e[index]);
}
