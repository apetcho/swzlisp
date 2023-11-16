#include "swzlisp.hpp"
#include<csignal>

#define SWZLISP_BUILTINS                    \
    SWZLISP_DEF("eval", _eval)              \
    SWZLISP_DEF("typename", _typename)      \
    SWZLISP_DEF("parse", _parse)            \
    SWZLISP_DEF("do", _do)                  \
    SWZLISP_DEF("if", _ifthenelse)          \
    SWZLISP_DEF("for", _for)                \
    SWZLISP_DEF("while", _while)            \
    SWZLISP_DEF("scope", _scope)            \
    SWZLISP_DEF("quote", _quote)            \
    SWZLISP_DEF("defun", _defun)            \
    SWZLISP_DEF("define", _define)          \
    SWZLISP_DEF("lambda", _lambda)          \
    SWZLISP_DEF("=", _equalp)               \
    SWZLISP_DEF("!=", _not_equalp)          \
    SWZLISP_DEF(">", _greaterp)             \
    SWZLISP_DEF("<", _lessp)                \
    SWZLISP_DEF(">=", _greater_equalp)      \
    SWZLISP_DEF("<=", _less_equalp)         \
    SWZLISP_DEF("+", _add)                  \
    SWZLISP_DEF("-", _sub)                  \
    SWZLISP_DEF("*", _mul)                  \
    SWZLISP_DEF("/", _div)                  \
    SWZLISP_DEF("%", _mod)                  \
    SWZLISP_DEF("list", _list)              \
    SWZLISP_DEF("insert", _insert)          \
    SWZLISP_DEF("index", _index)            \
    SWZLISP_DEF("remove", _remove)          \
    SWZLISP_DEF("length", _length)          \
    SWZLISP_DEF("push", _push)              \
    SWZLISP_DEF("pop", _pop)                \
    SWZLISP_DEF("head", _head)              \
    SWZLISP_DEF("tail", _tail)              \
    SWZLISP_DEF("first", _head)             \
    SWZLISP_DEF("rest", _tail)              \
    SWZLISP_DEF("last", _pop)               \
    SWZLISP_DEF("range", _range)            \
    SWZLISP_DEF("map", _map)                \
    SWZLISP_DEF("filter", _filter)          \
    SWZLISP_DEF("reduce", _reduce)          \
    SWZLISP_DEF("exit", _exit)              \
    SWZLISP_DEF("quit", _exit)              \
    SWZLISP_DEF("print", _print)            \
    SWZLISP_DEF("input", _input)            \
    SWZLISP_DEF("random", _random)          \
    SWZLISP_DEF("linspace", _linspace)      \
    SWZLISP_DEF("import", _import)          \
    SWZLISP_DEF("read-file", _read_file)    \
    SWZLISP_DEF("repr", _repr)              \
    SWZLISP_DEF("replace", _replace)        \
    SWZLISP_DEF("display", _display)        \
    SWZLISP_DEF("integer", _toInteger)      \
    SWZLISP_DEF("float", _toFloat)          \
    SWZLISP_DEF("float", _newline)

