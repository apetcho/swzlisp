#include "swzlisp.hpp"

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
        Object self = Object();
        std::string msg = "Invalid lambda expression";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }
    auto params = args[0];
    auto body = args[1];
    if(params.type()!=Type::List){
        Object self = Object();
        std::string msg = "Invalid 'lambda' expression";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }
    return Object(params.as_list(), body, env);
}

// -*-
// (if test yes no)
static Object fun_ifthenelse(std::vector<Object> args, Env& env){
    if(args.size() != 3){
        Object self = Object();
        std::string msg = "Invalid 'if' expression";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'define' expression";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'defun' expression";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'while-loop' expression";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'print' expression: not enough arguments";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'input' expression. Too many arguments";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
            Object self = Object();
            auto msg = stream.str();
            auto error = Error(self, env, msg.c_str());
            throw Error(error);
        }
        result = Object(my_random(max));
    }else if(args.size()==2){
        double min = args[0].as_float();
        double max = args[1].as_float();
        if(min > max){
            Object self = Object();
            auto msg = stream.str();
            auto error = Error(self, env, msg.c_str());
            throw Error(error);
        }
        result = Object(my_random(min, max));
    }else if(args.size()==3){
        double min = args[0].as_float();
        double max = args[1].as_float();
        size_t count = static_cast<size_t>(args[2].as_integer());
        if(min > max){
            Object self = Object();
            auto msg = stream.str();
            auto error = Error(self, env, msg.c_str());
            throw Error(error);
        }
        std::vector<Object> values{};
        for(size_t i=0; i < count; i++){
            values.push_back(Object(my_random(min, max)));
        }
        result = Object(values);
    }else{
        Object self = Object();
        auto msg = stream.str();
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }
    return result;
}

// -*-
static Object fun_read_file(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size() != 1){
        Object self = Object();
        std::string msg = "Invalid 'read-file' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }

    auto filename = args[0].as_string();
    auto data = Runtime::read_file(filename);
    return Object::create_string(data);
}

// -*-
static Object fun_write_file(std::vector<Object> args, Env& env){
    evaluate(args, env);
    if(args.size()!=2){
        Object self = Object();
        std::string msg = "Invalid 'write-file' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'import' expression. Too many arguments";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'eval' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '+' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '-' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '*' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '/' expression. Expect two numbers as arguments";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '%' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '=' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '!=' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '>' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '<' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '>=' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid '<=' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
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
        Object self = Object();
        std::string msg = "Invalid 'typename' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }
    
    auto name = args[0].type_name();
    Object result = Object::create_string(name);
    return result;
}

// -*-
// (float num)
static Object fun_toFloat(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        Object self = Object();
        std::string msg = "Invalid 'float' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }
    
    return args[0].to_float();
}

// -*-
static Object fun_toInteger(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 1){
        Object self = Object();
        std::string msg = "Invalid 'integer' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }
    
    return args[0].to_integer();
}

// -*-
//(index list i)
static Object fun_index(std::vector<Object> args, Env& env){
    evaluate(args, env);

    if(args.size() != 2){
        Object self = Object();
        std::string msg = "Invalid 'index' expression.";
        auto error = Error(self, env, msg.c_str());
        throw Error(error);
    }

    auto data = args[0].as_list();
    auto idx = args[1].as_integer();
    if(data.empty() || idx >= data.size()){
        std::ostringstream stream;
        stream << "index out of range";
        auto self = Object();
        throw Error(self, env, stream.str().c_str());
    }
    
    return data[idx];
}

static Object fun_insert(std::vector<Object> args, Env& env);
static Object fun_remove(std::vector<Object> args, Env& env);
static Object fun_length(std::vector<Object> args, Env& env);
static Object fun_push(std::vector<Object> args, Env& env);
static Object fun_pop(std::vector<Object> args, Env& env);
static Object fun_head(std::vector<Object> args, Env& env);
static Object fun_tail(std::vector<Object> args, Env& env);
static Object fun_parse(std::vector<Object> args, Env& env);
static Object fun_replace(std::vector<Object> args, Env& env);
static Object fun_display(std::vector<Object> args, Env& env);
static Object fun_debug(std::vector<Object> args, Env& env);
static Object fun_map(std::vector<Object> args, Env& env);
static Object fun_filter(std::vector<Object> args, Env& env);
static Object fun_reduce(std::vector<Object> args, Env& env);
static Object fun_range(std::vector<Object> args, Env& env);

// -*--------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                          -*-
// -*--------------------------------------------------------------------*-