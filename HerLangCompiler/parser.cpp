// parser.cpp - MyLang parser implementation
#include "parser.hpp"
#include "utils.hpp"
#include "error_system.hpp"
#include <stdexcept>
#include <iostream>

static int pos = 0;
static std::vector<Token> toks;

static Token dummy_eof_token() {
    return Token{ TokenType::EOFToken, "" };
}

static Token peek() {
    if (pos >= toks.size()) {
#if _DEBUG
        std::cerr << "[ERROR] peek: pos=" << pos << ", toks.size=" << toks.size() << "\n";
#endif
        return dummy_eof_token();
    }
    auto t = toks[pos];
#if _DEBUG
    std::cerr << "[peek] pos=" << pos << ", token=(" << t.value << ")\n";
#endif
    return t;
}

static Token advance() {
    if (pos >= toks.size()) {
#if _DEBUG
        std::cerr << "[ERROR] advance: pos=" << pos << ", toks.size=" << toks.size() << "\n";
#endif
        return dummy_eof_token();
    }
    auto t = toks[pos++];
#if _DEBUG
    std::cerr << "[advance] pos=" << pos << ", token=(" << t.value << ")\n";
#endif
    return t;
}

static void skip_newlines() {
    while (peek().type == TokenType::Newline) {
        advance();
    }
}

std::shared_ptr<Statement> parse_statement();

AST parse(const std::vector<Token>& tokens) {
    toks = tokens;
    pos = 0;
    AST ast;

    while (pos < toks.size()) {
        Token current = peek();
        if (current.type == TokenType::EOFToken) break;

        auto stmt = parse_statement();
        if (stmt) {
            ast.statements.push_back(stmt);
        }
        else {
            advance();
        }
    }

    return ast;
}

std::vector<std::shared_ptr<Statement>> parse_block() {
    std::vector<std::shared_ptr<Statement>> body;
    int safety_counter = 0;

    while (true) {
        skip_newlines();

        Token current = peek();
        if (current.type == TokenType::Keyword && current.value == "end") {
            advance(); // consume "end"
            break;
        }
        if (current.type == TokenType::EOFToken) {
            throw HerLangError(HerLangError::Type::SyntaxError, 
                "Unexpected end of file inside block", current.line)
                .with_suggestion("Add 'end' statement to close the block")
                .with_context("Looking for block terminator");
        }

        auto stmt = parse_statement();
        if (stmt) {
            body.push_back(stmt);
        }
        else {
            advance();
        }

        if (++safety_counter > 10000) {
            throw HerLangError(HerLangError::Type::SyntaxError,
                "Block parsing exceeded safety limit - possible infinite loop")
                .with_suggestion("Check for missing 'end' statement")
                .with_suggestion("Verify proper block structure")
                .with_context("Safety limit prevents infinite parsing");
        }
    }

    return body;
}

