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
        std::string msg = "Invalid lambda expression";
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
        std::string msg = "Invalid if expression";
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

static Object fun_define(std::vector<Object> args, Env& env);
static Object fun_defun(std::vector<Object> args, Env& env);
static Object fun_while(std::vector<Object> args, Env& env);
static Object fun_for(std::vector<Object> args, Env& env);
static Object fun_do(std::vector<Object> args, Env& env);
static Object fun_scope(std::vector<Object> args, Env& env);
static Object fun_quote(std::vector<Object> args, Env& env);
static Object fun_exit(std::vector<Object> args, Env& env);
static Object fun_print(std::vector<Object> args, Env& env);
static Object fun_input(std::vector<Object> args, Env& env);
static Object fun_random(std::vector<Object> args, Env& env);
static Object fun_read_file(std::vector<Object> args, Env& env);
static Object fun_write_file(std::vector<Object> args, Env& env);
static Object fun_import(std::vector<Object> args, Env& env);
static Object fun_eval(std::vector<Object> args, Env& env);
static Object fun_list(std::vector<Object> args, Env& env);
static Object fun_add(std::vector<Object> args, Env& env);
static Object fun_sub(std::vector<Object> args, Env& env);
static Object fun_mul(std::vector<Object> args, Env& env);
static Object fun_div(std::vector<Object> args, Env& env);
static Object fun_mod(std::vector<Object> args, Env& env);
static Object fun_equalp(std::vector<Object> args, Env& env);
static Object fun_not_equalp(std::vector<Object> args, Env& env);
static Object fun_greaterp(std::vector<Object> args, Env& env);
static Object fun_lessp(std::vector<Object> args, Env& env);
static Object fun_greater_equalp(std::vector<Object> args, Env& env);
static Object fun_less_equalp(std::vector<Object> args, Env& env);
static Object fun_typename(std::vector<Object> args, Env& env);
static Object fun_toFloat(std::vector<Object> args, Env& env);
static Object fun_toInteger(std::vector<Object> args, Env& env);
static Object fun_index(std::vector<Object> args, Env& env);
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