#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<ctype.h>
#include<assert.h>

#include "swzlisp.h"

// typedef struct {...} Result;
typedef struct {
    SWZObject *ok;
    int index;
    enum SWZError err;
} Result;

// #RESULT_ERR(obj, idx, err)
#define SWZ_RESULT_ERR(obj, idx, err)   do {    \
    Result _result;                             \
    _result.ok = (SWZObject *)(obj);            \
    _result.index = (idx);                      \
    _result.err = (err);                        \
    return _result;                             \
} while (0)

// #RESULT_OK(obj, idx)
#define SWZ_RESULT_OK(obj, idx) SWZ_RESULT_ERR(obj, idx, 0)

#define SWZ_COMMENT     ';'

// _swz_parse_obj_[internal|helper](...)
// _swz_parse_[integer|number](...)
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