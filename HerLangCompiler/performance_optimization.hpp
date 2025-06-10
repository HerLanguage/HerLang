// performance_optimization.hpp - High-performance optimizations for HerLang
#pragma once
#include <immintrin.h>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include <type_traits>
#include <cstdint>

namespace HerLang {

// Cache-aligned memory allocator
template<std::size_t Alignment = 64>
class CacheAlignedAllocator {
public:
    template<typename T>
    static T* allocate(std::size_t count) {
        std::size_t size = count * sizeof(T);
        void* ptr = std::aligned_alloc(Alignment, (size + Alignment - 1) & ~(Alignment - 1));
        if (!ptr) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(ptr);
    }
    
    template<typename T>
    static void deallocate(T* ptr) {
        std::free(ptr);
    }
};

// Cache-friendly data layout helpers
template<typename T>
class CacheFriendlyVector {
private:
    static constexpr std::size_t CACHE_LINE_SIZE = 64;
    static constexpr std::size_t ELEMENTS_PER_CACHE_LINE = CACHE_LINE_SIZE / sizeof(T);
    
    std::unique_ptr<T[], void(*)(T*)> data;
    std::size_t size_;
    std::size_t capacity_;
    
public:
    explicit CacheFriendlyVector(std::size_t initial_capacity = 16)
        : data(nullptr, [](T* p) { CacheAlignedAllocator<CACHE_LINE_SIZE>::deallocate(p); }),
          size_(0),
          capacity_(((initial_capacity + ELEMENTS_PER_CACHE_LINE - 1) / ELEMENTS_PER_CACHE_LINE) * ELEMENTS_PER_CACHE_LINE) {
        
        data.reset(CacheAlignedAllocator<CACHE_LINE_SIZE>::allocate<T>(capacity_));
    }
    
    void push_back(const T& value) {
        if (size_ >= capacity_) {
            resize_capacity(capacity_ * 2);
        }
        data[size_++] = value;
    }
    
    T& operator[](std::size_t index) { return data[index]; }
    const T& operator[](std::size_t index) const { return data[index]; }
    
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    
    T* begin() { return data.get(); }
    T* end() { return data.get() + size_; }
    const T* begin() const { return data.get(); }
    const T* end() const { return data.get() + size_; }
    
private:
    void resize_capacity(std::size_t new_capacity) {
        new_capacity = ((new_capacity + ELEMENTS_PER_CACHE_LINE - 1) / ELEMENTS_PER_CACHE_LINE) * ELEMENTS_PER_CACHE_LINE;
        
        auto new_data = std::unique_ptr<T[], void(*)(T*)>(
            CacheAlignedAllocator<CACHE_LINE_SIZE>::allocate<T>(new_capacity),
            [](T* p) { CacheAlignedAllocator<CACHE_LINE_SIZE>::deallocate(p); }
        );
        
        if (data) {
            std::memcpy(new_data.get(), data.get(), size_ * sizeof(T));
        }
        
        data = std::move(new_data);
        capacity_ = new_capacity;
    }
};

// SIMD operations for common data types
class SIMDOperations {
public:
    // Vector addition using AVX2
    static void add_vectors_f32(const float* a, const float* b, float* result, std::size_t count) {
        std::size_t simd_count = count & ~7; // Process 8 elements at a time
        
        for (std::size_t i = 0; i < simd_count; i += 8) {
            __m256 va = _mm256_load_ps(&a[i]);
            __m256 vb = _mm256_load_ps(&b[i]);
            __m256 vr = _mm256_add_ps(va, vb);
            _mm256_store_ps(&result[i], vr);
        }
        
        // Handle remaining elements
        for (std::size_t i = simd_count; i < count; ++i) {
            result[i] = a[i] + b[i];
        }
    }
    
