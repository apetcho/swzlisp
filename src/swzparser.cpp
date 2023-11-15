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
            unit = std::make_shared<Object>();
            this->skip_if((*this->m_iter==')'));
            //return unit;
        }
    }else{
        // otherwise, this is a list containing at least one element
        this->m_iter = ptr; // reset the pointer to its original place
    }
}

// -*------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                        -*-
// -*------------------------------------------------------------------*-