std::shared_ptr<Statement> parse_statement() {
    skip_newlines();

    Token tok = peek();

    if (tok.type == TokenType::EOFToken) {
        return nullptr;
    }

    // function definition
    if (tok.type == TokenType::Keyword && tok.value == "function") {
        advance(); // consume 'function'

        Token name = advance();
        Token maybe_param_or_colon = advance();

        std::string param = "";

        Token colon;
        if (maybe_param_or_colon.type == TokenType::Symbol && maybe_param_or_colon.value == ":") {
            colon = maybe_param_or_colon;
        }
        else {
            param = maybe_param_or_colon.value;
            colon = advance();
            if (colon.value != ":") {
                throw HerLangError(HerLangError::Type::SyntaxError,
                    "Expected ':' after parameter in function definition", colon.line)
                    .with_suggestion("Add a colon (:) after the parameter name")
                    .with_context("Parsing function definition");
            }
        }

        auto body = parse_block();
        return std::make_shared<FunctionDef>(name.value, param, body);
    }

    // start block
    if (tok.type == TokenType::Keyword && tok.value == "start") {
        advance();
        Token colon = advance();
        if (colon.value != ":") {
            throw HerLangError(HerLangError::Type::SyntaxError,
                "Expected ':' after start", colon.line)
                .with_suggestion("Add a colon (:) after 'start'")
                .with_context("Parsing start block");
        }
        auto body = parse_block();
        return std::make_shared<StartBlock>(body);
    }

    // say
    if (tok.type == TokenType::Keyword && tok.value == "say") {
        advance(); // consume 'say'

        std::vector<std::string> args;
        std::vector<bool> is_vars;
        std::string ending = "\\n"; // default end

        while (true) {
            Token next = peek();
#if _DEBUG
            std::cerr << "[DEBUG] say loop: next=" << next.value << ", type=" << static_cast<int>(next.type) << "\n";
#endif

            if (next.type == TokenType::Keyword && next.value == "end") {
                advance(); // consume 'end'

                Token eq = peek();
                if (eq.type != TokenType::Symbol || eq.value != "=") {
                    throw HerLangError(HerLangError::Type::SyntaxError,
                        "Expected '=' after 'end'", eq.line)
                        .with_suggestion("Use 'end = \"string\"' to specify output ending")
                        .with_context("Parsing say statement with custom ending");
                }
                advance(); // consume '='

                Token val = peek();
                if (val.type != TokenType::StringLiteral) {
                    throw HerLangError(HerLangError::Type::TypeError,
                        "Expected string literal after end=", val.line)
                        .with_suggestion("Use quotes around the ending string: end = \"your_ending\"")
                        .with_context("Setting custom output ending");
                }
                ending = advance().value;

                break;
            }

            if (next.type == TokenType::Newline || next.type == TokenType::EOFToken) {
                advance(); // consume newline/EOF
                break;
            }

            
            if (next.type == TokenType::StringLiteral || next.type == TokenType::Identifier) {
                Token arg = advance();
                args.push_back(arg.value);
                is_vars.push_back(arg.type == TokenType::Identifier);

                
                Token comma = peek();
                if (comma.type == TokenType::Symbol && comma.value == ",") {
                    advance(); // consume comma
                }
            }
            else {
                throw HerLangError(HerLangError::Type::UnexpectedToken,
                    "Unexpected token in 'say': " + next.value, next.line)
                    .with_suggestion("Use string literals in quotes or variable names")
                    .with_suggestion("Separate multiple arguments with commas")
                    .with_context("Parsing say statement arguments");
            }
        }

        if (args.size() != is_vars.size()) {
            throw HerLangError(HerLangError::Type::RuntimeError,
                "Internal parser error: argument type mismatch")
                .with_suggestion("This is a compiler bug - please report it")
                .with_context("Internal consistency check failed");
        }

        return std::make_shared<SayStatement>(args, is_vars, ending);
    }

    // set or var
    if ((tok.type == TokenType::Keyword && tok.value == "set") || 
        (tok.type == TokenType::Keyword && tok.value == "var")) {
        advance(); // consume 'set' or 'var'
        Token var = advance();
        
        std::string type_annotation = "";
        std::string initial_value = "";
        
        // Check for type annotation: var name: type
        Token next = peek();
        if (next.type == TokenType::Symbol && next.value == ":") {
            advance(); // consume ':'
            Token type_token = peek();
            if (type_token.type == TokenType::Keyword) {
                type_annotation = advance().value;
                
                // Check for nullable type: type?
                Token nullable = peek();
                if (nullable.type == TokenType::Symbol && nullable.value == "?") {
                    advance(); // consume '?'
                    type_annotation += "?";
                }
            }
        }
        
        // Check for initialization: = value
        next = peek();
        if (next.type == TokenType::Symbol && next.value == "=") {
            advance(); // consume '='
            Token value_token = advance();
            initial_value = value_token.value;
        }
        
        return std::make_shared<SetStatement>(var.value, type_annotation, initial_value);
    }

    // function call
    if (tok.type == TokenType::Identifier) {
        Token func = advance();
        Token next = peek();
        if (next.type == TokenType::StringLiteral || next.type == TokenType::Identifier) {
            Token arg = advance();
#if _DEBUG
            std::cerr << "[DEBUG] function call arg " << arg.value << " ";
            switch (arg.type) {
            case TokenType::Keyword:        std::cerr << "Keyword    "; break;
            case TokenType::Identifier:     std::cerr << "Identifier "; break;
            case TokenType::StringLiteral:  std::cerr << "String     "; break;
            case TokenType::Newline:        std::cerr << "Newline    "; break;
            case TokenType::EOFToken:       std::cerr << "EOF        "; break;
            case TokenType::Symbol:         std::cerr << "Symbol     "; break;
            }
            std::cerr << std::endl;
#endif
            return std::make_shared<FunctionCall>(func.value, arg.value, arg.type);
        }
        else {
            return std::make_shared<FunctionCall>(func.value, "", TokenType::EOFToken);
        }
    }


    advance();
    return nullptr;
}