    // Vector multiplication using AVX2
    static void multiply_vectors_f32(const float* a, const float* b, float* result, std::size_t count) {
        std::size_t simd_count = count & ~7;
        
        for (std::size_t i = 0; i < simd_count; i += 8) {
            __m256 va = _mm256_load_ps(&a[i]);
            __m256 vb = _mm256_load_ps(&b[i]);
            __m256 vr = _mm256_mul_ps(va, vb);
            _mm256_store_ps(&result[i], vr);
        }
        
        for (std::size_t i = simd_count; i < count; ++i) {
            result[i] = a[i] * b[i];
        }
    }
    
    // Dot product using AVX2
    static float dot_product_f32(const float* a, const float* b, std::size_t count) {
        __m256 sum = _mm256_setzero_ps();
        std::size_t simd_count = count & ~7;
        
        for (std::size_t i = 0; i < simd_count; i += 8) {
            __m256 va = _mm256_load_ps(&a[i]);
            __m256 vb = _mm256_load_ps(&b[i]);
            __m256 prod = _mm256_mul_ps(va, vb);
            sum = _mm256_add_ps(sum, prod);
        }
        
        // Horizontal sum of AVX register
        __m128 high = _mm256_extractf128_ps(sum, 1);
        __m128 low = _mm256_castps256_ps128(sum);
        __m128 sum128 = _mm_add_ps(high, low);
        
        sum128 = _mm_hadd_ps(sum128, sum128);
        sum128 = _mm_hadd_ps(sum128, sum128);
        
        float result = _mm_cvtss_f32(sum128);
        
        // Handle remaining elements
        for (std::size_t i = simd_count; i < count; ++i) {
            result += a[i] * b[i];
        }
        
        return result;
    }
    
    // Check CPU capabilities
    static bool has_avx2_support() {
        int cpu_info[4];
        __cpuid(cpu_info, 7);
        return (cpu_info[1] & (1 << 5)) != 0; // Check AVX2 bit
    }
    
    static bool has_sse42_support() {
        int cpu_info[4];
        __cpuid(cpu_info, 1);
        return (cpu_info[2] & (1 << 20)) != 0; // Check SSE4.2 bit
    }
};

// Prefetching hints for improved cache performance
class PrefetchHints {
public:
    // Prefetch for read
    template<typename T>
    static void prefetch_read(const T* addr) {
        _mm_prefetch(reinterpret_cast<const char*>(addr), _MM_HINT_T0);
    }
    
    // Prefetch for write
    template<typename T>
    static void prefetch_write(T* addr) {
        _mm_prefetch(reinterpret_cast<const char*>(addr), _MM_HINT_T0);
    }
    
    // Prefetch with temporal locality hint
    template<typename T>
    static void prefetch_temporal(const T* addr) {
        _mm_prefetch(reinterpret_cast<const char*>(addr), _MM_HINT_T1);
    }
    
    // Prefetch with non-temporal hint (won't pollute cache)
    template<typename T>
    static void prefetch_non_temporal(const T* addr) {
        _mm_prefetch(reinterpret_cast<const char*>(addr), _MM_HINT_NTA);
    }
};

// Cache-aware algorithms
template<typename T>
class CacheAwareAlgorithms {
public:
    // Cache-friendly matrix multiplication
    static void matrix_multiply(const T* a, const T* b, T* c, std::size_t n) {
        constexpr std::size_t BLOCK_SIZE = 64; // Tuned for L1 cache
        
        for (std::size_t i = 0; i < n; i += BLOCK_SIZE) {
            for (std::size_t j = 0; j < n; j += BLOCK_SIZE) {
                for (std::size_t k = 0; k < n; k += BLOCK_SIZE) {
                    
                    // Process blocks
                    std::size_t max_i = std::min(i + BLOCK_SIZE, n);
                    std::size_t max_j = std::min(j + BLOCK_SIZE, n);
                    std::size_t max_k = std::min(k + BLOCK_SIZE, n);
                    
                    for (std::size_t ii = i; ii < max_i; ++ii) {
                        for (std::size_t jj = j; jj < max_j; ++jj) {
                            
                            // Prefetch next cache line
                            if (jj + 8 < max_j) {
                                PrefetchHints::prefetch_read(&b[(k * n) + jj + 8]);
                            }
                            
                            T sum = T{};
                            for (std::size_t kk = k; kk < max_k; ++kk) {
                                sum += a[ii * n + kk] * b[kk * n + jj];
                            }
                            c[ii * n + jj] += sum;
                        }
                    }
                }
            }
        }
    }
    
