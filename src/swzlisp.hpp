#ifndef SWZLISP_H
#define SWZLISP_H
#include<exception>
#include<stdexcept>
#include<functional>
#include<iostream>
#include<sstream>
#include<fstream>
#include<variant>
#include<utility>
#include<limits>
#include<memory>
#include<vector>
#include<random>
#include<string>
#include<cmath>
#include<map>

#define SWZLISP_TYPES               \
    SWZLISP_DEF(Unit, "unit")       \
    SWZLISP_DEF(Atom, "atom")       \
    SWZLISP_DEF(Quote, "quote")     \
    SWZLISP_DEF(Integer, "integer") \
    SWZLISP_DEF(Float, "float")     \
    SWZLISP_DEF(String, "string")   \
    SWZLISP_DEF(List, "list")       \
    SWZLISP_DEF(Lambda, "lambda")   \
    SWZLISP_DEF(Builtin, "function")

#define SWZLISP_EXCEPTIONS                              \
    SWZLISP_DEF(TypeError, "TypeError")                 \
    SWZLISP_DEF(ValueError, "ValueError")               \
    SWZLISP_DEF(SyntaxError, "SyntaxError")             \
    SWZLISP_DEF(RuntimError, "RuntimeError")            \
    SWZLISP_DEF(ZeroDivisionError, "ZeroDivisionError")

// -*-------------------------------------------------------------------*-
// -*- namespace::swzlisp                                              -*-
// -*-------------------------------------------------------------------*-
namespace swzlisp{
// -*-
enum class Type{
#define SWZLISP_DEF(type, desc)     type,
    SWZLISP_TYPES
#undef SWZLISP_DEF
};

static std::map<Type, std::string> swzlispTypes = {
#define SWZLISP_DEF(sym, desc)     {Type::sym, desc},
    SWZLISP_TYPES
#undef SWZLISP_DEF
};

enum class ErrorKind{
#define SWZLISP_DEF(Err, desc) Err,
    SWZLISP_EXCEPTIONS
#undef SWZLISP_DEF
};

static std::map<ErrorKind, std::string> swzlispExceptions = {
#define SWZLISP_DEF(err, desc)  {ErrorKind::err, desc},
    SWZLISP_EXCEPTIONS
#undef SWZLISP_DEF
};

// -*-

// to_string()
// replace()
// is_symbol() | is_valid_char()

// -*-
template<typename T>
class Env{
public:
    Env();
    Env(const Env&);
    bool contains(const std::string& name) const;
    T get(const std::string& name) const;
    void put(std::string name, T value);

    friend std::ostream& operator<<(std::ostream& out, const Env&);

private:
    std::map<std::string, T> m_bindings;
    std::shared_ptr<Env> m_parent;
};

// -*-
template<typename T>
class Error {
public:
    Error();
    Error(T value, const Env<T>& env, ErrorKind);
    Error(T value, const Env<T>& env, const char *message);
    Error(const Error& other);
    ~Error();

    std::string describe();

private:
    ErrorKind m_error;
    std::shared_ptr<T> m_reason;
    Env<T> m_env;
    const char* m_message;
};

class Object;
typedef Object (*Fun)(std::vector<Object>, Env<Object>&);


// -*-
class Object{
public:
    Object();                                                                   // Type::Unit
    Object(long);                                                               // Type::Integer 
    Object(double);                                                             // Type::Float
    Object(std::vector<Object>);                                                // Type::List
    Object(std::vector<Object> params, Object ans, const Env<Object>& env);     // Type::Lambda
    Object(std::string, Fun);                                                   // Type::Builtin

    // -
    static Object create_quote(Object obj);                                     // Quote
    static Object create_atom(std::string str);                                 // Atom
    static Object create_string(std::string str);                               // String

    // -*-
    std::vector<std::string> atoms();
    bool is_builtin() const;
    Object apply(std::vector<Object> args, Env<Object>& env);
    Object eval(Env<Object>& env);
    bool is_number() const;
    bool as_boolean() const;
    long as_integer() const;
    double as_float() const;
    std::string as_string() const;
    std::string as_atom() const;
    std::vector<Object> as_list() const;
    void push(Object obj);
    Object pop();
    Object to_integer() const;
    Object to_float() const;
    bool operator==(Object other) const;
    bool operator!=(Object other) const;
    bool operator>=(Object other) const;
    bool operator<=(Object other) const;
    bool operator>(Object other) const;
    bool operator<(Object other) const;
    Object operator+(Object other) const;
    Object operator-(Object other) const;
    Object operator*(Object other) const;
    Object operator/(Object other) const;
    Object operator%(Object other) const;
    std::string type_name();
    std::string str() const;
    std::string repr() const;

    friend std::ostream& operator<<(std::ostream& os, const Object& obj);

private:
    // long -> Integer
    // double -> Float
    // std::string -> String, Atom
    // Fun -> Builtin
    // std::vector<Object> -> List, Lambda, Quote
    Type m_type;
    typedef std::vector<Object> List;
    typedef std::pair<std::string, Fun> Builtin;
    typedef std::variant<long, double, std::string, Builtin, List> Value;
    Env<Object> m_env; // lambda
    Value m_value;

    // -*-
    void unwrap(long& value){
        value = std::get<long>(m_value);
    }

    // -*-
    void unwrap(double& value){
        value = std::get<double>(m_value);
    }

    // -*-
    void unwrap(std::string& value){
        value = std::get<std::string>(m_value);
    }

    // -*-
    void unwrap(Builtin& value){
        value = std::get<Builtin>(m_value);
    }
    
    // -*-
    void unwrap(List& value){
        value = std::get<List>(m_value);
    }
};

// -*----------*-
// -*- Parser -*-
// -*----------*-
// to_string()
// replace()
// is_symbol() | is_valid_char()
// skip_whitespace()
// parse(std::string, int) | next_token() -> Object
// parse(std::string) -> std::vector<Object>
class Parser{
private:
    std::string m_source;
    std::string::const_iterator m_begin;
    std::string::const_iterator m_end;
    std::string::iterator m_iter;

public:
    Parser(std::string& source);
    ~Parser() = default;

    // -*-
    static void replace(std::string &input, std::string old, std::string neo);
    Object next_token();
    std::vector<Object> parse();
};

// -*-

class Runtime{
private:
    // Parser m_parser;
    // std::string m_filename;
    // std::string m_source;

public:
    Runtime() = default;
    ~Runtime() = default;
    // ::read_file(const std::string& filename) -> std::string
    static std::string read_file(const std::string& filename);
    // +run(std::string, Env<Object>&) -> Object
    static Object execute(Env<Object>& env);
    static Object execute(std::string source, Env<Object>& env);
    static Object execute(std::string filename);
    static Object repl(Env<Object>& env);
    static Env<Object> builtins;
};


// -*-------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                         -*-
// -*-------------------------------------------------------------------*-

#endif