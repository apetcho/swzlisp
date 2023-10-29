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
    swzlisp_dealloc(swz);
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

// -*---------------*-
// -*- caching API -*-
// -*---------------*-
typedef struct swztext Text;
static Text *_swz_textcache_lookup(HTable *cache, char *cstr) {
    Text text;
    text.cstr = cstr;
    return htable_get_key_ptr(cache, &text);
}

// -*-
static void _swz_textcache_save(HTable *cache, struct swztext *text){
    htable_insert_ptr(cache, text, NULL);
}

// -*-
void swz_textchach_remove(HTable *cache, struct swztext *text){
    Text *existing = NULL;
    existing = htable_get_key_ptr(cache, text);
    if(existing == text){
        htable_remove_ptr(cache, text);
    }
}

// -*-
static HTable* _swz_alloc_textcache(void){
    return htable_new(swz_text_hash, swz_text_compare, sizeof(Text *), 0);
}

// -*-
static char* _my_strdup(char *cstr){
    size_t len = strlen(cstr);
    char *str = _my_alloc(len + 1);
    strncpy(str, cstr, len);
    str[0] = '\0';
    return str;
}

// -*-
static Text* _swz_alloc_text(SWZRuntime *swz, SWZType *type, HTable *cache, char *cstr, int flags){
    Text *str = NULL;
    if(cache){
        str = _swz_textcache_lookup(cache, cstr);
        if(str){
            // -
            if((flags & SWZFLAG_OWN) && !(flags & SWZFLAG_COPY)){
                free(cstr);
            }
            return str;
        }
    }
    // -
    str = (Text *)swz_alloc(swz, type);
    if(flags & SWZFLAG_COPY){
        cstr = _my_strdup(cstr);
    }
    str->cstr = cstr;
    str->can_free = flags & SWZFLAG_OWN;
    if(cache){
        // -
        _swz_textcache_save(cache, str);
    }
    return str;
}

// -*-
SWZString* swz_alloc_string(SWZRuntime *swz, char *cstr, int flags){
    return (SWZString *)_swz_alloc_text(swz, swzString, swz->strcache, cstr, flags);
}

// -*-
SWZSymbol* swz_alloc_symbol(SWZRuntime *swz, const char *cstr, int flags){
    return (SWZSymbol *)_swz_alloc_text(swz, swzSymbol, swz->symcache, (char*)cstr, flags);
}

// -*-
void swzlisp_enable_string_cache(SWZRuntime *swz){
    swz->strcache = _swz_alloc_textcache();
}

// -*-
void swzlisp_enable_symbol_cache(SWZRuntime *swz){
    swz->symcache = _swz_alloc_textcache();
}

// -*-
void swzlisp_disable_string_cache(SWZRuntime *swz){
    htable_delete(swz->strcache);
    swz->strcache = NULL;
}

// -*-
void swzlisp_disable_symbol_cache(SWZRuntime *swz){
    htable_delete(swz->symcache);
    swz->symcache = NULL;
}

// -*----------------*-
// -*- GC utilities -*-
// -*----------------*-
// -*-
void swzlisp_init(SWZRuntime *swz){
    swz->nil = swzList->alloc(swz);
    swz->nil->mark = 0;
    swz->nil->type = swzList;
    swz->nil->next = NULL;
    swz->head = swz->nil;
    swz->tail = swz->nil;
    swz->ctx = NULL;
    rbuffer_init(&swz->rbuffer, sizeof(SWZObject *), 16);
    swz->error = NULL;
    swz->errline = 0;
    swz->errstack = NULL;
    swz->stack = (SWZList *)swz->nil;
    swz->sdepth = 0;
    swz->symcache = NULL;
    swz->strcache = NULL;
    swz->modules = swz_alloc_empty_env(swz);
    // load stdlib
    swz_register_module(swz, swz_create_module(swz)); //, "os"));
    // ... + other std-modules
}

// -*-
void swzlisp_dealloc(SWZRuntime *swz){
    swz->marked = false;    // ensure we sweep all
    swz_sweep(swz);
    rbuffer_dealloc(&swz->rbuffer);
    swz_dealloc(swz, swz->nil);
    if(swz->symcache){
        htable_delete(swz->symcache);
    }
    if(swz->strcache){
        htable_delete(swz->strcache);
    }
}

