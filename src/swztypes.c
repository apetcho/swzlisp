#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include<float.h>
#include<limits.h>
#include<math.h>
#include<stdio.h>

#include "swzlisp.h"

#define SWZ_TYPE_HEADER     \
    &_swzType,              \
    NULL,                   \
    'w'


// -*-
void* _my_alloc(size_t size){
    void *result = malloc(size);
    if(result == NULL){
        fprintf(stderr, "Error: memory allocation failure\n");
        abort();
    }
    memset(result, 0, size);
    return result;
}

// -*-
static SWZObject* _swz_eval_error(SWZRuntime *swz, SWZEnv *env, SWZObject *obj){
    (void)env;
    (void)obj;
    return swz_error(swz, SWZE_EVAL, "cannot evaluate this object");
}

// -*-
static SWZObject* _swz_eval_same(SWZRuntime *swz, SWZEnv *env, SWZObject *obj){
    (void)swz;
    (void)env;
    return obj;
}

// -*-
static SWZObject* _swz_call_error(SWZRuntime *swz, SWZEnv *env, SWZObject *obj, SWZList *list){
    (void)env;
    (void)obj;
    (void)list;
    return swz_error(swz, SWZE_CALL, "not callable!");
}

// -*-
static void _swz_simple_dealloc(SWZRuntime *swz, void *arg){
    (void)swz;
    free(arg);
}

// -*-
static bool _hash_next_index_lt_state(Iterator *iter){
    return iter->index < iter->stateIdx;
}

// -*-----------*-
// -*- SWZType -*-
// -*-----------*-
static void _swz_type_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_type_alloc(SWZRuntime *swz);
static bool _swz_type_compare(const SWZObject *self, const SWZObject *other);
static Iterator _swz_empty_iterator(SWZObject* obj);

static SWZType _swzType = {
    SWZ_TYPE_HEADER,
    "type",                 // .name
    _swz_type_print,        // .print()
    _swz_type_alloc,        // .alloc()
    _swz_simple_dealloc,    // .dealloc()
    _swz_empty_iterator,    // .iter()
    _swz_eval_error,        // .eval()
    _swz_call_error,        // .call()
    _swz_type_compare,      // .compare()
};

SWZType *swzType = &_swzType;

// -*-
static void _swz_type_print(FILE *stream, SWZObject *obj){
    SWZType *self = (SWZType *)obj;
    fprintf(stream, "%s", self->name);
}

// -*-
static SWZObject *_swz_type_alloc(SWZRuntime *swz){
    SWZType *self = NULL;
    SWZ_UNUSED(swz);
    self = _my_alloc(sizeof(SWZType));
    return (SWZObject *)self;
}

// -*-
static bool _swz_type_compare(const SWZObject *self, const SWZObject *other){
    if(self->type != other->type || self->type != swzType){
        return false;
    }
    return self == other;
}

static Iterator _swz_empty_iterator(SWZObject* obj){
    SWZ_UNUSED(obj);
    return iterator_empty();
}

// -*----------*-
// -*- SWZEnv -*-
// -*----------*-
static void _swz_env_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_env_alloc(SWZRuntime *swz);
static void _swz_env_dealloc(SWZRuntime *swz, void *arg);
static Iterator _swz_env_iter(SWZObject *obj);
static bool _swz_env_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzenv = {
    SWZ_TYPE_HEADER,
    "env",              // .name
    _swz_env_print,     // .print()
    _swz_env_alloc,     // .alloc()
    _swz_env_dealloc,   // .dealloc()
    _swz_env_iter,      // .iter()
    _swz_eval_error,    // .eval()
    _swz_call_error,    // .call()
    _swz_env_compare,   // .compare()
};

SWZType *swzEnv = &_swzenv;

// -*-
static uint32_t _swz_text_hash(void *arg){
    struct swztext **text = (struct swztext **)arg;
    return htable_string_hash(&(*text)->cstr);
}

// -*-
static bool _swz_text_compare(const void *lhs, const void *rhs){
    SWZSymbol *sym1 = *(SWZSymbol **)lhs;
    SWZSymbol *sym2 = *(SWZSymbol **)rhs;
    return strcmp(sym1->cstr, sym2->cstr) == 0;
}

