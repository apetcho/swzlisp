#ifndef SWZLISP_H
#define SWZLISP_H

#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<stddef.h>
#include<stdarg.h>
#include<stdint.h>
#include<limits.h>
#include<float.h>
#include<stdio.h>
#include<wchar.h>
#include<assert.h>
#include<math.h>

#define SWZ_UNUSED(arg)     (void)arg
#define SWZ_VERSION_MAJOR   "0"
#define SWZ_VERSION_MINOR   "1"
#define SWZ_VERSION_PATCH   "0"
#define SWZ_VERSION_FULL    \
    SWZ_VERSION_MAJOR "." SWZ_VERSION_MINOR "." SWZ_VERSION_PATCH


//
static void* _my_alloc(size_t size){
    void *result = malloc(size);
    if(result == NULL){
        fprintf(stderr, "Error: memory allocation failure\n");
        abort();
    }
    memset(result, 0, size);
    return result;
}

// -*-
static inline bool _almost_equal(double x, double y){
    static double eps = DBL_EPSILON;
    double diff = fabs(x - y);
    x = fabs(x);
    y = fabs(y);
    double xymax = (x > y) ? x : y;
    bool result = (diff <= xymax*eps) ? true: false;
    return result;
}

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
CharBuffer* cbuffer_new(uint32_t capacity);
void cbuffer_dealloc(CharBuffer* cbuffer);
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
void rbuffer_dealloc(RingBuffer *rbuffer);
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
    void *data;                     // the container data structure
    uint32_t index;                 // zero-based index for the iterator
    int stateIdx;                   // some state variables tha my help
    void *statePtr;
    // -
    bool (*has_next)(Iterator*);
    void *(*next)(Iterator*);
    void (*close)(Iterator*);
};

void iterator_close_noop(Iterator* iter);
Iterator iterator_empty();
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
    uint32_t len;           // number of items currently in the table
    uint32_t allocated;     // number of items allocated
    // -
    uint32_t ksize;
    uint32_t vsize;
    // -
    HashFn hashfn;
    CompareFn equalfn;
    // -
    void *table;
} HTable;

void htable_init(HTable *htable, HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize);
HTable* htable_alloc(HashFn hashfn, CompareFn equalfn, uint32_t ksize, uint32_t vsize);
void htable_dealloc(HTable *htable);
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
#define SWZK_LAMBDA     0
#define SWZK_MACRO      1
#define SWZFLAG_COPY    0x1
#define SWZFLAG_OWN     0x2

#define SWZ_ERRORS                                                          \
    SWZE_DEF(SWZE_ERROR, 1, "SWZError", "a catch-all")                      \
    SWZE_DEF(SWZE_EOF, 2, "SWZEof", "end of file while parsing")            \
    SWZE_DEF(SWZE_SYNTAX, 3, "SWZSyntaxError", "syntax error")              \
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
    SWZE_DEF(SWZE_ERRNO, 14, "", "")

#define SWZE_COUNT  (SWZE_ERRNO + 1)

// -*- lisp_error_check
#define SWZ_VALIDATE_PTR(ptr) do {  \
    if(!ptr){                       \
        return NULL;                \
    }                               \
}while(0)


// -*- internal data structures -*-
#define SWZ_GC_NOMARK 'w'
#define SWZ_GC_QUEUED 'g'
#define SWZ_GC_MARKED 'b'

#define SWZ_OBJECT_HEAD \
    SWZType *type;      \
    SWZObject *next;    \
    char mark

#define SWZ_FOREACH(list) \
    for (; list->type == swzList && !swz_nil_p((SWZObject *) list); list=(SWZList*)list->cdr)
    

//! @todo: docstring
// -*- core -*-
typedef struct swzruntime SWZRuntime;   // interpreter | Engine


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

typedef SWZObject *(*SWZFun)(SWZRuntime *, SWZEnv *, SWZList *, void *);


extern SWZType *swzType;
extern SWZType *swzEnv;
extern SWZType *swzList;
extern SWZType *swzSymbol;
extern SWZType *swzInteger;
extern SWZType *swzFloat;
extern SWZType *swzString;
extern SWZType *swzBuiltin;
extern SWZType *swzLambda;
extern SWZType *swzModule;

extern const char *const swzVersion;


