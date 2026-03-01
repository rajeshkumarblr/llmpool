#ifndef LLMPOOL_H
#define LLMPOOL_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class TaskQueue {
private:
  std::queue<std::function<void()>> tasks_;
  std::mutex mtx_;
  std::condition_variable cv_;
  bool shutting_down_ = false;

public:
  void push(std::function<void()> task) {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      tasks_.push(std::move(task));
    }
    cv_.notify_one();
  }

  bool pop(std::function<void()> &task) {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return !tasks_.empty() || shutting_down_; });

    if (shutting_down_ && tasks_.empty())
      return false;

    task = std::move(tasks_.front());
    tasks_.pop();
    return true;
  }

  void shutdown() {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      shutting_down_ = true;
    }
    cv_.notify_all();
  }
};

class LLMThreadPool {
private:
  std::vector<std::thread> workers_;
  TaskQueue queue_;

public:
  explicit LLMThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
      workers_.emplace_back([this, i] {
        std::function<void()> task;
        while (queue_.pop(task)) {
          // The main thread now handles the console output via the future,
          // so the worker just silently executes the heavy lifting.
          task();
        }
      });
    }
  }

  ~LLMThreadPool() {
    queue_.shutdown();
    for (auto &worker : workers_) {
      if (worker.joinable())
        worker.join();
    }
  }

  // The Principal-level Thread Pool paradigm:
  // Using variadic templates and packaged_task to return a std::future
  template <class F, class... Args>
  auto submit(F &&f, Args &&...args)
      -> std::future<typename std::invoke_result<F, Args...>::type> {

    using return_type = typename std::invoke_result<F, Args...>::type;

    // Wrap the function in a packaged_task to wire up the future state
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();

    // We push a disposable lambda that simply executes the packaged task
    queue_.push([task]() { (*task)(); });

    return res;
  }
};

#endif