// -*-
static SWZObject *_swz_env_alloc(SWZRuntime *swz){
    SWZ_UNUSED(swz);
    SWZEnv *env = NULL;
    env = _my_alloc(sizeof(SWZEnv));
    env->parent = NULL;
    htable_init(
        &env->scope,
        _swz_text_hash,
        _swz_text_compare,
        sizeof(void *),
        sizeof(void *)
    );

    return (SWZObject *)env;
}

static void _swz_env_dealloc(SWZRuntime *swz, void *arg){
    SWZ_UNUSED(swz);
    SWZEnv *env = NULL;
    env = (SWZEnv *)arg;
    htable_dealloc(&env->scope);
    free(env);
}

// -*-
static void _swz_env_print(FILE *stream, SWZObject *obj){
    SWZEnv *env = (SWZEnv *)obj;
    Iterator iterator = htable_iterator_keys_ptr(&env->scope);
    fprintf(stream, "(scope:");
    while(iterator.has_next(&iterator)){
        SWZObject *key = iterator.next(&iterator);
        SWZObject *value = htable_get_ptr(&env->scope, key);
        fprintf(stream, " ");
        swz_print(stream, key);
        fprintf(stream, ": ");
        swz_print(stream, value);
    }
    fprintf(stream, ")");
}

// -*-
static Iterator _swz_env_iter(SWZObject *obj){
    SWZEnv *env = (SWZEnv *)obj;
    if(env->parent){
        return iterator_concat3(
            iterator_single_value(env->parent),
            htable_iterator_keys_ptr(&env->scope),
            htable_iterator_values_ptr(&env->scope)
        );
    }else{
        return iterator_concat2(
            htable_iterator_keys_ptr(&env->scope),
            htable_iterator_values_ptr(&env->scope)
        );
    }
}

// -*-
static bool _swz_env_compare(const SWZObject *self, const SWZObject *other){
    SWZEnv *lhsEnv = NULL;
    SWZEnv *rhsEnv = NULL;
    SWZSymbol *key = NULL;
    SWZObject *value = NULL;
    SWZObject *rhs = NULL;
    Iterator iterator = {0};

    // - quick checks
    if(self == other){
        return true;
    }
    if(self->type != other->type || self->type != swzEnv){
        return false;
    }
    // -
    lhsEnv = (SWZEnv *)self;
    rhsEnv = (SWZEnv *)other;
    // - now actual equality
    if(lhsEnv->parent && rhsEnv->parent){
        // - both parent non-NULL
        if(!_swz_env_compare((SWZObject*)lhsEnv->parent, (SWZObject*)rhsEnv->parent)){
            return false;
        }
    }else if(lhsEnv->parent || rhsEnv->parent){
        return false;
    }

    // - now equality check of scope contents
    if(htable_length(&lhsEnv->scope) != htable_length(&rhsEnv->scope)){
        return false;
    }

    iterator = htable_iterator_keys_ptr(&lhsEnv->scope);
    while(iterator.has_next(&iterator)){
        key = (SWZSymbol *)iterator.next(&iterator);
        rhs = (SWZObject *)htable_get_ptr(&rhsEnv->scope, key);
        if(!rhs){
            iterator.close(&iterator);
            return false;
        }
        value = (SWZObject *)htable_get_ptr(&lhsEnv->scope, key);
        if(!swz_compare(value, rhs)){
            iterator.close(&iterator);
            return false;
        }
    }

    // -
    iterator.close(&iterator);
    return true;
}

// -*-----------*-
// -*- SWZList -*-
// -*-----------*-
static void _swz_list_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_list_alloc(SWZRuntime *swz);
static SWZObject *_swz_list_eval(SWZRuntime *swz, SWZEnv *env, SWZObject *obj);
static Iterator _swz_list_iter(SWZObject *obj);
static bool _swz_list_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzlist = {
    SWZ_TYPE_HEADER,
    "list",                 // .name
    _swz_list_print,        // .print()
    _swz_list_alloc,        // .alloc()
    _swz_simple_dealloc,    // .dealloc()
    _swz_list_iter,         // .iter()
    _swz_list_eval,         // .eval()
    _swz_call_error,        // .call()
    _swz_list_compare,      // .compare()
};

SWZType *swzList = &_swzlist;