// -*--------------------------------------------------------------------*-
// -*- namespace::swzlisp                                               -*-
// -*--------------------------------------------------------------------*-
namespace swzlisp{
// -
// -eval_args(Args&, Env&) -> void
static void evaluate(std::vector<Object>& args, Env& env){
    for(size_t i=0; i < args.size(); i++){
        args[i] = args[i].eval(env);
    }
}

// -*-
//(lambda (arg...) body)
// arg = args[0] -> Type::List
// body = args[1]
static Object fun_lambda(std::vector<Object> args, Env& env){
    if(args.size() != 2){
        //Object self = Object();
        std::string msg = "Invalid lambda expression";
        auto error = Error(env, msg.c_str());
        throw Error(error);
    }
    auto params = args[0];
    auto body = args[1];
    if(params.type()!=Type::List){
        // Object self = Object();
        std::string msg = "Invalid 'lambda' expression";
        auto error = Error(env, msg.c_str());
        throw Error(error);
    }
    return Object(params.as_list(), body, env);
}

// -*-
// (if test yes no)
static Object fun_ifthenelse(std::vector<Object> args, Env& env){
    if(args.size() != 3){
        throw Error(env, "Invalid 'if' expression");
    }
    Object result;
    Object test = args[0];
    Object yes = args[1];
    Object no = args[2];
    if(test.as_boolean()){
        result = yes.eval(env);
    }else{
        result = no.eval(env);
    }

    return result;
}

// -*-
// (define key val)
static Object fun_define(std::vector<Object> args, Env& env){
    if(args.size() != 2){
        throw Error(env, "Invalid 'define' expression");
    }

    auto key = args[0].str();
    auto val = args[1].eval(env);
    env.put(key, val);
    return val;
}

// -*-
// (defun name (param...) body)
// name = args[0]
// params = args[1]
// body = args[2]
static Object fun_defun(std::vector<Object> args, Env& env){
    if(args.size() != 3){
        throw Error(env, "Invalid 'defun' expression");
    }

    auto name = args[0].str();
    auto params = args[1].as_list();
    auto body = args[2];
    auto fun = Object(params, body, env);
    env.put(name, fun);
    return fun;
}

// -*-
// (while test body...)
static Object fun_while(std::vector<Object> args, Env& env){
    if(args.size()< 1){
        throw Error(env, "Invalid 'while' expression");
    }

    Object result;
    while(args[0].eval(env).as_boolean()){
        for(size_t i=1; i < args.size()-1; i++){
            args[i].eval(env);
        }
        result = args[args.size()-1].eval(env);
    }
    return result;
}

// -*-
static Object fun_for(std::vector<Object> args, Env& env){
    Object result;
    auto argv = args[1].eval(env).as_list();
    for(size_t i=0; i < argv.size(); i++){
        env.put(args[0].as_atom(), argv[i]);
        for(size_t j=1; j < args.size()-1; j++){
            args[j].eval(env);
        }
        result = args[args.size()-1].eval(env);
    }

    return result;
}

// -*-
// (do ....) <==> (progn ...)
static Object fun_do(std::vector<Object> args, Env& env){
    Object result;
    for(auto item: args){
        result = item.eval(env);
    }
    return result;
}

// -*-
// Evaluate a block of expression in a new environement
// (scope ...)
static Object fun_scope(std::vector<Object> args, Env& env){
    Env scope(env);
    Object result;
    for(auto self: args){
        result = self.eval(scope);
    }
    return result;
}

// -*-
static Object fun_quote(std::vector<Object> args, Env& env){
    std::vector<Object> rv;
    (void)env;
    for(auto self: args){
        rv.push_back(self);
    }

    //! @note: maybe we should use Object::create_quote()
    return Object(rv);
}

// -*-
static Object fun_exit(std::vector<Object> args, Env& env){
    evaluate(args, env);
    auto ecode = (
        args.size() < 1 ? 0 : args[0].to_integer().as_integer()
    );
    std::exit(ecode);
    return Object(); // never reached
}

// -*-
static Object fun_print(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() < 1){
        throw Error(env, "Invalid 'print' expression: not enough arguments");
    }

    Object result;
    for(auto self: args){
        result = self;
        std::cout << result.str() << " ";
    }
    std::cout << std::endl;
    return result;
}

// -*-
// (input [prompt])
static Object fun_input(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() > 1){
        throw Error(env, "Invalid 'input' expression. Too many arguments");
    }
    // [promt]?
    if(!args.empty()){ std::cout << args[0]; }

    std::string data;
    std::getline(std::cin, data);
    return Object::create_string(data);
}

// -*-
// (random) -> val in [0, 1)
// (random max) -> val in [0, max]
// (random min max) -> val in [min, max]
// (random min max count) -> List[val] where val in [min, max]

