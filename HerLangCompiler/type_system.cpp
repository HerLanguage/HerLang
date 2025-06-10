// type_system.cpp - Type system implementation
#include "type_system.hpp"
#include <regex>
#include <cctype>
#include <algorithm>

void TypeChecker::declare_variable(const std::string& name, const TypeInfo& type) {
    variable_types[name] = type;
}

TypeInfo TypeChecker::get_variable_type(const std::string& name) const {
    auto it = variable_types.find(name);
    return it != variable_types.end() ? it->second : TypeInfo(HerType::Unknown);
}

void TypeChecker::declare_function(const std::string& name, const TypeInfo& return_type) {
    function_return_types[name] = return_type;
}

TypeInfo TypeChecker::get_function_return_type(const std::string& name) const {
    auto it = function_return_types.find(name);
    return it != function_return_types.end() ? it->second : TypeInfo(HerType::Nothing);
}

bool TypeChecker::is_compatible(const TypeInfo& expected, const TypeInfo& actual) const {
    // If expected type is unknown, accept anything (gradual typing)
    if (expected.base_type == HerType::Unknown) return true;
    
    // If actual type is unknown, accept for now (inference will happen later)
    if (actual.base_type == HerType::Unknown) return true;
    
    // Exact type match
    if (expected.base_type == actual.base_type) {
        // Nullable can accept non-nullable
        return !expected.is_nullable || actual.is_nullable;
    }
    
    // Special compatibility rules
    // Numbers can be converted to text
    if (expected.base_type == HerType::Text && actual.base_type == HerType::Number) {
        return true;
    }
    
    return false;
}

TypeInfo TypeChecker::infer_literal_type(const std::string& literal) const {
    // Check if it's a number
    std::regex number_regex(R"(^-?\d+(\.\d+)?$)");
    if (std::regex_match(literal, number_regex)) {
        return TypeInfo(HerType::Number);
    }
    
    // Check if it's a boolean
    if (literal == "true" || literal == "false") {
        return TypeInfo(HerType::Truth);
    }
    
    // Default to text (string literals)
    return TypeInfo(HerType::Text);
}

void TypeChecker::check_assignment_compatibility(const std::string& var_name, 
                                                const TypeInfo& value_type, 
                                                int line) const {
    auto var_type = get_variable_type(var_name);
    
    if (!is_compatible(var_type, value_type)) {
        throw HerLangError(HerLangError::Type::TypeError,
            "Type mismatch: cannot assign " + value_type.type_name + 
            " to variable '" + var_name + "' of type " + var_type.type_name, line)
            .with_suggestion("Check the types are compatible")
            .with_suggestion("Consider explicit type conversion if needed")
            .with_context("Type checking assignment");
    }
}

std::optional<TypeInfo> TypeChecker::parse_type_annotation(const std::string& annotation) const {
    if (annotation.empty()) return std::nullopt;
    
    std::string clean_annotation = annotation;
    bool is_nullable = false;
    
    // Check for nullable marker
    if (!clean_annotation.empty() && clean_annotation.back() == '?') {
        is_nullable = true;
        clean_annotation.pop_back();
    }
    
    // Convert to lowercase for case-insensitive matching
    std::transform(clean_annotation.begin(), clean_annotation.end(), 
                   clean_annotation.begin(), ::tolower);
    
    HerType base_type = HerType::Unknown;
    if (clean_annotation == "text" || clean_annotation == "string") {
        base_type = HerType::Text;
    } else if (clean_annotation == "number" || clean_annotation == "num") {
        base_type = HerType::Number;
    } else if (clean_annotation == "truth" || clean_annotation == "bool") {
        base_type = HerType::Truth;
    } else if (clean_annotation == "nothing" || clean_annotation == "void") {
        base_type = HerType::Nothing;
    } else {
        return std::nullopt; // Unknown type annotation
    }
    
    return TypeInfo(base_type, is_nullable);
}