// -*-
static SWZObject *_swz_list_eval(SWZRuntime *swz, SWZEnv *env, SWZObject *obj){
    SWZObject *callable;
    SWZList *list = (SWZList *)obj;

    if(swz_nil_p(obj)){
        return swz_error(swz, SWZE_CALL, "Cannot call empty list");
    }
    if(list->cdr->type != swzList){
        return swz_error(swz, SWZE_SYNTAY, "unexpected cons cell");
    }
    callable = swz_eval(swz, env, list->car);
    return swz_call(swz, env, callable, (SWZList *)list->cdr);
}

// -*-
static void _swz_list_print_helper(FILE *stream, SWZList *list){
    if(swz_nil_p((SWZObject*)list)){
        return;
    }
    swz_print(stream, list->car);
    if(list->cdr->type != swzList){
        fprintf(stream, " . ");
        swz_print(stream, list->cdr);
        return;
    }else if(!swz_nil_p((SWZObject*)list)){
        fprintf(stream, " ");
        _swz_list_print_helper(stream, (SWZList *)list->cdr);
    }
}
// -*-
static void _swz_list_print(FILE *stream, SWZObject *obj){
    fprintf(stream, "(");
    _swz_list_print_helper(stream, (SWZList *)obj);
    fprintf(stream, ")");
}

// -*-
static SWZObject *_swz_list_alloc(SWZRuntime *swz){
    SWZ_UNUSED(swz);
    SWZList *list = NULL;
    list = _my_alloc(sizeof(SWZList));
    list->car = NULL;
    list->cdr = NULL;
    return (SWZObject *)list;
}

// -*-
bool swz_nil_p(SWZObject *obj){
    return (
        (obj->type == swzList) &&
        (((SWZList*)obj)->car == NULL) &&
        (((SWZList*)obj)->cdr == NULL)
    );
}

// -*-
static void* _swz_list_iter_next(Iterator *iterator){
    SWZList *list = (SWZList *)iterator->data;
    iterator->index++;
    SWZObject *result = NULL;
    switch (iterator->index){
    case 1:
        result = list->car;
        break;
    case 2:
        result = list->cdr;
        break;
    default:
        break;
    }
    return result;
}

// -*-
static bool _swz_list_has_next(Iterator *iterator){
    SWZObject *list = (SWZObject *)iterator->data;
    if(swz_nil_p(list)){
        return true;
    }
    return iterator->index < iterator->stateIdx;
}

// -*-
static Iterator _swz_list_iter(SWZObject *obj){
    Iterator iterator = {0};
    iterator.data = obj;
    iterator.stateIdx = 2;
    iterator.next = _swz_list_iter_next;
    iterator.close = iterator_close_noop;
    iterator.has_next = _swz_list_has_next;
    return iterator;
}

// -*-
static bool _swz_list_compare(const SWZObject *self, const SWZObject *other){
    if(self == other){
        return true;
    }
    if(self->type != other->type || self->type != swzList){
        return false;
    }
    SWZList *lhs = (SWZList *)self;
    SWZList *rhs = (SWZList *)other;
    SWZObject *x = (SWZObject *)self;
    SWZObject *y = (SWZObject *)other;
    if (swz_nil_p(x) && swz_nil_p(y)){
        return true;
    }
    if(swz_nil_p(x) || swz_nil_p(y)){
        return false;
    }
    return (
        swz_compare(lhs->car, rhs->car) && swz_compare(rhs->cdr, rhs->cdr)
    );
}

// -*-------------*-
// -*- SWZSymbol -*-
// -*-------------*-
static void _swz_txt_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_txt_alloc(SWZRuntime *swz);
static SWZObject *_swz_symbol_eval(SWZRuntime *swz, SWZEnv *env, SWZObject *obj);
static void _swz_txt_dealloc(SWZRuntime *swz, void *arg);
static bool _swz_txt_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzsymbol = {
    SWZ_TYPE_HEADER,
    "symbol",               // .name
    _swz_txt_print,         // .print()
    _swz_txt_alloc,         // .alloc()
    _swz_txt_dealloc,       // .dealloc()
    _swz_empty_iterator,    // .iter()
    _swz_symbol_eval,       // .eval()
    _swz_call_error,        // .call()
    _swz_txt_compare,       // .compare()
};

SWZType *swzSymbol = &_swzsymbol;

// -*-
static void _swz_txt_print(FILE *stream, SWZObject *obj){
    struct swztext *text = (struct swztext *)obj;
    fprintf(stream, "%s", text->cstr);
}

