// error_system.cpp - Implementation of enhanced error handling
#include "error_system.hpp"
#include <iostream>
#include <map>

HerLangError::HerLangError(Type type, const std::string& msg, int line)
    : error_type(type), message(msg), line_number(line) {
    help_url = "https://github.com/HerLang/docs/issues";
}

HerLangError& HerLangError::with_context(const std::string& ctx) {
    context = ctx;
    return *this;
}

HerLangError& HerLangError::with_suggestion(const std::string& suggestion) {
    suggestions.push_back(suggestion);
    return *this;
}

HerLangError& HerLangError::with_help_url(const std::string& url) {
    help_url = url;
    return *this;
}

std::string HerLangError::get_encouragement(Type error_type) {
    static const std::map<Type, std::string> encouragements = {
        {Type::SyntaxError, "Syntax takes practice - you're learning!"},
        {Type::TypeError, "Type mismatches happen to everyone, let's fix this together."},
        {Type::MemoryError, "Memory management can be tricky, but we can solve this."},
        {Type::RuntimeError, "Runtime issues are great learning opportunities."},
        {Type::UnexpectedToken, "The parser got confused, but we can clarify this."},
        {Type::UndefinedFunction, "Function not found - let's check the definition."},
        {Type::InvalidArgument, "Argument mismatch - let's align the parameters."}
    };
    
    auto it = encouragements.find(error_type);
    return it != encouragements.end() ? it->second : "Every error is a step toward mastery.";
}

std::vector<std::string> HerLangError::get_common_solutions(Type error_type) {
    static const std::map<Type, std::vector<std::string>> solutions = {
        {Type::SyntaxError, {
            "Check for missing colons (:) after function declarations",
            "Ensure 'end' statements match your blocks",
            "Verify proper indentation and spacing"
        }},
        {Type::TypeError, {
            "Check if variables are properly initialized",
            "Verify function parameters match expected types",
            "Consider using explicit type annotations"
        }},
        {Type::UndefinedFunction, {
            "Check if the function is defined before it's called",
            "Verify the function name spelling",
            "Ensure the function is in scope"
        }},
        {Type::UnexpectedToken, {
            "Check for missing quotes around strings",
            "Verify proper punctuation and symbols",
            "Look for unclosed parentheses or brackets"
        }}
    };
    
    auto it = solutions.find(error_type);
    return it != solutions.end() ? it->second : std::vector<std::string>{"Review the documentation and examples"};
}

void HerLangError::display_friendly_error() const {
    std::cerr << "\n🌸 HerLang Error Report\n";
    std::cerr << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    if (line_number > 0) {
        std::cerr << "📍 Line " << line_number << ": ";
    }
    std::cerr << message << "\n\n";
    
    if (!context.empty()) {
        std::cerr << "📝 Context: " << context << "\n\n";
    }
    
    std::cerr << "💝 " << get_encouragement(error_type) << "\n\n";
    
    auto common_solutions = get_common_solutions(error_type);
    if (!suggestions.empty() || !common_solutions.empty()) {
        std::cerr << "🤝 Suggestions:\n";
        
        for (const auto& suggestion : suggestions) {
            std::cerr << "   • " << suggestion << "\n";
        }
        
        for (const auto& solution : common_solutions) {
            std::cerr << "   • " << solution << "\n";
        }
        std::cerr << "\n";
    }
    
    std::cerr << "🌐 Need more help? Visit: " << help_url << "\n";
    std::cerr << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
}

const char* HerLangError::what() const noexcept {
    return message.c_str();
}

void ErrorReporter::report_with_support(const HerLangError& error) {
    error.display_friendly_error();
}

void ErrorReporter::suggest_learning_resources(HerLangError::Type error_type) {
    std::cerr << "📚 Learning Resources:\n";
    std::cerr << "   • HerLang Documentation: https://github.com/HerLanguage/HerLang\n";
    std::cerr << "   • Examples Repository: https://github.com/HerLanguage/HerLang\n";
    std::cerr << "   • Community Forums: https://github.com/Herlanguage/HerLang\n\n";
}

void ErrorReporter::offer_community_help() {
    std::cerr << "👭 Community Support:\n";
    std::cerr << "   • Join our discussion forums for help\n";
    std::cerr << "   • Share your code for collaborative debugging\n";
    std::cerr << "   • Connect with other HerLang developers\n\n";
}