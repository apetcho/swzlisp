#ifndef SWZLISP_H
#define SWZLISP_H

#include<stdbool.h>
#include<stddef.h>
#include<stdarg.h>
#include<stdint.h>
#include<stdio.h>
#include<wchar.h>

#define SWZ_UNUSED(arg)     (void)arg

// -*------------------------------------------------------------*-
// -*- CharBuffer                                               -*-
// -*------------------------------------------------------------*-
//! @todo: add docstring
typedef struct {
    char *buffer;
    uint32_t capacity;
    uint32_t len;
} CharBuffer;

void cbuffer_init(CharBuffer *cbuffer, uint32_t capacity);
CharBuffer* cbuffer_create(uint32_t capacity);
void cbuffer_destroy(CharBuffer* cbuffer);
void cbuffer_delete(CharBuffer* cbuffer);
void cbuffer_concat(CharBuffer* cbuffer, const char *cstr);
void cbuffer_append(CharBuffer* cbuffer, char c);
void cbuffer_trim(CharBuffer* cbuffer);
void cbuffer_clear(CharBuffer* cbuffer);
void cbuffer_printf(CharBuffer* cbuffer, const char* fmt, ...);
void cbuffer_vprintf(CharBuffer* cbuffer, const char* fmt, va_list args);

// -*------------------------------------------------------------*-
// -*- RingBuffer                                               -*-
// -*------------------------------------------------------------*-
//! @todo: docstring
typedef struct {
    void *buffer;
    uint32_t dsize;
    uint32_t nalloc;
    int start;
    int count;
} RingBuffer;

void rbuffer_init(RingBuffer *rbuffer, uint32_t dsize, int init);
void rbuffer_destroy(RingBuffer *rbuffer);
void rbuffer_push_front(RingBuffer *rbuffer, void *src);
void rbuffer_pop_front(RingBuffer *rbuffer, void *dst);
void rbuffer_push_back(RingBuffer *rbuffer, void *src);
void rbuffer_pop_back(RingBuffer *rbuffer, void *dst);
void rbuffer_grow(RingBuffer *rbuffer);

// -*------------------------------------------------------------*-
// -*- Iterator                                                 -*-
// -*------------------------------------------------------------*-
typedef struct iterator Iterator;
struct iterator {
    void *data;
    uint32_t index;
    int stateIdx;
    void *statePtr;
    bool (*has_next)(Iterator*);
    void *(*next)(Iterator*);
    void (*close)(Iterator*);
};

void iterator_close_noop(Iterator* iter);
//Iterator iterator_empty();
Iterator iterator_single_value(void *value);
Iterator iterator_concat(Iterator *iter, size_t n);
Iterator iterator_concat2(Iterator left, Iterator right);
Iterator iterator_concat3(Iterator a, Iterator b, Iterator c);
Iterator iterator_array(void **array, uint32_t len, bool own);
Iterator iterator_from_args(int n, ...);

// -*------------------------------------------------------------*-
// -*- HTable                                                   -*-
// -*------------------------------------------------------------*-
typedef uint32_t (*HashFn)(void *obj);
typedef bool (*CompareFn)(const void*, const void*);
typedef int (*PrintFn)(FILE*, const void *);

typedef struct {
    uint32_t len;
    uint32_t allocated;
    uint32_t ksize;
    uint32_t vsize;
    HashFn hashfn;
    CompareFn equalfn;
    void *table;
} HTable;

void htable_init(HTable *htable, HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize);
HTable* htable_create(HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize);
void htable_destroy(HTable *htable);
void htable_delete(HTable *htable);
void htable_insert(HTable *htable, void *key, void *value);
void htable_insert_ptr(HTable *htable, void *key, void *value);
int htable_remove(HTable *htable, void *key);
int htable_remove_ptr(HTable *htable, void *key);
void* htable_get(const HTable *htable, void *key);
void* htable_get_ptr(const HTable *htable, void *key);
void* htable_get_key(const HTable *htable, void *key);
void* htable_get_key_ptr(const HTable *htable, void *key);
bool htable_contains(const HTable *htable, void *key);
bool htable_contains_ptr(const HTable *htable, void *key);
uint32_t htable_length(const HTable *htable);

