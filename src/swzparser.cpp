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

// -*------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                        -*-
// -*------------------------------------------------------------------*-