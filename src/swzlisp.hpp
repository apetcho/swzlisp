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


// -*-------------------------------------------------------------------*-
// -*- namespace::swzlisp                                              -*-
// -*-------------------------------------------------------------------*-
namespace swzlisp{
// -*-
enum class Type{
    Unit,
    Atom,
    Quote,
    Integer,
    Float,
    String,
    List,
    Lambda,
    Builtin,
};

// -*-
class Object;
class Env;

// -*-

class Base{
public:
    Base()=default;
    virtual ~Base() = default;

    virtual std::string type_name() = 0;
    virtual std::string to_string() = 0;
    virtual std::string repr() = 0;
};


typedef Object (*Fun)(std::vector<Object>, Env&);

// Unit()
class Unit: public Base {
public:
    Unit(): Base() {}

    std::string type_name() override{
        return "unit";
    }

    std::string to_string() override {
        return "()";
    }

    std::string repr() override{
        return "()";
    }
};

// -*-
// Quote(Object)
class Quote{};

// Atom(std::string)
class Atom{};

// Integer(long)
class Integer {};

// Float(double)
class Float{};

// String(std::string)
class String{};

// List(std::vector<Object>)
class List;

// Lambda(std::vector<Object>, Object, const Env&)
class Lambda;

// Builtin(std::string, Fun)
class Builtin;


// -*-
class Error;

// -*-
class Env;

// -*-
class Object{
public:
    Object();                                           // Type::Unit
    Object(long);                                       // Type::Integer 
    Object(double);                                     // Type::Float
    Object(std::vector<Object>);                        // Type::List
    Object(const Object&);                              // Type::Quote
    Object(std::string, Type);                          // Type::Atom or Type::String
    Object(std::vector<Object>, Object, const Env&);    // Type::Lambda
    Object(std::string, std::shared_ptr<Builtin>);      // Type::Builtin


private:
};


// -*-------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                         -*-
// -*-------------------------------------------------------------------*-

#endif