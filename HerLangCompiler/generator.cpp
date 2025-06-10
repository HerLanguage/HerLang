// generator.cpp - AST to C++ generator
#include "generator.hpp"
#include "ast.hpp"
#include "type_system.hpp"
#include <sstream>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <iostream>


static std::string indent(int level) {
    return std::string(level * 4, ' ');
}

static std::string escape_string(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

static void gen_stmt(std::ostringstream& out, const std::shared_ptr<Statement>& stmt, int indent_level = 1) {
    std::string ind = indent(indent_level);

    if (auto say = dynamic_cast<SayStatement*>(stmt.get())) {
        out << ind << "std::cout";
        for (size_t i = 0; i < say->args.size(); ++i) {
            out << " << ";
            if (say->is_vars[i]) {
                out << say->args[i];
            }
            else {
                out << "\"" << escape_string(say->args[i]) << "\"";
            }
        }

        if (say->end == "\\n")
            out << " << std::endl;\n";
        else
            out << " << \"" << escape_string(say->end) << "\";\n";
    }
    else if (auto set = dynamic_cast<SetStatement*>(stmt.get())) {
        TypeChecker type_checker;
        
        std::string cpp_type = "auto";
        if (!set->type_annotation.empty()) {
            auto type_info = type_checker.parse_type_annotation(set->type_annotation);
            if (type_info.has_value()) {
                cpp_type = type_info->to_cpp_type();
            }
        }
        
        std::string default_value = "0";
        if (!set->initial_value.empty()) {
            if (set->type_annotation.find("text") != std::string::npos) {
                default_value = "\"" + escape_string(set->initial_value) + "\"";
            } else if (set->type_annotation.find("number") != std::string::npos) {
                default_value = set->initial_value; // No quotes for numbers
            } else if (set->type_annotation.find("truth") != std::string::npos) {
                default_value = set->initial_value; // true/false literals
            } else {
                // Try to infer from value
                auto inferred_type = type_checker.infer_literal_type(set->initial_value);
                if (inferred_type.base_type == HerType::Number) {
                    default_value = set->initial_value;
                } else if (inferred_type.base_type == HerType::Truth) {
                    default_value = set->initial_value;
                } else {
                    default_value = "\"" + escape_string(set->initial_value) + "\"";
                }
            }
        } else if (!set->type_annotation.empty()) {
            // Set default values based on type
            if (set->type_annotation.find("text") != std::string::npos) {
                default_value = "\"\"";
            } else if (set->type_annotation.find("number") != std::string::npos) {
                default_value = "0";
            } else if (set->type_annotation.find("truth") != std::string::npos) {
                default_value = "false";
            }
            if (set->type_annotation.find("?") != std::string::npos) {
                default_value = "std::nullopt"; // For nullable types
            }
        }
        
        out << ind << cpp_type << " " << set->var << " = " << default_value << ";\n";
    }
    else if (auto func = dynamic_cast<FunctionDef*>(stmt.get())) {
        if (!func->param.empty()) {
            out << "void " << func->name << "(auto " << func->param << ") {\n";
        }
        else {
            out << "void " << func->name << "() {\n";
        }

        for (auto& s : func->body) gen_stmt(out, s, indent_level + 1);
        out << "}\n";
    }
    else if (auto call = dynamic_cast<FunctionCall*>(stmt.get())) {
        out << ind << call->name << "(";
#if _DEBUG
        std::cerr << "[DEBUG] function call arg in gen " << escape_string(call->arg) << " ";
        switch (call->arg_type) {
        case TokenType::Keyword:        std::cerr << "Keyword    "; break;
        case TokenType::Identifier:     std::cerr << "Identifier "; break;
        case TokenType::StringLiteral:  std::cerr << "String     "; break;
        case TokenType::Newline:        std::cerr << "Newline    "; break;
        case TokenType::EOFToken:       std::cerr << "EOF        "; break;
        case TokenType::Symbol:         std::cerr << "Symbol     "; break;
        }
        std::cerr << std::endl;
#endif
        if (!call->arg.empty()) {
            if (call->arg_type == TokenType::StringLiteral) {
                out << "\"" << escape_string(call->arg) << "\"";
            }
            else {
                out << call->arg;
            }
        }
        out << ");\n";
    }
    else if (auto main = dynamic_cast<StartBlock*>(stmt.get())) {
        out << "int main() {\n#ifdef _WIN32\nSetConsoleOutputCP(CP_UTF8);\n#endif\n\n";
        for (auto& s : main->body) gen_stmt(out, s, indent_level + 1);
        out << indent(indent_level + 1) << "return 0;\n";
        out << "}\n";
    }
}

std::string generate_cpp(const AST& ast) {
    std::ostringstream out;
    out << "#include <iostream>\n#include <string>\n#include <optional>\n\n#ifdef _WIN32\n#include <windows.h>\n#endif\n\n";

    for (auto& stmt : ast.statements) {
        if (dynamic_cast<FunctionDef*>(stmt.get())) {
            gen_stmt(out, stmt, 0);
            out << '\n';
        }
    }

    for (auto& stmt : ast.statements) {
        if (dynamic_cast<StartBlock*>(stmt.get())) {
            gen_stmt(out, stmt, 0);
            out << '\n';
        }
    }

    return out.str();
}