// -*-
void swz_mark(SWZRuntime *swz, SWZObject *obj){
    rbuffer_push_back(&swz->rbuffer, &obj);
    swz->marked = true;
    while(swz->rbuffer.count > 0){
        Iterator iterator;
        rbuffer_pop_front(&swz->rbuffer, &obj);
        obj->mark = SWZ_GC_MARKED;
        iterator = obj->type->iter(obj);
        while(iterator.has_next(&iterator)){
            obj = iterator.next(&iterator);
            if(obj->mark == SWZ_GC_NOMARK){
                obj->mark = SWZ_GC_QUEUED;
                rbuffer_push_back(&swz->rbuffer, &obj);
            }
        }
        iterator.close(&iterator);
    }
}

// -*-
static void _swz_mark_basics(SWZRuntime *swz){
    if(swz->errstack){
        swz_mark(swz, (SWZObject *)swz->errstack);
    }
    swz_mark(swz, (SWZObject *)swz->stack);
    swz_mark(swz, (SWZObject *)swz->modules);
}

// -*-
void swz_sweep(SWZRuntime *swz){
    SWZObject *obj = swz->head;

    // -
    if(swz->marked){
        _swz_mark_basics(swz);
    }else{
        swz_clear_error(swz);
        swz->stack = (SWZList *)swz->nil;
        swz->sdepth = 0;
    }

    while(obj){
        if(obj->next->mark != SWZ_GC_MARKED){
            SWZObject *tmp = obj->next->next;
            swz_dealloc(swz, obj->next);
            obj->next = tmp;
        }else{
            obj->mark = SWZ_GC_NOMARK;
            obj = obj->next;
        }
    }

    if(obj == NULL){
        abort();
    }
    obj->mark = SWZ_GC_NOMARK;
    swz->tail = obj;
    swz->marked = false;
}

// -*--------------------------------------------------------------*-
// -*- SWZModule utilities and stdlib modules                     -*-
// -*--------------------------------------------------------------*-
// -*=============*-
// -*- os_module -*-
// -*=============*-
static SWZObject* _os_getenv(SWZRuntime *swz, SWZEnv *env, SWZList *args, void *params){
    SWZ_UNUSED(params);
    SWZ_UNUSED(env);
    SWZString *str;
    char *cstr;
    if(!swz_get_args(swz, args, "S", &str)){
        return NULL;
    }
    cstr = getenv(str->cstr);
    if(cstr){
        return (SWZObject *)swz_alloc_string(swz, cstr, SWZFLAG_COPY | SWZFLAG_OWN);
    }else{
        return swz_alloc_nil(swz);
    }
}

// -*-
// SWZModule* _os_module_init(SWZRuntime* swz, SWZModule* osmodule){
//     SWZModule *module = swz_alloc_module(
//         swz, swz_alloc_string(swz, "os", 0),
//         swz_alloc_string(swz, __FILE__, 0)
//     );
//     swz_env_add_builtin(swz, osmodule->env, "getenv", _os_getenv, NULL, 1);
//     return module;
// }

// -*-
SWZModule* swz_create_module(SWZRuntime *swz){
    SWZModule *module = swz_alloc_module(
        swz, swz_alloc_string(swz, "os", 0),
        swz_alloc_string(swz, __FILE__, 0)
    );
    swz_env_add_builtin(swz, module->env, "getenv", _os_getenv, NULL, 1);
    return module;
}

void swz_register_module(SWZRuntime *swz, SWZModule *module){
    // -
    swz_env_bind(swz->modules, (SWZSymbol *)module->name, (SWZObject *)module);
}

// -*==============*-
// -*- sys_module -*-
// -*==============*-
// static SWZObject* _sys_usname(SWZRuntime *swz, SWZEnv *env, SWZList *args, void *params);
// SWZModule* _sys_module_init(SWZModule* osmodule);


// -*-
// SWZModule* swz_create_module(SWZRuntime *swz, const char* modulename){
//     SWZModule *module = swz_alloc_module(
//         swz, swz_alloc_string(swz, (char*)modulename, 0),
//         swz_alloc_string(swz, __FILE__, 0)
//     );
//     return module;
// }

// -*-
SWZModule *swz_lookup_module(SWZRuntime *swz, SWZSymbol *name){
    SWZModule *module = (SWZModule *)swz_env_lookup(swz, swz->modules, name);
    if(!module){
        swz_clear_error(swz);
    }

    return module;
}

// -*-
SWZModule* swz_alloc_module(SWZRuntime *swz, SWZString *name, SWZString *path){
    SWZModule *module = (SWZModule *)swz_alloc(swz, swzModule);
    module->name = name;
    module->path = path;
    module->env = swz_alloc_empty_env(swz);
    return module;
}