static double my_random(){
    std::random_device device;
    std::mt19937 rng(device());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

static double my_random(double maxval){
    std::random_device device;
    std::mt19937 rng(device());
    std::uniform_real_distribution<double> dist(0.0, maxval);
    return dist(rng);
}

static double my_random(double minval, double maxval){
    std::random_device device;
    std::mt19937 rng(device());
    std::uniform_real_distribution<double> dist(minval, maxval);
    return dist(rng);
}

// -*-
static Object fun_random(std::vector<Object> args, Env& env){
    evaluate(args, env);
    Object result;
    std::ostringstream stream;
    stream << "Invalid 'random' call expression.\n";
    stream << "Examples of how to call 'random' \n\n";
    stream << "(random)               ; will return a random number in [0.0 1.0)\n";
    stream << "(random max)           ; will return a random number in [0.0 max)\n";
    stream << "(random min max)       ; will return a random number in [min max)\n";
    stream << "(random min max count) ; will return a list of 'count' random ";
    stream << "numbers in [min max)\n";
    if(args.size()==0){
        result = Object(my_random());
    }else if(args.size()==1){
        double max = args[0].as_float();
        if(max < 0){
            throw Error(env, stream.str().c_str());
        }
        result = Object(my_random(max));
    }else if(args.size()==2){
        double min = args[0].as_float();
        double max = args[1].as_float();
        if(min > max){
            throw Error(env, stream.str().c_str());
        }
        result = Object(my_random(min, max));
    }else if(args.size()==3){
        double min = args[0].as_float();
        double max = args[1].as_float();
        size_t count = static_cast<size_t>(args[2].as_integer());
        if(min > max){
            throw Error(env, stream.str().c_str());
        }
        std::vector<Object> values{};
        for(size_t i=0; i < count; i++){
            values.push_back(Object(my_random(min, max)));
        }
        result = Object(values);
    }else{
        throw Error(env, stream.str().c_str());
    }
    return result;
}

// -*-
static Object fun_read_file(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() != 1){
        throw Error(env, "Invalid 'read-file' expression.");
    }

    auto filename = args[0].as_string();
    auto data = Runtime::read_file(filename);
    return Object::create_string(data);
}

// -*-
static Object fun_write_file(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size()!=2){
        throw Error(env, "Invalid 'write-file' expression.");
    }

    auto filename = args[0].as_string();
    auto data = args[1].as_string();
    std::ofstream fout(filename);
    long rv = ((fout << data)? 1 : 0);
    if(fout.is_open()){
        fout.close();
    }
    return Object(rv);
}

// -*-
// (import module)
static Object fun_import(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() != 1){
        throw Error(env, "Invalid 'import' expression. Too many arguments");
    }

    Env libenv;
    auto filename = args[0].as_string();
    auto source = Runtime::read_file(filename);
    auto result = Runtime::execute(source, libenv);
    env.merge(libenv); 
    return result;
}

// -*-
// (eval arg)
static Object fun_eval(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() != 1){
        throw Error(env, "Invalid 'eval' expression.");
    }
    return args[0].eval(env);
}

// -*-
// (list ...)
static Object fun_list(std::vector<Object> args, Env& env){
    evaluate(args, env);
    return Object(args);
}

// -*-
// (+ n1 n2 ...)
static Object fun_add(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() < 2){
        throw Error(env, "Invalid '+' expression.");
    }
    Object result = args[0];
    for(size_t i=1; i < args.size(); i++){
        result = result + args[i];
    }
    return result;
}

// -*-
// (- n1 n2 ...)
static Object fun_sub(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() < 2){
        throw Error(env, "Invalid '-' expression.");
    }
    Object result = args[0];
    for(size_t i=1; i < args.size(); i++){
        result = result - args[i];
    }
    return result;
}

// -*-
// (* n1 n2 ...)
static Object fun_mul(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() < 2){
        throw Error(env, "Invalid '*' expression.");
    }
    Object result = args[0];
    for(size_t i=1; i < args.size(); i++){
        result = result * args[i];
    }
    return result;
}

// -*-
static Object fun_div(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() != 2){
        throw Error(env, "Invalid '/' expression. Expect two numbers as arguments");
    }
    Object result;
    auto x = args[0];
    auto y = args[1];
    result = x / y;
    return result;
}

// -*-
static Object fun_mod(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() != 2){
        throw Error(env, "Invalid '%' expression.");
    }
    
    auto x = args[0];
    auto y = args[1];
    Object result = x % y;
    return result;
}

// -*-
static Object fun_equalp(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid '=' expression.");
    }
    
    auto x = args[0];
    auto y = args[1];
    Object result = Object(static_cast<long>(x == y));
    return result;
}

