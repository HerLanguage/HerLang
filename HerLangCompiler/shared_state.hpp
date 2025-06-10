// shared_state.hpp - Protected shared state management for HerLang
#pragma once
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <functional>
#include <chrono>
#include <thread>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include "error_system.hpp"

namespace HerLang {

// Protected shared state with reader-writer synchronization
template<typename T>
class ProtectedSharedState {
private:
    T data;
    mutable std::shared_mutex protection_mutex;
    std::string state_name;
    std::atomic<size_t> read_count{0};
    std::atomic<size_t> write_count{0};
    std::chrono::steady_clock::time_point creation_time;
    
public:
    explicit ProtectedSharedState(T initial_data, const std::string& name = "shared_state")
        : data(std::move(initial_data)), state_name(name), 
          creation_time(std::chrono::steady_clock::now()) {}
    
    // Safe read access - allows multiple concurrent readers
    template<typename F>
    auto safe_read(F&& reader) const -> decltype(reader(data)) {
        std::shared_lock<std::shared_mutex> lock(protection_mutex);
        read_count++;
        
        try {
            return reader(data);
        } catch (...) {
            read_count--;
            throw;
        }
        
        read_count--;
    }
    
    // Safe write access - ensures exclusive access
    template<typename F>
    auto safe_write(F&& writer) -> decltype(writer(data)) {
        std::unique_lock<std::shared_mutex> lock(protection_mutex);
        write_count++;
        
        try {
            auto result = writer(data);
            write_count--;
            return result;
        } catch (...) {
            write_count--;
            throw;
        }
    }
    
    // Optimistic update - tries to update without blocking reads
    template<typename F>
    bool optimistic_update(F&& updater, int max_retries = 3) {
        for (int attempt = 0; attempt < max_retries; ++attempt) {
            T local_copy;
            
            // Read current state
            {
                std::shared_lock<std::shared_mutex> read_lock(protection_mutex);
                local_copy = data;
            }
            
            // Apply update to local copy
            T updated = updater(local_copy);
            
            // Try to commit the update
            {
                std::unique_lock<std::shared_mutex> write_lock(protection_mutex);
                
                // Check if data hasn't changed since we read it
                if (data == local_copy) {
                    data = std::move(updated);
                    write_count++;
                    return true;
                }
            }
            
            // Data changed, retry with small delay
            if (attempt < max_retries - 1) {
                std::this_thread::sleep_for(std::chrono::microseconds(100 * (attempt + 1)));
            }
        }
        
        return false; // Update failed after retries
    }
    
    // Try to update with timeout
    template<typename F>
    bool try_update_for(F&& updater, std::chrono::milliseconds timeout) {
        std::unique_lock<std::shared_mutex> lock(protection_mutex, timeout);
        
        if (!lock.owns_lock()) {
            return false; // Timeout occurred
        }
        
        data = updater(data);
        write_count++;
        return true;
    }
    
    // Get a copy of the current state
    T get_copy() const {
        return safe_read([](const T& d) { return d; });
    }
    
    // Set new value
    void set(T new_value) {
        safe_write([&new_value](T& d) { d = std::move(new_value); });
    }
    
    // Get access statistics
    struct AccessStats {
        std::string name;
        size_t total_reads;
        size_t total_writes;
        std::chrono::milliseconds lifetime;
        size_t current_readers;
    };
    
    AccessStats get_stats() const {
        auto now = std::chrono::steady_clock::now();
        auto lifetime = std::chrono::duration_cast<std::chrono::milliseconds>(now - creation_time);
        
        return AccessStats{
            state_name,
            read_count.load(),
            write_count.load(),
            lifetime,
            0 // Current readers would need more complex tracking
        };
    }
};

// Deadlock detection and prevention system
class DeadlockPrevention {
private:
    struct LockInfo {
        std::string lock_name;
        std::thread::id owner;
        std::chrono::steady_clock::time_point acquired_time;
    };
    
    std::map<std::thread::id, std::vector<std::string>> held_locks;
    std::map<std::string, LockInfo> lock_registry;
    std::mutex registry_mutex;
    
    // Lock ordering to prevent circular dependencies
    std::map<std::string, int> lock_hierarchy;
    int next_hierarchy_level = 0;
    
public:
    // Register a new lock type with hierarchy level
    void register_lock_type(const std::string& lock_name) {
        std::lock_guard<std::mutex> lock(registry_mutex);
        if (lock_hierarchy.find(lock_name) == lock_hierarchy.end()) {
            lock_hierarchy[lock_name] = next_hierarchy_level++;
        }
    }
    
