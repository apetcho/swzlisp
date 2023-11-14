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
    //! @todo
    return false;
}

// -*-
long Object::as_integer() const{
    //! @todo
    return 0;
}

// -*-
double Object::as_float() const{
    //! @todo
    return 0.0;
}

// -*-
std::string Object::as_string() const{
    //! @todo
    return "";
}

// -*-
std::string Object::as_atom() const {
    //! @todo
    return "";
}

// -*-
std::vector<Object> Object::as_list() const{
    //! @todo
    return {};
}

// -*-
void Object::push(Object obj){
    //! @todo
}

// -*-
Object Object::pop(){
    //! @todo
    return Object();
}

// -*-
Object Object::to_integer() const {
    //! @todo
    return Object();
}

// -*-
Object Object::to_float() const {
    //! @todo
    return Object();
}

// -*-
bool Object::operator==(Object other) const{
    //! @todo
    return false;
}

// -*-
bool Object::operator!=(Object other) const{
    //! @todo
    return false;
}

// -*-
bool Object::operator>=(Object other) const{
    //! @todo
    return false;
}

// -*-
bool Object::operator<=(Object other) const{
    //! @todo
    return false;
}

// -*-
bool Object::operator<(Object other) const{
    //! @todo
    return false;
}

// -*-
bool Object::operator>(Object other) const{
    //! @todo
    return false;
}

// -*-
Object Object::operator+(Object other) const {
    //! @todo
    return Object();
}

// -*-
Object Object::operator-(Object other) const {
    //! @todo
    return Object();
}

// -*-
Object Object::operator*(Object other) const {
    //! @todo
    return Object();
}

// -*-
Object Object::operator/(Object other) const {
    //! @todo
    return Object();
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