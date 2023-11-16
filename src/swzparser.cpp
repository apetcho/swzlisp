#include "swzlisp.hpp"
#include<cstdlib>
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

// -*-
bool Parser::is_valid_atom_char(){
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
Object Parser::read_unit(){
    std::string::iterator ptr = this->m_iter;
    Object unit;
    bool ok = false;
    bool predicate = (*this->m_iter == '(');
    this->skip_if(predicate);
    this->skip_whitespace();
    if(*this->m_iter == ')'){
        unit = Object();
        predicate = (*this->m_iter == ')');
        this->skip_if(predicate);
        ok = true;
    }
    // otherwise, this is a list containing at least one element
    this->m_iter = ptr; // reset the pointer to its original place
    if(!ok){
        // Jump to parse to read an actual list, therefore this is not an
        // actual Unit but a list.
        throw Error();
    }
    return unit;
}

// -*-
Object Parser::read_quote(){
    std::string::iterator ptr = this->m_iter;
    bool predicate = (*this->m_iter=='\'');
    Object result;
    if(predicate){
        this->skip_if(predicate);
        result = Object::create_quote(this->next_token());
    }else{
        throw Error();
    }
    return result;
}

// -*-
Object Parser::read_list(){
    std::string::iterator ptr = this->m_iter;
    bool predicate = (*this->m_iter == '(');
    this->skip_if(predicate);
    this->skip_whitespace();
    // To construct an empty list, we write:
    // (list)
    // otherwise
    // () result int Type::Unit
    Object result;
    if(*this->m_iter !=')'){
        result = Object(std::vector<Object>());
        while(*this->m_iter !=')'){
            result.push(this->next_token());
        }
        this->skip_whitespace();
        predicate = (*this->m_iter == ')');
        this->skip_if(predicate);
    }
    return result;
}

// -*-
Object Parser::read_number(){
    std::string::iterator ptr = this->m_iter;
    auto is_number_char = [&ptr]() -> bool {
        std::string chars = ".-+0123456789eE";
        return chars.find(*ptr) != std::string::npos;
    };
    while(is_number_char()){ ptr++;}

    Object result;

    std::string number = std::string(this->m_iter, ptr);
    this->skip_whitespace();
    if(number.find('.') != std::string::npos){
        double val{};
        try{
            val = std::stod(number);
        }catch(std::invalid_argument& err){
            auto msg = err.what();
            auto obj = Object();
            throw Error(obj, Env(), msg);
        }catch(std::out_of_range& err){
            auto msg = err.what();
            auto obj = Object();
            throw Error(obj, Env(), msg);
        }    
        result = Object(val);
    }else{
        long val{};
        try{
            val = std::stol(number);
        }catch(std::invalid_argument& err){
            auto msg = err.what();
            auto obj = Object();
            throw Error(obj, Env(), msg);
        }catch(std::out_of_range& err){
            auto msg = err.what();
            auto obj = Object();
            throw Error(obj, Env(), msg);
        }
        result = Object(val);
    }
    this->m_iter = ptr;

    return result;
}

// -*-
Object Parser::read_string(){
    std::string::iterator ptr = this->m_iter;
    while(*(++ptr)!='\"'){
        if(ptr==this->m_end){
            auto unit = Object();
            auto error = Error(unit, Env(), ErrorKind::SyntaxError);
            throw Error(error);
        }
        if(*ptr == '\\'){ ++ptr; }
    }
    Object result;
    std::string data = std::string(this->m_iter, ptr);
    ++ptr;
    this->m_iter = ptr;
    this->skip_whitespace();
    for(auto i=0; i < data.size(); i++){
        if(data[i] == '\\' && data[i+1]=='\\'){
            data.replace(i, 2, "\\");
        }else if(data[i]=='\\' && data[i+1]=='"'){
            data.replace(i, 2, "\"");
        }else if(data[i]=='\\' && data[i+1]=='n'){
            data.replace(i, 2, "\n");
        }else if(data[i]=='\\' && data[i+1]=='t'){
            data.replace(i, 2, "\t");
        }
    }

    result = Object::create_string(data);
    return result;
}

// -*-
Object Parser::read_atom(){
    std::string::iterator ptr = this->m_iter;
    while(this->is_valid_atom_char()){
        if(this->m_iter == this->m_end){
            auto self = Object();
            throw Error(self, Env(), ErrorKind::SyntaxError);
        }
        this->m_iter++;
    }

    Object result;
    std::string data = std::string(ptr, this->m_iter);
    this->skip_whitespace();
    result = Object::create_atom(data);
    return result;
}

// -*-
Object Parser::next_token(){
    this->skip_whitespace();
    if(*this->m_iter == ';'){
        bool predicate = (*this->m_iter==';');
        this->skip_if(predicate);
        this->skip_line();
        this->skip_whitespace();
        std::string::iterator ptr = this->m_iter;
        std::string::iterator end = this->m_source.end()-1;
        auto data = std::string(ptr, end);
        if(data == ""){
            return Object();
        }
    }

    Object result;
    if(this->m_iter == this->m_end){
        return Object();
    }else if(*this->m_iter=='\''){
        result = this->read_quote();
    }else if(*this->m_iter=='('){
        try{
            result = this->read_unit();
        }catch(Error& err){
            result = this->read_list();
        }
    }else if(std::isdigit(*this->m_iter)||(*this->m_iter=='-' && std::isdigit(*(this->m_iter+1)))){
        result = this->read_number();
    }else if(*this->m_iter=='\"'){
        result = this->read_string();
    }else if(this->is_valid_atom_char()){
        result = this->read_atom();
    }else{
        Object self = Object();
        std::string msg = "Malformed program";
        auto error = Error(self, Env(), msg.c_str());
        throw Error(error);
    }

    return result;
}

// -*-
std::vector<Object> Parser::parse(){
    std::vector<Object> result{};
    while(this->m_iter != this->m_end){
        result.push_back(this->next_token());
    }

    if(this->m_iter != this->m_end){
        Object self = Object();
        std::string msg = "Malformed program";
        auto error = Error(self, Env(), msg.c_str());
        throw Error(error);
    }

    return result;
}

// -*------------------------------------------------------------------*-
}//-*- end::namespace::swzlisp                                        -*-
// -*------------------------------------------------------------------*-