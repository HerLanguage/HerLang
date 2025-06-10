// error_system.hpp - Enhanced error handling for HerLang
#pragma once
#include <string>
#include <vector>
#include <exception>

class HerLangError : public std::exception {
public:
    enum class Type {
        SyntaxError,
        TypeError,
        MemoryError,
        RuntimeError,
        UnexpectedToken,
        UndefinedFunction,
        InvalidArgument
    };

private:
    Type error_type;
    std::string message;
    std::string context;
    int line_number;
    std::vector<std::string> suggestions;
    std::string help_url;

public:
    HerLangError(Type type, const std::string& msg, int line = -1);
    
    HerLangError& with_context(const std::string& ctx);
    HerLangError& with_suggestion(const std::string& suggestion);
    HerLangError& with_help_url(const std::string& url);
    
    void display_friendly_error() const;
    const char* what() const noexcept override;
    
    static std::string get_encouragement(Type error_type);
    static std::vector<std::string> get_common_solutions(Type error_type);
};

class ErrorReporter {
public:
    static void report_with_support(const HerLangError& error);
    static void suggest_learning_resources(HerLangError::Type error_type);
    static void offer_community_help();
};