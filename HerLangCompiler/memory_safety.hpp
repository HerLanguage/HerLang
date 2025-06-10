// memory_safety.hpp - Memory safety utilities for HerLang
#pragma once
#include <vector>
#include <memory>
#include <stdexcept>
#include "error_system.hpp"

template<typename T>
class SafeArray {
private:
    std::vector<T> data;
    std::string name;

public:
    SafeArray(const std::string& array_name = "array") : name(array_name) {}
    
    SafeArray(size_t size, const std::string& array_name = "array") 
        : data(size), name(array_name) {}
    
    SafeArray(std::initializer_list<T> init, const std::string& array_name = "array")
        : data(init), name(array_name) {}

    T& at(size_t index) {
        if (index >= data.size()) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Array index out of bounds for '" + name + "'")
                .with_suggestion("Index " + std::to_string(index) + 
                    " is >= array size " + std::to_string(data.size()))
                .with_suggestion("Use array.size() to check bounds before accessing")
                .with_context("Safe array bounds checking");
        }
        return data[index];
    }

    const T& at(size_t index) const {
        if (index >= data.size()) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Array index out of bounds for '" + name + "'")
                .with_suggestion("Index " + std::to_string(index) + 
                    " is >= array size " + std::to_string(data.size()))
                .with_suggestion("Use array.size() to check bounds before accessing")
                .with_context("Safe array bounds checking");
        }
        return data[index];
    }

    // Safe subscript operator
    T& operator[](size_t index) { return at(index); }
    const T& operator[](size_t index) const { return at(index); }

    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
    
    void push_back(const T& value) { data.push_back(value); }
    void push_back(T&& value) { data.push_back(std::move(value)); }
    
    typename std::vector<T>::iterator begin() { return data.begin(); }
    typename std::vector<T>::iterator end() { return data.end(); }
    typename std::vector<T>::const_iterator begin() const { return data.begin(); }
    typename std::vector<T>::const_iterator end() const { return data.end(); }
};

template<typename T>
using SafePtr = std::shared_ptr<T>;

template<typename T, typename... Args>
SafePtr<T> make_safe(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

class MemoryManager {
public:
    static constexpr size_t MAX_SAFE_ALLOCATION = 1024 * 1024 * 1024; // 1GB
    
    template<typename T>
    static SafeArray<T> create_array(size_t size, const std::string& name = "array") {
        if (size * sizeof(T) > MAX_SAFE_ALLOCATION) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Requested array size exceeds safety limit")
                .with_suggestion("Reduce array size or process data in chunks")
                .with_suggestion("Consider using streaming or iterator patterns")
                .with_context("Memory allocation safety check");
        }
        return SafeArray<T>(size, name);
    }
    
    static void validate_pointer(const void* ptr, const std::string& context = "") {
        if (!ptr) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Null pointer access detected")
                .with_suggestion("Check if the pointer was properly initialized")
                .with_suggestion("Verify the object was not prematurely destroyed")
                .with_context(context.empty() ? "Null pointer validation" : context);
        }
    }
};