    // Cache-friendly parallel reduction
    template<typename Op>
    static T parallel_reduce(const T* data, std::size_t count, T init, Op operation) {
        constexpr std::size_t CACHE_BLOCK_SIZE = 1024; // Elements per cache block
        std::vector<T> partial_results;
        
        // Process in cache-friendly blocks
        for (std::size_t i = 0; i < count; i += CACHE_BLOCK_SIZE) {
            std::size_t block_end = std::min(i + CACHE_BLOCK_SIZE, count);
            T block_result = init;
            
            // Prefetch next block
            if (block_end < count) {
                PrefetchHints::prefetch_read(&data[block_end]);
            }
            
            for (std::size_t j = i; j < block_end; ++j) {
                block_result = operation(block_result, data[j]);
            }
            
            partial_results.push_back(block_result);
        }
        
        // Combine partial results
        T final_result = init;
        for (const T& partial : partial_results) {
            final_result = operation(final_result, partial);
        }
        
        return final_result;
    }
};

// Branch prediction hints
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// Performance monitoring and optimization suggestions
class PerformanceAnalyzer {
private:
    struct PerformanceCounters {
        std::atomic<std::uint64_t> cache_misses{0};
        std::atomic<std::uint64_t> branch_mispredictions{0};
        std::atomic<std::uint64_t> simd_operations{0};
        std::atomic<std::uint64_t> scalar_operations{0};
    };
    
    static PerformanceCounters& get_counters() {
        static PerformanceCounters counters;
        return counters;
    }
    
public:
    static void record_cache_miss() {
        get_counters().cache_misses++;
    }
    
    static void record_branch_misprediction() {
        get_counters().branch_mispredictions++;
    }
    
    static void record_simd_operation() {
        get_counters().simd_operations++;
    }
    
    static void record_scalar_operation() {
        get_counters().scalar_operations++;
    }
    
    struct PerformanceReport {
        std::uint64_t cache_misses;
        std::uint64_t branch_mispredictions;
        std::uint64_t simd_operations;
        std::uint64_t scalar_operations;
        double simd_utilization;
        std::vector<std::string> optimization_suggestions;
    };
    
    static PerformanceReport generate_report() {
        auto& counters = get_counters();
        
        PerformanceReport report;
        report.cache_misses = counters.cache_misses.load();
        report.branch_mispredictions = counters.branch_mispredictions.load();
        report.simd_operations = counters.simd_operations.load();
        report.scalar_operations = counters.scalar_operations.load();
        
        std::uint64_t total_ops = report.simd_operations + report.scalar_operations;
        report.simd_utilization = total_ops > 0 ? 
            static_cast<double>(report.simd_operations) / total_ops : 0.0;
        
        // Generate optimization suggestions
        if (report.cache_misses > 1000) {
            report.optimization_suggestions.push_back("Consider using cache-friendly data layouts");
            report.optimization_suggestions.push_back("Add prefetch hints for predictable access patterns");
        }
        
        if (report.simd_utilization < 0.3 && total_ops > 100) {
            report.optimization_suggestions.push_back("Vectorize loops using SIMD operations");
            report.optimization_suggestions.push_back("Use aligned memory allocation for better SIMD performance");
        }
        
        if (report.branch_mispredictions > 500) {
            report.optimization_suggestions.push_back("Add branch prediction hints");
            report.optimization_suggestions.push_back("Consider branchless algorithms");
        }
        
        return report;
    }
    
    static void reset_counters() {
        auto& counters = get_counters();
        counters.cache_misses = 0;
        counters.branch_mispredictions = 0;
        counters.simd_operations = 0;
        counters.scalar_operations = 0;
    }
};

} // namespace HerLang