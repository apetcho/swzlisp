#include "swzlisp.hpp"
#include<iomanip>

// -*-------------------------------------------------------------------*-
// -*- namespace::swzlisp                                              -*-
// -*-------------------------------------------------------------------*-
namespace swzlisp{
// -*-------------------------------------------------------------------*-
// -*- Object                                                          -*-
// -*-------------------------------------------------------------------*-
Object::Object(): m_type{Type::Unit}{}

// -*-
Object::Object(long val): m_type{Type::Integer}, m_value{val}{}

// -*-
Object::Object(double val): m_type{Type::Float}, m_value{val}{}

// -*-
Object::Object(std::vector<Object> list): m_type{Type::List}, m_value{list}{}

// -*-
Object::Object(std::vector<Object> params, Object body, const Env& env)
: m_type{Type::Lambda}{
    std::vector<Object> self;
    self.push_back(Object(params));
    Lambda lambda;
    lambda.params = params;
    lambda.body = std::make_shared<Object>(body);
    for(auto name: body.atoms()){
        if(env.contains(name)){
            auto val = env.get(name);
            lambda.env.put(name, val);
        }
    }
    this->m_value = lambda;
}

// -*-
Object::Object(std::string name, Fun fun): m_type{Type::Builtin}{
    Builtin builtin = {name, fun};
    this->m_value = builtin;
}

// -*-
Object Object::create_quote(Object obj){
    Object self;
    self.m_type = Type::Quote;
    std::vector<Object> vec;
    vec.push_back(obj);
    self.m_value = vec;
    return self;
}

// -*-
Object Object::create_atom(std::string str){
    Object self;
    self.m_type = Type::Atom;
    self.m_value = str;
    return self;
}

// -*-
Object Object::create_string(std::string str){
    Object self;
    self.m_type = Type::String;
    self.m_value = str;
    return self;
}

// -*-
std::vector<std::string> Object::atoms(){
    std::vector<std::string> result;
    std::vector<std::string> tmp;
    std::vector<Object> items;

    switch(this->m_type){
    case Type::Atom:
        result.push_back(this->as_atom());
        break;
    case Type::Quote:
        this->unwrap(items);
        result = items[0].atoms();
        break;
    case Type::Lambda:
        this->unwrap(items);
        result = items[1].atoms();
        break;
    case Type::List:
        this->unwrap(items);
        for(auto item: items){
            tmp = item.atoms();
            result.insert(result.end(), tmp.begin(), tmp.end());
        }
        break;
    default:
        break;
    }
    return result;
}

// -*-
bool Object::is_builtin() const{
    return this->m_type == Type::Builtin;
}

// -*-
Object Object::apply(std::vector<Object> args, Env& env){
    Env scope;
    Object result;
    List params;
    switch(this->m_type){
    case Type::Lambda:{
            Lambda lambda;
            unwrap(lambda);
            params = lambda.params;
            if(params.size() != args.size()){
                std::string msg = (
                    args.size() > params.size() ?
                    "No enough arguments" : "Too many arguments"
                );
                msg = swzlispExceptions[ErrorKind::SyntaxError] + ": " + msg;
                //auto xxx = *this;
                throw Error(env, msg.c_str());
            }
            lambda.env.set_parent(env.get_pointer());
            for(size_t i=0; i < params.size(); i++){
                if(params[i].m_type!=Type::Atom){
                    throw Error(env, ErrorKind::RuntimError);
                }
                Object obj = params[i];
                std::string name;
                obj.unwrap(name);
                lambda.env.put(name, args[i]);
            }
            result = lambda.body->eval(lambda.env);
        }//
        break;
    case Type::Builtin:{
            Builtin builtin;
            unwrap(builtin);
            result = builtin.fun(args, env);
        }//
        break;
    default:{
            std::string message = swzlispExceptions[ErrorKind::SyntaxError];
            message += ": expect a function or a lambda";
            //auto xxx = *this;
            throw Error(env, message.c_str());
        }//
        break;
    }
    return result;
}

// -*-
Object Object::eval(Env& env){
    Object result;
    switch(this->m_type){
    case Type::Quote:{
            List data;
            unwrap(data);
            result = data[0];
        }//
        break;
    case Type::Atom:{
            std::string data;
            unwrap(data);
            result = env.get(data);
        }//
        break;
    case Type::List:{
            List argv;
            Object fun;
            List data;
            unwrap(data);
            if(data.size() == 0){
                throw Error(env, ErrorKind::SyntaxError);
            }
            argv = std::vector<Object>(data.begin()+1, data.end());
            fun = data[0].eval(env);
            if(!fun.is_builtin()){
                for(size_t i=0; i < argv.size(); i++){
                    argv[i] = argv[i].eval(env);
                }
            }
            result = fun.apply(argv, env);
        }//
        break;
    default:
        result = *this;
        break;
    }
    return result;
}

// -*-
bool Object::is_number() const{
    return this->m_type==Type::Integer || this->m_type==Type::Float;
}

// -*-
bool Object::as_boolean() const{
    return *this != Object(long(0)) || *this != Object(double(0));
}

// -*-
long Object::as_integer() const{
    if(!this->is_number()){
        throw Error(Env(), ErrorKind::TypeError);
    }
    auto self = this->to_integer();
    long result;
    self.unwrap(result);
    return result;
}

// -*-
double Object::as_float() const{
    if(!this->is_number()){
        throw Error(Env(), ErrorKind::TypeError);
    }
    auto self = this->to_float();
    double result;
    self.unwrap(result);
    return result;
}

// -*-
std::string Object::as_string() const{
    if(this->m_type != Type::String){
        throw Error(Env(), ErrorKind::TypeError);
    }
    std::string result{};
    auto self = *this;
    self.unwrap(result);
    return result;
}

// -*-
std::string Object::as_atom() const {
    if(this->m_type != Type::Atom){
        throw Error(Env(), ErrorKind::TypeError);
    }
    auto self = *this;
    std::string result{};
    self.unwrap(result);
    return result;
}

// -*-
std::vector<Object> Object::as_list() const{
    if(this->m_type!=Type::List){
        throw Error(Env(), ErrorKind::TypeError);
    }
    auto self = *this;
    std::vector<Object> result = {};
    self.unwrap(result);
    return result;
}

// -*-
void Object::push(Object obj){
    if(this->m_type != Type::List){
        throw Error(Env(), ErrorKind::TypeError);
    }

    List& self = std::get<List>(this->m_value);
    self.push_back(obj);
}

// -*-
Object Object::pop(){
    if(this->m_type != Type::List){
        throw Error(Env(), ErrorKind::TypeError);
    }
    auto& self = std::get<List>(this->m_value);
    size_t len = self.size();
    auto result = self[len-1];
    self.pop_back();
    return result;
}

// -*-
Object Object::to_integer() const {
    if(!this->is_number()){
        throw Error(Env(), ErrorKind::TypeError);
    }
    if(this->m_type == Type::Integer){
        return *this;
    }
    // This object is as Float
    auto result = Object(static_cast<long>(this->as_float()));
    return result;
}

// -*-
Object Object::to_float() const {
    if(!this->is_number()){
        throw Error(Env(), ErrorKind::TypeError);
    }
    if(this->m_type == Type::Float){
        return *this;
    }
    // This object is an Integer
    auto result = Object(static_cast<double>(this->as_integer()));
    return result;
}

// -*-
bool Object::operator==(Object other) const{
    if(this->m_type==Type::Float && other.m_type==Type::Integer){
        return *this == other.to_float();
    }
    if(this->m_type==Type::Integer && other.m_type==Type::Float){
        return this->to_float()==other;
    }
    if(this->m_type != other.m_type){
        return false;
    }

    bool result = false;
    switch(this->m_type){
    case Type::Float:{
            double x, y;
            x = std::get<double>(this->m_value);
            other.unwrap(y);
            result = (x == y);
        }//
        break;
    case Type::Integer:{
            long x, y;
            x = std::get<long>(this->m_value);
            other.unwrap(y);
            result = (x == y);
        }//
        break;
    case Type::Builtin:{
            Builtin fn1;
            Builtin fn2;
            fn1 = std::get<Builtin>(this->m_value);
            other.unwrap(fn2);
            result = (
                fn1.name==fn2.name && fn1.fun==fn2.fun
            );
        }
        break;
    case Type::String:
    case Type::Atom:{
            std::string x, y;
            x = std::get<std::string>(this->m_value);
            other.unwrap(y);
            result = x == y;
        }//
        break;
    case Type::Lambda:
    case Type::List:{
            List x, y;
            x = std::get<List>(this->m_value);
            other.unwrap(y);
            result = (x == y);
        }//
        break;
    case Type::Quote:{
            List x, y;
            x = std::get<List>(this->m_value);
            other.unwrap(y);
            result = (x[0]==y[0]);
        }//
        break;
    default:
        result = true;
        break;
    }
    return result;
}

// -*-
bool Object::operator!=(Object other) const{
    return !(*this==other);
}

// -*-
bool Object::operator>=(Object other) const{
    return !(*this < other);
}

// -*-
bool Object::operator<=(Object other) const{
    return (*this == other) || (*this < other);
}

// -*-
bool Object::operator>(Object other) const{
    return !(*this <= other);
}

// -*-
bool Object::operator<(Object other) const{
    if(!this->is_number()){
        throw Error(Env(), ErrorKind::TypeError);
    }
    bool result = false;
    if(this->m_type==Type::Float){
        auto self = *this;
        double x, y;
        self.unwrap(x);
        other.to_float().unwrap(y);
        result = (x < y);
    }else if(this->m_type==Type::Integer){
        if(other.m_type==Type::Float){
            double x, y;
            this->to_float().unwrap(x);
            other.unwrap(y);
            result = (x < y);
        }else{
            long x, y;
            this->to_integer().unwrap(x);
            other.unwrap(y);
            result = (x < y);
        }
    }
    return result;
}

// -*-
Object Object::operator+(Object other) const {
    if(other.m_type==Type::Unit && this->is_number()){
        return *this;
    }
    if(this->m_type==Type::Unit && other.is_number()){
        return other;
    }

    if(!(this->is_number() && other.is_number())){
        throw Error(Env(), ErrorKind::SyntaxError);
    }
    Object result;

    if(this->m_type==Type::Float){
        double x, y;
        this->to_float().unwrap(x);
        other.to_float().unwrap(y);
        result.m_type = Type::Float;
        result.m_value = (x+y);
    }else if(this->m_type==Type::Integer){
        if(other.m_type==Type::Float){
            double x, y;
            this->to_float().unwrap(x);
            other.to_float().unwrap(y);
            result.m_type = Type::Float;
            result.m_value = (x + y);
        }else{
            long x, y;
            this->to_integer().unwrap(x);
            other.to_integer().unwrap(y);
            result.m_type = Type::Integer;
            result.m_value = (x+y);
        }
    }else{
        throw Error(Env(), ErrorKind::TypeError);
    }
    //! @note: ---- maybe implement the same operator for String & List

    return result;
}

// -*-
Object Object::operator-(Object other) const {
    if(other.m_type==Type::Unit && this->is_number()){
        return *this;
    }
    if(this->m_type==Type::Unit && other.is_number()){
        return other;
    }
    if(!(this->is_number() && other.is_number())){
        throw Error(Env(), ErrorKind::SyntaxError);
    }

    Object result;
    if(this->m_type==Type::Float){
        double x, y;
        this->to_float().unwrap(x);
        other.to_float().unwrap(y);
        result.m_type = Type::Float;
        result.m_value = (x - y);
    }else if(this->m_type==Type::Integer){
        if(other.m_type==Type::Float){
            double x, y;
            this->to_float().unwrap(x);
            other.to_float().unwrap(y);
            result.m_type = Type::Float;
            result.m_value = (x - y);
        }else{
            long x, y;
            this->to_integer().unwrap(x);
            other.to_integer().unwrap(y);
            result.m_type = Type::Integer;
            result.m_value = (x - y);
        }
    }else{
        throw Error(Env(), ErrorKind::TypeError);
    }

    return result;
}

// -*-
Object Object::operator*(Object other) const {
    if(this->m_type==Type::Unit && other.is_number()){ return other; }
    if(other.m_type==Type::Unit && other.is_number()){ return *this; }
    if(!(this->is_number() && other.is_number())){
        throw Error(Env(), ErrorKind::SyntaxError);
    }

    Object result;
    if(this->m_type==Type::Float){
        double x, y;
        this->to_float().unwrap(x);
        other.to_float().unwrap(y);
        result.m_type = Type::Float;
        result.m_value = (x*y);
    }else if(this->m_type==Type::Integer){
        if(other.m_type==Type::Float){
            double x, y;
            this->to_float().unwrap(x);
            other.to_float().unwrap(y);
            result.m_type = Type::Float;
            result.m_value = (x * y);
        }else{
            long x, y;
            this->to_integer().unwrap(x);
            other.to_integer().unwrap(y);
            result.m_type = Type::Integer;
            result.m_value = (x * y);
        }
    }else{
        throw Error(Env(), ErrorKind::TypeError);
    }
    return result;
}

// -*-
Object Object::operator/(Object other) const {
    if(this->m_type==Type::Unit && other.is_number()){
        return other;
    }
    if(other.m_type==Type::Unit && this->is_number()){
        return *this;
    }

    if(!(this->is_number() && other.is_number())){
        throw Error(Env(), ErrorKind::SyntaxError);
    }

    auto almost_equal = [](double x, double y) -> bool {
        double eps = std::numeric_limits<double>::epsilon();
        double delta = std::fabs(x-y);
        x = std::fabs(x);
        y = std::fabs(y);
        double xymax = (x > y) ? x : y;
        bool result = (delta <= xymax*eps) ? true: false;
        return result;
    };

    Object result;
    if(this->m_type==Type::Float){
        double x, y;
        this->to_float().unwrap(x);
        other.to_float().unwrap(y);
        if(almost_equal(y, 0.0)){
            throw Error(Env(), ErrorKind::ZeroDivisionError);
        }
        result.m_type = Type::Float;
        result.m_value = (x / y);
    }else if(this->m_type==Type::Integer){
        if(other.m_type==Type::Float){
            double x, y;
            this->to_float().unwrap(x);
            other.to_float().unwrap(y);
            if(almost_equal(y, 0.0)){
                throw Error(Env(), ErrorKind::ZeroDivisionError);
            }
            result.m_type = Type::Float;
            result.m_value = (x / y);
        }else{
            long x, y;
            this->to_integer().unwrap(x);
            other.to_integer().unwrap(y);
            if(y==0){
                throw Error(Env(), ErrorKind::ZeroDivisionError);
            }
            result.m_type = Type::Integer;
            result.m_value = (x / y);
        }
    }else{
        throw Error(Env(), ErrorKind::TypeError);
    }
    return result;
}

// -*-
Object Object::operator%(Object other) const {
    if(this->m_type==Type::Unit && other.is_number()){
        return other;
    }
    if(other.m_type==Type::Unit && this->is_number()){
        return *this;
    }

    if(!(this->is_number() && other.is_number())){
        throw Error(Env(), ErrorKind::SyntaxError);
    }

    auto almost_equal = [](double x, double y) -> bool {
        double eps = std::numeric_limits<double>::epsilon();
        double delta = std::fabs(x-y);
        x = std::fabs(x);
        y = std::fabs(y);
        double xymax = (x > y) ? x : y;
        bool result = (delta <= xymax*eps) ? true: false;
        return result;
    };

    Object result;
    if(this->m_type==Type::Float){
        double x, y;
        this->to_float().unwrap(x);
        other.to_float().unwrap(y);
        if(almost_equal(y, 0.0)){
            throw Error(Env(), ErrorKind::ZeroDivisionError);
        }
        result.m_type = Type::Float;
        result.m_value = std::fmod(x, y);
    }else if(this->m_type==Type::Integer){
        if(other.m_type==Type::Float){
            double x, y;
            this->to_float().unwrap(x);
            other.to_float().unwrap(y);
            if(almost_equal(y, 0.0)){
                throw Error(Env(), ErrorKind::ZeroDivisionError);
            }
            result.m_type = Type::Float;
            result.m_value = std::fmod(x, y);
        }else{
            long x, y;
            this->to_integer().unwrap(x);
            other.to_integer().unwrap(y);
            if(y==0){
                throw Error(Env(), ErrorKind::ZeroDivisionError);
            }
            result.m_type = Type::Integer;
            result.m_value = (x % y);
        }
    }else{
        throw Error(Env(), ErrorKind::RuntimError);
    }
    return result;
}

// -*-
std::string Object::type_name(){
    std::string result = swzlispTypes[this->m_type];
    return result;
}

// -*-
std::string Object::str(){
    std::string result;
    switch(this->m_type){
    case Type::Quote:{
            List data;
            unwrap(data);
            result = "'" + data[0].repr();
        }//
        break;
    case Type::Atom:{
            unwrap(result);
        }//
        break;
    case Type::Integer:{
            long data;
            this->to_integer().unwrap(data);
            std::ostringstream stream;
            stream << data;
            result = stream.str();
        }//
        break;
    case Type::Float:{
            double data;
            this->to_float().unwrap(data);
            std::ostringstream stream;
            stream << data;
            result = stream.str();
        }//
        break;
    case Type::String:{
            unwrap(result);
        }//
        break;
    case Type::Lambda:{
            std::string data = "";
            List items;
            unwrap(items);
            for(auto item: items){
                data += item.repr() + " ";
            }
            data = std::string(data.begin(), data.end()-1);
            result = "(lambda " + data + ")";
        }//
        break;
    case Type::List:{
            List items;
            unwrap(items);
            std::string data = "";
            for(auto item: items){
                data += item.repr() + " ";
            }
            data = std::string(data.begin(), data.end()-1);
            result = "(" + data + ")";
        }//
        break;
    case Type::Builtin:{
            Builtin builtin;
            unwrap(builtin);
            std::ostringstream stream;
            stream << "<Procedure::" << builtin.name << "@";
            stream << "0x" << std::hex << reinterpret_cast<std::uint64_t>(builtin.fun) << ">";
            result = stream.str();
        }//
        break;
    case Type::Unit:
        result = "@";
        break;
    default:
        throw Error();
    }
    return result;
}

// -*-
std::string Object::repr() {
    std::string result{};
    switch(this->m_type){
    case Type::Quote:{
            List data;
            unwrap(data);
            result = "'" + data[0].repr();
        }//
        break;
    case Type::Atom:{
            unwrap(result);
        }//
        break;
    case Type::Integer:{
            long x;
            this->to_integer().unwrap(x);
            std::ostringstream stream;
            stream << x;
            result = stream.str();
        }//
        break;
    case Type::Float:{
            double x;
            this->to_float().unwrap(x);
            std::ostringstream stream;
            stream << x;
            result = stream.str();
        }//
        break;
    case Type::String:{
            std::string data;
            unwrap(data);
            for(size_t i=0; i < data.size(); i++){
                if(data[i]=='"'){ result += "\\\""; }
                else{ result.push_back(data[i]); }
            }
            result = "\"" + data + "\"";
        }//
        break;
    case Type::Lambda:{
            List items;
            unwrap(items);
            std::string data = "";
            for(auto item: items){
                data += item.repr() + " ";
            }
            data = std::string(data.begin(), data.end()-1);
            result = "(lambda " + data + ")";
        }//
        break;
    case Type::List:{
            List items;
            unwrap(items);
            std::string data = "";
            for(auto item: items){
                data += item.repr() + " ";
            }
            data = std::string(data.begin(), data.end()-1);
            result = "(" + data + ")";
        }//
        break;
    case Type::Builtin:{
            Builtin builtin;
            unwrap(builtin);
            std::ostringstream stream;
            // + builtin.name 
            stream << "<Procedure::" << "@0x";
            stream << std::hex << reinterpret_cast<std::uint64_t>(builtin.fun) << ">";
            result = stream.str();
        }//
        break;
    case Type::Unit:
        result = "@";
        break;
    default:
        throw Error();
    }
    return result;
}

// -*-------------------------------------------------------------------*-
// -*- Error                                                           -*-
// -*-------------------------------------------------------------------*-
// -*-
static std::string get_default_error_message(ErrorKind err){
    std::string result{};
    switch(err){
    case ErrorKind::RuntimError:
        result = "unknown error";
        break;
    case ErrorKind::SyntaxError:
        result = "invalid syntax";
        break;
    case ErrorKind::TypeError:
        result = "type mismatch";
        break;
    case ErrorKind::ValueError:
        result = "invalid value or argument";
        break;
    case ErrorKind::ZeroDivisionError:
        result = "invalid operation. Division by zero";
        break;
    }
    return result;
}

// -*-
Error::Error(){
    this->m_error = ErrorKind::RuntimError;
    //this->m_reason = Object().get_pointer();
    this->m_env = Env();
    this->m_message = "";
}

/*
ErrorKind m_error;
std::shared_ptr<T> m_reason;
Env<T> m_env;
const char* m_message;
*/
// -*-
Error::Error(const Env& env, ErrorKind err){
    this->m_error = err;
    //this->m_reason = value.get_pointer();
    this->m_env = env;
    auto entry = swzlispExceptions.find(err);
    if(entry == swzlispExceptions.end()){
        this->m_error = ErrorKind::RuntimError;
    }
    this->m_message = "";
}

// -*-
Error::Error(const Env& env, const char *message){
    this->m_error = ErrorKind::RuntimError;
    //this->m_reason = value.get_pointer();
    this->m_env = env;
    this->m_message = message;
}

// -*-
Error::Error(const Error& other){
    this->m_error = other.m_error;
    this->m_env = other.m_env;
    //this->m_reason = other.m_reason;
    this->m_message = other.m_message;
}


std::string Error::describe(){
    std::string result = swzlispExceptions[this->m_error] + ": ";
    if(this->m_message==""){
        result += get_default_error_message(this->m_error);
    }else{
        result += this->m_message;
    }
    return result;
}

// -*-------------------------------------------------------------------*-
// -*- Env                                                             -*-
// -*-------------------------------------------------------------------*-
Env::Env(){
    this->m_bindings = {};
    this->m_parent = nullptr;
}

Env::Env(const Env& other){
    this->m_bindings = {};
    for(auto entry=other.m_bindings.begin(); entry!=other.m_bindings.end(); entry++){
        this->m_bindings[entry->first] = entry->second;
    }
    this->m_parent = other.m_parent;
}

bool Env::contains(const std::string& name) const {
    bool result = false;
    auto ptr = this->m_bindings.find(name);
    if(ptr!=this->m_bindings.end()){
        result = true;
    }else{
        result = this->m_parent->contains(name);
    }
    return result;
}

const Object& Env::get(std::string name) const {
    auto entry = this->m_bindings.find(name);
    if(entry != this->m_bindings.end()){
        return entry->second;
    }else if(this->m_parent != nullptr){
        entry = this->m_parent->m_bindings.find(name);
        if(entry != this->m_parent->m_bindings.end()){
            return entry->second;
        }else{
            return this->m_parent->get(name);
        }
    }
    throw std::runtime_error(
        "'" + name + "' has no binding in the current environment"
    );
}

// -*-
void Env::put(std::string name, Object& value){
    this->m_bindings[name] = value;
}

void Env::merge(const Env& other){
    auto entry = other.m_bindings.begin();
    while(entry != other.m_bindings.end()){
        this->m_bindings[entry->first] = entry->second;
        entry++;
    }
}

// -*-------------------------------------------------------------------*-
// -*- operator<<                                                      -*-
// -*-------------------------------------------------------------------*-
// -*-
std::ostream& operator<<(std::ostream& os, const Object& obj){
    Object self(obj);
    os << self.str();
    return os;
}

// -*-
std::ostream& operator<<(std::ostream& os, const Env& env){
    os << "{ " << std::endl;
    for(auto entry: env.m_bindings){
        os << "    '" << std::setw(12) << std::left << entry.first << ": " << entry.second.repr() << ",\n";
    }
    os << "}" << std::endl;
    return os;
}

// -*-------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                         -*-
// -*-------------------------------------------------------------------*-