enum SWZError{
#define SWZE_DEF(err, ev, msg, desc) err = ev, 
    SWZ_ERRORS
#undef SWZE_DEF
};

extern const char *swzErrorNames[SWZE_COUNT];

struct swzobject{
    SWZ_OBJECT_HEAD;
};

struct swzruntime{
    SWZObject *head;
    SWZObject *tail;
    RingBuffer rbuffer;
    bool marked;
    SWZObject *nil;
    void *ctx;              // <user>
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
    SWZObject *(*alloc)(SWZRuntime*);
    void (*dealloc)(SWZRuntime*, void*);
    Iterator (*iter)(SWZObject*);     // iter()
    SWZObject *(*eval)(SWZRuntime*, SWZEnv*, SWZObject*);
    SWZObject *(*call)(SWZRuntime *, SWZEnv*, SWZObject*, SWZList*);
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

//! @note: integer and float should be combined as follow:
/* -- Probably, we'll use this privately in the parser.
struct swznumber{
    SWZ_OBJECT_HEAD;
    bool isInt;
    union{
        long ival;
        double fval;
    };
};
*/
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
    SWZFun call;
    const char *name;   // maybe <symbol> rather??? 
    void *params;
    int evald;
};

// -
struct swzlambda {
    SWZ_OBJECT_HEAD;
    SWZList *params;
    SWZList *body;          // code
    SWZEnv *env;            // closure
    SWZSymbol *name;        // first_binding
    int kind;
};

// -*-
struct swzmodule {
    SWZ_OBJECT_HEAD;
    SWZEnv *env;            // contents
    SWZString *name;        // <should be SWZSymbol instead>
    SWZString *path;        // <should be SWZSymbol instead>:: file-full-path
};

// -
typedef SWZObject *(*SWZMapFn)(SWZRuntime*, SWZEnv*, void*, SWZObject*);

// -*-
SWZRuntime* swzlisp_new(void);
void swzlisp_init(SWZRuntime *swz);
void swzlisp_dealloc(SWZRuntime *swz);
void swzlisp_delete(SWZRuntime *swz);
void *swzlisp_get_ctx(SWZRuntime *swz);
void swzlisp_set_ctx(SWZRuntime *swz, void *ctx);
void swzlisp_enable_string_cache(SWZRuntime *swz);
void swzlisp_enable_symbol_cache(SWZRuntime *swz);
void swzlisp_disable_string_cache(SWZRuntime *swz);
void swzlisp_disable_symbol_cache(SWZRuntime *swz);

// - Type API
void swz_dealloc(SWZRuntime *swz, SWZObject *obj);
SWZObject *swz_alloc(SWZRuntime *swz, SWZType *type);
void swz_print(FILE *stream, SWZObject *obj);
SWZObject* swz_eval(SWZRuntime *swz, SWZEnv *env, SWZObject *obj);
SWZObject *swz_call(SWZRuntime *swz, SWZEnv *env, SWZObject *callable, SWZList *args);
bool swz_compare(const SWZObject *self, const SWZObject *other);
// -

SWZEnv* swz_alloc_default_env(SWZRuntime *swz);
SWZEnv* swz_alloc_empty_env(SWZRuntime *swz);
SWZObject *swz_alloc_nil(SWZRuntime *swz);
SWZString* swz_alloc_string(SWZRuntime *swz, char *cstr, int flags);
SWZSymbol* swz_alloc_symbol(SWZRuntime *swz, const char *cstr, int flags);
SWZInteger* swz_alloc_integer(SWZRuntime *swz, long num);
SWZFloat *swz_alloc_float(SWZRuntime *swz, double num);
SWZList* swz_alloc_list(SWZRuntime *swz, SWZObject *car, SWZObject *cdr);
SWZBuiltin *swz_alloc_builtin(SWZRuntime *swz, const char* name, SWZFun fun, void *params, int evald);
SWZModule* swz_alloc_module(SWZRuntime *swz, SWZString *name, SWZString *path);

// -*-
void swz_env_populate_builtins(SWZRuntime *swz, SWZEnv *env);
void swz_env_bind(SWZEnv *env, SWZSymbol *symbol, SWZObject *obj);
SWZObject* swz_env_lookup(SWZRuntime *swz, SWZEnv *env, SWZSymbol *symbol);
SWZObject *swz_env_lookup_string(SWZRuntime *swz, SWZEnv *env, const char* key);
void swz_env_add_builtin(
    SWZRuntime *swz, SWZEnv *env,
    const char* name, SWZFun fun,
    void *params, int evald
);

