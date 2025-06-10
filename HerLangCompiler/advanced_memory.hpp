// advanced_memory.hpp - Advanced memory safety for HerLang
#pragma once
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <string>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "error_system.hpp"

namespace HerLang {

// Memory block metadata for comprehensive tracking
struct SafeMemoryBlock {
    void* data;
    size_t size;
    size_t alignment;
    std::chrono::steady_clock::time_point allocated_time;
    std::string allocation_context;
    std::atomic<int> ref_count{1};
    bool is_protected = true;
    
    bool check_access(size_t offset, size_t access_size) const {
        return offset + access_size <= size;
    }
};

// Boundary-guarded pointer with comprehensive safety checks
template<typename T>
class BoundaryGuardedPtr {
private:
    T* ptr;
    std::shared_ptr<SafeMemoryBlock> block_info;
    
public:
    BoundaryGuardedPtr(T* p, std::shared_ptr<SafeMemoryBlock> block)
        : ptr(p), block_info(block) {}
    
    // Safe array access with bounds checking
    T& operator[](size_t index) {
        if (!block_info || !block_info->check_access(index * sizeof(T), sizeof(T))) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Array access out of bounds")
                .with_suggestion("Check array size before accessing")
                .with_suggestion("Use safe_at() method for bounds-checked access")
                .with_context("Boundary-guarded pointer access");
        }
        return ptr[index];
    }
    
    const T& operator[](size_t index) const {
        if (!block_info || !block_info->check_access(index * sizeof(T), sizeof(T))) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Array access out of bounds")
                .with_suggestion("Check array size before accessing")
                .with_suggestion("Use safe_at() method for bounds-checked access")
                .with_context("Boundary-guarded pointer access");
        }
        return ptr[index];
    }
    
    // Safe access with optional return
    std::optional<T> safe_at(size_t index) const {
        if (!block_info || !block_info->check_access(index * sizeof(T), sizeof(T))) {
            return std::nullopt;
        }
        return ptr[index];
    }
    
    // Get raw pointer (use with caution)
    T* get() const { return ptr; }
    
    // Get block size in elements
    size_t size() const {
        return block_info ? block_info->size / sizeof(T) : 0;
    }
    
    // Disable dangerous pointer arithmetic
    BoundaryGuardedPtr operator+(ptrdiff_t) = delete;
    BoundaryGuardedPtr operator-(ptrdiff_t) = delete;
    BoundaryGuardedPtr& operator+=(ptrdiff_t) = delete;
    BoundaryGuardedPtr& operator-=(ptrdiff_t) = delete;
    BoundaryGuardedPtr& operator++() = delete;
    BoundaryGuardedPtr& operator--() = delete;
    BoundaryGuardedPtr operator++(int) = delete;
    BoundaryGuardedPtr operator--(int) = delete;
};

// Advanced memory manager with comprehensive safety
class AdvancedMemoryManager {
private:
    std::unordered_map<void*, std::shared_ptr<SafeMemoryBlock>> protected_blocks;
    mutable std::shared_mutex guard_mutex;
    static constexpr size_t MAX_ALLOCATION = 1ULL * 1024 * 1024 * 1024; // 1GB
    static constexpr size_t ALIGNMENT = alignof(std::max_align_t);
    
public:
    // Allocate protected memory
    template<typename T>
    BoundaryGuardedPtr<T> allocate(size_t count, const std::string& context = "") {
        size_t total_size = count * sizeof(T);
        
        if (total_size > MAX_ALLOCATION) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Allocation size exceeds safety limit")
                .with_suggestion("Reduce allocation size")
                .with_suggestion("Use streaming or chunked processing")
                .with_context("Memory allocation safety check");
        }
        
        void* raw_ptr = std::aligned_alloc(ALIGNMENT, total_size);
        if (!raw_ptr) {
            throw HerLangError(HerLangError::Type::MemoryError,
                "Memory allocation failed")
                .with_suggestion("Reduce memory usage")
                .with_suggestion("Check system memory availability")
                .with_context("Memory allocation");
        }
        
