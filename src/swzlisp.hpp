#ifndef SWZLISP_H
#define SWZLISP_H
#include<exception>
#include<stdexcept>
#include<functional>
#include<iostream>
#include<sstream>
#include<fstream>
#include<variant>
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
    Error(T value, const Env<T>& env, const char *cstr);
    Error(const Error& other);
    ~Error();

    std::string describe();

private:
    std::shared_ptr<T> m_reason;
    Env<T> m_env;
    const char* m_cstr;
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
    Object(const Object&);                                                      // Type::Quote
    Object(std::vector<Object> params, Object ans, const Env<Object>& env);     // Type::Lambda
    Object(std::string, Fun);                                                   // Type::Builtin

    // -
    static Object create_quote(Object obj);                                     // Quote
    static Object create_atom(std::string str);                                 // Atom
    static Object create_string(std::string& str);                              // String

    // -*-
    std::vector<std::string> atoms();
    bool is_builtin() const;
    Object apply(std::vector<Object> args, Env<Object>& env);
    Object eval(Env<Object>& env);
    bool is_number() const;
    bool as_bool() const;
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
    std::string to_string() const;
    std::string repr() const;

    friend std::ostream& operator<<(std::ostream& os, const Object& obj);

private:
    // long -> Integer
    // double -> Float
    // std::string -> String, Atom
    // Fun -> Builtin
    // std::vector<Object> -> List, Lambda, Quote
    Type m_type;
    typedef std::variant<long, double, std::string, Fun, std::vector<Object>> Value;
    Env<Object> m_env; // lambda
    Value m_value;

    // -*-
    void get_value(long& value){
        value = std::get<long>(m_value);
    }

    // -*-
    void get_value(double& value){
        value = std::get<double>(m_value);
    }

    // -*-
    void get_value(std::string& value){
        value = std::get<std::string>(m_value);
    }

    // -*-
    void get_value(Fun& value){
        value = std::get<Fun>(m_value);
    }
    
    // -*-
    void get_value(std::vector<Object>& value){
        value = std::get<std::vector<Object>>(m_value);
    }
};



// -*-------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                         -*-
// -*-------------------------------------------------------------------*-

#endif