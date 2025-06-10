// type_system.hpp - Type system for HerLang
#pragma once
#include <string>
#include <optional>
#include <map>
#include <memory>
#include "error_system.hpp"

enum class HerType {
    Unknown,
    Text,      // string
    Number,    // int/float
    Truth,     // bool
    Maybe,     // optional/nullable type
    Nothing    // void
};

struct TypeInfo {
    HerType base_type;
    bool is_nullable;
    std::string type_name;
    
    TypeInfo(HerType type = HerType::Unknown, bool nullable = false) 
        : base_type(type), is_nullable(nullable) {
        switch (type) {
            case HerType::Text: type_name = "text"; break;
            case HerType::Number: type_name = "number"; break;
            case HerType::Truth: type_name = "truth"; break;
            case HerType::Maybe: type_name = "maybe"; break;
            case HerType::Nothing: type_name = "nothing"; break;
            default: type_name = "unknown"; break;
        }
        if (nullable) type_name += "?";
    }
    
    std::string to_cpp_type() const {
        std::string cpp_type;
        switch (base_type) {
            case HerType::Text: cpp_type = "std::string"; break;
            case HerType::Number: cpp_type = "double"; break;
            case HerType::Truth: cpp_type = "bool"; break;
            case HerType::Nothing: cpp_type = "void"; break;
            default: cpp_type = "auto"; break;
        }
        return is_nullable ? "std::optional<" + cpp_type + ">" : cpp_type;
    }
};

class TypeChecker {
private:
    std::map<std::string, TypeInfo> variable_types;
    std::map<std::string, TypeInfo> function_return_types;
    
public:
    void declare_variable(const std::string& name, const TypeInfo& type);
    TypeInfo get_variable_type(const std::string& name) const;
    
    void declare_function(const std::string& name, const TypeInfo& return_type);
    TypeInfo get_function_return_type(const std::string& name) const;
    
    bool is_compatible(const TypeInfo& expected, const TypeInfo& actual) const;
    
    TypeInfo infer_literal_type(const std::string& literal) const;
    
    void check_assignment_compatibility(const std::string& var_name, 
                                      const TypeInfo& value_type, 
                                      int line) const;
                                      
    std::optional<TypeInfo> parse_type_annotation(const std::string& annotation) const;
};