// -*- List APIs
SWZList* swz_list_singleton(SWZRuntime *swz, SWZObject *entry);
SWZList* swz_list_of_strings(SWZRuntime *swz, char **list, size_t n, int flag);
uint32_t swz_list_length(const SWZList *list);
SWZObject *swz_list_get_car(const SWZList *list);
void swz_list_set_car(SWZList *list, SWZObject *car);
SWZObject *swz_list_get_cdr(const SWZList *list);
void swz_list_set_cdr(SWZList *list, SWZObject *cdr);
void swz_list_append(SWZRuntime *swz, SWZList **head, SWZList **tail, SWZObject *item);
bool swz_nil_p(SWZObject *obj);

// -*- Getters
char* swz_get_string(const SWZString *self); // get
char* swz_get_symbol(const SWZSymbol *self); //
long swz_get_integer(const SWZInteger *self);
//! @note: extension
double swz_get_float(const SWZFloat *self);
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
bool swz_get_args(SWZRuntime *swz, SWZList *list, char* fmt, ...);
//! @note: extension
bool swz_is_number(SWZRuntime *swz, SWZObject *obj);
bool swz_is_integer(SWZRuntime *swz, SWZObject *obj);
bool swz_is_float(SWZRuntime *swz, SWZObject *obj);

// -*- Evaluator
SWZList *swz_eval_list(SWZRuntime *swz, SWZEnv *env, SWZList *list);
SWZObject *swz_progn(SWZRuntime *swz, SWZEnv *env, SWZList *list);

// -*- Module API
//! @todo: change swz_module_get_env() -> swz_module_env()
SWZEnv* swz_module_get_env(const SWZModule *module);
void swz_register_module(SWZRuntime *swz, SWZModule *module);
SWZModule *swz_import_file(SWZRuntime *swz, SWZString *name, SWZString *path);
SWZModule* swz_do_import(SWZRuntime *swz, SWZSymbol *name);
//! @note: changed the signature
SWZModule *swz_create_module(SWZRuntime *swz); //, const char *modulename);
SWZModule *swz_lookup_module(SWZRuntime *swz, SWZSymbol *name);

// -*- Parser utilies
int swz_parse(SWZRuntime *swz, const char* input, int index, SWZObject **output);
SWZObject* swz_parse_progn(SWZRuntime *swz, const char *input); // parse_string()
SWZObject* swz_parse_progn_f(SWZRuntime *swz, FILE *file); // parse_file()
SWZObject* swz_load_file(SWZRuntime *swz, SWZEnv *env, FILE *input);
//! @todo: this function will be removed, because we don't want to process
// a main procedure in any specific way.
SWZObject *swz_run_main_if_exists(SWZRuntime *swz, SWZEnv *env, int argc, char **argv);

// -*- Garbage collector utilities
void swz_mark(SWZRuntime *swz, SWZObject *obj);
void swz_sweep(SWZRuntime *swz);

// -*- Error checking & handlers
SWZObject *swz_error(SWZRuntime *swz, enum SWZError errnum, const char *errmsg);
void swz_dump_stack(SWZRuntime *swz, SWZList *stack, FILE *stream);
char *swz_get_error(SWZRuntime *swz);
void swz_clear_error(SWZRuntime *swz);
enum SWZError swz_get_errno(SWZRuntime *swz);
enum SWZError swz_symbol_to_errno(SWZSymbol *symbol);

SWZList *swz_map(SWZRuntime* swz, SWZEnv* env, void *params, SWZMapFn mapfn, SWZList *args);
SWZList *swz_quote(SWZRuntime *swz, SWZObject *obj);
SWZList *swz_quote_with(SWZRuntime *swz, SWZObject *obj, char* sym);

void swz_eprint(SWZRuntime *swz, FILE *stream);
bool swz_is(SWZObject *obj, SWZType *type);
bool swz_is_bad_list(SWZList *list);
bool swz_is_bad_list_of_lists(SWZList *list);

uint32_t swz_text_hash(void *text);
bool swz_text_compare(const void *left, const void* right);
void swz_textcache_remove(HTable *cache, struct swztext *text);
bool swz_truthy(SWZObject *obj);

#endif