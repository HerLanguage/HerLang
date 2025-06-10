// cooperative_threading.hpp - Cooperative threading system for HerLang
#pragma once
#include <thread>
#include <queue>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <memory>
#include <future>
#include <shared_mutex>
#include "error_system.hpp"

namespace HerLang {

// Worker wellness monitoring and management
class WorkerWellness {
public:
    struct WellnessMetrics {
        std::chrono::steady_clock::time_point last_break;
        std::atomic<int> consecutive_tasks{0};
        std::atomic<float> stress_level{0.0f};
        std::chrono::steady_clock::time_point start_time;
        std::atomic<size_t> total_tasks_completed{0};
        
        WellnessMetrics() : last_break(std::chrono::steady_clock::now()), 
                           start_time(std::chrono::steady_clock::now()) {}
    };
    
private:
    WellnessMetrics metrics;
    static constexpr int MAX_CONSECUTIVE_TASKS = 50;
    static constexpr auto MAX_WORK_DURATION = std::chrono::hours(2);
    static constexpr float MAX_STRESS_LEVEL = 0.8f;
    static constexpr auto BREAK_DURATION = std::chrono::minutes(15);
    
public:
    bool needs_mandatory_break() const {
        auto now = std::chrono::steady_clock::now();
        auto work_duration = now - metrics.last_break;
        
        return metrics.consecutive_tasks >= MAX_CONSECUTIVE_TASKS ||
               work_duration >= MAX_WORK_DURATION ||
               metrics.stress_level >= MAX_STRESS_LEVEL;
    }
    
    void take_wellness_break() {
        std::this_thread::sleep_for(BREAK_DURATION);
        metrics.stress_level = metrics.stress_level * 0.5f; // Reduce stress
        metrics.consecutive_tasks = 0;
        metrics.last_break = std::chrono::steady_clock::now();
    }
    
    void record_task_completion() {
        metrics.consecutive_tasks++;
        metrics.total_tasks_completed++;
        
        // Increase stress based on task frequency
        auto now = std::chrono::steady_clock::now();
        auto time_since_last = now - metrics.last_break;
        if (time_since_last < std::chrono::minutes(1)) {
            metrics.stress_level = std::min(1.0f, metrics.stress_level + 0.1f);
        } else {
            metrics.stress_level = std::max(0.0f, metrics.stress_level - 0.05f);
        }
    }
    
    float get_stress_level() const { return metrics.stress_level; }
    int get_consecutive_tasks() const { return metrics.consecutive_tasks; }
    size_t get_total_tasks() const { return metrics.total_tasks_completed; }
    
    std::chrono::milliseconds get_work_duration() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - metrics.start_time);
    }
};

// Individual worker in the cooperative thread pool
class CooperativeWorker {
private:
    std::unique_ptr<std::thread> worker_thread;
    std::queue<std::function<void()>> task_queue;
    std::mutex task_mutex;
    std::condition_variable task_cv;
    std::atomic<bool> should_stop{false};
    WorkerWellness wellness;
    size_t worker_id;
    
public:
    explicit CooperativeWorker(size_t id) : worker_id(id) {
        worker_thread = std::make_unique<std::thread>(&CooperativeWorker::work_loop, this);
    }
    
    ~CooperativeWorker() {
        should_stop = true;
        task_cv.notify_all();
        if (worker_thread && worker_thread->joinable()) {
            worker_thread->join();
        }
    }
    
    // Non-copyable, non-movable for safety
    CooperativeWorker(const CooperativeWorker&) = delete;
    CooperativeWorker& operator=(const CooperativeWorker&) = delete;
    CooperativeWorker(CooperativeWorker&&) = delete;
    CooperativeWorker& operator=(CooperativeWorker&&) = delete;
    
    bool try_assign_task(std::function<void()> task) {
        // Check if worker needs a break first
        if (wellness.needs_mandatory_break()) {
            return false; // Cannot assign task, worker needs rest
        }
        
        {
            std::lock_guard<std::mutex> lock(task_mutex);
            task_queue.push(std::move(task));
        }
        task_cv.notify_one();
        return true;
    }
    
    void force_wellness_break() {
        // Signal worker to take a break at next opportunity
        {
            std::lock_guard<std::mutex> lock(task_mutex);
            task_queue.push([this]() { wellness.take_wellness_break(); });
        }
        task_cv.notify_one();
    }
    
    float get_stress_level() const { return wellness.get_stress_level(); }
    size_t get_queue_size() const {
        std::lock_guard<std::mutex> lock(task_mutex);
        return task_queue.size();
    }
    
    WorkerWellness::WellnessMetrics get_wellness_stats() const {
        return {wellness.get_stress_level(), wellness.get_consecutive_tasks(), 
                wellness.get_total_tasks(), wellness.get_work_duration()};
    }
    
private:
    void work_loop() {
        while (!should_stop) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(task_mutex);
                task_cv.wait(lock, [this] { return !task_queue.empty() || should_stop; });
                
                if (should_stop) break;
                
                if (!task_queue.empty()) {
                    task = std::move(task_queue.front());
                    task_queue.pop();
                }
            }
            