//! @note: maybe, the next four (4) function should be private
uint32_t htable_string_hash(void *data);
bool htable_string_equal(const void *lhs, const void *rhs);
bool htable_int_equal(const void *lhs, const void *rhs);
//! @note: extension to floating-point numbers

bool htable_float_equal(const void *lhs, const void *rhs);
Iterator htable_iterator_keys(HTable *htable);
Iterator htable_iterator_keys_ptr(HTable *htable);
Iterator htable_iterator_values(HTable *htable);
Iterator htable_iterator_values_ptr(HTable *htable);

// -*------------------------------------------------------------*-
// -*- Core + Internals Data Structures                         -*-
// -*------------------------------------------------------------*-
//! @todo: docstring
// -*- core -*-
typedef struct swzlisp SWZLisp; // interpreter
SWZLisp swzlisp_new();
void swzlisp_set_ctx(SWZLisp *swz, void *ctx);
void *swzlisp_get_ctx(SWZLisp *swz);
void swzlisp_free(SWZLisp *swz);
void swzlisp_enable_string_cache(SWZLisp *swz);
void swzlisp_enable_symbol_cache(SWZLisp *swz);
void swzlisp_disable_string_cache(SWZLisp *swz);
void swzlisp_disable_symbol_cache(SWZLisp *swz);

typedef struct swzobject SWZObject;
typedef struct swztype SWZType;
typedef struct swzenv SWZEnv;
typedef struct swztext SWZSymbol;
typedef struct swztext SWZString;
typedef struct swzinteger SWZInteger;
//! @note: extension
typedef struct swzfloat SWZFloat;

typedef struct swzbuiltin SWZBuiltin;
typedef struct swzlambda SWZLambda;
typedef struct swzlist SWZList;
typedef struct swzmodule SWZModule;

extern SWZType *swzType;

void swz_print(FILE *file, SWZObject *obj);
SWZObject* swz_eval(SWZLisp *swz, SWZEnv *env, SWZObject *obj);
SWZObject *swz_call(SWZLisp *swz, SWZEnv *env, SWZObject *callable, SWZList *args);
bool swz_compare(SWZObject *self, SWZObject *other);
bool swz_is(SWZObject *obj, SWZType *type);

extern SWZType *swzEnv;

SWZEnv* swz_new_default_env(SWZLisp *swz);
SWZEnv* swz_new_empty_env(SWZLisp *swz);
void swz_env_populate_builtins(SWZLisp *swz, SWZEnv *env);
void swz_env_bind(SWZEnv *env, SWZSymbol *symbol, SWZObject *obj);
SWZObject* swz_env_lookup(SWZLisp *swz, SWZEnv *env, SWZSymbol *symbol);
SWZObject *swz_env_lookup_string(SWZLisp *swz, SWZEnv *env, const char* key);

extern SWZType *swzList;
SWZList* swz_new_list(SWZLisp *swz, SWZObject *left, SWZObject *right);
SWZList* swz_list_singleton(SWZLisp *swz, SWZObject *entry);
SWZList* swz_list_of_strings(SWZLisp *swz, char **list, size_t n, int flag);
uint32_t swz_list_length(const SWZList *list);
SWZObject *swz_list_get_left(const SWZList *list);
void swz_list_set_left(SWZList *list, SWZObject *left);
SWZObject *swz_list_right(const SWZList *list);
void swz_list_set_right(SWZList *list, SWZObject *right);
void swz_list_append(SWZLisp *swz, SWZList **head, SWZList **tail, SWZObject *item);
SWZObject *swz_new_nil(SWZLisp *swz);
bool swz_nil_p(SWZObject *obj);

extern SWZType *swzSymbol;
extern SWZType *swzInteger;
extern SWZType *swzFloat;
extern SWZType *swzString;
extern SWZType *swzBuiltin;
extern SWZType *swzLambda;
extern SWZType *swzModule;


#define SWZFLAG_COPY    0x1
#define SWZFLAG_OWN     0x2

SWZString* swz_new_string(SWZLisp *swz, char *cstr, int flags);
char* swz_get_string(const SWZString *self);
SWZSymbol* swz_new_symbol(SWZLisp *swz, char *cstr, int flags);
char* swz_get_symbol(const SWZSymbol *self);
SWZInteger* swz_new_integer(SWZLisp *swz, long num);
long swz_get_integer(const SWZInteger *self);
//! @note: extension
SWZFloat *swz_new_float(double num);
double swz_get_float(const SWZFloat *self);

