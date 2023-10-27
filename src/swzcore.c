#include "swzlisp.h"
#include<stdlib.h>
#include<string.h>

void* _my_alloc(size_t size){
    void *result = malloc(size);
    if(result == NULL){
        fprintf(stderr, "Error: memory allocation failure\n");
        abort();
    }
    memset(result, 0, size);
    return result;
}
// -*-------------*-
// -*- utilities -*-
// -*-------------*-
const char* const swzVersion = SWZ_VERSION_FULL;

#define SWZE_DEF(code, id, name, desc) [code] = name,
const char* swzErrorNames[SWZE_COUNT] = {
    SWZ_ERRORS
};
#undef SWZE_DEF

// -*-
void swz_env_bind(SWZEnv *env, SWZSymbol *symbol, SWZObject *obj){
    htable_insert_ptr(&env->scope, symbol, obj);

    SWZLambda *lambda = NULL;
    if(obj->type == swzLambda){
        lambda = (SWZLambda *)obj;
        if(!lambda->name){
            lambda->name = symbol;
        }
    }
}

// -*-
SWZObject* swz_env_lookup(SWZRuntime *swz, SWZEnv *env, SWZSymbol *symbol){
    SWZObject *obj = htable_get_ptr(&env->scope, symbol);
    if(!obj){
        if(env->parent){
            return swz_env_lookup(swz, env->parent, symbol);
        }else{
            return swz_error(swz, SWZE_NOT_FOUND, "symbol not found in scope");
        }
    }else{
        return obj;
    }
}

// -*-
SWZObject *swz_env_lookup_string(SWZRuntime *swz, SWZEnv *env, const char* key){
    // -
    SWZSymbol symbol;
    symbol.type = swzSymbol;
    symbol.cstr = (char*)key;
    return swz_env_lookup(swz, env, &symbol);
}

// -*-
void swz_env_add_builtin(
    SWZRuntime *swz, SWZEnv *env, const char* name,
    SWZFun fun, void *args, int evald
){
    SWZSymbol *symbol = swz_alloc_symbol(swz, name, 0);
    SWZBuiltin *builtin = swz_alloc_builtin(swz, name, fun, args, evald);
    swz_env_bind(env, symbol, (SWZObject *)builtin);
}

// -*-
static SWZObject* _swz_mapper_eval(SWZRuntime *swz, SWZEnv *env, void *params, SWZObject *args){
    SWZ_UNUSED(params);
    return swz_eval(swz, env, args);
}

// -*- list === args
SWZList *swz_eval_list(SWZRuntime *swz, SWZEnv *env, SWZList *list){
    return swz_map(swz, env, NULL, _swz_mapper_eval, list);
}

// -*-
SWZObject *swz_progn(SWZRuntime *swz, SWZEnv *env, SWZList *list){
    if(swz_nil_p((SWZObject*)list)){
        return swz_alloc_nil(swz);
    }
    SWZObject *obj = NULL;
    while(1){
        obj = swz_eval(swz, env, list->car);
        if(swz_nil_p(list->cdr)){
            return obj;
        }else{
            list = (SWZList *)list->cdr;
        }
    }
}

// -*-
uint32_t swz_list_length(const SWZList *list){
    uint32_t len = 0;
    SWZ_FOREACH(list){
        len++;
    }
    return len;
}

// -*-
SWZList *swz_quote_with(SWZRuntime *swz, SWZObject *obj, char* sym){
    SWZList *seq = NULL;
    SWZList *list = NULL;
    SWZSymbol *symbol = NULL;

    list = (SWZList *)swz_alloc(swz, swzList);
    symbol = swz_alloc_symbol(swz, sym, 0);
    list->car = (SWZObject *)symbol;
    seq = (SWZList *)swz_alloc(swz, swzList);
    seq->cdr = swz_alloc_nil(swz);
    list->cdr = (SWZObject *)seq;
    seq->car = obj;
    return list;
}

// -*-
SWZList *swz_quote(SWZRuntime *swz, SWZObject *obj){
    return swz_quote_with(swz, obj, "quote");
}

// -*-
static SWZType* _swz_get_type(char c){
    SWZType *result = NULL;
    switch (c){
    case 'd':
        result = swzInteger;
        break;
    case 'f':
        result = swzFloat;
        break;
    case 'l':
        result = swzList;
        break;
    case 's':
        result = swzSymbol;
        break;
    case 'S':
        result = swzString;
        break;
    case 'o':
        result = swzEnv;
        break;
    case 'b':
        result = swzBuiltin;
        break;
    case 't':
        result = swzType;
        break;
    default:
        break;
    }

    return result;
}

// -*-
bool swz_get_args(SWZRuntime *swz, SWZList *list, char* fmt, ...){
    SWZObject **ref;
    SWZType *type;

    va_list args;
    va_start(args, fmt);
    while(!swz_nil_p((SWZObject*)list) && *fmt != '\0'){
        ref = va_arg(args, SWZObject **);
        if(*fmt == 'R'){ // 'R' -> rest of arguments
            *ref = (SWZObject *)list;
            return true;
        }
        type = _swz_get_type(*fmt);
        if(type != NULL && type != list->car->type){
            swz->error = "incorrect argument type";
            swz->errnum = SWZE_TYPE;
            return false;
        }
        *ref = list->car;
        list = (SWZList *)list->cdr;
        fmt += 1;
    }
    if(*fmt != '\0'){
        swz->error = "not enough arguments";
        swz->errnum = SWZE_TOO_FEW;
        return false;
    }else if(!swz_nil_p((SWZObject*)list)){
        swz->error = "too many arguments";
        swz->errnum = SWZE_TOO_MANY;
        return false;
    }

    return true;
}

