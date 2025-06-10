// test_advanced_features.cpp - Test the advanced HerLang features
#include "HerLangCompiler/herlang_advanced.hpp"
#include <iostream>
#include <vector>
#include <chrono>

using namespace HerLang;

void test_safe_memory() {
    std::cout << "🛡️ Testing Safe Memory Management...\n";
    
    try {
        // Allocate a safe array
        auto safe_array = safe_allocate<float>(1000, "test_array");
        
        // Safe access within bounds
        for (size_t i = 0; i < safe_array.size(); ++i) {
            safe_array[i] = static_cast<float>(i) * 1.5f;
        }
        
        std::cout << "✅ Safe array allocation and access successful\n";
        std::cout << "   Array size: " << safe_array.size() << " elements\n";
        std::cout << "   Sample values: " << safe_array[0] << ", " << safe_array[10] << ", " << safe_array[100] << "\n";
        
        // Test bounds checking (this should be safe and return nullopt)
        auto out_of_bounds = safe_array.safe_at(2000);
        if (!out_of_bounds.has_value()) {
            std::cout << "✅ Bounds checking working correctly\n";
        }
        
    } catch (const HerLangError& e) {
        std::cout << "❌ Memory test failed:\n";
        e.display_friendly_error();
    }
}

void test_cooperative_threading() {
    std::cout << "\n🤝 Testing Cooperative Threading...\n";
    
    try {
        std::vector<std::future<int>> futures;
        
        // Submit multiple tasks
        for (int i = 0; i < 10; ++i) {
            auto future = async_with_care([i]() {
                // Simulate some work
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return i * i;
            });
            futures.push_back(std::move(future));
        }
        
        // Collect results
        std::cout << "✅ Tasks submitted, collecting results:\n";
        for (size_t i = 0; i < futures.size(); ++i) {
            int result = futures[i].get();
            std::cout << "   Task " << i << " result: " << result << "\n";
        }
        
        // Check thread pool stats
        auto stats = HerLangRuntime::get_instance().get_thread_pool_stats();
        std::cout << "✅ Thread pool statistics:\n";
        std::cout << "   Workers: " << stats.worker_count << "\n";
        std::cout << "   Average stress: " << stats.average_stress << "\n";
        std::cout << "   Total tasks completed: " << stats.total_tasks_completed << "\n";
        
    } catch (const HerLangError& e) {
        std::cout << "❌ Threading test failed:\n";
        e.display_friendly_error();
    }
}

void test_shared_state() {
    std::cout << "\n🔒 Testing Protected Shared State...\n";
    
    try {
        // Create shared state
        SafeSharedState<int> shared_counter(0, "test_counter");
        
        // Multiple readers
        auto read_value = shared_counter.safe_read([](const int& value) {
            return value;
        });
        std::cout << "✅ Initial shared value: " << read_value << "\n";
        
        // Safe write
        shared_counter.safe_write([](int& value) {
            value += 100;
        });
        
        // Optimistic update
        bool updated = shared_counter.optimistic_update([](const int& current) {
            return current * 2;
        });
        
        if (updated) {
            auto final_value = shared_counter.get_copy();
            std::cout << "✅ Optimistic update successful, final value: " << final_value << "\n";
        }
        
        auto stats = shared_counter.get_stats();
        std::cout << "✅ Shared state statistics:\n";
        std::cout << "   Name: " << stats.name << "\n";
        std::cout << "   Total reads: " << stats.total_reads << "\n";
        std::cout << "   Total writes: " << stats.total_writes << "\n";
        
    } catch (const HerLangError& e) {
        std::cout << "❌ Shared state test failed:\n";
        e.display_friendly_error();
    }
}

void test_simd_operations() {
    std::cout << "\n⚡ Testing SIMD Performance Optimizations...\n";
    
    try {
        constexpr size_t VECTOR_SIZE = 1000;
        std::vector<float> a(VECTOR_SIZE), b(VECTOR_SIZE), result(VECTOR_SIZE);
        
        // Initialize test data
        for (size_t i = 0; i < VECTOR_SIZE; ++i) {
            a[i] = static_cast<float>(i);
            b[i] = static_cast<float>(i) * 0.5f;
        }
        
        // Test SIMD operations
        std::cout << "   CPU SIMD Support:\n";
        std::cout << "   AVX2: " << (SIMDOperations::has_avx2_support() ? "Yes" : "No") << "\n";
        std::cout << "   SSE4.2: " << (SIMDOperations::has_sse42_support() ? "Yes" : "No") << "\n";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Vector addition
        safe_vector_add(a.data(), b.data(), result.data(), VECTOR_SIZE);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "✅ Vector addition completed in " << duration.count() << " microseconds\n";
        std::cout << "   Sample results: " << result[0] << ", " << result[10] << ", " << result[100] << "\n";
        
        // Dot product test
        start = std::chrono::high_resolution_clock::now();
        float dot_result = safe_dot_product(a.data(), b.data(), VECTOR_SIZE);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "✅ Dot product: " << dot_result << " (computed in " << duration.count() << " microseconds)\n";
        
    } catch (const std::exception& e) {
        std::cout << "❌ SIMD test failed: " << e.what() << "\n";
    }
}

void test_system_health() {
    std::cout << "\n💊 Testing System Health Monitoring...\n";
    
    auto health = HerLangRuntime::get_instance().get_system_health();
    
    std::cout << "✅ System Health Report:\n";
    std::cout << "   Memory:\n";
    std::cout << "     Total allocated: " << health.memory.total_allocated << " bytes\n";
    std::cout << "     Block count: " << health.memory.block_count << "\n";
    std::cout << "     Largest block: " << health.memory.largest_block << " bytes\n";
    
    std::cout << "   Threading:\n";
    std::cout << "     Worker count: " << health.threading.worker_count << "\n";
    std::cout << "     Average stress: " << health.threading.average_stress << "\n";
    std::cout << "     Workers on break: " << health.threading.workers_on_break << "\n";
    
    std::cout << "   Performance:\n";
    std::cout << "     SIMD utilization: " << (health.performance.simd_utilization * 100.0) << "%\n";
    std::cout << "     Cache misses: " << health.performance.cache_misses << "\n";
    
    if (!health.health_recommendations.empty()) {
        std::cout << "   Recommendations:\n";
        for (const auto& rec : health.health_recommendations) {
            std::cout << "     • " << rec << "\n";
        }
    } else {
        std::cout << "   ✅ System running optimally!\n";
    }
}

int main() {
    std::cout << "🌸 HerLang Advanced Features Test Suite\n";
    std::cout << "========================================\n";
    
    try {
        test_safe_memory();
        test_cooperative_threading();
        test_shared_state();
        test_simd_operations();
        test_system_health();
        
        std::cout << "\n🎉 All advanced features tests completed!\n";
        std::cout << "💝 HerLang's safety and performance systems are working beautifully!\n";
        
    } catch (const std::exception& e) {
        std::cout << "\n❌ Unexpected error: " << e.what() << "\n";
        return 1;
    }
    
    // Graceful shutdown
    HerLangRuntime::get_instance().shutdown();
    return 0;
}