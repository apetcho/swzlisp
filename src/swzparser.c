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
// _swz_escape(...)
// _swz_parse_string(...)
// _swz_parse_list_or_sexpr(...)
// _split_symbol(...)
// _swz_parse_symbol(...)
// _swz_parse_quote(...)
// _swz_parse_obj_[internal|helper](...)
// _set_error_lineno(...)
// _read_file(...)
// swz_parse_obj(...)
// swz_parse_progn(...)
// swz_parse_progn_f()
// swz_load_file(...)