            if (task) {
                // Check wellness before executing
                if (wellness.needs_mandatory_break()) {
                    wellness.take_wellness_break();
                }
                
                try {
                    task();
                    wellness.record_task_completion();
                } catch (const std::exception& e) {
                    // Log error but continue working
                    // In a real implementation, this would use a proper logging system
                }
            }
        }
    }
};

// Cooperative thread pool with worker wellness management
class CooperativeThreadPool {
private:
    std::vector<std::unique_ptr<CooperativeWorker>> workers;
    std::atomic<size_t> round_robin_counter{0};
    mutable std::shared_mutex pool_mutex;
    
    // Pool configuration
    static constexpr size_t DEFAULT_WORKER_COUNT = std::thread::hardware_concurrency();
    static constexpr float STRESS_THRESHOLD = 0.6f;
    
public:
    explicit CooperativeThreadPool(size_t worker_count = DEFAULT_WORKER_COUNT) {
        workers.reserve(worker_count);
        for (size_t i = 0; i < worker_count; ++i) {
            workers.push_back(std::make_unique<CooperativeWorker>(i));
        }
    }
    
    ~CooperativeThreadPool() = default;
    
    // Submit task with wellness consideration
    template<typename F, typename... Args>
    auto submit_with_care(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        // Find the least stressed available worker
        auto worker = find_optimal_worker();
        if (!worker) {
            throw HerLangError(HerLangError::Type::RuntimeError,
                "All workers are overwhelmed and need rest")
                .with_suggestion("Reduce task submission rate")
                .with_suggestion("Wait for workers to complete their wellness breaks")
                .with_context("Thread pool task submission");
        }
        
        bool assigned = worker->try_assign_task([task]() { (*task)(); });
        if (!assigned) {
            throw HerLangError(HerLangError::Type::RuntimeError,
                "Could not assign task - worker needs wellness break")
                .with_suggestion("Retry after a short delay")
                .with_context("Worker wellness protection");
        }
        
        return result;
    }
    
    // Get pool statistics
    struct PoolStats {
        size_t worker_count;
        float average_stress;
        size_t total_tasks_completed;
        size_t workers_on_break;
        size_t total_queue_size;
    };
    
    PoolStats get_pool_stats() const {
        std::shared_lock<std::shared_mutex> lock(pool_mutex);
        
        PoolStats stats{};
        stats.worker_count = workers.size();
        
        float total_stress = 0;
        size_t total_tasks = 0;
        size_t total_queue = 0;
        size_t high_stress_workers = 0;
        
        for (const auto& worker : workers) {
            float stress = worker->get_stress_level();
            total_stress += stress;
            total_queue += worker->get_queue_size();
            
            auto wellness = worker->get_wellness_stats();
            total_tasks += wellness.total_tasks_completed;
            
            if (stress > STRESS_THRESHOLD) {
                high_stress_workers++;
            }
        }
        
        stats.average_stress = total_stress / workers.size();
        stats.total_tasks_completed = total_tasks;
        stats.workers_on_break = high_stress_workers;
        stats.total_queue_size = total_queue;
        
        return stats;
    }
    
    // Force wellness breaks for overstressed workers
    void ensure_worker_wellness() {
        std::shared_lock<std::shared_mutex> lock(pool_mutex);
        
        for (auto& worker : workers) {
            if (worker->get_stress_level() > STRESS_THRESHOLD) {
                worker->force_wellness_break();
            }
        }
    }
    
private:
    CooperativeWorker* find_optimal_worker() {
        std::shared_lock<std::shared_mutex> lock(pool_mutex);
        
        CooperativeWorker* best_worker = nullptr;
        float lowest_stress = 1.0f;
        
        // Find worker with lowest stress level
        for (auto& worker : workers) {
            float stress = worker->get_stress_level();
            if (stress < lowest_stress && stress < STRESS_THRESHOLD) {
                lowest_stress = stress;
                best_worker = worker.get();
            }
        }
        
        // If no low-stress worker found, use round-robin as fallback
        if (!best_worker && !workers.empty()) {
            size_t index = round_robin_counter++ % workers.size();
            best_worker = workers[index].get();
        }
        
        return best_worker;
    }
};

// High-level async operations with wellness consideration
class WellnessAwareAsync {
private:
    static CooperativeThreadPool& get_global_pool() {
        static CooperativeThreadPool pool;
        return pool;
    }
    
public:
    template<typename F, typename... Args>
    static auto async(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        return get_global_pool().submit_with_care(std::forward<F>(f), std::forward<Args>(args)...);
    }
    
    static CooperativeThreadPool::PoolStats get_system_stats() {
        return get_global_pool().get_pool_stats();
    }
    
    static void ensure_system_wellness() {
        get_global_pool().ensure_worker_wellness();
    }
};

} // namespace HerLang