// -*-
static SWZObject *_swz_txt_alloc(SWZRuntime *swz){
    struct swztext *text = NULL;
    SWZ_UNUSED(swz);
    text = _my_alloc(sizeof(struct swztext));
    text->cstr = NULL;
    text->can_free = 1;
    return (SWZObject *)text;
}

// -*-
static void _swz_txt_dealloc(SWZRuntime *swz, void *arg){
    struct swztext *text = (struct swztext *)arg;
    if(text->type == swzString && swz->strcache){
        swz_textchach_remove(swz->strcache, text);
    }else if(text->type == swzSymbol && swz->symcache){
        swz_textchach_remove(swz->symcache, text);
    }

    // - deal with the ownership
    if(text->can_free){
        free(text->cstr);
    }
    free(text);
}

// -*-
static SWZObject *_swz_symbol_eval(SWZRuntime *swz, SWZEnv *env, SWZObject *obj){
    SWZ_UNUSED(swz);
    SWZSymbol *symbol = NULL;
    symbol = (SWZSymbol *)obj;
    return swz_env_lookup(swz, env, symbol);
}

// -*-
static bool _swz_txt_compare(const SWZObject *self, const SWZObject *other){
    if(self == other){
        return true;
    }
    if(self->type != other->type){
        return false;
    }
    struct swztext *lhs = (struct swztext *)self;
    struct swztext *rhs = (struct swztext *)other;
    return strcmp(lhs->cstr, rhs->cstr) == 0;
}

// -*--------------*-
// -*- SWZInteger -*-
// -*--------------*-
static void _swz_integer_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_integer_alloc(SWZRuntime *swz);
static bool _swz_integer_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzinteger = {
    SWZ_TYPE_HEADER,
    "integer",              // .name
    _swz_integer_print,     // .print()
    _swz_integer_alloc,     // .alloc()
    _swz_simple_dealloc,    // .dealloc()
    _swz_empty_iterator,    // .iter()
    _swz_eval_same,         // .eval()
    _swz_call_error,        // .call()
    _swz_integer_compare,   // .compare()
};

SWZType *swzInteger = &_swzinteger;

// -*-
static void _swz_integer_print(FILE *stream, SWZObject *obj){
    SWZInteger *self = (SWZInteger *)obj;
    fprintf(stream, "%ld", self->val);
}

// -*-
static SWZObject *_swz_integer_alloc(SWZRuntime *swz){
    SWZ_UNUSED(swz);
    SWZInteger *self = _my_alloc(sizeof(SWZInteger));
    self->val = 0;
    return (SWZObject *)self;
}

// -*-
static bool _swz_is_integer(SWZObject *obj){
    if(!obj){
        return false;
    }
    if(obj->type != swzInteger){
        return false;
    }
    return true;
}

// -*-
static bool _swz_is_float(SWZObject *obj){
    if(!obj){
        return false;
    }
    if(obj->type != swzFloat){
        return false;
    }
    return true;
}

// -*-
static bool _swz_is_number(SWZObject *obj){
    return (_swz_is_integer(obj) || _swz_is_float(obj));
}

// -
static bool _swz_almostEqual(double x, double y){
    static double eps = DBL_EPSILON;
    double diff = fabs(x - y);
    x = fabs(x);
    y = fabs(y);
    double xymax = (x > y) ? x : y;
    bool result = (diff <= xymax*eps) ? true: false;
    return result;
}

// -*-
static bool _swz_num_compare(SWZObject *self, SWZObject *other){
    if(self == other){
        return true;
    }
    if(!_swz_is_number(self)){
        return false;
    }
    if(!_swz_is_number(other)){
        return false;
    }
    if(_swz_is_integer(self)){
        SWZInteger *lhs = (SWZInteger *)self;
        if(_swz_is_integer(other)){
            SWZInteger *rhs = (SWZInteger *)other;
        }else{
            double x = (double)lhs->val;
            double y = ((SWZFloat *)other)->val;
            return _swz_almostEqual(x, y);
        }
    }
    // self is a SWZFloat
    if(_swz_is_integer(other)){
        double x = ((SWZFloat *)self)->val;
        double y = (double)((SWZInteger *)other)->val;
        return _swz_almostEqual(x, y);
    }
    // both are SWZFloat
    double x = ((SWZFloat *)self)->val;
    double y = ((SWZFloat *)other)->val;
    return _swz_almostEqual(x, y);
}

