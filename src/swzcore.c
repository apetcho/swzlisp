#include "swzlisp.h"

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

// SWZObject *swz_env_lookup_string(SWZRuntime *swz, SWZEnv *env, const char* key);
// void swz_env_add_builtin(
//     SWZRuntime *swz, SWZEnv *env, const char* name,
//     SWZFun fun, void *args, int evald
// );
// static _swz_mapper_eval(SWZRuntime *swz, SWZEnv *env, void *user, SWZObject *input);
// SWZList *swz_eval_list(SWZRuntime *swz, SWZEnv *env, SWZList *list);
// SWZObject *swz_progn(SWZRuntime *swz, SWZEnv *env, SWZList *list);
// uint32_t swz_list_length(const SWZList *list);
// SWZList *swz_quote_with(SWZRuntime *swz, SWZObject *obj, char* sym);
// SWZList *swz_quote(SWZRuntime *swz, SWZObject *obj);
// static SWZType* _swz_get_type(char c);
// bool swz_get_args(SWZRuntime *swz, SWZList *list, char* fmt, ...);
// SWZList* swz_list_of_strings(SWZRuntime *swz, char **list, size_t n, int flag);
// SWZList* swz_list_singleton(SWZRuntime *swz, SWZObject *entry);
// SWZRuntime swzlisp_new(void);
// void swzlisp_set_ctx(SWZRuntime *swz, void *ctx);
// void *swzlisp_get_ctx(SWZRuntime *swz);
// void swzlisp_destroy(SWZRuntime *swz);
// bool swz_is(SWZObject *obj, SWZType *type);
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