// -*-
static Object fun_not_equalp(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid '!=' expression.");
    }
    
    auto x = args[0];
    auto y = args[1];
    Object result = Object(static_cast<long>(x != y));
    return result;
}

// -*-
static Object fun_greaterp(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid '>' expression.");
    }
    
    auto x = args[0];
    auto y = args[1];
    Object result = Object(static_cast<long>(x > y));
    return result;
}

// -*-
static Object fun_lessp(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid '<' expression.");
    }
    
    auto x = args[0];
    auto y = args[1];
    Object result = Object(static_cast<long>(x < y));
    return result;
}

// -*-
static Object fun_greater_equalp(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid '>=' expression.");
    }
    
    auto x = args[0];
    auto y = args[1];
    Object result = Object(static_cast<long>(x >= y));
    return result;
}

// -*-
static Object fun_less_equalp(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid '<=' expression.");
    }
    
    auto x = args[0];
    auto y = args[1];
    Object result = Object(static_cast<long>(x <= y));
    return result;
}

// -*-
static Object fun_typename(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'typename' expression.");
    }
    
    auto name = args[0].type_name();
    Object result = Object::create_string(name);
    return result;
}

// -*-
static Object fun_newline(std::vector<Object> args, Env& env){
    (void)args;
    (void)env;
    return Object::create_string("\n");
}
// -*-
// (float num)
static Object fun_toFloat(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'float' expression.");
    }
    
    return args[0].to_float();
}

// -*-
static Object fun_toInteger(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'integer' expression.");
    }
    
    return args[0].to_integer();
}

// -*-
//(index list i)
static Object fun_index(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid 'index' expression.");
    }

    auto data = args[0].as_list();
    auto idx = args[1].as_integer();
    if(data.empty() || idx >= data.size()){
        throw Error(env, "index out of range");
    }
    
    return data[idx];
}

// -*-
// (insert data idx val)
static Object fun_insert(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 3){
        throw Error(env, "Invalid 'insert' expression.");
    }

    auto data = args[0].as_list();
    auto idx = args[1].as_integer();
    if(idx < 0 || idx > data.size()){
        throw Error(env, "index out of range");
    }
    data.insert(data.begin() + idx, args[2]);

    return Object(data);
}

// -*-
static Object fun_remove(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid 'remove' expression.");
    }

    auto data = args[0].as_list();
    auto idx = args[1].as_integer();
    if( data.empty() || idx < 0 || idx > data.size()){
        throw Error(env, "index out of range");
    }
    data.erase(data.begin()+idx);

    return Object(data);
}

// -*-
// (length list)
static Object fun_length(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'length' expression.");
    }

    auto data = args[0].as_list();
    return Object(static_cast<long>(data.size()));
}

// -*-
//(push data item1 item2 ... itemn)
static Object fun_push(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() == 0){
        throw Error(env, "Invalid 'push' expression.");
    }

    for(size_t i=1; i < args.size(); i++){
        args[0].push(args[i]);
    }
    return args[0];
}

// -*-
static Object fun_pop(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'pop' expression.");
    }

    return args[0].pop();
}

// -*-
// (head listObj)
static Object fun_head(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'head' expression.");
    }

    auto data = args[0].as_list();
    if(data.empty()){
        throw Error(env, "index out of range");
    }

    return data[0];
}

// -*-
static Object fun_tail(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'tail' expression.");
    }

    std::vector<Object> result{};
    std::vector<Object> data = args[0].as_list();
    for(size_t i=1; i < data.size(); i++){
        result.push_back(data[i]);
    }

    return Object(result);
}

// -*-
static Object fun_parse(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'parse' expression.");
    }
    if(args[0].type()!=Type::String){
        throw Error(env, ErrorKind::TypeError);
    }

    auto source = args[0].as_string();
    auto parser = Parser(source);
    auto ans = parser.parse();
    return Object(ans);
}

// -*-
// (replace stringObj old new)
static Object fun_replace(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 3){
        throw Error(env, "Invalid 'replace' expression.");
    }
    auto text = args[0].as_string();
    auto old = args[1].as_string();
    auto neo = args[2].as_string();
    Parser::replace(text, old, neo);

    return Object::create_string(text);
}