// -*-
static bool _swz_integer_compare(const SWZObject *self, const SWZObject *other){
    return _swz_num_compare((SWZObject*)self, (SWZObject*)other);
}

// -*------------*-
// -*- SWZFloat -*-
// -*------------*-
static void _swz_float_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_float_alloc(SWZRuntime *swz);
static bool _swz_float_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzfloat = {
    SWZ_TYPE_HEADER,
    "float",                // .name
    _swz_float_print,       // .print()
    _swz_float_alloc,       // .alloc()
    _swz_simple_dealloc,    // .dealloc()
    _swz_empty_iterator,    // .iter()
    _swz_eval_same,         // .eval()
    _swz_call_error,        // .call()
    _swz_float_compare,     // .compare()
};

SWZType *swzFloat = &_swzfloat;

// -*-
static void _swz_float_print(FILE *stream, SWZObject *obj){
    SWZFloat *self = (SWZFloat *)obj;
    fprintf(stream, "%lf", self->val);
}

// -*-
static SWZObject *_swz_float_alloc(SWZRuntime *swz){
    SWZ_UNUSED(swz);
    SWZFloat *self = _my_alloc(sizeof(SWZFloat));
    self->val = (double)0;
    return (SWZObject *)self;
}

// -*-
static bool _swz_float_compare(const SWZObject *self, const SWZObject *other){
    return _swz_num_compare((SWZObject*)self, (SWZObject*)other);
}

// -*-
bool swz_is_number(SWZRuntime *swz, SWZObject *obj){
    SWZ_UNUSED(swz);
    return _swz_is_number(obj);
}

// -*-
bool swz_is_integer(SWZRuntime *swz, SWZObject *obj){
    SWZ_UNUSED(swz);
    return _swz_is_integer(obj);
}

// -*-
bool swz_is_float(SWZRuntime *swz, SWZObject *obj){
    SWZ_UNUSED(swz);
    return _swz_is_float(obj);
}

// -*-------------*-
// -*- SWZString -*-
// -*-------------*-
static SWZType _swzstring = {
    SWZ_TYPE_HEADER,
    "string",               // .name
    _swz_txt_print,         // .print()
    _swz_txt_alloc,         // .alloc()
    _swz_txt_dealloc,       // .dealloc()
    _swz_empty_iterator,    // .iter()
    _swz_eval_same,         // .eval()
    _swz_call_error,        // .call()
    _swz_txt_compare,       // .compare()
};

SWZType *swzString = &_swzstring;

// -*--------------*-
// -*- SWZBuiltin -*-
// -*--------------*-
static void _swz_builtin_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_builtin_alloc(SWZRuntime *swz);
static SWZObject *_swz_builtin_call(SWZRuntime* swz, SWZEnv *env, SWZObject *callable, SWZList *args);
static bool _swz_builtin_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzbuiltin = {
    SWZ_TYPE_HEADER,
    "builtin",              // .name
    _swz_builtin_print,     // .print()
    _swz_builtin_alloc,     // .alloc()
    _swz_simple_dealloc,    // .dealloc()
    _swz_empty_iterator,    // .iter()
    _swz_eval_error,        // .eval()
    _swz_builtin_call,      // .call()
    _swz_builtin_compare,   // .compare()
};

SWZType *swzBuiltin = &_swzbuiltin;

// -*-
static void _swz_builtin_print(FILE *stream, SWZObject *obj){
    SWZBuiltin *builtin = (SWZBuiltin *)obj;
    fprintf(stream, "<builtin function '%s' @ 0x%p>", builtin->name, (void*)builtin);
}

// -*-
static SWZObject *_swz_builtin_alloc(SWZRuntime *swz){
    SWZ_UNUSED(swz);
    SWZBuiltin *builtin = _my_alloc(sizeof(SWZBuiltin));
    builtin->fun = NULL;
    builtin->name = NULL;
    builtin->evald = 0;
    return (SWZObject *)builtin;
}

// -*-
static SWZObject *_swz_builtin_call(SWZRuntime* swz, SWZEnv *env, SWZObject *callable, SWZList *args){
    SWZBuiltin *builtin = (SWZBuiltin *)callable;
    if(builtin->evald){
        args = swz_eval_list(swz, env, args);
        SWZ_IS_VALID_PTR(args);
    }else if (swz_is_bad_list(args)){
        return swz_error(swz, SWZE_SYNTAY, "unexpected cons cell");
    }
    return builtin->fun(swz, env, args, builtin->params);
}