typedef SWZObject* (*SWZFun)(SWZLisp*, SWZEnv*, SWZList*, void*);

SWZBuiltin *swz_new_builtin(SWZLisp *swz, const char* name, SWZFun fun, void *args, int evald);
void swz_env_add_builtin(
    SWZLisp *swz, SWZEnv *env, const char* name,
    SWZFun fun, void *args, int evald
);
SWZList *swz_eval_list(SWZLisp *swz, SWZEnv *env, SWZList *list);
SWZObject *swz_progn(SWZLisp *swz, SWZEnv *env, SWZList *list);

/*
Arguments formats:

    d - SWZInteger
    f - SWZFloat
    l - SWZFist
    s - SWZSymbol
    S - SWZString
    o - SWZEnv
    e - SWZError
    * - anything
    R - Rest of arguments
*/
bool swz_get_args(SWZLisp *swz, SWZList *list, char* fmt, ...);

SWZModule* swz_new_module(SWZLisp *swz, SWZString *name, SWZString *filename);
SWZEnv* swz_module_get_env(const SWZModule *module);
void swz_register_module(SWZLisp *swz, SWZModule *module);
SWZModule* swz_import_file(SWZLisp *swz, SWZString *name, SWZString *file);
SWZModule* swz_do_import(SWZLisp *swz, SWZSymbol *name);

int swz_parse_object(SWZLisp *swz, const char* input, int index, SWZObject **output);
SWZObject* swz_parse_progn(SWZLisp *swz, const char *input);
SWZObject* swz_parse_progn_f(SWZLisp *swz, FILE *file);
SWZObject* swz_load_file(SWZLisp *swz, SWZEnv *env, FILE *input);
//! @todo: this function will be removed, because we don't want to process
// a main procedure in any specific way.
SWZObject *swz_run_main_if_exists(SWZLisp *swz, SWZEnv *emv, int argc, char *argv);

void swz_mark(SWZLisp *swz, SWZObject *obj);
void swz_sweek(SWZLisp *swz);

SWZList *swz_quote(SWZLisp *swz, SWZObject *obj);

#define SWZ_ERRORS                                                          \
    SWZE_DEF(SWZE_ERROR, 1, "SWZError", "a catch-all")                      \
    SWZE_DEF(SWZE_EOF, 2, "SWZEof", "end of file while parsing")            \
    SWZE_DEF(SWZE_SYNTAY, 3, "SWZSyntaxError", "syntax error")              \
    SWZE_DEF(SWZE_FILE, 4, "SWZFileError", "error reading file")            \
    SWZE_DEF(SWZE_TOO_MANY, 5, "SWZTooManyArgsError", "too many arguments")  \
    SWZE_DEF(SWZE_TOO_FEW, 6, "SWZNotEnoughArgsError", "not enough arguments")\
    SWZE_DEF(SWZE_TYPE, 7, "SWZTypeError", "wrong type argument in function") \
    SWZE_DEF(SWZE_CALL, 8, "SWZNotCallableError", "not callable")           \
    SWZE_DEF(SWZE_EVAL, 9, "SWZNotEvaluatableError", "not avaluatable")     \
    SWZE_DEF(SWZE_NOT_FOUND, 10, "SWZNotFoundError", "not found")           \
    SWZE_DEF(SWZE_EXIT, 11, "SWZExitError", "exit the interpreter")         \
    SWZE_DEF(SWZE_ASSERT, 12, "SWZAssertionError", "assertion error")       \
    SWZE_DEF(SWZE_VALUE, 13, "SWZValueError", "invalid argument")           \
    SWZE_DEF(SWZE_ERRNO, 14, "", "")                                        \
    SWZE_DEF(SWZE_MAX_ERR, 15, "", "")

enum SWZError{
#define SWZE_DEF(err, ev, msg, desc) err = ev, 
    SWZ_ERRORS
#undef SWZE_DEF
};

extern const char *swzErrorNames[SWZE_MAX_ERR];

SWZObject *swz_error(SWZLisp *swz, enum SWZError errnum, const char *errmsg);
void swz_dump_stack(SWZLisp *swz, SWZLisp *stack, FILE *file);

#define SWZ_IS_VALID(ptr) do {  \
    if(!ptr){                   \
        return NULL;            \
    }                           \
}while(0)

