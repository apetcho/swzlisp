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
    SWZLISP_DEF(Lambda, "function") \
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

class Object;
// -*-
//template<typename T>
class Env: public std::enable_shared_from_this<Env>{
public:
    Env();
    Env(const Env&);
    bool contains(const std::string& name) const;
    const Object& get(std::string name) const;
    void put(std::string name, Object& value);

    std::shared_ptr<Env> get_pointer(){
        return shared_from_this();
    }
    
    void set_parent(const std::shared_ptr<Env>& parent){
        this->m_parent = parent;
    }

    friend std::ostream& operator<<(std::ostream& out, const Env&);

private:
    std::map<std::string, Object> m_bindings;
    std::shared_ptr<Env> m_parent;
};

// -*-
//template<typename T>
class Error {
public:
    Error();
    Error(Object& value, const Env& env, ErrorKind);
    Error(Object& value, const Env& env, const char *message);
    Error(const Error& other);
    ~Error();

    std::string describe();

private:
    ErrorKind m_error;
    std::shared_ptr<Object> m_reason;
    Env m_env;
    std::string m_message;
};

class Object;
typedef Object (*Fun)(std::vector<Object>, Env&);


// -*-
class Object: public std::enable_shared_from_this<Object>{
public:
    Object();                                                                   // Type::Unit
    Object(long);                                                               // Type::Integer 
    Object(double);                                                             // Type::Float
    Object(std::vector<Object>);                                                // Type::List
    Object(std::vector<Object> params, Object ans, const Env& env);             // Type::Lambda
    Object(std::string, Fun);                                                   // Type::Builtin
    
    Object(const Object& other){
        this->m_type = other.m_type;
        this->m_value = other.m_value;
    }

    Object& operator=(const Object& other){
        if(this!=&other){
            this->m_type = other.m_type;
            this->m_value = other.m_value;
        }
        return *this;
    }

    // -
    static Object create_quote(Object obj);                                     // Quote
    static Object create_atom(std::string str);                                 // Atom
    static Object create_string(std::string str);                               // String

    // -*-
    std::shared_ptr<Object> get_pointer(){
        return shared_from_this();
    }
    // -*-

    Type type() const{ return this->m_type; }
    bool is_integer() const { return this->m_type==Type::Integer; }
    bool is_float() const { return this->m_type==Type::Float; }
    bool is_string() const { return this->m_type==Type::String; }

    // -*-
    std::vector<std::string> atoms();
    bool is_builtin() const;
    Object apply(std::vector<Object> args, Env& env);
    Object eval(Env& env);
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
    struct Builtin{
        std::string name;
        Fun fun;
    };
    struct Lambda{
        List params;
        std::shared_ptr<Object> body;
        Env env; // lambda
    };
    
    typedef std::variant<long, double, std::string, Lambda, Builtin, List> Value;
    
    Value m_value;

    // -*-
    void unwrap(Lambda& value){
        value = std::get<Lambda>(m_value);
    }
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
    Parser(const std::string& source);
    ~Parser() = default;

    // -*-
    static void replace(std::string &input, std::string old, std::string neo);
    std::vector<Object> parse();
private:
    void skip_whitespace();
    void skip_line();
    bool is_valid_atom_char();
    Object next_token();
    void skip_if(bool predicate);
    void read_unit(std::shared_ptr<Object>& objp);
    void read_quote(std::shared_ptr<Object>& objp);
    void read_list(std::shared_ptr<Object>& objp);
    void read_number(std::shared_ptr<Object>& objp);
    void read_string(std::shared_ptr<Object>& objp);
    void read_atom(std::shared_ptr<Object>& objp);
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
    static Object execute(Env& env);
    static Object execute(std::string source, Env& env);
    static Object execute(std::string filename);
    static Object repl(Env& env);
    static Env builtins;
};


// -*-------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                         -*-
// -*-------------------------------------------------------------------*-

#endif