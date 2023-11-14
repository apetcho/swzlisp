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

class Object;
//class Env;
typedef Object (*Fun)(std::vector<Object>, Env<Object>&);


// -*-
class Object{
public:
    Object();                                           // Type::Unit
    Object(long);                                       // Type::Integer 
    Object(double);                                     // Type::Float
    Object(std::vector<Object>);                        // Type::List
    Object(const Object&);                              // Type::Quote
    Object(std::string, Type);                          // Type::Atom or Type::String
    Object(std::vector<Object>, Object, const Env<Object>&);    // Type::Lambda
    Object(std::string, Fun);      // Type::Builtin


private:
};



// -*-------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                         -*-
// -*-------------------------------------------------------------------*-

#endif