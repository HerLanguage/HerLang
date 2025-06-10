// main.cpp - Entry point for HerLangCompiler
#include "lexer.hpp"
#include "parser.hpp"
#include "generator.hpp"
#include "warnings.hpp"
#include "utils.hpp"
#include "error_system.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: hcp in.herc out.cpp\n";
        return 1;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Cannot open input file: " << argv[1] << "\n";
        return 1;
    }

    std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    input.close();

    try {
        check_indentation(source); // Check for indentation warnings

        auto lines = split_lines(source);
        auto tokens = lex(lines);
#if _DEBUG
    std::cerr << "=== Tokens ===\n";
    for (const auto& tok : tokens) {
        std::cerr << "[" << tok.line << "] ";
        switch (tok.type) {
        case TokenType::Keyword:        std::cerr << "Keyword    "; break;
        case TokenType::Identifier:     std::cerr << "Identifier "; break;
        case TokenType::StringLiteral:  std::cerr << "String     "; break;
        case TokenType::Newline:        std::cerr << "Newline    "; break;
        case TokenType::EOFToken:       std::cerr << "EOF        "; break;
        case TokenType::Symbol:         std::cerr << "Symbol     "; break;
        }
        std::cerr << ": " << tok.value << "\n";
    }
    std::cerr << "==============\n";

#endif
    auto ast = parse(tokens);
#if _DEBUG
    std::cerr << "=== AST ===\n";
    for (const auto& stmt : ast.statements) {
        if (auto func = std::dynamic_pointer_cast<FunctionDef>(stmt)) {
            std::cerr << "Function: " << func->name << "(" << func->param << "), body size = " << func->body.size() << "\n";
            for (auto& inner : func->body) {
                if (auto say = std::dynamic_pointer_cast<SayStatement>(inner)) {
                    std::cerr << "  Say: ";
                    for (size_t i = 0; i < say->args.size(); ++i) {
                        if (say->is_vars[i]) {
                            std::cerr << "VAR(" << say->args[i] << ") ";
                        }
                        else {
                            std::cerr << "\"" << say->args[i] << "\" ";
                        }
                    }
                    std::cerr << "ending = \"" << say->end << "\"\n";
                }
            }
        }
        else if (auto start = std::dynamic_pointer_cast<StartBlock>(stmt)) {
            std::cerr << "Start block, body size = " << start->body.size() << "\n";
        }
    }
    std::cerr << "===========\n";
#endif
    auto cpp_code = generate_cpp(ast);

    std::ofstream output(argv[2]);
    if (!output) {
        std::cerr << "Cannot write to output file: " << argv[2] << "\n";
        return 1;
    }
        output << cpp_code;
        output.close();

        std::cout << "🌸 Compilation successful: " << argv[2] << "\n";
        std::cout << "💝 Your HerLang code has been transformed into C++!\n";
        return 0;
    }
    catch (const HerLangError& e) {
        ErrorReporter::report_with_support(e);
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "🌸 Unexpected error: " << e.what() << "\n";
        std::cerr << "💝 Please report this issue to help improve HerLang!\n";
        return 1;
    }
}
