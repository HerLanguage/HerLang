// lexer.cpp - MyLang lexer implementation
#include "lexer.hpp"
#include "utils.hpp"
#include "error_system.hpp"
#include <regex>
#include <sstream>
#include <cctype>

std::vector<Token> lex(const std::vector<std::string>& lines) {
    std::vector<Token> tokens;
    std::regex string_regex("\"(.*?)\"");

    for (int i = 0; i < lines.size(); ++i) {
        std::string line = trim(lines[i]);
        if (line.empty() || line[0] == '#') continue;

        size_t j = 0;
        while (j < line.size()) {
            if (std::isspace(line[j])) {
                ++j;
                continue;
            }

            if (line[j] == '"') {
                // Parse string literal
                size_t end = line.find('"', j + 1);
                if (end == std::string::npos) {
                    throw HerLangError(HerLangError::Type::SyntaxError,
                        "Unterminated string literal", i + 1)
                        .with_suggestion("Add closing quote (\") to complete the string")
                        .with_suggestion("Check for escaped quotes if needed")
                        .with_context("String started but never closed");
                }
                std::string str = line.substr(j + 1, end - j - 1);
                tokens.push_back({ TokenType::StringLiteral, str, i + 1 });
                j = end + 1;
            }
            else if (std::isalpha(line[j]) || line[j] == '_') {
                // Identifier or keyword
                size_t start = j;
                while (j < line.size() && (std::isalnum(line[j]) || line[j] == '_')) ++j;
                std::string word = line.substr(start, j - start);

                if (word == "function" || word == "start" || word == "end" ||
                    word == "if" || word == "elif" || word == "else" ||
                    word == "say" || word == "set" || word == "var" ||
                    word == "add" || word == "minus" ||
                    word == "multiply" || word == "divide" ||
                    word == "text" || word == "number" || word == "truth" ||
                    word == "maybe" || word == "nothing") {
                    tokens.push_back({ TokenType::Keyword, word, i + 1 });
                }
                else {
                    tokens.push_back({ TokenType::Identifier, word, i + 1 });
                }
            }
            else if (line[j] == ':' || line[j] == '=' || line[j] == '(' || line[j] == ')' || line[j] == '?') {
                // Symbols
                tokens.push_back({ TokenType::Symbol, std::string(1, line[j]), i + 1 });
                ++j;
            }
            else {
                // Unexpected character, skip or report
                ++j;
            }
        }

        tokens.push_back({ TokenType::Newline, "\\n", i + 1 });
    }

    tokens.push_back({ TokenType::EOFToken, "", (int)lines.size() });
    return tokens;
}