// -*-
static bool _swz_builtin_compare(const SWZObject *self, const SWZObject *other){
    if(self == other){
        return true;
    }
    if(self->type != other->type || self->type != swzBuiltin){
        return false;
    }
    SWZBuiltin *lhs = (SWZBuiltin *)self;
    SWZBuiltin *rhs = (SWZBuiltin *)other;

    return (
        (lhs->fun==rhs->fun) &&
        (lhs->params==rhs->params) &&
        (strcmp(lhs->name, rhs->name)==0)
    );
}

// -*-------------*-
// -*- SWZLambda -*-
// -*-------------*-
static void _swz_lambda_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_lambda_alloc(SWZRuntime *swz);
static SWZObject *_swz_lambda_call(SWZRuntime *swz, SWZEnv *env, SWZObject *callable, SWZList *args);
static Iterator _swz_lambda_iter(SWZObject *obj);
static bool _swz_lambda_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzlambda = {
    SWZ_TYPE_HEADER,
    "lambda",               // .name
    _swz_lambda_print,      // .print()
    _swz_lambda_alloc,      // .alloc()
    _swz_simple_dealloc,    // .dealloc()
    _swz_lambda_iter,       // .iter()
    _swz_eval_error,        // .eval()
    _swz_lambda_call,       // .call()
    _swz_lambda_compare,    // .compare()
};

SWZType *swzLambda = &_swzlambda;

// -*-
static void _swz_lambda_print(FILE *stream, SWZObject *obj){
    SWZLambda *lambda = (SWZLambda *)obj;
    char *cstr = lambda->name ? lambda->name->cstr : "anonymous";
    if(lambda->kind == SWZK_LAMBDA){
        fprintf(stream, "<lambda '%s' @ 0x%p>", cstr, (void*)lambda);
    }else{
        fprintf(stream, "<macro '%s' @ 0x%p>", cstr, (void*)lambda);
    }
}

// -*-
static SWZObject *_swz_lambda_alloc(SWZRuntime *swz){
    SWZ_UNUSED(swz);
    SWZLambda *lambda = _my_alloc(sizeof(SWZLambda));
    lambda->params = NULL;
    lambda->body = NULL;
    lambda->env = NULL;
    lambda->name = NULL;
    lambda->kind = SWZK_LAMBDA;
    return (SWZObject *)lambda;
}

// -*-
static SWZObject *_swz_lambda_call(SWZRuntime *swz, SWZEnv *env, SWZObject *callable, SWZList *args){
    SWZLambda *lambda = (SWZLambda *)callable;
    SWZList *argv;
    SWZList *iter1;
    SWZList *iter2;
    SWZEnv *inner;
    SWZObject *result;

    if(lambda->kind == SWZK_MACRO){
        argv = args;
    }else{
        argv = swz_eval_list(swz, env, args);
        SWZ_IS_VALID_PTR(argv);
    }

    if(swz_is_bad_list(argv)){
        return swz_error(swz, SWZE_SYNTAY, "unexpected cons cell");
    }

    inner = (SWZEnv *)swz_alloc(swz, swzEnv);
    inner->parent = lambda->env;
    iter1 = lambda->params;
    iter2 = argv;
    while(!swz_nil_p((SWZObject*)iter1) && !swz_nil_p((SWZObject*)iter2)){
        swz_env_bind(inner, (SWZSymbol *)iter1->car, iter2->car);
        iter1 = (SWZList *)iter1->cdr;
        iter2 = (SWZList *)iter2->cdr;
    }
    if(!swz_nil_p((SWZObject*)iter1)){
        return swz_error(swz, SWZE_TOO_FEW, "not enought argument to lambda call");
    }

    if(!swz_nil_p((SWZObject*)iter2)){
        return swz_error(swz, SWZE_TOO_MANY, "too many arguments to lambda call");
    }

    result = swz_progn(swz, inner, lambda->body);
    if(lambda->kind == SWZK_MACRO){
        result = swz_eval(swz, env, result);
    }

    return result;
}

// -*-
static void *_swz_lambda_iter_next(Iterator *iterator){
    SWZLambda *lambda = (SWZLambda *)iterator->data;
    iterator->index++;
    void *result = NULL;
    switch (iterator->index){
    case 1:
        result = lambda->params;
        break;
    case 2:
        result = lambda->body;
        break;
    case 3:
        result = lambda->env;
        break;
    case 4:
        result = lambda->name;
        break;
    default:
        break;
    }
    return result;
}

