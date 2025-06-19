/**
 * @file thread_pool.hpp
 * @brief A simple thread pool implementation in C++.
 * This file provides a ThreadPool class that allows you to submit tasks
 * to be executed by a pool of worker threads. The tasks can return results
 * using std::future and std::promise.
 */
#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#include <future>
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <type_traits>

/**
 * @brief A simple thread pool class that manages a pool of worker threads.
 * This class allows you to submit tasks that can be executed asynchronously.
 * The tasks can return results using std::future and std::promise.
 */
class ThreadPool
{
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    bool stop_ = false;
    std::condition_variable status_update_;

    void workerThread();
public:
    /**
     * @brief Constructs a ThreadPool with a specified number of worker threads.
     * @param num_threads The number of threads in the pool. Default is 1.
     * This constructor initializes the worker threads.
     */
     ThreadPool(int num_threads = 1);
    /**
     * @brief Destroys the ThreadPool and joins all worker threads.
     * This destructor ensures that all tasks are completed before the pool is destroyed.
     */
     ~ThreadPool();
    /**
     * @brief Submits a task to the thread pool.
     * @tparam F The type of the function to be executed.
     * @tparam Args The types of the arguments to be passed to the function.
     * @param f The function to be executed.
     * @param args The arguments to be passed to the function.
     * @return A std::future that will hold the result of the task.
     * This method allows you to submit a task to the thread pool and get a future
     * that can be used to retrieve the result of the task once it is completed.
     */
      template <typename F, typename... Args>
       std::future<typename  std::invoke_result<F,Args...>::type> submit(F&& f, Args&&... args);
}; 

ThreadPool::ThreadPool(int num_threads)
{
    for (int i = 0; i < num_threads; ++i) {
        workers_.emplace_back(std::thread(&ThreadPool::workerThread, this));
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    status_update_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

}

template <typename F, typename... Args>
std::future<typename  std::invoke_result<F,Args...>::type> ThreadPool::submit(F&& f, Args&&... args)
{
    using return_type = typename  std::invoke_result<F,Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> result = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        tasks_.emplace([task]() {(*task)(); });
    }
    status_update_.notify_one();
    return result;

}


void ThreadPool::workerThread()
{
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            status_update_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

#endif // THREAD_POOL_HPP