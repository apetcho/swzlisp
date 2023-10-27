#include "swzlisp.h"
#include<stdlib.h>
#include<string.h>
#include<errno.h>


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

// -*-
SWZEnv* swz_alloc_empty_env(SWZRuntime *swz){
    return (SWZEnv *)swz_alloc(swz, swzEnv);
}

// -*-
SWZEnv* swz_alloc_default_env(SWZRuntime *swz){
    SWZEnv *env = swz_alloc_empty_env(swz);
    swz_env_populate_builtins(swz, env);
    return env;
}

// -*-
SWZObject *swz_run_main_if_exists(SWZRuntime *swz, SWZEnv *env, int argc, char **argv){
    SWZList *args = NULL;
    SWZObject *mainfn = swz_env_lookup(
        swz, env, swz_alloc_symbol(swz, "main", 0)
    );
    if(mainfn==NULL){
        swz_clear_error(swz);
        return swz_alloc_nil(swz);
    }
    args = swz_list_of_strings(swz, argv, argc, 0);
    args = swz_quote(swz, (SWZObject *)args);
    args = swz_list_singleton(swz, (SWZObject *)args);
    return swz_call(swz, env, mainfn, args);
}

// -*-
SWZBuiltin *swz_alloc_builtin(SWZRuntime *swz, const char* name, SWZFun fun, void *params, int evald){
    SWZBuiltin *builtin = (SWZBuiltin *)swz_alloc(swz, swzBuiltin);
    builtin->fun = fun;
    builtin->name = name;
    builtin->params = params;
    builtin->evald = evald;
    return builtin;
}

// -*-
SWZObject *swz_alloc_nil(SWZRuntime *swz){
    if(swz->nil == NULL){
        swz->nil = swz_alloc(swz, swzList);
    }
    return swz->nil;
}

// -*-
char* swz_get_string(const SWZString *self){
    return self->cstr;
}

// -*-
char* swz_get_symbol(const SWZSymbol *self){
    return self->cstr;
}

// -*-
SWZList* swz_alloc_list(SWZRuntime *swz, SWZObject *car, SWZObject *cdr){
    SWZList *list = (SWZList *)swz_alloc(swz, swzList);
    list->car = car;
    list->cdr = cdr;
    return list;
}

// -*-
SWZObject *swz_list_get_car(const SWZList *list){
    return list->car;
}

// -*-
SWZObject *swz_list_get_cdr(const SWZList *list){
    return list->cdr;
}

// -*-
void swz_list_set_car(SWZList *list, SWZObject *car){
    list->car = car;
}

// -*-
void swz_list_set_cdr(SWZList *list, SWZObject *cdr){
    list->cdr = cdr;
}

// -*-
void swz_list_append(SWZRuntime *swz, SWZList **head, SWZList **tail, SWZObject *item){
    if(swz_nil_p((SWZObject*)*head)){
        *head = swz_alloc_list(swz, item, (SWZObject *)*head);
        *tail = *head;
    }else{
        (*tail)->cdr = (SWZObject *)swz_alloc_list(
            swz, item, swz_alloc_nil(swz)
        );
        *tail = (SWZList *)(*tail)->cdr;
    }
}

// -*-
SWZInteger* swz_alloc_integer(SWZRuntime *swz, long num){
    SWZInteger *number = (SWZInteger *)swz_alloc(swz, swzInteger);
    number->val = num;
    return number;
}

// -*-
long swz_get_integer(const SWZInteger *self){
    return self->val;
}

// -*-
void swz_dump_stack(SWZRuntime *swz, SWZList *stack, FILE *stream){
    if(!stack){
        stack = swz->stack;
    }

    fprintf(stream, "Stack trace (most recent call first):\n");
    SWZ_FOREACH(stack){
        fprintf(stream, " ");
        swz_print(stream, stack->car);
        fprintf(stream, "\n");
    }
}

// -*-
SWZObject *swz_error(SWZRuntime *swz, enum SWZError errnum, const char *errmsg){
    swz->error = (char*)errmsg;
    swz->errnum = errnum;
    swz->errstack = swz->stack;
    return NULL;
}

// -*-
char *swz_get_error(SWZRuntime *swz){
    return swz->error;
}

// -*-
enum SWZError swz_get_errno(SWZRuntime *swz){
    return swz->errnum;
}

// -*-
void swz_clear_error(SWZRuntime *swz){
    swz->error = NULL;
    swz->errstack = NULL;
    swz->errline = 0;
    swz->errnum = 0;
}

// -*-
void swz_eprint(SWZRuntime *swz, FILE *stream){
    if(swz->error){
        fprintf(stderr, "FATAL: swz_eprint() expects error, found none\n");
        return;
    }
    if(swz->errline){
        fprintf(stream, "at line: %d: ", (int)swz->errline);
    }
    char *errmsg = NULL;
    if (swz->errnum == SWZE_ERRNO){
        errmsg = strerror(errno);
        fprintf(
            stream, "Error %s: %s\nSystem error: %s\n",
            swzErrorNames[swz->errnum], swz->error, errmsg
        );
    }else{
        fprintf(stream, "Error %s: %s\n", swzErrorNames[swz->errnum], swz->error);
    }

    if(swz->errstack){
        swz_dump_stack(swz, swz->errstack, stream);
    }
}

// -*-
enum SWZError swz_symbol_to_errno(SWZSymbol *symbol){
    for (int i = 0; i < SWZE_COUNT; i++){
        if(strcmp(symbol->cstr, swzErrorNames[i])==0){
            return (enum SWZError)i;
        }
    }
    return SWZE_COUNT;
}

// -*-
bool swz_is_bad_list(SWZList *list){
    if(list->type != swzList){
        return true;
    }
    SWZ_FOREACH(list){}
    return list->type != swzList;
}

// -*-
bool swz_is_bad_list_of_lists(SWZList *list){
    if(list->type != swzList){
        return true;
    }
    SWZ_FOREACH(list){
        if(swz_is_bad_list((SWZList*)list->car)){
            return true;
        }
    }

    return list->type != swzList;
}

// -*-
SWZList *swz_map(SWZRuntime* swz, SWZEnv* env, void *params, SWZMapFn mapfn, SWZList *args){
    SWZList *head = NULL;
    SWZList *node = NULL;

    if(swz_nil_p((SWZObject*)args)){
        return args;
    }

    SWZ_FOREACH(args){
        if(!head){
            head = (SWZList *)swz_alloc(swz, swzList);
            node = head;
        }else{
            node->cdr = swz_alloc(swz, swzList);
            node = (SWZList *)node->cdr;
        }
        node->car = mapfn(swz, env, params, args->car);
        SWZ_IS_VALID_PTR(node->car);
    }

    if(args->type != swzList){
        // badly behaved cons cell in list
        return (SWZList *)swz_error(swz, SWZE_SYNTAY, "unexpected cons cell in list.");
    }
    if(node==NULL){
        abort();
    }
    node->cdr = swz_alloc_nil(swz);
    return head;
}

// -*-
bool swz_truthy(SWZObject *obj){
    return swz_is_number(NULL, obj) && !swz_compare(obj, (SWZObject*)swz_alloc_float(0));
}

// SWZEnv* swz_alloc_empty_env(SWZRuntime *swz);


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