// -*-
static Iterator _swz_lambda_iter(SWZObject *obj){
    SWZLambda *lambda = (SWZLambda *)obj;
    Iterator iterator = {0};
    iterator.data = obj;
    iterator.stateIdx = lambda->name ? 4 : 3;
    iterator.index = 0;
    iterator.next = _swz_lambda_iter_next;
    iterator.close = iterator_close_noop;
    iterator.has_next = _hash_next_index_lt_state;
    return iterator;
}

// -*-
static bool _swz_lambda_compare(const SWZObject *self, const SWZObject *other){
    if(self == other){
        return true;
    }

    if(self->type != other->type || self->type != swzLambda){
        return false;
    }

    SWZLambda *lhs = (SWZLambda *)self;
    SWZLambda *rhs = (SWZLambda *)other;

    return (
        lhs->kind == rhs->kind &&
        swz_compare((SWZObject*)lhs->params, (SWZObject*)rhs->params) &&
        swz_compare((SWZObject*)lhs->body, (SWZObject*)rhs->body) &&
        swz_compare((SWZObject*)lhs->env, (SWZObject*)rhs->env)
    );
}

// -*------------------*-
// -*- shortcuts APIs -*-
// -*------------------*-
void swz_print(FILE *stream, SWZObject *obj){
    obj->type->print(stream, obj);
}

// -*-
void swz_dealloc(SWZRuntime *swz, SWZObject *obj){
    obj->type->dealloc(swz, obj);
}

// -*-
SWZObject* swz_eval(SWZRuntime *swz, SWZEnv *env, SWZObject *obj){
    return obj->type->eval(swz, env, obj);
}

// -*-
SWZObject *swz_call(SWZRuntime *swz, SWZEnv *env, SWZObject *callable, SWZList *args){
    SWZObject *result = NULL;

    // create a new stack frame
    swz->stack = swz_alloc_list(swz, callable, (SWZObject *)swz->stack);
    swz->sdepth++;

    // make function call;
    result = callable->type->call(swz, env, callable, args);

    // get rid of stack frame
    swz->stack = (SWZList *)swz->stack->cdr;
    swz->sdepth--;
    return result;
}

// -*-
SWZObject *swz_alloc(SWZRuntime *swz, SWZType *type){
    SWZObject *obj = type->alloc(swz);
    obj->type = type;
    obj->next = NULL;
    obj->mark = SWZ_GC_NOMARK;
    if(swz->head == NULL){
        swz->head = obj;
        swz->tail = obj;
    }else{
        swz->tail->next = obj;
        swz->tail = obj;
    }
    return obj;
}

// -*-
bool swz_compare(const SWZObject *self, const SWZObject *other){
    return self->type->compare(self, other);
}

// -*-------------*-
// -*- SWZModule -*-
// -*-------------*-
static void _swz_module_print(FILE *stream, SWZObject *obj);
static SWZObject *_swz_module_alloc(SWZRuntime *swz);
static Iterator _swz_module_iter(SWZObject *obj);
static bool _swz_module_compare(const SWZObject *self, const SWZObject *other);

static SWZType _swzmodule = {
    SWZ_TYPE_HEADER,
    "module",               // .name
    _swz_module_print,      // .print()
    _swz_module_alloc,      // .alloc()
    _swz_simple_dealloc,    // .deallo()
    _swz_module_iter,       // .iter()
    _swz_eval_error,        // .eval()
    _swz_call_error,        // .call()
    _swz_module_compare,    // .compare()
};

SWZType *swzModule = &_swzmodule;

// -*-
static SWZObject *_swz_module_alloc(SWZRuntime *swz){
    SWZ_UNUSED(swz);
    SWZModule* module = _my_alloc(sizeof(SWZModule));
    module->env = NULL;
    module->name = NULL;
    module->path = NULL;
    return (SWZObject*)module;
}

// -*-
static void _swz_module_print(FILE *stream, SWZObject *obj){
    SWZModule *module = (SWZModule *)obj;
    fprintf(
        stream, "<module '%s' from '%s' at 0x%p>",
        module->name->cstr, module->path->cstr, (void*)module
    );
}