#include "swzlisp.hpp"

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
Object::Object(std::vector<Object> params, Object ans, const Env<Object>& env)
: m_type{Type::Lambda}{
    std::vector<Object> self;
    self.push_back(Object(params));
    self.push_back(ans);
    auto _atoms = ans.atoms();
    for(auto name: _atoms){
        if(env.contains(name)){
            this->m_env.put(name, env.get(name));
        }
    }
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
Object Object::apply(std::vector<Object> args, Env<Object>& env){
    //! @todo
    return Object();
}

// -*-
Object Object::eval(Env<Object>& env){
    //! @todo
    return Object();
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
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }
    auto self = this->to_integer();
    long result;
    self.unwrap(result);
    return result;
}

// -*-
double Object::as_float() const{
    if(!this->is_number()){
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }
    auto self = this->to_float();
    double result;
    self.unwrap(result);
    return result;
}

// -*-
std::string Object::as_string() const{
    if(this->m_type != Type::String){
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }
    std::string result{};
    auto self = *this;
    self.unwrap(result);
    return result;
}

// -*-
std::string Object::as_atom() const {
    if(this->m_type != Type::Atom){
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }
    auto self = *this;
    std::string result{};
    self.unwrap(result);
    return result;
}

// -*-
std::vector<Object> Object::as_list() const{
    if(this->m_type!=Type::List){
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }
    auto self = *this;
    std::vector<Object> result = {};
    self.unwrap(result);
    return result;
}

// -*-
void Object::push(Object obj){
    if(this->m_type != Type::List){
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }

    List& self = std::get<List>(this->m_value);
    self.push_back(obj);
}

// -*-
Object Object::pop(){
    if(this->m_type != Type::List){
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
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
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
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
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
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
            auto self = *this;
            self.unwrap(x);
            other.unwrap(y);
            result = (x == y);
        }//
        break;
    case Type::Integer:{
            long x, y;
            auto self = *this;
            self.unwrap(x);
            other.unwrap(y);
            result = (x == y);
        }//
        break;
    case Type::Builtin:{
            Builtin fn1;
            Builtin fn2;
            auto self = *this;
            self.unwrap(fn1);
            other.unwrap(fn2);
            result = (
                fn1.first==fn2.first && fn1.second==fn2.second
            );
        }
        break;
    case Type::String:
    case Type::Atom:{
            std::string x, y;
            auto self = *this;
            self.unwrap(x);
            other.unwrap(y);
            result = x == y;
        }//
        break;
    case Type::Lambda:
    case Type::List:{
            List x, y;
            auto self = *this;
            self.unwrap(x);
            other.unwrap(y);
            result = (x == y);
        }//
        break;
    case Type::Quote:{
            List x, y;
            auto self = *this;
            self.unwrap(x);
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
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
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
        throw Error(*this, Env<Object>(), ErrorKind::SyntaxError);
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
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
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
        throw Error(*this, Env<Object>(), ErrorKind::SyntaxError);
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
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }

    return result;
}



// -*-
Object Object::operator*(Object other) const {
    if(this->m_type==Type::Unit && other.is_number()){ return other; }
    if(other.m_type==Type::Unit && other.is_number()){ return *this; }
    if(!(this->is_number() && other.is_number())){
        throw Error(*this, Env<Object>(), ErrorKind::SyntaxError);
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
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
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
        throw Error(*this, Env<Object>(), ErrorKind::SyntaxError);
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
            throw Error(*this, Env<Object>(), ErrorKind::ZeroDivisionError);
        }
        result.m_type = Type::Float;
        result.m_value = (x / y);
    }else if(this->m_type==Type::Integer){
        if(other.m_type==Type::Float){
            double x, y;
            this->to_float().unwrap(x);
            other.to_float().unwrap(y);
            if(almost_equal(y, 0.0)){
                throw Error(*this, Env<Object>(), ErrorKind::ZeroDivisionError);
            }
            result.m_type = Type::Float;
            result.m_value = (x / y);
        }else{
            long x, y;
            this->to_integer().unwrap(x);
            other.to_integer().unwrap(y);
            if(y==0){
                throw Error(*this, Env<Object>(), ErrorKind::ZeroDivisionError);
            }
            result.m_type = Type::Integer;
            result.m_value = (x / y);
        }
    }else{
        throw Error(*this, Env<Object>(), ErrorKind::TypeError);
    }
    return result;
}

// -*-
Object Object::operator%(Object other) const {
    //! @todo
    return Object();
}

// -*-
std::string Object::type_name(){
    //! @todo
    return "";
}

// -*-
std::string Object::to_string() const {
    //! @todo
    return "";
}

// -*-
std::string Object::repr() const {
    //! @todo
    return "";
}

// -*-------------------------------------------------------------------*-
// -*- Error                                                           -*-
// -*-------------------------------------------------------------------*-
template<typename T>
Error<T>::Error(){}

template<typename T>
Error<T>::Error(T value, const Env<T>& env, const char *message){}

template<typename T>
Error<T>::Error(const Error& other){}

template<typename T>
std::string Error<T>::describe(){}

// -*-------------------------------------------------------------------*-
// -*- Env                                                             -*-
// -*-------------------------------------------------------------------*-
template<typename T>
Env<T>::Env(){}

template<typename T>
Env<T>::Env(const Env<T>& other){}

template<typename T>
bool Env<T>::contains(const std::string& name) const {
    //! @todo
    return false;
}

template<typename T>
T Env<T>::get(const std::string& name) const{
    //! @todo
    return T{};
}

template<typename T>
void Env<T>::put(std::string name, T value){}

// -*-------------------------------------------------------------------*-
// -*- operator<<                                                      -*-
// -*-------------------------------------------------------------------*-
// -*-
std::ostream& operator<<(std::ostream& os, const Object& obj){
    //! @todo
    return os;
}

// -*-
std::ostream& operator<<(std::ostream& os, const Env<Object>& enf){
    //! @todo
    return os;
}

// -*-------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                         -*-
// -*-------------------------------------------------------------------*-