// -*-
SWZList* swz_list_of_strings(SWZRuntime *swz, char **list, size_t n, int flag){
    if(n==0){
        return (SWZList *)swz_alloc_nil(swz);
    }
    SWZList *result = NULL;
    SWZList *lst = NULL;
    SWZString *str = NULL;

    result = (SWZList *)swz_alloc(swz, swzList);
    lst = result;
    for (size_t i = 0; i < n; i++){
        str = swz_alloc_string(swz, list[i], flag);
        lst->car = (SWZObject *)str;
        lst->cdr = swz_alloc(swz, swzList);
        lst = (SWZList *)lst->cdr;
    }
    lst->cdr = swz_alloc_nil(swz);
    return result;
}

// -*-
SWZList* swz_list_singleton(SWZRuntime *swz, SWZObject *entry){
    SWZList *singleton = (SWZList *)swz_alloc(swz, swzList);
    singleton->car = entry;
    singleton->cdr = swz_alloc_nil(swz);
    return singleton;
}

// -*-
SWZRuntime* swzlisp_new(void){
    SWZRuntime *swz = _my_alloc(sizeof(SWZRuntime));
    swzlisp_init(swz);
    return swz;
}

// -*-
void swzlisp_set_ctx(SWZRuntime *swz, void *ctx){
    swz->ctx = ctx;
}

// -*-
void *swzlisp_get_ctx(SWZRuntime *swz){
    return swz->ctx;
}

// -*-
void swzlisp_delete(SWZRuntime *swz){
    swzlisp_destroy(swz);
    free(swz);
}

// -*-
bool swz_is(SWZObject *obj, SWZType *type){
    return obj->type == type;
}

// SWZEnv* swz_alloc_empty_env(SWZRuntime *swz);
// SWZEnv* swz_alloc_default_env(SWZRuntime *swz);
// SWZObject *swz_run_main_if_exists(SWZRuntime *swz, SWZEnv *emv, int argc, char *argv);
// SWZBuiltin *swz_alloc_builtin(SWZRuntime *swz, const char* name, SWZFun fun, void *args, int evald);
// SWZObject *swz_alloc_nil(SWZRuntime *swz);
// char* swz_get_string(const SWZString *self); // get
// char* swz_get_symbol(const SWZSymbol *self); //
// SWZEnv* swz_alloc_empty_env(SWZRuntime *swz);
// SWZObject *swz_list_get_car(const SWZList *list);
// SWZObject *swz_list_get_cdr(const SWZList *list);
// void swz_list_set_car(SWZRuntime *list, SWZObject *car);
// void swz_list_set_cdr(SWZList *list, SWZObject *cdr);
// void swz_list_append(SWZRuntime *swz, SWZList **head, SWZList **tail, SWZObject *item);
// SWZInteger* swz_alloc_integer(SWZRuntime *swz, long num);
// long swz_get_integer(const SWZInteger *self);
// void swz_dump_stack(SWZRuntime *swz, SWZList *stack, FILE *stream);
// SWZObject *swz_error(SWZRuntime *swz, enum SWZError errnum, const char *errmsg);
// char *swz_get_error(SWZRuntime *swz);
// enum SWZError swz_get_errno(SWZRuntime *swz);
// void swz_clear_error(SWZRuntime *swz);
// void swz_eprint(SWZRuntime *swz, FILE *stream);
// enum SWZError swz_symbol_to_errno(SWZRuntime *symbol);
// int swz_is_bad_list(SWZList *list);
// int swz_is_bad_list_of_lists(SWZList *list);
// SWZList *swz_map(SWZRuntime* swz, SWZEnv* env, void *user, SWZMapFn mapfn, SWZList *args);
// bool swz_truthy(SWZObject *obj);

// -*---------------*-
// -*- caching API -*-
// -*---------------*-
// static struct swztext* _swz_textcache_lookup(HTable *cache, char *cstr);
// static void _swz_textcache_save(HTable *cache, struct swztext *text);
// void swz_textchach_remove(HTable *cache, struct swztext *text);
// static HTable* _swz_alloc_textcache(void);
// static char* _my_strdup(char *cstr);
// static struct swztext* _swz_alloc_text(SWZRuntime *swz, SWZType *type, HTaböe *cache, char *cstr, int flags);
// SWZString* swz_alloc_string(SWZRuntime *swz, char *cstr, int flags);
// SWZSymbol* swz_alloc_symbol(SWZRuntime *swz, char *cstr, int flags);
// void swzlisp_enable_string_cache(SWZRuntime *swz);
// void swzlisp_enable_symbol_cache(SWZRuntime *swz);
// void swzlisp_disable_string_cache(SWZRuntime *swz);
// void swzlisp_disable_symbol_cache(SWZRuntime *swz);

// -*----------------*-
// -*- GC utilities -*-
// -*----------------*-
// void swz_init(SWZRuntime *swz);
// void swz_destroy(SWZRuntime *swz);
// void swz_mark(SWZRuntime *swz, SWZObject *obj);
// static void _swz_mark_basics(SWZRuntime *swz);
// void swz_sweek(SWZRuntime *swz);