        auto block = std::make_shared<SafeMemoryBlock>();
        block->data = raw_ptr;
        block->size = total_size;
        block->alignment = ALIGNMENT;
        block->allocated_time = std::chrono::steady_clock::now();
        block->allocation_context = context;
        
        {
            std::unique_lock<std::shared_mutex> lock(guard_mutex);
            protected_blocks[raw_ptr] = block;
        }
        
        return BoundaryGuardedPtr<T>(static_cast<T*>(raw_ptr), block);
    }
    
    // Safe deallocation
    void deallocate(void* ptr) {
        std::unique_lock<std::shared_mutex> lock(guard_mutex);
        auto it = protected_blocks.find(ptr);
        if (it != protected_blocks.end()) {
            std::free(ptr);
            protected_blocks.erase(it);
        }
    }
    
    // Get allocation info
    std::optional<SafeMemoryBlock> get_block_info(void* ptr) const {
        std::shared_lock<std::shared_mutex> lock(guard_mutex);
        auto it = protected_blocks.find(ptr);
        return it != protected_blocks.end() ? std::optional<SafeMemoryBlock>(*it->second) : std::nullopt;
    }
    
    // Memory usage statistics
    struct MemoryStats {
        size_t total_allocated;
        size_t block_count;
        size_t largest_block;
        std::chrono::milliseconds oldest_allocation;
    };
    
    MemoryStats get_stats() const {
        std::shared_lock<std::shared_mutex> lock(guard_mutex);
        MemoryStats stats{0, 0, 0, std::chrono::milliseconds(0)};
        
        auto now = std::chrono::steady_clock::now();
        stats.block_count = protected_blocks.size();
        
        for (const auto& [ptr, block] : protected_blocks) {
            stats.total_allocated += block->size;
            stats.largest_block = std::max(stats.largest_block, block->size);
            
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - block->allocated_time);
            stats.oldest_allocation = std::max(stats.oldest_allocation, age);
        }
        
        return stats;
    }
};

// Incremental garbage collector
class GentleGarbageCollector {
private:
    AdvancedMemoryManager& memory_manager;
    std::thread gc_thread;
    std::atomic<bool> should_run{true};
    std::atomic<float> system_load{0.0f};
    std::condition_variable gc_cv;
    std::mutex gc_mutex;
    
    // Configuration
    static constexpr float MAX_SYSTEM_LOAD = 0.7f;
    static constexpr size_t MAX_CLEANUP_PER_CYCLE = 10;
    static constexpr auto CLEANUP_GRACE_PERIOD = std::chrono::minutes(5);
    static constexpr auto GC_CYCLE_INTERVAL = std::chrono::milliseconds(100);
    
public:
    explicit GentleGarbageCollector(AdvancedMemoryManager& mm) 
        : memory_manager(mm), gc_thread(&GentleGarbageCollector::gc_loop, this) {}
    
    ~GentleGarbageCollector() {
        should_run = false;
        gc_cv.notify_all();
        if (gc_thread.joinable()) {
            gc_thread.join();
        }
    }
    
    void update_system_load(float load) {
        system_load = load;
    }
    
    void request_cleanup() {
        gc_cv.notify_one();
    }
    
private:
    void gc_loop() {
        while (should_run) {
            std::unique_lock<std::mutex> lock(gc_mutex);
            gc_cv.wait_for(lock, GC_CYCLE_INTERVAL);
            
            if (!should_run) break;
            
            // Only run during low system load
            if (system_load < MAX_SYSTEM_LOAD) {
                perform_incremental_cleanup();
            }
        }
    }
    
    void perform_incremental_cleanup() {
        auto stats = memory_manager.get_stats();
        
        // If memory usage is high, be more aggressive
        size_t max_cleanup = MAX_CLEANUP_PER_CYCLE;
        if (stats.total_allocated > 100 * 1024 * 1024) { // > 100MB
            max_cleanup *= 2;
        }
        
        // Note: In a real implementation, we would need access to the
        // protected_blocks to perform actual cleanup. This would require
        // friendship or additional interfaces in AdvancedMemoryManager.
        
        // For now, we just simulate the cleanup logic
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
};

} // namespace HerLang