// -*-
static Object fun_display(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'display' expression.");
    }

    return Object::create_string(args[0].str());
}

// -*-
static Object fun_repr(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        throw Error(env, "Invalid 'repr' expression.");
    }

    return Object::create_string(args[0].repr());
}

// -*-
// (map fun listObj)
static Object fun_map(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        throw Error(env, "Invalid 'map' expression.");
    }
    std::vector<Object> result{};
    std::vector<Object> tmp{};
    auto fun = args[0];
    std::vector<Object> data = args[1].as_list();
    //! @note: fun must be a lambda, builtin or user define function
    //! @todo: need to handle error appropriately
    for(size_t i=0; data.size(); i++){
        tmp.push_back(data[i]);
        result.push_back(fun.apply(tmp, env));
        tmp.clear();
    }

    return Object(result);
}

// -*-
// (filter predicate listobj)
static Object fun_filter(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        auto error = Error(env, "Invalid 'filter' expression.");
    }

    std::vector<Object> result{};
    std::vector<Object> tmp{};
    std::vector<Object> data = args[1].as_list();
    auto predicate = args[0];
    for(size_t i=0; i < data.size(); i++){
        tmp.push_back(data[i]);
        // filter
        if(predicate.apply(tmp, env).as_boolean()){
            result.push_back(data[i]);
        }
        tmp.clear();
    }

    return Object(result);
}

// -*-
// (reduce fun acc listobj)
static Object fun_reduce(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 3){
        throw Error(env, "Invalid 'reduce' expression.");
    }

    std::vector<Object> data = args[2].as_list();
    std::vector<Object> tmp{};
    Object acc = args[1];
    Object fun = args[0];
    for(size_t i=0; i < data.size(); i++){
        tmp.push_back(acc);
        tmp.push_back(data[i]);
        acc = fun.apply(tmp, env);
        tmp.clear();
    }

    return acc;
}

// -*-
static std::vector<long> my_range(long stop){
    std::vector<long> result{};
    long val = 0;
    while(val < stop){
        result.push_back(val);
        val++;
    }
    return result;
}

// -
static std::vector<long> my_range(long start, long stop){
    std::vector<long> result{};
    long val = start;
    while(val < stop){
        result.push_back(val);
        val++;
    }
    return result;
}

// -
static std::vector<double> my_linspace(double start, double stop){
    std::vector<double> result{};
    double nstep = 10;
    double dx = (stop - start)/nstep;
    size_t i = 0;
    while(i < nstep){
        double val = start + i*dx;
        result.push_back(val);
        i++;
    }

    return result;
}

static std::vector<long> my_range(long start, long stop, long step){
    std::vector<long> result{};
    long val = start;
    while(val < stop){
        result.push_back(val);
        val += step;
    }
    return result;
}

// -*-
static std::vector<double> my_linspace(double start, double stop, size_t nstep){
    std::vector<double> result{};
    double dx = (stop - start)/nstep;
    size_t i = 0;
    while(i < nstep){
        double val = start + i * dx;
        result.push_back(val);
        i++;
    }
    return result;
}

// (range stop)             ==> (0, 1, ... stop-1)
// (range start stop)       ==> (start, start+1, ..., stop-1)
// (range start stop step)  ==> (start, start+step, ..., last)
// where last < stop
static Object fun_range(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() < 1 || args.size() > 3){
        throw Error(env, "Invalid 'range' expression.");
    }

    std::ostringstream err;
    err << "Invalid call to 'range' and argument are integer values\n";
    err << "Here is how to call 'range':\n\n";
    err << "(range stop)            ; result: (0, 1, ... stop-1)\n";
    err << "(range start stop)      ; result: (start, start+1, ..., stop-1)\n";
    err << "(range start stop step) ; result: (start, start+step, ... last) ";
    err << " where last <= stop\n";

    std::vector<Object> result{};
    if(args.size() == 1){
        auto stop = args[0];
        if(!stop.is_integer()){
            throw Error(env, err.str().c_str());
        }
        auto values = my_range(stop.as_integer());
        for(auto val: values){
            result.emplace_back(Object(val));
        }
    }else if(args.size() == 2){
        auto start = args[0];
        auto stop = args[1];
        if(!start.is_integer() || !stop.is_integer()){
            throw Error(env, err.str().c_str());
        }
        auto values = my_range(start.as_integer(), stop.as_integer());
        for(auto val: values){
            result.emplace_back(Object(val));
        }
    }else if(args.size() == 3){
        auto start = args[0];
        auto stop = args[1];
        auto step = args[2];
        bool test = (
            start.is_integer() && stop.is_integer() && step.is_integer()
        );
        if(!test){
            throw Error(env, err.str().c_str());
        }
        auto values = my_range(
            start.as_integer(), stop.as_integer(), step.as_integer()
        );
        for(auto val: values){
            result.emplace_back(Object(val));
        }
    }

    return Object(result);
}

