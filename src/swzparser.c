#include<stdbool.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include<assert.h>

#include "swzlisp.h"

// typedef struct {...} Result;
typedef struct {
    SWZObject *ok;
    int index;
    enum SWZError err;
} Result;

// #RESULT_ERR(obj, idx, err)
#define SWZ_RESULT_ERR(obj, idx, error)   do {      \
    Result _result;                                 \
    _result.ok = (SWZObject *)(obj);                \
    _result.index = (idx);                          \
    _result.err = (error);                          \
    return _result;                                 \
} while (0)

// #RESULT_OK(obj, idx)
#define SWZ_RESULT_OK(obj, idx) SWZ_RESULT_ERR(obj, idx, 0)

#define SWZ_COMMENT     ';'

// _swz_parse_obj_[internal|helper](...)
static Result _swz_parse_helper(SWZRuntime *swz, char *src, int idx);

// _swz_parse_[integer|number](...)
static Result _swz_parse_number(SWZRuntime *swz, char *src, int idx){
    int n, rc;
    double num;
    rc = sscanf(src + idx, "%lg%n", &num, &n);
    if(rc != 1){
        swz->error = "SyntaxError: error parsing number";
        SWZ_RESULT_ERR(NULL, idx, SWZE_SYNTAX);
    }else{
        double diff = num - ((long)num);
        SWZFloat x, zero;
        x.type = swzFloat;
        x.val = diff;
        zero.type = swzFloat;
        zero.val = 0;
        if (swz_compare((SWZObject *)&x, (SWZObject *)&zero)){
            SWZInteger *result = (SWZInteger *)swz_alloc(swz, swzInteger);
            result->val = (long)num;
            SWZ_RESULT_OK(result, idx + n);
        }
        SWZFloat *result = (SWZFloat *)swz_alloc(swz, swzFloat);
        result->val = num;
        SWZ_RESULT_OK(result, idx + n);
    }
}

// _skip_space_and_comments(...)
static int _skip_space_and_comments(char *src, int idx){
    for(;;){
        while(isspace(src[idx])){
            idx++;
        }
        if(src[idx] && src[idx] == SWZ_COMMENT){
            while(src[idx] && src[idx] != '\n'){
                idx++;
            }
        }else{
            return idx;
        }
    }
}

// _swz_escape(...)
static char _swz_escape(char c){
    switch(c){
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    default:
        return c;
    }
}

// _swz_parse_string(...)
static Result _swz_parse_string(SWZRuntime *swz, char *src, int idx){
    int i;
    CharBuffer cbuffer;
    SWZString *str;

    i = idx + 1;
    cbuffer_init(&cbuffer, 16);
    while(src[i] && src[i] != '"'){
        if(src[i] == '\\'){
            cbuffer_append(&cbuffer, _swz_escape(src[++i]));
        }else{
            cbuffer_append(&cbuffer, src[i]);
        }
        i++;
    }

    if(!src[i]){
        cbuffer_dealloc(&cbuffer);
        swz->error = "unexpected eof while parsing string";
        SWZ_RESULT_ERR(NULL, i, SWZE_SYNTAX);
    }
    cbuffer_trim(&cbuffer);
    str = (SWZString *)swz_alloc(swz, swzString);
    str->cstr = cbuffer.buffer;
    str->can_free = 1;
    i++;
    SWZ_RESULT_OK(str, i);
}

// _swz_parse_list_or_sexpr(...)
static Result _swz_parse_list_or_sexpr(SWZRuntime *swz, char *src, int idx){
    Result result;
    SWZList *self, *obj;
    idx = _skip_space_and_comments(src, idx);
    if(!src[idx]){
        swz->error = "unexpected eof while parsing list";
        SWZ_RESULT_ERR(NULL, idx, SWZE_EOF);
    }else if(src[idx] == ')'){
        SWZ_RESULT_OK(swz_alloc_nil(swz), idx + 1);
    }

    result = _swz_parse_helper(swz, src, idx);
    if(result.err){
        return result;
    }else if(!result.ok){
        SWZ_RESULT_ERR(NULL, result.index, 1);
    }
    idx = result.index;
    self = (SWZList *)swz_alloc(swz, swzList);
    self->car = result.ok;
    obj = self;

    while(true){
        idx = _skip_space_and_comments(src, idx);
        if(!src[idx]){
            swz->error = "unexpected eof while parsing list";
            SWZ_RESULT_ERR(NULL, idx, SWZE_EOF);
        }else if(src[idx] == '.'){
            idx++;
            result = _swz_parse_helper(swz, src, idx);
            if(result.err){
                return result;
            }else if(!result.ok){
                SWZ_RESULT_ERR(NULL, result.index, 1);
            }
            idx = result.index;
            obj->cdr = result.ok;
            // -
            idx = _skip_space_and_comments(src, idx);
            if(src[idx] != ')'){
                swz->error = "bad s-expression form";
                SWZ_RESULT_ERR(NULL, idx, SWZE_SYNTAX);
            }
            idx++;
            SWZ_RESULT_OK(self, idx);
        }else if(src[idx] == ')'){
            idx++;
            obj->cdr = swz_alloc_nil(swz);
            SWZ_RESULT_OK(self, idx);
        }else{
            result = _swz_parse_helper(swz, src, idx);
            if(result.err){
                return result;
            }else if(!result.ok){
                SWZ_RESULT_ERR(NULL, result.index, 1);
            }
            obj->cdr = swz_alloc(swz, swzList);
            obj = (SWZList *)result.ok;
            idx = result.index;
        }
    }
}

// _split_symbol(...)
static SWZObject* _split_symbol(SWZRuntime *swz, char *src, int ndot, int remain){
    char *delim;
    char *token;
    int i, len;
    SWZObject *prev = NULL;
    SWZSymbol *symbol;
    SWZList *self;

    SWZSymbol *getattr = swz_alloc_symbol(swz, "getattr", 0);

    // -
    delim = strchr(src, '.');
    len = (delim - src);
    token = malloc(len + 1);
    strncpy(token, src, len);
    token[len] = '\0';
    symbol = swz_alloc_symbol(swz, token, SWZFLAG_OWN);
    free(token);
    prev = (SWZObject *)symbol;
    src = delim + 1;
    remain += len + 1;

    // -
    for (i = 0; i < ndot; i++){
        // -
        if(i < ndot - 1){
            delim = strchr(src, '.');
            len = (int)(delim - src);
        }else{
            len = remain;
        }
        token = malloc(len + 1);
        strncpy(token, src, len);
        token[len] = '\0';
        symbol = swz_alloc_symbol(swz, token, SWZFLAG_OWN);
        free(token);

        // -
        self = swz_alloc_list(
            swz, (SWZObject*)getattr,
            (SWZObject*)swz_alloc_list(swz, prev,
                (SWZObject*)swz_alloc_list(swz,
                    (SWZObject*)swz_quote_with(swz, (SWZObject*)symbol, "quote"),
                    swz_alloc_nil(swz)
                )
            )
        );
        prev = (SWZObject*)self;
        src = delim + 1;
        remain -= len + 1;
    }

    return prev;
}

// _swz_parse_symbol(...)
// _swz_parse_quote(...)
// _swz_parse_obj_[internal|helper](...)
// _set_error_lineno(...)
// _read_file(...)
// swz_parse_obj(...)
// swz_parse_progn(...)
// swz_parse_progn_f()
// swz_load_file(...)