    // Check if acquiring a lock would create a deadlock
    bool can_acquire_safely(const std::string& lock_name) {
        std::lock_guard<std::mutex> lock(registry_mutex);
        auto tid = std::this_thread::get_id();
        
        // Check hierarchy ordering
        auto it = lock_hierarchy.find(lock_name);
        if (it == lock_hierarchy.end()) {
            register_lock_type(lock_name);
            return true; // New lock type, safe to acquire
        }
        
        int target_level = it->second;
        
        // Check if this thread already holds locks with higher hierarchy levels
        auto held_it = held_locks.find(tid);
        if (held_it != held_locks.end()) {
            for (const auto& held_lock : held_it->second) {
                auto held_it_hier = lock_hierarchy.find(held_lock);
                if (held_it_hier != lock_hierarchy.end() && held_it_hier->second > target_level) {
                    return false; // Would violate lock hierarchy
                }
            }
        }
        
        return !would_create_cycle(tid, lock_name);
    }
    
    // Register successful lock acquisition
    void register_lock_acquisition(const std::string& lock_name) {
        std::lock_guard<std::mutex> lock(registry_mutex);
        auto tid = std::this_thread::get_id();
        
        held_locks[tid].push_back(lock_name);
        lock_registry[lock_name] = LockInfo{
            lock_name, tid, std::chrono::steady_clock::now()
        };
    }
    
    // Register lock release
    void register_lock_release(const std::string& lock_name) {
        std::lock_guard<std::mutex> lock(registry_mutex);
        auto tid = std::this_thread::get_id();
        
        auto& locks = held_locks[tid];
        locks.erase(std::remove(locks.begin(), locks.end(), lock_name), locks.end());
        
        lock_registry.erase(lock_name);
    }
    
    // Get deadlock detection report
    struct DeadlockReport {
        bool potential_deadlock_detected;
        std::vector<std::string> involved_locks;
        std::vector<std::thread::id> involved_threads;
        std::string description;
    };
    
    DeadlockReport analyze_potential_deadlocks() {
        std::lock_guard<std::mutex> lock(registry_mutex);
        
        DeadlockReport report;
        report.potential_deadlock_detected = false;
        
        // Simple cycle detection in lock dependency graph
        for (const auto& [tid, locks] : held_locks) {
            if (locks.size() > 1) {
                // Check for hierarchy violations
                for (size_t i = 0; i < locks.size() - 1; ++i) {
                    auto level1 = lock_hierarchy[locks[i]];
                    auto level2 = lock_hierarchy[locks[i + 1]];
                    
                    if (level1 > level2) {
                        report.potential_deadlock_detected = true;
                        report.involved_locks = locks;
                        report.involved_threads.push_back(tid);
                        report.description = "Lock hierarchy violation detected";
                        break;
                    }
                }
            }
        }
        
        return report;
    }
    
private:
    bool would_create_cycle(std::thread::id tid, const std::string& target_lock) {
        // Simple implementation - in production this would be more sophisticated
        auto it = lock_registry.find(target_lock);
        if (it != lock_registry.end() && it->second.owner != tid) {
            // Lock is held by another thread
            auto other_locks = held_locks.find(it->second.owner);
            if (other_locks != held_locks.end()) {
                // Check if the other thread is waiting for any locks we hold
                auto our_locks = held_locks.find(tid);
                if (our_locks != held_locks.end()) {
                    for (const auto& our_lock : our_locks->second) {
                        for (const auto& other_lock : other_locks->second) {
                            if (our_lock == other_lock) {
                                return true; // Potential cycle
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
};

// Scoped lock guard with deadlock prevention
template<typename MutexType>
class SafeLockGuard {
private:
    std::unique_lock<MutexType> lock;
    std::string lock_name;
    static DeadlockPrevention& get_deadlock_prevention() {
        static DeadlockPrevention instance;
        return instance;
    }
    
public:
    explicit SafeLockGuard(MutexType& mutex, const std::string& name)
        : lock_name(name) {
        
        auto& prevention = get_deadlock_prevention();
        
        if (!prevention.can_acquire_safely(lock_name)) {
            throw HerLangError(HerLangError::Type::RuntimeError,
                "Potential deadlock detected for lock: " + lock_name)
                .with_suggestion("Review lock acquisition order")
                .with_suggestion("Consider using try_lock with timeout")
                .with_context("Deadlock prevention system");
        }
        
        lock = std::unique_lock<MutexType>(mutex);
        prevention.register_lock_acquisition(lock_name);
    }
    
    ~SafeLockGuard() {
        if (lock.owns_lock()) {
            get_deadlock_prevention().register_lock_release(lock_name);
        }
    }
    
    // Non-copyable, non-movable
    SafeLockGuard(const SafeLockGuard&) = delete;
    SafeLockGuard& operator=(const SafeLockGuard&) = delete;
    SafeLockGuard(SafeLockGuard&&) = delete;
    SafeLockGuard& operator=(SafeLockGuard&&) = delete;
};

} // namespace HerLang