void swz_eprint(SWZLisp *swz, FILE *file);
char *swz_get_error(SWZLisp *swz);
enum SWZError swz_get_errno(SWZLisp *swz);
void swz_clear_error(SWZLisp *swz);

extern const char *const swzVersion;

// -*- internal data structures -*-
#define SWZ_GC_NOMARK 'w'
#define SWZ_GC_QUEUED 'g'
#define SWZ_GC_MARKED 'b'

#define SWZ_OBJECT_HEAD \
    SWZType *type;      \
    SWZObject *next;    \
    char mark

#define SWZ_FOREACH(list) \
    for (; list->type == swzList && !swz_nil_p((SWZObject *) list); list=(SWZList*)list->right)

struct swzobject{
    SWZ_OBJECT_HEAD;
};

struct swzlisp{
    SWZObject *head;
    SWZObject *tail;
    RingBuffer rbuffer;
    SWZObject *nil;
    void *user;
    char *error;
    enum SWZError errnum;
    uint32_t errline;
    SWZList *errstack;
    SWZList *stack;
    uint32_t sdepth; // stack depth
    HTable *symcache;
    HTable *strcache;
    SWZEnv *modules;
};

// -
struct swzenv{
    SWZ_OBJECT_HEAD;
    HTable scope;
    SWZEnv *parent;
};

// -
struct swzlist{
    SWZ_OBJECT_HEAD;
    SWZObject *car;
    SWZObject *cdr;
};

// -
struct swztype{
    SWZ_OBJECT_HEAD;
    const char *name;
    void (*print)(FILE*, SWZObject*);
    SWZObject *(*create)(SWZLisp*);
    void (*destroy)(SWZLisp*, void*);
    Iterator (*iter)(SWZObject*);     // iter()
    SWZObject *(*eval)(SWZLisp*, SWZEnv*, SWZObject*);
    SWZObject *(*call)(SWZLisp *, SWZEnv*, SWZObject*, SWZList*);
    bool (*compare)(const SWZObject*, const SWZObject*);
    // equalfn
    // hashfn
};

// -
struct swztext{
    SWZ_OBJECT_HEAD;
    char can_free;
    char *cstr;
};

// -
struct swzinteger{
    SWZ_OBJECT_HEAD;
    long val;
};

//! @note: extension
struct swzfloat{
    SWZ_OBJECT_HEAD;
    double val;
};

// -
struct swzbuiltin {
    SWZ_OBJECT_HEAD;
    SWZFun fun;
    const char *name;
    void *data;
    int evald;
};

// -
struct swzlambda {
    SWZ_OBJECT_HEAD;
    SWZList *args;
    SWZList *body;          // code
    SWZEnv *env;            // closure
    SWZSymbol *fbinding;    // first_binding
    int lambda_type;
};

// -*-
struct swzmodule {
    SWZ_OBJECT_HEAD;
    SWZEnv *env;            // contents
    SWZString *name;
    SWZString *filename;    // file-full-path
};

// -
typedef SWZObject *(*SWZMapFn)(SWZLisp*, SWZEnv*, void*, SWZObject*);

SWZList *swz_map(SWZLisp* swz, SWZEnv* env, void *user, SWZMapFn mapf, SWZLisp *argv);

#define SWZT_LAMBDA     0
#define SWZT_MACRO      1

void swz_init(SWZLisp *swz);
void swz_destroy(SWZLisp *swz);

void swz_free(SWZLisp *swz, SWZObject *obj);
SWZObject *swz_new(SWZLisp *swz, SWZType *type);
SWZList *swz_quote_with(SWZLisp *swz, SWZObject *obj, char* sym);

enum SWZError swz_symbol_to_errno(SWZSymbol *symbol);

int swz_is_bad_list(SWZList *list);
int swz_is_bad_list_of_lists(SWZList *list);

uint32_t swz_text_hash(void *text);
bool swz_text_compare(const void *left, const void* right);

void swz_textchach_remove(HTable *cache, struct swztext *text);
bool swz_truthy(SWZObject *obj);

//! @note: changed the signature
SWZModule* swz_create_module(SWZLisp *swz, const char* modulename);
SWZModule *swz_lookup_module(SWZLisp *swz, SWZSymbol *name);

#endif