// ast.hpp - AST structure for HerLang
#pragma once
#include "lexer.hpp"
#include <string>
#include <vector>
#include <memory>

// Simplified safe array for AST nodes
template<typename T>
class SafeArray {
private:
    std::vector<T> data;
public:
    SafeArray(const std::vector<T>& vec, const std::string& = "") : data(vec) {}
    SafeArray(std::initializer_list<T> init, const std::string& = "") : data(init) {}
    
    T at(size_t index) { return data.at(index); }
    const T& at(size_t index) const { return data.at(index); }
    T operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }
    
    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    
    typename std::vector<T>::iterator begin() { return data.begin(); }
    typename std::vector<T>::iterator end() { return data.end(); }
    typename std::vector<T>::const_iterator begin() const { return data.begin(); }
    typename std::vector<T>::const_iterator end() const { return data.end(); }
};

struct Statement {
    virtual ~Statement() = default;
};

class SayStatement : public Statement {
public:
    SafeArray<std::string> args;
    SafeArray<bool> is_vars;
    std::string end;

    SayStatement(const std::vector<std::string>& args_vec,
        const std::vector<bool>& is_vars_vec,
        const std::string& end)
        : args(args_vec, "say_args"), is_vars(is_vars_vec, "say_vars"), end(end) {}
};


struct SetStatement : public Statement {
    std::string var;
    std::string type_annotation;
    std::string initial_value;
    
    SetStatement(const std::string& var, const std::string& type = "", const std::string& value = "") 
        : var(var), type_annotation(type), initial_value(value) {}
};

struct FunctionCall : Statement {
    std::string name;
    std::string arg;
    TokenType arg_type;
    FunctionCall(const std::string& n, const std::string& a, TokenType t = TokenType::EOFToken)
        : name(n), arg(a), arg_type(t) {}
};


struct FunctionDef : public Statement {
    std::string name;
    std::string param;
    std::vector<std::shared_ptr<Statement>> body;
    FunctionDef(const std::string& name, const std::string& param,
        const std::vector<std::shared_ptr<Statement>>& body)
        : name(name), param(param), body(body) {}
};

struct StartBlock : public Statement {
    std::vector<std::shared_ptr<Statement>> body;
    StartBlock(const std::vector<std::shared_ptr<Statement>>& body)
        : body(body) {}
};

// Parallel execution block
struct ParallelBlock : public Statement {
    std::vector<std::shared_ptr<Statement>> tasks;
    bool wellness_aware;
    
    ParallelBlock(const std::vector<std::shared_ptr<Statement>>& tasks, bool wellness = true)
        : tasks(tasks), wellness_aware(wellness) {}
};

// Safe memory allocation statement
struct SafeAllocStatement : public Statement {
    std::string var_name;
    std::string element_type;
    std::string size_expr;
    std::string context;
    
    SafeAllocStatement(const std::string& var, const std::string& type, 
                      const std::string& size, const std::string& ctx = "")
        : var_name(var), element_type(type), size_expr(size), context(ctx) {}
};

// Shared state declaration
struct SharedStateDecl : public Statement {
    std::string var_name;
    std::string type_annotation;
    std::string initial_value;
    
    SharedStateDecl(const std::string& var, const std::string& type, const std::string& value)
        : var_name(var), type_annotation(type), initial_value(value) {}
};

struct AST {
    std::vector<std::shared_ptr<Statement>> statements;
};
