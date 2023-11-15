#include "swzlisp.hpp"
#include<cctype>

// -*------------------------------------------------------------------*-
// -*- namespace::swzlisp                                             -*-
// -*------------------------------------------------------------------*-
namespace swzlisp{
// -*-
Parser::Parser(const std::string& source): m_source{source}{
    this->m_begin = this->m_source.cbegin();
    this->m_end = this->m_source.cend();
    this->m_iter = this->m_source.begin();
}

// -*-
void Parser::skip_whitespace(){
    while(this->m_iter != this->m_end && std::isspace(*this->m_iter)){
        this->m_iter++;
    }
}

Object Parser::next_token(){
    //! @todo
    return Object();
}

// -*-
bool Parser::is_valid_symbol_char(){
    bool result = (
        (std::isalpha(*this->m_iter) || std::ispunct(*this->m_iter)) &&
        *this->m_iter != '(' && *this->m_iter != ')' &&
        *this->m_iter != '"' && *this->m_iter != '\''
    );

    return result;
}

// -*-
void Parser::replace(std::string& source, std::string old, std::string neo){
    auto idx = source.find(old, 0);
    while(idx != std::string::npos){
        source.replace(idx, old.size(), neo);
        idx += neo.size();
        idx = source.find(old, idx);
    }
}

// -*-
void Parser::skip_line(){
    while(*this->m_iter !='\n'){ this->m_iter++; }
}

// -*-
void Parser::skip_if(bool predicate){
    if(predicate){ this->m_iter++; }
}

// -*-
void Parser::read_unit(std::shared_ptr<Object>& unit){
    std::string::iterator ptr = this->m_iter;
    if(*this->m_iter == '('){
        this->skip_if(*this->m_iter=='(');
        this->skip_whitespace();
        if(*this->m_iter == ')'){
            if(unit != nullptr){ unit.reset(); }
            unit = std::make_shared<Object>();
            this->skip_if((*this->m_iter==')'));
            //return unit;
        }
    }else{
        // otherwise, this is a list containing at least one element
        this->m_iter = ptr; // reset the pointer to its original place
    }
}

// -*-
void Parser::read_quote(std::shared_ptr<Object>& objp){
    std::string::iterator ptr = this->m_iter;
    bool predicate = (*this->m_iter=='\'');
    if(predicate){
        this->skip_if(predicate);
        Object self = Object::create_quote(this->next_token());
        if(objp != nullptr){ objp.reset(); }
        objp = std::make_shared<Object>(self);
    }else{
        throw Error();
    }
}

// -*-
void Parser::read_list(std::shared_ptr<Object>& objp){
    std::string::iterator ptr = this->m_iter;
    bool predicate = (*this->m_iter == '(');
    this->skip_if(predicate);
    this->skip_whitespace();
    // To construct an empty list, we write:
    // (list)
    // otherwise
    // () result int Type::Unit
    if(*this->m_iter !=')'){
        Object self = Object(std::vector<Object>());
        while(*this->m_iter !=')'){
            self.push(this->next_token());
        }
        this->skip_whitespace();
        predicate = (*this->m_iter == ')');
        this->skip_if(predicate);
        if(objp != nullptr){ objp.reset();}
        objp = std::make_shared<Object>(self);
    }
}

// -*-
void Parser::read_number(std::shared_ptr<Object>& objp){
    bool negate = (*this->m_iter == '-');
    if(negate){ this->m_iter++; }
    std::string::iterator ptr = this->m_iter;
    auto is_number_char = [&ptr]() -> bool{
        std::string chars = "0123456789.eE";
        return chars.find(*ptr) != std::string::npos;
    };
    while(is_number_char()){ ptr++;}

    std::string number = std::string(this->m_iter, ptr);
    this->skip_whitespace();
    if(objp != nullptr){ objp.reset(); }
    if(number.find('.') != std::string::npos){
        double val = std::strtod(number.c_str(), NULL);
        val = negate ? -1.0* val : val;
        auto self = Object(val);
        objp = std::make_shared<Object>(self);
    }else{
        long val = std::strtol(number.c_str(), nullptr, 10);
        val = negate ? -1*val : val;
        auto self = Object(val);
        objp = std::make_shared<Object>(self);
    }
    this->m_iter = ptr;
}

// -*------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                        -*-
// -*------------------------------------------------------------------*-