// -*-
// (linspace start stop)
// (linspace start stop count)
static Object fun_linspace(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() < 2 || args.size() > 3){
        auto error = Error(env, "Invalid 'linspace' expression.");
    }

    std::ostringstream err;
    err << "Invalid call to 'linspace'.\n";
    err << "Here is how to call 'linspace':\n\n";
    err << "(range start stop)          ; start and stop are numbers\n";
    err << "(range start stop count)    ; count is an integer\n";

    std::vector<Object> result{};
    if(args.size() == 2){
        auto start = args[0];
        auto stop = args[1];
        if(!start.is_number() || !stop.is_number()){
            throw Error(env, err.str().c_str());
        }
        auto values = my_linspace(start.as_float(), stop.as_float());
        for(auto val: values){
            result.emplace_back(Object(val));
        }
    }else if(args.size() == 3){
        auto start = args[0];
        auto stop = args[1];
        auto step = args[2];
        bool test = (
            start.is_number() && stop.is_number() && step.is_integer()
        );
        if(!test){
            throw Error(env, err.str().c_str());
        }
        auto values = my_linspace(
            start.as_float(), stop.as_float(), step.as_integer()
        );
        for(auto val: values){
            result.emplace_back(Object(val));
        }
    }

    return Object(result);
}

// -*--------------------------------------------------------------------*-
// -*- Runtime                                                          -*-
// -*--------------------------------------------------------------------*-
Object Runtime::execute(std::string source, Env& env){
    Parser parser(source);
    auto result = parser.parse();
    for(size_t i=0; i < result.size()-1; i++){
        result[i].eval(env);
    }
    return result[result.size()-1].eval(env);
}

// -*-
std::string Runtime::read_file(const std::string& filename){
    std::ifstream fin;
    fin.open(filename.c_str());
    if(!fin.is_open()){
        throw Error(Env(), ("could not open file '" + filename + "'").c_str());
    }
    fin.seekg(0, std::ios::end);
    std::string data;
    data.reserve(fin.tellg());
    fin.seekg(0, std::ios::beg);
    data.assign(
        std::istreambuf_iterator<char>(fin),
        std::istreambuf_iterator<char>()
    );
    fin.close();
    return data;
}

// -*-
static void help(){
    std::cout << ":help     Print this message\n";
    std::cout << ":whos     Print all variables currently in the local environment\n";
    std::cout << ":clear    Clear all variables currently in the local environment\n";
    std::cout << ":export   Request the writing all expressions currently in\n";
    std::cout << "          the environment to a file\n";
    std::cout << ":exit     Exit the interpreter\n";
    std::cout << ":quit     Same as :exit\n";
    std::cout << ":bye      Same as :exit\n";
}

// -*-
static void clear(Env& env){
    env.clear();
}

// -*-
void whos(const std::map<std::string, Object>& bindings){
    // -------------------------------------------------------
    //  name |     object | size
    // -------------------------------------------------------
    std::cout << "-------------------------------------------------\n";
    std::cout << "    NAME    |      OBJECT      |   SIZE (bytes)  \n";
    std::cout << "-------------------------------------------------\n";

    auto show = [](std::string key, Object val){
        std::cout << " " << std::setw(10) << key << " |";
        std::cout << " " << std::setw(16) << val.str() << " | ";
        auto size = sizeof(val);
        std::cout << size << " bytes" << std::endl;
    };
    for(auto [key, val]: bindings){
        show(key, val);
    }
}