// -*-
SWZEnv* swz_module_get_env(const SWZModule *module){
    return module->env;
}

// -*-
SWZModule* swz_import_file(SWZRuntime *swz, SWZString *name, SWZString *path){
    FILE *stream;
    SWZEnv *builtins = swz_alloc_default_env(swz);
    SWZEnv *module_env = swz_alloc_empty_env(swz);
    SWZModule *module;
    SWZObject *obj = NULL;
    module_env->parent = builtins;
    stream = fopen(path->cstr, "r");
    if(stream){
        return (SWZModule *)swz_error(swz, SWZE_ERRNO, "error opening file for import");
    }

    obj = swz_load_file(swz, module_env, stream);
    SWZ_IS_VALID_PTR(obj);

    module = (SWZModule *)swz_alloc(swz, swzModule);
    module->env = module_env;
    module->name = name;
    module->path = path;
    swz_register_module(swz, module);
    return module;
}

// -*-
SWZModule* swz_do_import(SWZRuntime *swz, SWZSymbol *name){
    SWZModule *module = NULL;
    SWZString *modulename = NULL;
    SWZString *modulepath = NULL;
    char *filename;
    module = swz_lookup_module(swz, name);
    if(module){
        return module;
    }

    size_t len = strlen(name->cstr);
    len += 1 /*null*/ + 7 /* ./.lisp*/;
    filename = _my_alloc(len);
    sprintf(filename, "./%s.lisp", name->cstr);
    modulepath = swz_alloc_string(swz, filename, SWZFLAG_OWN);
    modulename = swz_alloc_symbol(swz, name->cstr, SWZFLAG_OWN | SWZFLAG_COPY);
    return swz_import_file(swz, modulename, modulepath);
}

// -*------------*-
// -*- BUILTINS -*-
// -*------------*-
// _swz_builtin_eval(...)
static SWZObject* _swz_builtin_eval(SWZRuntime *swz, SWZEnv *env, SWZList *args, void *params){
    SWZ_UNUSED(params);
    return swz_eval(swz, env, args->car);
}

// _swz_builtin_car(...)
static SWZObject* _swz_builtin_car(SWZRuntime *swz, SWZEnv *env, SWZList *args, void *params){
    SWZList *self;
    SWZ_UNUSED(params);
    SWZ_UNUSED(env);
    if(!swz_get_args(swz, args, "l", &self)){
        return NULL;
    }
    if(swz_nil_p((SWZObject*)self)){
        swz_error(swz, SWZE_ERROR, "car of nil list");
    }
    return self->car;
}

// _swz_builtin_cdr(...)
static SWZObject* _swz_builtin_cdr(SWZRuntime *swz, SWZEnv *env, SWZList *args, void *params){
    SWZList *self;
    SWZ_UNUSED(params);
    SWZ_UNUSED(env);
    if(!swz_get_args(swz, args, "l", &self)){
        return NULL;
    }
    if(swz_nil_p((SWZObject*)self)){
        return swz_error(swz, SWZE_ERROR, "cdr of nil list");
    }
    return self->cdr;
}

// _swz_builtin_quote(...)
// _swz_builtin_cons(...)
// _swz_builtin_lambda(...)
// _swz_builtin_macro(...)
// _swz_builtin_define(...)
// _swz_builtin_plus(...)
// _swz_builtin_minus(...)
// _swz_builtin_multiply(...)
// _swz_builtin_divide(...)
// #CMP_EQ
// #CMP_NE
// #CMP_LT
// #CMP_LE
// #CMP_GT
// #CMP_GE
// _swz_builtin_cmp(...)
// _swz_builtin_if(...)
// _swz_builtin_nullp(...)
// _swz_get_quoted_left_items(...)
// _swz_advance_lists(...)
// _swz_builtin_map(...)
// _swz_new_pair_list(...)
// _swz_builtin_reduce(...)
// _swz_builtin_print(...)
// _swz_builtin_dump_stack(...)
// _swz_builtin_progn(...)
// _swz_builtin_unquote(...)
// _swz_quasiquote(...)
// _swz_builtin_quasiquote(...)
// _swz_builtin_eq(...)
// _swz_builtin_equal(...)
// _swz_builtin_assert(...)
// _swz_builtin_assert_error(...)
// _swz_builtin_cond(...)
// _swz_builtin_list(...)
// _swz_builtin_let(...)
// _swz_builtin_import(...)
// _swz_builtin_getattr(...)
// ... math function???
// -*-
// swz_env_populate_builtins(SWZRuntime *swz, SWZEnv *env);