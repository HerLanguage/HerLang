// herlang_advanced.hpp - Advanced features integration for HerLang
#pragma once
#include "advanced_memory.hpp"
#include "cooperative_threading.hpp"
#include "shared_state.hpp"
#include "performance_optimization.hpp"
#include "error_system.hpp"
#include <memory>
#include <future>

namespace HerLang {

// Main HerLang runtime system integrating all advanced features
class HerLangRuntime {
private:
    std::unique_ptr<AdvancedMemoryManager> memory_manager;
    std::unique_ptr<GentleGarbageCollector> gc;
    std::unique_ptr<CooperativeThreadPool> thread_pool;
    std::unique_ptr<DeadlockPrevention> deadlock_prevention;
    
    static HerLangRuntime* instance;
    static std::mutex instance_mutex;
    
    HerLangRuntime() {
        memory_manager = std::make_unique<AdvancedMemoryManager>();
        gc = std::make_unique<GentleGarbageCollector>(*memory_manager);
        thread_pool = std::make_unique<CooperativeThreadPool>();
        deadlock_prevention = std::make_unique<DeadlockPrevention>();
    }
    
public:
    static HerLangRuntime& get_instance() {
        std::lock_guard<std::mutex> lock(instance_mutex);
        if (!instance) {
            instance = new HerLangRuntime();
        }
        return *instance;
    }
    
    // Memory management interface
    template<typename T>
    BoundaryGuardedPtr<T> allocate_safe_array(size_t count, const std::string& context = "") {
        return memory_manager->allocate<T>(count, context);
    }
    
    void deallocate_safe_memory(void* ptr) {
        memory_manager->deallocate(ptr);
    }
    
    AdvancedMemoryManager::MemoryStats get_memory_stats() const {
        return memory_manager->get_stats();
    }
    
    // Threading interface
    template<typename F, typename... Args>
    auto submit_async_task(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        return thread_pool->submit_with_care(std::forward<F>(f), std::forward<Args>(args)...);
    }
    
    CooperativeThreadPool::PoolStats get_thread_pool_stats() const {
        return thread_pool->get_pool_stats();
    }
    
    void ensure_worker_wellness() {
        thread_pool->ensure_worker_wellness();
    }
    
    // Deadlock prevention interface
    bool can_acquire_lock_safely(const std::string& lock_name) {
        return deadlock_prevention->can_acquire_safely(lock_name);
    }
    
    void register_lock_acquisition(const std::string& lock_name) {
        deadlock_prevention->register_lock_acquisition(lock_name);
    }
    
    void register_lock_release(const std::string& lock_name) {
        deadlock_prevention->register_lock_release(lock_name);
    }
    
    DeadlockPrevention::DeadlockReport analyze_deadlocks() {
        return deadlock_prevention->analyze_potential_deadlocks();
    }
    
    // System health monitoring
    struct SystemHealthReport {
        AdvancedMemoryManager::MemoryStats memory;
        CooperativeThreadPool::PoolStats threading;
        DeadlockPrevention::DeadlockReport deadlock;
        PerformanceAnalyzer::PerformanceReport performance;
        std::vector<std::string> health_recommendations;
    };
    
    SystemHealthReport get_system_health() {
        SystemHealthReport report;
        report.memory = get_memory_stats();
        report.threading = get_thread_pool_stats();
        report.deadlock = analyze_deadlocks();
        report.performance = PerformanceAnalyzer::generate_report();
        
        // Generate health recommendations
        if (report.memory.total_allocated > 500 * 1024 * 1024) { // > 500MB
            report.health_recommendations.push_back("High memory usage detected - consider memory optimization");
        }
        
        if (report.threading.average_stress > 0.7f) {
            report.health_recommendations.push_back("Thread pool stress is high - reduce task submission rate");
        }
        
        if (report.deadlock.potential_deadlock_detected) {
            report.health_recommendations.push_back("Potential deadlock detected - review lock acquisition order");
        }
        
        if (report.performance.cache_misses > 1000) {
            report.health_recommendations.push_back("High cache miss rate - optimize data layout and access patterns");
        }
        
        return report;
    }
    
    // Graceful shutdown
    void shutdown() {
        if (thread_pool) {
            thread_pool.reset();
        }
        if (gc) {
            gc.reset();
        }
        if (memory_manager) {
            memory_manager.reset();
        }
    }
    
    ~HerLangRuntime() {
        shutdown();
    }
};

// Global convenience functions
template<typename T>
BoundaryGuardedPtr<T> safe_allocate(size_t count, const std::string& context = "") {
    return HerLangRuntime::get_instance().allocate_safe_array<T>(count, context);
}

template<typename F, typename... Args>
auto async_with_care(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
    return HerLangRuntime::get_instance().submit_async_task(std::forward<F>(f), std::forward<Args>(args)...);
}

template<typename T>
using SafeSharedState = ProtectedSharedState<T>;

// Safe lock acquisition macro
#define SAFE_LOCK(mutex, name) SafeLockGuard<std::decay_t<decltype(mutex)>> lock_##__LINE__(mutex, name)

// Performance-optimized vector operations
template<typename T>
void safe_vector_add(const T* a, const T* b, T* result, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        if (SIMDOperations::has_avx2_support()) {
            SIMDOperations::add_vectors_f32(a, b, result, count);
            PerformanceAnalyzer::record_simd_operation();
            return;
        }
    }
    
    // Fallback to scalar operation
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
    PerformanceAnalyzer::record_scalar_operation();
}

template<typename T>
T safe_dot_product(const T* a, const T* b, size_t count) {
    if constexpr (std::is_same_v<T, float>) {
        if (SIMDOperations::has_avx2_support()) {
            PerformanceAnalyzer::record_simd_operation();
            return SIMDOperations::dot_product_f32(a, b, count);
        }
    }
    
    // Fallback to scalar operation
    T result = T{};
    for (size_t i = 0; i < count; ++i) {
        result += a[i] * b[i];
    }
    PerformanceAnalyzer::record_scalar_operation();
    return result;
}

// Static initialization
HerLangRuntime* HerLangRuntime::instance = nullptr;
std::mutex HerLangRuntime::instance_mutex;

} // namespace HerLang

// Convenience macros for common operations
#define HERLANG_LIKELY(x) LIKELY(x)
#define HERLANG_UNLIKELY(x) UNLIKELY(x)

#define HERLANG_PREFETCH_READ(addr) HerLang::PrefetchHints::prefetch_read(addr)
#define HERLANG_PREFETCH_WRITE(addr) HerLang::PrefetchHints::prefetch_write(addr)

#define HERLANG_CACHE_FRIENDLY_VECTOR(T) HerLang::CacheFriendlyVector<T>
#define HERLANG_SAFE_ARRAY(T) HerLang::BoundaryGuardedPtr<T>
#define HERLANG_SHARED_STATE(T) HerLang::ProtectedSharedState<T>