// -*-
void export_to_file(const std::string& data){
    std::cout << "Enter filename to export to: ";
    std::string filename;
    std::getline(std::cin, filename);
    std::ofstream fout(filename.c_str());
    fout << data;
    fout.close();
}

// -*-
void Runtime::repl(Env& env){
    std::string source;
    std::string input;
    Object result;
    std::vector<Object> parsed{};
    // -*- banner -*-
    std::cout << "swzlisp  VERSION 0.1; MIT License" << std::endl;
    std::cout << "Type :quit to exit" << std::endl;
    std::cout << "Type :help for help" << std::endl;
    //std::cout << banner.str();
    /*
    - :help
    - :whos
    - :export
    - :quit
    - :bye
    - :lookfor 
    */

    Env localEnv;
    auto self = std::make_shared<Env>(env);
    localEnv.set_parent(self);
    while(true){
        std::cout << ">>> ";
        std::getline(std::cin, input);
        if(input==":quit" || input==":bye" || input==":exit" || input==":q"){
            break;
        }else if(input==":whos"){
            whos(localEnv.bindings());
        }else if(input==":export"){
            export_to_file(source);
        }else if(input != ""){
            try{
                result = Runtime::execute(input, localEnv);
                std::cout << " => " << result.repr() << std::endl;
                source += input + "\n";
            }catch(Error& err){
                std::cerr << err.describe() << std::endl;
            }catch(std::exception& err){
                std::cout << err.what() << std::endl;
            }
        }
    }
}

// -*-
Env Runtime::builtins = Env();

static Env swzlisp_init(){
    Env env{};
#define SWZLISP_DEF(name, fname) { name, Object(name, fun##fname) },
    
    std::map<std::string, Object> keyvals{
        SWZLISP_BUILTINS
    };

#undef SWZLISP_DEF

    for(auto [key, val]: keyvals){
        env.put(key, val);
    }

    return env;
}

// -*-
// ./prog
// ./prog -h
// ./prog -i
// ./prog -f filename
// ./prog -c sexpr


static std::string progname;

static void usage(){
    std::string help = progname + " [-h]|[-i]|[-c sexpr]|[-f filename]\n";
    std::cout << help << std::endl;
    std::cout << "Options:\n";
    std::cout << "     -h              Print this message\n";
    std::cout << "     -i              Enter interactive mode\n";
    std::cout << "     -c sexpr        Run 'sexpr'\n";
    std::cout << "     -f script       Run 'scipt' in batch mode" << std::endl;
}

// -*--------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                          -*-
// -*--------------------------------------------------------------------*-

static void sighandler(int sig){
    if(sig==SIGINT || sig==SIGTERM){
        std::exit(EXIT_SUCCESS);
    }
}

// -*-------------------------*-
// -*- M A I N   D R I V E R -*-
// -*-------------------------*-
int main(int argc, char **argv){
    std::signal(SIGINT, sighandler);
    std::signal(SIGTERM, sighandler);    
    swzlisp::Runtime::builtins = swzlisp::swzlisp_init();
    swzlisp::Env workspace(swzlisp::Runtime::builtins);
    std::vector<swzlisp::Object> args;
    for(int i=0; i < argc; i++){
        args.emplace_back(swzlisp::Object::create_string(std::string(argv[i])));
    }
    swzlisp::Object self(args);
    workspace.put(":argv", self);

    try{
        if(argc == 1 || (argc==2 && std::string(argv[1]) == "-i")){
            swzlisp::Runtime::repl(workspace);
        }else if(argc == 2 && std::string(argv[1])=="-h"){
            swzlisp::help();
        }else if(argc==3 && std::string(argv[1])=="-c"){
            std::string sexpr(argv[2]);
            swzlisp::Runtime::execute(sexpr, workspace);
        }else if(argc==3 && std::string(argv[1])=="-f"){
            std::string filename(argv[2]);
            std::string source = swzlisp::Runtime::read_file(filename);
            swzlisp::Runtime::execute(source, workspace);
        }
    }catch(swzlisp::Error& err){
        std::cerr << err.describe() << std::endl;
    }catch(std::exception& err){
        std::cerr << err.what() << std::endl;
    }

    return EXIT_SUCCESS;
}